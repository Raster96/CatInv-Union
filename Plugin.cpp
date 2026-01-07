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
        CatInvCore::searchActive = false;
        CatInvCore::searchInputActive = false;
        CatInvCore::searchText = L"";
        CatInvCore::searchView = nullptr;
    }

    void Game_Exit() {
    }

    void Game_PreLoop() {
        if (!player || !screen || !zinput || !ogame) return;
        if (ogame->inScriptStartup || ogame->inLoadSaveGame || ogame->inLevelChange) return;

        try {
            if (player->inventory2.IsOpen()) {
                bool shiftPressed = zinput->KeyPressed(KEY_LSHIFT) || zinput->KeyPressed(KEY_RSHIFT);
                
                if (shiftPressed && zinput->KeyToggled(KEY_F)) {
                    if (!CatInvCore::searchActive) {
                        CatInvCore::ActivateSearch();
                    } else if (!CatInvCore::searchInputActive) {
                        CatInvCore::searchInputActive = true;
                    }
                }
                
                if (CatInvCore::searchActive && CatInvCore::searchInputActive) {
                    if (zinput->KeyToggled(KEY_ESCAPE)) {
                        CatInvCore::DeactivateSearch();
                    }
                    else if (zinput->KeyToggled(KEY_RETURN)) {
                        CatInvCore::searchInputActive = false;
                    }
                    else if (zinput->KeyToggled(KEY_BACK)) {
                        CatInvCore::RemoveLastSearchChar();
                    }
                    else {
                        if (zinput->AnyKeyPressed()) {
                            BYTE keys[256] = {};
                            auto keyboardLayout = GetKeyboardLayout(0);
                            
                            if (GetKeyboardState(keys) != FALSE) {
                                keys[VK_CAPITAL] = (BYTE)GetKeyState(VK_CAPITAL);
                                keys[VK_SHIFT] = (BYTE)GetKeyState(VK_SHIFT);
                                
                                wchar_t buff[] = { 0, 0 };
                                
                                for (int i = 0; i < MAX_KEYS; i++) {
                                    auto scan = MapVirtualKeyExW(i, MAPVK_VSC_TO_VK_EX, keyboardLayout);
                                    if (scan != 0 && zinput->KeyToggled(i)) {
                                        auto numChars = ToUnicodeEx(scan, scan, keys, buff, 2, 0, keyboardLayout);
                                        if (numChars == 1 && iswprint(buff[0])) {
                                            CatInvCore::searchText += buff[0];
                                            CatInvCore::UpdateAllContainers();
                                        }
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    
                    zinput->ClearKeyBuffer();
                }
                else if (CatInvCore::searchActive && !CatInvCore::searchInputActive) {
                    if (zinput->KeyToggled(KEY_ESCAPE)) {
                        CatInvCore::DeactivateSearch();
                    }
                    else if (shiftPressed) {
                        if (zinput->KeyToggled(KEY_LEFT)) {
                            CatInvCore::DeactivateSearch();
                            CatInvCore::ShiftCategory(-1);
                        }
                        else if (zinput->KeyToggled(KEY_RIGHT)) {
                            CatInvCore::DeactivateSearch();
                            CatInvCore::ShiftCategory(1);
                        }
                        else if (zinput->KeyToggled(KEY_HOME)) {
                            CatInvCore::DeactivateSearch();
                            CatInvCore::SetCategoryFirst();
                        }
                        else if (zinput->KeyToggled(KEY_END)) {
                            CatInvCore::DeactivateSearch();
                            CatInvCore::SetCategoryLast();
                        }
                    }
                    else {
                        if (zinput->KeyToggled(KEY_HOME)) {
                            oCItemContainer* activeContainer = nullptr;
                            if (CatInvCore::containerBySide[1] && CatInvCore::containerBySide[1]->IsActive()) {
                                activeContainer = CatInvCore::containerBySide[1];
                            } else if (CatInvCore::containerBySide[0] && CatInvCore::containerBySide[0]->IsActive()) {
                                activeContainer = CatInvCore::containerBySide[0];
                            } else {
                                activeContainer = &player->inventory2;
                            }
                            CatInvCore::SetSelectionFirst(activeContainer);
                        }
                        else if (zinput->KeyToggled(KEY_END)) {
                            oCItemContainer* activeContainer = nullptr;
                            if (CatInvCore::containerBySide[1] && CatInvCore::containerBySide[1]->IsActive()) {
                                activeContainer = CatInvCore::containerBySide[1];
                            } else if (CatInvCore::containerBySide[0] && CatInvCore::containerBySide[0]->IsActive()) {
                                activeContainer = CatInvCore::containerBySide[0];
                            } else {
                                activeContainer = &player->inventory2;
                            }
                            CatInvCore::SetSelectionLast(activeContainer);
                        }
                    }
                }
                else if (shiftPressed) {
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
                else {
                    if (zinput->KeyToggled(KEY_HOME)) {
                        oCItemContainer* activeContainer = nullptr;
                        if (CatInvCore::containerBySide[1] && CatInvCore::containerBySide[1]->IsActive()) {
                            activeContainer = CatInvCore::containerBySide[1];
                        } else if (CatInvCore::containerBySide[0] && CatInvCore::containerBySide[0]->IsActive()) {
                            activeContainer = CatInvCore::containerBySide[0];
                        } else {
                            activeContainer = &player->inventory2;
                        }
                        CatInvCore::SetSelectionFirst(activeContainer);
                    }
                    else if (zinput->KeyToggled(KEY_END)) {
                        oCItemContainer* activeContainer = nullptr;
                        if (CatInvCore::containerBySide[1] && CatInvCore::containerBySide[1]->IsActive()) {
                            activeContainer = CatInvCore::containerBySide[1];
                        } else if (CatInvCore::containerBySide[0] && CatInvCore::containerBySide[0]->IsActive()) {
                            activeContainer = CatInvCore::containerBySide[0];
                        } else {
                            activeContainer = &player->inventory2;
                        }
                        CatInvCore::SetSelectionLast(activeContainer);
                    }
                }
            }
        }
        catch (...) { }
    }

    void Game_Loop() {
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
        CatInvCore::searchActive = false;
        CatInvCore::searchInputActive = false;
        CatInvCore::searchText = L"";
        CatInvCore::searchView = nullptr;
        CatInvCore::activeCategory = CatInvOptions::G1Mode ? 1 : 0;
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
