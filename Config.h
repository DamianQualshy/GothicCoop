#include <string>

namespace toml {
    class table;
}

namespace GOTHIC_ENGINE {
    class Config {
    public:
        struct Values {
            std::string connectionServer;
            int connectionPort = 0;
            std::string nickname;
            std::string friendInstanceId;
            std::string BodyModel = "";
            int BodyTex = 0;
            std::string HeadModel = "";
            int HeadTex = 0;
            int BodyColor = 0;
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
        const std::string& FriendInstanceId() const;
        const std::string& BodyModel() const;
        int BodyTex() const;
        const std::string& HeadModel() const;
        int HeadTex() const;
        int SkinColor() const;
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

        std::string ReadString(const toml::table& table,
                               const char* section,
                               const char* key,
                               const std::string& defaultValue,
                               bool required,
                               bool* needsPersist) const;
        int ReadInt(const toml::table& table,
                    const char* section,
                    const char* key,
                    int defaultValue,
                    int minValue,
                    int maxValue,
                    bool required,
                    bool* needsPersist) const;
        std::string ReadKeyString(const toml::table& table,
                                  const char* key,
                                  const std::string& defaultValue,
                                  bool* needsPersist) const;
        int ToKeyCode(const std::string& keyValue, const std::string& fallbackKey) const;
        bool IsValidKeyCode(const std::string& keyValue) const;
    };
}