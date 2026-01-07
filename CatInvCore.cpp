#include "UnionAfx.h"
#include <algorithm>

namespace GOTHIC_ENGINE {
    int CatInvCore::activeCategory = 0;
    zCView* CatInvCore::categoryView = nullptr;
    bool CatInvCore::initialized = false;
    zCListSort<oCItem>* CatInvCore::backupListBySide[2] = { nullptr, nullptr };
    oCItemContainer* CatInvCore::containerBySide[2] = { nullptr, nullptr };
    zCListSort<oCItem>* CatInvCore::filteredListBySide[2] = { nullptr, nullptr };
    bool CatInvCore::hooksActive = false;
    bool CatInvCore::searchActive = false;
    bool CatInvCore::searchInputActive = false;
    std::wstring CatInvCore::searchText = L"";
    zCView* CatInvCore::searchView = nullptr;
    int CatInvCore::previousCategory = 0;

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
        zCPar_Symbol* txtInvCatSymbol = nullptr;
        if (parser) {
            txtInvCatSymbol = parser->GetSymbol("TXT_INV_CAT");
        }
        
        if (txtInvCatSymbol && categoryID >= 0 && categoryID < (int)INV_CAT_MAX) {
            if (txtInvCatSymbol->type == zPAR_TYPE_STRING && txtInvCatSymbol->ele > (unsigned int)categoryID) {
                if (txtInvCatSymbol->stringdata && txtInvCatSymbol->stringdata[categoryID].Length() > 0) {
                    return txtInvCatSymbol->stringdata[categoryID];
                }
            }
        }
        
        if (categoryID >= 0 && categoryID < INV_CAT_MAX) {
            return zSTRING(DEFAULT_CATEGORY_NAMES[categoryID]);
        }
        return zSTRING("");
    }

    bool CatInvCore::IsWorldReady() {
        if (!ogame) return false;
        if (ogame->inScriptStartup || ogame->inLoadSaveGame || ogame->inLevelChange) return false;
        if (!screen || !zinput) return false;
        return true;
    }

    bool CatInvCore::ItemMatchesCategory(oCItem* item, int category) {
        if (!item) return false;
        if (category == INV_CAT_ALL) return true;

        int catID = GetCategoryID(category);
        if (catID >= 0 && catID < INV_CAT_MAX) {
            return (item->mainflag & INV_CAT_GROUPS[catID]) != 0;
        }
        return false;
    }

    bool CatInvCore::SetCategory(int category) {
        int newCategory = category;
        
        int minCat = CatInvOptions::G1Mode ? 1 : 0;
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
        return SetCategory(activeCategory + offset);
    }

    void CatInvCore::SetCategoryFirst() {
        SetCategory(CatInvOptions::G1Mode ? 1 : 0);
    }

    void CatInvCore::SetCategoryLast() {
        SetCategory(INV_CAT_MAX - 1);
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
            if (containerBySide[side] == container && backupListBySide[side]) {
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

        container->contents = filteredListBySide[side];
        
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
            if (isDeadBody) {
                SetCategory(INV_CAT_ALL);
            } else {
                if (!CatInvOptions::G1Mode) {
                    SetCategoryFirst();
                }
            }
            SetSelectionFirst(container);
        }

        if (activeCategory != INV_CAT_ALL) {
            FilterContainerByCategory(container);
        }
    }

    void CatInvCore::OnContainerClose(oCItemContainer* container) {
    }

    void CatInvCore::DrawCategory(oCItemContainer* container) {
        if (!container) return;
        if (!IsWorldReady()) return;
        if (activeCategory == 0 && !CatInvOptions::G1Mode && !searchActive) return;
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
                return;
            }
        }
        THISCALL(Hook_oCItemContainer_NextItem)();
    }

    HOOK Hook_oCItemContainer_PrevItem PATCH(&oCItemContainer::PrevItem, &oCItemContainer::PrevItem_Union);
    void oCItemContainer::PrevItem_Union() {
        if (CatInvCore::IsWorldReady() && zinput && (zinput->KeyPressed(KEY_LSHIFT) || zinput->KeyPressed(KEY_RSHIFT))) {
            if (CatInvCore::SupportCategories(this)) {
                return;
            }
        }
        THISCALL(Hook_oCItemContainer_PrevItem)();
    }

    HOOK Hook_oCItemContainer_NextItemLine PATCH(&oCItemContainer::NextItemLine, &oCItemContainer::NextItemLine_Union);
    void oCItemContainer::NextItemLine_Union() {
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
    }

    HOOK Hook_oCItemContainer_PrevItemLine PATCH(&oCItemContainer::PrevItemLine, &oCItemContainer::PrevItemLine_Union);
    void oCItemContainer::PrevItemLine_Union() {
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
    }

    HOOK Hook_oCItemContainer_OpenPassive PATCH(&oCItemContainer::OpenPassive, &oCItemContainer::OpenPassive_Union);
    void oCItemContainer::OpenPassive_Union(int a, int b, int c) {
        THISCALL(Hook_oCItemContainer_OpenPassive)(a, b, c);
        
        if (CatInvCore::IsWorldReady()) {
            CatInvCore::OnContainerOpen(this);
        }
    }
    
    HOOK Hook_oCItemContainer_Close PATCH(&oCItemContainer::Close, &oCItemContainer::Close_Union);
    void oCItemContainer::Close_Union() {
        if (CatInvCore::IsWorldReady()) {
            CatInvCore::OnContainerClose(this);
            CatInvCore::ResetContainer(this);
        }
        THISCALL(Hook_oCItemContainer_Close)();
    }
    
    HOOK Hook_oCItemContainer_DrawCategory PATCH(&oCItemContainer::DrawCategory, &oCItemContainer::DrawCategory_Union);
    void oCItemContainer::DrawCategory_Union() {
        THISCALL(Hook_oCItemContainer_DrawCategory)();
        if (CatInvCore::IsWorldReady()) {
            bool needsFilter = CatInvCore::activeCategory != INV_CAT_ALL || CatInvCore::searchActive;
            if (needsFilter) {
                CatInvCore::FilterContainerByCategory(this);
            }
            CatInvCore::DrawCategory(this);
        }
    }

    HOOK Hook_oCItemContainer_CheckSelectedItem PATCH(&oCItemContainer::CheckSelectedItem, &oCItemContainer::CheckSelectedItem_Union);
    void oCItemContainer::CheckSelectedItem_Union() {
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

    HOOK Hook_oCStealContainer_CreateList PATCH(&oCStealContainer::CreateList, &oCStealContainer::CreateList_Union);
    void oCStealContainer::CreateList_Union() {
        int side = this->right ? 1 : 0;
        
        if (CatInvCore::containerBySide[side] == this && CatInvCore::backupListBySide[side]) {
            this->contents = CatInvCore::backupListBySide[side];
        }
        
        THISCALL(Hook_oCStealContainer_CreateList)();
        
        if (CatInvCore::containerBySide[side] == this) {
            CatInvCore::backupListBySide[side] = this->contents;
        }
        
        bool needsFilter = CatInvCore::activeCategory != INV_CAT_ALL || CatInvCore::searchActive;
        if (CatInvCore::IsWorldReady() && needsFilter) {
            CatInvCore::FilterContainerByCategory(this);
        }
    }

    HOOK Hook_oCNpcContainer_CreateList PATCH(&oCNpcContainer::CreateList, &oCNpcContainer::CreateList_Union);
    void oCNpcContainer::CreateList_Union() {
        int side = this->right ? 1 : 0;
        
        CatInvCore::activeCategory = INV_CAT_ALL;
        
        if (CatInvCore::containerBySide[side] == this && CatInvCore::backupListBySide[side]) {
            this->contents = CatInvCore::backupListBySide[side];
        }
        
        THISCALL(Hook_oCNpcContainer_CreateList)();
        
        if (CatInvCore::containerBySide[side] == this) {
            CatInvCore::backupListBySide[side] = this->contents;
        }
    }

    HOOK Hook_oCItemContainer_Insert PATCH(&oCItemContainer::Insert, &oCItemContainer::Insert_Union);
    oCItem* oCItemContainer::Insert_Union(oCItem* item) {
        int side = this->right ? 1 : 0;
        
        bool isChest = !dynamic_cast<oCNpcInventory*>(this) && 
                       !dynamic_cast<oCStealContainer*>(this) && 
                       !dynamic_cast<oCNpcContainer*>(this);
        
        bool needsFilter = CatInvCore::activeCategory != INV_CAT_ALL || CatInvCore::searchActive;
        
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
        int side = this->right ? 1 : 0;
        
        bool isChest = !dynamic_cast<oCNpcInventory*>(this) && 
                       !dynamic_cast<oCStealContainer*>(this) && 
                       !dynamic_cast<oCNpcContainer*>(this);
        
        bool needsFilter = CatInvCore::activeCategory != INV_CAT_ALL || CatInvCore::searchActive;
        
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
    }
    
    HOOK Hook_oCNpcInventory_Insert PATCH(&oCNpcInventory::Insert, &oCNpcInventory::Insert_Union);
    oCItem* oCNpcInventory::Insert_Union(oCItem* item) {
        int side = this->right ? 1 : 0;
        
        if (CatInvCore::containerBySide[side] == this && CatInvCore::backupListBySide[side]) {
            this->contents = CatInvCore::backupListBySide[side];
        }
        
        oCItem* result = (oCItem*)THISCALL(Hook_oCNpcInventory_Insert)(item);
        
        if (CatInvCore::containerBySide[side] == this) {
            CatInvCore::backupListBySide[side] = this->contents;
        }
        
        return result;
    }
    
    HOOK Hook_oCNpcInventory_Remove PATCH(&oCNpcInventory::Remove, &oCNpcInventory::Remove_Union);
    oCItem* oCNpcInventory::Remove_Union(oCItem* item, int amount) {
        int side = this->right ? 1 : 0;
        
        if (CatInvCore::containerBySide[side] == this && CatInvCore::backupListBySide[side]) {
            this->contents = CatInvCore::backupListBySide[side];
        }
        
        oCItem* result = (oCItem*)THISCALL(Hook_oCNpcInventory_Remove)(item, amount);
        
        if (CatInvCore::containerBySide[side] == this) {
            CatInvCore::backupListBySide[side] = this->contents;
        }
        
        return result;
    }
    
    HOOK Hook_oCNpcInventory_RemoveByPtr PATCH(&oCNpcInventory::RemoveByPtr, &oCNpcInventory::RemoveByPtr_Union);
    oCItem* oCNpcInventory::RemoveByPtr_Union(oCItem* item, int amount) {
        int side = this->right ? 1 : 0;
        
        if (CatInvCore::containerBySide[side] == this && CatInvCore::backupListBySide[side]) {
            this->contents = CatInvCore::backupListBySide[side];
        }
        
        oCItem* result = (oCItem*)THISCALL(Hook_oCNpcInventory_RemoveByPtr)(item, amount);
        
        if (CatInvCore::containerBySide[side] == this) {
            CatInvCore::backupListBySide[side] = this->contents;
        }
        
        return result;
    }
    
    HOOK Hook_oCItemContainer_TransferItem PATCH(&oCItemContainer::TransferItem, &oCItemContainer::TransferItem_Union);
    int oCItemContainer::TransferItem_Union(int a, int b) {
        int result = (int)THISCALL(Hook_oCItemContainer_TransferItem)(a, b);
        
        bool needsFilter = CatInvCore::activeCategory != INV_CAT_ALL || CatInvCore::searchActive;
        
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

        if (CatInvOptions::G1Mode) {
            activeCategory = 1;
        } else {
            activeCategory = 0;
        }

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
        itemName.Upper();
        
        std::wstring searchUpper = searchText;
        std::transform(searchUpper.begin(), searchUpper.end(), searchUpper.begin(), ::towupper);
        
        std::wstring itemNameW = AToW(itemName.ToChar());
        std::transform(itemNameW.begin(), itemNameW.end(), itemNameW.begin(), ::towupper);
        
        return itemNameW.find(searchUpper) != std::wstring::npos;
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

        int posY = viewTitle->vposy + viewTitle->vsizey - 1;
        int posX;
        if (container->right) {
            posX = viewTitle->vposx;
        } else {
            posX = viewTitle->vposx + viewTitle->vsizex - width;
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
}