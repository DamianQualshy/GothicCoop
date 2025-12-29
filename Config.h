#ifdef A
#pragma push_macro("A")
#undef A
#define TOMLPP_RESTORE_A
#endif

#ifdef min
#pragma push_macro("min")
#undef min
#define TOMLPP_RESTORE_MIN
#endif

#ifdef max
#pragma push_macro("max")
#undef max
#define TOMLPP_RESTORE_MAX
#endif

#include "dependencies/toml++/toml.hpp"

#ifdef TOMLPP_RESTORE_MAX
#pragma pop_macro("max")
#undef TOMLPP_RESTORE_MAX
#endif

#ifdef TOMLPP_RESTORE_MIN
#pragma pop_macro("min")
#undef TOMLPP_RESTORE_MIN
#endif

#ifdef TOMLPP_RESTORE_A
#pragma pop_macro("A")
#undef TOMLPP_RESTORE_A
#endif

#include <string>

namespace GOTHIC_ENGINE {
    class Config {
    public:
        struct Values {
            std::string connectionServer;
            int connectionPort = 0;
            std::string nickname;
            std::string friendInstance;
            std::string bodyModel = "";
            int bodyTex = 0;
            std::string headModel = "";
            int headTex = 0;
            int skinColor = 0;
            int playersDamageMultiplier = 0;
            int npcsDamageMultiplier = 0;
            std::string toggleGameLogKey;
            std::string toggleGameStatsKey;
            std::string startServerKey;
            std::string startConnectionKey;
            std::string reinitPlayersKey;
            std::string revivePlayerKey;
        };

        Config();

        bool LoadFromFile(const std::string& path, bool persistDefaults);

        const std::string& ConnectionServer() const;
        int ConnectionPort() const;
        const std::string& Nickname() const;
        const std::string& FriendInstance() const;
        const std::string& BodyModel() const;
        int BodyTex() const;
        const std::string& HeadModel() const;
        int HeadTex() const;
        int BodyColor() const;
        int PlayersDamageMultiplier() const;
        int NpcsDamageMultiplier() const;

        int ToggleGameLogKeyCode() const;
        int ToggleGameStatsKeyCode() const;
        int StartServerKeyCode() const;
        int StartConnectionKeyCode() const;
        int ReinitPlayersKeyCode() const;
        int RevivePlayerKeyCode() const;

    private:
        Values values_;

        Values DefaultValues() const;
        bool PersistConfig(const std::string& path) const;

        void LogIssue(const std::string& message) const;

        int ToKeyCode(const std::string& keyValue, const std::string& fallbackKey) const;
        bool IsValidKeyCode(const std::string& keyValue) const;
    };
}