namespace GOTHIC_ENGINE {
	RemoteNpc* addSyncedNpc(string uniqueName) {
		auto myFriend = new RemoteNpc(uniqueName);
		SyncNpcs[uniqueName] = myFriend;
		return myFriend;
	}

	void removeSyncedNpc(string uniqueName) {
		auto removedIt = SyncNpcs.find(uniqueName);
		if (removedIt == SyncNpcs.end()) {
			return;
		}
		auto removedNpc = removedIt->second;
		if (!removedNpc) {
			return;
		}
		PlayerStateUpdatePacket update;
		update.updateType = DESTROY_NPC;
		removedNpc->localUpdates.push_back(update);
	}
}