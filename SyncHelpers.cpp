namespace GOTHIC_ENGINE {
	RemoteNpc* addSyncedNpc(string uniqueName) {
		auto myFriend = new RemoteNpc(uniqueName);
		SyncNpcs[uniqueName] = myFriend;
		return myFriend;
	}

	void removeSyncedNpc(string uniqueName) {
		auto removedNpc = SyncNpcs[uniqueName];
		json j; 
		j["type"] = DESTROY_NPC;
		removedNpc->localUpdates.push_back(j);
	}
}