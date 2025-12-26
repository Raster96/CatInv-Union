// CatInv Options
#pragma once

namespace GOTHIC_ENGINE {
    namespace CatInvOptions {
        bool ChangeOnLast = false;
        bool G1Mode = false;
        zSTRING CategoryOrder = "";

        int invCatOrder[INV_CAT_MAX];

        // Helper function to map category name to ID
        int GetCategoryIDFromName(const char* name) {
            if (_stricmp(name, "COMBAT") == 0 || _stricmp(name, "WEAPON") == 0) return INV_CAT_WEAPON;
            if (_stricmp(name, "ARMOR") == 0) return INV_CAT_ARMOR;
            if (_stricmp(name, "RUNE") == 0) return INV_CAT_RUNE;
            if (_stricmp(name, "MAGIC") == 0) return INV_CAT_MAGIC;
            if (_stricmp(name, "FOOD") == 0) return INV_CAT_FOOD;
            if (_stricmp(name, "POTION") == 0 || _stricmp(name, "POTIONS") == 0) return INV_CAT_POTION;
            if (_stricmp(name, "DOCS") == 0 || _stricmp(name, "DOC") == 0) return INV_CAT_DOC;
            if (_stricmp(name, "OTHER") == 0 || _stricmp(name, "MISC") == 0) return INV_CAT_MISC;
            if (_stricmp(name, "NONE") == 0) return 0;
            // Try to parse as number
            return atoi(name);
        }

        void ReadOptions() {
            if (!zoptions) {
                return;
            }
            
            ChangeOnLast = zoptions->ReadBool(PLUGIN_NAME, "invCatChangeOnLast", false);
            G1Mode = zoptions->ReadBool(PLUGIN_NAME, "invCatG1Mode", false);
            CategoryOrder = zoptions->ReadString(PLUGIN_NAME, "invCatOrder", "COMBAT,ARMOR,RUNE,MAGIC,FOOD,POTION,DOCS,OTHER");

            for (int i = 0; i < INV_CAT_MAX; i++) {
                invCatOrder[i] = i;
            }

            const char* str = CategoryOrder.ToChar();
            int len = CategoryOrder.Length();
            int catIndex = 1;
            int tokenStart = 0;
            
            for (int i = 0; i <= len && catIndex < INV_CAT_MAX; i++) {
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
                            if (catID >= 1 && catID < INV_CAT_MAX) {
                                invCatOrder[catID] = catIndex - 1;
                            }
                            catIndex++;
                        }
                    }
                    tokenStart = i + 1;
                }
            }
        }

        void AddTrivias() {
        }
    }
}
