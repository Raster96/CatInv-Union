#include "UnionAfx.h"
#include <algorithm>
#include <vector>
#include <map>
#include <unordered_map>
#include <climits>

namespace GOTHIC_ENGINE {
    int CatInvCore::activeCategory = 0;
    zCView* CatInvCore::categoryView = nullptr;
    bool CatInvCore::initialized = false;
    zCListSort<oCItem>* CatInvCore::backupListBySide[2] = { nullptr, nullptr };
    oCItemContainer* CatInvCore::containerBySide[2] = { nullptr, nullptr };
    zCListSort<oCItem>* CatInvCore::filteredListBySide[2] = { nullptr, nullptr };
    bool CatInvCore::hooksActive = false;
    bool CatInvCore::isEquipping = false;
    std::set<int> CatInvCore::recentlyRemovedInstances;
    int CatInvCore::usedItemInstanz = 0;
    bool CatInvCore::batchUpdatePending = false;
    bool CatInvCore::searchActive = false;
    bool CatInvCore::searchInputActive = false;
    std::wstring CatInvCore::searchText = L"";
    zCView* CatInvCore::searchView = nullptr;
    int CatInvCore::previousCategory = 0;
    int CatInvCore::activeSortMode = 0;
    zCView* CatInvCore::sortView = nullptr;
    std::set<int> CatInvCore::favoriteItems;
    std::deque<int> CatInvCore::recentItems;
    std::unordered_map<int, int> CatInvCore::inventorySnapshot;
    std::unordered_set<oCItem*> CatInvCore::recentItemPointers;
    std::deque<int> CatInvCore::pickupQueue;

    int CatInvCore::GetCategoryID(int offset) {
        if (offset >= 0 && offset < INV_CAT_MAX) {
            return offset;
        }
        return 0;
    }

    bool CatInvCore::SupportCategories(oCItemContainer* container) {
        if (!container) return false;
        
        if (container->right) return true;
        
        if (dynamic_cast<oCStealContainer*>(container)) return true;
        if (dynamic_cast<oCNpcContainer*>(container)) return true;
        
        return true;
    }

    zSTRING CatInvCore::GetCategoryName(int categoryID) {
        if (categoryID < 0 || categoryID >= INV_CAT_MAX) {
            return zSTRING("");
        }
        
        if (categoryID == INV_CAT_FAVORITES) {
            return zSTRING(GetFavoritesText());
        }
        
        if (categoryID == INV_CAT_RECENT) {
            return zSTRING(GetRecentText());
        }
        
        zCPar_Symbol* txtInvCatSymbol = nullptr;
        if (parser) {
            txtInvCatSymbol = parser->GetSymbol("TXT_INV_CAT");
        }
                
        int txtInvCatIndex = categoryID - 2; // -2 because we now have Favorites and Recent before ALL
        
        if (txtInvCatSymbol && txtInvCatIndex >= 0 && txtInvCatIndex < 9) {
            if (txtInvCatSymbol->type == zPAR_TYPE_STRING && txtInvCatSymbol->ele > (unsigned int)txtInvCatIndex) {
                if (txtInvCatSymbol->stringdata && txtInvCatSymbol->stringdata[txtInvCatIndex].Length() > 0) {
                    return txtInvCatSymbol->stringdata[txtInvCatIndex];
                }
            }
        }
        
        return zSTRING(DEFAULT_CATEGORY_NAMES[categoryID]);
    }

    bool CatInvCore::IsWorldReady() {
        if (!ogame) return false;
        if (!screen || !zinput) return false;
        return true;
    }

    bool CatInvCore::ItemMatchesCategory(oCItem* item, int category) {
        if (!item) return false;
        
        if (category == INV_CAT_FAVORITES) {
            return IsFavorite(item);
        }
        
        if (category == INV_CAT_RECENT) {
            return IsRecent(item);
        }
        
        if (category == INV_CAT_ALL) return true;

        int catID = GetCategoryID(category);
        if (catID >= 0 && catID < INV_CAT_MAX) {
            return (item->mainflag & INV_CAT_GROUPS[catID]) != 0;
        }
        return false;
    }

    bool CatInvCore::SetCategory(int category) {
        int newCategory = category;
        
        int minCat = INV_CAT_FAVORITES;
        int maxCat = INV_CAT_MAX - 1;
        
        if (newCategory < minCat) newCategory = minCat;
        if (newCategory > maxCat) newCategory = maxCat;

        if (newCategory == activeCategory) return false;

        activeCategory = newCategory;
        
        if (searchActive) {
            DeactivateSearch();
        }
        
        UpdateAllContainers();
        
        return true;
    }

    bool CatInvCore::ShiftCategory(int offset) {
        if (searchActive) {
            DeactivateSearch();
        }
        
        struct CategoryItem {
            int catID;
            int order;
        };
        CategoryItem availableCategories[INV_CAT_MAX];
        int availableCount = 0;
        
        for (int catID = INV_CAT_FAVORITES; catID < INV_CAT_MAX; catID++) {
            if (CatInvOptions::invCatOrder[catID] == -1) continue;
            
            if (catID == INV_CAT_FAVORITES && CatInvOptions::HideFavorites) continue;
            if (catID == INV_CAT_RECENT && CatInvOptions::HideRecent) continue;
            
            availableCategories[availableCount].catID = catID;
            availableCategories[availableCount].order = CatInvOptions::invCatOrder[catID];
            availableCount++;
        }
        
        for (int i = 0; i < availableCount - 1; i++) {
            for (int j = i + 1; j < availableCount; j++) {
                if (availableCategories[j].order < availableCategories[i].order) {
                    CategoryItem temp = availableCategories[i];
                    availableCategories[i] = availableCategories[j];
                    availableCategories[j] = temp;
                }
            }
        }
        
        DEV_LOG("ShiftCategory: offset=" << offset << " current=" << activeCategory << " sorted order: ");
        for (int i = 0; i < availableCount; i++) {
            DEV_LOG(availableCategories[i].catID << "(" << availableCategories[i].order << ") ");
        }
        DEV_LOG(endl);
        
        int currentIndex = -1;
        for (int i = 0; i < availableCount; i++) {
            if (availableCategories[i].catID == activeCategory) {
                currentIndex = i;
                break;
            }
        }
        
        if (currentIndex == -1) {
            DEV_LOG("ShiftCategory: current not found, going to first: " << availableCategories[0].catID << endl);
            return SetCategory(availableCategories[0].catID);
        }
        
        int nextIndex = currentIndex + offset;
        
        if (CatInvOptions::ChangeOnLast) {
            if (nextIndex >= availableCount) {
                nextIndex = 0;
            } else if (nextIndex < 0) {
                nextIndex = availableCount - 1;
            }
        } else {
            if (nextIndex >= availableCount) {
                nextIndex = availableCount - 1;
            } else if (nextIndex < 0) {
                nextIndex = 0;
            }
        }
        
        DEV_LOG("ShiftCategory: currentIdx=" << currentIndex << " nextIdx=" << nextIndex << " -> category " << availableCategories[nextIndex].catID << endl);
        
        return SetCategory(availableCategories[nextIndex].catID);
    }

    void CatInvCore::SetCategoryFirst() {
        SetCategory(INV_CAT_ALL);
    }

    void CatInvCore::SetCategoryLast() {
        int lastCat = INV_CAT_MISC;
        int maxOrder = -1;
        
        for (int catID = INV_CAT_FAVORITES; catID < INV_CAT_MAX; catID++) {
            if (CatInvOptions::invCatOrder[catID] == -1) continue;
            
            if (catID == INV_CAT_FAVORITES && CatInvOptions::HideFavorites) continue;
            if (catID == INV_CAT_RECENT && CatInvOptions::HideRecent) continue;
            
            if (CatInvOptions::invCatOrder[catID] > maxOrder) {
                maxOrder = CatInvOptions::invCatOrder[catID];
                lastCat = catID;
            }
        }
        
        SetCategory(lastCat);
    }

    void CatInvCore::ResetOffset(oCItemContainer* container) {
        if (!container) return;
        container->offset = 0;
    }

    void CatInvCore::SetSelectionFirst(oCItemContainer* container) {
        if (!container) return;
        container->offset = 0;
        container->selectedItem = 0;
        container->prepared = 0;
        oCItemContainer::Container_PrepareDraw();
    }

    void CatInvCore::SetSelectionLast(oCItemContainer* container) {
        if (!container) return;
        
        int numItems = 0;
        if (container->contents) {
            zCListSort<oCItem>* list = container->contents->next;
            while (list) {
                numItems++;
                list = list->next;
            }
        }
        
        if (numItems <= 0) return;
        
        int maxCols = container->maxSlotsCol;
        int maxRows = container->maxSlotsRow;
        int visibleSlots = maxCols * maxRows;
        
        container->selectedItem = numItems - 1;
        
        if (numItems > visibleSlots) {
            int lastItemRow = (numItems - 1) / maxCols;
            int firstVisibleRow = lastItemRow - maxRows + 1;
            if (firstVisibleRow < 0) firstVisibleRow = 0;
            container->offset = firstVisibleRow * maxCols;
        } else {
            container->offset = 0;
        }
        
        container->prepared = 0;
        oCItemContainer::Container_PrepareDraw();
    }

    void CatInvCore::ResetContainer(oCItemContainer* container) {
        if (!container) return;
        int side = container->right ? 1 : 0;
        if (containerBySide[side] == container && backupListBySide[side]) {
            container->contents = backupListBySide[side];
            backupListBySide[side] = nullptr;
            containerBySide[side] = nullptr;
        }
    }

    void CatInvCore::FilterContainerByCategory(oCItemContainer* container) {
        if (!container) return;
        if (!container->contents) return;
        int side = container->right ? 1 : 0;
        
        if (searchActive) {
            FilterContainerBySearch(container);
            return;
        }
        
        if (activeCategory == INV_CAT_ALL) {
            if (!backupListBySide[side] || containerBySide[side] != container) {
                if (containerBySide[side] && backupListBySide[side]) {
                    containerBySide[side]->contents = backupListBySide[side];
                }
                backupListBySide[side] = container->contents;
                containerBySide[side] = container;
            }
            
            if (activeSortMode != SORT_NONE && backupListBySide[side]) {
                    if (!filteredListBySide[side]) {
                        filteredListBySide[side] = new zCListSort<oCItem>();
                    }
                    filteredListBySide[side]->next = nullptr;
                    filteredListBySide[side]->data = nullptr;
                    
                    if (backupListBySide[side] && backupListBySide[side]->Compare) {
                        filteredListBySide[side]->Compare = backupListBySide[side]->Compare;
                    }
                    
                    zCListSort<oCItem>* tail = filteredListBySide[side];
                    zCListSort<oCItem>* node = backupListBySide[side]->next;
                    while (node) {
                        if (node->data) {
                            zCListSort<oCItem>* newNode = new zCListSort<oCItem>();
                            newNode->data = node->data;
                            newNode->next = nullptr;
                            tail->next = newNode;
                            tail = newNode;
                        }
                        node = node->next;
                    }
                    
                    container->contents = filteredListBySide[side];
                    SortContainer(container);
                    container->CheckSelectedItem();
                    container->prepared = 0;
                    oCItemContainer::Container_PrepareDraw();
                } else {
                    container->contents = backupListBySide[side];
                    container->CheckSelectedItem();
                    container->prepared = 0;
                    oCItemContainer::Container_PrepareDraw();
                }
            return;
        }
        
        if (!SupportCategories(container)) return;

        if (containerBySide[side] != container) {
            if (containerBySide[side] && backupListBySide[side]) {
                containerBySide[side]->contents = backupListBySide[side];
            }
            backupListBySide[side] = container->contents;
            containerBySide[side] = container;
        }

        if (!backupListBySide[side]) {
            backupListBySide[side] = container->contents;
            containerBySide[side] = container;
        }

        if (!filteredListBySide[side]) {
            filteredListBySide[side] = new zCListSort<oCItem>();
        }
        filteredListBySide[side]->next = nullptr;
        filteredListBySide[side]->data = nullptr;
        
        if (backupListBySide[side] && backupListBySide[side]->Compare) {
            filteredListBySide[side]->Compare = backupListBySide[side]->Compare;
        }
        
        zCListSort<oCItem>* tail = filteredListBySide[side];

        int catID = GetCategoryID(activeCategory);
        if (catID < 0 || catID >= INV_CAT_MAX) {
            return;
        }

        if (catID == INV_CAT_FAVORITES) {
            zCListSort<oCItem>* node = backupListBySide[side]->next;
            while (node) {
                if (node->data) {
                    oCItem* item = node->data;
                    if (IsFavorite(item)) {
                        zCListSort<oCItem>* newNode = new zCListSort<oCItem>();
                        newNode->data = item;
                        newNode->next = nullptr;
                        tail->next = newNode;
                        tail = newNode;
                    }
                }
                node = node->next;
            }
        }
        else if (catID == INV_CAT_RECENT) {
            if (side == 0) {
                zCListSort<oCItem>* node = backupListBySide[side]->next;
                while (node) {
                    if (node->data) {
                        zCListSort<oCItem>* newNode = new zCListSort<oCItem>();
                        newNode->data = node->data;
                        newNode->next = nullptr;
                        tail->next = newNode;
                        tail = newNode;
                    }
                    node = node->next;
                }
            }
            else {
                std::unordered_map<int, std::vector<oCItem*>> availableItems;
                zCListSort<oCItem>* node = backupListBySide[side]->next;
                while (node) {
                    if (node->data) {
                        availableItems[node->data->instanz].push_back(node->data);
                    }
                    node = node->next;
                }
                
                std::unordered_map<int, int> usedCount;
                
                for (int instanz : recentItems) {
                    if (availableItems.find(instanz) != availableItems.end()) {
                        int idx = usedCount[instanz];
                        if (idx < (int)availableItems[instanz].size()) {
                            oCItem* item = availableItems[instanz][idx];
                            
                            if (recentItemPointers.find(item) != recentItemPointers.end()) {
                                zCListSort<oCItem>* newNode = new zCListSort<oCItem>();
                                newNode->data = item;
                                newNode->next = nullptr;
                                tail->next = newNode;
                                tail = newNode;
                            }
                            
                            usedCount[instanz]++;
                        }
                    }
                }
            }
        }
        else {
            int categoryMask = INV_CAT_GROUPS[catID];

            int itemCount = 0;
            int matchedCount = 0;
            zCListSort<oCItem>* node = backupListBySide[side]->next;
            while (node) {
                if (node->data) {
                    oCItem* item = node->data;
                    itemCount++;
                    if (item->mainflag & categoryMask) {
                        zCListSort<oCItem>* newNode = new zCListSort<oCItem>();
                        newNode->data = item;
                        newNode->next = nullptr;
                        tail->next = newNode;
                        tail = newNode;
                        matchedCount++;
                    }
                }
                node = node->next;
            }
        }

        container->contents = filteredListBySide[side];
        if (activeSortMode != SORT_NONE) {
            SortContainer(container);
        }
        
        ResetOffset(container);
        container->CheckSelectedItem();
        container->prepared = 0;
        oCItemContainer::Container_PrepareDraw();
    }

    void CatInvCore::UpdateAllContainers() {
        for (int side = 0; side < 2; ++side) {
            if (containerBySide[side]) {
                if (SupportCategories(containerBySide[side])) {
                    FilterContainerByCategory(containerBySide[side]);
                }
            }
        }
    }

    void CatInvCore::OnContainerOpen(oCItemContainer* container) {
        if (!container) return;
        
        if (!container->contents) return;

        int side = container->right ? 1 : 0;
        
        if (searchActive) {
            DeactivateSearch();
        }
        
        bool isPlayerInventory = (container->right != 0);
        bool isDeadBody = (dynamic_cast<oCNpcContainer*>(container) != nullptr);
        bool isChest = (!dynamic_cast<oCNpcInventory*>(container) && 
                       !dynamic_cast<oCStealContainer*>(container) && 
                       !dynamic_cast<oCNpcContainer*>(container));
        
        if (InDevelopment) {
            cmd << "OnContainerOpen: ";
            if (dynamic_cast<oCNpcInventory*>(container)) cmd << "oCNpcInventory";
            else if (dynamic_cast<oCNpcContainer*>(container)) cmd << "oCNpcContainer(DEAD)";
            else if (dynamic_cast<oCStealContainer*>(container)) cmd << "oCStealContainer";
            else cmd << "CHEST(base oCItemContainer)";
            cmd << " Side=" << (container->right ? "RIGHT" : "LEFT") << endl;
        }
        
        if (isPlayerInventory && player && dynamic_cast<oCNpcInventory*>(container) && 
            static_cast<oCNpcInventory*>(container)->owner == player) {
            
            if (recentItemPointers.empty() && !recentItems.empty()) {
                DEV_LOG("OnContainerOpen: Rebuilding recent pointers (first open after load)" << endl);
                RebuildRecentPointers();
            }
        }
        
        if (isPlayerInventory && player && dynamic_cast<oCNpcInventory*>(container) && 
            static_cast<oCNpcInventory*>(container)->owner == player) {
            DEV_LOG( "OnContainerOpen: Detecting new items before filtering..." << endl);
            DetectNewItems();
        }
        
        if (containerBySide[side] != container) {
            if (containerBySide[side] && backupListBySide[side]) {
                containerBySide[side]->contents = backupListBySide[side];
            }
            backupListBySide[side] = container->contents;
            containerBySide[side] = container;
        } else if (isChest) {
            backupListBySide[side] = container->contents;
        }

        if (!isPlayerInventory) {
            SetCategory(INV_CAT_ALL);
            activeSortMode = SORT_NONE;
            SetSelectionFirst(container);
        }

        if (activeCategory != INV_CAT_ALL || activeSortMode != SORT_NONE) {
            FilterContainerByCategory(container);
        }
    }

    void CatInvCore::OnContainerClose(oCItemContainer* container) {
    }

    void CatInvCore::DrawCategory(oCItemContainer* container) {
        if (!container) return;
        if (!IsWorldReady()) return;
        if (activeCategory == INV_CAT_ALL && !searchActive) return;
        if (!SupportCategories(container)) return;
        if (!container->viewTitle) return;
        if (!container->contents) return;

        if (searchActive) {
            DrawSearchBox(container);
            return;
        }

        if (categoryView == NULL) {
            categoryView = new zCView(0, 0, 8192, 8192);
        }

        zSTRING categoryText = GetCategoryName(GetCategoryID(activeCategory));
        if (categoryText.IsEmpty()) return;

        zCView* viewTitle = container->viewTitle;

        int defaultWidth = *(int*)DEFAULT_WIDTH_ADDR;
        int width = 2 * defaultWidth;
        int height = viewTitle->vsizey;

        int posY = viewTitle->vposy;
        int posX;
        if (container->right) {
            posX = viewTitle->vposx - width + 1;
        } else {
            posX = viewTitle->vposx + viewTitle->vsizex - 1;
        }

        categoryView->SetPos(posX, posY);
        categoryView->SetSize(width, height);
        
        zCTexture* backTex = nullptr;
        if (container->viewBack && container->viewBack->backTex) {
            backTex = container->viewBack->backTex;
        }
        
        zCTexture* titleTex = nullptr;
        if (container->viewTitle && container->viewTitle->backTex) {
            titleTex = container->viewTitle->backTex;
        }
        
        if (screen) {
            screen->InsertItem(categoryView, 0);
        }
        
        if (backTex) {
            categoryView->InsertBack(backTex);
        }
        categoryView->SetTransparency(255);
        categoryView->ClrPrintwin();
        categoryView->Blit();
        
        if (titleTex) {
            categoryView->InsertBack(titleTex);
        }
        
        categoryView->PrintCXY(categoryText);
        categoryView->Blit();
        
        if (screen) {
            screen->RemoveItem(categoryView);
        }
    }

    void CatInvCore::HandleCategorySwitch(oCItemContainer* container, bool forward) {
        if (!container) return;
        if (!SupportCategories(container)) return;

        if (zinput->KeyPressed(KEY_LSHIFT) || zinput->KeyPressed(KEY_RSHIFT)) {
            ShiftCategory(forward ? 1 : -1);
        }
    }

    void CatInvCore::HandleKeyEvent(oCItemContainer* container, int key) {
        if (!container) return;

        bool shiftPressed = zinput->KeyPressed(KEY_LSHIFT) || zinput->KeyPressed(KEY_RSHIFT);

        if (key == KEY_HOME) {
            if (shiftPressed && SupportCategories(container)) {
                SetCategoryFirst();
            } else {
                SetSelectionFirst(container);
            }
        }
        else if (key == KEY_END) {
            if (shiftPressed && SupportCategories(container)) {
                SetCategoryLast();
            } else {
                SetSelectionLast(container);
            }
        }
    }

    bool CatInvCore::SwitchContainer(oCItemContainer* container) {
        if (!container) return false;
        
        int direction = container->right ? -1 : 1;
        
        int result = container->ActivateNextContainer(direction);
        return result != 0;
    }

    HOOK Hook_oCItemContainer_NextItem PATCH(&oCItemContainer::NextItem, &oCItemContainer::NextItem_Union);
    void oCItemContainer::NextItem_Union() {
        if (CatInvCore::IsWorldReady() && zinput && (zinput->KeyPressed(KEY_LSHIFT) || zinput->KeyPressed(KEY_RSHIFT))) {
            if (CatInvCore::SupportCategories(this)) {
                CatInvCore::ShiftCategory(1);
                return;
            }
        }
        THISCALL(Hook_oCItemContainer_NextItem)();
    }

    HOOK Hook_oCItemContainer_PrevItem PATCH(&oCItemContainer::PrevItem, &oCItemContainer::PrevItem_Union);
    void oCItemContainer::PrevItem_Union() {
        if (CatInvCore::IsWorldReady() && zinput && (zinput->KeyPressed(KEY_LSHIFT) || zinput->KeyPressed(KEY_RSHIFT))) {
            if (CatInvCore::SupportCategories(this)) {
                CatInvCore::ShiftCategory(-1);
                return;
            }
        }
        THISCALL(Hook_oCItemContainer_PrevItem)();
    }

    HOOK Hook_oCItemContainer_NextItemLine PATCH(&oCItemContainer::NextItemLine, &oCItemContainer::NextItemLine_Union);
    void oCItemContainer::NextItemLine_Union() {
        if (CatInvCore::IsWorldReady() && zinput && (zinput->KeyPressed(KEY_LSHIFT) || zinput->KeyPressed(KEY_RSHIFT))) {
            if (CatInvCore::SupportCategories(this)) {
                if (CatInvOptions::ChangeOnLast) {
                    CatInvCore::activeSortMode = (CatInvCore::activeSortMode + 1) % 5;
                } else {
                    if (CatInvCore::activeSortMode < 4) {
                        CatInvCore::activeSortMode++;
                    }
                }
                CatInvCore::UpdateAllContainers();
                return;
            }
        }
        
        bool isTrading = false;
        if (CatInvCore::containerBySide[0] != nullptr && CatInvCore::containerBySide[1] != nullptr) {
            if (dynamic_cast<oCStealContainer*>(CatInvCore::containerBySide[0])) {
                isTrading = true;
            }
        }
        
        if (!isTrading) {
            int numItems = 0;
            if (this->contents) {
                zCListSort<oCItem>* list = this->contents->next;
                while (list) {
                    numItems++;
                    list = list->next;
                }
            }
            
            int maxCols = this->maxSlotsCol;
            int maxRows = this->maxSlotsRow;
            int visibleSlots = maxCols * maxRows;
            int newSelected = this->selectedItem + maxCols;
            
            if (newSelected < numItems) {
                this->selectedItem = newSelected;
                
                if (this->selectedItem >= this->offset + visibleSlots) {
                    this->offset += maxCols;
                }
                
                this->prepared = 0;
                oCItemContainer::Container_PrepareDraw();
            }
            return;
        }
        
        const int ITM_FLAG_ACTIVE = 1 << 30;
        int maxCols = this->maxSlotsCol;
        int maxRows = this->maxSlotsRow;
        int visibleSlots = maxCols * maxRows;
        
        int currentCol = this->selectedItem % maxCols;
        
        int targetIndex = this->selectedItem + maxCols;
        int itemIndex = 0;
        bool found = false;
        
        if (this->contents) {
            zCListSort<oCItem>* list = this->contents->next;
            while (list) {
                if (itemIndex >= targetIndex) {
                    int itemCol = itemIndex % maxCols;
                    if (itemCol == currentCol && list->data && !(list->data->flags & ITM_FLAG_ACTIVE)) {
                        this->selectedItem = itemIndex;
                        found = true;
                        break;
                    }
                }
                itemIndex++;
                list = list->next;
            }
        }
        
        if (found) {
            if (this->selectedItem >= this->offset + visibleSlots) {
                this->offset += maxCols;
            }
            
            this->prepared = 0;
            oCItemContainer::Container_PrepareDraw();
        }
    }

    HOOK Hook_oCItemContainer_PrevItemLine PATCH(&oCItemContainer::PrevItemLine, &oCItemContainer::PrevItemLine_Union);
    void oCItemContainer::PrevItemLine_Union() {
        if (CatInvCore::IsWorldReady() && zinput && (zinput->KeyPressed(KEY_LSHIFT) || zinput->KeyPressed(KEY_RSHIFT))) {
            if (CatInvCore::SupportCategories(this)) {
                if (CatInvOptions::ChangeOnLast) {
                    CatInvCore::activeSortMode = (CatInvCore::activeSortMode + 4) % 5;
                } else {
                    if (CatInvCore::activeSortMode > 0) {
                        CatInvCore::activeSortMode--;
                    }
                }
                CatInvCore::UpdateAllContainers();
                return;
            }
        }
        
        bool isTrading = false;
        if (CatInvCore::containerBySide[0] != nullptr && CatInvCore::containerBySide[1] != nullptr) {
            if (dynamic_cast<oCStealContainer*>(CatInvCore::containerBySide[0])) {
                isTrading = true;
            }
        }
        
        if (!isTrading) {
            int maxCols = this->maxSlotsCol;
            int newSelected = this->selectedItem - maxCols;
            
            if (newSelected >= 0) {
                this->selectedItem = newSelected;
                
                if (this->selectedItem < this->offset) {
                    this->offset -= maxCols;
                    if (this->offset < 0) this->offset = 0;
                }
                
                this->prepared = 0;
                oCItemContainer::Container_PrepareDraw();
            }
            return;
        }
        
        const int ITM_FLAG_ACTIVE = 1 << 30;
        int maxCols = this->maxSlotsCol;
        
        int currentCol = this->selectedItem % maxCols;
        
        int targetIndex = this->selectedItem - maxCols;
        
        if (targetIndex < 0) {
            return;
        }
        
        std::vector<int> candidatesInColumn;
        int itemIndex = 0;
        
        if (this->contents) {
            zCListSort<oCItem>* list = this->contents->next;
            while (list && itemIndex < this->selectedItem) {
                int itemCol = itemIndex % maxCols;
                if (itemCol == currentCol && list->data && !(list->data->flags & ITM_FLAG_ACTIVE)) {
                    candidatesInColumn.push_back(itemIndex);
                }
                itemIndex++;
                list = list->next;
            }
        }
        
        if (!candidatesInColumn.empty()) {
            int newSelectedItem = candidatesInColumn.back();
            this->selectedItem = newSelectedItem;
            
            if (this->selectedItem < this->offset) {
                this->offset -= maxCols;
                if (this->offset < 0) this->offset = 0;
            }
            
            this->prepared = 0;
            oCItemContainer::Container_PrepareDraw();
        }
    }

    HOOK Hook_oCItemContainer_OpenPassive PATCH(&oCItemContainer::OpenPassive, &oCItemContainer::OpenPassive_Union);
    void oCItemContainer::OpenPassive_Union(int a, int b, int c) {
        DEV_LOG( "OpenPassive_Union: IsWorldReady=" << CatInvCore::IsWorldReady() << endl);
        
        THISCALL(Hook_oCItemContainer_OpenPassive)(a, b, c);
        
        if (CatInvCore::IsWorldReady()) {
            CatInvCore::OnContainerOpen(this);
        } else {
            DEV_LOG( "OpenPassive_Union: Skipping OnContainerOpen because IsWorldReady=false" << endl);
        }
    }
    
    HOOK Hook_oCItemContainer_Close PATCH(&oCItemContainer::Close, &oCItemContainer::Close_Union);
    void oCItemContainer::Close_Union() {
        if (CatInvCore::IsWorldReady()) {
            if (player && dynamic_cast<oCNpcInventory*>(this) && static_cast<oCNpcInventory*>(this)->owner == player) {
                CatInvCore::DetectNewItems();
            }
            
            CatInvCore::OnContainerClose(this);
            CatInvCore::ResetContainer(this);
        }
        THISCALL(Hook_oCItemContainer_Close)();
    }
    
    HOOK Hook_oCItemContainer_DrawCategory PATCH(&oCItemContainer::DrawCategory, &oCItemContainer::DrawCategory_Union);
    void oCItemContainer::DrawCategory_Union() {
        THISCALL(Hook_oCItemContainer_DrawCategory)();
        if (CatInvCore::IsWorldReady()) {
            if (CatInvCore::batchUpdatePending) {
                DEV_LOG( "DrawCategory: Processing batch update" << endl);
                CatInvCore::DetectNewItems();
                
                if (CatInvCore::activeCategory == INV_CAT_RECENT) {
                    CatInvCore::UpdateAllContainers();
                }
                
                CatInvCore::batchUpdatePending = false;
            }
            
            bool needsFilter = CatInvCore::activeCategory != INV_CAT_ALL || CatInvCore::searchActive || CatInvCore::activeSortMode != CatInvCore::SORT_NONE;
            if (needsFilter) {
                CatInvCore::FilterContainerByCategory(this);
            }
            CatInvCore::DrawCategory(this);
            CatInvCore::DrawSortMode(this);
        }
    }

    HOOK Hook_oCItemContainer_CheckSelectedItem PATCH(&oCItemContainer::CheckSelectedItem, &oCItemContainer::CheckSelectedItem_Union);
    void oCItemContainer::CheckSelectedItem_Union() {
        const int ITM_FLAG_ACTIVE = 1 << 30;
        
        int numItems = 0;
        if (this->contents) {
            zCListSort<oCItem>* list = this->contents->next;
            while (list) {
                numItems++;
                list = list->next;
            }
        }
        
        if (this->selectedItem < 0) {
            this->selectedItem = 0;
        }
        if (numItems > 0 && this->selectedItem >= numItems) {
            this->selectedItem = numItems - 1;
        }
        
        bool isTrading = false;
        if (CatInvCore::containerBySide[0] != nullptr && CatInvCore::containerBySide[1] != nullptr) {
            if (dynamic_cast<oCStealContainer*>(CatInvCore::containerBySide[0])) {
                isTrading = true;
            }
        }
        
        if (isTrading && this->contents && numItems > 0) {
            int side = this->right ? 1 : 0;
            bool isFilteredList = (CatInvCore::containerBySide[side] == this && 
                                  CatInvCore::backupListBySide[side] &&
                                  this->contents != CatInvCore::backupListBySide[side]);
            bool needsFilter = CatInvCore::activeCategory != INV_CAT_ALL || CatInvCore::searchActive || CatInvCore::activeSortMode != CatInvCore::SORT_NONE;
            
            if (!needsFilter || isFilteredList) {
            int currentIndex = 0;
            zCListSort<oCItem>* list = this->contents->next;
            
            bool currentIsEquipped = false;
            zCListSort<oCItem>* checkList = this->contents->next;
            int checkIndex = 0;
            while (checkList && checkIndex <= this->selectedItem) {
                if (checkIndex == this->selectedItem && checkList->data && (checkList->data->flags & ITM_FLAG_ACTIVE)) {
                    currentIsEquipped = true;
                    break;
                }
                checkIndex++;
                checkList = checkList->next;
            }
            
            if (currentIsEquipped) {
                list = this->contents->next;
                currentIndex = 0;
                bool foundNonEquipped = false;
                while (list) {
                    if (list->data && !(list->data->flags & ITM_FLAG_ACTIVE)) {
                        this->selectedItem = currentIndex;
                        foundNonEquipped = true;
                        break;
                    }
                    currentIndex++;
                    list = list->next;
                }
                
                if (!foundNonEquipped) {
                    this->selectedItem = -1;
                    this->offset = 0;
                    return;
                }
            }
            }
        }
        
        if (this->selectedItem >= 0) {
            int maxCols = this->maxSlotsCol;
            int maxRows = this->maxSlotsRow;
            int visibleSlots = maxCols * maxRows;
            
            if (this->selectedItem < this->offset) {
                this->offset = (this->selectedItem / maxCols) * maxCols;
            }
            if (this->selectedItem >= this->offset + visibleSlots) {
                int selectedRow = this->selectedItem / maxCols;
                int firstVisibleRow = selectedRow - maxRows + 1;
                if (firstVisibleRow < 0) firstVisibleRow = 0;
                this->offset = firstVisibleRow * maxCols;
            }
            
            if (this->offset < 0) this->offset = 0;
        }
    }

    HOOK Hook_oCStealContainer_CreateList PATCH(&oCStealContainer::CreateList, &oCStealContainer::CreateList_Union);
    void oCStealContainer::CreateList_Union() {
        int side = this->right ? 1 : 0;
        
        int savedSelectedItem = this->selectedItem;
        
        if (CatInvCore::containerBySide[side] == this && CatInvCore::backupListBySide[side]) {
            this->contents = CatInvCore::backupListBySide[side];
        }
        
        THISCALL(Hook_oCStealContainer_CreateList)();
        
        this->selectedItem = savedSelectedItem;
        
        if (CatInvCore::containerBySide[side] == this) {
            CatInvCore::backupListBySide[side] = this->contents;
        }
        
        bool needsFilter = CatInvCore::activeCategory != INV_CAT_ALL || CatInvCore::searchActive || CatInvCore::activeSortMode != CatInvCore::SORT_NONE;
        if (CatInvCore::IsWorldReady() && needsFilter) {
            CatInvCore::FilterContainerByCategory(this);
        }
    }

    HOOK Hook_oCNpcContainer_CreateList PATCH(&oCNpcContainer::CreateList, &oCNpcContainer::CreateList_Union);
    void oCNpcContainer::CreateList_Union() {
        int side = this->right ? 1 : 0;
        
        int savedSelectedItem = this->selectedItem;
        
        if (InDevelopment) {
            cmd << "oCNpcContainer::CreateList Side=" << (this->right ? "RIGHT" : "LEFT");
            cmd << " Category=" << CatInvCore::activeCategory;
            cmd << " Sort=" << CatInvCore::activeSortMode << endl;
        }
        
        CatInvCore::activeCategory = INV_CAT_ALL;
        CatInvCore::activeSortMode = CatInvCore::SORT_NONE;
        
        if (CatInvCore::containerBySide[side] == this && CatInvCore::backupListBySide[side]) {
            this->contents = CatInvCore::backupListBySide[side];
        }
        
        THISCALL(Hook_oCNpcContainer_CreateList)();
        
        this->selectedItem = savedSelectedItem;
        
        if (CatInvCore::containerBySide[side] == this) {
            CatInvCore::backupListBySide[side] = this->contents;
        }
    }

    HOOK Hook_oCNpcContainer_Remove PATCH(&oCNpcContainer::Remove, &oCNpcContainer::Remove_Union);
    void oCNpcContainer::Remove_Union(oCItem* item) {
        int side = this->right ? 1 : 0;
        
        if (InDevelopment) {
            cmd << "oCNpcContainer::Remove Side=" << (this->right ? "RIGHT" : "LEFT");
            cmd << " Category=" << CatInvCore::activeCategory;
            cmd << " Sort=" << CatInvCore::activeSortMode;
            if (item) cmd << " Item=" << item->name.ToChar();
            cmd << endl;
        }
        
        if (CatInvCore::containerBySide[side] == this && CatInvCore::backupListBySide[side]) {
            this->contents = CatInvCore::backupListBySide[side];
        }
        
        THISCALL(Hook_oCNpcContainer_Remove)(item);
        
        if (CatInvCore::containerBySide[side] == this) {
            CatInvCore::backupListBySide[side] = this->contents;
            
            bool needsFilter = CatInvCore::activeCategory != INV_CAT_ALL || CatInvCore::searchActive || CatInvCore::activeSortMode != CatInvCore::SORT_NONE;
            if (needsFilter) {
                CatInvCore::FilterContainerByCategory(this);
            }
        }
        
        if (player && this->IsOpen() && CatInvCore::IsWorldReady()) {
            CatInvCore::batchUpdatePending = true;
        }
    }

    HOOK Hook_oCItemContainer_IsEmpty PATCH(&oCItemContainer::IsEmpty, &oCItemContainer::IsEmpty_Union);
    int oCItemContainer::IsEmpty_Union() {
        int side = this->right ? 1 : 0;
        
        bool isDeadBody = (dynamic_cast<oCNpcContainer*>(this) != nullptr);
        bool hasFiltering = CatInvCore::activeCategory != INV_CAT_ALL || CatInvCore::searchActive || CatInvCore::activeSortMode != CatInvCore::SORT_NONE;
        
        if (isDeadBody && hasFiltering && CatInvCore::containerBySide[side] == this && CatInvCore::backupListBySide[side]) {
            int itemCount = 0;
            zCListSort<oCItem>* node = CatInvCore::backupListBySide[side]->next;
            while (node) {
                if (node->data) itemCount++;
                node = node->next;
            }
            
            DEV_LOG( "IsEmpty check: DeadBody with filtering, backup has " << itemCount << " items" << endl);
            return (itemCount == 0) ? 1 : 0;
        }
        
        return THISCALL(Hook_oCItemContainer_IsEmpty)();
    }

    HOOK Hook_oCItemContainer_Insert PATCH(&oCItemContainer::Insert, &oCItemContainer::Insert_Union);
    oCItem* oCItemContainer::Insert_Union(oCItem* item) {
        int side = this->right ? 1 : 0;
        
        if (InDevelopment) {
            cmd << "Insert: Container=";
            if (dynamic_cast<oCNpcInventory*>(this)) cmd << "oCNpcInventory";
            else if (dynamic_cast<oCNpcContainer*>(this)) cmd << "oCNpcContainer";
            else if (dynamic_cast<oCStealContainer*>(this)) cmd << "oCStealContainer";
            else cmd << "CHEST(base)";
            cmd << " Side=" << (this->right ? "RIGHT" : "LEFT");
            if (item) cmd << " Item=" << item->name.ToChar();
            cmd << endl;
        }
        
        bool isChest = !dynamic_cast<oCNpcInventory*>(this) && 
                       !dynamic_cast<oCStealContainer*>(this) && 
                       !dynamic_cast<oCNpcContainer*>(this);
        
        bool needsFilter = CatInvCore::activeCategory != INV_CAT_ALL || CatInvCore::searchActive || CatInvCore::activeSortMode != CatInvCore::SORT_NONE;
        
        if (isChest && CatInvCore::containerBySide[side] == this && 
            CatInvCore::backupListBySide[side] && needsFilter) {
            this->contents = CatInvCore::backupListBySide[side];
        }
        
        oCItem* result = (oCItem*)THISCALL(Hook_oCItemContainer_Insert)(item);
        
        if (isChest && CatInvCore::containerBySide[side] == this) {
            CatInvCore::backupListBySide[side] = this->contents;
            
            if (needsFilter) {
                CatInvCore::FilterContainerByCategory(this);
            }
        }
        
        return result;
    }
    
    HOOK Hook_oCItemContainer_Remove PATCH(&oCItemContainer::Remove, &oCItemContainer::Remove_Union);
    void oCItemContainer::Remove_Union(oCItem* item) {
        if (InDevelopment) {
            cmd << "Remove: Container=";
            if (dynamic_cast<oCNpcInventory*>(this)) cmd << "oCNpcInventory";
            else if (dynamic_cast<oCNpcContainer*>(this)) cmd << "oCNpcContainer(DEAD)";
            else if (dynamic_cast<oCStealContainer*>(this)) cmd << "oCStealContainer";
            else cmd << "CHEST(base)";
            cmd << " Side=" << (this->right ? "RIGHT" : "LEFT");
            cmd << " Category=" << CatInvCore::activeCategory;
            cmd << " Sort=" << CatInvCore::activeSortMode;
            if (item) {
                cmd << " Item=" << item->name.ToChar();
                cmd << " Instanz=" << item->instanz;
            }
            cmd << endl;
        }
        
        int side = this->right ? 1 : 0;
        
        bool isChest = !dynamic_cast<oCNpcInventory*>(this) && 
                       !dynamic_cast<oCStealContainer*>(this) && 
                       !dynamic_cast<oCNpcContainer*>(this);
        
        bool needsFilter = CatInvCore::activeCategory != INV_CAT_ALL || CatInvCore::searchActive || CatInvCore::activeSortMode != CatInvCore::SORT_NONE;
        
        if (isChest && CatInvCore::containerBySide[side] == this && 
            CatInvCore::backupListBySide[side] && needsFilter) {
            this->contents = CatInvCore::backupListBySide[side];
        }
        
        THISCALL(Hook_oCItemContainer_Remove)(item);
        
        if (isChest && CatInvCore::containerBySide[side] == this) {
            CatInvCore::backupListBySide[side] = this->contents;
            
            if (needsFilter) {
                CatInvCore::FilterContainerByCategory(this);
            }
        }
        
        if (player && this->IsOpen() && CatInvCore::IsWorldReady()) {
            CatInvCore::batchUpdatePending = true;
        }
    }
    
    HOOK Hook_oCNpcInventory_Insert PATCH(&oCNpcInventory::Insert, &oCNpcInventory::Insert_Union);
    oCItem* oCNpcInventory::Insert_Union(oCItem* item) {
        int side = this->right ? 1 : 0;
        
        if (player && this->owner == player && item && !CatInvCore::isEquipping) {
            if (CatInvCore::usedItemInstanz != item->instanz) {
                DEV_LOG( "Insert_Union: Adding to pickupQueue instanz=" << item->instanz 
                    << " name=" << item->name.ToChar() << endl);
                CatInvCore::pickupQueue.push_back(item->instanz);
            } else {
                DEV_LOG( "Insert_Union: Skipping pickupQueue for used item instanz=" << item->instanz 
                    << " name=" << item->name.ToChar() << endl);
            }
        }
        
        zCListSort<oCItem>* savedContents = this->contents;
        bool wasFiltered = (CatInvCore::containerBySide[side] == this && 
                           CatInvCore::backupListBySide[side] &&
                           this->contents != CatInvCore::backupListBySide[side]);
        
        if (CatInvCore::containerBySide[side] == this && CatInvCore::backupListBySide[side]) {
            this->contents = CatInvCore::backupListBySide[side];
        }
        
        oCItem* result = (oCItem*)THISCALL(Hook_oCNpcInventory_Insert)(item);
        
        if (CatInvCore::containerBySide[side] == this) {
            CatInvCore::backupListBySide[side] = this->contents;
        }
        
        if (wasFiltered && savedContents) {
            this->contents = savedContents;
        }
        
        return result;
    }
    
    HOOK Hook_oCNpc_EquipItem PATCH(&oCNpc::EquipItem, &oCNpc::EquipItem_Union);
    void oCNpc::EquipItem_Union(oCItem* item) {
        CatInvCore::isEquipping = true;
        THISCALL(Hook_oCNpc_EquipItem)(item);
        CatInvCore::isEquipping = false;
    }
    
    HOOK Hook_oCNpc_UnequipItem PATCH(&oCNpc::UnequipItem, &oCNpc::UnequipItem_Union);
    void oCNpc::UnequipItem_Union(oCItem* item) {
        CatInvCore::isEquipping = true;
        THISCALL(Hook_oCNpc_UnequipItem)(item);
        CatInvCore::isEquipping = false;
    }
        
    void CatInvCore::TakeInventorySnapshot() {
        if (!inventorySnapshot.empty()) {
            DEV_LOG( "Snapshot: Already exists, skipping" << endl);
            return;
        }
        
        if (!player) return;
        
        oCNpcInventory* inv = &player->inventory2;
        if (!inv) return;
        
        if (inv->inventory.next) {
            zCListSort<oCItem>* node = inv->inventory.next;
            while (node) {
                if (node->data) {
                    inventorySnapshot[node->data->instanz] += node->data->amount;
                }
                node = node->next;
            }
        }
        
        DEV_LOG( "Snapshot: Created with " << inventorySnapshot.size() << " unique items" << endl);
    }
    
    void CatInvCore::DetectNewItems() {
        if (!player) return;
        
        oCNpcInventory* inv = &player->inventory2;
        if (!inv) return;
        
        std::unordered_map<int, int> currentInv;
        
        if (inv->inventory.next) {
            zCListSort<oCItem>* node = inv->inventory.next;
            while (node) {
                if (node->data) {
                    currentInv[node->data->instanz] += node->data->amount;
                }
                node = node->next;
            }
        }
        
        DEV_LOG( "DetectNewItems: recentItems.size BEFORE = " << recentItems.size() << endl);
        DEV_LOG( "DetectNewItems: recentItemPointers.size BEFORE = " << recentItemPointers.size() << endl);
        DEV_LOG( "DetectNewItems: currentInv has " << currentInv.size() << " unique items" << endl);
        DEV_LOG( "DetectNewItems: inventorySnapshot has " << inventorySnapshot.size() << " unique items" << endl);
        
        std::unordered_set<oCItem*> pointersToRemove;
        
        DEV_LOG( "DetectNewItems: Checking " << recentItemPointers.size() << " recent pointers..." << endl);
        for (oCItem* recentPtr : recentItemPointers) {
            bool stillExists = false;
            
            if (recentPtr->instanz == usedItemInstanz) {
                DEV_LOG( "DetectRemoved: Skipping check for used item instanz=" << recentPtr->instanz 
                    << " name=" << recentPtr->name.ToChar() << endl);
                continue;
            }
            
            if (inv->inventory.next) {
                zCListSort<oCItem>* node = inv->inventory.next;
                while (node) {
                    if (node->data == recentPtr) {
                        stillExists = true;
                        break;
                    }
                    node = node->next;
                }
            }
            
            if (!stillExists) {
                DEV_LOG( "DetectRemoved: ptr no longer in inventory, instanz=" << recentPtr->instanz 
                    << " name=" << recentPtr->name.ToChar() << endl);
                pointersToRemove.insert(recentPtr);
            } else {
                DEV_LOG( "DetectRemoved: ptr still exists, instanz=" << recentPtr->instanz 
                    << " name=" << recentPtr->name.ToChar() << endl);
            }
        }
        
        for (oCItem* ptr : pointersToRemove) {
            int instanz = ptr->instanz;
            recentItemPointers.erase(ptr);
            
            auto it = std::find(recentItems.begin(), recentItems.end(), instanz);
            if (it != recentItems.end()) {
                recentItems.erase(it);
                DEV_LOG( "DetectRemoved: removed ONE entry of instanz=" << instanz << " from recentItems" << endl);
            }
        }
        
        DEV_LOG( "DetectNewItems: removed " << pointersToRemove.size() << " recent items" << endl);
        
        DEV_LOG( "DetectNewItems: Processing pickupQueue with " << pickupQueue.size() << " items" << endl);
        
        while (!pickupQueue.empty()) {
            int instanz = pickupQueue.front();
            pickupQueue.pop_front();
            
            int currentAmount = (currentInv.find(instanz) != currentInv.end()) ? currentInv[instanz] : 0;
            int previousAmount = inventorySnapshot[instanz];
            
            if (currentAmount > previousAmount) {
                oCItem* item = nullptr;
                bool isStackable = false;
                if (inv->inventory.next) {
                    zCListSort<oCItem>* node = inv->inventory.next;
                    while (node) {
                        if (node->data && node->data->instanz == instanz) {
                            item = node->data;
                            isStackable = (node->data->flags & ITM_FLAG_MULTI) != 0;
                            break;
                        }
                        node = node->next;
                    }
                }
                
                DEV_LOG( "DetectNewItems: From pickupQueue - instanz=" << instanz 
                    << " name=" << (item ? item->name.ToChar() : "UNKNOWN")
                    << " stackable=" << isStackable << endl);
                
                CatInvCore::AddRecentItem(instanz);
                
                inventorySnapshot[instanz]++;
            } else {
                DEV_LOG( "DetectNewItems: PickupQueue item instanz=" << instanz << " not confirmed in inventory, skipping" << endl);
            }
        }
        
        int addedCount = 0;
        DEV_LOG( "DetectNewItems: usedItemInstanz=" << usedItemInstanz << endl);
        for (const auto& pair : currentInv) {
            int instanz = pair.first;
            int currentAmount = pair.second;
            int previousAmount = inventorySnapshot[instanz];
            
            if (currentAmount > previousAmount) {
                if (usedItemInstanz != 0 && instanz == usedItemInstanz) {
                    oCItem* item = nullptr;
                    if (inv->inventory.next) {
                        zCListSort<oCItem>* node = inv->inventory.next;
                        while (node) {
                            if (node->data && node->data->instanz == instanz) {
                                item = node->data;
                                break;
                            }
                            node = node->next;
                        }
                    }
                    
                    if (item) {
                        DEV_LOG( "DetectNew: instanz=" << instanz << " name=" << item->name.ToChar() 
                            << " is the USED item (mainflag=" << item->mainflag 
                            << "), skipping to prevent false positive" << endl);
                        continue;
                    }
                }
                
                bool wasInRecent = (std::find(recentItems.begin(), recentItems.end(), instanz) != recentItems.end());
                
                oCItem* item = nullptr;
                bool isStackable = false;
                if (inv->inventory.next) {
                    zCListSort<oCItem>* node = inv->inventory.next;
                    while (node) {
                        if (node->data && node->data->instanz == instanz) {
                            item = node->data;
                            isStackable = (node->data->flags & ITM_FLAG_MULTI) != 0;
                            break;
                        }
                        node = node->next;
                    }
                }
                
                zSTRING itemName = item ? item->name : "UNKNOWN";
                int amountDiff = currentAmount - previousAmount;
                
                DEV_LOG( "DetectNew: instanz=" << instanz << " name=" << itemName.ToChar() 
                    << " was=" << previousAmount << " now=" << currentAmount 
                    << " diff=" << amountDiff << " stackable=" << isStackable 
                    << " wasInRecent=" << wasInRecent << endl);
                
                int timesToAdd = isStackable ? 1 : amountDiff;
                
                for (int i = 0; i < timesToAdd; i++) {
                    CatInvCore::AddRecentItem(instanz);
                }
                
                if (!wasInRecent) addedCount++;
            }
        }
        
        DEV_LOG( "DetectNewItems: added " << addedCount << " new items" << endl);
        DEV_LOG( "DetectNewItems: recentItems.size AFTER = " << recentItems.size() << endl);
        
        inventorySnapshot = currentInv;
        DEV_LOG( "Snapshot: Updated to current state (" << inventorySnapshot.size() << " items)" << endl);
        
        if (usedItemInstanz != 0) {
            DEV_LOG( "DetectNewItems: Resetting usedItemInstanz from " << usedItemInstanz << " to 0" << endl);
            usedItemInstanz = 0;
        }
    }
    
    HOOK Hook_oCNpc_EV_UseItemToState PATCH(&oCNpc::EV_UseItemToState, &oCNpc::EV_UseItemToState_Union);
    int oCNpc::EV_UseItemToState_Union(oCMsgManipulate* message)
    {
        if (this && this->IsAPlayer() && message && 
            (message->subType == oCMsgManipulate::EV_USEITEMTOSTATE || 
             message->subType == oCMsgManipulate::EV_USEITEM))
        {
            oCItem* item = dynamic_cast<oCItem*>(message->targetVob);
            if (item)
            {
                CatInvCore::usedItemInstanz = item->instanz;
                DEV_LOG( "EV_UseItemToState: Item being used name=" << item->name.ToChar() 
                    << " instanz=" << item->instanz 
                    << " mainflag=" << item->mainflag 
                    << ", storing instanz" << endl);
            }
        }
        
        int result = THISCALL(Hook_oCNpc_EV_UseItemToState)(message);
        
        return result;
    }
    
    HOOK Hook_oCNpcInventory_Remove PATCH(&oCNpcInventory::Remove, &oCNpcInventory::Remove_Union);
    oCItem* oCNpcInventory::Remove_Union(oCItem* item, int amount) {
        int side = this->right ? 1 : 0;
        
        zCListSort<oCItem>* savedContents = this->contents;
        bool wasFiltered = (CatInvCore::containerBySide[side] == this && 
                           CatInvCore::backupListBySide[side] &&
                           this->contents != CatInvCore::backupListBySide[side]);
        
        if (CatInvCore::containerBySide[side] == this && CatInvCore::backupListBySide[side]) {
            this->contents = CatInvCore::backupListBySide[side];
        }
        
        oCItem* result = (oCItem*)THISCALL(Hook_oCNpcInventory_Remove)(item, amount);
        
        if (CatInvCore::containerBySide[side] == this) {
            CatInvCore::backupListBySide[side] = this->contents;
        }
        
        if (wasFiltered && savedContents) {
            this->contents = savedContents;
        }
        
        if (player && this->owner == player && this->IsOpen() && CatInvCore::IsWorldReady()) {
            CatInvCore::batchUpdatePending = true;
        }
        
        return result;
    }
    
    HOOK Hook_oCNpcInventory_RemoveByPtr PATCH(&oCNpcInventory::RemoveByPtr, &oCNpcInventory::RemoveByPtr_Union);
    oCItem* oCNpcInventory::RemoveByPtr_Union(oCItem* item, int amount) {
        int side = this->right ? 1 : 0;
        
        zCListSort<oCItem>* savedContents = this->contents;
        bool wasFiltered = (CatInvCore::containerBySide[side] == this && 
                           CatInvCore::backupListBySide[side] &&
                           this->contents != CatInvCore::backupListBySide[side]);
        
        if (CatInvCore::containerBySide[side] == this && CatInvCore::backupListBySide[side]) {
            this->contents = CatInvCore::backupListBySide[side];
        }
        
        oCItem* result = (oCItem*)THISCALL(Hook_oCNpcInventory_RemoveByPtr)(item, amount);
        
        if (CatInvCore::containerBySide[side] == this) {
            CatInvCore::backupListBySide[side] = this->contents;
        }
        
        if (wasFiltered && savedContents) {
            this->contents = savedContents;
        }
        
        if (player && this->owner == player && this->IsOpen() && CatInvCore::IsWorldReady()) {
            CatInvCore::batchUpdatePending = true;
        }
        
        return result;
    }
    
    HOOK Hook_oCItemContainer_TransferItem PATCH(&oCItemContainer::TransferItem, &oCItemContainer::TransferItem_Union);
    int oCItemContainer::TransferItem_Union(int a, int b) {
        int result = (int)THISCALL(Hook_oCItemContainer_TransferItem)(a, b);
        
        bool needsFilter = CatInvCore::activeCategory != INV_CAT_ALL || CatInvCore::searchActive || CatInvCore::activeSortMode != CatInvCore::SORT_NONE;
        
        if (CatInvCore::IsWorldReady() && needsFilter) {
            for (int side = 0; side < 2; ++side) {
                oCItemContainer* cont = CatInvCore::containerBySide[side];
                if (cont) {
                    if (!cont->right) {
                        oCStealContainer* stealCont = dynamic_cast<oCStealContainer*>(cont);
                        if (stealCont) {
                            stealCont->CreateList();
                        } else {
                            oCNpcContainer* npcCont = dynamic_cast<oCNpcContainer*>(cont);
                            if (npcCont) {
                                npcCont->CreateList();
                            } else {
                                CatInvCore::FilterContainerByCategory(cont);
                            }
                        }
                    } else {
                        CatInvCore::FilterContainerByCategory(cont);
                    }
                }
            }
        }
        
        return result;
    }
    
    void CatInvCore::Init() {
        if (initialized) return;

        CatInvOptions::ReadOptions();
        
        TrimRecentItems();

        activeCategory = INV_CAT_ALL;

        initialized = true;
    }
    
    void CatInvCore::ActivateSearch() {
        if (searchActive) return;
        
        previousCategory = activeCategory;
        searchText = L"";
        
        if (activeCategory != INV_CAT_ALL) {
            activeCategory = INV_CAT_ALL;
            UpdateAllContainers();
        }
        
        searchActive = true;
        searchInputActive = true;
    }
    
    void CatInvCore::DeactivateSearch() {
        if (!searchActive) return;
        
        searchActive = false;
        searchInputActive = false;
        searchText = L"";
        
        UpdateAllContainers();
    }
    
    void CatInvCore::UpdateSearchText(char c) {
        if (!searchActive) return;
        
        searchText += (wchar_t)c;
        UpdateAllContainers();
    }
    
    void CatInvCore::RemoveLastSearchChar() {
        if (!searchActive) return;
        if (searchText.length() == 0) return;
        
        searchText = searchText.substr(0, searchText.length() - 1);
        UpdateAllContainers();
    }
    
    bool CatInvCore::ItemMatchesSearch(oCItem* item) {
        if (!item) return false;
        if (!searchActive) return true;
        if (searchText.length() == 0) return true;
        
        zSTRING itemName = item->name;
        
        std::wstring itemNameW = AToW(itemName.ToChar());
        std::wstring searchW = searchText;
        
        CharUpperW(&itemNameW[0]);
        CharUpperW(&searchW[0]);
        
        return itemNameW.find(searchW) != std::wstring::npos;
    }
    
    void CatInvCore::FilterContainerBySearch(oCItemContainer* container) {
        if (!container) return;
        if (!container->contents) return;
        if (!searchActive) return;
        
        int side = container->right ? 1 : 0;
        
        if (!SupportCategories(container)) return;

        if (containerBySide[side] != container) {
            if (containerBySide[side] && backupListBySide[side]) {
                containerBySide[side]->contents = backupListBySide[side];
            }
            backupListBySide[side] = container->contents;
            containerBySide[side] = container;
        }

        if (!backupListBySide[side]) {
            backupListBySide[side] = container->contents;
            containerBySide[side] = container;
        }

        if (!filteredListBySide[side]) {
            filteredListBySide[side] = new zCListSort<oCItem>();
        }
        filteredListBySide[side]->next = nullptr;
        filteredListBySide[side]->data = nullptr;
        
        if (backupListBySide[side] && backupListBySide[side]->Compare) {
            filteredListBySide[side]->Compare = backupListBySide[side]->Compare;
        }
        
        zCListSort<oCItem>* tail = filteredListBySide[side];

        zCListSort<oCItem>* node = backupListBySide[side]->next;
        while (node) {
            if (node->data) {
                oCItem* item = node->data;
                if (ItemMatchesSearch(item)) {
                    zCListSort<oCItem>* newNode = new zCListSort<oCItem>();
                    newNode->data = item;
                    newNode->next = nullptr;
                    tail->next = newNode;
                    tail = newNode;
                }
            }
            node = node->next;
        }

        container->contents = filteredListBySide[side];
        
        if (activeSortMode != SORT_NONE) {
            SortContainer(container);
        }
        
        ResetOffset(container);
        container->CheckSelectedItem();
        container->prepared = 0;
        oCItemContainer::Container_PrepareDraw();
    }
    
    void CatInvCore::DrawSearchBox(oCItemContainer* container) {
        if (!container) return;
        if (!IsWorldReady()) return;
        if (!searchActive) return;
        if (!SupportCategories(container)) return;
        if (!container->viewTitle) return;
        if (!container->contents) return;

        if (searchView == NULL) {
            searchView = new zCView(0, 0, 8192, 8192);
        }

        zCView* viewTitle = container->viewTitle;
        
        int defaultWidth = *(int*)DEFAULT_WIDTH_ADDR;
        int width = 2 * defaultWidth;
        int height = viewTitle->vsizey;

        int posY = viewTitle->vposy;
        int posX;
        if (container->right) {
            posX = viewTitle->vposx - width + 1;
        } else {
            posX = viewTitle->vposx + viewTitle->vsizex - 1;
        }

        searchView->SetPos(posX, posY);
        searchView->SetSize(width, height);
        
        zCTexture* backTex = nullptr;
        if (container->viewBack && container->viewBack->backTex) {
            backTex = container->viewBack->backTex;
        }
        
        zCTexture* titleTex = nullptr;
        if (container->viewTitle && container->viewTitle->backTex) {
            titleTex = container->viewTitle->backTex;
        }
        
        if (screen) {
            screen->InsertItem(searchView, 0);
        }
        
        if (backTex) {
            searchView->InsertBack(backTex);
        }
        searchView->SetTransparency(255);
        searchView->ClrPrintwin();
        searchView->Blit();
        
        if (titleTex) {
            searchView->InsertBack(titleTex);
        }
        
        zSTRING displayText = WToA(searchText).c_str();
        if (searchInputActive) {
            displayText += "_";
        }
        searchView->PrintCXY(displayText);
        searchView->Blit();
        
        if (screen) {
            screen->RemoveItem(searchView);
        }
    }
    
    void CatInvCore::ToggleFavorite(oCItem* item) {
        if (!item) return;
        
        int itemId = item->instanz;
        
        if (favoriteItems.find(itemId) != favoriteItems.end()) {
            favoriteItems.erase(itemId);
        } else {
            favoriteItems.insert(itemId);
        }
        
        if (activeCategory == INV_CAT_FAVORITES) {
            UpdateAllContainers();
        }
    }
    
    bool CatInvCore::IsFavorite(oCItem* item) {
        if (!item) return false;
        return favoriteItems.find(item->instanz) != favoriteItems.end();
    }
    
    void CatInvCore::SaveFavorites(int slotId) {
        SaveRecentItems(slotId);
    }
    
    void CatInvCore::LoadFavorites(int slotId) {
        ClearFavorites();
        ClearRecentItems();
        inventorySnapshot.clear();
        
        if (!zoptions) {
            return;
        }
        
        zSTRING savesDir = zoptions->GetDirString(zTOptionPaths::DIR_SAVEGAMES);
        zSTRING slotDir = SaveLoadGameInfo.GetSaveSlotName(slotId);
        zSTRING savePath = savesDir + "\\" + slotDir + "\\CATINV.SAV";
        
        zCArchiver* ar = zarcFactory->CreateArchiverRead(savePath, 0);
        if (!ar) {
             return;
        }

        int version = ar->ReadInt("version");
        if (version != 4) {
            ar->Close();
            ar->Release();
            return;
        }
        
        int favCount = ar->ReadInt("favoritesCount");
        for (int idx = 0; idx < favCount; idx++) {
            char key[64];
            sprintf(key, "favorite%d", idx);
            int itemId = ar->ReadInt(key);
            if (itemId > 0) {
                favoriteItems.insert(itemId);
            }
        }
        
        int recentCount = ar->ReadInt("recentCount");
        for (int idx = 0; idx < recentCount; idx++) {
            char key[64];
            sprintf(key, "recent%d", idx);
            int itemId = ar->ReadInt(key);
            if (itemId > 0) {
                recentItems.push_back(itemId);
            }
        }
        
        int pickupCount = ar->ReadInt("pickupQueueCount");
        for (int idx = 0; idx < pickupCount; idx++) {
            char key[64];
            sprintf(key, "pickup%d", idx);
            int itemId = ar->ReadInt(key);
            if (itemId > 0) {
                pickupQueue.push_back(itemId);
            }
        }
        DEV_LOG( "LoadFavorites: Loaded pickupQueue with " << pickupQueue.size() << " items" << endl);
        
        int snapshotCount = ar->ReadInt("snapshotCount");
        for (int idx = 0; idx < snapshotCount; idx++) {
            char keyInst[64], keyAmt[64];
            sprintf(keyInst, "snapshotInst%d", idx);
            sprintf(keyAmt, "snapshotAmt%d", idx);
            int instanz = ar->ReadInt(keyInst);
            int amount = ar->ReadInt(keyAmt);
            if (instanz > 0) {
                inventorySnapshot[instanz] = amount;
            }
        }
        DEV_LOG( "LoadFavorites: Loaded snapshot with " << inventorySnapshot.size() << " items" << endl);
        
        ar->Close();
        ar->Release();
    }
    
    void CatInvCore::ClearFavorites() {
        favoriteItems.clear();
    }
    
    void CatInvCore::AddRecentItem(int instanz) {
        if (instanz <= 0) return;
        
        bool isStackable = false;
        oCItem* itemExample = nullptr;
        if (player) {
            oCNpcInventory* inv = &player->inventory2;
            if (inv && inv->inventory.next) {
                zCListSort<oCItem>* node = inv->inventory.next;
                while (node) {
                    if (node->data && node->data->instanz == instanz) {
                        itemExample = node->data;
                        isStackable = (node->data->flags & ITM_FLAG_MULTI) != 0;
                        break;
                    }
                    node = node->next;
                }
            }
        }
        
        if (isStackable) {
            auto it = std::find(recentItems.begin(), recentItems.end(), instanz);
            if (it != recentItems.end()) {
                recentItems.erase(it);
                DEV_LOG( "AddRecentItem: Removed old entry for stackable instanz=" << instanz << " (moving to front)" << endl);
                
                if (itemExample && recentItemPointers.find(itemExample) != recentItemPointers.end()) {
                    recentItemPointers.erase(itemExample);
                }
            }
        }
        
        recentItems.push_front(instanz);
        
        if (player) {
            oCNpcInventory* inv = &player->inventory2;
            if (inv && inv->inventory.next) {
                zCListSort<oCItem>* node = inv->inventory.next;
                while (node) {
                    if (node->data && node->data->instanz == instanz) {
                        if (recentItemPointers.find(node->data) == recentItemPointers.end()) {
                            recentItemPointers.insert(node->data);
                            DEV_LOG( "AddRecentItem: Added new pointer for instanz=" << instanz 
                                << " stackable=" << isStackable << endl);
                            break;
                        }
                    }
                    node = node->next;
                }
            }
        }
        
        while (recentItems.size() > (size_t)CatInvOptions::MaxRecentItems) {
            int removedInstanz = recentItems.back();
            recentItems.pop_back();
            
            if (player) {
                oCNpcInventory* inv = &player->inventory2;
                if (inv && inv->inventory.next) {
                    zCListSort<oCItem>* node = inv->inventory.next;
                    while (node) {
                        if (node->data && node->data->instanz == removedInstanz) {
                            if (recentItemPointers.find(node->data) != recentItemPointers.end()) {
                                recentItemPointers.erase(node->data);
                                DEV_LOG("AddRecentItem: Removed old pointer for instanz=" << removedInstanz << " (trim)" << endl);
                                break;
                            }
                        }
                        node = node->next;
                    }
                }
            }
        }
    }
    
    void CatInvCore::RemoveRecentItem(int instanz) {
        if (instanz <= 0) return;
        
        auto it = std::find(recentItems.begin(), recentItems.end(), instanz);
        if (it != recentItems.end()) {
            recentItems.erase(it);
        }
        
        if (player) {
            oCNpcInventory* inv = &player->inventory2;
            if (inv && inv->inventory.next) {
                zCListSort<oCItem>* node = inv->inventory.next;
                while (node) {
                    if (node->data && node->data->instanz == instanz) {
                        recentItemPointers.erase(node->data);
                    }
                    node = node->next;
                }
            }
        }
    }
    
    bool CatInvCore::IsRecent(oCItem* item) {
        if (!item) return false;
        
        if (recentItemPointers.find(item) != recentItemPointers.end()) {
            return true;
        }
        
        auto it = std::find(recentItems.begin(), recentItems.end(), item->instanz);
        return (it != recentItems.end());
    }
    
    void CatInvCore::RebuildRecentPointers() {
        DEV_LOG( "=== RebuildRecentPointers START ===" << endl);
        DEV_LOG( "RebuildRecentPointers: recentItemPointers.size BEFORE clear = " << recentItemPointers.size() << endl);
        recentItemPointers.clear();
        
        if (!player) {
            DEV_LOG( "RebuildRecentPointers: No player!" << endl);
            return;
        }
        
        oCNpcInventory* inv = &player->inventory2;
        if (!inv || !inv->inventory.next) {
            DEV_LOG( "RebuildRecentPointers: No inventory!" << endl);
            return;
        }
        
        DEV_LOG( "RebuildRecentPointers: Building for " << recentItems.size() << " recent entries" << endl);
        
        std::unordered_map<int, int> usedCount;
        
        for (int instanz : recentItems) {
            DEV_LOG( "RebuildRecentPointers: Looking for instanz=" << instanz << " (used so far: " << usedCount[instanz] << ")" << endl);
            
            int skipCount = usedCount[instanz];
            int currentCount = 0;
            bool found = false;
            
            zCListSort<oCItem>* node = inv->inventory.next;
            while (node) {
                if (node->data && node->data->instanz == instanz) {
                    if (currentCount >= skipCount) {
                        recentItemPointers.insert(node->data);
                        usedCount[instanz]++;
                        DEV_LOG( "RebuildRecentPointers: Added item #" << currentCount << " for instanz=" << instanz 
                            << " name=" << node->data->name.ToChar() << endl);
                        found = true;
                        break;
                    }
                    currentCount++;
                }
                node = node->next;
            }
            
            if (!found) {
                DEV_LOG( "RebuildRecentPointers: WARNING - instanz=" << instanz << " item #" << skipCount << " not found!" << endl);
            }
        }
        
        DEV_LOG( "RebuildRecentPointers: Built " << recentItemPointers.size() << " pointers" << endl);
        DEV_LOG( "=== RebuildRecentPointers END ===" << endl);
    }
    
    void CatInvCore::ClearRecentItems() {
        recentItems.clear();
        recentItemPointers.clear();
        pickupQueue.clear();
    }
    
    void CatInvCore::TrimRecentItems() {
        while (recentItems.size() > (size_t)CatInvOptions::MaxRecentItems) {
            int removedInstanz = recentItems.back();
            recentItems.pop_back();
            
            if (player) {
                oCNpcInventory* inv = &player->inventory2;
                if (inv && inv->inventory.next) {
                    zCListSort<oCItem>* node = inv->inventory.next;
                    while (node) {
                        if (node->data && node->data->instanz == removedInstanz) {
                            if (recentItemPointers.find(node->data) != recentItemPointers.end()) {
                                recentItemPointers.erase(node->data);
                                DEV_LOG( "TrimRecentItems: Removed pointer for instanz=" << removedInstanz << endl);
                                break;
                            }
                        }
                        node = node->next;
                    }
                }
            }
        }
    }
    
    void CatInvCore::SaveRecentItems(int slotId) {
        if (!zoptions) return;
        
        zSTRING savesDir = zoptions->GetDirString(zTOptionPaths::DIR_SAVEGAMES);
        zSTRING slotDir = SaveLoadGameInfo.GetSaveSlotName(slotId);
        zSTRING savePath = savesDir + "\\" + slotDir + "\\CATINV.SAV";
        
        zCArchiver* ar = zarcFactory->CreateArchiverWrite(savePath, zARC_MODE_ASCII, 0, 0);
        if (!ar) return;
        
        ar->WriteInt("version", 4);
        
        ar->WriteInt("favoritesCount", favoriteItems.size());
        int idx = 0;
        for (std::set<int>::iterator it = favoriteItems.begin(); it != favoriteItems.end(); ++it) {
            char key[64];
            sprintf(key, "favorite%d", idx);
            ar->WriteInt(key, *it);
            idx++;
        }
        
        ar->WriteInt("recentCount", recentItems.size());
        idx = 0;
        for (std::deque<int>::iterator it = recentItems.begin(); it != recentItems.end(); ++it) {
            char key[64];
            sprintf(key, "recent%d", idx);
            ar->WriteInt(key, *it);
            idx++;
        }
        
        ar->WriteInt("pickupQueueCount", pickupQueue.size());
        idx = 0;
        for (std::deque<int>::iterator it = pickupQueue.begin(); it != pickupQueue.end(); ++it) {
            char key[64];
            sprintf(key, "pickup%d", idx);
            ar->WriteInt(key, *it);
            idx++;
        }
        
        ar->WriteInt("snapshotCount", inventorySnapshot.size());
        idx = 0;
        for (std::unordered_map<int, int>::iterator it = inventorySnapshot.begin(); it != inventorySnapshot.end(); ++it) {
            char keyInst[64], keyAmt[64];
            sprintf(keyInst, "snapshotInst%d", idx);
            sprintf(keyAmt, "snapshotAmt%d", idx);
            ar->WriteInt(keyInst, it->first);  // instanz
            ar->WriteInt(keyAmt, it->second);  // amount
            idx++;
        }
        
        ar->Close();
        ar->Release();
    }
    
    bool inContainerDraw = false;
    
    HOOK Hook_Ivk_oCItemContainer_Draw PATCH(&oCItemContainer::Draw, &oCItemContainer::Draw_Union);
    void oCItemContainer::Draw_Union() {
        inContainerDraw = true;
        THISCALL(Hook_Ivk_oCItemContainer_Draw)();
        inContainerDraw = false;
    }
    
    HOOK Hook_Ivk_oCItemContainer_DrawItemInfo PATCH(&oCItemContainer::DrawItemInfo, &oCItemContainer::DrawItemInfo_Union);
    void oCItemContainer::DrawItemInfo_Union(oCItem* item, zCWorld* world) {
        bool prevFlag = inContainerDraw;
        inContainerDraw = false;
        THISCALL(Hook_Ivk_oCItemContainer_DrawItemInfo)(item, world);
        inContainerDraw = prevFlag;
    }
    
    HOOK Hook_Ivk_oCItem_RenderItem PATCH(&oCItem::RenderItem, &oCItem::RenderItem_Union);
    void oCItem::RenderItem_Union(zCWorld* world, zCViewBase* viewBase, float time) {
        THISCALL(Hook_Ivk_oCItem_RenderItem)(world, viewBase, time);
        
        if (!inContainerDraw) return;
        
        if (!CatInvCore::IsFavorite(this)) return;
        if (!CatInvCore::IsWorldReady()) return;
        
        zCView* view = dynamic_cast<zCView*>(viewBase);
        if (!view) return;
        
        int x1, y1, x2, y2;
        int baseSize = 1200;
        
        float sizeMultiplier = 1.0f;
        switch (CatInvOptions::FavoriteIconSize) {
            case 1: sizeMultiplier = 1.5f; break;
            case 2: sizeMultiplier = 2.0f; break;
            default: sizeMultiplier = 1.0f; break;
        }
        
        int iconSize = (int)(baseSize * sizeMultiplier);
        int margin = 300;   
        
        switch (CatInvOptions::FavoriteIconCorner) {
            case 0: // Bottom-left (default)
                x1 = margin;
                y1 = 8192 - iconSize - margin;
                x2 = margin + iconSize;
                y2 = 8192 - margin;
                break;
            case 1: // Bottom-right
                x1 = 8192 - iconSize - margin;
                y1 = 8192 - iconSize - margin;
                x2 = 8192 - margin;
                y2 = 8192 - margin;
                break;
            case 2: // Top-left
                x1 = margin;
                y1 = margin;
                x2 = margin + iconSize;
                y2 = margin + iconSize;
                break;
            case 3: // Top-right
                x1 = 8192 - iconSize - margin;
                y1 = margin;
                x2 = 8192 - margin;
                y2 = margin + iconSize;
                break;
            default:
                x1 = margin;
                y1 = 8192 - iconSize - margin;
                x2 = margin + iconSize;
                y2 = 8192 - margin;
                break;
        }
        
        zCView iconView(x1, y1, x2, y2);
        iconView.InsertBack("FAV.TGA");
        iconView.SetAlphaBlendFunc(zRND_ALPHA_FUNC_BLEND);
        iconView.SetTransparency(255);
        view->InsertItem(&iconView);
        iconView.Blit();
        view->RemoveItem(&iconView);
    }
        
    zSTRING CatInvCore::GetSortModeName() {
        if (activeSortMode == SORT_NONE) {
            return zSTRING("");
        }
        
        LANGID langId = GetSystemDefaultLangID();
        WORD primaryLang = PRIMARYLANGID(langId);
        
        // 9-1 = descending (high to low), 1-9 = ascending (low to high)
        const char* sortNames[5][10] = {
            {"", "", "", "", "", "", "", "", "", ""},
            {"Price: 9-1", "Cena: 9-1", "Preis: 9-1", "Цена: 9-1", "Cena: 9-1", "Prix: 9-1", "Prezzo: 9-1", "Precio: 9-1", "Ár: 9-1", "Ціна: 9-1"},
            {"Price: 1-9", "Cena: 1-9", "Preis: 1-9", "Цена: 1-9", "Cena: 1-9", "Prix: 1-9", "Prezzo: 1-9", "Precio: 1-9", "Ár: 1-9", "Ціна: 1-9"},
            {"Name: A-Z", "Nazwa: A-Z", "Name: A-Z", "Имя: А-Я", "Název: A-Z", "Nom: A-Z", "Nome: A-Z", "Nombre: A-Z", "Név: A-Z", "Ім'я: А-Я"},
            {"Name: Z-A", "Nazwa: Z-A", "Name: Z-A", "Имя: Я-А", "Název: Z-A", "Nom: Z-A", "Nome: Z-A", "Nombre: Z-A", "Név: Z-A", "Ім'я: Я-А"}
        };
        
        int langIndex = 0;
        switch (primaryLang) {
            case LANG_POLISH:     langIndex = 1; break;
            case LANG_GERMAN:     langIndex = 2; break;
            case LANG_RUSSIAN:    langIndex = 3; break;
            case LANG_CZECH:      langIndex = 4; break;
            case LANG_FRENCH:     langIndex = 5; break;
            case LANG_ITALIAN:    langIndex = 6; break;
            case LANG_SPANISH:    langIndex = 7; break;
            case LANG_HUNGARIAN:  langIndex = 8; break;
            case LANG_UKRAINIAN:  langIndex = 9; break;
        }
        
        return zSTRING(sortNames[activeSortMode][langIndex]);
    }
    
    void CatInvCore::SortContainer(oCItemContainer* container) {
        if (!container) return;
        if (!container->contents) return;
        if (activeSortMode == SORT_NONE) return;
        
        std::vector<oCItem*> items;
        zCListSort<oCItem>* node = container->contents->next;
        while (node) {
            if (node->data) {
                items.push_back(node->data);
            }
            node = node->next;
        }
        
        switch (activeSortMode) {
            case SORT_PRICE_DESC:
                std::sort(items.begin(), items.end(), [](oCItem* a, oCItem* b) {
                    return a->value > b->value;
                });
                break;
            case SORT_PRICE_ASC:
                std::sort(items.begin(), items.end(), [](oCItem* a, oCItem* b) {
                    return a->value < b->value;
                });
                break;
            case SORT_NAME_AZ:
                std::sort(items.begin(), items.end(), [](oCItem* a, oCItem* b) {
                    return CompareStringsLocaleAware(a->name.ToChar(), b->name.ToChar()) < 0;
                });
                break;
            case SORT_NAME_ZA:
                std::sort(items.begin(), items.end(), [](oCItem* a, oCItem* b) {
                    return CompareStringsLocaleAware(a->name.ToChar(), b->name.ToChar()) > 0;
                });
                break;
        }
        
        zCListSort<oCItem>* head = container->contents;
        zCListSort<oCItem>* current = head;
        
        for (size_t i = 0; i < items.size(); i++) {
            if (!current->next) {
                current->next = new zCListSort<oCItem>();
            }
            current = current->next;
            current->data = items[i];
        }
        
        if (current && current->next) {
            current->next = nullptr;
        }
        
        container->prepared = 0;
        oCItemContainer::Container_PrepareDraw();
    }
    
    void CatInvCore::DrawSortMode(oCItemContainer* container) {
        if (!container) return;
        if (!IsWorldReady()) return;
        if (!SupportCategories(container)) return;
        if (!container->viewTitle) return;
        if (!container->contents) return;
        if (activeSortMode == SORT_NONE) return;
        
        if (sortView == NULL) {
            sortView = new zCView(0, 0, 8192, 8192);
        }
        
        zCView* viewTitle = container->viewTitle;
        
        int defaultWidth = *(int*)DEFAULT_WIDTH_ADDR;
        int width = 2 * defaultWidth;
        int height = viewTitle->vsizey;
        
        int posY;
        if (CatInvOptions::SortWindowPosition == 1) {
            // Top position: above gold window (viewTitle)
            posY = viewTitle->vposy - height + 1;
        } else {
            // Bottom position (default): below gold window (viewTitle)
            posY = viewTitle->vposy + viewTitle->vsizey - 1;
        }
        
        int posX;
        if (container->right) {
            posX = viewTitle->vposx;
        } else {
            posX = viewTitle->vposx + viewTitle->vsizex - width;
        }
        
        sortView->SetPos(posX, posY);
        sortView->SetSize(width, height);
        
        zCTexture* backTex = nullptr;
        if (container->viewBack && container->viewBack->backTex) {
            backTex = container->viewBack->backTex;
        }
        
        zCTexture* titleTex = nullptr;
        if (container->viewTitle && container->viewTitle->backTex) {
            titleTex = container->viewTitle->backTex;
        }
        
        if (screen) {
            screen->InsertItem(sortView, 0);
        }
        
        if (backTex) {
            sortView->InsertBack(backTex);
        }
        sortView->SetTransparency(255);
        sortView->ClrPrintwin();
        sortView->Blit();
        
        if (titleTex) {
            sortView->InsertBack(titleTex);
        }
        
        zSTRING displayText = GetSortModeName();
        sortView->PrintCXY(displayText);
        sortView->Blit();
        
        if (screen) {
            screen->RemoveItem(sortView);
        }
    }
    
    void CatInvCore::JumpToItemCategory() {
        if (!player) return;
        
        if (!player->inventory2.IsOpen()) return;
        
        oCItemContainer* activeContainer = nullptr;
        bool isPlayerInventory = false;
        
        if (containerBySide[1] && containerBySide[1]->IsActive()) {
            activeContainer = containerBySide[1];
            isPlayerInventory = true;
        } else if (containerBySide[0] && containerBySide[0]->IsActive()) {
            activeContainer = containerBySide[0];
            isPlayerInventory = false;
        } else {
            return;
        }
        
        if (!isPlayerInventory && dynamic_cast<oCNpcContainer*>(activeContainer)) {
            DEV_LOG("CatInv: JumpToItemCategory blocked for corpse" << endl);
            return;
        }
        
        oCItem* selectedItem = activeContainer->GetSelectedItem();
        if (!selectedItem) {
            DEV_LOG("CatInv: JumpToItemCategory - no item selected" << endl);
            return;
        }
        
        if (!isPlayerInventory) {
            if (!containerBySide[1]) {
                DEV_LOG("CatInv: JumpToItemCategory - player inventory not available" << endl);
                return;
            }
            
            bool haveItemInInventory = false;
            if (containerBySide[1]->contents) {
                zCListSort<oCItem>* list = containerBySide[1]->contents->next;
                while (list) {
                    if (list->data && list->data->instanz == selectedItem->instanz) {
                        haveItemInInventory = true;
                        break;
                    }
                    list = list->next;
                }
            }
            
            if (!haveItemInInventory) {
                DEV_LOG("CatInv: Item not found in player inventory, canceling jump" << endl);
                return;
            }
            
            DEV_LOG("CatInv: Switching from left side to player inventory" << endl);
            
            int result = activeContainer->ActivateNextContainer(1); // 1 = move to right
            if (result == 0) {
                DEV_LOG("CatInv: Failed to switch containers" << endl);
                return;
            }
            
            activeContainer = containerBySide[1];
        }
        
        int realCategory = INV_CAT_ALL;
        for (int catID = INV_CAT_WEAPON; catID < INV_CAT_MAX; catID++) {
            if (selectedItem->mainflag & INV_CAT_GROUPS[catID]) {
                realCategory = catID;
                break;
            }
        }
        
        if (!isPlayerInventory) {
        } else {
            if (activeCategory == realCategory) {
                DEV_LOG("CatInv: Already in item's real category (" << realCategory << ")" << endl);
                return;
            }
        }
        
        DEV_LOG("CatInv: Jumping to item's real category: " << realCategory << " from " << activeCategory << endl);
        
        if (searchActive) {
            DeactivateSearch();
        }
        
        oCItem* targetItemPtr = selectedItem;
        int targetInstanz = selectedItem->instanz;
        
        activeCategory = realCategory;
        activeSortMode = SORT_NONE;
        
        FilterContainerByCategory(activeContainer);
        
        if (activeContainer->contents) {
            zCListSort<oCItem>* list = activeContainer->contents->next;
            int index = 0;
            bool foundExactPointer = false;
            
            while (list) {
                if (list->data == targetItemPtr) {
                    activeContainer->selectedItem = index;
                    foundExactPointer = true;
                    
                    int maxCols = activeContainer->maxSlotsCol;
                    int maxRows = activeContainer->maxSlotsRow;
                    int visibleSlots = maxCols * maxRows;
                    
                    if (index >= activeContainer->offset + visibleSlots) {
                        activeContainer->offset = (index / maxCols) * maxCols;
                    } else if (index < activeContainer->offset) {
                        activeContainer->offset = (index / maxCols) * maxCols;
                    }
                    
                    DEV_LOG("CatInv: Selected exact item pointer at index " << index << " in real category" << endl);
                    break;
                }
                index++;
                list = list->next;
            }
            
            if (!foundExactPointer) {
                list = activeContainer->contents->next;
                index = 0;
                while (list) {
                    if (list->data && list->data->instanz == targetInstanz) {
                        activeContainer->selectedItem = index;
                        
                        int maxCols = activeContainer->maxSlotsCol;
                        int maxRows = activeContainer->maxSlotsRow;
                        int visibleSlots = maxCols * maxRows;
                        
                        if (index >= activeContainer->offset + visibleSlots) {
                            activeContainer->offset = (index / maxCols) * maxCols;
                        } else if (index < activeContainer->offset) {
                            activeContainer->offset = (index / maxCols) * maxCols;
                        }
                        
                        DEV_LOG("CatInv: Selected item with same instanz at index " << index << " in real category" << endl);
                        break;
                    }
                    index++;
                    list = list->next;
                }
            }
        }
        
        activeContainer->prepared = 0;
        oCItemContainer::Container_PrepareDraw();
    }
}