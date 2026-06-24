// CatInv Options
#pragma once

namespace GOTHIC_ENGINE {
    namespace CatInvOptions {
        bool ChangeOnLast = false;
        bool HideFavorites = false;
        bool HideRecent = false;
        zSTRING CategoryOrder = "";
        int FavoriteIconCorner = 0; // 0=bottom-left, 1=bottom-right, 2=top-left, 3=top-right
        int FavoriteIconSize = 0; // 0=100%, 1=150%, 2=200%
        int MaxRecentItems = 15; // Maximum number of recent items to track (default: 15, min: 1, no upper limit)
        int KeyToggleFavorite = KEY_EQUALS; // Key to toggle favorite (default: = key, configurable in menu)

        int invCatOrder[INV_CAT_MAX];

        // Helper function to map category name to ID
        int GetCategoryIDFromName(const char* name) {
            if (_stricmp(name, "FAVORITES") == 0) return INV_CAT_FAVORITES; // 0
            if (_stricmp(name, "RECENT") == 0) return INV_CAT_RECENT;       // 1
            if (_stricmp(name, "ALL") == 0) return INV_CAT_ALL;             // 2
            if (_stricmp(name, "COMBAT") == 0 || _stricmp(name, "WEAPON") == 0 || _stricmp(name, "WEAPONS") == 0) return INV_CAT_WEAPON;  // 3
            if (_stricmp(name, "ARMOR") == 0) return INV_CAT_ARMOR;         // 4
            if (_stricmp(name, "RUNE") == 0 || _stricmp(name, "MAGIC") == 0) return INV_CAT_RUNE;  // 5
            if (_stricmp(name, "ARTIFACT") == 0 || _stricmp(name, "ARTIFACTS") == 0) return INV_CAT_MAGIC;  // 6
            if (_stricmp(name, "FOOD") == 0) return INV_CAT_FOOD;           // 7
            if (_stricmp(name, "POTION") == 0 || _stricmp(name, "POTIONS") == 0) return INV_CAT_POTION;  // 8
            if (_stricmp(name, "DOCS") == 0 || _stricmp(name, "DOC") == 0 || _stricmp(name, "WRITINGS") == 0) return INV_CAT_DOC;  // 9
            if (_stricmp(name, "OTHER") == 0 || _stricmp(name, "MISC") == 0 || _stricmp(name, "MISCELLANEOUS") == 0) return INV_CAT_MISC;  // 10
            // Try to parse as number
            return atoi(name);
        }

        void ReadOptions() {
            if (!zoptions) {
                return;
            }
            
            ChangeOnLast = zoptions->ReadBool(PLUGIN_NAME, "invCatChangeOnLast", false);
            HideFavorites = zoptions->ReadBool(PLUGIN_NAME, "invCatHideFavorites", false);
            HideRecent = zoptions->ReadBool(PLUGIN_NAME, "invCatHideRecent", false);
            CategoryOrder = zoptions->ReadString(PLUGIN_NAME, "invCatOrder", "FAVORITES,RECENT,ALL,COMBAT,ARMOR,RUNE,ARTIFACTS,FOOD,POTION,DOCS,OTHER");
            FavoriteIconCorner = zoptions->ReadInt(PLUGIN_NAME, "FavoriteIconCorner", 0);
            FavoriteIconSize = zoptions->ReadInt(PLUGIN_NAME, "FavoriteIconSize", 0);
            MaxRecentItems = zoptions->ReadInt(PLUGIN_NAME, "MaxRecentItems", 15);
            
            // Read key binding from [CATINV_UNION] section
            KeyToggleFavorite = zoptions->ReadInt("CATINV_UNION", "KeyToggleFavorite", KEY_EQUALS);
            
            // Clamp MaxRecentItems to minimum 1 (no upper limit)
            if (MaxRecentItems < 1) MaxRecentItems = 1;
            
            DEV_LOG("CatInv: MaxRecentItems set to " << MaxRecentItems << endl);
            
            // Trim recentItems list if MaxRecentItems was reduced
            // This is declared in CatInvCore, need to trim from there after ReadOptions
            // (See TrimRecentItems() call in CatInvCore initialization)

            // Initialize with -1 to mark as "not in list" (hidden)
            for (int i = 0; i < INV_CAT_MAX; i++) {
                invCatOrder[i] = -1;
            }

            const char* str = CategoryOrder.ToChar();
            int len = CategoryOrder.Length();
            int orderIndex = 0; // Order position
            int tokenStart = 0;
            
            DEV_LOG("CatInv: Parsing category order: " << CategoryOrder.ToChar() << endl);
            
            for (int i = 0; i <= len; i++) {
                if (i == len || str[i] == ',') {
                    if (i > tokenStart) {
                        char token[32] = {0};
                        int tokenLen = i - tokenStart;
                        if (tokenLen < 32) {
                            for (int j = 0; j < tokenLen; j++) {
                                token[j] = str[tokenStart + j];
                            }
                            int start = 0;
                            while (start < tokenLen && (token[start] == ' ' || token[start] == '\t')) start++;
                            int end = tokenLen - 1;
                            while (end > start && (token[end] == ' ' || token[end] == '\t' || token[end] == 0)) end--;
                            token[end + 1] = 0;
                            
                            int catID = GetCategoryIDFromName(&token[start]);
                            if (catID >= INV_CAT_FAVORITES && catID < INV_CAT_MAX) {
                                invCatOrder[catID] = orderIndex;
                                DEV_LOG("  Category " << catID << " (" << &token[start] << ") -> order " << orderIndex << endl);
                                orderIndex++;
                            }
                        }
                    }
                    tokenStart = i + 1;
                }
            }
            
            if (InDevelopment) {
                cmd << "CatInv: Hidden categories: ";
                bool anyHidden = false;
                for (int i = 0; i < INV_CAT_MAX; i++) {
                    if (invCatOrder[i] == -1) {
                        cmd << i << " ";
                        anyHidden = true;
                    }
                }
                if (!anyHidden) {
                    cmd << "none";
                }
                cmd << endl;
            }
        }

        void AddTrivias() {
        }
    }
}
