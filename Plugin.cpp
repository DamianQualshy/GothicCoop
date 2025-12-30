#include "resource.h"
#include <sstream>

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
        StartupGuardMs = CoopConfig.StartupGuardMs();

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

        auto friendInstance = CoopConfig.FriendInstance();
        if (!friendInstance.empty()) {
            auto stdStringFriendInstance = friendInstance;
            std::transform(stdStringFriendInstance.begin(), stdStringFriendInstance.end(), stdStringFriendInstance.begin(), ::tolower);
            FriendInstance = string(stdStringFriendInstance.c_str()).ToChar();
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
            bool startupGuardActive = StartupGuardMs > 0 && LastLoadEndMs > 0 && CurrentMs < LastLoadEndMs + StartupGuardMs;
            auto logStartupGuardBlocked = [&](const char* action, const char* chatMessage) {
                long long remainingMs = LastLoadEndMs + StartupGuardMs - CurrentMs;
                if (remainingMs < 0) {
                    remainingMs = 0;
                }
                std::ostringstream message;
                message << "[StartupGuard] " << action << " blocked for " << remainingMs << " ms after load end.\r\n";
                CoopLog(message.str());
                ChatLog(string(chatMessage));
            };

            if (zinput->KeyToggled(StartServerKey)) {
                if (startupGuardActive) {
                    logStartupGuardBlocked("Server start request", "(Server) Start blocked during startup guard.");
                }
                else if (ServerThread || ClientThread) {
                    CoopLog("[Server] Start key pressed but server/client already running.\r\n");
                    ChatLog("(Server) Already running.");
                }
                else {
                    CoopLog("[Server] Start key pressed. Initializing server thread.\r\n");
                    ChatLog("(Server) Starting...");
                    wchar_t mappedPort[1234];
                    std::wcsncpy(mappedPort, L"UDP", 1234);
                    new MappedPort(ConnectionPort, mappedPort, mappedPort);

                    ServerThreadStorage.Init(&CoopServerThread);
                    ServerThreadStorage.Detach();
                    ServerThread = &ServerThreadStorage;
                    CoopLog("[Server] Server thread started.\r\n");
                    MyselfId = "HOST";
                    player->SetAdditionalVisuals(zSTRING(MyBodyModel.c_str()), MyBodyTex, MyBodyColor, zSTRING(MyHeadModel.c_str()), MyHeadTex, 0, -1);
                }
            }

            if (zinput->KeyToggled(StartConnectionKey)) {
                if (startupGuardActive) {
                    logStartupGuardBlocked("Client start request", "(Client) Start blocked during startup guard.");
                }
                else if (ServerThread) {
                    CoopLog("[Client] Connect key pressed but server is running.\r\n");
                    ChatLog("(Client) Cannot connect while hosting.");
                }
                else if (!ClientThread) {
                    CoopLog("[Client] Connect key pressed. Initializing client thread.\r\n");
                    ChatLog("(Client) Connecting...");
                    addSyncedNpc("HOST");

                    ClientThreadStorage.Init(&CoopClientThread);
                    ClientThreadStorage.Detach();
                    ClientThread = &ClientThreadStorage;
                    CoopLog("[Client] Client thread started.\r\n");

                    ogame->SetTime(ogame->GetWorldTimer()->GetDay(), 12, 00);
                    rtnMan->RestartRoutines();
                    player->SetAdditionalVisuals(zSTRING(MyBodyModel.c_str()), MyBodyTex, MyBodyColor, zSTRING(MyHeadModel.c_str()), MyHeadTex, 0, -1);
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

            if ((ClientThread || ServerThread) && !Myself) {
                PluginState = "Myself_Init";
                Myself = new LocalNpc(player, MyselfId);
            }

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

        for (auto& syncNpc : SyncNpcs) {
            syncNpc.second->ReinitCoopFriendNpc();
        }

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

        if (player) {
            player->SetAdditionalVisuals(zSTRING(MyBodyModel.c_str()), MyBodyTex, MyBodyColor, zSTRING(MyHeadModel.c_str()), MyHeadTex, 0, -1);
        }

        if (Myself) {
            Myself->Reinit();
        }

        IsLoadingLevel = false;
        LastLoadEndMs = GetCurrentMs();
    }

    void Game_LoadEnd_SaveGame() {
        LoadEnd();

        auto coopFriendInstance = GetFriendDefaultInstanceId();
        auto* list = ogame->GetGameWorld()->voblist_npcs->next;

        while (list) {
            auto npc = list->data;
            if (npc->GetInstance() == coopFriendInstance && npc->GetAttribute(NPC_ATR_STRENGTH) == COOP_MAGIC_NUMBER) {
                ogame->spawnman->DeleteNpc(npc);
            }
            list = list->next;
        }
    }

    void Game_LoadBegin_Trigger() {
        return;
    }
}
