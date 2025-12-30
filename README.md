# GothicCoop ⚔️🤝
GothicCoop is a cooperative multiplayer mod for **Gothic** that lets you experience the game together. It synchronizes players, NPCs, combat, and world state so a party can adventure through the same campaign. **Fair warning:** the mod is poorly written and may not behave like a regular player would expect, so expect rough edges and jank. 🧪🧯

## Features ✨
- Co-op multiplayer gameplay for Gothic
- Player and NPC synchronization
- Shared combat and damage events
- Networked spell casting and animations
- Configurable connection settings

## Download & Install 📦
#### Requirements
1. Union
2. zFriendlyFire plugin

### From [GitHub Releases](https://github.com/DamianQualshy/GothicCoop/releases)
1. **Download the file from the latest release:**
-  **Gothic I:** `GothicCoop_G1.dll`
-  **Gothic Sequel:** `GothicCoop_G1A.dll`
-  **Gothic II:** `GothicCoop_G2.dll`
-  **Gothic II Night of the Raven:** `GothicCoop_G2A.dll`
2. **Place it in this location:** `(game path)\Autorun`.
> example of the game path: `D:\Steam\steamapps\common\Gothic`
3. **You don't exactly *need* to download the config file, because the mod would create a default one on first startup. But it's recommended anyway.**

### From Steam Workshop 
- **Gothic I:** https://steamcommunity.com/sharedfiles/filedetails/?id=2982357924&tscn=1767130891
- **Gothic II Night of the Raven:** https://steamcommunity.com/sharedfiles/filedetails/?id=2982365243

## Configuration ⚙️
GothicCoop uses a TOML configuration file.
- **File:** `GothicCoopConfig.toml`
- **Location:** `(game path)\System`.
> example of the game path: `D:\Steam\steamapps\common\Gothic`

### Example 🧾
```toml
[connection]
server = "localhost"
port = 1234

[player]
nickname = "player"
friendInstance = "CH"

[appearance]
bodyModel = "HUM_BODY_NAKED0"
bodyTex = 9
headModel = "HUM_HEAD_PONY"
headTex = 18
#skinColor = 1

[gameplay]
playersDamageMultiplier = 100
npcsDamageMultiplier = 100

[controls]
toggleGameLogKey = "KEY_P"
toggleGameStatsKey = "KEY_O"
startServerKey = "KEY_F1"
startConnectionKey = "KEY_F2"
reinitPlayersKey = "KEY_F3"
revivePlayerKey = "KEY_F4"
```

### Settings Reference 📚
#### `[connection]` 🔌
- (string) `server`: Server address to connect to. Use `localhost`/`127.0.0.1` for local testing.
- (int) `port`: Port number for multiplayer connection. Default `1234`, valid range `1024-65535`.

#### `[player]` 🧑‍🤝‍🧑
- (string) `nickname`: In-game nickname shown to other players. Keep it short (around 20 characters).
- (string) `friendInstance`: NPC instance used for the multiplayer friend character. Must match an instance defined in your Gothic scripts.
> By default set to CH, please don't change it thoughtlessly like "it will be funny to be Xardas". Some mods may have the Character Helper (ch) removed, so ONLY then should you change this option. An example is Gothic NEW BALANCE, for Gothic Returning/NB, we set this option to PC_HEROMUL.

#### `[appearance]` 🎭
- (string) `bodyModel`: Body model name without extension. 
> Vanilla options: `HUM_BODY_NAKED0` (male), `HUM_BODY_BABE0` (female).
- (int) `bodyTex`: Body texture index. Defaults: Gothic II `9`, Gothic I `4`.
- (string) `headModel`: Head model name without extension. 
> Vanilla options: `HUM_HEAD_PONY`, `HUM_HEAD_BALD`, `HUM_HEAD_FATBALD`, `HUM_HEAD_FIGHTER`, `HUM_HEAD_THIEF`, `HUM_HEAD_PSIONIC`.
- (int) `headTex`: Head texture index. Defaults: Gothic II `18`, Gothic I `9`.
- (int, Gothic 1 only) `skinColor`: Skin color index, valid range `0-3`. 
> Uncomment only if you are playing Gothic 1.

#### `[gameplay]` 🥊
- (int) `playersDamageMultiplier`: Player damage multiplier in percent. `100` = normal, `50` = half, `200` = double.
> MUST be the same for all players! This is the percentage of damage dealt by the player, 100% by default. You can change anything from 1 to 100.
- (int) `npcsDamageMultiplier`: NPC damage multiplier in percent. `100` = normal, `50` = half, `200` = double.
> MUST be the same for all players! This is the percentage of damage dealt by all NPCs to players, 100% by default. You can change anything from 100 to 500.

#### `[controls]` 🎮
- (string) `toggleGameLogKey`: Toggle chat/game log overlay.
- (string) `toggleGameStatsKey`: Toggle network stats overlay.
- (string) `startServerKey`: Start/stop hosting.
- (string) `startConnectionKey`: Connect to server / pause synchronization.
- (string) `reinitPlayersKey`: Reinitialize multiplayer NPCs.
- (string) `revivePlayerKey`: Revive a downed teammate.

**Available key codes** ⌨️
- Letters: `KEY_A` through `KEY_Z`
- Numbers: `KEY_0` through `KEY_9`
- Function keys: `KEY_F1` through `KEY_F12`
- Special keys: `KEY_ESCAPE`, `KEY_SPACE`, `KEY_RETURN`, `KEY_BACK`, `KEY_TAB`
- Arrows: `KEY_UP`, `KEY_DOWN`, `KEY_LEFT`, `KEY_RIGHT`
- Modifiers: `KEY_LSHIFT`, `KEY_RSHIFT`, `KEY_LCONTROL`, `KEY_RCONTROL`, `KEY_LMENU`, `KEY_RMENU`
- Navigation: `KEY_INSERT`, `KEY_DELETE`, `KEY_HOME`, `KEY_END`, `KEY_PRIOR`, `KEY_NEXT`
- Numpad: `KEY_NUMPAD0` through `KEY_NUMPAD9`, `KEY_MULTIPLY`, `KEY_ADD`, `KEY_SUBTRACT`, `KEY_DIVIDE`

### Notes 📝
- Restart the game after editing the config file.
- If the mod fails to load, verify the config file path and syntax.

## Support 🛠️
If something breaks, open an issue with your game version, mod loader, and logs if available.