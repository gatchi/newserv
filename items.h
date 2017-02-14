#define ITEM_ERROR_DO_NOT_DELETE 0x50000000 // error code returned when a "use"d item should not be deleted from the player's inventory

int ClientUseItem(LOBBY* l,CLIENT* c,PLAYER_ITEM* item);

int SetupNonRareSystem();
int DecideNonRareType(bool box,DWORD det);
bool CreateNonRareItem(ITEM_DATA* item,bool box,int ep,int diff,int area,int secid);
