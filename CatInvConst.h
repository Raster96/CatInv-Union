#pragma once

namespace GOTHIC_ENGINE {
    // Development mode flag - set to false for release builds to disable debug logging
    const bool InDevelopment = false;
    
    // Helper macro for conditional logging
    #define DEV_LOG(msg) if (InDevelopment) { cmd << msg; }
    
    const int INV_CAT_FAVORITES = 0;
    const int INV_CAT_RECENT    = 1;
    const int INV_CAT_ALL       = 2;
    const int INV_CAT_WEAPON    = 3;
    const int INV_CAT_ARMOR     = 4;
    const int INV_CAT_RUNE      = 5;
    const int INV_CAT_MAGIC     = 6;
    const int INV_CAT_FOOD      = 7;
    const int INV_CAT_POTION    = 8;
    const int INV_CAT_DOC       = 9;
    const int INV_CAT_MISC      = 10;
    const int INV_CAT_MAX       = 11;

    const int ITEM_KAT_NONE     = 1 << 0;
    const int ITEM_KAT_NF       = 1 << 1;
    const int ITEM_KAT_FF       = 1 << 2;
    const int ITEM_KAT_MUN      = 1 << 3;
    const int ITEM_KAT_ARMOR    = 1 << 4;
    const int ITEM_KAT_FOOD     = 1 << 5;
    const int ITEM_KAT_DOCS     = 1 << 6;
    const int ITEM_KAT_POTIONS  = 1 << 7;
    const int ITEM_KAT_LIGHT    = 1 << 8;
    const int ITEM_KAT_RUNE     = 1 << 9;
    const int ITEM_KAT_MAGIC    = 1 << 31;

    const int INV_CAT_GROUPS[INV_CAT_MAX] = {
        0,                                         // Favorites
        0,                                         // Recent
        0,                                         // All
        ITEM_KAT_NF | ITEM_KAT_FF | ITEM_KAT_MUN, // Weapons
        ITEM_KAT_ARMOR,                            // Armor
        ITEM_KAT_RUNE,                             // Magic
        ITEM_KAT_MAGIC,                            // Artifacts
        ITEM_KAT_FOOD,                             // Food
        ITEM_KAT_POTIONS,                          // Potions
        ITEM_KAT_DOCS,                             // Writings
        ITEM_KAT_NONE | ITEM_KAT_LIGHT            // Miscellaneous
    };

    const char* DEFAULT_CATEGORY_NAMES[INV_CAT_MAX] = {
        "Favorites",
        "Recent",
        "All",             
        "Weapons",       
        "Armor",          
        "Magic",          
        "Artifacts",     
        "Food",        
        "Potions",         
        "Writings",       
        "Miscellaneous"    
    };
    
    inline const char* GetFavoritesText() {
        LANGID langId = GetSystemDefaultLangID();
        WORD primaryLang = PRIMARYLANGID(langId);
        
        switch (primaryLang) {
            case LANG_ENGLISH:
                return "Favorites";
            case LANG_POLISH:
                return "Ulubione";
            case LANG_GERMAN:
                return "Favoriten";
            case LANG_FRENCH:
                return "Favoris";
            case LANG_ITALIAN:
                return "Preferiti";
            case LANG_SPANISH:
                return "Favoritos";
            case LANG_RUSSIAN:
                return "Избранное";
            case LANG_CZECH:
                return "Oblíbené";
            case LANG_HUNGARIAN:
                return "Kedvencek";
            case LANG_UKRAINIAN:
                return "Обране";
            default:
                return "Favorites";
        }
    }
    
    inline const char* GetRecentText() {
        LANGID langId = GetSystemDefaultLangID();
        WORD primaryLang = PRIMARYLANGID(langId);
        
        switch (primaryLang) {
            case LANG_ENGLISH:
                return "Recent";
            case LANG_POLISH:
                return "Najnowsze";
            case LANG_GERMAN:
                return "Neueste";
            case LANG_FRENCH:
                return "Récent";
            case LANG_ITALIAN:
                return "Recente";
            case LANG_SPANISH:
                return "Reciente";
            case LANG_RUSSIAN:
                return "Новое";
            case LANG_CZECH:
                return "Nedávné";
            case LANG_HUNGARIAN:
                return "Legutóbbi";
            case LANG_UKRAINIAN:
                return "Нещодавні";
            default:
                return "Recent";
        }
    }

#if ENGINE == Engine_G1A
    const int DEFAULT_HEIGHT_ADDR = 0x00981AB4;
    const int DEFAULT_WIDTH_ADDR  = 0x00981AF4;
#elif ENGINE == Engine_G2 || ENGINE == Engine_G2A
    const int DEFAULT_HEIGHT_ADDR = 0x00AB0F68;
    const int DEFAULT_WIDTH_ADDR  = 0x00AB0FA8;
#else
    const int DEFAULT_HEIGHT_ADDR = 0x00AB0F68;
    const int DEFAULT_WIDTH_ADDR  = 0x00AB0FA8;
#endif
}
