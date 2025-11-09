namespace GOTHIC_ENGINE {
    void SpellCastProcessorLoop() {
        PluginState = "SpellCastProcessorLoop";

        if (ReadyToSyncSpellCasts.isEmpty()) {
            return;
        }

        auto spellCast = ReadyToSyncSpellCasts.dequeue();

        if (!ClientThread && !ServerThread) {
            return;
        }

        if (Myself && spellCast.npc == Myself->npc && NpcToUniqueNameList.count(spellCast.targetNpc)) {
            spellCast.npcUniqueName = Myself->name;
            spellCast.targetNpcUniqueName = NpcToUniqueNameList[spellCast.targetNpc];
            Myself->spellCastsToSync.push_back(spellCast);
            return;
        }

        if (ServerThread) {
            auto uniqueNpcName = NpcToUniqueNameList.count(spellCast.npc) ? NpcToUniqueNameList[spellCast.npc] : NULL;

            if (uniqueNpcName && BroadcastNpcs.count(uniqueNpcName)) {
                spellCast.npcUniqueName = uniqueNpcName;

                if (spellCast.targetNpc == player) {
                    spellCast.targetNpcUniqueName = "HOST";
                    BroadcastNpcs[uniqueNpcName]->spellCastsToSync.push_back(spellCast);
                }
                else if (PlayerNpcs.count(spellCast.targetNpc)) {
                    spellCast.targetNpcUniqueName = PlayerNpcs[spellCast.targetNpc];
                    BroadcastNpcs[uniqueNpcName]->spellCastsToSync.push_back(spellCast);
                }
                else if (NpcToUniqueNameList.count(spellCast.targetNpc)) {
                    spellCast.targetNpcUniqueName = NpcToUniqueNameList[spellCast.targetNpc];
                    BroadcastNpcs[uniqueNpcName]->spellCastsToSync.push_back(spellCast);
                }
            }
        }
    }
}