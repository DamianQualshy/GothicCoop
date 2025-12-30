namespace GOTHIC_ENGINE {
    string ReviveState = "READY";
    oCNpc* ReviveNpc;
    long long ReviveStartTime = 0;

    void ReviveFriendLoop() {
        PluginState = "ReviveFriendLoop";

        if (!ServerThread && !ClientThread) {
            return;
        }

        if (ReviveStartTime + 10000 < CurrentMs && ReviveState.Compare("HEALING")) {
            if (IsNpcDead(player) || player->IsUnconscious()) {
                ReviveStartTime = 0;
                ReviveState = "READY";
                return;
            }

            Myself->SyncRevived(ReviveNpc->GetObjectName());

            if (ReviveNpc) {
                ReviveNpc->StopFaceAni("T_HURT");
                ReviveNpc->SetWeaponMode2(NPC_WEAPON_NONE);
            }

            ReviveStartTime = 0;
            ReviveState = "READY";
            ReviveNpc = NULL;
            ChatLog("Your friend is back alive!");
            return;
        }

        if (ReviveState.Compare("HEALING") && ReviveNpc && ReviveNpc->GetPositionWorld().Distance(player->GetPositionWorld()) > 200) {
            ReviveStartTime = 0;
            ReviveState = "READY";

            ChatLog("You cannot move while reviving your friend.");
            return;
        }

        if (zinput->KeyToggled(RevivePlayerKey)) {
            auto focusedNpc = player->GetFocusNpc();
            if (!focusedNpc || !IsCoopPlayer(focusedNpc->GetObjectName()))
            {
                return;
            }

            ReviveNpc = focusedNpc;

            if (ReviveState.Compare("READY")) {
                auto frinedHp = ReviveNpc->GetAttribute(NPC_ATR_HITPOINTS);
                if (frinedHp != 0) {
                    ChatLog("You cannot revive your friend when alive.");
                    ReviveNpc = NULL;
                    return;
                }

                if (ReviveNpc->GetPositionWorld().Distance(player->GetPositionWorld()) > 200) {
                    ChatLog("You are too far away to revive your friend.");
                    ReviveNpc = NULL;
                    return;
                }

                ChatLog("You are reviving your friend, please do not move for 10 seconds.");
                ReviveState = "HEALING";
                ReviveStartTime = CurrentMs;
                player->GetModel()->StartAni("T_PLUNDER", 1);
            }
            else if (ReviveState.Compare("HEALING")) {
                ChatLog("You already reviving your friend, please wait.");
            }
        }
    }
}