#include "resource.h"

namespace GOTHIC_ENGINE {
    TSaveLoadGameInfo& SaveLoadGameInfo = UnionCore::SaveLoadGameInfo;

    void _Game_Entry() {
        Game_Entry();
    }

    void _Game_Init() {
        Game_Init();
    }

    void _Game_Loop() {
        try {
            Game_Loop();
        }
        catch (...) {
            SaveErrorDetails();
        }
    }

    void _Game_Exit() {
    }

    void _Game_PreLoop() {
    }

    void _Game_PostLoop() {
    }

    void _Game_MenuLoop() {
    }

    void _Game_SaveBegin() {
        try {
            Game_SaveBegin();
        }
        catch (...) {
            SaveErrorDetails();
        }
    }

    void _Game_SaveEnd() {
        try {
            Game_SaveEnd();
        }
        catch (...) {
            SaveErrorDetails();
        }
    }

    void _LoadBegin() {
        try {
            LoadBegin();
        }
        catch (...) {
            SaveErrorDetails();
        }
    }

    void _LoadEnd() {
        try {
            LoadEnd();
        }
        catch (...) {
            SaveErrorDetails();
        }
    }

    void _Game_LoadBegin_NewGame() {
        _LoadBegin();
    }

    void _Game_LoadEnd_NewGame() {
        _LoadEnd();
    }

    void _Game_LoadBegin_SaveGame() {
        _LoadBegin();
    }

    void _Game_LoadEnd_SaveGame() {
        try {
            Game_LoadEnd_SaveGame();
        }
        catch (...) {
            SaveErrorDetails();
        }
    }

    void _Game_LoadBegin_ChangeLevel() {
        _LoadBegin();
    }

    void _Game_LoadEnd_ChangeLevel() {
        _LoadEnd();
    }

    void _Game_LoadBegin_Trigger() {
        try {
            Game_LoadBegin_Trigger();
        }
        catch (...) {
            SaveErrorDetails();
        }
    }

    void _Game_LoadEnd_Trigger() {
    }

    void _Game_Pause() {
    }

    void _Game_Unpause() {
    }

    void _Game_DefineExternals() {
    }

    void _Game_ApplyOptions() {
    }

    /*
    Functions call order on Game initialization:
      - Game_Entry           * Gothic entry point
      - Game_DefineExternals * Define external script functions
      - Game_Init            * After DAT files init

    Functions call order on Change level:
      - Game_LoadBegin_Trigger     * Entry in trigger
      - Game_LoadEnd_Trigger       *
      - Game_Loop                  * Frame call window
      - Game_LoadBegin_ChangeLevel * Load begin
      - Game_SaveBegin             * Save previous level information
      - Game_SaveEnd               *
      - Game_LoadEnd_ChangeLevel   *

    Functions call order on Save game:
      - Game_Pause     * Open menu
      - Game_Unpause   * Click on save
      - Game_Loop      * Frame call window
      - Game_SaveBegin * Save begin
      - Game_SaveEnd   *

    Functions call order on Load game:
      - Game_Pause              * Open menu
      - Game_Unpause            * Click on load
      - Game_LoadBegin_SaveGame * Load begin
      - Game_LoadEnd_SaveGame   *
    */


#define AppDefault True
    CApplication* lpApplication = !CHECK_THIS_ENGINE ? Null : CApplication::CreateRefApplication(
        Enabled(AppDefault) _Game_Entry,
        Enabled(AppDefault) _Game_Init,
        Enabled(AppDefault) _Game_Exit,
        Enabled(AppDefault) _Game_PreLoop,
        Enabled(AppDefault) _Game_Loop,
        Enabled(AppDefault) _Game_PostLoop,
        Enabled(AppDefault) _Game_MenuLoop,
        Enabled(AppDefault) _Game_SaveBegin,
        Enabled(AppDefault) _Game_SaveEnd,
        Enabled(AppDefault) _Game_LoadBegin_NewGame,
        Enabled(AppDefault) _Game_LoadEnd_NewGame,
        Enabled(AppDefault) _Game_LoadBegin_SaveGame,
        Enabled(AppDefault) _Game_LoadEnd_SaveGame,
        Enabled(AppDefault) _Game_LoadBegin_ChangeLevel,
        Enabled(AppDefault) _Game_LoadEnd_ChangeLevel,
        Enabled(AppDefault) _Game_LoadBegin_Trigger,
        Enabled(AppDefault) _Game_LoadEnd_Trigger,
        Enabled(AppDefault) _Game_Pause,
        Enabled(AppDefault) _Game_Unpause,
        Enabled(AppDefault) _Game_DefineExternals,
        Enabled(AppDefault) _Game_ApplyOptions
    );
}