#pragma once

namespace GOTHIC_ENGINE {
    const int INV_CAT_ALL       = 0;
    const int INV_CAT_WEAPON    = 1;
    const int INV_CAT_ARMOR     = 2;
    const int INV_CAT_RUNE      = 3;
    const int INV_CAT_MAGIC     = 4;
    const int INV_CAT_FOOD      = 5;
    const int INV_CAT_POTION    = 6;
    const int INV_CAT_DOC       = 7;
    const int INV_CAT_MISC      = 8;
    const int INV_CAT_MAX       = 9;

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
        0,
        ITEM_KAT_NF | ITEM_KAT_FF | ITEM_KAT_MUN,
        ITEM_KAT_ARMOR,
        ITEM_KAT_RUNE,
        ITEM_KAT_MAGIC,
        ITEM_KAT_FOOD,
        ITEM_KAT_POTIONS,
        ITEM_KAT_DOCS,
        ITEM_KAT_NONE | ITEM_KAT_LIGHT
    };

    const char* DEFAULT_CATEGORY_NAMES[INV_CAT_MAX] = {
        "",
        "Weapons",
        "Armor",
        "Magic",
        "Artifacts",
        "Food",
        "Potions",
        "Writings",
        "Miscellaneous"
    };

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
