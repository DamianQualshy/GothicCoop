namespace GOTHIC_ENGINE {
    int LastHpBeforeDamage = -1;
    oCNpc::oSDamageDescriptor* LastIgnoredDamDesc = NULL;

    void __fastcall oCNpc_OnDamage_Hit(oCNpc*, void*, oCNpc::oSDamageDescriptor&);
#if ENGINE >= Engine_G2
    CInvoke<void(__thiscall*)(oCNpc*, oCNpc::oSDamageDescriptor&)> Ivk_oCNpc_OnDamage_Hit(0x00666610, &oCNpc_OnDamage_Hit);
#else
    CInvoke<void(__thiscall*)(oCNpc*, oCNpc::oSDamageDescriptor&)> Ivk_oCNpc_OnDamage_Hit(0x00731410, &oCNpc_OnDamage_Hit);
#endif
    void __fastcall oCNpc_OnDamage_Hit(oCNpc* _this, void* vtable, oCNpc::oSDamageDescriptor& damdesc) {
        if (damdesc.pNpcAttacker == player && IsCoopPlayer(_this->GetObjectName())) {
            return;
        }

        if (IsCoopPaused) {
            Ivk_oCNpc_OnDamage_Hit(_this, damdesc);
            return;
        }

#if ENGINE == Engine_G1
        if (ServerThread && damdesc.pNpcAttacker && (damdesc.pNpcAttacker == player || PlayerNpcs.count(damdesc.pNpcAttacker))) {
            if (_this->GetAttitude(damdesc.pNpcAttacker) == NPC_ATT_HOSTILE) {
                auto randValue = GetRandVal(0, 100);
                if (randValue > 33) {
                    _this->SetEnemy(damdesc.pNpcAttacker);
                }
            }
        }
#endif
        // Blocking animations on clients for NPC is not preventing attacking sometimes (so do not call unless attack from the coop engine)
        if (ClientThread && 
            damdesc.pNpcAttacker != player && 
            (_this == player || IsCoopPlayer(_this->GetObjectName())) && 
            damdesc.fDamageTotal != COOP_MAGIC_NUMBER &&
            damdesc.enuModeDamage != oETypeDamage::oEDamageType_Fall) {
            return;
        }

        if (ClientThread && damdesc.pNpcAttacker != player) {
            Ivk_oCNpc_OnDamage_Hit(_this, damdesc);
            return;
        }

        if (ServerThread && PlayerNpcs.count(damdesc.pNpcAttacker)) {
            Ivk_oCNpc_OnDamage_Hit(_this, damdesc);
            return;
        }

        if (damdesc.pNpcAttacker == player && damdesc.fDamageTotal == COOP_MAGIC_NUMBER) {
            LastIgnoredDamDesc = &damdesc;
            Ivk_oCNpc_OnDamage_Hit(_this, damdesc);
            return;
        }

        float HpMultipler = 1.0f;
        if (damdesc.pNpcAttacker == player) {
            HpMultipler = PlayersDamageMultipler / 100.0f;
        }
        else if (_this == player) {
            HpMultipler = NpcsDamageMultipler / 100.0f;
        }

        int HpBeforeOnDamage = _this->GetAttribute(NPC_ATR_HITPOINTS);
        int MaxHpBeforeOnDamage = _this->GetAttribute(NPC_ATR_HITPOINTSMAX);
        LastHpBeforeDamage = HpBeforeOnDamage;

        if (HpMultipler == 1.0f) {
            Ivk_oCNpc_OnDamage_Hit(_this, damdesc);
            return;
        }

        _this->SetAttribute(NPC_ATR_HITPOINTS, 100000);
        _this->SetAttribute(NPC_ATR_HITPOINTSMAX, 100000);

        Ivk_oCNpc_OnDamage_Hit(_this, damdesc);

        int RealDamage = (100000 - _this->GetAttribute(NPC_ATR_HITPOINTS));
        int HpDamage = static_cast<int>(RealDamage * HpMultipler);

        _this->SetAttribute(NPC_ATR_HITPOINTS, HpBeforeOnDamage - HpDamage);
        _this->SetAttribute(NPC_ATR_HITPOINTSMAX, MaxHpBeforeOnDamage);
    }

    void __fastcall oCNpc_OnDamage_Sound(oCNpc*, void*, oCNpc::oSDamageDescriptor&);
#if ENGINE >= Engine_G2
    CInvoke<void(__thiscall*)(oCNpc*, oCNpc::oSDamageDescriptor&)> Ivk_oCNpc_OnDamage_Sound(0x0067A8A0, &oCNpc_OnDamage_Sound);
#else
    CInvoke<void(__thiscall*)(oCNpc*, oCNpc::oSDamageDescriptor&)> Ivk_oCNpc_OnDamage_Sound(0x00746660, &oCNpc_OnDamage_Sound);
#endif
    void __fastcall oCNpc_OnDamage_Sound(oCNpc* _this, void* vtable, oCNpc::oSDamageDescriptor& damdesc) {
        if (damdesc.pNpcAttacker == player && IsCoopPlayer(_this->GetObjectName())) {
            return;
        }

        if (LastIgnoredDamDesc == &damdesc) {
            Ivk_oCNpc_OnDamage_Sound(_this, damdesc);
            LastIgnoredDamDesc = NULL;
            return;
        }

        if (IsCoopPaused) {
            Ivk_oCNpc_OnDamage_Sound(_this, damdesc);
            return;
        }

        bool IsFinishUnconsciousHitWithOnlySoundFunctionCall = !damdesc.pNpcAttacker && damdesc.bIsDead;
        if (IsFinishUnconsciousHitWithOnlySoundFunctionCall) {
            Ivk_oCNpc_OnDamage_Sound(_this, damdesc);
            return;
        }

        if (ClientThread && damdesc.pNpcAttacker != player) {
            Ivk_oCNpc_OnDamage_Sound(_this, damdesc);
            return;
        }

        if (ServerThread && PlayerNpcs.count(damdesc.pNpcAttacker)) {
            Ivk_oCNpc_OnDamage_Sound(_this, damdesc);
            return;
        }

        if (damdesc.pNpcAttacker == player && damdesc.fDamageTotal == COOP_MAGIC_NUMBER) {
            Ivk_oCNpc_OnDamage_Sound(_this, damdesc);
            return;
        }

        PlayerHit hit;
        hit.damage = static_cast<float>(LastHpBeforeDamage - _this->GetAttribute(NPC_ATR_HITPOINTS));
        hit.attacker = damdesc.pNpcAttacker;
        hit.npc = _this;
        hit.isDead = _this->IsDead();
        hit.isUnconscious = _this->IsUnconscious();
        hit.damageMode = damdesc.enuModeDamage;
        ReadyToSyncDamages.enqueue(hit);

        Ivk_oCNpc_OnDamage_Sound(_this, damdesc);
    }

    void __fastcall oCNpc_EV_AttackFinish(oCNpc*, void*, oCMsgAttack*);
#if ENGINE >= Engine_G2
    CInvoke<void(__thiscall*)(oCNpc*, oCMsgAttack*)> Ivk_oCNpc_EV_AttackFinish(0x00751AF0, &oCNpc_EV_AttackFinish);
#else
    CInvoke<void(__thiscall*)(oCNpc*, oCMsgAttack*)> Ivk_oCNpc_EV_AttackFinish(0x006AC180, &oCNpc_EV_AttackFinish);
#endif
    void __fastcall oCNpc_EV_AttackFinish(oCNpc* _this, void* vtable, oCMsgAttack* attack) {
        if (!ClientThread || _this != player) {
            Ivk_oCNpc_EV_AttackFinish(_this, attack);
            return;
        }

        auto model = _this->GetModel();
        auto hitAni = model->GetAniFromAniID(attack->hitAni);

        if (model && hitAni && model->IsAniActive(hitAni)) {
            float progress = model->GetProgressPercent(attack->hitAni);
            if (progress >= 0.5 && attack->target) {
                oCNpc* enemy = zDYNAMIC_CAST<oCNpc>(attack->target);
                if (enemy && enemy->IsUnconscious())
                {
                    PlayerHit hit;
                    hit.damage = 1;
                    hit.attacker = player;
                    hit.npc = enemy;
                    hit.isDead = true;
                    hit.isUnconscious = 0;
                    hit.damageMode = oETypeDamage::oEDamageType_Edge;
                    ReadyToSyncDamages.enqueue(hit);
                }
            }
        }

        Ivk_oCNpc_EV_AttackFinish(_this, attack);
    }

    int __fastcall oCAIHuman_StandActions(oCAIHuman*, void*);
#if ENGINE >= Engine_G2
    CInvoke<int(__thiscall*)(oCAIHuman*)> Ivk_oCAIHuman_StandActions(0x00698EA0, &oCAIHuman_StandActions);
#else
    CInvoke<int(__thiscall*)(oCAIHuman*)> Ivk_oCAIHuman_StandActions(0x00612840, &oCAIHuman_StandActions);
#endif
    int __fastcall oCAIHuman_StandActions(oCAIHuman* _this, void* vtable) {
        if (ServerThread) {
            return Ivk_oCAIHuman_StandActions(_this);
        }

        if (_this->npc != player) {
            return Ivk_oCAIHuman_StandActions(_this);
        }

        auto focusedNpc = player->GetFocusNpc();
        if (!focusedNpc) {
            return Ivk_oCAIHuman_StandActions(_this);
        }

        if (IsCoopPlayer(focusedNpc->GetObjectName())) {
            return 0;
        }

        if (focusedNpc->IsDead() || focusedNpc->IsUnconscious() || focusedNpc->GetWeaponMode() != NPC_WEAPON_NONE) {
            return Ivk_oCAIHuman_StandActions(_this);
        }

        auto activeAnims = GetCurrentAni(focusedNpc);
        for (auto ani : activeAnims) {
            focusedNpc->GetModel()->StopAnimation(ani->aniName);
        }

        focusedNpc->GetEM()->KillMessages();
        focusedNpc->ClearEM();

        return Ivk_oCAIHuman_StandActions(_this);
    }

    void __fastcall oCMag_Book_Spell_Cast(oCMag_Book*, void*);
#if ENGINE >= Engine_G2
    CInvoke<void(__thiscall*)(oCMag_Book*)> Ivk_oCMag_Book_Spell_Cast(0x004767A0, &oCMag_Book_Spell_Cast);
#else
    CInvoke<void(__thiscall*)(oCMag_Book*)> Ivk_oCMag_Book_Spell_Cast(0x0046FC00, &oCMag_Book_Spell_Cast);
#endif
    void __fastcall oCMag_Book_Spell_Cast(oCMag_Book* _this, void* vtable) {
        auto castingNpc = (oCNpc*)_this->owner;

        if (IsCoopPaused) {
            Ivk_oCMag_Book_Spell_Cast(_this);
            return;
        }

        if (!castingNpc) {
            Ivk_oCMag_Book_Spell_Cast(_this);
            return;
        }

        oCMag_Book* book = castingNpc->GetSpellBook();
        if (!book) {
            Ivk_oCMag_Book_Spell_Cast(_this);
            return;
        }

        if (IsCoopPlayer(castingNpc->GetObjectName())) {
            int spellID = book->GetSelectedSpellNr();
            if (spellID >= 0)
            {
                oCItem* item = book->GetSpellItem(spellID);
                if (item)
                {
                    auto itemName = item->GetInstanceName();
                    int insIndex = parser->GetIndex(itemName);
                    if (insIndex > 0) {
                        auto spellItem = CreateCoopItem(insIndex);
                        if (spellItem) {
                            castingNpc->DoPutInInventory(spellItem);
                        }
                    }
                }
            }
        }

        Ivk_oCMag_Book_Spell_Cast(_this);

        SpellCast sc;
        sc.npc = (oCNpc*)_this->owner;
        sc.targetNpc = sc.npc->GetFocusNpc();
        ReadyToSyncSpellCasts.enqueue(sc);
    }

    void __fastcall oCNpc_OpenDeadNpc(oCNpc*, void*);
#if ENGINE >= Engine_G2
    CInvoke<void(__thiscall*)(oCNpc*)> Ivk_oCNpc_OpenDeadNpc(0x00762970, &oCNpc_OpenDeadNpc);
#else
    CInvoke<void(__thiscall*)(oCNpc*)> Ivk_oCNpc_OpenDeadNpc(0x006BB890, &oCNpc_OpenDeadNpc);
#endif
    void __fastcall oCNpc_OpenDeadNpc(oCNpc* _this, void* vtable) {
        auto focusNpc = _this->GetFocusNpc();

        if (focusNpc && IsCoopPlayer(focusNpc->GetObjectName())) {
            return;
        }

        Ivk_oCNpc_OpenDeadNpc(_this);
    }

    int __fastcall oCNpc_CanUse(oCNpc*, void*, oCItem*);
#if ENGINE >= Engine_G2
    CInvoke<int(__thiscall*)(oCNpc*, oCItem*)> Ivk_oCNpc_CanUse(0x007319B0, &oCNpc_CanUse);
#else
    CInvoke<int(__thiscall*)(oCNpc*, oCItem*)> Ivk_oCNpc_CanUse(0x0068EF00, &oCNpc_CanUse);
#endif
    int __fastcall oCNpc_CanUse(oCNpc* _this, void* vtable, oCItem* n) {
        if (IsCoopPlayer(_this->GetObjectName())) {
            return true;
        }

        return Ivk_oCNpc_CanUse(_this, n);
    }

    void __fastcall oCWorld_RemoveVob(oCWorld*, void*, zCVob*);
#if ENGINE >= Engine_G2
    CInvoke<void(__thiscall*)(oCWorld*, zCVob*)> Ivk_oCWorld_RemoveVob(0x007800C0, &oCWorld_RemoveVob);
#else
    CInvoke<void(__thiscall*)(oCWorld*, zCVob*)> Ivk_oCWorld_RemoveVob(0x006D6EF0, &oCWorld_RemoveVob);
#endif
    void __fastcall oCWorld_RemoveVob(oCWorld* _this, void* vtable, zCVob* vob) {
        if (IsSavingGame) {
            return Ivk_oCWorld_RemoveVob(_this, vob);
        }

        if (vob->GetCharacterClass() == 2) {
            auto npc = (oCNpc*)vob;
            if (npc && !IsCoopPlayer(npc->GetObjectName())) {
                if (NpcToUniqueNameList.count(npc) > 0) {
                    auto uniqueName = NpcToUniqueNameList[npc];
                    if (BroadcastNpcs.count(uniqueName) > 0) {
                        auto player = BroadcastNpcs[uniqueName];
                        if (player) {
                            player->destroyed = true;
                            BroadcastNpcs.erase(BroadcastNpcs.find(uniqueName));
                        }
                    }
                    auto syncIt = SyncNpcs.find(uniqueName);
                    if (syncIt != SyncNpcs.end()) {
                        auto remoteNpc = syncIt->second;
                        if (remoteNpc) {
                            remoteNpc->destroyed = true;
                            delete remoteNpc;
                        }
                        SyncNpcs.erase(syncIt);
                    }

                    UniqueNameToNpcList.erase(uniqueName);
                    NpcToUniqueNameList.erase(npc);
                }
            }
        }

        return Ivk_oCWorld_RemoveVob(_this, vob);
    }

    void __fastcall zCModel_StartAni(zCModel*, void*, zCModelAni*, int);
#if ENGINE >= Engine_G2
    CInvoke<void(__thiscall*)(zCModel*, zCModelAni*, int)> Ivk_zCModel_StartAni(0x0057B0C0, &zCModel_StartAni);
#else
    CInvoke<void(__thiscall*)(zCModel*, zCModelAni*, int)> Ivk_zCModel_StartAni(0x005612F0, &zCModel_StartAni);
#endif
    void __fastcall zCModel_StartAni(zCModel* _this, void* vtable, zCModelAni* a, int b) {
        if (IsLoadingLevel) {
            Ivk_zCModel_StartAni(_this, a, b);
            return;
        }

        if (zinput->KeyPressed(KEY_F2) && !WorldEditMode) {
            Ivk_zCModel_StartAni(_this, a, b);
            return;
        }

        if (IsCoopPaused) {
            Ivk_zCModel_StartAni(_this, a, b);
            return;
        }

        if (ClientThread && b != COOP_MAGIC_NUMBER && _this->homeVob && IgnoredSyncNpc(_this->homeVob)) {
            Ivk_zCModel_StartAni(_this, a, 0);
            return;
        }

        if (ClientThread && b == COOP_MAGIC_NUMBER && _this->homeVob && IgnoredSyncNpc(_this->homeVob)) {
            return;
        }

        if (!ClientThread || _this->homeVob == player) {
            Ivk_zCModel_StartAni(_this, a, b);
            return;
        }

        if (ClientThread && b == COOP_MAGIC_NUMBER) {
            Ivk_zCModel_StartAni(_this, a, 0);
            return;
        }

        if (!WorldEditMode && player && player->GetFocusNpc() == _this->homeVob && a && a->aniName &&
           (a->aniName == "S_RUN" || a->aniName == "S_WALK" || a->aniName == "T_LOOK" || a->aniName == "T_WALKTURNR" || a->aniName == "T_WALKTURNL")) 
        {
            Ivk_zCModel_StartAni(_this, a, b);
            return;
        }

        if (ClientThread && _this->homeVob && _this->homeVob->GetCharacterClass() != 2) {
            Ivk_zCModel_StartAni(_this, a, b);
            return;
        }

        if (ClientThread && IsPlayerTalkingWithNpc(_this->homeVob)) {
            Ivk_zCModel_StartAni(_this, a, b);
            return;
        }

        auto npc = (oCNpc*)_this->homeVob;
        if (ClientThread && npc && npc->IsDead()) {
            Ivk_zCModel_StartAni(_this, a, b);
            return;
        }

        // TMP: Use the logic to start/stop anim only for NB, it works by just not calling the function in g1/g2
        if (FriendInstanceId.Compare("pc_heromul") && a && a->aniName && (a->aniName == "S_RUN" || a->aniName == "S_FISTRUN")) {
            auto activeBefore = _this->IsAnimationActive(a->aniName);
            Ivk_zCModel_StartAni(_this, a, b);

            if (activeBefore == 0) {
                _this->StopAnimation(a->aniName);
            }
        }
    }

    // Shared crash reporting helper is used here to avoid duplicating SaveErrorDetails logic.
    static void __cdecl zCExceptionHandlerUnhandledExceptionFilter(struct _EXCEPTION_POINTERS*);
#if ENGINE >= Engine_G2
    CInvoke<void(*)(struct _EXCEPTION_POINTERS*)> Ivk_zCExceptionHandlerUnhandledExceptionFilter(0x004C88C0, &zCExceptionHandlerUnhandledExceptionFilter);
#else
    CInvoke<void(*)(struct _EXCEPTION_POINTERS*)> Ivk_zCExceptionHandlerUnhandledExceptionFilter(0x004BF560, &zCExceptionHandlerUnhandledExceptionFilter);
#endif
    void zCExceptionHandlerUnhandledExceptionFilter(struct _EXCEPTION_POINTERS* pointers) {
        TrackLastExecutedFunctions = false;

        CrashReportData report = BuildCrashReport(false);
        std::vector<std::string> last10CoreMethodCalls(report.lastCoreMethodCalls.end() - 10, report.lastCoreMethodCalls.end());
        std::string errorMessage = string::Combine("[GothicCoop] Error (v. %i):\n", COOP_VERSION).ToChar();
        std::string errorLog = "";

        if (PluginState.compare("") != 0) {
            errorMessage += "State:\n";
            errorMessage += PluginState;
            errorMessage += "\n";
        }

        errorMessage += "Packages:\n";
        for (auto data : lastProcessedPackages) {
            errorMessage += data;
            errorMessage += "\n";
        }

        errorMessage += "Calls:\n";
        for (const auto& piece : report.lastMethodCalls) {
            errorMessage += piece;
            errorMessage += "\n";
        }

        errorMessage += "Core calls:\n";
        for (const auto& piece : last10CoreMethodCalls) {
            errorMessage += piece;
            errorMessage += "\n";
        }

        Message::Error(errorMessage.c_str());
        Ivk_zCExceptionHandlerUnhandledExceptionFilter(pointers);
    }

    int __fastcall oCNpc_DoTakeVob(oCNpc*, void*, zCVob*);
#if ENGINE >= Engine_G2
    CInvoke<int(__thiscall*)(oCNpc*, zCVob*)> Ivk_oCNpc_DoTakeVob(0x007449C0, &oCNpc_DoTakeVob);
#else
    CInvoke<int(__thiscall*)(oCNpc*, zCVob*)> Ivk_oCNpc_DoTakeVob(0x006A0D10, &oCNpc_DoTakeVob);
#endif
    int __fastcall oCNpc_DoTakeVob(oCNpc* _this, void* vtable, zCVob* vob) {
        if (Myself && _this && _this->IsAPlayer() && vob)
        {
            if (oCItem* item = zDYNAMIC_CAST<oCItem>(vob))
            {
                if (item->GetObjectName().HasWord("RX_DROPPED_ITEM_"))
                {
                    Myself->pItemTaken = zfactory->CreateItem(item->GetInstance());
                    Myself->pItemTaken->flags = item->flags;
                    Myself->pItemTaken->amount = item->amount;
                    Myself->pItemTaken->SetObjectName(item->GetObjectName());
                    Myself->pItemTakenPos = item->GetPositionWorld();
                    Myself->SyncOnTakeItem();
                }
            }
        }

        return Ivk_oCNpc_DoTakeVob(_this, vob);
    }

    int __fastcall oCNpc_DoDropVob(oCNpc*, void*, zCVob*);
#if ENGINE >= Engine_G2
    CInvoke<int(__thiscall*)(oCNpc*, zCVob*)> Ivk_oCNpc_DoDropVob(0x00744DD0, &oCNpc_DoDropVob);
#else
    CInvoke<int(__thiscall*)(oCNpc*, zCVob*)> Ivk_oCNpc_DoDropVob(0x006A10F0, &oCNpc_DoDropVob);
#endif
    int __fastcall oCNpc_DoDropVob(oCNpc* _this, void* vtable, zCVob* vob) {
        if (Myself && _this && _this->IsAPlayer() && vob && (!_this->IsDead() && !_this->IsUnconscious()))
        {
            if (auto pItem = vob->CastTo<oCItem>())
            {
                int randVal = GetRandVal(0, 2000000000);
                pItem->SetObjectName("RX_DROPPED_ITEM_" + Z randVal);
                Myself->pItemDropped = pItem;
                Myself->itemDropReady = true;
                Myself->SyncOnDropItem();

                return Ivk_oCNpc_DoDropVob(_this, vob);
            }
        }

        auto dropResult = Ivk_oCNpc_DoDropVob(_this, vob);

        if (_this && vob && IsCoopPlayer(_this->GetObjectName())) {
            if (auto pItem = vob->CastTo<oCItem>())
            {
                if (!pItem->GetObjectName().StartWith("RX_DROPPED_ITEM_")) {
                    pItem->RemoveVobFromWorld();
                }
            }
        }

        return dropResult;
    }
}