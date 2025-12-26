#pragma once

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
