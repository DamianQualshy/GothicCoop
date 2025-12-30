#include <set>

namespace GOTHIC_ENGINE {
    const int COOP_VERSION = 60;
    const int COOP_MAGIC_NUMBER = 1337;
    int BROADCAST_DISTANCE = 4500;

    DWORD MainThreadId;
    std::string PluginState = "";
    Chat* GameChat = new Chat();

    char GothicExeFolderPath[MAX_PATH];
    std::string GothicCoopLogPath;

    string MyselfId = "_player_";
    static LocalNpc* Myself = NULL;
    std::set<int> ActiveFriendIds;
    long long CurrentMs = 0;
    Config CoopConfig;
    static bool IsLoadingLevel = false;
    static bool IsSavingGame = false;
    static bool IsCoopPaused = false;
    long long LastUpdateListOfVisibleNpcs = 0;
    long long LastNpcListRefreshTime = 0;
    zVEC3* CurrentWorldTOTPosition;
    int CurrentPing = -1;

    string FriendInstance = "ch";
    string MyNickname = "";

    int PlayersDamageMultipler = 50;
    int NpcsDamageMultipler = 100;
    int ToggleGameLogKey;
    int ToggleGameStatsKey;
    int StartServerKey;
    int StartConnectionKey;
    int ReinitPlayersKey;
    int RevivePlayerKey;

    string MyBodyModel = "HUM_BODY_NAKED0";
    string MyHeadModel = "HUM_HEAD_PONY";
#if ENGINE >= Engine_G2
    int MyBodyTex = 9;
    int MyHeadTex = 18;
    int MyBodyColor = 0;
#else
    int MyBodyTex = 4;
    int MyHeadTex = 9;
    int MyBodyColor = 1;
#endif

    int ConnectionPort = 1234;

    static Thread* ServerThread = NULL;
    static Thread* ClientThread = NULL;

    std::map<string, LocalNpc*> BroadcastNpcs;
    std::map<string, RemoteNpc*> SyncNpcs;

    static std::map<string, oCNpc*> UniqueNameToNpcList;
    static std::map<oCNpc*, string> NpcToUniqueNameList;

    static std::map<string, int> NamesCounter;
    static std::map<oCNpc*, string> NpcToFirstRoutineWp;

    static std::map<oCNpc*, string> PlayerNpcs;
    static std::map<string, oCNpc*> PlayerNameToNpc;

    static std::map<string, oCNpc*> KilledByPlayerNpcNames;

    static SafeQueue<NetworkPacket> ReadyToSendPackets;
    static SafeQueue<NetworkPacket> ReadyToBeDistributedPackets;
    static SafeQueue<ENetEvent> ReadyToBeReceivedPackets;
    static SafeQueue<PlayerHit> ReadyToSyncDamages;
    static SafeQueue<SpellCast> ReadyToSyncSpellCasts;

    static const int IgnoredSyncNpcsCount = 9;
    static const char IgnoredSyncNpcs[IgnoredSyncNpcsCount][30] =
    {
        {"AD_OLDGHOSTRIDDLE1AD_LAST"},
        {"AD_OLDGHOSTRIDDLE2AD_LAST"},
        {"AD_OLDGHOSTRIDDLE3AD_LAST"},
        {"AD_OLDGHOSTRIDDLE4AD_LAST"},
        {"AD_OLDGHOSTRIDDLE5AD_LAST"},
        {"AD_OLDGHOSTRIDDLE6AD_LAST"},
        {"AD_OLDGHOSTRIDDLE7AD_LAST"},
        {"AD_OLDGHOSTRIDDLE8AD_LAST"},
        {"WISP_DETECTOR"},
    };
}