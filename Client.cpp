namespace GOTHIC_ENGINE {
    DWORD WINAPI CoopClientThread(void*)
    {
        struct ThreadExitReset {
            Thread** slot;
            explicit ThreadExitReset(Thread** target) : slot(target) {}
            ~ThreadExitReset() {
                if (slot) {
                    *slot = NULL;
                }
            }
        } resetClientThread(&ClientThread);

        CoopLog("[Client] Thread entry.");
        if (enet_initialize() != 0)
        {
            CoopLog("[Client] ENet init failed.");
            ChatLog("An error occurred while initializing ENet.");
            return EXIT_FAILURE;
        }
        atexit(enet_deinitialize);

        ENetHost* client;
        client = enet_host_create(NULL, 1, 2, 0, 0);
        if (client == NULL)
        {
            CoopLog("[Client] ENet host create failed.");
            ChatLog("An error occurred while trying to create an ENet client host.");
            return EXIT_FAILURE;
        }

        CoopLog("[Client] ENet host created.");
        ENetAddress address;
        ENetEvent event;
        ENetPeer* peer;
        auto serverIp = CoopConfig.ConnectionServer();
        enet_address_set_host(&address, serverIp.c_str());
        address.port = ConnectionPort;
        peer = enet_host_connect(client, &address, 2, 0);
        if (peer == NULL)
        {
            CoopLog("[Client] ENet connect failed to create peer.");
            ChatLog("No available peers for initiating an ENet connection.");
            return EXIT_FAILURE;
        }

        CoopLog("[Client] ENet connect initiated.");
        if (
            enet_host_service(client, &event, 5000) > 0
            && event.type == ENET_EVENT_TYPE_CONNECT)
        {
            CoopLog("[Client] ENet connect succeeded.");
            ChatLog(string::Combine("Connection to the server %s succeeded (v. %i, port %i).", string(serverIp.c_str()), COOP_VERSION, address.port));
        }
        else
        {
            CoopLog("[Client] ENet connect timed out or failed.");
            enet_peer_reset(peer);
            ChatLog(string::Combine("Connection to the server %s failed (v. %i, port %i).", string(serverIp.c_str()), COOP_VERSION, address.port));
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
                        ChatLog(string::Combine("Failed to serialize packet: %s", string(error.c_str())));
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