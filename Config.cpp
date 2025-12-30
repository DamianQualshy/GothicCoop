#include <fstream>
#include <sstream>

namespace GOTHIC_ENGINE {
    void CoopLog(std::string l);

    namespace {
        const char* kDefaultServer = "localhost";
        const int kDefaultPort = 1234;
        const char* kDefaultFriendInstance = "CH";
        const int kDefaultPlayersDamageMultiplier = 50;
        const int kDefaultNpcsDamageMultiplier = 100;
        const int kDefaultStartupGuardMs = 2000;
        const char* kDefaultBodyModel = "HUM_BODY_NAKED0";
        const char* kDefaultHeadModel = "HUM_HEAD_PONY";

#if ENGINE >= Engine_G2
        const int kDefaultBodyTex = 9;
        const int kDefaultHeadTex = 18;
        const int kDefaultBodyColor = 0;
#else
        const int kDefaultBodyTex = 4;
        const int kDefaultHeadTex = 9;
        const int kDefaultBodyColor = 1;
#endif

        const char* kDefaultToggleGameLogKey = "KEY_P";
        const char* kDefaultToggleGameStatsKey = "KEY_O";
        const char* kDefaultStartServerKey = "KEY_F1";
        const char* kDefaultStartConnectionKey = "KEY_F2";
        const char* kDefaultReinitPlayersKey = "KEY_F3";
        const char* kDefaultRevivePlayerKey = "KEY_F4";

        const int kPortMin = 1024;
        const int kPortMax = 65535;
        const int kBodyTexMin = 0;
        const int kBodyTexMax = 255;
        const int kHeadTexMin = 0;
        const int kHeadTexMax = 255;
        const int kSkinColorMin = 0;
        const int kSkinColorMax = 3;
        const int kDamageMultiplierMin = 0;
        const int kDamageMultiplierMax = 500;
        const int kStartupGuardMin = 0;
        const int kStartupGuardMax = 10000;

        const toml::node* FindNode(const toml::table& table, const char* section, const char* key) {
            if (section && section[0] != '\0') {
                if (auto sectionTable = table[section].as_table()) {
                    return sectionTable->get(key);
                }
                return nullptr;
            }

            return table.get(key);
        }

        std::string DescribeKey(const char* section, const char* key) {
            if (!section || section[0] == '\0') {
                return std::string(key);
            }

            std::string description(section);
            description.append(".");
            description.append(key);
            return description;
        }

        template <typename LogFn>
        std::string ReadString(const toml::table& table,
                               const char* section,
                               const char* key,
                               const std::string& defaultValue,
                               bool required,
                               bool* needsPersist,
                               LogFn&& logIssue) {
            const toml::node* node = FindNode(table, section, key);
            if (!node) {
                if (required) {
                    logIssue("Missing required key '" + DescribeKey(section, key) + "', using default.");
                    if (needsPersist) {
                        *needsPersist = true;
                    }
                }
                return defaultValue;
            }

            auto value = node->value<std::string>();
            if (!value) {
                logIssue("Invalid type for key '" + DescribeKey(section, key) + "', expected string.");
                if (needsPersist) {
                    *needsPersist = true;
                }
                return defaultValue;
            }

            if (required && value->empty()) {
                logIssue("Empty value for key '" + DescribeKey(section, key) + "', using default.");
                if (needsPersist) {
                    *needsPersist = true;
                }
                return defaultValue;
            }

            return *value;
        }

        template <typename LogFn>
        int ReadInt(const toml::table& table,
                    const char* section,
                    const char* key,
                    int defaultValue,
                    int minValue,
                    int maxValue,
                    bool required,
                    bool* needsPersist,
                    LogFn&& logIssue) {
            const toml::node* node = FindNode(table, section, key);
            if (!node) {
                if (required) {
                    logIssue("Missing required key '" + DescribeKey(section, key) + "', using default.");
                    if (needsPersist) {
                        *needsPersist = true;
                    }
                }
                return defaultValue;
            }

            auto value = node->value<int64_t>();
            if (!value) {
                logIssue("Invalid type for key '" + DescribeKey(section, key) + "', expected integer.");
                if (needsPersist) {
                    *needsPersist = true;
                }
                return defaultValue;
            }

            if (*value < minValue || *value > maxValue) {
                std::ostringstream message;
                message << "Value for key '" << DescribeKey(section, key)
                        << "' out of range (" << minValue << "-" << maxValue << "), using default.";
                logIssue(message.str());
                if (needsPersist) {
                    *needsPersist = true;
                }
                return defaultValue;
            }

            return static_cast<int>(*value);
        }

        template <typename LogFn, typename ValidatorFn>
        std::string ReadKeyString(const toml::table& table,
                                  const char* key,
                                  const std::string& defaultValue,
                                  bool* needsPersist,
                                  LogFn&& logIssue,
                                  ValidatorFn&& isValidKey) {
            const toml::node* node = FindNode(table, "controls", key);
            if (!node) {
                logIssue("Missing required key '" + DescribeKey("controls", key) + "', using default.");
                if (needsPersist) {
                    *needsPersist = true;
                }
                return defaultValue;
            }

            auto value = node->value<std::string>();
            if (!value || value->empty()) {
                logIssue("Invalid value for key '" + DescribeKey("controls", key) + "', expected non-empty string.");
                if (needsPersist) {
                    *needsPersist = true;
                }
                return defaultValue;
            }

            if (!isValidKey(*value)) {
                logIssue("Unknown key code '" + *value + "' for key '" + DescribeKey("controls", key) + "', using default.");
                if (needsPersist) {
                    *needsPersist = true;
                }
                return defaultValue;
            }

            return *value;
        }
    }

    Config::Config() {
        values_ = DefaultValues();
    }

    Config::Values Config::DefaultValues() const {
        Values defaults;
        defaults.connectionServer = kDefaultServer;
        defaults.connectionPort = kDefaultPort;
        defaults.nickname.clear();
        defaults.friendInstance = kDefaultFriendInstance;
        defaults.bodyModel = kDefaultBodyModel;
        defaults.bodyTex = kDefaultBodyTex;
        defaults.headModel = kDefaultHeadModel;
        defaults.headTex = kDefaultHeadTex;
        defaults.skinColor = kDefaultBodyColor;
        defaults.playersDamageMultiplier = kDefaultPlayersDamageMultiplier;
        defaults.npcsDamageMultiplier = kDefaultNpcsDamageMultiplier;
        defaults.startupGuardMs = kDefaultStartupGuardMs;
        defaults.toggleGameLogKey = kDefaultToggleGameLogKey;
        defaults.toggleGameStatsKey = kDefaultToggleGameStatsKey;
        defaults.startServerKey = kDefaultStartServerKey;
        defaults.startConnectionKey = kDefaultStartConnectionKey;
        defaults.reinitPlayersKey = kDefaultReinitPlayersKey;
        defaults.revivePlayerKey = kDefaultRevivePlayerKey;
        return defaults;
    }

    bool Config::LoadFromFile(const std::string& path, bool persistDefaults) {
        values_ = DefaultValues();
        bool needsPersist = false;

        std::ifstream configFile(path);
        if (!configFile.good()) {
            LogIssue("Config file not found: " + path + ". Using defaults.");
            if (persistDefaults) {
                PersistConfig(path);
            }
            return false;
        }

        toml::table config;
        try {
            config = toml::parse_file(path);
        }
        catch (const toml::parse_error& error) {
            std::ostringstream message;
            message << "TOML parsing error: " << error.description();
            if (error.source().begin) {
                message << " at line " << error.source().begin.line << " column " << error.source().begin.column;
            }
            LogIssue(message.str());
            if (persistDefaults) {
                PersistConfig(path);
            }
            return false;
        }

        auto logIssue = [this](const std::string& message) { LogIssue(message); };

        values_.connectionServer = ReadString(config, "connection", "server", values_.connectionServer, true, &needsPersist, logIssue);
        values_.connectionPort = ReadInt(config, "connection", "port", values_.connectionPort, kPortMin, kPortMax, true, &needsPersist, logIssue);
        values_.nickname = ReadString(config, "player", "nickname", values_.nickname, false, &needsPersist, logIssue);
        values_.friendInstance = ReadString(config, "player", "friendInstance", values_.friendInstance, false, &needsPersist, logIssue);
        values_.bodyModel = ReadString(config, "appearance", "bodyModel", values_.bodyModel, false, &needsPersist, logIssue);
        values_.bodyTex = ReadInt(config, "appearance", "bodyTex", values_.bodyTex, kBodyTexMin, kBodyTexMax, false, &needsPersist, logIssue);
        values_.headModel = ReadString(config, "appearance", "headModel", values_.headModel, false, &needsPersist, logIssue);
        values_.headTex = ReadInt(config, "appearance", "headTex", values_.headTex, kHeadTexMin, kHeadTexMax, false, &needsPersist, logIssue);
        values_.skinColor = ReadInt(config, "appearance", "skinColorG1", values_.skinColor, kSkinColorMin, kSkinColorMax, false, &needsPersist, logIssue);
        values_.playersDamageMultiplier = ReadInt(config, "gameplay", "playersDamageMultiplier", values_.playersDamageMultiplier, kDamageMultiplierMin, kDamageMultiplierMax, false, &needsPersist, logIssue);
        values_.npcsDamageMultiplier = ReadInt(config, "gameplay", "npcsDamageMultiplier", values_.npcsDamageMultiplier, kDamageMultiplierMin, kDamageMultiplierMax, false, &needsPersist, logIssue);
        values_.startupGuardMs = ReadInt(config, "gameplay", "startupGuardMs", values_.startupGuardMs, kStartupGuardMin, kStartupGuardMax, false, &needsPersist, logIssue);

        auto isValidKey = [this](const std::string& keyValue) { return IsValidKeyCode(keyValue); };
        values_.toggleGameLogKey = ReadKeyString(config, "toggleGameLogKey", values_.toggleGameLogKey, &needsPersist, logIssue, isValidKey);
        values_.toggleGameStatsKey = ReadKeyString(config, "toggleGameStatsKey", values_.toggleGameStatsKey, &needsPersist, logIssue, isValidKey);
        values_.startServerKey = ReadKeyString(config, "startServerKey", values_.startServerKey, &needsPersist, logIssue, isValidKey);
        values_.startConnectionKey = ReadKeyString(config, "startConnectionKey", values_.startConnectionKey, &needsPersist, logIssue, isValidKey);
        values_.reinitPlayersKey = ReadKeyString(config, "reinitPlayersKey", values_.reinitPlayersKey, &needsPersist, logIssue, isValidKey);
        values_.revivePlayerKey = ReadKeyString(config, "revivePlayerKey", values_.revivePlayerKey, &needsPersist, logIssue, isValidKey);

        if (persistDefaults && needsPersist) {
            PersistConfig(path);
        }

        return true;
    }

    const std::string& Config::ConnectionServer() const {
        return values_.connectionServer;
    }

    int Config::ConnectionPort() const {
        return values_.connectionPort;
    }

    const std::string& Config::Nickname() const {
        return values_.nickname;
    }

    const std::string& Config::FriendInstance() const {
        return values_.friendInstance;
    }

    const std::string& Config::BodyModel() const {
        return values_.bodyModel;
    }

    int Config::BodyTex() const {
        return values_.bodyTex;
    }

    const std::string& Config::HeadModel() const {
        return values_.headModel;
    }

    int Config::HeadTex() const {
        return values_.headTex;
    }

    int Config::BodyColor() const {
        return values_.skinColor;
    }

    int Config::PlayersDamageMultiplier() const {
        return values_.playersDamageMultiplier;
    }

    int Config::NpcsDamageMultiplier() const {
        return values_.npcsDamageMultiplier;
    }

    int Config::StartupGuardMs() const {
        return values_.startupGuardMs;
    }

    int Config::ToggleGameLogKeyCode() const {
        return ToKeyCode(values_.toggleGameLogKey, kDefaultToggleGameLogKey);
    }

    int Config::ToggleGameStatsKeyCode() const {
        return ToKeyCode(values_.toggleGameStatsKey, kDefaultToggleGameStatsKey);
    }

    int Config::StartServerKeyCode() const {
        return ToKeyCode(values_.startServerKey, kDefaultStartServerKey);
    }

    int Config::StartConnectionKeyCode() const {
        return ToKeyCode(values_.startConnectionKey, kDefaultStartConnectionKey);
    }

    int Config::ReinitPlayersKeyCode() const {
        return ToKeyCode(values_.reinitPlayersKey, kDefaultReinitPlayersKey);
    }

    int Config::RevivePlayerKeyCode() const {
        return ToKeyCode(values_.revivePlayerKey, kDefaultRevivePlayerKey);
    }

    bool Config::PersistConfig(const std::string& path) const {
        toml::table config;
        config.insert("connection", toml::table{
            {"server", values_.connectionServer},
            {"port", values_.connectionPort}
        });
        config.insert("player", toml::table{
            {"nickname", values_.nickname},
            {"friendInstance", values_.friendInstance}
        });
        config.insert("appearance", toml::table{
            {"bodyModel", values_.bodyModel},
            {"bodyTex", values_.bodyTex},
            {"headModel", values_.headModel},
            {"headTex", values_.headTex},
            {"skinColorG1", values_.skinColor}
        });
        config.insert("gameplay", toml::table{
            {"playersDamageMultiplier", values_.playersDamageMultiplier},
            {"npcsDamageMultiplier", values_.npcsDamageMultiplier},
            {"startupGuardMs", values_.startupGuardMs}
        });
        config.insert("controls", toml::table{
            {"toggleGameLogKey", values_.toggleGameLogKey},
            {"toggleGameStatsKey", values_.toggleGameStatsKey},
            {"startServerKey", values_.startServerKey},
            {"startConnectionKey", values_.startConnectionKey},
            {"reinitPlayersKey", values_.reinitPlayersKey},
            {"revivePlayerKey", values_.revivePlayerKey}
        });

        std::ofstream output(path);
        if (!output.good()) {
            LogIssue("Unable to write config file: " + path);
            return false;
        }

        output << config;
        return true;
    }

    void Config::LogIssue(const std::string& message) const {
        CoopLog("[Config] " + message + "\r\n");
    }

    int Config::ToKeyCode(const std::string& keyValue, const std::string& fallbackKey) const {
        int keyCode = GetEmulationKeyCode(string(keyValue.c_str()));
        if (keyCode == 0) {
            LogIssue("Invalid key code '" + keyValue + "', using fallback '" + fallbackKey + "'.");
            return GetEmulationKeyCode(string(fallbackKey.c_str()));
        }
        return keyCode;
    }

    bool Config::IsValidKeyCode(const std::string& keyValue) const {
        return GetEmulationKeyCode(string(keyValue.c_str())) != 0;
    }
}