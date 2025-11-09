namespace GOTHIC_ENGINE {
    bool displayNetworkStats = false;

    void GameStatsLoop() {
        PluginState = "GameStatsLoop";

        if (WorldEditMode) {
            return;
        }

        if (zinput->KeyToggled(ToggleGameStatsKey)) {
            displayNetworkStats = !displayNetworkStats;
            GameChat->Clear();
        }

        if (zinput->KeyToggled(ToggleGameLogKey)) {
            GameChat->ToggleShowing();
        }

        if (displayNetworkStats) {
            GameChat->Clear();
            ChatLog("readyToSendJsons:");
            ChatLog(ReadyToSendJsons.size());
            ChatLog("readyToBeReceivedPackets:");
            ChatLog(ReadyToBeReceivedPackets.size());
            ChatLog("possition:");
            auto pos = player->GetPositionWorld();
            ChatLog(string::Combine("x: %f y: %f z: %f", pos.n[0], pos.n[1], pos.n[2]));
            if (ServerThread) {
                ChatLog("broadcastNpcs:");
                ChatLog(BroadcastNpcs.size());

                for each (auto i in BroadcastNpcs) {
                    ChatLog(i.first);
                }
            }
            if (ClientThread) {
                ChatLog("SyncNpcs:");
                ChatLog(SyncNpcs.size());

                for each (auto i in SyncNpcs) {
                    ChatLog(i.first);
                }
            }
        }
    }
}