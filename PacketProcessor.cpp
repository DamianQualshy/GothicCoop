namespace GOTHIC_ENGINE {
    void ProcessCoopPacket(const NetworkPacket& packetData, ENetEvent packet) {
        if (packetData.type == PacketType::JoinGame) {
            if (packetData.joinGame.connectId == packet.peer->connectID) {
                MyselfId = packetData.joinGame.name.c_str();
            }
            return;
        }

        if (packetData.type == PacketType::PlayerDisconnect) {
            string name = packetData.disconnect.name.c_str();
            string nickname = packetData.disconnect.hasNickname ? packetData.disconnect.nickname.c_str() : "";
            auto displayName = nickname.IsEmpty() ? name : nickname;
            ChatLog(string::Combine("%s disconnected.", displayName));

            removeSyncedNpc(name);
            return;
        }

        if (packetData.type != PacketType::PlayerStateUpdate) {
            ChatLog("Invalid packet received (unknown type).");
            return;
        }

        if (packetData.senderId.empty()) {
            ChatLog("Invalid packet received (missing sender id).");
            return;
        }

        auto id = packetData.senderId;
        auto type = packetData.stateUpdate.updateType;
        RemoteNpc* npcToSync = NULL;
        auto syncIt = SyncNpcs.find(id.c_str());
        if (syncIt != SyncNpcs.end()) {
            npcToSync = syncIt->second;
        }

        if (type == INIT_NPC) {
            auto peerData = (PeerData*)packet.peer->data;
            if (peerData) {
                auto nickname = packetData.stateUpdate.initNpc.nickname;
                if (!nickname.empty()) {
                    peerData->nickname = nickname.c_str();
                }
                if (!peerData->announced && !peerData->nickname.IsEmpty()) {
                    ChatLog(string::Combine("(Server) Player %s connected.", string(peerData->nickname)));
                    peerData->announced = true;
                }
            }
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
            npcToSync->localUpdates.push_back(packetData.stateUpdate);
        }
    }

    void ProcessServerPacket(ENetEvent packet) {
        switch (packet.type) {
        case ENET_EVENT_TYPE_CONNECT:
        {
            auto friendIdNumber = GetFreePlayerId();
            auto playerName = string::Combine("FRIEND_%i", friendIdNumber);

            NetworkPacket joinPacket;
            joinPacket.type = PacketType::JoinGame;
            joinPacket.senderId = "HOST";
            joinPacket.joinGame.name = playerName.ToChar();
            joinPacket.joinGame.connectId = packet.peer->connectID;
            ReadyToSendPackets.enqueue(joinPacket);

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
            NetworkPacket incoming;
            std::string error;
            if (!DeserializeNetworkPacket(reinterpret_cast<const std::uint8_t*>(data), dataLenght, incoming, error, PacketDecodeMode::Server)) {
                ChatLog(string::Combine("Invalid packet received: %s", string(error.c_str())));
                enet_packet_destroy(packet.packet);
                break;
            }
            if (incoming.type != PacketType::PlayerStateUpdate) {
                ChatLog("Invalid packet received (unexpected type).");
                enet_packet_destroy(packet.packet);
                break;
            }
            incoming.senderId = player->friendId.ToChar();

            ReadyToBeDistributedPackets.enqueue(incoming);
            ProcessCoopPacket(incoming, packet);
#if defined(COOP_ENABLE_JSON_DEBUG) && COOP_ENABLE_JSON_DEBUG
            SaveNetworkPacket(DescribePacket(incoming).c_str());
#endif
            enet_packet_destroy(packet.packet);
            break;
        }
        case ENET_EVENT_TYPE_DISCONNECT:
        {
            auto remoteNpc = (PeerData*)(packet.peer->data);
            auto displayName = remoteNpc ? (remoteNpc->nickname.IsEmpty() ? string("Player") : remoteNpc->nickname) : string("Player");
            ChatLog(string::Combine("%s disconnected.", displayName));

            NetworkPacket disconnectPacket;
            disconnectPacket.type = PacketType::PlayerDisconnect;
            disconnectPacket.senderId = "HOST";
            disconnectPacket.disconnect.name = remoteNpc ? std::string(remoteNpc->friendId.ToChar()) : std::string("Player");
            disconnectPacket.disconnect.hasNickname = remoteNpc && !remoteNpc->nickname.IsEmpty();
            if (disconnectPacket.disconnect.hasNickname) {
                disconnectPacket.disconnect.nickname = remoteNpc->nickname.ToChar();
            }
            ReadyToSendPackets.enqueue(disconnectPacket);

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
            NetworkPacket incoming;
            std::string error;
            if (!DeserializeNetworkPacket(reinterpret_cast<const std::uint8_t*>(data), dataLenght, incoming, error, PacketDecodeMode::Client)) {
                ChatLog(string::Combine("Invalid packet received: %s", string(error.c_str())));
                enet_packet_destroy(packet.packet);
                break;
            }
            ProcessCoopPacket(incoming, packet);
#if defined(COOP_ENABLE_JSON_DEBUG) && COOP_ENABLE_JSON_DEBUG
            SaveNetworkPacket(DescribePacket(incoming).c_str());
#endif

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