#include <cstdint>
#include <vector>
#include <string>
#include <sstream>
#include <cmath>
#include <cstring>

namespace GOTHIC_ENGINE {
    enum class PacketType : std::uint8_t {
        JoinGame = 1,
        PlayerDisconnect = 2,
        PlayerStateUpdate = 3,
    };

    struct JoinGamePacket {
        std::uint32_t connectId = 0;
        std::string name;
    };

    struct PlayerDisconnectPacket {
        std::string name;
        std::string nickname;
        bool hasNickname = false;
    };

    struct InitNpcPayload {
        int instanceId = 0;
        std::string nickname;
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        int bodyTextVarNr = 0;
        int headVarNr = 0;
    };

    struct SyncPosPayload {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    struct SyncHeadingPayload {
        float heading = 0.0f;
    };

    struct SyncAnimationPayload {
        int animationId = 0;
    };

    struct SyncWeaponModePayload {
        int weaponMode = 0;
    };

    struct SyncMagicSetupPayload {
        std::string spellInstanceName;
    };

    struct SpellCastInfo {
        std::string target;
        int spellInstanceId = 0;
        int spellLevel = 0;
        int spellCharge = 0;
    };

    struct SyncSpellCastPayload {
        std::vector<SpellCastInfo> casts;
    };

    struct SyncArmorPayload {
        std::string armor;
    };

    struct SyncWeaponsPayload {
        std::string weapon1;
        std::string weapon2;
    };

    struct SyncHpPayload {
        int hp = 0;
        int hpMax = 0;
    };

    struct SyncBodyStatePayload {
        int bodyState = 0;
    };

    struct SyncOverlaysPayload {
        std::vector<int> overlayIds;
    };

    struct SyncProtectionsPayload {
        int protections[8] = {0};
    };

    struct SyncTalentsPayload {
        int talents[4] = {0};
    };

    struct SyncHandPayload {
        std::string leftItem;
        std::string rightItem;
    };

    struct SyncTimePayload {
        int hour = 0;
        int minute = 0;
    };

    struct SyncRevivedPayload {
        std::string name;
    };

    struct AttackInfo {
        std::string target;
        float damage = 0.0f;
        int isUnconscious = 0;
        bool isDead = false;
        bool isFinish = false;
        unsigned long damageMode = 0;
    };

    struct SyncAttacksPayload {
        std::vector<AttackInfo> attacks;
    };

    struct SyncDropItemPayload {
        std::string itemDropped;
        int count = 0;
        int flags = 0;
        std::string itemUniqueName;
    };

    struct SyncTakeItemPayload {
        std::string itemDropped;
        int count = 0;
        int flags = 0;
        std::string uniqueName;
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    struct PlayerStateUpdatePacket {
        UpdateType updateType = SYNC_POS;
        InitNpcPayload initNpc;
        SyncPosPayload pos;
        SyncHeadingPayload heading;
        SyncAnimationPayload animation;
        SyncWeaponModePayload weaponMode;
        SyncMagicSetupPayload magicSetup;
        SyncSpellCastPayload spellCasts;
        SyncArmorPayload armor;
        SyncWeaponsPayload weapons;
        SyncHpPayload hp;
        SyncBodyStatePayload bodyState;
        SyncOverlaysPayload overlays;
        SyncProtectionsPayload protections;
        SyncTalentsPayload talents;
        SyncHandPayload hand;
        SyncTimePayload time;
        SyncRevivedPayload revived;
        SyncAttacksPayload attacks;
        SyncDropItemPayload dropItem;
        SyncTakeItemPayload takeItem;
    };

    struct NetworkPacket {
        PacketType type = PacketType::PlayerStateUpdate;
        std::string senderId;
        JoinGamePacket joinGame;
        PlayerDisconnectPacket disconnect;
        PlayerStateUpdatePacket stateUpdate;
    };

    enum class PacketDecodeMode {
        Client,
        Server,
    };

    constexpr std::uint8_t kNetworkPacketVersion = 1;
    constexpr std::size_t kMaxPacketBytes = 16384;
    constexpr std::size_t kMaxNameLength = 64;
    constexpr std::size_t kMaxNicknameLength = 32;
    constexpr std::size_t kMaxInstanceNameLength = 64;
    constexpr std::size_t kMaxUniqueNameLength = 96;
    constexpr std::size_t kMaxOverlayCount = 96;
    constexpr std::size_t kMaxSpellCastCount = 32;
    constexpr std::size_t kMaxAttackCount = 32;

    static bool IsFinite(float value) {
        return std::isfinite(value);
    }

    static bool IsPrintableServerChar(unsigned char c) {
        return c >= 32 && c <= 126;
    }

    static bool SanitizeServerText(std::string& text, std::size_t maxLen) {
        std::string sanitized;
        sanitized.reserve(text.size());
        for (unsigned char c : text) {
            if (IsPrintableServerChar(c)) {
                sanitized.push_back(static_cast<char>(c));
            }
        }
        if (sanitized.size() > maxLen) {
            sanitized.resize(maxLen);
        }
        if (sanitized.empty() && !text.empty()) {
            return false;
        }
        text = sanitized;
        return true;
    }

    class PacketWriter {
    public:
        bool writeU8(std::uint8_t value) {
            buffer.push_back(value);
            return true;
        }

        bool writeU16(std::uint16_t value) {
            buffer.push_back(static_cast<std::uint8_t>(value & 0xFF));
            buffer.push_back(static_cast<std::uint8_t>((value >> 8) & 0xFF));
            return true;
        }

        bool writeU32(std::uint32_t value) {
            for (int i = 0; i < 4; ++i) {
                buffer.push_back(static_cast<std::uint8_t>((value >> (8 * i)) & 0xFF));
            }
            return true;
        }

        bool writeI32(std::int32_t value) {
            return writeU32(static_cast<std::uint32_t>(value));
        }

        bool writeBool(bool value) {
            return writeU8(value ? 1 : 0);
        }

        bool writeFloat(float value) {
            static_assert(sizeof(float) == sizeof(std::uint32_t), "Unexpected float size.");
            std::uint32_t raw;
            std::memcpy(&raw, &value, sizeof(raw));
            return writeU32(raw);
        }

        bool writeString(const std::string& value, std::size_t maxLen) {
            if (value.size() > maxLen) {
                return false;
            }
            if (!writeU16(static_cast<std::uint16_t>(value.size()))) {
                return false;
            }
            buffer.insert(buffer.end(), value.begin(), value.end());
            return true;
        }

        const std::vector<std::uint8_t>& data() const { return buffer; }

    private:
        std::vector<std::uint8_t> buffer;
    };

    class PacketReader {
    public:
        PacketReader(const std::uint8_t* data, std::size_t size)
            : data(data)
            , size(size)
            , offset(0) {}

        bool readU8(std::uint8_t& value) {
            if (offset + 1 > size) {
                return false;
            }
            value = data[offset];
            offset += 1;
            return true;
        }

        bool readU16(std::uint16_t& value) {
            if (offset + 2 > size) {
                return false;
            }
            value = static_cast<std::uint16_t>(data[offset])
                | (static_cast<std::uint16_t>(data[offset + 1]) << 8);
            offset += 2;
            return true;
        }

        bool readU32(std::uint32_t& value) {
            if (offset + 4 > size) {
                return false;
            }
            value = static_cast<std::uint32_t>(data[offset])
                | (static_cast<std::uint32_t>(data[offset + 1]) << 8)
                | (static_cast<std::uint32_t>(data[offset + 2]) << 16)
                | (static_cast<std::uint32_t>(data[offset + 3]) << 24);
            offset += 4;
            return true;
        }

        bool readI32(std::int32_t& value) {
            std::uint32_t raw = 0;
            if (!readU32(raw)) {
                return false;
            }
            value = static_cast<std::int32_t>(raw);
            return true;
        }

        bool readBool(bool& value) {
            std::uint8_t raw = 0;
            if (!readU8(raw)) {
                return false;
            }
            value = raw != 0;
            return true;
        }

        bool readFloat(float& value) {
            std::uint32_t raw = 0;
            if (!readU32(raw)) {
                return false;
            }
            std::memcpy(&value, &raw, sizeof(raw));
            return true;
        }

        bool readString(std::string& value, std::size_t maxLen) {
            std::uint16_t length = 0;
            if (!readU16(length)) {
                return false;
            }
            if (length > maxLen) {
                return false;
            }
            if (offset + length > size) {
                return false;
            }
            value.assign(reinterpret_cast<const char*>(data + offset), length);
            offset += length;
            return true;
        }

    private:
        const std::uint8_t* data;
        std::size_t size;
        std::size_t offset;
    };

    static bool ValidateRange(int value, int minValue, int maxValue) {
        return value >= minValue && value <= maxValue;
    }

    static bool ValidateRangeFloat(float value, float minValue, float maxValue) {
        return IsFinite(value) && value >= minValue && value <= maxValue;
    }

    static bool ReadSanitizedText(PacketReader& reader, std::string& out, std::size_t maxLen, bool sanitizeText) {
        if (!reader.readString(out, maxLen)) {
            return false;
        }
        if (sanitizeText) {
            if (!SanitizeServerText(out, maxLen)) {
                return false;
            }
        }
        return true;
    }

    bool SerializeNetworkPacket(const NetworkPacket& packet, std::vector<std::uint8_t>& out, std::string& error) {
        PacketWriter writer;
        writer.writeU8(kNetworkPacketVersion);
        writer.writeU8(static_cast<std::uint8_t>(packet.type));
        writer.writeBool(!packet.senderId.empty());
        if (!packet.senderId.empty()) {
            if (!writer.writeString(packet.senderId, kMaxNameLength)) {
                error = "Sender id too long.";
                return false;
            }
        }

        switch (packet.type) {
        case PacketType::JoinGame:
            writer.writeU32(packet.joinGame.connectId);
            if (!writer.writeString(packet.joinGame.name, kMaxNameLength)) {
                error = "JoinGame name too long.";
                return false;
            }
            break;
        case PacketType::PlayerDisconnect:
            if (!writer.writeString(packet.disconnect.name, kMaxNameLength)) {
                error = "Disconnect name too long.";
                return false;
            }
            writer.writeBool(packet.disconnect.hasNickname);
            if (packet.disconnect.hasNickname) {
                if (!writer.writeString(packet.disconnect.nickname, kMaxNicknameLength)) {
                    error = "Disconnect nickname too long.";
                    return false;
                }
            }
            break;
        case PacketType::PlayerStateUpdate:
            writer.writeU8(static_cast<std::uint8_t>(packet.stateUpdate.updateType));
            switch (packet.stateUpdate.updateType) {
            case INIT_NPC:
                writer.writeI32(packet.stateUpdate.initNpc.instanceId);
                if (!writer.writeString(packet.stateUpdate.initNpc.nickname, kMaxNicknameLength)) {
                    error = "Init nickname too long.";
                    return false;
                }
                writer.writeFloat(packet.stateUpdate.initNpc.x);
                writer.writeFloat(packet.stateUpdate.initNpc.y);
                writer.writeFloat(packet.stateUpdate.initNpc.z);
                writer.writeI32(packet.stateUpdate.initNpc.bodyTextVarNr);
                writer.writeI32(packet.stateUpdate.initNpc.headVarNr);
                break;
            case SYNC_POS:
                writer.writeFloat(packet.stateUpdate.pos.x);
                writer.writeFloat(packet.stateUpdate.pos.y);
                writer.writeFloat(packet.stateUpdate.pos.z);
                break;
            case SYNC_HEADING:
                writer.writeFloat(packet.stateUpdate.heading.heading);
                break;
            case SYNC_ANIMATION:
                writer.writeI32(packet.stateUpdate.animation.animationId);
                break;
            case SYNC_WEAPON_MODE:
                writer.writeI32(packet.stateUpdate.weaponMode.weaponMode);
                break;
            case SYNC_MAGIC_SETUP:
                if (!writer.writeString(packet.stateUpdate.magicSetup.spellInstanceName, kMaxInstanceNameLength)) {
                    error = "Magic setup spell too long.";
                    return false;
                }
                break;
            case SYNC_SPELL_CAST:
                if (packet.stateUpdate.spellCasts.casts.size() > kMaxSpellCastCount) {
                    error = "Spell cast count too large.";
                    return false;
                }
                writer.writeU8(static_cast<std::uint8_t>(packet.stateUpdate.spellCasts.casts.size()));
                for (const auto& cast : packet.stateUpdate.spellCasts.casts) {
                    if (!writer.writeString(cast.target, kMaxUniqueNameLength)) {
                        error = "Spell cast target too long.";
                        return false;
                    }
                    writer.writeI32(cast.spellInstanceId);
                    writer.writeI32(cast.spellLevel);
                    writer.writeI32(cast.spellCharge);
                }
                break;
            case SYNC_ARMOR:
                if (!writer.writeString(packet.stateUpdate.armor.armor, kMaxInstanceNameLength)) {
                    error = "Armor name too long.";
                    return false;
                }
                break;
            case SYNC_WEAPONS:
                if (!writer.writeString(packet.stateUpdate.weapons.weapon1, kMaxInstanceNameLength)
                    || !writer.writeString(packet.stateUpdate.weapons.weapon2, kMaxInstanceNameLength)) {
                    error = "Weapon name too long.";
                    return false;
                }
                break;
            case SYNC_HP:
                writer.writeI32(packet.stateUpdate.hp.hp);
                writer.writeI32(packet.stateUpdate.hp.hpMax);
                break;
            case SYNC_BODYSTATE:
                writer.writeI32(packet.stateUpdate.bodyState.bodyState);
                break;
            case SYNC_OVERLAYS:
                if (packet.stateUpdate.overlays.overlayIds.size() > kMaxOverlayCount) {
                    error = "Overlay count too large.";
                    return false;
                }
                writer.writeU8(static_cast<std::uint8_t>(packet.stateUpdate.overlays.overlayIds.size()));
                for (int overlayId : packet.stateUpdate.overlays.overlayIds) {
                    writer.writeI32(overlayId);
                }
                break;
            case SYNC_PROTECTIONS:
                for (int i = 0; i < 8; ++i) {
                    writer.writeI32(packet.stateUpdate.protections.protections[i]);
                }
                break;
            case SYNC_TALENTS:
                for (int i = 0; i < 4; ++i) {
                    writer.writeI32(packet.stateUpdate.talents.talents[i]);
                }
                break;
            case SYNC_HAND:
                if (!writer.writeString(packet.stateUpdate.hand.leftItem, kMaxInstanceNameLength)
                    || !writer.writeString(packet.stateUpdate.hand.rightItem, kMaxInstanceNameLength)) {
                    error = "Hand item name too long.";
                    return false;
                }
                break;
            case SYNC_TIME:
                writer.writeI32(packet.stateUpdate.time.hour);
                writer.writeI32(packet.stateUpdate.time.minute);
                break;
            case SYNC_REVIVED:
                if (!writer.writeString(packet.stateUpdate.revived.name, kMaxNameLength)) {
                    error = "Revived name too long.";
                    return false;
                }
                break;
            case SYNC_ATTACKS:
                if (packet.stateUpdate.attacks.attacks.size() > kMaxAttackCount) {
                    error = "Attack count too large.";
                    return false;
                }
                writer.writeU8(static_cast<std::uint8_t>(packet.stateUpdate.attacks.attacks.size()));
                for (const auto& attack : packet.stateUpdate.attacks.attacks) {
                    if (!writer.writeString(attack.target, kMaxUniqueNameLength)) {
                        error = "Attack target too long.";
                        return false;
                    }
                    writer.writeFloat(attack.damage);
                    writer.writeI32(attack.isUnconscious);
                    writer.writeBool(attack.isDead);
                    writer.writeBool(attack.isFinish);
                    writer.writeU32(static_cast<std::uint32_t>(attack.damageMode));
                }
                break;
            case SYNC_DROPITEM:
                if (!writer.writeString(packet.stateUpdate.dropItem.itemDropped, kMaxInstanceNameLength)
                    || !writer.writeString(packet.stateUpdate.dropItem.itemUniqueName, kMaxUniqueNameLength)) {
                    error = "Drop item name too long.";
                    return false;
                }
                writer.writeI32(packet.stateUpdate.dropItem.count);
                writer.writeI32(packet.stateUpdate.dropItem.flags);
                break;
            case SYNC_TAKEITEM:
                if (!writer.writeString(packet.stateUpdate.takeItem.itemDropped, kMaxInstanceNameLength)
                    || !writer.writeString(packet.stateUpdate.takeItem.uniqueName, kMaxUniqueNameLength)) {
                    error = "Take item name too long.";
                    return false;
                }
                writer.writeI32(packet.stateUpdate.takeItem.count);
                writer.writeI32(packet.stateUpdate.takeItem.flags);
                writer.writeFloat(packet.stateUpdate.takeItem.x);
                writer.writeFloat(packet.stateUpdate.takeItem.y);
                writer.writeFloat(packet.stateUpdate.takeItem.z);
                break;
            case DESTROY_NPC:
                break;
            case SYNC_PLAYER_NAME:
            case PLAYER_DISCONNECT:
                error = "Invalid update type in state update.";
                return false;
            }
            break;
        default:
            error = "Unknown packet type.";
            return false;
        }

        out = writer.data();
        if (out.size() > kMaxPacketBytes) {
            error = "Packet exceeds size limit.";
            return false;
        }
        return true;
    }

    bool DeserializeNetworkPacket(const std::uint8_t* data, std::size_t size, NetworkPacket& out, std::string& error, PacketDecodeMode mode) {
        if (!data || size == 0) {
            error = "Empty packet.";
            return false;
        }
        if (size > kMaxPacketBytes) {
            error = "Packet exceeds size limit.";
            return false;
        }

        PacketReader reader(data, size);
        std::uint8_t version = 0;
        if (!reader.readU8(version) || version != kNetworkPacketVersion) {
            error = "Unsupported packet version.";
            return false;
        }

        std::uint8_t typeRaw = 0;
        if (!reader.readU8(typeRaw)) {
            error = "Missing packet type.";
            return false;
        }
        out.type = static_cast<PacketType>(typeRaw);

        bool hasSender = false;
        if (!reader.readBool(hasSender)) {
            error = "Missing sender flag.";
            return false;
        }
        out.senderId.clear();
        if (hasSender) {
            if (!ReadSanitizedText(reader, out.senderId, kMaxNameLength, mode == PacketDecodeMode::Server)) {
                error = "Invalid sender id.";
                return false;
            }
            if (mode == PacketDecodeMode::Server) {
                error = "Sender id not allowed from client.";
                return false;
            }
        }

        switch (out.type) {
        case PacketType::JoinGame:
        {
            std::uint32_t connectId = 0;
            if (!reader.readU32(connectId)) {
                error = "Invalid join packet.";
                return false;
            }
            out.joinGame.connectId = connectId;
            if (!ReadSanitizedText(reader, out.joinGame.name, kMaxNameLength, mode == PacketDecodeMode::Server)) {
                error = "Invalid join name.";
                return false;
            }
            break;
        }
        case PacketType::PlayerDisconnect:
        {
            if (!ReadSanitizedText(reader, out.disconnect.name, kMaxNameLength, mode == PacketDecodeMode::Server)) {
                error = "Invalid disconnect name.";
                return false;
            }
            bool hasNickname = false;
            if (!reader.readBool(hasNickname)) {
                error = "Invalid disconnect nickname flag.";
                return false;
            }
            out.disconnect.hasNickname = hasNickname;
            out.disconnect.nickname.clear();
            if (hasNickname) {
                if (!ReadSanitizedText(reader, out.disconnect.nickname, kMaxNicknameLength, mode == PacketDecodeMode::Server)) {
                    error = "Invalid disconnect nickname.";
                    return false;
                }
            }
            break;
        }
        case PacketType::PlayerStateUpdate:
        {
            std::uint8_t updateRaw = 0;
            if (!reader.readU8(updateRaw)) {
                error = "Missing update type.";
                return false;
            }
            out.stateUpdate.updateType = static_cast<UpdateType>(updateRaw);
            switch (out.stateUpdate.updateType) {
            case INIT_NPC:
            {
                int instanceId = 0;
                if (!reader.readI32(instanceId)) {
                    error = "Invalid init packet.";
                    return false;
                }
                out.stateUpdate.initNpc.instanceId = instanceId;
                if (!ReadSanitizedText(reader, out.stateUpdate.initNpc.nickname, kMaxNicknameLength, mode == PacketDecodeMode::Server)) {
                    error = "Invalid nickname.";
                    return false;
                }
                if (!reader.readFloat(out.stateUpdate.initNpc.x)
                    || !reader.readFloat(out.stateUpdate.initNpc.y)
                    || !reader.readFloat(out.stateUpdate.initNpc.z)) {
                    error = "Invalid init position.";
                    return false;
                }
                if (!reader.readI32(out.stateUpdate.initNpc.bodyTextVarNr)
                    || !reader.readI32(out.stateUpdate.initNpc.headVarNr)) {
                    error = "Invalid init appearance.";
                    return false;
                }
                if (!ValidateRange(instanceId, 1, 100000)
                    || !ValidateRange(out.stateUpdate.initNpc.bodyTextVarNr, 0, 200)
                    || !ValidateRange(out.stateUpdate.initNpc.headVarNr, 0, 200)
                    || !ValidateRangeFloat(out.stateUpdate.initNpc.x, -100000.0f, 100000.0f)
                    || !ValidateRangeFloat(out.stateUpdate.initNpc.y, -100000.0f, 100000.0f)
                    || !ValidateRangeFloat(out.stateUpdate.initNpc.z, -100000.0f, 100000.0f)) {
                    error = "Init packet out of range.";
                    return false;
                }
                break;
            }
            case SYNC_POS:
                if (!reader.readFloat(out.stateUpdate.pos.x)
                    || !reader.readFloat(out.stateUpdate.pos.y)
                    || !reader.readFloat(out.stateUpdate.pos.z)) {
                    error = "Invalid position packet.";
                    return false;
                }
                if (!ValidateRangeFloat(out.stateUpdate.pos.x, -100000.0f, 100000.0f)
                    || !ValidateRangeFloat(out.stateUpdate.pos.y, -100000.0f, 100000.0f)
                    || !ValidateRangeFloat(out.stateUpdate.pos.z, -100000.0f, 100000.0f)) {
                    error = "Position out of range.";
                    return false;
                }
                break;
            case SYNC_HEADING:
                if (!reader.readFloat(out.stateUpdate.heading.heading)) {
                    error = "Invalid heading packet.";
                    return false;
                }
                if (!ValidateRangeFloat(out.stateUpdate.heading.heading, -360.0f, 360.0f)) {
                    error = "Heading out of range.";
                    return false;
                }
                break;
            case SYNC_ANIMATION:
                if (!reader.readI32(out.stateUpdate.animation.animationId)) {
                    error = "Invalid animation packet.";
                    return false;
                }
                if (!ValidateRange(out.stateUpdate.animation.animationId, 0, 100000)) {
                    error = "Animation id out of range.";
                    return false;
                }
                break;
            case SYNC_WEAPON_MODE:
                if (!reader.readI32(out.stateUpdate.weaponMode.weaponMode)) {
                    error = "Invalid weapon mode packet.";
                    return false;
                }
                if (!ValidateRange(out.stateUpdate.weaponMode.weaponMode, 0, 100)) {
                    error = "Weapon mode out of range.";
                    return false;
                }
                break;
            case SYNC_MAGIC_SETUP:
                if (!ReadSanitizedText(reader, out.stateUpdate.magicSetup.spellInstanceName, kMaxInstanceNameLength, mode == PacketDecodeMode::Server)) {
                    error = "Invalid magic setup packet.";
                    return false;
                }
                break;
            case SYNC_SPELL_CAST:
            {
                std::uint8_t count = 0;
                if (!reader.readU8(count)) {
                    error = "Invalid spell cast count.";
                    return false;
                }
                if (count > kMaxSpellCastCount) {
                    error = "Spell cast count too large.";
                    return false;
                }
                out.stateUpdate.spellCasts.casts.clear();
                out.stateUpdate.spellCasts.casts.reserve(count);
                for (std::uint8_t i = 0; i < count; ++i) {
                    SpellCastInfo cast;
                    if (!ReadSanitizedText(reader, cast.target, kMaxUniqueNameLength, mode == PacketDecodeMode::Server)) {
                        error = "Invalid spell cast target.";
                        return false;
                    }
                    if (!reader.readI32(cast.spellInstanceId)
                        || !reader.readI32(cast.spellLevel)
                        || !reader.readI32(cast.spellCharge)) {
                        error = "Invalid spell cast packet.";
                        return false;
                    }
                    if (!ValidateRange(cast.spellInstanceId, 0, 100000)
                        || !ValidateRange(cast.spellLevel, 0, 100)
                        || !ValidateRange(cast.spellCharge, 0, 10000)) {
                        error = "Spell cast out of range.";
                        return false;
                    }
                    out.stateUpdate.spellCasts.casts.push_back(cast);
                }
                break;
            }
            case SYNC_ARMOR:
                if (!ReadSanitizedText(reader, out.stateUpdate.armor.armor, kMaxInstanceNameLength, mode == PacketDecodeMode::Server)) {
                    error = "Invalid armor packet.";
                    return false;
                }
                break;
            case SYNC_WEAPONS:
                if (!ReadSanitizedText(reader, out.stateUpdate.weapons.weapon1, kMaxInstanceNameLength, mode == PacketDecodeMode::Server)
                    || !ReadSanitizedText(reader, out.stateUpdate.weapons.weapon2, kMaxInstanceNameLength, mode == PacketDecodeMode::Server)) {
                    error = "Invalid weapon packet.";
                    return false;
                }
                break;
            case SYNC_HP:
                if (!reader.readI32(out.stateUpdate.hp.hp)
                    || !reader.readI32(out.stateUpdate.hp.hpMax)) {
                    error = "Invalid hp packet.";
                    return false;
                }
                if (!ValidateRange(out.stateUpdate.hp.hp, 0, 100000)
                    || !ValidateRange(out.stateUpdate.hp.hpMax, 0, 100000)) {
                    error = "HP out of range.";
                    return false;
                }
                break;
            case SYNC_BODYSTATE:
                if (!reader.readI32(out.stateUpdate.bodyState.bodyState)) {
                    error = "Invalid body state packet.";
                    return false;
                }
                if (!ValidateRange(out.stateUpdate.bodyState.bodyState, 0, 1000)) {
                    error = "Body state out of range.";
                    return false;
                }
                break;
            case SYNC_OVERLAYS:
            {
                std::uint8_t count = 0;
                if (!reader.readU8(count)) {
                    error = "Invalid overlay count.";
                    return false;
                }
                if (count > kMaxOverlayCount) {
                    error = "Overlay count too large.";
                    return false;
                }
                out.stateUpdate.overlays.overlayIds.clear();
                out.stateUpdate.overlays.overlayIds.reserve(count);
                for (std::uint8_t i = 0; i < count; ++i) {
                    int overlayId = 0;
                    if (!reader.readI32(overlayId)) {
                        error = "Invalid overlay packet.";
                        return false;
                    }
                    if (!ValidateRange(overlayId, 0, 200000)) {
                        error = "Overlay id out of range.";
                        return false;
                    }
                    out.stateUpdate.overlays.overlayIds.push_back(overlayId);
                }
                break;
            }
            case SYNC_PROTECTIONS:
                for (int i = 0; i < 8; ++i) {
                    if (!reader.readI32(out.stateUpdate.protections.protections[i])) {
                        error = "Invalid protections packet.";
                        return false;
                    }
                    if (!ValidateRange(out.stateUpdate.protections.protections[i], 0, 10000)) {
                        error = "Protection value out of range.";
                        return false;
                    }
                }
                break;
            case SYNC_TALENTS:
                for (int i = 0; i < 4; ++i) {
                    if (!reader.readI32(out.stateUpdate.talents.talents[i])) {
                        error = "Invalid talents packet.";
                        return false;
                    }
                    if (!ValidateRange(out.stateUpdate.talents.talents[i], 0, 1000)) {
                        error = "Talent value out of range.";
                        return false;
                    }
                }
                break;
            case SYNC_HAND:
                if (!ReadSanitizedText(reader, out.stateUpdate.hand.leftItem, kMaxInstanceNameLength, mode == PacketDecodeMode::Server)
                    || !ReadSanitizedText(reader, out.stateUpdate.hand.rightItem, kMaxInstanceNameLength, mode == PacketDecodeMode::Server)) {
                    error = "Invalid hand packet.";
                    return false;
                }
                break;
            case SYNC_TIME:
                if (!reader.readI32(out.stateUpdate.time.hour)
                    || !reader.readI32(out.stateUpdate.time.minute)) {
                    error = "Invalid time packet.";
                    return false;
                }
                if (!ValidateRange(out.stateUpdate.time.hour, 0, 23)
                    || !ValidateRange(out.stateUpdate.time.minute, 0, 59)) {
                    error = "Time out of range.";
                    return false;
                }
                break;
            case SYNC_REVIVED:
                if (!ReadSanitizedText(reader, out.stateUpdate.revived.name, kMaxNameLength, mode == PacketDecodeMode::Server)) {
                    error = "Invalid revived packet.";
                    return false;
                }
                break;
            case SYNC_ATTACKS:
            {
                std::uint8_t count = 0;
                if (!reader.readU8(count)) {
                    error = "Invalid attack count.";
                    return false;
                }
                if (count > kMaxAttackCount) {
                    error = "Attack count too large.";
                    return false;
                }
                out.stateUpdate.attacks.attacks.clear();
                out.stateUpdate.attacks.attacks.reserve(count);
                for (std::uint8_t i = 0; i < count; ++i) {
                    AttackInfo attack;
                    if (!ReadSanitizedText(reader, attack.target, kMaxUniqueNameLength, mode == PacketDecodeMode::Server)) {
                        error = "Invalid attack target.";
                        return false;
                    }
                    if (!reader.readFloat(attack.damage)
                        || !reader.readI32(attack.isUnconscious)
                        || !reader.readBool(attack.isDead)
                        || !reader.readBool(attack.isFinish)) {
                        error = "Invalid attack packet.";
                        return false;
                    }
                    std::uint32_t damageMode = 0;
                    if (!reader.readU32(damageMode)) {
                        error = "Invalid damage mode.";
                        return false;
                    }
                    attack.damageMode = damageMode;
                    if (!ValidateRangeFloat(attack.damage, 0.0f, 100000.0f)
                        || !ValidateRange(attack.isUnconscious, 0, 1)) {
                        error = "Attack value out of range.";
                        return false;
                    }
                    out.stateUpdate.attacks.attacks.push_back(attack);
                }
                break;
            }
            case SYNC_DROPITEM:
                if (!ReadSanitizedText(reader, out.stateUpdate.dropItem.itemDropped, kMaxInstanceNameLength, mode == PacketDecodeMode::Server)
                    || !ReadSanitizedText(reader, out.stateUpdate.dropItem.itemUniqueName, kMaxUniqueNameLength, mode == PacketDecodeMode::Server)) {
                    error = "Invalid drop item packet.";
                    return false;
                }
                if (!reader.readI32(out.stateUpdate.dropItem.count)
                    || !reader.readI32(out.stateUpdate.dropItem.flags)) {
                    error = "Invalid drop item values.";
                    return false;
                }
                if (!ValidateRange(out.stateUpdate.dropItem.count, 0, 10000)
                    || !ValidateRange(out.stateUpdate.dropItem.flags, 0, 1000000)) {
                    error = "Drop item values out of range.";
                    return false;
                }
                break;
            case SYNC_TAKEITEM:
                if (!ReadSanitizedText(reader, out.stateUpdate.takeItem.itemDropped, kMaxInstanceNameLength, mode == PacketDecodeMode::Server)
                    || !ReadSanitizedText(reader, out.stateUpdate.takeItem.uniqueName, kMaxUniqueNameLength, mode == PacketDecodeMode::Server)) {
                    error = "Invalid take item packet.";
                    return false;
                }
                if (!reader.readI32(out.stateUpdate.takeItem.count)
                    || !reader.readI32(out.stateUpdate.takeItem.flags)
                    || !reader.readFloat(out.stateUpdate.takeItem.x)
                    || !reader.readFloat(out.stateUpdate.takeItem.y)
                    || !reader.readFloat(out.stateUpdate.takeItem.z)) {
                    error = "Invalid take item values.";
                    return false;
                }
                if (!ValidateRange(out.stateUpdate.takeItem.count, 0, 10000)
                    || !ValidateRange(out.stateUpdate.takeItem.flags, 0, 1000000)
                    || !ValidateRangeFloat(out.stateUpdate.takeItem.x, -100000.0f, 100000.0f)
                    || !ValidateRangeFloat(out.stateUpdate.takeItem.y, -100000.0f, 100000.0f)
                    || !ValidateRangeFloat(out.stateUpdate.takeItem.z, -100000.0f, 100000.0f)) {
                    error = "Take item values out of range.";
                    return false;
                }
                break;
            case DESTROY_NPC:
                break;
            default:
                error = "Unknown update type.";
                return false;
            }
            break;
        }
        default:
            error = "Unknown packet type.";
            return false;
        }

        return true;
    }

    std::string DescribePacket(const NetworkPacket& packet) {
        std::ostringstream stream;
        stream << "Packet(type=" << static_cast<int>(packet.type);
        if (!packet.senderId.empty()) {
            stream << ", sender=" << packet.senderId;
        }
        if (packet.type == PacketType::PlayerStateUpdate) {
            stream << ", update=" << static_cast<int>(packet.stateUpdate.updateType);
        }
        stream << ")";
        return stream.str();
    }
}