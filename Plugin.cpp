#include "UnionAfx.h"
#include "resource.h"

namespace GOTHIC_ENGINE {
    
    // Include menu handling
    #include "CatInvMenu.h"

    void Game_Entry() {
    }
    
    void Game_Init() {
        if (zoptions) {
            CatInvOptions::ReadOptions();
        }
        CatInvCore::activeCategory = INV_CAT_ALL; // Start at ALL (0)
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
        
        // Take initial snapshot if empty (new game)
        CatInvCore::TakeInventorySnapshot();
        
        if (parser) {
            int catinvG1Mode = parser->GetIndex("CATINV_G1MODE");
            if (catinvG1Mode >= 0) {
                zCPar_Symbol* sym = parser->GetSymbol(catinvG1Mode);
                if (sym && sym->type == zPAR_TYPE_INT) {
                    sym->single_intdata = 1;
                }
            }
            
            int symIdx = parser->GetIndex("CATINV_INIT");
            if (symIdx >= 0) {
                zCPar_Symbol* sym = parser->GetSymbol(symIdx);
                if (sym && sym->type == zPAR_TYPE_FUNC) {
                    int codePos = sym->single_intdata;
                    if (codePos >= 0 && codePos < parser->stack.stacksize) {
                        parser->stack.stack[codePos] = zPAR_TOK_RET;
                    }
                }
            }
        }
    }

    void Game_Exit() {
    }

    void Game_PreLoop() {
        if (!player || !screen || !zinput || !ogame) return;
        if (ogame->inScriptStartup || ogame->inLoadSaveGame || ogame->inLevelChange) return;

        try {
            // Check if we're waiting for key binding input
            if (CatInvMenu::waitingForKeyPress) {
                CatInvMenu::ProcessKeyCapture();
                return; // Don't process other inputs while capturing key
            }
            
            if (player->inventory2.IsOpen()) {
                bool shiftPressed = zinput->KeyPressed(KEY_LSHIFT) || zinput->KeyPressed(KEY_RSHIFT);
                
                if (!CatInvCore::searchInputActive && zinput->KeyToggled(CatInvOptions::KeyToggleFavorite)) {
                    oCItemContainer* activeContainer = nullptr;
                    if (CatInvCore::containerBySide[1] && CatInvCore::containerBySide[1]->IsActive()) {
                        activeContainer = CatInvCore::containerBySide[1];
                    } else if (CatInvCore::containerBySide[0] && CatInvCore::containerBySide[0]->IsActive()) {
                        activeContainer = CatInvCore::containerBySide[0];
                    } else {
                        activeContainer = &player->inventory2;
                    }
                    
                    if (activeContainer && activeContainer->contents) {
                        oCItem* selectedItem = nullptr;
                        int currentIndex = 0;
                        zCListSort<oCItem>* list = activeContainer->contents->next;
                        
                        while (list) {
                            if (currentIndex == activeContainer->selectedItem) {
                                selectedItem = list->data;
                                break;
                            }
                            currentIndex++;
                            list = list->next;
                        }
                        
                        if (selectedItem) {
                            CatInvCore::ToggleFavorite(selectedItem);
                        }
                    }
                }
                
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
                        if (CatInvCore::searchText.length() == 0) {
                            CatInvCore::DeactivateSearch();
                        } else {
                            CatInvCore::searchInputActive = false;
                        }
                    }
                    else if (zinput->KeyToggled(KEY_UP) || zinput->KeyToggled(KEY_DOWN) || 
                             zinput->KeyToggled(KEY_LEFT) || zinput->KeyToggled(KEY_RIGHT)) {
                        if (CatInvCore::searchText.length() == 0) {
                            CatInvCore::DeactivateSearch();
                        } else {
                            CatInvCore::searchInputActive = false;
                        }
                    }
                    else if (zinput->KeyToggled(KEY_BACK)) {
                        // Shift+Backspace clears entire search text
                        bool shiftPressed = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
                        if (shiftPressed) {
                            CatInvCore::searchText = L"";
                            CatInvCore::UpdateAllContainers();
                        } else {
                            CatInvCore::RemoveLastSearchChar();
                        }
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
                        if (zinput->KeyToggled(KEY_HOME)) {
                            CatInvCore::DeactivateSearch();
                            CatInvCore::SetCategoryFirst();
                            // Clear only KEY_HOME, not Shift keys
                            zinput->SetKey(KEY_HOME, 0);
                        }
                        else if (zinput->KeyToggled(KEY_END)) {
                            CatInvCore::DeactivateSearch();
                            CatInvCore::SetCategoryLast();
                            // Clear only KEY_END, not Shift keys
                            zinput->SetKey(KEY_END, 0);
                        }
                        else if (zinput->KeyToggled(KEY_H)) {
                            CatInvCore::DeactivateSearch();
                            CatInvCore::SetCategory(INV_CAT_FAVORITES);
                            // Clear only KEY_H, not Shift keys
                            zinput->SetKey(KEY_H, 0);
                        }
                        else if (zinput->KeyToggled(KEY_R)) {
                            CatInvCore::DeactivateSearch();
                            CatInvCore::SetCategory(INV_CAT_RECENT);
                            // Clear only KEY_R, not Shift keys
                            zinput->SetKey(KEY_R, 0);
                        }
                        else if (zinput->KeyToggled(KEY_SPACE)) {
                            CatInvCore::JumpToItemCategory();
                            // Clear only KEY_SPACE, not Shift keys
                            zinput->SetKey(KEY_SPACE, 0);
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
                    if (zinput->KeyToggled(KEY_HOME)) {
                        CatInvCore::SetCategoryFirst();
                        // Clear only KEY_HOME, not Shift keys
                        zinput->SetKey(KEY_HOME, 0);
                    }
                    else if (zinput->KeyToggled(KEY_END)) {
                        CatInvCore::SetCategoryLast();
                        // Clear only KEY_END, not Shift keys
                        zinput->SetKey(KEY_END, 0);
                    }
                    else if (zinput->KeyToggled(KEY_H)) {
                        CatInvCore::SetCategory(INV_CAT_FAVORITES);
                        // Clear only KEY_H, not Shift keys
                        zinput->SetKey(KEY_H, 0);
                    }
                    else if (zinput->KeyToggled(KEY_R)) {
                        CatInvCore::SetCategory(INV_CAT_RECENT);
                        // Clear only KEY_R, not Shift keys
                        zinput->SetKey(KEY_R, 0);
                    }
                    else if (zinput->KeyToggled(KEY_SPACE)) {
                        CatInvCore::JumpToItemCategory();
                        // Clear only KEY_SPACE, not Shift keys
                        zinput->SetKey(KEY_SPACE, 0);
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
        // Process key capture if waiting for key binding
        if (CatInvMenu::waitingForKeyPress) {
            CatInvMenu::ProcessKeyCapture();
            return; // Don't process other menu logic while capturing
        }
        
        // Check if user clicked on key binding menu option
        CatInvMenu::CheckKeyBindingActivation();
    }

    TSaveLoadGameInfo& SaveLoadGameInfo = UnionCore::SaveLoadGameInfo;

    void Game_SaveBegin() {
        // Clear runtime pointers before save (will be rebuilt on next container open)
        CatInvCore::recentItemPointers.clear();
        DEV_LOG("Game_SaveBegin: Cleared recentItemPointers" << endl);
    }

    void Game_SaveEnd() {
        int slotId = SaveLoadGameInfo.slotID;
        CatInvCore::SaveFavorites(slotId);
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
        CatInvCore::activeSortMode = 0;
        CatInvCore::sortView = nullptr;
        CatInvCore::activeCategory = INV_CAT_ALL;
        CatInvCore::previousCategory = 0;
        CatInvCore::recentlyRemovedInstances.clear();
        CatInvCore::isEquipping = false;
    }

    void LoadEnd() {
        if (parser) {
            zCPar_Symbol* catSym = parser->GetSymbol("CATINV_ACTIVECATEGORY");
            if (catSym && catSym->type == zPAR_TYPE_INT) {
                catSym->single_intdata = 0;
            }
        }
    }

    void Game_LoadBegin_NewGame() {
        LoadBegin();
        CatInvCore::ClearFavorites();
        CatInvCore::recentItemPointers.clear();
        CatInvCore::pickupQueue.clear();
        DEV_LOG("Game_LoadBegin_NewGame: Cleared recentItemPointers and pickupQueue" << endl);
    }

    void Game_LoadEnd_NewGame() {
        LoadEnd();
        CatInvCore::ClearRecentItems();
    }

    void Game_LoadBegin_SaveGame() {
        LoadBegin();
        CatInvCore::recentItemPointers.clear();
        CatInvCore::pickupQueue.clear();
        DEV_LOG("Game_LoadBegin_SaveGame: Cleared recentItemPointers and pickupQueue" << endl);
        int slotId = SaveLoadGameInfo.slotID;
        CatInvCore::LoadFavorites(slotId);
    }

    void Game_LoadEnd_SaveGame() {
        LoadEnd();
    }

    void Game_LoadBegin_ChangeLevel() {
        LoadBegin();
        CatInvCore::recentItemPointers.clear();
        DEV_LOG("Game_LoadBegin_ChangeLevel: Cleared recentItemPointers" << endl);
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
        // Block TCOM original Daedalus-based CatInv to prevent conflicts with CatInv-Union.
        if (parser) {
            // First, try to set CATINV_G1MODE to disable TCOM CatInv keyboard handling
            int catinvG1Mode = parser->GetIndex("CATINV_G1MODE");
            if (catinvG1Mode >= 0) {
                zCPar_Symbol* sym = parser->GetSymbol(catinvG1Mode);
                if (sym && sym->type == zPAR_TYPE_INT) {
                    sym->single_intdata = 1;  // Set to TRUE/1
                }
            }
            
            // Block init function to prevent hooks installation (if it hasn't run yet)
            int symIdx = parser->GetIndex("CATINV_INIT");
            if (symIdx >= 0) {
                zCPar_Symbol* sym = parser->GetSymbol(symIdx);
                if (sym && sym->type == zPAR_TYPE_FUNC) {
                    int codePos = sym->single_intdata;
                    if (codePos >= 0 && codePos < parser->stack.stacksize) {
                        parser->stack.stack[codePos] = zPAR_TOK_RET;
                    }
                }
            }
            
            // Also block ALL CATINV functions that handle keyboard events
            // This is redundant but ensures no TCOM CatInv code runs
            const char* catinvFunctions[] = {
                "CATINV_HANDLEEVENT",
                "CATINV_HANDLEEVENTEDI",
                "CATINV_HANDLEEVENTEBX",
                "CATINV_HANDLEEVENTNPCINVENTORY",
                "CATINV_RIGHT",
                "CATINV_LEFT"
            };
            
            for (int i = 0; i < sizeof(catinvFunctions) / sizeof(catinvFunctions[0]); i++) {
                int idx = parser->GetIndex(catinvFunctions[i]);
                if (idx >= 0) {
                    zCPar_Symbol* sym = parser->GetSymbol(idx);
                    if (sym && sym->type == zPAR_TYPE_FUNC) {
                        int codePos = sym->single_intdata;
                        if (codePos >= 0 && codePos < parser->stack.stacksize) {
                            parser->stack.stack[codePos] = zPAR_TOK_RET;
                        }
                    }
                }
            }
        }
        
        // Register CatInv key bindings in Gothic's system
        if (zoptions) {
            // Ensure the key is registered in [CATINV_UNION] section (will use default if not already set)
            if (!zoptions->EntryExists("CATINV_UNION", "KeyToggleFavorite")) {
                zoptions->WriteInt("CATINV_UNION", "KeyToggleFavorite", KEY_EQUALS, false);
            }
        }
        
        // Initialize menu handlers
        CatInvMenu::Initialize();
    }

    void Game_ApplyOptions() {
        CatInvOptions::ReadOptions();
        // Trim recent items if user reduced MaxRecentItems in options
        CatInvCore::TrimRecentItems();
        // Update menu to show current key binding
        CatInvMenu::UpdateKeyBindingText();
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
