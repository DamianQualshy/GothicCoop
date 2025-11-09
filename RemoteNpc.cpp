namespace GOTHIC_ENGINE {
    class RemoteNpc
    {
    public:
        string name;
        string playerNickname = "";
        int playerHeadVarNr;
        int playerBodyTextVarNr;
        oCNpc* npc = NULL;
        bool destroyed = false;
        bool isSpawned = false;
        bool hasNpc = false;
        bool hasModel = false;
        std::vector<json> localUpdates;

        zVEC3* lastPositionFromServer = NULL;
        float lastHeadingFromServer = -1;
        int lastHpFromServer = -1;
        int lastMaxHpFromServer = -1;
        int lastWeaponMode = -1;
        int lastWeapon1 = -1;
        int lastWeapon2 = -1;
        int lastArmor = -1;
        zSTRING lastSpellInstanceName;
        oCItem* spellItem;
        std::map<oCVob*, bool> syncedNpcItems;

        RemoteNpc(string playerName) {
            name = playerName;
        }

        void Update() {
            if (destroyed) {
                return;
            }

            UpdateHasNpcAndHasModel();
            //RemoveCoopItemsFromGround();
            RespawnOrDestroyBasedOnDistance();

            if (npc == NULL && UniqueNameToNpcList.count(name) > 0) {
                npc = UniqueNameToNpcList[name];
            }

            if (npc && IsPlayerTalkingWithNpc(npc)) {
                return;
            }

            if (IsNpcInTot()) {
                return;
            }

            for (unsigned int i = 0; i < localUpdates.size(); i++)
            {
                auto update = localUpdates.front();
                localUpdates.erase(localUpdates.begin());

                auto type = update["type"].get<int>();
                PluginState = "Updating NPC " + name + " TYPE: " + type;

                switch (type) {
                case INIT_NPC:
                {
                    UpdateInitialization(update);
                    break;
                }
                case SYNC_POS:
                {
                    UpdatePosition(update);
                    break;
                }
                case SYNC_HEADING:
                {
                    UpdateAngle(update);
                    break;
                }
                case SYNC_ANIMATION:
                {
                    UpdateAnimation(update);
                    break;
                }
                case SYNC_WEAPON_MODE:
                {
                    UpdateWeaponMode(update);
                    break;
                }
                case SYNC_HP:
                {
                    UpdateHp(update);
                    break;
                }
                case SYNC_TALENTS:
                {
                    UpdateTalents(update);
                    break;
                }
                case SYNC_PROTECTIONS:
                {
                    UpdateProtection(update);
                    break;
                }
                case SYNC_ARMOR:
                {
                    UpdateArmor(update);
                    break;
                }
                case SYNC_MAGIC_SETUP:
                {
                    UpdateMagicSetup(update);
                    break;
                }
                case SYNC_HAND: {
                    UpdateHand(update);
                    break;
                }
                case SYNC_WEAPONS:
                {
                    UpdateWeapons(update);
                    break;
                }
                case SYNC_SPELL_CAST:
                {
                    UpdateSpellCasts(update);
                    break;
                }
                case SYNC_ATTACKS:
                {
                    UpdateAttacks(update);
                    break;
                }
                case SYNC_TIME: {
                    UpdateTime(update);
                    break;
                }
                case SYNC_REVIVED: {
                    UpdateRevived(update);
                    break;
                }
                case DESTROY_NPC:
                {
                    DestroyNpc();
                    break;
                }
                case SYNC_BODYSTATE:
                {
                    UpdateBodystate(update);
                    break;
                }
                case SYNC_OVERLAYS:
                {
                    UpdateOverlays(update);
                    break;
                }
                case SYNC_DROPITEM:
                {
                    UpdateDropItem(update);
                    break;
                }
                case SYNC_TAKEITEM:
                {
                    UpdateTakeItem(update);
                    break;
                }

                }
            }

            PluginState = "UpdateSyncNpcs";
            UpdateNpcBasedOnLastDataFromServer();
        }

        void UpdateInitialization(json update) {
            if (npc == NULL) {
                auto x = update["x"].get<float>();
                auto y = update["y"].get<float>();
                auto z = update["z"].get<float>();
                auto nickname = update["nickname"].get<std::string>();
                auto headNumber = update["headVarNr"].get<int>();
                auto bodyNumber = update["bodyTextVarNr"].get<int>();

                playerNickname = nickname.c_str();
                playerHeadVarNr = headNumber;
                playerBodyTextVarNr = bodyNumber;
                lastPositionFromServer = new zVEC3(x, y, z);

                if (IsCoopPlayer(name)) {
                    InitCoopFriendNpc();
                    UpdateHasNpcAndHasModel();
                }
                else if (npc) {
                    ogame->spawnman->InsertNpc(npc, *lastPositionFromServer);
                }
            }
        }

        void UpdatePosition(json update) {
            auto x = update["x"].get<float>();
            auto y = update["y"].get<float>();
            auto z = update["z"].get<float>();

            if (CurrentWorldTOTPosition) {
                auto newPosition = zVEC3(x, y, z);
                auto totPos = *CurrentWorldTOTPosition;
                int dist = GetDistance3D(newPosition.n[0], newPosition.n[1], newPosition.n[2], totPos.n[0], totPos.n[1], totPos.n[2]);
                if (dist < 500) {
                    destroyed = true;
                    return;
                }
            }

            lastPositionFromServer = new zVEC3(x, y, z);
        }

        void UpdateAngle(json update) {
            auto h = update["h"].get<float>();
            lastHeadingFromServer = h;
            if (hasModel) {
                npc->ResetRotationsWorld();
                npc->RotateWorldY(h);
            }
        }

        void UpdateAnimation(json update) {
            if (hasModel) {
                auto a = update["a"].get<int>();
                npc->GetModel()->StartAni(a, COOP_MAGIC_NUMBER);
            }
        }

        void UpdateWeaponMode(json update) {
            if (hasModel) {
                auto wm = update["wm"].get<int>();
                lastWeaponMode = wm;
                npc->SetWeaponMode2(wm);

                if (!IsCoopPlayer(name)) {
                    npc->GetEM()->KillMessages();
                    npc->ClearEM();
                    npc->state.ClearAIState();
                }
                else {
                    // if coop friend is changing weapon mode while the game thinks IsUnconscious -> restart AI state
                    if (npc->IsUnconscious()) {
                        npc->GetEM()->KillMessages();
                        npc->ClearEM();
                        npc->state.ClearAIState();
                    }
                }
            }
        }

        void UpdateHp(json update) {
            auto hp = update["hp"].get<int>();
            auto hpMax = update["hp_max"].get<int>();

            if (!IsCoopPlayer(name) && hp == 0) {
                if (hasNpc) {
                    if (!npc->IsDead() && KilledByPlayerNpcNames.count(name) == 0) {
                        npc->SetAttribute(NPC_ATR_HITPOINTS, 1);
                    } else {
                        npc->SetAttribute(NPC_ATR_HITPOINTS, 0);
                    }
                }
                lastHpFromServer = -1;
                return;
            }

            lastHpFromServer = hp;
            lastMaxHpFromServer = hpMax;
            if (hasNpc) {
                npc->SetAttribute(NPC_ATR_HITPOINTS, hp);
                npc->SetAttribute(NPC_ATR_HITPOINTSMAX, hpMax);
            }
        }

        void UpdateTalents(json update) {
            auto t0 = update["t0"].get<int>();
            auto t1 = update["t1"].get<int>();
            auto t2 = update["t2"].get<int>();
            auto t3 = update["t3"].get<int>();

            if (hasNpc) {
                npc->SetTalentSkill(oCNpcTalent::NPC_TAL_1H, t0);
                npc->SetTalentSkill(oCNpcTalent::NPC_TAL_2H, t1);
                npc->SetTalentSkill(oCNpcTalent::NPC_TAL_BOW, t2);
                npc->SetTalentSkill(oCNpcTalent::NPC_TAL_CROSSBOW, t3);
            }
        }

        void UpdateProtection(json update) {
            auto p0 = update["p0"].get<int>();
            auto p1 = update["p1"].get<int>();
            auto p2 = update["p2"].get<int>();
            auto p3 = update["p3"].get<int>();
            auto p4 = update["p4"].get<int>();
            auto p5 = update["p5"].get<int>();
            auto p6 = update["p6"].get<int>();
            auto p7 = update["p7"].get<int>();

            if (hasNpc) {
                npc->SetProtectionByIndex(static_cast<oEIndexDamage>(0), p0);
                npc->SetProtectionByIndex(static_cast<oEIndexDamage>(1), p1);
                npc->SetProtectionByIndex(static_cast<oEIndexDamage>(2), p2);
                npc->SetProtectionByIndex(static_cast<oEIndexDamage>(3), p3);
                npc->SetProtectionByIndex(static_cast<oEIndexDamage>(4), p4);
                npc->SetProtectionByIndex(static_cast<oEIndexDamage>(5), p5);
                npc->SetProtectionByIndex(static_cast<oEIndexDamage>(6), p6);
                npc->SetProtectionByIndex(static_cast<oEIndexDamage>(7), p7);
            }
        }

        void UpdateArmor(json update) {
            if (hasNpc) {
                auto armor = update["armor"].get<std::string>();

                auto currentArmor = npc->GetEquippedArmor();
                if (currentArmor) {
                    lastArmor = -1;
                    npc->UnequipItem(currentArmor);
                }

                if (armor != "NULL") {
                    int insIndex = parser->GetIndex(armor.c_str());
                    if (insIndex > 0) {
                        auto newArmor = CreateCoopItem(insIndex);
                        if (newArmor) {
                            lastArmor = insIndex;
                            npc->Equip(newArmor);
                        }
                    }

                }
            }
        }

        void UpdateBodystate(json update) {
            if (hasNpc) {
                auto bs = update["bs"].get<int>();
                npc->SetBodyState(bs);
            }
        }

        void UpdateOverlays(json update) {
            if (hasNpc) {
                std::vector<json> overlays = update["overlays"];
                zCArray<int> overlaysNew;

                for each (auto a in overlays) {
                    auto overlayId = a["over"].get<int>();
                    overlaysNew.InsertEnd(overlayId);
                }

                if (!npc->CompareOverlaysArray(overlaysNew))
                {
                    npc->ApplyOverlaysArray(overlaysNew);
                }
            }
        }

        void UpdateMagicSetup(json update) {
            if (hasNpc && hasModel) {
                auto spellInstanceName = update["spell"].get<std::string>();
                oCMag_Book* book = npc->GetSpellBook();
                if (book)
                {
                    auto selectedSpell = book->GetSelectedSpell();
                    if (selectedSpell) {
                        auto selectedSpellItem = book->GetSpellItem(selectedSpell);
                        npc->DoDropVob(selectedSpellItem);
                        selectedSpellItem->RemoveVobFromWorld();
                        selectedSpell->Kill();
                    }

                    book->spellitems.EmptyList();
                    book->spells.EmptyList();
                }

                if (spellInstanceName.compare("NULL") != 0) {
                    int insIndex = parser->GetIndex(spellInstanceName.c_str());
                    if (insIndex > 0) {
                        auto spellItem = CreateCoopItem(insIndex);
                        if (spellItem) {
                            npc->DoPutInInventory(spellItem);
                            npc->Equip(spellItem);

                            oCMag_Book* book = npc->GetSpellBook();
                            if (book) {
                                book->Open(0);
                            }
                        }
                    }
                }
            }
        }

        void UpdateHand(json update) {
            if (!hasModel) {
                return;
            }
            auto leftItem = update["left"].get<std::string>();
            auto rightItem = update["right"].get<std::string>();

            auto leftHandItem = npc->GetLeftHand();
            if (leftHandItem)
            {
                npc->DoDropVob(leftHandItem);
                leftHandItem->RemoveVobFromWorld();
                syncedNpcItems.erase(leftHandItem);
            }

            if (leftItem != "NULL") {
                int insIndex = parser->GetIndex(leftItem.c_str());
                if (insIndex > 0) {
                    auto newItem = CreateCoopItem(insIndex);
                    if (newItem) {
                        syncedNpcItems[newItem] = true;
                        npc->SetLeftHand(newItem);
                    }
                }
            }

            auto rightHandItem = npc->GetRightHand();
            if (rightHandItem)
            {
                npc->DoDropVob(rightHandItem);
                rightHandItem->RemoveVobFromWorld();
                syncedNpcItems.erase(rightHandItem);
            }

            if (rightItem != "NULL") {
                int insIndex = parser->GetIndex(rightItem.c_str());
                if (insIndex > 0) {
                    auto newItem = CreateCoopItem(insIndex);
                    if (newItem) {
                        syncedNpcItems[newItem] = true;
                        npc->SetRightHand(newItem);
                    }
                }
            }
        }

        void UpdateWeapons(json update) {
            if (hasModel) {
                auto weapon1 = update["w1"].get<std::string>();
                auto weapon2 = update["w2"].get<std::string>();

                auto currentWeapon1 = npc->GetEquippedMeleeWeapon();
                auto currentWeapon2 = npc->GetEquippedRangedWeapon();

                if (currentWeapon1) {
                    npc->UnequipItem(currentWeapon1);
                    lastWeapon1 = -1;
                }

                if (currentWeapon2) {
                    npc->UnequipItem(currentWeapon2);
                    lastWeapon2 = -1;
                }

                if (weapon1 != "NULL") {
                    int insIndex = parser->GetIndex(weapon1.c_str());
                    if (insIndex > 0) {
                        auto newWeapon = CreateCoopItem(insIndex);
                        if (newWeapon) {
                            lastWeapon1 = insIndex;
                            npc->Equip(newWeapon);
                            syncedNpcItems[newWeapon] = true;
                        }
                    }

                }

                if (weapon2 != "NULL") {
                    int insIndex = parser->GetIndex(weapon2.c_str());
                    if (insIndex > 0) {
                        auto newWeapon = CreateCoopItem(insIndex);
                        if (newWeapon) {
                            lastWeapon2 = insIndex;
                            npc->Equip(newWeapon);
                            syncedNpcItems[newWeapon] = true;
                        }
                    }

                }
            }
        }

        void UpdateSpellCasts(json update) {
            if (!hasModel) {
                return;
            }

            oCMag_Book* book = npc->GetSpellBook();
            if (book)
            {
                auto selectedSpell = book->GetSelectedSpell();
                if (selectedSpell) {
                    std::vector<json> casts = update["casts"];
                    for each (auto c in casts) {
                        auto target = c["target"].get<std::string>();

                        if (!target.empty() && UniqueNameToNpcList.count(target.c_str()) > 0) {
                            book->Spell_Setup(0, npc, UniqueNameToNpcList[target.c_str()]);
                        }
                        else {
                            zCVob* nullVob = NULL;
                            book->Spell_Setup(0, npc, nullVob);
                        }

                        book->Spell_Invest();
                        book->Spell_Cast();
                    }
                }
            }
        }

        void UpdateAttacks(json update) {
            if (!hasModel) {
                return;
            }

            std::vector<json> atts = update["att"];
            for each (auto a in atts) {
                auto target = a["target"].get<std::string>();
                auto damage = a["damage"].get<float>();
                auto isUnconscious = a["isUnconscious"].get<int>();
                auto stillAlive = !a["isDead"].get<bool>();
                auto damageMode = a["damageMode"].get<unsigned long>();

                // attack player (client only, eg. wolf attacks player)
                if (target.compare(MyselfId) == 0) {
                    auto targetNpc = player;
                    int health = targetNpc->GetAttribute(NPC_ATR_HITPOINTS);

                    if (player->GetAnictrl()->CanParade(npc)) {
                        break;
                    }

                    if (isUnconscious && stillAlive) {
                        targetNpc->SetWeaponMode2(NPC_WEAPON_NONE);
                        targetNpc->DropUnconscious(1, npc);
                    }

                    if (stillAlive) {
                        targetNpc->SetAttribute(NPC_ATR_HITPOINTS, 999999);
                        targetNpc->GetEM(false)->OnDamage(targetNpc, npc, COOP_MAGIC_NUMBER, damageMode, targetNpc->GetPositionWorld());
                        targetNpc->SetAttribute(NPC_ATR_HITPOINTS, health - damage);
                    }
                    else {
                        targetNpc->SetAttribute(NPC_ATR_HITPOINTS, 1);
                        targetNpc->GetEM(false)->OnDamage(targetNpc, npc, COOP_MAGIC_NUMBER, damageMode, targetNpc->GetPositionWorld());
                    }

                    break;
                }

                // attack other coop player (client only, eg. wolf attacks host)
                if (IsCoopPlayer(target) && PlayerNameToNpc.count(target.c_str())) {
                    auto targetNpc = PlayerNameToNpc[target.c_str()];
                    int health = targetNpc->GetAttribute(NPC_ATR_HITPOINTS);

                    if (isUnconscious && stillAlive) {
                        targetNpc->SetWeaponMode2(NPC_WEAPON_NONE);
                        targetNpc->DropUnconscious(1, npc);
                    }

                    if (stillAlive) {
                        targetNpc->SetAttribute(NPC_ATR_HITPOINTS, 999999);
                        targetNpc->GetEM(false)->OnDamage(targetNpc, npc, 1, damageMode, targetNpc->GetPositionWorld());
                        targetNpc->SetAttribute(NPC_ATR_HITPOINTS, health);
                    }
                    else {
                        targetNpc->SetAttribute(NPC_ATR_HITPOINTS, 0);
                        targetNpc->DoDie(npc);
                    }

                    break;
                }

                // attack any world npc (eg. client attacks Moe, Cavalorn attacks goblin, wolf attacks sheep)
                auto targetNpc = UniqueNameToNpcList[target.c_str()];
                if (targetNpc) {
                    int health = targetNpc->GetAttribute(NPC_ATR_HITPOINTS);
                    auto isTalkingWith = IsPlayerTalkingWithNpc(targetNpc);
                    static int AIV_PARTYMEMBER = GetPartyMemberID();

                    if (isUnconscious && stillAlive) {
                        targetNpc->SetWeaponMode2(NPC_WEAPON_NONE);

                        if (IsCoopPlayer(npc->GetObjectName()) || npc->aiscriptvars[AIV_PARTYMEMBER] == True) {
                            targetNpc->DropUnconscious(1, player);
                        }
                        else {
                            targetNpc->DropUnconscious(1, npc);
                        }
                    }

                    if (stillAlive) {
                        if (!isTalkingWith && !isUnconscious) {
                            targetNpc->SetAttribute(NPC_ATR_HITPOINTS, 999999);
                            targetNpc->GetEM(false)->OnDamage(targetNpc, npc, COOP_MAGIC_NUMBER, damageMode, targetNpc->GetPositionWorld());
                        }
                        if (ServerThread) {
                            targetNpc->SetAttribute(NPC_ATR_HITPOINTS, health - damage > 0 ? health - damage : 0);
                        }
                    }
                    else {
                        targetNpc->SetAttribute(NPC_ATR_HITPOINTS, 1);

                        if (IsCoopPlayer(npc->GetObjectName()) || npc->aiscriptvars[AIV_PARTYMEMBER] == True) {
                            targetNpc->OnDamage(targetNpc, player, COOP_MAGIC_NUMBER, damageMode, targetNpc->GetPositionWorld());
                        }

                        if (SyncNpcs.count(target.c_str()) > 0) {
                            auto syncedKilledNpc = SyncNpcs[target.c_str()];
                            syncedKilledNpc->lastHpFromServer = -1;
                        }

                        targetNpc->SetAttribute(NPC_ATR_HITPOINTS, 0);
                    }
                }
            }
        }

        void UpdateTime(json update) {
            if (!IsPlayerTalkingWithAnybody()) {
                auto h = update["h"].get<int>();
                auto m = update["m"].get<int>();
                ogame->GetWorldTimer()->SetTime(h, m);
            }
        }

        void UpdateRevived(json update) {
            auto name = update["name"].get<std::string>();

            if (player->IsDead() && name.compare(MyselfId) == 0) {
                player->StopFaceAni("T_HURT");
                player->SetWeaponMode2(NPC_WEAPON_NONE);
                player->ResetPos(player->GetPositionWorld());
                player->SetAttribute(NPC_ATR_HITPOINTS, 1);
                parser->CallFuncByName("RX_Mult_ReviveHero");
            }
        }

        void UpdateDropItem(json update) {
            if (!hasModel) {
                return;
            }

            auto itemName = update["itemDropped"].get<std::string>();
            auto count = update["count"].get<int>();
            auto flags = update["flags"].get<int>();
            auto itemUniqName = update["itemUniqName"].get<std::string>();

            int index = parser->GetIndex(itemName.c_str());

            if (index != -1)
            {
                oCItem* item = CreateCoopItem(index);
                
                if (item)
                {
                    item->amount = count;
                    item->SetObjectName(itemUniqName.c_str());

                    npc->DoPutInInventory(item);
                    npc->DoDropVob(item);
                }
            }

        }

        void UpdateTakeItem(json update) {
            if (!hasModel) {
                return;
            }

            auto itemName = update["itemDropped"].get<std::string>();
            auto count = update["count"].get<int>();
            auto flags = update["flags"].get<int>();
            auto x = update["x"].get<float>();
            auto y = update["y"].get<float>();
            auto z = update["z"].get<float>();
            auto uniqName = update["uniqName"].get<std::string>();
            auto itemPos = zVEC3(x, y, z);

            auto pList = CollectVobsInRadius(itemPos, 2500);
            int index = parser->GetIndex(itemName.c_str());

            if (index == -1) {
                return;
            }

            for (int i = 0; i < pList.GetNumInList(); i++)
            {
                if (auto pVob = pList.GetSafe(i))
                {
                    if (auto pItem = pVob->CastTo<oCItem>())
                    {
                        if (pItem->GetInstance() == index && pItem->GetObjectName() == uniqName.c_str())
                        {
                            pItem->RemoveVobFromWorld();
                            break;
                        }
                    }
                }
            }
        }

        void DestroyNpc() {
            if (npc != NULL) {
                PlayerNpcs.erase(npc);
                PlayerNameToNpc.erase(name);

                ogame->spawnman->DeleteNpc(npc);
                destroyed = true;
                npc = NULL;
            }
        }

        void UpdateNpcBasedOnLastDataFromServer() {
            if (npc && hasModel) {
                if (lastPositionFromServer) {
                    UpdateNpcPosition();
                }

                if (lastHpFromServer != -1 && lastHpFromServer != npc->GetAttribute(NPC_ATR_HITPOINTS)) {
                    if (!npc->IsDead() || IsCoopPlayer(name)) {
                        npc->SetAttribute(NPC_ATR_HITPOINTS, lastHpFromServer);
                    }
                }

                if (lastWeaponMode != -1 && lastWeaponMode != npc->GetWeaponMode()) {
                    npc->SetWeaponMode2(lastWeaponMode);
                }

                if (lastHeadingFromServer != -1) {
                    npc->ResetRotationsWorld();
                    npc->RotateWorldY(lastHeadingFromServer);
                }

                if (IsCoopPlayer(name)) {
                    static int AIV_PARTYMEMBER = GetPartyMemberID();
                    npc->aiscriptvars[AIV_PARTYMEMBER] = True;
                }
            }
        }

        void UpdateNpcPosition() {
            auto currentPosition = npc->GetPositionWorld();
            auto dist = (int)(*lastPositionFromServer - currentPosition).LengthApprox();
            auto pos = *lastPositionFromServer;

            if (dist < 200) {
                npc->SetCollDet(FALSE);
                npc->SetPositionWorld(pos);
                npc->SetCollDet(TRUE);

                return;
            }

            bool inMove = npc->isInMovementMode;
            if (inMove) {
#if ENGINE >= Engine_G2
                npc->EndMovement(false);
#else
                npc->EndMovement();
#endif
            }

            npc->SetCollDet(FALSE);
            npc->trafoObjToWorld.SetTranslation(pos);
            npc->SetCollDet(TRUE);

            if (inMove) {
                npc->BeginMovement();
            }
        }

        void UpdateHasNpcAndHasModel() {
            hasNpc = npc != NULL;
            hasModel = npc && npc->GetModel() && npc->vobLeafList.GetNum() > 0;
        }

        void ReinitCoopFriendNpc() {
            if (npc) {
                PlayerNpcs.erase(npc);
                PlayerNameToNpc.erase(name);
                ogame->spawnman->DeleteNpc(npc);
                npc = NULL;
                InitCoopFriendNpc();
            }
        }

        void InitCoopFriendNpc() {
            int instanceId = GetFriendDefaultInstanceId();
            if (instanceId <= 0) {
                ChatLog("Invalid NPC instance id.");
                return;
            }
            if (!npc) {
                npc = dynamic_cast<oCNpc*>(ogame->GetGameWorld()->CreateVob(zTVobType::zVOB_TYPE_NSC, instanceId));
            }

            ogame->spawnman->InsertNpc(npc, *lastPositionFromServer);
            isSpawned = true;
            npc->name[0] = playerNickname.IsEmpty() ? zSTRING(name) : playerNickname;

            npc->SetObjectName(name);
            npc->SetVobName(name);
            npc->SetVobPresetName(name);
            npc->MakeSpellBook();

            npc->UseStandAI();
            npc->dontWriteIntoArchive = TRUE;
            npc->idx = 69133769;

            npc->SetAdditionalVisuals(zSTRING("hum_body_Naked0"), playerBodyTextVarNr, DefaultBodyTexColorNr, zSTRING("HUM_HEAD_PONY"), playerHeadVarNr, 0, -1);

#if ENGINE >= Engine_G2
            npc->SetHitChance(1, 100);
            npc->SetHitChance(2, 100);
            npc->SetHitChance(3, 100);
            npc->SetHitChance(4, 100);
#endif
            if (lastMaxHpFromServer != -1) {
                npc->SetAttribute(NPC_ATR_HITPOINTSMAX, lastMaxHpFromServer);
            }
            npc->SetAttribute(NPC_ATR_STRENGTH, COOP_MAGIC_NUMBER);
            npc->SetAttribute(NPC_ATR_DEXTERITY, COOP_MAGIC_NUMBER);
            npc->SetAttribute(NPC_ATR_MANA, 10000);
            npc->SetAttribute(NPC_ATR_MANAMAX, 10000);

            auto armor = npc->GetEquippedArmor();
            if (armor) {
                npc->UnequipItem(armor);
            }

            auto weapon1 = npc->GetEquippedMeleeWeapon();
            if (weapon1) {
                npc->UnequipItem(weapon1);
            }

            auto weapon2 = npc->GetEquippedRangedWeapon();
            if (weapon2) {
                npc->UnequipItem(weapon2);
            }

            if (lastWeapon1 > 0) {
                auto weapon = CreateCoopItem(lastWeapon1);
                npc->Equip(weapon);
            }

            if (lastWeapon2 > 0) {
                auto weapon = CreateCoopItem(lastWeapon2);
                npc->Equip(weapon);
            }

            if (lastArmor > 0) {
                auto armor = CreateCoopItem(lastArmor);
                npc->Equip(armor);
            }

            if (lastWeaponMode > 0) {
                npc->SetWeaponMode2(lastWeaponMode);
            }

            if (IsCoopPlayer(name)) {
                static int AIV_PARTYMEMBER = GetPartyMemberID();
                npc->aiscriptvars[AIV_PARTYMEMBER] = True;
            }

            PlayerNpcs[npc] = name;
            PlayerNameToNpc[name] = npc;
        }

        void RemoveCoopItemsFromGround() {
            if (hasNpc && syncedNpcItems.size() > 0) {
                auto rightItem = npc->GetRightHand();
                auto leftItem = npc->GetLeftHand();
                auto currentWeapon1 = npc->GetEquippedMeleeWeapon();
                auto currentWeapon2 = npc->GetEquippedRangedWeapon();

                for (auto syncedItem : syncedNpcItems) {
                    auto item = syncedItem.first;

                    if (!item) {
                        syncedNpcItems.erase(item);
                    }

                    if (item != leftItem && item != rightItem && item != currentWeapon1 && item != currentWeapon2) {
                        item->RemoveVobFromWorld();
                        syncedNpcItems.erase(item);
                    }
                }
            }
        }

        void RespawnOrDestroyBasedOnDistance() {
            if (hasNpc && lastPositionFromServer) {
                auto dist = (int)(*lastPositionFromServer - player->GetPositionWorld()).LengthApprox();

                if (IsCoopPlayer(name)) {
                    if (dist > BROADCAST_DISTANCE && isSpawned) {
                        ogame->spawnman->DeleteNpc(npc);
                        isSpawned = false;
                    }
                    if (dist < BROADCAST_DISTANCE && (!isSpawned || !hasModel)) {
                        InitCoopFriendNpc();
                        isSpawned = true;
                    }
                }
                else if (dist > BROADCAST_DISTANCE * 1.5) {
                    destroyed = true;
                    return;
                }
                else if (dist < BROADCAST_DISTANCE && !hasModel) {
                    ogame->spawnman->InsertNpc(npc, *lastPositionFromServer);
                }
            }
        }

        bool IsNpcInTot() {
            if (lastPositionFromServer && CurrentWorldTOTPosition && npc && !IsCoopPlayer(name)) {
                auto newPosition = npc->GetPositionWorld();
                auto totPos = *CurrentWorldTOTPosition;
                int dist = GetDistance3D(newPosition.n[0], newPosition.n[1], newPosition.n[2], totPos.n[0], totPos.n[1], totPos.n[2]);
                if (dist < 500) {
                    destroyed = true;
                    return true;
                }
            }

            return false;
        }
    };
}