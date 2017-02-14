#define LOBBY_FLAG_HACKPROTECT      0x00000001 // enable hack protection 
#define LOBBY_FLAG_CHEAT            0x00000002 // [game] cheat mode enabled 
#define LOBBY_FLAG_GAME             0x00000004 // lobby is a game 
#define LOBBY_FLAG_PUBLIC           0x00000008 // [lobby] lobby is public 
#define LOBBY_FLAG_DEFAULT          0x00000010 // [lobby] lobby is numbered (default) 
#define LOBBY_FLAG_EP3              0x00000020 // [lobby] lobby is available only to ep3 players 
#define LOBBY_FLAG_QUESTING         0x00000040 // [game] quest is in progress 
#define LOBBY_FLAG_OPEN_QUESTING    0x00000080 // [game] joinable quest is in progress 

typedef struct {
    OPERATION_LOCK operation;
    DWORD lobbyID;

    DWORD minlevel;
    DWORD maxlevel;

    // item crap 
    DWORD numEnemies;
    GAME_ENEMY_ENTRY* enemies;
    BATTLE_PARAM* battleParamTable;
    ITEM_RARE_SET* rareSet;
    DWORD nextItemID[4];
    DWORD nextGameItemID;
    DWORD numItems;
    PLAYER_ITEM item;
    PLAYER_ITEM* items;
    DWORD variations[0x20];

    // game config 
    BYTE version;
    BYTE sectionID;
    BYTE episode;
    BYTE difficulty;
    BYTE mode;
    wchar_t password[36];
    wchar_t name[36];

    // when this is uncommented, make sure to change DeleteGame() to reflect this 
    //EP3_GAME_CONFIG* ep3; // only present if this is an Episode 3 game 

    // lobby stuff 
    BYTE event;
    BYTE block;
    BYTE type; // number to give to PSO for the lobby number 
    DWORD leaderID;
    DWORD maxClients;
    DWORD flags;
    DWORD questLoading; // for use with joinable quests 
    CLIENT* clients[12];
} LOBBY;

bool CheckLoadingPlayers(LOBBY* l);
unsigned int CountLobbyPlayers(LOBBY* l);
bool AddClient(LOBBY* lb,CLIENT* c);
bool RemoveClient(LOBBY* lb,CLIENT* c);
bool ChangeLobby(LOBBY* current,LOBBY* next,CLIENT* c);

BYTE CompatibleLobbyNumber(CLIENT* c,BYTE ln);
int ConvertLobbyEventToGameEvent(int event);
void DeleteLobby(LOBBY* g);

int AddItem(LOBBY* l,PLAYER_ITEM* item);
int RemoveItem(LOBBY* l,DWORD itemID,PLAYER_ITEM* item);
int FindItem(LOBBY* l,DWORD itemID);
DWORD GetNextItemID(LOBBY* l,DWORD player);

////////////////////////////////////////////////////////////////////////////////

/*
bool BankDepositItem(TEAM_INFO*,PLAYER_INVENTORY*,DWORD,BYTE); // player, item id, number of item 
DWORD BankWithdrawItem(TEAM_INFO*,PLAYER_INVENTORY*,DWORD,BYTE); // player, item id, number of item 
DWORD PlayerDropItem(TEAM_INFO*,PLAYER*,DWORD,DWORD); // team, player, item id, number of item 
bool PlayerPickUpItem(TEAM_INFO*,PLAYER*,DWORD); // team, player, item id 
DWORD ShopBuyItem(TEAM_INFO*,PLAYER*,BYTE);
bool ShopSellItem(TEAM_INFO*,PLAYER*,DWORD);
DWORD BankAction(TEAM_INFO*,PLAYER*,BYTE,BYTE,DWORD,DWORD);
bool TeamCreateItem(TEAM_INFO*,PLAYER_ITEM*); // like if a dude or box drops one 
bool TeamDeleteItem(TEAM_INFO*,DWORD); // team, item id 

int FindItem(PLAYER_INVENTORY*,DWORD);
int FindBankItem(PLAYER_INVENTORY*,DWORD);
int FindDroppedItem(TEAM_INFO*,DWORD);

bool IsItemIDInUse(TEAM_INFO*,DWORD);
void GetUnusedItemID(TEAM_INFO*,PLAYER_ITEM*,DWORD playernum);
void ChangeItemIDs(TEAM_INFO*,PLAYER*);
*/

