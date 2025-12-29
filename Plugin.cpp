#include "resource.h"

namespace GOTHIC_ENGINE {
    void Game_Entry() {
        GetCurrentDirectory(MAX_PATH, GothicExeFolderPath);

        SymInitialize(GetCurrentProcess(), NULL, true);
        SymSetSearchPath(GetCurrentProcess(), GothicExeFolderPath);

        std::string logFilePath = GothicExeFolderPath;
        logFilePath.append("\\GothicCoopLog.log");
        GothicCoopLogPath = logFilePath;

        std::string configFilePath = GothicExeFolderPath;
        configFilePath.append("\\GothicCoopConfig.toml");
        bool loadedConfig = CoopConfig.LoadFromFile(configFilePath, true);
        if (!loadedConfig) {
            Message::Error("(Gothic Coop) Config file missing or invalid. Defaults will be used.");
        }
    }

    void Game_Init() {
        MainThreadId = GetCurrentThreadId();
        ToggleGameLogKey = CoopConfig.ToggleGameLogKeyCode();
        ToggleGameStatsKey = CoopConfig.ToggleGameStatsKeyCode();
        StartServerKey = CoopConfig.StartServerKeyCode();
        StartConnectionKey = CoopConfig.StartConnectionKeyCode();
        ReinitPlayersKey = CoopConfig.ReinitPlayersKeyCode();
        RevivePlayerKey = CoopConfig.RevivePlayerKeyCode();

        ConnectionPort = CoopConfig.ConnectionPort();
        MyBodyModel = CoopConfig.BodyModel();
        MyBodyTex = CoopConfig.BodyTex();
        MyHeadModel = CoopConfig.HeadModel();
        MyHeadTex = CoopConfig.HeadTex();
#if ENGINE < Engine_G2
        MyBodyColor = CoopConfig.BodyColor();
#endif
        PlayersDamageMultipler = CoopConfig.PlayersDamageMultiplier();
        NpcsDamageMultipler = CoopConfig.NpcsDamageMultiplier();

        auto friendInstanceId = CoopConfig.FriendInstanceId();
        if (!friendInstanceId.empty()) {
            auto stdStringFriendInstanceId = friendInstanceId;
            std::transform(stdStringFriendInstanceId.begin(), stdStringFriendInstanceId.end(), stdStringFriendInstanceId.begin(), ::tolower);
            FriendInstanceId = string(stdStringFriendInstanceId.c_str()).ToChar();
        }
        auto nickname = CoopConfig.Nickname();
        if (!nickname.empty()) {
            MyNickname = string(nickname.c_str()).ToChar();
        }
    }

    void Game_Loop() {
        PluginState = "GameLoop";
        if (IsLoadingLevel) {
            return;
        }

        CurrentMs = GetCurrentMs();
        GameChat->Render();
        GameStatsLoop();
        PacketProcessorLoop();
        DamageProcessorLoop();
        SpellCastProcessorLoop();
        ReviveFriendLoop();

        if (CurrentMs > LastNpcListRefreshTime + 1000) {
            BuildGlobalNpcList();
            LastNpcListRefreshTime = CurrentMs;
            PluginState = "GameLoop";
        }

        if (CurrentMs > LastUpdateListOfVisibleNpcs + 500) {
            UpdateVisibleNpc();
            LastUpdateListOfVisibleNpcs = CurrentMs;
            PluginState = "GameLoop";
        }

        if (!IsCoopPaused) {
            PluginState = "PulseMyself";
            if (Myself) {
                Myself->Pulse();
                Myself->PackUpdate();
            }

            PluginState = "PulseBroadcastNpcs";
            for (auto p : BroadcastNpcs) {
                p.second->Pulse();
                p.second->PackUpdate();
            }

            PluginState = "UpdateSyncNpcs";
            for (auto it = SyncNpcs.begin(); it != SyncNpcs.end();) {
                auto npc = it->second;
                npc->Update();

                if (npc->destroyed) {
                    delete npc;
                    it = SyncNpcs.erase(it);
                }
                else {
                    ++it;
                }
            }

            PluginState = "DisplayPing";
            if (ClientThread && CurrentPing > 0) {
                auto font = screen->GetFontName();
                auto color = screen->fontColor;

                screen->SetFont(zSTRING("Font_Old_10_White_Hi.TGA"));
                screen->SetFontColor(CurrentPing > 100 ? GFX_RED : GFX_WHITE);
                screen->Print(50, 0, "Ping: " + Z CurrentPing);

                screen->SetFont(font);
                screen->SetColor(color);
            }
        }

        PluginState = "KeysPressedChecks";
        if (!IsPlayerTalkingWithAnybody()) {
            if (zinput->KeyToggled(StartServerKey) && !ServerThread && !ClientThread) {
                wchar_t mappedPort[1234];
                std::wcsncpy(mappedPort, L"UDP", 1234);
                new MappedPort(ConnectionPort, mappedPort, mappedPort);

                Thread t;
                t.Init(&CoopServerThread);
                t.Detach();
                ServerThread = &t;
                MyselfId = "HOST";
                player->SetAdditionalVisuals(zSTRING("hum_body_Naked0"), MyBodyTex, MyBodyColor, zSTRING("HUM_HEAD_PONY"), MyHeadTex, 0, -1);
            }

            if (zinput->KeyToggled(StartConnectionKey) && !ServerThread) {
                if (!ClientThread) {
                    addSyncedNpc("HOST");

                    Thread  t;
                    t.Init(&CoopClientThread);
                    t.Detach();
                    ClientThread = &t;

                    ogame->SetTime(ogame->GetWorldTimer()->GetDay(), 12, 00);
                    rtnMan->RestartRoutines();
                    player->SetAdditionalVisuals(zSTRING("hum_body_Naked0"), MyBodyTex, MyBodyColor, zSTRING("HUM_HEAD_PONY"), MyHeadTex, 0, -1);
                }
                else {
                    if (IsCoopPaused) {
                        ChatLog("Restoring world synchronization.");
                    }
                    else {
                        ChatLog("Stop world synchronization.");
                    }

                    IsCoopPaused = !IsCoopPaused;
                    rtnMan->RestartRoutines();
                }
            }

            if (zinput->KeyToggled(ReinitPlayersKey)) {
                ChatLog("Respawning all coop friend NPCs.");

                std::map<oCNpc*, string> PlayerNpcsCopy = PlayerNpcs;
                for (auto playerNpcToName : PlayerNpcsCopy) {
                    auto name = playerNpcToName.second;
                    if (SyncNpcs.count(name)) {
                        SyncNpcs[name]->ReinitCoopFriendNpc();
                    }
                }
            }
        }

        if ((ClientThread || ServerThread) && !Myself) {
            PluginState = "Myself_Init";
            Myself = new LocalNpc(player, MyselfId);
        }

        PluginState = "GameLoop_End";
    }

    void Game_SaveBegin() {
        IsSavingGame = true;
    }

    void Game_SaveEnd() {
        for (auto playerNpcToName : PlayerNpcs) {
            auto name = playerNpcToName.second;
            if (SyncNpcs.count(name)) {
                SyncNpcs[name]->InitCoopFriendNpc();
            }
        }

        IsSavingGame = false;
    }

    void LoadBegin() {
        IsLoadingLevel = true;
        Myself = NULL;
    }

    void LoadEnd() {
        if (ServerThread || ClientThread) {
            Myself = new LocalNpc(player, MyselfId);
        }

        std::map<string, RemoteNpc*> syncPlayerNpcs;
        for (auto playerNpcToName : PlayerNpcs) {
            auto name = playerNpcToName.second;
            if (SyncNpcs.count(name)) {
                syncPlayerNpcs[name] = SyncNpcs[name];
            }
        }

        for (auto existingNpc : SyncNpcs) {
            if (syncPlayerNpcs.count(existingNpc.first) == 0) {
                delete existingNpc.second;
            }
        }

        SyncNpcs = syncPlayerNpcs;

        BroadcastNpcs.clear();
        UniqueNameToNpcList.clear();
        NpcToUniqueNameList.clear();
        NamesCounter.clear();
        NpcToFirstRoutineWp.clear();
        GameChat->Clear();
        LastNpcListRefreshTime = 0;
        LastUpdateListOfVisibleNpcs = 0;
        KilledByPlayerNpcNames.clear();

        auto totWp = ogame->GetWorld()->wayNet->GetWaypoint("TOT");
        if (totWp) {
            CurrentWorldTOTPosition = &totWp->GetPositionWorld();
        }
        else {
            CurrentWorldTOTPosition = NULL;
        }

        IsLoadingLevel = false;
    }

    void Game_LoadEnd_SaveGame() {
        LoadEnd();

        auto coopFriendInstanceId = GetFriendDefaultInstanceId();
        auto* list = ogame->GetGameWorld()->voblist_npcs->next;

        while (list) {
            auto npc = list->data;
            if (npc->GetInstance() == coopFriendInstanceId && npc->GetAttribute(NPC_ATR_STRENGTH) == COOP_MAGIC_NUMBER) {
                ogame->spawnman->DeleteNpc(npc);
            }
            list = list->next;
        }
    }

    void Game_LoadBegin_Trigger() {
        return;
    }
}
