// Supported with union (c) 2020 Union team
// User API for oCItemContainer
// CatInv plugin hooks

void OpenPassive_Union(int, int, int);
void Close_Union();
void DrawCategory_Union();
void Container_PrepareDraw_Union();
void NextItem_Union();
void PrevItem_Union();
void NextItemLine_Union();
void PrevItemLine_Union();
int HandleEvent_Union(int);
void CheckSelectedItem_Union();
int TransferItem_Union(int, int);
oCItem* Insert_Union(oCItem*);
void Remove_Union(oCItem*);
