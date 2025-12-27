namespace GOTHIC_ENGINE {
    enum UpdateType
    {
        SYNC_POS,
        SYNC_HEADING,
        SYNC_ANIMATION,
        SYNC_WEAPON_MODE,
        INIT_NPC,
        DESTROY_NPC,
        SYNC_ATTACKS,
        SYNC_ARMOR,
        SYNC_WEAPONS,
        SYNC_HP,
        SYNC_TIME,
        SYNC_HAND,
        SYNC_MAGIC_SETUP,
        SYNC_SPELL_CAST,
        SYNC_REVIVED,
        SYNC_PROTECTIONS,
        SYNC_PLAYER_NAME,
        PLAYER_DISCONNECT,
        SYNC_TALENTS,
        SYNC_BODYSTATE,
        SYNC_OVERLAYS,
        SYNC_DROPITEM,
        SYNC_TAKEITEM,
    };

    struct PlayerHit
    {
        string npcUniqueName;
        float damage;
        oCNpc* npc;
        oCNpc* attacker;
        int isUnconscious;
        bool isDead;
        bool isFinish;
        unsigned long damageMode;
    };

    struct SpellCast
    {
        oCNpc* npc;
        oCNpc* targetNpc;
        string npcUniqueName;
        string targetNpcUniqueName;
        int spellInstanceId = 0;
        int spellLevel = 0;
        int spellCharge = 0;
    };

    class PeerData
    {
    public:
        string friendId;
        string nickname;
        int friendIdNumber = -1;
        bool announced = false;
        PeerData() {}
    };
}