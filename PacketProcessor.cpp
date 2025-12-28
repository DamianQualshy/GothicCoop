namespace GOTHIC_ENGINE {
    void ProcessCoopPacket(json e, ENetEvent packet) {
        if (!e.contains("id") || !e["id"].is_string() || !e.contains("type") || !e["type"].is_number_integer()) {
            ChatLog("Invalid packet received (missing id/type).");
            return;
        }

        auto id = e["id"].get<std::string>();
        auto type = e["type"].get<int>();
        RemoteNpc* npcToSync = NULL;
        auto syncIt = SyncNpcs.find(id.c_str());
        if (syncIt != SyncNpcs.end()) {
            npcToSync = syncIt->second;
        }

        if (type == SYNC_PLAYER_NAME) {
            if (!e.contains("connectId") || !e["connectId"].is_number_integer() || !e.contains("name") || !e["name"].is_string()) {
                ChatLog("Invalid SYNC_PLAYER_NAME packet.");
                return;
            }

            auto connectId = e["connectId"].get<enet_uint32>();

            if (connectId == packet.peer->connectID) {
                std::string name = e["name"].get<std::string>();
                MyselfId = name.c_str();
            }

            return;
        }

        if (type == INIT_NPC) {
            auto peerData = (PeerData*)packet.peer->data;
            if (peerData && e.contains("nickname")) {
                if (!e["nickname"].is_string()) {
                    ChatLog("Invalid INIT_NPC packet (nickname).");
                    return;
                }

                auto nickname = e["nickname"].get<std::string>();
                if (!nickname.empty()) {
                    peerData->nickname = nickname.c_str();
                    if (!peerData->announced) {
                        ChatLog(string::Combine("(Server) Player %s connected.", string(peerData->nickname)));
                        peerData->announced = true;
                    }
                }
            }
        }

        if (type == PLAYER_DISCONNECT) {
            if (!e.contains("name") || !e["name"].is_string()) {
                ChatLog("Invalid PLAYER_DISCONNECT packet.");
                return;
            }

            string name = e["name"].get<std::string>().c_str();
            string nickname = "";
            if (e.contains("nickname")) {
                if (!e["nickname"].is_string()) {
                    ChatLog("Invalid PLAYER_DISCONNECT packet (nickname).");
                    return;
                }
                
                nickname = string(e["nickname"].get<std::string>().c_str());
            }
            auto displayName = nickname.IsEmpty() ? name : nickname;
            ChatLog(string::Combine("%s disconnected.", displayName));

            removeSyncedNpc(name);
            return;
        }

        if (IsCoopPaused) {
            return;
        }

        if (type == SYNC_SPELL_CAST) {
            //ChatLog(e.dump().c_str());
        }

        if (npcToSync == NULL) {
            if (IsCoopPlayer(id)) {
                npcToSync = addSyncedNpc(id.c_str());
                if (Myself) {
                    Myself->Reinit();
                }
            }
            else if (UniqueNameToNpcList.count(id.c_str())) {
                npcToSync = addSyncedNpc(id.c_str());
            }
        }

        if (npcToSync) {
            npcToSync->localUpdates.push_back(e);
        }
    }

    void ProcessServerPacket(ENetEvent packet) {
        switch (packet.type) {
        case ENET_EVENT_TYPE_CONNECT:
        {
            auto friendIdNumber = GetFreePlayerId();
            auto playerName = string::Combine("FRIEND_%i", friendIdNumber);

            json j;
            j["id"] = "HOST";
            j["type"] = SYNC_PLAYER_NAME;
            j["name"] = string(playerName).ToChar();
            j["connectId"] = packet.peer->connectID;
            ReadyToSendJsons.enqueue(j);

            addSyncedNpc(playerName);

            auto d = new PeerData();
            d->friendId = playerName;
            d->friendIdNumber = friendIdNumber;
            packet.peer->data = d;

            if (Myself) {
                Myself->Reinit();
            }
            for each (auto n in BroadcastNpcs)
            {
                n.second->Reinit();
            }
            break;
        }
        case ENET_EVENT_TYPE_RECEIVE:
        {
            auto player = (PeerData*)packet.peer->data;
            if (!player) {
                ChatLog("Received packet from peer without data; ignoring.");
                enet_packet_destroy(packet.packet);
                break;
            }
            auto dataLenght = packet.packet->dataLength;
            const char* data = (const char*)packet.packet->data;

            std::vector<std::uint8_t> bytesVector;
            for (size_t i = 0; i < dataLenght; i++) {
                bytesVector.push_back(data[i]);
            }

            auto j = json::from_bson(bytesVector);
            j["id"] = player->friendId.ToChar();

            ReadyToBeDistributedPackets.enqueue(j);
            ProcessCoopPacket(j, packet);
            SaveNetworkPacket(j.dump(-1, ' ', false, json::error_handler_t::ignore).c_str());
            enet_packet_destroy(packet.packet);
            break;
        }
        case ENET_EVENT_TYPE_DISCONNECT:
        {
            auto remoteNpc = (PeerData*)(packet.peer->data);
            auto displayName = remoteNpc ? (remoteNpc->nickname.IsEmpty() ? string("Player") : remoteNpc->nickname) : string("Player");
            ChatLog(string::Combine("%s disconnected.", displayName));

            json j;
            j["id"] = "HOST";
            j["type"] = PLAYER_DISCONNECT;
            j["name"] = remoteNpc ? string(remoteNpc->friendId).ToChar() : "Player";
            if (remoteNpc && !remoteNpc->nickname.IsEmpty()) {
                j["nickname"] = string(remoteNpc->nickname).ToChar();
            }
            ReadyToSendJsons.enqueue(j);

            if (remoteNpc) {
                removeSyncedNpc(remoteNpc->friendId);
                ReleasePlayerId(remoteNpc->friendIdNumber);
                delete remoteNpc;
            }
            packet.peer->data = NULL;
            break;
        }
        }
    }

    void ProcessClientPacket(ENetEvent packet) {
        switch (packet.type) {
        case ENET_EVENT_TYPE_RECEIVE:
        {
            auto dataLenght = packet.packet->dataLength;
            const char* data = (const char*)packet.packet->data;
            std::vector<std::uint8_t> bytesVector;
            for (size_t i = 0; i < dataLenght; i++) {
                bytesVector.push_back(data[i]);
            }

            auto j = json::from_bson(bytesVector);
            ProcessCoopPacket(j, packet);
            SaveNetworkPacket(j.dump(-1, ' ', false, json::error_handler_t::ignore).c_str());

            CurrentPing = packet.peer->roundTripTime;
            enet_packet_destroy(packet.packet);
            break;
        }
        case ENET_EVENT_TYPE_DISCONNECT:
        {
            ChatLog("Connection to the server lost.");
            break;
        }
        }
    }

    void PacketProcessorLoop() {
        PluginState = "PacketProcessorLoop";
        // Called from the main game tick loop, so avoid blocking when idle.

        while (!ReadyToBeReceivedPackets.isEmpty()) {
            auto packet = ReadyToBeReceivedPackets.dequeue();

            if (ServerThread) {
                ProcessServerPacket(packet);
            }
            else if (ClientThread)
            {
                ProcessClientPacket(packet);
            }
        }
    }
}