namespace GOTHIC_ENGINE {
    int CoopServerThread()
    {
        if (enet_initialize() != 0)
        {
            LogMessage(spdlog::level::err, "An error occurred while initializing ENet.");
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
            LogMessage(spdlog::level::err, "An error occurred while trying to create an ENet server host.");
            return EXIT_FAILURE;
        }

        LogMessage(spdlog::level::info, string::Combine("(Server) Ready (v. %i, port %i).", COOP_VERSION, address.port));
        while (true) {
            try {
                ENetEvent event;
                auto eventStatus = enet_host_service(server, &event, 1);
                if (eventStatus > 0) {
                    ReadyToBeReceivedPackets.enqueue(event);
                }

                if (!ReadyToBeDistributedPackets.isEmpty()) {
                    auto networkPacket = ReadyToBeDistributedPackets.dequeue();
                    auto playerId = networkPacket.senderId;

                    for (size_t i = 0; i < server->peerCount; i++) {
                        auto peer = &server->peers[i];
                        auto player = (PeerData*)peer->data;

                        if (!peer || !player) {
                            continue;
                        }

                        if (playerId.empty() || !player->friendId.Compare(playerId.c_str())) {
                            std::vector<std::uint8_t> payload;
                            std::string error;
                            if (!SerializeNetworkPacket(networkPacket, payload, error)) {
                                LogRateLimited(spdlog::level::warn,
                                               "server.serialize.failed",
                                               string::Combine("Failed to serialize packet: %s", string(error.c_str())),
                                               5000);
                                continue;
                            }

                            ENetPacket* packet = enet_packet_create(payload.data(), payload.size(), PacketFlag(networkPacket));
                            enet_peer_send(peer, PacketChannel(networkPacket), packet);
                        }
                    }
                }

                if (!ReadyToSendPackets.isEmpty()) {
                    auto outboundPacket = ReadyToSendPackets.dequeue();
                    std::vector<std::uint8_t> payload;
                    std::string error;
                    if (!SerializeNetworkPacket(outboundPacket, payload, error)) {
                        LogRateLimited(spdlog::level::warn,
                                       "server.broadcast.serialize.failed",
                                       string::Combine("Failed to serialize packet: %s", string(error.c_str())),
                                       5000);
                        continue;
                    }

                    ENetPacket* packet = enet_packet_create(payload.data(), payload.size(), PacketFlag(outboundPacket));
                    enet_host_broadcast(server, PacketChannel(outboundPacket), packet);
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