namespace GOTHIC_ENGINE {
    float GetVec3LengthApprox(const zVEC3& vec) {
    #ifdef __G1A
            return vec.Length();
    #else
            return vec.LengthApprox();
    #endif
    }

    class RemoteNpc
    {
    public:
        string name;
        string playerNickname = "";
        std::string playerBodyModel;
        int playerHeadTex;
        int playerBodyTex;
        int playerBodyColor = 0;
        std::string playerHeadModel;
        oCNpc* npc = NULL;
        bool destroyed = false;
        bool isSpawned = false;
        bool hasNpc = false;
        bool hasModel = false;
        std::vector<PlayerStateUpdatePacket> localUpdates;

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

        ~RemoteNpc() {
            delete lastPositionFromServer;
            lastPositionFromServer = NULL;
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

            while (!localUpdates.empty()) {
                auto update = localUpdates.front();
                localUpdates.erase(localUpdates.begin());

                auto type = update.updateType;
                PluginState = "Updating NPC " + name + " TYPE: " + static_cast<int>(type);

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

        void UpdateInitialization(const PlayerStateUpdatePacket& update) {
            if (npc == NULL) {
                auto x = update.initNpc.x;
                auto y = update.initNpc.y;
                auto z = update.initNpc.z;
                auto nickname = update.initNpc.nickname;
                auto bodyModel = update.initNpc.bodyModel;
                auto headNumber = update.initNpc.HeadTex;
                auto bodyNumber = update.initNpc.BodyTex;
                auto bodyColor = update.initNpc.BodyColor;
                auto headModel = update.initNpc.headModel;

                playerNickname = nickname.c_str();
                playerBodyModel = bodyModel;
                playerHeadTex = headNumber;
                playerBodyTex = bodyNumber;
                playerBodyColor = bodyColor;
                playerHeadModel = headModel;
                delete lastPositionFromServer;
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

        void UpdatePosition(const PlayerStateUpdatePacket& update) {
            auto x = update.pos.x;
            auto y = update.pos.y;
            auto z = update.pos.z;

            if (CurrentWorldTOTPosition) {
                auto newPosition = zVEC3(x, y, z);
                auto totPos = *CurrentWorldTOTPosition;
                float dist = GetDistance3D(newPosition.n[0], newPosition.n[1], newPosition.n[2], totPos.n[0], totPos.n[1], totPos.n[2]);
                if (dist < 500.0f) {
                    destroyed = true;
                    return;
                }
            }

            delete lastPositionFromServer;
            lastPositionFromServer = new zVEC3(x, y, z);
        }

        void UpdateAngle(const PlayerStateUpdatePacket& update) {
            auto h = update.heading.heading;
            lastHeadingFromServer = h;
            if (hasModel) {
                npc->ResetRotationsWorld();
                npc->RotateWorldY(h);
            }
        }

        void UpdateAnimation(const PlayerStateUpdatePacket& update) {
            if (hasModel) {
                auto a = update.animation.animationId;
                npc->GetModel()->StartAni(a, COOP_MAGIC_NUMBER);
            }
        }

        void UpdateWeaponMode(const PlayerStateUpdatePacket& update) {
            if (hasModel) {
                auto wm = update.weaponMode.weaponMode;
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

        void UpdateHp(const PlayerStateUpdatePacket& update) {
            auto hp = update.hp.hp;
            auto hpMax = update.hp.hpMax;

            if (!IsCoopPlayer(name) && hp == 0) {
                if (hasNpc) {
                    if (!IsNpcDead(npc) && KilledByPlayerNpcNames.count(name) == 0) {
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

        void UpdateTalents(const PlayerStateUpdatePacket& update) {
            auto t0 = update.talents.talents[0];
            auto t1 = update.talents.talents[1];
            auto t2 = update.talents.talents[2];
            auto t3 = update.talents.talents[3];

            if (hasNpc) {
                npc->SetTalentSkill(oCNpcTalent::NPC_TAL_1H, t0);
                npc->SetTalentSkill(oCNpcTalent::NPC_TAL_2H, t1);
                npc->SetTalentSkill(oCNpcTalent::NPC_TAL_BOW, t2);
                npc->SetTalentSkill(oCNpcTalent::NPC_TAL_CROSSBOW, t3);
            }
        }

        void UpdateProtection(const PlayerStateUpdatePacket& update) {
            auto p0 = update.protections.protections[0];
            auto p1 = update.protections.protections[1];
            auto p2 = update.protections.protections[2];
            auto p3 = update.protections.protections[3];
            auto p4 = update.protections.protections[4];
            auto p5 = update.protections.protections[5];
            auto p6 = update.protections.protections[6];
            auto p7 = update.protections.protections[7];

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

        void UpdateArmor(const PlayerStateUpdatePacket& update) {
            if (hasNpc) {
                auto armor = update.armor.armor;

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

        void UpdateBodystate(const PlayerStateUpdatePacket& update) {
            if (hasNpc) {
                auto bs = update.bodyState.bodyState;
                npc->SetBodyState(bs);
            }
        }

        void UpdateOverlays(const PlayerStateUpdatePacket& update) {
            if (hasNpc) {
                zCArray<int> overlaysNew;

                for (auto overlayId : update.overlays.overlayIds) {
                    overlaysNew.InsertEnd(overlayId);
                }

                if (!npc->CompareOverlaysArray(overlaysNew))
                {
                    npc->ApplyOverlaysArray(overlaysNew);
                }
            }
        }

        void UpdateMagicSetup(const PlayerStateUpdatePacket& update) {
            if (hasNpc && hasModel) {
                auto spellInstanceName = update.magicSetup.spellInstanceName;
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

        void EnsureSpellSetup(int spellInstanceId) {
            if (!hasNpc || !hasModel || spellInstanceId <= 0) {
                return;
            }

            oCMag_Book* book = npc->GetSpellBook();
            if (!book) {
                return;
            }

            auto selectedSpell = book->GetSelectedSpell();
            if (selectedSpell) {
                auto selectedSpellItem = book->GetSpellItem(selectedSpell);
                if (selectedSpellItem) {
                    int currentInstanceId = parser->GetIndex(selectedSpellItem->GetInstanceName());
                    if (currentInstanceId == spellInstanceId) {
                        return;
                    }
                }
            }

            if (selectedSpell) {
                auto selectedSpellItem = book->GetSpellItem(selectedSpell);
                if (selectedSpellItem) {
                    npc->DoDropVob(selectedSpellItem);
                    selectedSpellItem->RemoveVobFromWorld();
                }
                selectedSpell->Kill();
            }

            book->spellitems.EmptyList();
            book->spells.EmptyList();

            auto spellItem = CreateCoopItem(spellInstanceId);
            if (spellItem) {
                npc->DoPutInInventory(spellItem);
                npc->Equip(spellItem);

                oCMag_Book* refreshedBook = npc->GetSpellBook();
                if (refreshedBook) {
                    refreshedBook->Open(0);
                }
            }
        }

        void UpdateHand(const PlayerStateUpdatePacket& update) {
            if (!hasModel) {
                return;
            }
            auto leftItem = update.hand.leftItem;
            auto rightItem = update.hand.rightItem;

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

        void UpdateWeapons(const PlayerStateUpdatePacket& update) {
            if (hasModel) {
                auto weapon1 = update.weapons.weapon1;
                auto weapon2 = update.weapons.weapon2;

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

        void UpdateSpellCasts(const PlayerStateUpdatePacket& update) {
            if (!hasModel) {
                return;
            }

            oCMag_Book* book = npc->GetSpellBook();
            if (book)
            {
                for (const auto& c : update.spellCasts.casts) {
                    auto target = c.target;
                    auto spellInstanceId = c.spellInstanceId;
                    auto spellLevel = c.spellLevel;
                    auto spellCharge = c.spellCharge;

                    EnsureSpellSetup(spellInstanceId);
                    book = npc->GetSpellBook();
                    if (!book) {
                        continue;
                    }

                    auto selectedSpell = book->GetSelectedSpell();
                    if (!selectedSpell) {
                        continue;
                    }

                    selectedSpell->spellLevel = spellLevel;
                    selectedSpell->SetInvestedMana(spellCharge);

                    int spellIndex = book->GetSelectedSpellNr();
                    if (spellIndex < 0) {
                        spellIndex = 0;
                    }

                    if (!target.empty() && UniqueNameToNpcList.count(target.c_str()) > 0) {
                        book->Spell_Setup(spellIndex, npc, UniqueNameToNpcList[target.c_str()]);
                    }
                    else {
                        zCVob* nullVob = NULL;
                        book->Spell_Setup(spellIndex, npc, nullVob);
                    }

                    book->Spell_Invest();
                    book->Spell_Cast();
                }
            }
        }

        void UpdateAttacks(const PlayerStateUpdatePacket& update) {
            if (!hasModel) {
                return;
            }

            for (const auto& a : update.attacks.attacks) {
                auto target = a.target;
                auto damage = a.damage;
                auto isUnconscious = a.isUnconscious;
                auto isFinish = a.isFinish;
                auto stillAlive = !a.isDead;
                auto damageMode = a.damageMode;

                // attack player (client only, eg. wolf attacks player)
                if (target.compare(MyselfId) == 0) {
                    auto targetNpc = player;
                    int health = targetNpc->GetAttribute(NPC_ATR_HITPOINTS);

                    if (player->GetAnictrl()->CanParade(npc)) {
                        break;
                    }

                    if (isUnconscious) {
                        targetNpc->SetWeaponMode2(NPC_WEAPON_NONE);
                        targetNpc->DropUnconscious(1, npc);
                    }

                    if (isFinish) {
                        break;
                    }

                    if (stillAlive) {
                        targetNpc->SetAttribute(NPC_ATR_HITPOINTS, 999999);
                        targetNpc->GetEM(false)->OnDamage(targetNpc, npc, COOP_MAGIC_NUMBER, damageMode, targetNpc->GetPositionWorld());
                        targetNpc->SetAttribute(NPC_ATR_HITPOINTS, health - static_cast<int>(damage));
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

                    if (isUnconscious) {
                        targetNpc->SetWeaponMode2(NPC_WEAPON_NONE);
                        targetNpc->DropUnconscious(1, npc);
                    }

                    if (isFinish) {
                        break;
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
                auto targetNpcEntry = UniqueNameToNpcList.find(target.c_str());
                if (targetNpcEntry == UniqueNameToNpcList.end()) {
                    BuildGlobalNpcList();
                    targetNpcEntry = UniqueNameToNpcList.find(target.c_str());
                }

                auto targetNpc = targetNpcEntry != UniqueNameToNpcList.end() ? targetNpcEntry->second : nullptr;
                if (targetNpc) {
                    int health = targetNpc->GetAttribute(NPC_ATR_HITPOINTS);
                    auto isTalkingWith = IsPlayerTalkingWithNpc(targetNpc);
                    static int AIV_PARTYMEMBER = GetPartyMemberID();

                    if (isUnconscious) {
                        targetNpc->SetWeaponMode2(NPC_WEAPON_NONE);

                        if (IsCoopPlayer(npc->GetObjectName()) || npc->aiscriptvars[AIV_PARTYMEMBER] == True) {
                            targetNpc->DropUnconscious(1, player);
                        }
                        else {
                            targetNpc->DropUnconscious(1, npc);
                        }
                    }

                    if (isFinish) {
                        continue;
                    }

                    if (stillAlive) {
                        if (!isTalkingWith && !isUnconscious) {
                            targetNpc->SetAttribute(NPC_ATR_HITPOINTS, 999999);
                            targetNpc->GetEM(false)->OnDamage(targetNpc, npc, COOP_MAGIC_NUMBER, damageMode, targetNpc->GetPositionWorld());
                        }
                        if (ServerThread) {
                            const int remainingHealth = health - static_cast<int>(damage);
                            targetNpc->SetAttribute(NPC_ATR_HITPOINTS, remainingHealth > 0 ? remainingHealth : 0);
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

        void UpdateTime(const PlayerStateUpdatePacket& update) {
            if (!IsPlayerTalkingWithAnybody()) {
                auto h = update.time.hour;
                auto m = update.time.minute;
                ogame->GetWorldTimer()->SetTime(h, m);
            }
        }

        void UpdateRevived(const PlayerStateUpdatePacket& update) {
            auto name = update.revived.name;

            if (IsNpcDead(player) && name.compare(MyselfId) == 0) {
                player->StopFaceAni("T_HURT");
                player->SetWeaponMode2(NPC_WEAPON_NONE);
                player->ResetPos(player->GetPositionWorld());
                player->SetAttribute(NPC_ATR_HITPOINTS, 1);
                parser->CallFuncByName("RX_Mult_ReviveHero");
            }
        }

        void UpdateDropItem(const PlayerStateUpdatePacket& update) {
            if (!hasModel) {
                return;
            }

            auto itemName = update.dropItem.itemDropped;
            auto count = update.dropItem.count;
            auto flags = update.dropItem.flags;
            auto itemUniqName = update.dropItem.itemUniqueName;

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

        void UpdateTakeItem(const PlayerStateUpdatePacket& update) {
            if (!hasModel) {
                return;
            }

            auto itemName = update.takeItem.itemDropped;
            auto count = update.takeItem.count;
            auto flags = update.takeItem.flags;
            auto x = update.takeItem.x;
            auto y = update.takeItem.y;
            auto z = update.takeItem.z;
            auto uniqName = update.takeItem.uniqueName;
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
                    if (!IsNpcDead(npc) || IsCoopPlayer(name)) {
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
            auto dist = static_cast<int>(GetVec3LengthApprox(*lastPositionFromServer - currentPosition));
            auto pos = *lastPositionFromServer;

            if (dist < 200) {
                npc->SetCollDet(FALSE);
                npc->SetPositionWorld(pos);
                npc->SetCollDet(TRUE);

                return;
            }

            bool inMove = npc->isInMovementMode;
            if (inMove) {
#ifdef __G2A
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

            const char* bodyModelName = playerBodyModel.empty() ? MyBodyModel.c_str() : playerBodyModel.c_str();
            const char* headModelName = playerHeadModel.empty() ? MyHeadModel.c_str() : playerHeadModel.c_str();
            int bodyColor = playerBodyColor;

            npc->SetAdditionalVisuals(zSTRING(bodyModelName), playerBodyTex, bodyColor, zSTRING(headModelName), playerHeadTex, 0, -1);

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
                float dist = GetVec3LengthApprox(*lastPositionFromServer - player->GetPositionWorld());

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
                float dist = GetDistance3D(newPosition.n[0], newPosition.n[1], newPosition.n[2], totPos.n[0], totPos.n[1], totPos.n[2]);
                if (dist < 500.0f) {
                    destroyed = true;
                    return true;
                }
            }

            return false;
        }
    };
}