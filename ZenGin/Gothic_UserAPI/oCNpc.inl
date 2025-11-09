// Supported with union (c) 2020 Union team

// User API for oCNpc
// Add your methods here

void ApplyOverlayMds(const zSTRING& mds);
void RemoveOverlayMds(const zSTRING& mds);
void oCNpc::ApplyOverlaysNpc(oCNpc* npc);
void oCNpc::ApplyOverlaysArray(zCArray<int> npcMdsArray);
int oCNpc::CompareOverlaysMds(oCNpc* npc);
int oCNpc::CompareOverlaysArray(zCArray<int> npcMdsList);