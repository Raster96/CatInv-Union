#pragma once
#include <string>

namespace GOTHIC_ENGINE {
    class CatInvCore {
    public:
        static int activeCategory;
        static zCView* categoryView;
        static bool initialized;
        static zCListSort<oCItem>* backupListBySide[2];
        static oCItemContainer* containerBySide[2];
        static zCListSort<oCItem>* filteredListBySide[2];
        static bool hooksActive;
        
        // Search functionality
        static bool searchActive;
        static bool searchInputActive;  // True when typing, false after Enter
        static std::wstring searchText;
        static zCView* searchView;
        static int previousCategory;

        static int GetCategoryID(int offset);
        static bool SupportCategories(oCItemContainer* container);
        static bool SetCategory(int category);
        static bool ShiftCategory(int offset);
        static void SetCategoryFirst();
        static void SetCategoryLast();

        static bool ItemMatchesCategory(oCItem* item, int category);
        static void ResetContainer(oCItemContainer* container);
        static void FilterContainerByCategory(oCItemContainer* container);

        static void OnContainerOpen(oCItemContainer* container);
        static void OnContainerClose(oCItemContainer* container);
        static void UpdateAllContainers();

        static void DrawCategory(oCItemContainer* container);
        static zSTRING GetCategoryName(int categoryID);
        
        // Search functions
        static void ActivateSearch();
        static void DeactivateSearch();
        static void UpdateSearchText(char c);
        static void RemoveLastSearchChar();
        static bool ItemMatchesSearch(oCItem* item);
        static void FilterContainerBySearch(oCItemContainer* container);
        static void DrawSearchBox(oCItemContainer* container);

        static bool IsWorldReady();

        static void HandleCategorySwitch(oCItemContainer* container, bool forward);
        static void HandleKeyEvent(oCItemContainer* container, int key);
        static bool SwitchContainer(oCItemContainer* container);

        static void SetSelectionFirst(oCItemContainer* container);
        static void SetSelectionLast(oCItemContainer* container);
        static void ResetOffset(oCItemContainer* container);

        static void Init();
    };
}
