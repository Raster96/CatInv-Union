#pragma once
#include <string>
#include <set>
#include <map>
#include <deque>
#include <unordered_map>
#include <unordered_set>

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
        static bool isEquipping;  // Flag to prevent adding recent items during equip/unequip operations
        static std::set<int> recentlyRemovedInstances;  // Track instances recently removed (used items)
        static int usedItemInstanz;  // Track the specific item instance being used (0 = none)
        static bool batchUpdatePending;  // Flag to defer updates during batch operations
        
        // Search functionality
        static bool searchActive;
        static bool searchInputActive;  // True when typing, false after Enter
        static std::wstring searchText;
        static zCView* searchView;
        static int previousCategory;
        
        // Sort functionality
        enum SortMode {
            SORT_NONE = 0,
            SORT_PRICE_DESC = 1,
            SORT_PRICE_ASC = 2,
            SORT_NAME_AZ = 3,
            SORT_NAME_ZA = 4
        };
        static int activeSortMode;
        static zCView* sortView;
        
        // Favorites functionality
        static std::set<int> favoriteItems;  // Set of favorite item instance IDs
        
        // Recent items functionality (chronological order, newest first)
        static std::deque<int> recentItems;        // Deque of recent item instance IDs (max 10)
        static std::unordered_map<int, int> inventorySnapshot; // For tracking new items (instanz -> amount)
        static std::unordered_set<oCItem*> recentItemPointers; // Runtime pointers to recent items (rebuilt after load)
        static std::deque<int> pickupQueue; // Queue of items picked up from ground (before inventory open)

        static int GetCategoryID(int offset);
        static bool SupportCategories(oCItemContainer* container);
        static bool SetCategory(int category);
        static bool ShiftCategory(int offset);
        static void SetCategoryFirst();
        static void SetCategoryLast();
        static void JumpToItemCategory();  // Jump from virtual category (Search/Favorites/Recent) to real category

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
        
        // Sort functions
        static zSTRING GetSortModeName();
        static void SortContainer(oCItemContainer* container);
        static void DrawSortMode(oCItemContainer* container);
        
        // Favorites functions
        static void ToggleFavorite(oCItem* item);
        static bool IsFavorite(oCItem* item);
        
        // Recent items functions
        static void AddRecentItem(int instanz);
        static void RemoveRecentItem(int instanz);
        static bool IsRecent(oCItem* item);
        static void ClearRecentItems();
        static void TrimRecentItems(); // Trim list to MaxRecentItems size
        static void RebuildRecentPointers(); // Rebuild pointer set after game load
        
        // Inventory snapshot functions (for detecting new items)
        static void TakeInventorySnapshot();
        static void DetectNewItems();
        
        // Save/Load functions
        static void SaveFavorites(int slotId);
        static void LoadFavorites(int slotId);
        static void ClearFavorites();
        static void SaveRecentItems(int slotId);

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
