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
        std::ifstream configFile(configFilePath);

        if (configFile.good()) {
            try {
                CoopConfig = ParseTomlConfig(configFile);
            }
            catch (...) {
                Message::Error("(Gothic Coop) Invalid config file, please check your GothicCoopConfig.toml file!");
                exit(1);
            }
        }
        else {
            Message::Error("(Gothic Coop) No config file found!");
            Message::Error(configFilePath.c_str());
            exit(1);
        }
    }

    void Game_Init() {
        MainThreadId = GetCurrentThreadId();
        ToggleGameLogKey = ReadConfigKey("controls", "toggleGameLogKey", "KEY_P");
        ToggleGameStatsKey = ReadConfigKey("controls", "toggleGameStatsKey", "KEY_O");
        StartServerKey = ReadConfigKey("controls", "startServerKey", "KEY_F1");
        StartConnectionKey = ReadConfigKey("controls", "startConnectionKey", "KEY_F2");
        ReinitPlayersKey = ReadConfigKey("controls", "reinitPlayersKey", "KEY_F3");
        RevivePlayerKey = ReadConfigKey("controls", "revivePlayerKey", "KEY_F4");

        ConnectionPort = ReadConfigInt("connection", "port", ConnectionPort);
        MyBodyTextVarNr = ReadConfigInt("appearance", "bodyTextVarNr", MyBodyTextVarNr);
        MyHeadVarNr = ReadConfigInt("appearance", "headVarNr", MyHeadVarNr);
        PlayersDamageMultipler = ReadConfigInt("gameplay", "playersDamageMultiplier", PlayersDamageMultipler);
        NpcsDamageMultipler = ReadConfigInt("gameplay", "npcsDamageMultiplier", NpcsDamageMultipler);

        auto friendInstanceId = ReadConfigString("player", "friendInstanceId", "");
        if (!friendInstanceId.empty()) {
            auto stdStringFriendInstanceId = friendInstanceId;
            std::transform(stdStringFriendInstanceId.begin(), stdStringFriendInstanceId.end(), stdStringFriendInstanceId.begin(), ::tolower);
            FriendInstanceId = string(stdStringFriendInstanceId.c_str()).ToChar();
        }
        auto nickname = ReadConfigString("player", "nickname", "");
        if (!nickname.empty()) {
            MyNickname = string(nickname.c_str()).ToChar();
        }

        auto editModeValue = ReadConfigString("gameplay", "editmode", "");
        if (editModeValue.empty()) {
            editModeValue = ReadConfigString("", "editmode", "");
        }
        if (!editModeValue.empty()) {
            auto editModeValueString = string(editModeValue.c_str());
            if (editModeValueString.Compare("MARVIN")) {
                WorldEditMode = true;
                Thread  t;
                ClientThread = &t;
                ChatLog("World edit mode is activated!");
            }
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
        if (!IsPlayerTalkingWithAnybody() && !WorldEditMode) {
            if (zinput->KeyToggled(StartServerKey) && !ServerThread && !ClientThread) {
                wchar_t mappedPort[1234];
                std::wcsncpy(mappedPort, L"UDP", 1234);
                new MappedPort(ConnectionPort, mappedPort, mappedPort);

                Thread t;
                t.Init(&CoopServerThread);
                t.Detach();
                ServerThread = &t;
                MyselfId = "HOST";
                player->SetAdditionalVisuals(zSTRING("hum_body_Naked0"), MyBodyTextVarNr, DefaultBodyTexColorNr, zSTRING("HUM_HEAD_PONY"), MyHeadVarNr, 0, -1);
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
                    player->SetAdditionalVisuals(zSTRING("hum_body_Naked0"), MyBodyTextVarNr, DefaultBodyTexColorNr, zSTRING("HUM_HEAD_PONY"), MyHeadVarNr, 0, -1);
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

        if (WorldEditMode) {
            if (zinput->KeyToggled(StartServerKey)) {
                if (player->GetObjectName() == "PC_HERO") {
                    return;
                }

                auto currentNpc = player;
                Myself->npc->SetAsPlayer();

                ogame->spawnman->SpawnNpc(currentNpc, currentNpc->GetPositionWorld(), 0.f);
                ChatLog(string::Combine("%s was added to the world!", string(currentNpc->GetObjectName())));
            }

            if (zinput->KeyToggled(RevivePlayerKey)) {
                if (player->GetObjectName() == "PC_HERO") {
                    return;
                }

                auto currentNpc = player;
                Myself->npc->SetAsPlayer();

                ogame->spawnman->DeleteNpc(currentNpc);
                ogame->GetGameWorld()->RemoveVob(currentNpc);

                ChatLog(string::Combine("%s was removed from the world!", string(currentNpc->GetObjectName())));
            }

            if (zinput->KeyToggled(ToggleGameLogKey)) {
                if (player->GetObjectName() == "PC_HERO") {
                    return;
                }

                Myself->npc->SetAsPlayer();
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
