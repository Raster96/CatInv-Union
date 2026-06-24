// CatInv Menu Key Binding System - Header
// This file declares the menu key binding functionality
//
// USAGE IN OTHER PLUGINS:
// 1. Copy CatInvMenu.cpp and CatInvMenu.h to your plugin
// 2. Include CatInvMenu.h in your Plugin.cpp
// 3. Call CatInvMenu::Initialize() in Game_DefineExternals()
// 4. Call CatInvMenu::CheckKeyBindingActivation() in Game_MenuLoop()
// 5. Call CatInvMenu::ProcessKeyCapture() in Game_MenuLoop() when waitingForKeyPress is true
// 6. Update the keyBindings[] array in CatInvMenu.cpp with your key bindings

#pragma once

namespace GOTHIC_ENGINE {
    namespace CatInvMenu {
        // State variables
        extern bool waitingForKeyPress;
        
        // Main API functions
        void Initialize();                      // Call in Game_DefineExternals()
        void CheckKeyBindingActivation();       // Call in Game_MenuLoop()
        void ProcessKeyCapture();               // Call in Game_MenuLoop() when waitingForKeyPress == true
    }
}
