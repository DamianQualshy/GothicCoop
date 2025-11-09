namespace GOTHIC_ENGINE {
    void UpdateVisibleNpc() {
        PluginState = "UpdateVisibleNpc";

        if (ServerThread) {
            auto npcs = GetVisibleNpcs();
            std::map<string, LocalNpc*> updatedBroadcastNpcs;

            for each (auto npc in npcs)
            {
                if (NpcToUniqueNameList.count(npc) == 0) {
                    continue;
                }

                auto npcUniqueName = NpcToUniqueNameList[npc];
                if (BroadcastNpcs.count(npcUniqueName) == 0) {
                    updatedBroadcastNpcs[npcUniqueName] = new LocalNpc(npc, npcUniqueName);
                }
                else {
                    updatedBroadcastNpcs[npcUniqueName] = BroadcastNpcs[npcUniqueName];
                }
            }

            BroadcastNpcs = updatedBroadcastNpcs;
        }
    }
}