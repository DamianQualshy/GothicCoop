namespace GOTHIC_ENGINE {
    int CoopServerThread()
    {
        if (enet_initialize() != 0)
        {
            ChatLog("An error occurred while initializing ENet.");
            return EXIT_FAILURE;
        }
        atexit(enet_deinitialize);

        ENetAddress address;
        ENetHost* server;
        enet_address_set_host(&address, "0.0.0.0");
        address.port = ConnectionPort;
        server = enet_host_create(&address, 32, 2, 0, 0);
        if (server == NULL)
        {
            ChatLog("An error occurred while trying to create an ENet server host.");
            return EXIT_FAILURE;
        }

        ChatLog(string::Combine("(Server) Ready (v. %i, port %i).", COOP_VERSION, address.port));
        while (true) {
            try {
                ENetEvent event;
                auto eventStatus = enet_host_service(server, &event, 1);
                if (eventStatus > 0) {
                    ReadyToBeReceivedPackets.enqueue(event);
                }

                if (!ReadyToBeDistributedPackets.isEmpty()) {
                    auto jsonPacket = ReadyToBeDistributedPackets.dequeue();
                    auto playerId = jsonPacket["id"].get<std::string>();

                    for (size_t i = 0; i < server->peerCount; i++) {
                        auto peer = &server->peers[i];
                        auto player = (PeerData*)peer->data;

                        if (!peer || !player) {
                            continue;
                        }

                        if (!player->friendId.Compare(playerId.c_str())) {
                            auto bson = json::to_bson(jsonPacket);

                            ENetPacket* packet = enet_packet_create(&bson[0], bson.size(), PacketFlag(jsonPacket));
                            enet_peer_send(peer, PacketChannel(jsonPacket), packet);
                        }
                    }
                }

                if (!ReadyToSendJsons.isEmpty()) {
                    auto rawJson = ReadyToSendJsons.dequeue();
                    auto bson = json::to_bson(rawJson);

                    ENetPacket* packet = enet_packet_create(&bson[0], bson.size(), PacketFlag(rawJson));
                    enet_host_broadcast(server, PacketChannel(rawJson), packet);
                }
            }
            catch (std::exception& ex) {
                Message::Error(ex.what(), "Server Thread Exception");
                return EXIT_FAILURE;
            }
            catch (...) {
                Message::Error("Caught unknown exception in server thread!");
                return EXIT_FAILURE;
            }
        }
    }
}