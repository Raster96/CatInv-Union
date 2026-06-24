// CatInv Menu Key Binding System
// This file handles key binding configuration in the options menu
// Can be copied to other plugins for reusable key binding functionality
//
// HOW TO ADD NEW KEY BINDINGS:
// 1. Add the key variable to your Options class (e.g., CatInvOptions::KeySearch)
// 2. Add a new entry to the keyBindings[] array below with all required info
// 3. Create menu items in Daedalus (.d file) following the pattern:
//    - MenuItem_XXX (description, selectable)
//    - MenuItem_XXX_Choice (displays key name, NOT selectable)
// 4. That's it! The system handles everything else automatically.

#include "UnionAfx.h"

namespace GOTHIC_ENGINE {
    namespace CatInvMenu {
        // Configuration structure for key binding
        struct KeyBindingConfig {
            const char* menuName;              // Menu name (e.g., "CATINV:MENU_OPT_CATINV")
            const char* menuItemName;          // Menu item to detect click (e.g., "CATINV:MENUITEM_OPT_CATINV_KEYTOGGLEFAVORITE")
            const char* displayItemName;       // Menu item that displays key name (e.g., "CATINV:MENUITEM_OPT_CATINV_KEYTOGGLEFAVORITE_CHOICE")
            const char* iniSection;            // INI section (e.g., "CATINV_UNION")
            const char* iniKeyName;            // INI key name (e.g., "KeyToggleFavorite")
            int* keyCodePtr;                   // Pointer to the variable storing the key code
            int defaultKeyCode;                // Default key code if not in INI
        };
        
        // ============================================================================
        // REGISTER ALL KEY BINDINGS HERE - ADD NEW KEYS TO THIS ARRAY
        // ============================================================================
        static KeyBindingConfig keyBindings[] = {
            {
                "CATINV:MENU_OPT_CATINV",
                "CATINV:MENUITEM_OPT_CATINV_KEYTOGGLEFAVORITE",
                "CATINV:MENUITEM_OPT_CATINV_KEYTOGGLEFAVORITE_CHOICE",
                "CATINV_UNION",
                "KeyToggleFavorite",
                &CatInvOptions::KeyToggleFavorite,
                KEY_EQUALS
            }
            // ADD MORE KEY BINDINGS HERE - Example:
            // {
            //     "CATINV:MENU_OPT_CATINV",
            //     "CATINV:MENUITEM_OPT_CATINV_KEYSEARCH",
            //     "CATINV:MENUITEM_OPT_CATINV_KEYSEARCH_CHOICE",
            //     "CATINV_UNION",
            //     "KeySearch",
            //     &CatInvOptions::KeySearch,
            //     KEY_F
            // },
        };
        
        static const int numKeyBindings = sizeof(keyBindings) / sizeof(keyBindings[0]);
        
        // State variables
        bool waitingForKeyPress = false;
        zCMenuItem* keyMenuItem = nullptr;
        KeyBindingConfig* currentBinding = nullptr;
        
        // Update key binding menu item text with current key name
        void UpdateKeyBindingText() {
            if (!keyMenuItem || !currentBinding) {
                DEV_LOG("CatInvMenu: UpdateKeyBindingText - keyMenuItem or currentBinding is NULL" << endl);
                return;
            }
            
            // Get key name from zinput
            zSTRING keyName = zCInput::GetNameByControlValue(*(currentBinding->keyCodePtr));
            
            // Use SetText() to properly update the menu item
            keyMenuItem->SetText(keyName, 0, 1);
            
            DEV_LOG("CatInvMenu: Updated key binding text to '" << keyName << "'" << endl);
        }
        
        // Called when user selects a key binding menu item
        void OnKeyBindingSelected(zCMenuItem* activeItemPtr, KeyBindingConfig* binding) {
            if (!activeItemPtr || !binding) {
                DEV_LOG("CatInvMenu: OnKeyBindingSelected - NULL pointer!" << endl);
                return;
            }
            
            DEV_LOG("CatInvMenu: Starting key capture for " << binding->iniKeyName << endl);
            
            keyMenuItem = activeItemPtr;
            currentBinding = binding;
            
            // Update text to show we're waiting for key press
            keyMenuItem->SetText(zSTRING("..."), 0, 1);
            
            waitingForKeyPress = true;
            DEV_LOG("CatInvMenu: waitingForKeyPress set to TRUE" << endl);
            
            // Clear any pending keys
            if (zinput) {
                zinput->ClearKeyBuffer();
            }
        }
        
        // Process key capture when waiting for user input
        void ProcessKeyCapture() {
            if (!waitingForKeyPress || !keyMenuItem || !zinput || !currentBinding) {
                return;
            }
            
            static bool loggedOnce = false;
            if (!loggedOnce) {
                DEV_LOG("CatInvMenu: ProcessKeyCapture active, scanning for keys..." << endl);
                loggedOnce = true;
            }
            
            // Check for ESC to cancel
            if (zinput->KeyToggled(KEY_ESCAPE)) {
                DEV_LOG("CatInvMenu: ESC pressed, canceling" << endl);
                waitingForKeyPress = false;
                loggedOnce = false;
                UpdateKeyBindingText(); // Restore original key name
                
                // Clear key buffer to prevent ESC from closing the menu
                zinput->ClearKeyBuffer();
                return;
            }
            
            // Scan for any key press (excluding special keys we don't want to bind)
            for (int keyCode = 0; keyCode < 256; keyCode++) {
                // Skip keys we don't want to allow binding
                if (keyCode == KEY_ESCAPE || keyCode == KEY_RETURN) continue;
                
                if (zinput->KeyToggled(keyCode)) {
                    // Found a key press!
                    DEV_LOG("CatInvMenu: Key pressed! Code = " << keyCode << endl);
                    
                    // Update the key code in the options
                    *(currentBinding->keyCodePtr) = keyCode;
                    
                    // Save to Gothic.ini
                    if (zoptions) {
                        zoptions->WriteInt(currentBinding->iniSection, currentBinding->iniKeyName, keyCode, false);
                    }
                    
                    // Update menu display
                    waitingForKeyPress = false;
                    loggedOnce = false;
                    UpdateKeyBindingText();
                    
                    DEV_LOG("CatInv: Key binding " << currentBinding->iniKeyName << " changed to " << keyCode << " (" << zCInput::GetNameByControlValue(keyCode).ToChar() << ")" << endl);
                    
                    currentBinding = nullptr;
                    break;
                }
            }
        }
        
        // Initialize menu handlers (called on game start)
        void Initialize() {
            // Nothing to initialize at game start - menu items don't exist yet
        }
        
        // Initialize key menu items and update text (called when menu is opened)
        void InitializeKeyMenuItems() {
            zCMenu* activeMenu = zCMenu::GetActive();
            if (!activeMenu) return;
            
            // Check each registered key binding
            for (int b = 0; b < numKeyBindings; b++) {
                KeyBindingConfig* binding = &keyBindings[b];
                
                // Only initialize if we're in the correct menu
                if (activeMenu->GetName() != binding->menuName) continue;
                
                // Find the display item
                for (int i = 0; i < activeMenu->m_listItems.GetNum(); i++) {
                    zCMenuItem* menuItem = activeMenu->m_listItems[i];
                    if (menuItem && menuItem->GetName() == binding->displayItemName) {
                        // Update the display with current key name
                        zSTRING keyName = zCInput::GetNameByControlValue(*(binding->keyCodePtr));
                        menuItem->SetText(keyName, 0, 1);
                        DEV_LOG("CatInvMenu: Initialized " << binding->iniKeyName << " with key '" << keyName << "'" << endl);
                        break;
                    }
                }
            }
        }
        
        // Check if user clicked on any key binding option in menu
        // Call this from Game_MenuLoop()
        void CheckKeyBindingActivation() {
            if (!zinput) return;
            
            // Initialize key menu items if not done yet (first time menu is opened)
            static bool initialized = false;
            zCMenu* menu = zCMenu::GetActive();
            
            if (menu) {
                // Check if we're in any registered menu and initialize if needed
                for (int b = 0; b < numKeyBindings; b++) {
                    if (menu->GetName() == keyBindings[b].menuName && !initialized) {
                        InitializeKeyMenuItems();
                        initialized = true;
                        break;
                    }
                }
            }
            
            // Reset flag when leaving all registered menus
            bool inAnyMenu = false;
            if (menu) {
                for (int b = 0; b < numKeyBindings; b++) {
                    if (menu->GetName() == keyBindings[b].menuName) {
                        inAnyMenu = true;
                        break;
                    }
                }
            }
            if (!inAnyMenu) {
                initialized = false;
            }
            
            // Check for Enter key or left mouse button click
            if (zinput->KeyToggled(KEY_RETURN) || zinput->GetMouseButtonToggledLeft()) {
                if (!menu) return;
                
                zCMenuItem* item = menu->GetActiveItem();
                if (!item) return;
                
                zSTRING itemName = item->GetName();
                
                // Check each registered key binding
                for (int b = 0; b < numKeyBindings; b++) {
                    KeyBindingConfig* binding = &keyBindings[b];
                    
                    // Check if we're in the correct menu and clicked the correct item
                    if (menu->GetName() == binding->menuName && itemName == binding->menuItemName) {
                        DEV_LOG(">>> Starting key capture for " << binding->iniKeyName << "!" << endl);
                        
                        // Find the display item
                        for (int i = 0; i < menu->m_listItems.GetNum(); i++) {
                            zCMenuItem* menuItem = menu->m_listItems[i];
                            if (menuItem && menuItem->GetName() == binding->displayItemName) {
                                OnKeyBindingSelected(menuItem, binding);
                                zinput->ClearKeyBuffer();
                                return; // Found and handled
                            }
                        }
                    }
                }
            }
        }
    }
}
