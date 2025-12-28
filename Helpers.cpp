#include <stdexcept>

namespace GOTHIC_ENGINE {
    static std::string TrimWhitespace(const std::string& text) {
        const auto first = text.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) {
            return "";
        }
        const auto last = text.find_last_not_of(" \t\r\n");
        return text.substr(first, last - first + 1);
    }

    static std::string StripTomlComment(const std::string& text) {
        bool inQuotes = false;
        for (size_t i = 0; i < text.size(); i++) {
            if (text[i] == '"' && (i == 0 || text[i - 1] != '\\')) {
                inQuotes = !inQuotes;
                continue;
            }

            if (text[i] == '#' && !inQuotes) {
                return text.substr(0, i);
            }
        }

        return text;
    }

    json ParseTomlConfig(std::istream& input) {
        json config = json::object();
        std::string line;
        std::string currentSection;
        int lineNumber = 0;

        while (std::getline(input, line)) {
            lineNumber += 1;
            auto cleaned = TrimWhitespace(StripTomlComment(line));
            if (cleaned.empty()) {
                continue;
            }

            if (cleaned.front() == '[' && cleaned.back() == ']') {
                currentSection = TrimWhitespace(cleaned.substr(1, cleaned.size() - 2));
                if (currentSection.empty()) {
                    throw std::runtime_error("TOML parsing error: empty section name on line " + std::to_string(lineNumber));
                }
                continue;
            }

            auto separator = cleaned.find('=');
            if (separator == std::string::npos) {
                throw std::runtime_error("TOML parsing error: missing '=' on line " + std::to_string(lineNumber));
            }

            auto key = TrimWhitespace(cleaned.substr(0, separator));
            auto valueRaw = TrimWhitespace(cleaned.substr(separator + 1));

            if (key.empty() || valueRaw.empty()) {
                throw std::runtime_error("TOML parsing error: invalid key/value on line " + std::to_string(lineNumber));
            }

            json value;
            if (valueRaw.front() == '"' && valueRaw.back() == '"' && valueRaw.size() >= 2) {
                value = valueRaw.substr(1, valueRaw.size() - 2);
            }
            else {
                try {
                    value = std::stoi(valueRaw);
                }
                catch (...) {
                    value = valueRaw;
                }
            }

            if (currentSection.empty()) {
                config[key] = value;
            }
            else {
                config[currentSection][key] = value;
            }
        }

        return config;
    }

    static bool TryReadConfigValue(const std::string& section, const std::string& key, json& valueOut) {
        if (section.empty()) {
            if (!CoopConfig.contains(key)) {
                return false;
            }
            valueOut = CoopConfig[key];
            return true;
        }

        if (!CoopConfig.contains(section)) {
            return false;
        }

        auto& sectionConfig = CoopConfig[section];
        if (!sectionConfig.contains(key)) {
            return false;
        }

        valueOut = sectionConfig[key];
        return true;
    }

    std::string ReadConfigString(const std::string& section, const std::string& key, const std::string& defaultValue) {
        json value;
        if (TryReadConfigValue(section, key, value) && value.is_string()) {
            return value.get<std::string>();
        }

        return defaultValue;
    }

    int ReadConfigInt(const std::string& section, const std::string& key, int defaultValue) {
        json value;
        if (TryReadConfigValue(section, key, value) && value.is_number_integer()) {
            return value.get<int>();
        }

        return defaultValue;
    }

    void CoopLog(std::string l)
    {
        std::ofstream CoopLog(GothicCoopLogPath, std::ios_base::app | std::ios_base::out);
        CoopLog << l;
    }

    void ChatLog(string text, zCOLOR color = zCOLOR(255, 255, 255, 255)) {
        GameChat->AddLine(text, color);
    };

    int GetFreePlayerId() {
        for (int id = 1; ; id++) {
            if (ActiveFriendIds.count(id) == 0) {
                ActiveFriendIds.insert(id);
                return id;
            }
        }
    }

    void ReleasePlayerId(int id) {
        if (id > 0) {
            ActiveFriendIds.erase(id);
        }
    }

    bool IsCoopPlayer(std::string name) {
        string cStringName = name.c_str();
        return cStringName == "HOST" || cStringName.StartWith("FRIEND_");
    }

    bool IsCoopPlayer(string name) {
        return name == "HOST" || name.StartWith("FRIEND_");
    }

    oCItem* CreateCoopItem(int insIndex) {
        return zfactory->CreateItem(insIndex);
    }

    std::vector<oCNpc*> GetVisibleNpcs() {
        std::vector<oCNpc*> npcs;
        auto* list = ogame->GetGameWorld()->voblist_npcs->next;

        while (list) {
            auto npc = list->data;
            auto npcPosition = npc->GetPositionWorld();
            auto playerPosition = player->GetPositionWorld();

            if (npc->vobLeafList.GetNum() == 0) {
                list = list->next;
                continue;
            }

            if (playerPosition.Distance(npcPosition) < BROADCAST_DISTANCE) {
                if (!npc->IsAPlayer() && !npc->GetObjectName().StartWith("FRIEND_")) {
                    npcs.push_back(npc);
                }
            }
            list = list->next;
        }

        return npcs;
    }

    float GetHeading(oCNpc* npc)
    {
        float x = *(float*)((DWORD)npc + 0x44);
        float rotx = asin(x) * 180.0f / 3.14f;
        float y = *(float*)((DWORD)npc + 0x64);
        if (y > 0)
        {
            if (x < 0)
                rotx = 360 + rotx;
        }
        else
        {
            if (rotx > 0)
                rotx = 180 - rotx;
            else
            {
                rotx = 180 + rotx;
                rotx = 360 - rotx;
            }
        }
        return rotx;
    };

    int ReadConfigKey(const std::string& section, const std::string& key, string _default) {
        auto stringKey = string(ReadConfigString(section, key, _default.ToChar()).c_str()).ToChar();

        return GetEmulationKeyCode(stringKey);
    }

    bool IsPlayerTalkingWithAnybody() {
        return ogame->GetCameraAI()->GetMode().Compare("CAMMODDIALOG");
    }

    bool IsPlayerTalkingWithNpc(zCVob* npc) {
        if (!ogame->GetCameraAI()->GetMode().Compare("CAMMODDIALOG")) {
            return false;
        }

        if (player->talkOther == npc) {
            return true;
        }

        if (ogame->GetCameraAI()->targetVobList.GetNum() > 0) {
            for (int i = 0; i < ogame->GetCameraAI()->targetVobList.GetNum(); i++) {
                auto vob = ogame->GetCameraAI()->targetVobList[i];
                if (npc == vob) {
                    return true;
                }
            }
        }

        return false;
    }

    float GetDistance3D(float aX, float aY, float aZ, float bX, float bY, float bZ)
    {
        float distX = aX - bX;
        float distY = aY - bY;
        float distZ = aZ - bZ;
        return sqrt(distX * distX + distY * distY + distZ * distZ);
    };

    bool IgnoredSyncNpc(zCVob* npc) {
        auto name = npc->GetObjectName();

        for (unsigned int i = 0; i < IgnoredSyncNpcsCount; i++)
        {
            if (strcmp(name.ToChar(), IgnoredSyncNpcs[i]) == 0)
                return true;
        }

        return false;
    }

    void BuildGlobalNpcList() {
        PluginState = "BuildGlobalNpcList";

        auto* list = ogame->GetGameWorld()->voblist_npcs->next;
        auto firstRun = NpcToFirstRoutineWp.size() == 0;

        if (firstRun) {
            auto* rtnList = rtnMan->rtnList.next;
            while (rtnList) {
                auto rtn = rtnList->data;
                if (NpcToFirstRoutineWp.count(rtn->npc) == 0) {
                    NpcToFirstRoutineWp[rtn->npc] = rtn->wpname.ToChar();
                }
                rtnList = rtnList->next;
            }

            while (list) {
                auto npc = list->data;
                NamesCounter[string(npc->GetObjectName())] += 1;
                list = list->next;
            }
        }

        list = ogame->GetGameWorld()->voblist_npcs->next;
        while (list) {
            auto npc = list->data;

            if (npc->IsAPlayer() || npc->GetObjectName().StartWith("FRIEND_") || IgnoredSyncNpc(npc) || NpcToUniqueNameList.count(npc)) {
                list = list->next;
                continue;
            }

            auto objectName = string(npc->GetObjectName());
            CStringA name = "";
            if (NamesCounter[objectName] == 1) {
                name = objectName;
            }
            else {
                auto secondUniquePart = npc->wpname ? string(npc->wpname) : string("UNKNOW");
                if (NpcToFirstRoutineWp.count(npc) > 0 && !NpcToFirstRoutineWp[npc].IsEmpty()) {
                    secondUniquePart = NpcToFirstRoutineWp[npc];
                }
                if (!firstRun) {
                    secondUniquePart = npc->wpname ? string::Combine("DYNAMIC-%s", string(npc->wpname)) : string("DYNAMIC");
                }
                name = string::Combine("%s-%s", string(npc->GetObjectName()), secondUniquePart);
                NamesCounter[name] += 1;
            }

            auto uniqueName = string::Combine("%s-%i", name, NamesCounter[name]);
            UniqueNameToNpcList[uniqueName] = npc;
            NpcToUniqueNameList[npc] = uniqueName;

            list = list->next;
        }
    }

    bool EnsureNpcUniqueName(oCNpc* npc) {
        if (!npc) {
            return false;
        }

        if (NpcToUniqueNameList.count(npc) > 0) {
            return true;
        }

        if (npc->IsAPlayer() || npc->GetObjectName().StartWith("FRIEND_") || IgnoredSyncNpc(npc)) {
            return false;
        }

        BuildGlobalNpcList();
        return NpcToUniqueNameList.count(npc) > 0;
    }

    long long GetCurrentMs() {
        std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
            );

        return ms.count();
    }

    static int GetPartyMemberID() {
        zCPar_Symbol* sym = parser->GetSymbol("AIV_PARTYMEMBER");
        if (!sym)
#if ENGINE >= Engine_G2
            return 15;
#else
            return 36;
#endif
        int id;
        sym->GetValue(id, 0);
        return id;
    }

    static int GetFriendDefaultInstanceId() {
        auto instId = parser->GetIndex(FriendInstanceId);

        return instId;
    }
}