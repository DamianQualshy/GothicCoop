// Supported with union (c) 2020 Union team
// Union SOURCE file

namespace GOTHIC_ENGINE {
	

    // interpolation for translation
    zVEC3 Lerp(zVEC3 a, zVEC3 b, float t)
    {
        return a + (b - a) * t;
    }

    // min max limit for a float variable
    void zClamp(float& value, float bl, float bh) {
        if (value < bl)
            value = bl;
        else if (value > bh)
            value = bh;
    }

	Common::Map<zSTRING, int> MdsHumansMap;

	void CheckMdsMap() {
		static int check = 0;
		if (!check) {
			check = 1;
			MdsHumansMap.Insert("HUMANS.MDS", 0);
			MdsHumansMap.Insert("HUMANS_RELAXED.MDS", 1);
			MdsHumansMap.Insert("HUMANS_1HST2.MDS", 2);
			MdsHumansMap.Insert("HUMANS_2HST2.MDS", 3);
			MdsHumansMap.Insert("HUMANS_BOWT2.MDS", 4);
			MdsHumansMap.Insert("HUMANS_CBOWT2.MDS", 5);
			MdsHumansMap.Insert("HUMANS_MILITIA.MDS", 6);
			MdsHumansMap.Insert("HUMANS_1HST3.MDS", 7);
			MdsHumansMap.Insert("HUMANS_2HST3.MDS", 8);
			MdsHumansMap.Insert("HUMANS_ARROGANCE.MDS", 9);
			MdsHumansMap.Insert("HUMANS_1HST1.MDS", 10);
			MdsHumansMap.Insert("HUMANS_2HST1.MDS", 11);
			MdsHumansMap.Insert("HUMANS_BOWT1.MDS", 12);
			MdsHumansMap.Insert("HUMANS_CBOWT1.MDS", 13);
			MdsHumansMap.Insert("HUMANS_SPST2.MDS", 14);
			MdsHumansMap.Insert("HUMANS_ACROBATIC.MDS", 15);
			MdsHumansMap.Insert("HUMANS_MAGESPRINT.MDS", 16);
			MdsHumansMap.Insert("HUMANS_TIRED.MDS", 17);
			MdsHumansMap.Insert("HUMANS_MAGE.MDS", 18);
			MdsHumansMap.Insert("HUMANS_SKELETON.MDS", 19);
			MdsHumansMap.Insert("HUMANS_SKELETON_FLY.MDS", 20);
			MdsHumansMap.Insert("HUMANS_BABE.MDS", 21);
			MdsHumansMap.Insert("SHIELD.MDS", 22);
			MdsHumansMap.Insert("HUMANS_PIRATE.MDS", 23);
			MdsHumansMap.Insert("HUMANS_ARR.MDS", 24);
			MdsHumansMap.Insert("SHIELD_ST1.MDS", 25);
			MdsHumansMap.Insert("HUMANS_SPST1.MDS", 26);
			MdsHumansMap.Insert("HUMANS_TRD.MDS", 27);
			MdsHumansMap.Insert("HUMANS_SIT_EAT.MDS", 28);
			MdsHumansMap.Insert("HUMANS_SIT_DRINK.MDS", 29);
			MdsHumansMap.Insert("SHIELD_ST2.MDS", 30);
			MdsHumansMap.Insert("HUMANS_REL.MDS", 31);
			MdsHumansMap.Insert("HUMANS_AXEST2.MDS", 32);
			MdsHumansMap.Insert("HUMANS_NEWTORCH.MDS", 33);

		}
	}

	int GetMdsIndex(zSTRING mds) {
		CheckMdsMap();
		mds.Upper();

		auto arr = MdsHumansMap.GetArray();
		const int count = static_cast<int>(arr.GetNum());
		for (int i = 0; i < count; i++) {
			if (auto pair = arr.GetSafe(i)) {
				if (mds == pair->GetKey()) {
					return pair->GetValue();
				}
			}
		}


		//cmd << "GetMdsIndex invalid " << mds << endl;

		return -1;
	}

	zSTRING GetMdsByIndex(int index) {
		CheckMdsMap();
		auto arr = MdsHumansMap.GetArray();
		const int count = static_cast<int>(arr.GetNum());
		for (int i = 0; i < count; i++) {
			if (auto pair = arr.GetSafe(i)) {
				if (index == pair->GetValue()) {
					return pair->GetKey();
				}
			}
		}

		return "";
	}

	zCArray<int> GetNpcMds(oCNpc* npc) {
		zCArray<int> arr;
		if (npc && npc->GetModel()) {
			auto proto = npc->GetModel()->modelProtoList;
			for (int i = 0; i < proto.GetNumInList(); i++) {
				if (auto protoInst = proto.GetSafe(i)) {
					arr.Insert(GetMdsIndex(protoInst->modelProtoFileName));
				}
			}
		}
		return arr;
	}

	void oCNpc::ApplyOverlaysNpc(oCNpc* npc) {
		if (!npc) return;

		zCArray<int> npcMdsList = GetNpcMds(npc);
		ApplyOverlaysArray(npcMdsList);
	}

	void oCNpc::ApplyOverlaysArray(zCArray<int> npcMdsArray) {
		zCArray<zCModelPrototype*> protoList = GetModel()->modelProtoList;
		zCArray<zSTRING> mdsList;
		for (int i = 0; i < protoList.GetNumInList(); i++) {
			if (zCModelPrototype* proto = protoList.GetSafe(i)) {
				if (proto->modelProtoFileName != "HUMANS.MDS") {
					mdsList.Insert(proto->modelProtoFileName);
				}
			}
		}

		for (int i = 0; i < mdsList.GetNumInList(); i++) {
			zSTRING mds = mdsList.GetSafe(i);
			this->RemoveOverlayMds(mds);
		}

		for (int i = 0; i < npcMdsArray.GetNumInList(); i++) {
			int index = npcMdsArray.GetSafe(i);
			zSTRING mds = GetMdsByIndex(index);

			if (!mds.IsEmpty() && mds != "HUMANS.MDS") {
				this->ApplyOverlayMds(mds);
			}
		}
	}

	int oCNpc::CompareOverlaysMds(oCNpc* npc) {
		if (!npc) return 0;
		zCArray<int> npcMdsList = GetNpcMds(npc);
		zCArray<int> npcMdsListMy = GetNpcMds(this);
		if (npcMdsListMy.GetNumInList() != npcMdsList.GetNumInList()) {
			return 0;
		}
		else {
			for (int i = 0; i < npcMdsListMy.GetNumInList(); i++) {
				int mdsMy = npcMdsListMy.GetSafe(i);
				if (!npcMdsList.IsInList(mdsMy)) {
					return 0;
				}
			}
		}


		return 1;
	}

	int oCNpc::CompareOverlaysArray(zCArray<int> npcMdsList) {

		zCArray<int> npcMdsListMy = GetNpcMds(this);

		if (npcMdsListMy.GetNumInList() != npcMdsList.GetNumInList()) {
			return 0;
		}
		else {
			for (int i = 0; i < npcMdsListMy.GetNumInList(); i++) {
				int mdsMy = npcMdsListMy.GetSafe(i);
				if (!npcMdsList.IsInList(mdsMy)) {
					return 0;
				}
			}
		}


		return 1;
	}

	void oCNpc::RemoveOverlayMds(const zSTRING& mds) {
		RemoveOverlay(mds);
		if (GetAnictrl())
			GetAnictrl()->InitAnimations();
	};

	void oCNpc::ApplyOverlayMds(const zSTRING& mds) {
		ApplyOverlay(mds);
		if (GetAnictrl())
			GetAnictrl()->InitAnimations();
	};

	void zCParser::CallFuncByName(zSTRING name) {
		int idx = GetIndex(name);

		if (idx != -1) {
			CallFunc(idx);
		}
	}

	int GetRandVal(int min, int max) {
		return (rand() % (max - min + 1) + min);
	}

	zCArray<zCVob*> CollectVobsInRadius(zVEC3 pos, int radius)
	{
		zTBBox3D box;
		zCArray<zCVob*> vobList;

		if (!player->GetHomeWorld())
		{
			return vobList;
		}

		const float radiusFloat = static_cast<float>(radius);
		box.maxs = pos + (zVEC3(1, 1, 1) * radiusFloat);
		box.mins = pos - (zVEC3(1, 1, 1) * radiusFloat);

		oCNpc* other = NULL;

		player->GetHomeWorld()->bspTree.bspRoot->CollectVobsInBBox3D(vobList, box);
		return vobList;
	}

	ENetPacketFlag PacketFlag(json packet)
	{
		auto updateType = (UpdateType)packet["type"].get<int>();

		if (updateType == SYNC_POS || updateType == SYNC_HEADING) {
			return ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT;
		}

		return ENET_PACKET_FLAG_RELIABLE;
	}

	int PacketChannel(json packet)
	{
		auto updateType = (UpdateType)packet["type"].get<int>();

		if (updateType == SYNC_POS || updateType == SYNC_HEADING) {
			return 1;
		}

		return 0;
	}
}