int DANGERGetCurrentAni(GOTHIC_ENGINE::zCModel* model, int index) {
    __try
    {
        return model->aniChannels[index]->protoAni->aniID;
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return -1;
    }
}

namespace GOTHIC_ENGINE {
    static const char BlockedAnimationsForAll[8][30] =
    {
        {"T_STUMBLE"},
        {"T_STUMBLEB"},
        {"T_GOTHIT"},
        {"C_LOOK_1"},
        {"R_ROAM1"},
        {"R_ROAM2"},
        {"R_ROAM3"},
        {"C_BOW_1"},
    };

    static const char AnimationTurning[44][30] =
    {
        {"T_RUNTURNL"},
        {"T_RUNTURNR"},

        {"T_SWIMTURNL"},
        {"T_SWIMTURNR"},

        {"T_WALKTURNL"},
        {"T_WALKTURNR"},

        {"T_SNEAKTURNL"},
        {"T_SNEAKTURNR"},

        {"T_FISTRUNTURNL"},
        {"T_FISTRUNTURNR"},
        {"T_FISTWALKTURNL"},
        {"T_FISTWALKTURNR"},
        {"T_FISTSNEAKTURNL"},
        {"T_FISTSNEAKTURNR"},

        {"T_1HRUNTURNL"},
        {"T_1HRUNTURNR"},
        {"T_1HWALKTURNL"},
        {"T_1HWALKTURNR"},
        {"T_1HSNEAKTURNL"},
        {"T_1HSNEAKTURNR"},

        {"T_2HRUNTURNL"},
        {"T_2HRUNTURNR"},
        {"T_2HWALKTURNL"},
        {"T_2HWALKTURNR"},
        {"T_2HSNEAKTURNL"},
        {"T_2HSNEAKTURNR"},

        {"T_CBOWRUNTURNL"},
        {"T_CBOWRUNTURNR"},
        {"T_CBOWWALKTURNL"},
        {"T_CBOWWALKTURNR"},
        {"T_CBOWSNEAKTURNL"},
        {"T_CBOWSNEAKTURNR"},

        {"T_BOWRUNTURNL"},
        {"T_BOWRUNTURNR"},
        {"T_BOWWALKTURNL"},
        {"T_BOWWALKTURNR"},
        {"T_BOWSNEAKTURNL"},
        {"T_BOWSNEAKTURNR"},

        {"T_MAGRUNTURNL"},
        {"T_MAGRUNTURNR"},
        {"T_MAGWALKTURNL"},
        {"T_MAGWALKTURNR"},
        {"T_MAGSNEAKTURNL"},
        {"T_MAGSNEAKTURNR"}
    };

    bool IsAniBlockedForAll(zCModelAni* modelAni)
    {
        if (modelAni)
        {
            auto aniName = modelAni->GetAniName();
            for (unsigned int i = 0; i < 8; i++)
            {
                if (aniName && strcmp(aniName.ToChar(), BlockedAnimationsForAll[i]) == 0)
                    return true;
            }
        }
        return false;
    };

    bool IsAnimationTurning(zCModelAni* modelAni)
    {
        auto aniName = modelAni->GetAniName();
        for (unsigned int i = 0; i < 44; i++)
        {
            if (aniName && strcmp(aniName.ToChar(), AnimationTurning[i]) == 0)
                return true;
        }
        return false;
    };

    zCModelAni* GetLastAniFromHistory(oCNpc* npc)
    {
        zCModel* model = npc->GetModel();
        if (!model) {
            return NULL;
        }

        if (!model->aniHistoryList) {
            return NULL;
        }

        for (int i = 1; i <= MAX_ANIHISTORY; i++) {
            auto lastAni = model->aniHistoryList[MAX_ANIHISTORY - i];
            if (lastAni && !IsAniBlockedForAll(lastAni) && !IsAnimationTurning(lastAni)) {
                return lastAni;
            }
        }

        return NULL;
    }

    std::vector<zCModelAni*> GetCurrentAni(oCNpc* npc)
    {
        zCModel* model = npc->GetModel();
        if (!model) {
            return std::vector<zCModelAni*>();
        }

        int activeAnimCount = model->numActiveAnis;
        std::vector<int> aniIdList;

        for (int index = 0; index < activeAnimCount; index++) {
            int aniId = DANGERGetCurrentAni(model, index);
            aniIdList.push_back(aniId);
        }

        std::vector<zCModelAni*> allowedAniIdList;
        for each (auto activeAniId in aniIdList)
        {
            zCModelAni* ani = model->GetAniFromAniID(activeAniId);
            if (ani && !IsAniBlockedForAll(ani) && !IsAnimationTurning(ani)) {
                allowedAniIdList.push_back(ani);
            }
        }

        return allowedAniIdList;
    }
}