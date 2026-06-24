META
{
  Parser = Menu;
  After = zUnionMenu.d;
  Namespace = CatInv;
};

// Namespace = CatInv
// File encoding: UTF-8 (without BOM).

// ------ Constants ------
const int Start_PY  = 1400;
const int Title_PY  = 450;
const int Menu_DY   = 550;
// Text
const int Text_PX   = 400;
const int Text_SX   = 8000;
const int Text_SY   = 750;
const int Text_DY   = 120;
// Choice
const int Choice_PX = 6400;
const int Choice_SX = 1500;
const int Choice_SY = 350;
const int Choice_DY = 120;

const string MenuBackPic   = "UnionMenu_BackPic.tga";
const string ItemBackPic   = "";
const string ChoiceBackPic = "MENU_CHOICE_BACK.TGA";
const string FontSmall     = "font_old_10_white.tga";
const string FontBig       = "font_old_20_white.tga";

var int CurrentMenuItem_PY;

// ------ Prototypes ------
func void InitializeBackPicturesAndFonts()
{
  MenuBackPic   = MENU_BACK_PIC;
  ItemBackPic   = MENU_ITEM_BACK_PIC;
  ChoiceBackPic = MENU_CHOICE_BACK_PIC;
  FontSmall     = MENU_FONT_SMALL;
  FontBig       = MENU_FONT_DEFAULT;
};

prototype C_EMPTY_MENU_DEF(C_MENU)
{
  InitializeBackPicturesAndFonts();
  C_MENU_DEF();
  backpic    = MenuBackPic;
  items[0]   = "";
  items[100] = "Union_menuitem_back";
  flags      = flags | MENU_SHOW_INFO;
};

instance C_MENU_ITEM_TEXT_BASE(C_MENU_ITEM_DEF)
{
  backpic        = ItemBackPic;
  posx           = Text_PX;
  posy           = Start_PY;
  dimx           = Text_SX;
  dimy           = Text_SY;
  flags          = flags | IT_EFFECTS_NEXT;
  onselaction[0] = SEL_ACTION_UNDEF;
};

instance C_MENUITEM_CHOICE_BASE(C_MENU_ITEM_DEF)
{
  backpic  = ChoiceBackPic;
  type     = MENU_ITEM_CHOICEBOX;
  fontname = FontSmall;
  posx     = Choice_PX;
  posy     = Start_PY + Choice_DY;
  dimx     = Choice_SX;
  dimy     = Choice_SY;
  flags    = flags & ~IT_SELECTABLE;
  flags    = flags | IT_TXT_CENTER;
};

instance MenuItem_Opt_Headline(C_MENU_ITEM_DEF)
{
  type    = MENU_ITEM_TEXT;
  posx    = 0;
  posy    = Title_PY;
  dimx    = 8100;
  flags   = flags & ~IT_SELECTABLE;
  flags   = flags | IT_TXT_CENTER;
  text[0] = Str_GetLocalizedString(
    "CatInv - НАСТРОЙКИ",
    "CatInv - SETTINGS",
    "CatInv - EINSTELLUNGEN",
    "CatInv - USTAWIENIA"
  );
};

func int Act_OpenWebLink()
{
  Open_Link("https://github.com/Raster96/CatInv-Union#usage");
  return 0;
};

instance MenuItem_Opt_Open_Link(C_MENU_ITEM_DEF)
{
  C_MENU_ITEM_TEXT_BASE();
  posy += MENU_DY * 7;

  posx             = 64;
  onselaction[0]   = SEL_ACTION_UNDEF;
  oneventaction[1] = Act_OpenWebLink;
  flags            = flags | IT_TXT_CENTER;
  text[0]          = Str_GetLocalizedString(
    "Управление",
    "Controls",
    "Steuerung",
    "Sterowanie"
  );

  text[1]          = "https://github.com/Raster96/CatInv-Union#usage";
};

// ------ Menu ------
instance MenuItem_Union_Auto_CatInv(C_MENU_ITEM_UNION_DEF)
{
  text[0]          = "CatInv";
  text[1] = Str_GetLocalizedString(
    "Настроить параметры CatInv",
    "Configure CatInv settings",
    "CatInv-Einstellungen konfigurieren",
    "Konfiguruj ustawienia CatInv"
  );
  onselaction[0]   = SEL_ACTION_STARTMENU;
  onselaction_s[0] = "CatInv:Menu_Opt_CatInv";
};

instance Menu_Opt_CatInv(C_EMPTY_MENU_DEF)
{
  Menu_SearchItems("CatInv:MENUITEM_OPT_CATINV_*");
};

instance MenuItem_Opt_CatInv_Headline(C_MENU_ITEM)
{
  MenuItem_Opt_Headline();
};

// ====== 1. Favorite Icon Corner ======
instance MenuItem_Opt_CatInv_IconCorner(C_MENU_ITEM)
{
  CurrentMenuItem_PY = 1;
  C_MENU_ITEM_TEXT_BASE();
  fontname = FontSmall;
  posy += Menu_DY * CurrentMenuItem_PY + Text_DY;

  text[0] = Str_GetLocalizedString(
    "Положение значка сердца",
    "Favorite Heart Icon Position",
    "Herzsymbol-Position",
    "Położenie ikony serca"
  );
  text[1] = Str_GetLocalizedString(
    "Положение значка сердца на рамке предмета",
    "Position of the heart icon on item frame",
    "Position des Herzsymbols am Gegenstandsrahmen",
    "Położenie ikony serca na ramce przedmiotu"
  );
};

instance MenuItem_Opt_CatInv_IconCorner_Choice(C_MENU_ITEM_DEF)
{
  C_MENUITEM_CHOICE_BASE();
  posy += Menu_DY * CurrentMenuItem_PY;

  onchgsetoption        = "FavoriteIconCorner";
  onchgsetoptionsection = "CatInv";
  text[0]               = Str_GetLocalizedString(
    "Л-Низ|П-Низ|Л-Верх|П-Верх",
    "Bottom-Left|Bottom-Right|Top-Left|Top-Right",
    "Unten-Links|Unten-Rechts|Oben-Links|Oben-Rechts",
    "Lewy-Dolny|Prawy-Dolny|Lewy-Górny|Prawy-Górny"
  );
};

// ====== 2. Favorite Icon Size ======
instance MenuItem_Opt_CatInv_IconSize(C_MENU_ITEM)
{
  CurrentMenuItem_PY = 2;
  C_MENU_ITEM_TEXT_BASE();
  fontname = FontSmall;
  posy += Menu_DY * CurrentMenuItem_PY + Text_DY;

  text[0] = Str_GetLocalizedString(
    "Размер значка сердца",
    "Favorite Heart Icon Size",
    "Herzsymbol-Größe",
    "Rozmiar ikony serca"
  );
  text[1] = Str_GetLocalizedString(
    "Размер значка сердца на рамке предмета",
    "Size of the heart icon on item frame",
    "Größe des Herzsymbols am Gegenstandsrahmen",
    "Rozmiar ikony serca na ramce przedmiotu"
  );
};

instance MenuItem_Opt_CatInv_IconSize_Choice(C_MENU_ITEM_DEF)
{
  C_MENUITEM_CHOICE_BASE();
  posy += Menu_DY * CurrentMenuItem_PY;

  onchgsetoption        = "FavoriteIconSize";
  onchgsetoptionsection = "CatInv";
  text[0]               = "100%|150%|200%";
};

// ====== 3. Category Wrap-Around ======
instance MenuItem_Opt_CatInv_ChangeOnLast(C_MENU_ITEM)
{
  CurrentMenuItem_PY = 3;
  C_MENU_ITEM_TEXT_BASE();
  fontname = FontSmall;
  posy += Menu_DY * CurrentMenuItem_PY + Text_DY;

  text[0] = Str_GetLocalizedString(
    "Зацикливание категорий",
    "Category Wrap-Around",
    "Kategorien-Umlauf",
    "Przewijanie kategorii"
  );
  text[1] = Str_GetLocalizedString(
    "После последней категории переходить к первой (и наоборот)",
    "After last category, jump to first (and vice versa)",
    "Nach der letzten Kategorie zur ersten springen (und umgekehrt)",
    "Po ostatniej kategorii przejdź do pierwszej (i odwrotnie)"
  );
};

instance MenuItem_Opt_CatInv_ChangeOnLast_Choice(C_MENU_ITEM_DEF)
{
  C_MENUITEM_CHOICE_BASE();
  posy += Menu_DY * CurrentMenuItem_PY;

  onchgsetoption        = "invCatChangeOnLast";
  onchgsetoptionsection = "CatInv";
  text[0]               = Str_GetLocalizedString(
    "Выкл|Вкл",
    "Off|On",
    "Aus|An",
    "Wył|Wł"
  );
};

// ====== 4. Hide Favorites Category ======
instance MenuItem_Opt_CatInv_HideFavorites(C_MENU_ITEM)
{
  CurrentMenuItem_PY = 4;
  C_MENU_ITEM_TEXT_BASE();
  fontname = FontSmall;
  posy += Menu_DY * CurrentMenuItem_PY + Text_DY;

  text[0] = Str_GetLocalizedString(
    "Скрыть категорию Избранное",
    "Hide Favorites Category",
    "Favoriten-Kategorie ausblenden",
    "Ukryj kategorię Ulubione"
  );
  text[1] = Str_GetLocalizedString(
    "Категория Избранное доступна только по SHIFT+H",
    "Favorites category accessible only via SHIFT+H",
    "Favoriten-Kategorie nur über SHIFT+H erreichbar",
    "Kategoria Ulubione dostępna tylko przez SHIFT+H"
  );
};

instance MenuItem_Opt_CatInv_HideFavorites_Choice(C_MENU_ITEM_DEF)
{
  C_MENUITEM_CHOICE_BASE();
  posy += Menu_DY * CurrentMenuItem_PY;

  onchgsetoption        = "invCatHideFavorites";
  onchgsetoptionsection = "CatInv";
  text[0]               = Str_GetLocalizedString(
    "Выкл|Вкл",
    "Off|On",
    "Aus|An",
    "Wył|Wł"
  );
};

// ====== 5. Hide Recent Category ======
instance MenuItem_Opt_CatInv_HideRecent(C_MENU_ITEM)
{
  CurrentMenuItem_PY = 5;
  C_MENU_ITEM_TEXT_BASE();
  fontname = FontSmall;
  posy += Menu_DY * CurrentMenuItem_PY + Text_DY;

  text[0] = Str_GetLocalizedString(
    "Скрыть категорию Новое",
    "Hide Recent Category",
    "Neueste-Kategorie ausblenden",
    "Ukryj kategorię Najnowsze"
  );
  text[1] = Str_GetLocalizedString(
    "Категория Новое доступна только по SHIFT+R",
    "Recent category accessible only via SHIFT+R",
    "Neueste-Kategorie nur über SHIFT+R erreichbar",
    "Kategoria Najnowsze dostępna tylko przez SHIFT+R"
  );
};

instance MenuItem_Opt_CatInv_HideRecent_Choice(C_MENU_ITEM_DEF)
{
  C_MENUITEM_CHOICE_BASE();
  posy += Menu_DY * CurrentMenuItem_PY;

  onchgsetoption        = "invCatHideRecent";
  onchgsetoptionsection = "CatInv";
  text[0]               = Str_GetLocalizedString(
    "Выкл|Вкл",
    "Off|On",
    "Aus|An",
    "Wył|Wł"
  );
};

// ====== 6. Toggle Favorite Key ======
instance MenuItem_Opt_CatInv_KeyToggleFavorite(C_MENU_ITEM)
{
  CurrentMenuItem_PY = 6;
  C_MENU_ITEM_TEXT_BASE();
  fontname = FontSmall;
  posy += Menu_DY * CurrentMenuItem_PY + Text_DY;
  flags = flags | IT_SELECTABLE;

  text[0] = Str_GetLocalizedString(
    "Клавиша добавления в Избранное",
    "Toggle Favorite Key",
    "Favoriten-Taste",
    "Klawisz Ulubione"
  );
  text[1] = Str_GetLocalizedString(
    "Клавиша для добавления/удаления предмета из Избранного",
    "Key to add/remove item from Favorites",
    "Taste zum Hinzufügen/Entfernen von Gegenständen zu Favoriten",
    "Klawisz do dodawania/usuwania przedmiotu z Ulubionych"
  );
};

instance MenuItem_Opt_CatInv_KeyToggleFavorite_Choice(C_MENU_ITEM_DEF)
{
  C_MENUITEM_CHOICE_BASE();
  posy += Menu_DY * CurrentMenuItem_PY;

  type     = MENU_ITEM_TEXT;
  text[0]  = "=";  // Will be updated by C++ code
  text[1]  = "";   // Empty description
  // Note: IT_SELECTABLE is already removed by C_MENUITEM_CHOICE_BASE(), don't add it back
};

// ====== 7. Open Project Page ======
instance MenuItem_Opt_CATINV_Open_Link(C_MENU_ITEM)
{
  MenuItem_Opt_Open_Link();
};
