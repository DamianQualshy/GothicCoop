namespace GOTHIC_ENGINE {
    int CoopClientThread()
    {
        if (enet_initialize() != 0)
        {
            ChatLog("An error occurred while initializing ENet.");
            return EXIT_FAILURE;
        }
        atexit(enet_deinitialize);

        ENetHost* client;
        client = enet_host_create(NULL, 1, 2, 0, 0);
        if (client == NULL)
        {
            ChatLog("An error occurred while trying to create an ENet client host.");
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
            ChatLog("No available peers for initiating an ENet connection.");
            return EXIT_FAILURE;
        }

        if (
            enet_host_service(client, &event, 5000) > 0
            && event.type == ENET_EVENT_TYPE_CONNECT)
        {
            ChatLog(string::Combine("Connection to the server %s succeeded (v. %i, port %i).", string(serverIp.c_str()), COOP_VERSION, address.port));
        }
        else
        {
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

                if (!ReadyToSendJsons.isEmpty()) {
                    auto rawJson = ReadyToSendJsons.dequeue();
                    auto bjson = json::to_bson(rawJson);

                    ENetPacket* packet = enet_packet_create(&bjson[0], bjson.size(), PacketFlag(rawJson));
                    enet_peer_send(peer, PacketChannel(rawJson), packet);
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