namespace GOTHIC_ENGINE {
    int CoopClientThread()
    {
        if (enet_initialize() != 0)
        {
            LogMessage(spdlog::level::err, "An error occurred while initializing ENet.");
            return EXIT_FAILURE;
        }
        atexit(enet_deinitialize);

        ENetHost* client;
        client = enet_host_create(NULL, 1, 2, 0, 0);
        if (client == NULL)
        {
            LogMessage(spdlog::level::err, "An error occurred while trying to create an ENet client host.");
            return EXIT_FAILURE;
        }

        ENetAddress address;
        ENetEvent event;
        ENetPeer* peer;
        auto serverIp = CoopConfig.ConnectionServer();
        enet_address_set_host(&address, serverIp.c_str());
        address.port = ConnectionPort;
        peer = enet_host_connect(client, &address, 2, 0);
        if (peer == NULL)
        {
            LogMessage(spdlog::level::warn, "No available peers for initiating an ENet connection.");
            return EXIT_FAILURE;
        }

        if (
            enet_host_service(client, &event, 5000) > 0
            && event.type == ENET_EVENT_TYPE_CONNECT)
        {
            LogMessage(spdlog::level::info, string::Combine("Connection to the server %s succeeded (v. %i, port %i).", string(serverIp.c_str()), COOP_VERSION, address.port));
        }
        else
        {
            enet_peer_reset(peer);
            LogMessage(spdlog::level::warn, string::Combine("Connection to the server %s failed (v. %i, port %i).", string(serverIp.c_str()), COOP_VERSION, address.port));
        }

        while (true) {
            try {
                ENetEvent event;
                auto eventStatus = enet_host_service(client, &event, 1);

                if (eventStatus > 0) {
                    ReadyToBeReceivedPackets.enqueue(event);
                }

                if (!ReadyToSendPackets.isEmpty()) {
                    auto outboundPacket = ReadyToSendPackets.dequeue();
                    std::vector<std::uint8_t> payload;
                    std::string error;
                    if (!SerializeNetworkPacket(outboundPacket, payload, error)) {
                        LogRateLimited(spdlog::level::warn,
                                       "client.serialize.failed",
                                       string::Combine("Failed to serialize packet: %s", string(error.c_str())),
                                       5000);
                        continue;
                    }

                    ENetPacket* packet = enet_packet_create(payload.data(), payload.size(), PacketFlag(outboundPacket));
                    enet_peer_send(peer, PacketChannel(outboundPacket), packet);
                }
            }
            catch (std::exception& ex) {
                Message::Error(ex.what(), "Client Thread Exception");
                return EXIT_FAILURE;
            }
            catch (...) {
                Message::Error("Caught unknown exception in client thread!");
                return EXIT_FAILURE;
            }
        }
    }
}