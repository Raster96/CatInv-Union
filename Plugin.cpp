#include "UnionAfx.h"
#include "resource.h"

namespace GOTHIC_ENGINE {

    void Game_Entry() {
    }
    
    void Game_Init() {
        if (zoptions) {
            CatInvOptions::ReadOptions();
        }
        CatInvCore::activeCategory = CatInvOptions::G1Mode ? 1 : 0;
        CatInvCore::categoryView = nullptr;
        CatInvCore::backupListBySide[0] = nullptr;
        CatInvCore::backupListBySide[1] = nullptr;
        CatInvCore::containerBySide[0] = nullptr;
        CatInvCore::containerBySide[1] = nullptr;
        CatInvCore::filteredListBySide[0] = nullptr;
        CatInvCore::filteredListBySide[1] = nullptr;
        CatInvCore::hooksActive = false;
    }

    void Game_Exit() {
        // Cleanup disabled for now
    }

    void Game_PreLoop() {
    }

    void Game_Loop() {
        if (!player || !screen || !zinput || !ogame) return;
        if (ogame->inScriptStartup || ogame->inLoadSaveGame || ogame->inLevelChange) return;

        try {
            if (player->inventory2.IsOpen()) {
                bool shiftPressed = zinput->KeyPressed(KEY_LSHIFT) || zinput->KeyPressed(KEY_RSHIFT);
                
                if (shiftPressed) {
                    if (zinput->KeyToggled(KEY_LEFT)) {
                        CatInvCore::ShiftCategory(-1);
                    }
                    else if (zinput->KeyToggled(KEY_RIGHT)) {
                        CatInvCore::ShiftCategory(1);
                    }
                    else if (zinput->KeyToggled(KEY_HOME)) {
                        CatInvCore::SetCategoryFirst();
                    }
                    else if (zinput->KeyToggled(KEY_END)) {
                        CatInvCore::SetCategoryLast();
                    }
                }
            }
        }
        catch (...) { }
    }

    void Game_PostLoop() {
    }

    void Game_MenuLoop() {
    }

    TSaveLoadGameInfo& SaveLoadGameInfo = UnionCore::SaveLoadGameInfo;

    void Game_SaveBegin() {
    }

    void Game_SaveEnd() {
    }

    void LoadBegin() {
        CatInvCore::backupListBySide[0] = nullptr;
        CatInvCore::backupListBySide[1] = nullptr;
        CatInvCore::containerBySide[0] = nullptr;
        CatInvCore::containerBySide[1] = nullptr;
        CatInvCore::filteredListBySide[0] = nullptr;
        CatInvCore::filteredListBySide[1] = nullptr;
        CatInvCore::categoryView = nullptr;
        CatInvCore::hooksActive = false;
    }

    void LoadEnd() {
    }

    void Game_LoadBegin_NewGame() {
        LoadBegin();
    }

    void Game_LoadEnd_NewGame() {
        LoadEnd();
    }

    void Game_LoadBegin_SaveGame() {
        LoadBegin();
    }

    void Game_LoadEnd_SaveGame() {
        LoadEnd();
    }

    void Game_LoadBegin_ChangeLevel() {
        LoadBegin();
    }

    void Game_LoadEnd_ChangeLevel() {
        LoadEnd();
    }

    void Game_LoadBegin_Trigger() {
    }
    
    void Game_LoadEnd_Trigger() {
    }
    
    void Game_Pause() {
    }
    
    void Game_Unpause() {
    }
    
    void Game_DefineExternals() {
    }

    void Game_ApplyOptions() {
        CatInvOptions::ReadOptions();
    }

#define AppDefault True
    CApplication* lpApplication = !CHECK_THIS_ENGINE ? Null : CApplication::CreateRefApplication(
        Enabled( AppDefault ) Game_Entry,
        Enabled( AppDefault ) Game_Init,
        Enabled( AppDefault ) Game_Exit,
        Enabled( AppDefault ) Game_PreLoop,
        Enabled( AppDefault ) Game_Loop,
        Enabled( AppDefault ) Game_PostLoop,
        Enabled( AppDefault ) Game_MenuLoop,
        Enabled( AppDefault ) Game_SaveBegin,
        Enabled( AppDefault ) Game_SaveEnd,
        Enabled( AppDefault ) Game_LoadBegin_NewGame,
        Enabled( AppDefault ) Game_LoadEnd_NewGame,
        Enabled( AppDefault ) Game_LoadBegin_SaveGame,
        Enabled( AppDefault ) Game_LoadEnd_SaveGame,
        Enabled( AppDefault ) Game_LoadBegin_ChangeLevel,
        Enabled( AppDefault ) Game_LoadEnd_ChangeLevel,
        Enabled( AppDefault ) Game_LoadBegin_Trigger,
        Enabled( AppDefault ) Game_LoadEnd_Trigger,
        Enabled( AppDefault ) Game_Pause,
        Enabled( AppDefault ) Game_Unpause,
        Enabled( AppDefault ) Game_DefineExternals,
        Enabled( AppDefault ) Game_ApplyOptions
    );
}
