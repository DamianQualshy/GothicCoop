namespace GOTHIC_ENGINE {
    class LocalNpc
    {
    public:
        oCNpc* npc;
        string name;
        bool initialized = false;
        bool destroyed = false;
        bool hasModel = false;
        std::list<int> pendingUpdates;
        std::list<PlayerHit> hitsToSync;
        std::list<SpellCast> spellCastsToSync;
        std::vector<int> newAnimIds;
        zCModelAni* lastAnimation;
        zCArray<int> pArrOverlays;
        zVEC3 lastPosition;
        float lastHeading = 0;
        int lastWeaponMode;
        int lastSyncHp = -1;
        int lastSyncMaxHp;
        int lastProtections[8];
        int lastTalents[4];
        int lastBodyState;
        zSTRING lastSpellInstanceName = "NULL";
        zSTRING lastWeapon1Name;
        zSTRING lastWeapon2Name;
        zSTRING lastArmorName;
        zSTRING lastLeftHandInstanceName;
        zSTRING lastRightHandInstanceName;
        zSTRING pendingLeftHandInstanceName;
        zSTRING pendingRightHandInstanceName;
        zSTRING revivedFriend = "";
        long long lastTimeSyncTime = 0;
        long long lastHandChangeTime = 0;
        oCItem* pItemDropped = NULL;
        oCItem* pItemTaken = NULL;
        bool itemDropReady = false;
        zVEC3 pItemTakenPos;

        LocalNpc(oCNpc* _npc, string _name) {
            npc = _npc;
            name = _name;
        }

        void Pulse() {
            if (destroyed) {
                return;
            }

            hasModel = npc && npc->GetModel() && npc->vobLeafList.GetNum() > 0;
 
            this->SyncInitialization();
            //this->SyncBodystate();
            this->SyncPosition();
            this->SyncAngle();
            this->SyncOverlays();
            this->SyncAnimation();
            this->SyncWeaponMode();
            this->SyncAttacks();
            this->SyncSpellCasts();
            this->SyncMagicSetup();
            this->SyncHp();
            

            if (npc == player) {
                this->SyncArmor();
                this->SyncWeapons();
                this->SyncProtection();
                this->SyncTalents();
                this->SyncHand();

                if (ServerThread) {
                    if (CurrentMs > lastTimeSyncTime + 60000) {
                        this->SyncTime();
                        lastTimeSyncTime = CurrentMs;
                    }
                }
            }
        }

        void Reinit() {
            initialized = false;
            lastPosition = NULL;
            lastHeading = 0;
            lastWeaponMode = 0;
            lastSyncHp = -1;
            lastSyncMaxHp = 0;
            lastBodyState = 0;
            lastWeapon1Name = zSTRING();
            lastWeapon2Name = zSTRING();
            lastArmorName = zSTRING();
            lastLeftHandInstanceName = zSTRING();
            lastRightHandInstanceName = zSTRING();
            pendingLeftHandInstanceName = zSTRING();
            pendingRightHandInstanceName = zSTRING();
            lastSpellInstanceName = "NULL";
            lastTimeSyncTime = 0;
            lastHandChangeTime = 0;
            lastProtections[0] = -1;
            lastProtections[1] = -1;
            lastProtections[2] = -1;
            lastProtections[3] = -1;
            lastProtections[4] = -1;
            lastProtections[5] = -1;
            lastProtections[6] = -1;
            lastProtections[7] = -1;
            lastTalents[0] = 0;
            lastTalents[1] = 0;
            lastTalents[2] = 0;
            lastTalents[3] = 0;
            pItemDropped = NULL;
            itemDropReady = false;
            pItemTaken = NULL;
            pItemTakenPos = zVEC3(0, 0, 0);
            pArrOverlays.DeleteList();
            
            if (lastAnimation) {
                newAnimIds.push_back(lastAnimation->aniID);
            }
        }

        void SyncInitialization() {
            if (!initialized) {
                addUpdate(INIT_NPC);
                initialized = true;
            }
        }

        void SyncPosition()
        {
            zVEC3 playerPos = npc->GetPositionWorld();

            float dist = lastPosition != NULL ?
                GetDistance3D(playerPos.n[0], playerPos.n[1], playerPos.n[2], lastPosition.n[0], lastPosition.n[1], lastPosition.n[2]) :
                999.0f;

            if (dist > 5.0f)
            {
                addUpdate(SYNC_POS);
                lastPosition = playerPos;
            }
        };

        void SyncAngle() {
            float currentHeading = GetHeading(npc);
            if (abs(currentHeading - lastHeading) > 3)
            {
                addUpdate(SYNC_HEADING);
                lastHeading = currentHeading;
            }
        }

        void SyncAnimation() {
            if (!hasModel) {
                return;
            }

            auto currentLastAnim = GetLastAniFromHistory(npc);

            if (!currentLastAnim) {
                return;
            }

            if (currentLastAnim != lastAnimation) {
                newAnimIds.push_back(currentLastAnim->aniID);
                lastAnimation = currentLastAnim;
            }

            if (newAnimIds.size() > 0)
            {
                addUpdate(SYNC_ANIMATION);
            }
        }

        void SyncWeaponMode() {
            int currentWeaponMode = npc->GetWeaponMode();
            if (currentWeaponMode != lastWeaponMode) {
                addUpdate(SYNC_WEAPON_MODE);
                lastWeaponMode = currentWeaponMode;
            }
        }

        void SyncAttacks() {
            if (hitsToSync.size() > 0) {
                addUpdate(SYNC_ATTACKS);
            }
        }

        void SyncSpellCasts() {
            if (spellCastsToSync.size() > 0) {
                addUpdate(SYNC_SPELL_CAST);
            }
        }

        void SyncHand() {
            auto leftHand = npc->GetLeftHand();
            auto rightHand = npc->GetRightHand();

            auto leftHandInstanceName = leftHand ? leftHand->GetInstanceName() : "NULL";
            auto rightHandInstanceName = rightHand ? rightHand->GetInstanceName() : "NULL";

            if (!pendingLeftHandInstanceName.Compare(leftHandInstanceName) || !pendingRightHandInstanceName.Compare(rightHandInstanceName)) {
                pendingLeftHandInstanceName = leftHandInstanceName;
                pendingRightHandInstanceName = rightHandInstanceName;
                lastHandChangeTime = CurrentMs;
                return;
            }

            if (CurrentMs < lastHandChangeTime + 250) {
                return;
            }

            if (!lastLeftHandInstanceName.Compare(pendingLeftHandInstanceName) || !lastRightHandInstanceName.Compare(pendingRightHandInstanceName)) {
                addUpdate(SYNC_HAND);
                lastLeftHandInstanceName = pendingLeftHandInstanceName;
                lastRightHandInstanceName = pendingRightHandInstanceName;
            }
        }

        void SyncArmor() {
            auto armor = npc->GetEquippedArmor();
            auto armorName = armor ? armor->GetInstanceName().ToChar() : "NULL";

            if (!lastArmorName.Compare(armorName)) {
                addUpdate(SYNC_ARMOR);
                lastArmorName = armorName;
            }
        }

        void SyncOnDropItem()
        {
            if (itemDropReady)
            {
                addUpdate(SYNC_DROPITEM);
            }
        }

        void SyncOnTakeItem()
        {
            if (pItemTaken)
            {
                addUpdate(SYNC_TAKEITEM);
            }
        }

        void SyncWeapons() {
            if (!npc->IsHuman()) {
                return;
            }

            auto weapon1 = npc->GetEquippedMeleeWeapon();
            auto weapon1Name = weapon1 ? weapon1->GetInstanceName() : "NULL";

            auto weapon2 = npc->GetEquippedRangedWeapon();
            auto weapon2Name = weapon2 ? weapon2->GetInstanceName() : "NULL";

            if (!lastWeapon1Name.Compare(weapon1Name) || !lastWeapon2Name.Compare(weapon2Name)) {
                addUpdate(SYNC_WEAPONS);
                
                lastWeapon1Name = weapon1Name;
                lastWeapon2Name = weapon2Name;
            }

        }

        void SyncBodystate() {
            auto bs = npc->GetBodyState();

            if (bs != lastBodyState)
            {
                addUpdate(SYNC_BODYSTATE);
                lastBodyState = bs;
            }
        }

        void SyncOverlays() {
            if (!npc->CompareOverlaysArray(pArrOverlays))
            {
                addUpdate(SYNC_OVERLAYS);
                pArrOverlays = GetNpcMds(npc);
            }
        }

        void SyncHp() {
            auto currentHp = npc->GetAttribute(NPC_ATR_HITPOINTS);
            auto currentMaxHp = npc->GetAttribute(NPC_ATR_HITPOINTSMAX);

            if (npc && npc->GetObjectName().Compare("BDT_50011_Addon_Raven") && currentHp == 100000) {
                return; // BDT_50011_Addon_Raven souls attack cutscene hack
            }

            if (currentHp != lastSyncHp || currentMaxHp != lastSyncMaxHp) {
                addUpdate(SYNC_HP);

                lastSyncHp = currentHp;
                lastSyncMaxHp = currentMaxHp;
            }
        }

        void SyncTalents() {
            int currentTalents[4];

            currentTalents[0] = npc->GetTalentSkill(oCNpcTalent::NPC_TAL_1H);
            currentTalents[1] = npc->GetTalentSkill(oCNpcTalent::NPC_TAL_2H);
            currentTalents[2] = npc->GetTalentSkill(oCNpcTalent::NPC_TAL_BOW);
            currentTalents[3] = npc->GetTalentSkill(oCNpcTalent::NPC_TAL_CROSSBOW);

            for (int i = 0; i < 4; i++) {
                if (currentTalents[i] != lastTalents[i]) {
                    addUpdate(SYNC_TALENTS);
                    break;
                }
            }

            for (int i = 0; i < 4; i++) {
                lastTalents[i] = currentTalents[i];
            }
        }

        void SyncProtection() {
            int currentProtections[8];
            for (int i = 0; i < 8; i++) {
                currentProtections[i] = npc->GetProtectionByIndex(static_cast<oEIndexDamage>(i));
            }

            for (int i = 0; i < 8; i++) {
                if (currentProtections[i] != lastProtections[i]) {
                    addUpdate(SYNC_PROTECTIONS);
                    break;
                }
            }

            for (int i = 0; i < 8; i++) {
                lastProtections[i] = currentProtections[i];
            }
        }

        void SyncMagicSetup() {
            if (!hasModel || lastSyncHp == 0) {
                return;
            }

            if (npc->GetWeaponMode() != NPC_WEAPON_MAG && !lastSpellInstanceName.Compare("NULL")) {
                addUpdate(SYNC_MAGIC_SETUP);
                lastSpellInstanceName = "NULL";
            }

            if (npc->GetWeaponMode() == NPC_WEAPON_MAG)
            {
                oCMag_Book* book = npc->GetSpellBook();
                if (book)
                {
                    int spellID = book->GetSelectedSpellNr();
                    if (spellID >= 0)
                    {
                        oCItem* item = book->GetSpellItem(spellID);
                        if (item)
                        {
                            auto itemName = item->GetInstanceName();
                            if (!itemName.Compare(lastSpellInstanceName)) {
                                addUpdate(SYNC_MAGIC_SETUP);
                                lastSpellInstanceName = itemName;
                            }
                        }
                    }
                }
            }
        }

        void SyncTime() {
            addUpdate(SYNC_TIME);
        }

        void SyncRevived(zSTRING friendName) {
            revivedFriend = friendName;
            addUpdate(SYNC_REVIVED);
        }

        void addUpdate(int type) {
            bool found = std::find(pendingUpdates.begin(), pendingUpdates.end(), type) != pendingUpdates.end();
            if (!found) {
                pendingUpdates.push_back(type);
            }
        }

        void PackUpdate() {
            for each (auto type in pendingUpdates)
            {
                NetworkPacket packet;
                packet.type = PacketType::PlayerStateUpdate;
                packet.senderId = ServerThread ? std::string(name.ToChar()) : std::string();
                packet.stateUpdate.updateType = static_cast<UpdateType>(type);
                this->AddUpdatePayload(type, packet.stateUpdate);
                ReadyToSendPackets.enqueue(packet);
            }

            pendingUpdates.clear();
        }

        void AddUpdatePayload(int type, PlayerStateUpdatePacket& packet) {
            switch (type)
            {
                case INIT_NPC:
                {
                    packet.initNpc.instanceId = parser->GetIndex(npc->GetInstanceName());
                    packet.initNpc.nickname = MyNickname.ToChar();
                    packet.initNpc.x = lastPosition.n[0];
                    packet.initNpc.y = lastPosition.n[1];
                    packet.initNpc.z = lastPosition.n[2];
                    packet.initNpc.BodyTex = MyBodyTex;
                    packet.initNpc.HeadTex = MyHeadTex;
                    break;
                }
                case SYNC_POS:
                {
                    packet.pos.x = lastPosition.n[0];
                    packet.pos.y = lastPosition.n[1];
                    packet.pos.z = lastPosition.n[2];
                    break;
                }
                case SYNC_HEADING:
                {
                    packet.heading.heading = lastHeading;
                    break;
                }
                case SYNC_ANIMATION:
                {
                    packet.animation.animationId = newAnimIds.back();
                    newAnimIds.pop_back();
                    break;
                }
                case SYNC_WEAPON_MODE:
                {
                    packet.weaponMode.weaponMode = lastWeaponMode;
                    break;
                }
                case SYNC_MAGIC_SETUP:
                {
                    packet.magicSetup.spellInstanceName = lastSpellInstanceName.ToChar();
                    break;
                }
                case SYNC_SPELL_CAST:
                {
                    auto casts = nlohmann::json::array();
                    for each (auto sc in spellCastsToSync) {
                        SpellCastInfo cast;
                        cast.target = sc.targetNpcUniqueName.ToChar();
                        cast.spellInstanceId = sc.spellInstanceId;
                        cast.spellLevel = sc.spellLevel;
                        cast.spellCharge = sc.spellCharge;
                        packet.spellCasts.casts.push_back(cast);
                    }

                    spellCastsToSync.clear();
                    break;
                }
                case SYNC_ARMOR:
                {
                    packet.armor.armor = lastArmorName.ToChar();
                    break;
                }
                case SYNC_WEAPONS:
                {
                    packet.weapons.weapon1 = lastWeapon1Name.ToChar();
                    packet.weapons.weapon2 = lastWeapon2Name.ToChar();
                    break;
                }
                case SYNC_HP:
                {
                    packet.hp.hp = lastSyncHp;
                    packet.hp.hpMax = lastSyncMaxHp;
                    break;
                }
                case SYNC_BODYSTATE:
                {
                    packet.bodyState.bodyState = lastBodyState;
                    break;
                }
                case SYNC_OVERLAYS:
                {
                    for (int i = 0; i < pArrOverlays.GetNumInList(); i++)
                    {
                        packet.overlays.overlayIds.push_back(pArrOverlays.GetSafe(i));
                    }
    
                    break;
                }
                case SYNC_PROTECTIONS:
                {
                    for (int i = 0; i < 8; i++) {
                        packet.protections.protections[i] = lastProtections[i];
                    }
                    break;
                }
                case SYNC_TALENTS:
                {
                    for (int i = 0; i < 4; i++) {
                        packet.talents.talents[i] = lastTalents[i];
                    }
                    break;
                }
                case SYNC_HAND:
                {
                    packet.hand.leftItem = lastLeftHandInstanceName.ToChar();
                    packet.hand.rightItem = lastRightHandInstanceName.ToChar();
                    break;
                }
                case SYNC_TIME:
                {
                    int _a, h, m;
                    ogame->GetTime(_a, h, m);

                    packet.time.hour = h;
                    packet.time.minute = m;
                    break;
                }
                case SYNC_REVIVED:
                {
                    packet.revived.name = revivedFriend.ToChar();
                    revivedFriend = "";
                    break;
                }
                case SYNC_ATTACKS:
                {
                    for each (auto at in hitsToSync) {
                        AttackInfo attack;
                        attack.target = at.npcUniqueName.ToChar();
                        attack.damage = at.damage;
                        attack.isUnconscious = at.isUnconscious;
                        attack.isDead = at.isDead;
                        attack.isFinish = at.isFinish;
                        attack.damageMode = at.damageMode;
                        packet.attacks.attacks.push_back(attack);
                    }

                    hitsToSync.clear();
                    break;
                }
                case SYNC_DROPITEM:
                {
                    if (pItemDropped && itemDropReady)
                    {
                        packet.dropItem.itemDropped = pItemDropped->GetInstanceName();
                        packet.dropItem.count = pItemDropped->amount;
                        packet.dropItem.flags = pItemDropped->flags;
                        packet.dropItem.itemUniqueName = pItemDropped->GetObjectName();

                        itemDropReady = false;
                    }
                    break;
                }
                case SYNC_TAKEITEM:
                {
                    if (pItemTaken)
                    {
                        packet.takeItem.itemDropped = pItemTaken->GetInstanceName();
                        packet.takeItem.count = pItemTaken->amount;
                        packet.takeItem.flags = pItemTaken->flags;
                        packet.takeItem.uniqueName = pItemTaken->GetObjectName();
                        packet.takeItem.x = pItemTakenPos.n[0];
                        packet.takeItem.y = pItemTakenPos.n[1];
                        packet.takeItem.z = pItemTakenPos.n[2];

                        pItemTaken->RemoveVobFromWorld();
                        pItemTaken = NULL;
                    }
                    break;
                }
                case DESTROY_NPC:
                {
                    break;
                }
            } 
        }
    };
}