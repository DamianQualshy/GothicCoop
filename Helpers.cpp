namespace GOTHIC_ENGINE {
    void CoopLog(std::string l)
    {
        LogMessage(spdlog::level::info, l);
    }

    void ChatLog(string text, zCOLOR color = zCOLOR(255, 255, 255, 255)) {
        GameChat->AddLine(text, color);
    };

    int GetFreePlayerId() {
        for (int id = 1; ; id++) {
            if (ActiveFriendIds.count(id) == 0) {
                ActiveFriendIds.insert(id);
                return id;
            }
        }
    }

    void ReleasePlayerId(int id) {
        if (id > 0) {
            ActiveFriendIds.erase(id);
        }
    }

    bool IsCoopPlayer(std::string name) {
        string cStringName = name.c_str();
        return cStringName == "HOST" || cStringName.StartWith("FRIEND_");
    }

    bool IsCoopPlayer(string name) {
        return name == "HOST" || name.StartWith("FRIEND_");
    }

    oCItem* CreateCoopItem(int insIndex) {
        return zfactory->CreateItem(insIndex);
    }

    std::vector<oCNpc*> GetVisibleNpcs() {
        std::vector<oCNpc*> npcs;
        auto* list = ogame->GetGameWorld()->voblist_npcs->next;

        while (list) {
            auto npc = list->data;
            auto npcPosition = npc->GetPositionWorld();
            auto playerPosition = player->GetPositionWorld();

            if (npc->vobLeafList.GetNum() == 0) {
                list = list->next;
                continue;
            }

            if (playerPosition.Distance(npcPosition) < BROADCAST_DISTANCE) {
                if (!npc->IsAPlayer() && !npc->GetObjectName().StartWith("FRIEND_")) {
                    npcs.push_back(npc);
                }
            }
            list = list->next;
        }

        return npcs;
    }

    float GetHeading(oCNpc* npc)
    {
        float x = *(float*)((DWORD)npc + 0x44);
        float rotx = asin(x) * 180.0f / 3.14f;
        float y = *(float*)((DWORD)npc + 0x64);
        if (y > 0)
        {
            if (x < 0)
                rotx = 360 + rotx;
        }
        else
        {
            if (rotx > 0)
                rotx = 180 - rotx;
            else
            {
                rotx = 180 + rotx;
                rotx = 360 - rotx;
            }
        }
        return rotx;
    };

    bool IsPlayerTalkingWithAnybody() {
        return ogame->GetCameraAI()->GetMode().Compare("CAMMODDIALOG") == 0;
    }

    bool IsPlayerTalkingWithNpc(zCVob* npc) {
        if (ogame->GetCameraAI()->GetMode().Compare("CAMMODDIALOG") != 0) {
            return false;
        }

        if (player->talkOther == npc) {
            return true;
        }

        if (ogame->GetCameraAI()->targetVobList.GetNum() > 0) {
            for (int i = 0; i < ogame->GetCameraAI()->targetVobList.GetNum(); i++) {
                auto vob = ogame->GetCameraAI()->targetVobList[i];
                if (npc == vob) {
                    return true;
                }
            }
        }

        return false;
    }

    float GetDistance3D(float aX, float aY, float aZ, float bX, float bY, float bZ)
    {
        float distX = aX - bX;
        float distY = aY - bY;
        float distZ = aZ - bZ;
        return sqrt(distX * distX + distY * distY + distZ * distZ);
    };

    bool IgnoredSyncNpc(zCVob* npc) {
        auto name = npc->GetObjectName();

        for (unsigned int i = 0; i < IgnoredSyncNpcsCount; i++)
        {
            if (strcmp(name.ToChar(), IgnoredSyncNpcs[i]) == 0)
                return true;
        }

        return false;
    }

    void BuildGlobalNpcList() {
        PluginState = "BuildGlobalNpcList";

        auto* list = ogame->GetGameWorld()->voblist_npcs->next;
        auto firstRun = NpcToFirstRoutineWp.size() == 0;

        if (firstRun) {
            auto* rtnList = rtnMan->rtnList.next;
            while (rtnList) {
                auto rtn = rtnList->data;
                if (NpcToFirstRoutineWp.count(rtn->npc) == 0) {
                    NpcToFirstRoutineWp[rtn->npc] = rtn->wpname.ToChar();
                }
                rtnList = rtnList->next;
            }

            while (list) {
                auto npc = list->data;
                NamesCounter[string(npc->GetObjectName())] += 1;
                list = list->next;
            }
        }

        list = ogame->GetGameWorld()->voblist_npcs->next;
        while (list) {
            auto npc = list->data;

            if (npc->IsAPlayer() || npc->GetObjectName().StartWith("FRIEND_") || IgnoredSyncNpc(npc) || NpcToUniqueNameList.count(npc)) {
                list = list->next;
                continue;
            }

            auto objectName = string(npc->GetObjectName());
            CStringA name = "";
            if (NamesCounter[objectName] == 1) {
                name = objectName;
            }
            else {
                auto secondUniquePart = npc->wpname ? string(npc->wpname) : string("UNKNOW");
                if (NpcToFirstRoutineWp.count(npc) > 0 && !NpcToFirstRoutineWp[npc].IsEmpty()) {
                    secondUniquePart = NpcToFirstRoutineWp[npc];
                }
                if (!firstRun) {
                    secondUniquePart = npc->wpname ? string::Combine("DYNAMIC-%s", string(npc->wpname)) : string("DYNAMIC");
                }
                name = string::Combine("%s-%s", string(npc->GetObjectName()), secondUniquePart);
                NamesCounter[name] += 1;
            }

            auto uniqueName = string::Combine("%s-%i", name, NamesCounter[name]);
            UniqueNameToNpcList[uniqueName] = npc;
            NpcToUniqueNameList[npc] = uniqueName;

            list = list->next;
        }
    }

    bool EnsureNpcUniqueName(oCNpc* npc) {
        if (!npc) {
            return false;
        }

        if (NpcToUniqueNameList.count(npc) > 0) {
            return true;
        }

        if (npc->IsAPlayer() || npc->GetObjectName().StartWith("FRIEND_") || IgnoredSyncNpc(npc)) {
            return false;
        }

        BuildGlobalNpcList();
        return NpcToUniqueNameList.count(npc) > 0;
    }

    long long GetCurrentMs() {
        std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
            );

        return ms.count();
    }

    static int GetPartyMemberID() {
        zCPar_Symbol* sym = parser->GetSymbol("AIV_PARTYMEMBER");
        if (!sym)
#if ENGINE >= Engine_G2
            return 15;
#else
            return 36;
#endif
        int id;
        sym->GetValue(id, 0);
        return id;
    }

    static int GetFriendDefaultInstanceId() {
        auto instId = parser->GetIndex(FriendInstance);

        return instId;
    }
}