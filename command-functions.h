////////////////////////////////////////////////////////////////////////////////
// required defines and structures for some input/output functions 

// different client states used by the block server
#define BBSTATE_INITIAL_LOGIN 0x00 // initial connection. server will redirect client to another port.
#define BBSTATE_DOWNLOAD_DATA 0x01 // second connection. server will send client game data and account data.
#define BBSTATE_CHOOSE_CHAR   0x02 // third connection. choose character menu
#define BBSTATE_SAVE_CHAR     0x03 // fourth connection, used for saving characters only. if you do not create a character, server sets this state in order to skip it.
#define BBSTATE_SHIP_SELECT   0x04 // last connection. redirects client to login server.

// for PC/GC command 0x9A 
#define GC_LOGIN_ERROR_NONE            0x00000000 // ok (client replies with 9E) 
#define GC_LOGIN_ERROR_REGISTER        0x00000001 // ok (client replies with 9C) 
#define GC_LOGIN_ERROR_NONE2           0x00000002 // ok (client replies with 9E) 
#define GC_LOGIN_ERROR_ACCESSKEY       0x00000003 // invalid access key (client disconnects) 
#define GC_LOGIN_ERROR_SERIALNUMBER    0x00000004 // invalid serial number (client disconnects) 
#define GC_LOGIN_ERROR_NETWORKERROR    0x00000005 // network error? (client disconnects) 
#define GC_LOGIN_ERROR_NETWORKERROR2   0x00000006 // network error? (client disconnects) 
#define GC_LOGIN_ERROR_INVALIDLICENSE  0x00000007 // invalid license (client disconnects) 
#define GC_LOGIN_ERROR_EXPIREDLICENSE  0x00000008 // expired license (client disconnects) 
#define GC_LOGIN_ERROR_NETWORKERROR3   0x00000009 // network error? (client disconnects) 
#define GC_LOGIN_ERROR_NETWORKERROR4   0x0000000A // network error? (client disconnects) 
#define GC_LOGIN_ERROR_NOTREGISTERED   0x0000000B // license not registered (client disconnects) 
#define GC_LOGIN_ERROR_NOTREGISTERED2  0x0000000C // license not registered (client disconnects) 
#define GC_LOGIN_ERROR_NOTREGISTERED3  0x0000000D // license not registered (client disconnects) 
#define GC_LOGIN_ERROR_CONNECTION      0x0000000E // connection error (client disconnects) 
#define GC_LOGIN_ERROR_BANNED          0x0000000F // banned (client disconnects) 
#define GC_LOGIN_ERROR_BANNED2         0x00000010 // banned (client disconnects) 

// for CommandBBClientInit 
#define BB_LOGIN_ERROR_NONE        0x00000000
#define BB_LOGIN_ERROR_UNKNOWN     0x00000001
#define BB_LOGIN_ERROR_UNREG       0x00000002
#define BB_LOGIN_ERROR_UNREG2      0x00000003
#define BB_LOGIN_ERROR_MAINT       0x00000004
#define BB_LOGIN_ERROR_USERINUSE   0x00000005
#define BB_LOGIN_ERROR_BANNED      0x00000006
#define BB_LOGIN_ERROR_BANNED2     0x00000007
#define BB_LOGIN_ERROR_UNREG3      0x00000008
#define BB_LOGIN_ERROR_INVALID     0x00000009
#define BB_LOGIN_ERROR_LOCKED      0x0000000A
#define BB_LOGIN_ERROR_PATCH       0x0000000B

// for CommandShipSelect 
#define MENU_ITEM_FLAG_NOGAMECUBE         0x00000001
#define MENU_ITEM_FLAG_NOBLUEBURST        0x00000002
#define MENU_ITEM_FLAG_NOPC               0x00000004
#define MENU_ITEM_FLAG_NODC               0x00000008
#define MENU_ITEM_FLAG_VERSION_MASK       0x0000000F
#define MENU_ITEM_FLAG_REQ_MESSAGEBOX     0x40000000
#define MENU_ITEM_FLAG_INVISIBLE          0x80000000
#define MENU_FLAG_AUTOSELECT_ONE_CHOICE   0x00000001
#define MENU_FLAG_IS_INFO_MENU            0x00000002
#define MENU_FLAG_IS_DLQ_MENU             0x00000002
#define MENU_FLAG_USE_ITEM_IDS            0x00000004

typedef struct {
    wchar_t* name;
    bool (*GetDescription)(int,wchar_t*);
    DWORD flags;
    wchar_t* items[30];
    wchar_t* descriptions[30];
    DWORD itemFlags[30];
    DWORD itemIDs[30];
} SHIP_SELECT_MENU;

// for CommandPlayerStatsModify 
#define HP_SUB      0 // subtract HP
#define TP_SUB      1 // subtract TP
#define MESETA_SUB  2 // subtract meseta (strangely, there's no add meseta command; I assume it can be done by creating items)
#define HP_ADD      3 // add HP
#define TP_ADD      4 // add TP

// for ProcessCommand_GameCommand_Check
#define SUBCOMMAND_ERROR_NOT_PRESENT       0x40000001 // some data is missing from the command
#define SUBCOMMAND_ERROR_IMPERSONATION     0x40000002 // command has a different client's ID as the source
#define SUBCOMMAND_ERROR_SIZE_MISMATCH     0x40000003 // command is the wrong size
#define SUBCOMMAND_ERROR_INVALID_COMMAND   0x40000004 // command is invalid
#define SUBCOMMAND_ERROR_ABSORBED          0x4FFFFFFF // command is valid, but should not be echoed

// for ProcedureCheckToJoinGame 
#define JOINGAME_ERROR_FULL             0x70000000 // game is full
#define JOINGAME_ERROR_VERSION          0x70000001 // game is for a different version
#define JOINGAME_ERROR_QUEST            0x70000002 // game is in a quest
#define JOINGAME_ERROR_LOADING          0x70000003 // someone is loading in this game
#define JOINGAME_ERROR_SOLO             0x70000004 // game is solo mode
#define JOINGAME_ERROR_PASSWORD         0x70000005 // game has a password or incorrect password
#define JOINGAME_ERROR_LEVEL_TOO_LOW    0x70000006 // your level is too low
#define JOINGAME_ERROR_LEVEL_TOO_HIGH   0x70000007 // your level is too high

////////////////////////////////////////////////////////////////////////////////
// in command-functions.cpp 

// PSOPC command header
typedef struct {
    WORD size;
    BYTE command;
    BYTE flag;
} COMMAND_HEADER_PC;

// PSODC/PSOGC command header
typedef struct {
    BYTE command;
    BYTE flag;
    WORD size;
} COMMAND_HEADER_DCGC;

// PSOBB command header
typedef struct {
    WORD size;
    WORD command;
    DWORD flag;
} COMMAND_HEADER_BB;

// generic union for accessing data in subcommands
typedef union {
    DWORD dword;
    WORD word[2];
    BYTE byte[4];
} SUBCOMMAND;

// functions for translating commands between versions
bool TranslateCommandBBDCGC(COMMAND_HEADER_BB*,COMMAND_HEADER_DCGC*);
bool TranslateCommandBBPC(COMMAND_HEADER_BB*,COMMAND_HEADER_PC*);
bool TranslateCommandDCGCBB(COMMAND_HEADER_DCGC*,COMMAND_HEADER_BB*);
bool TranslateCommandDCGCPC(COMMAND_HEADER_DCGC*,COMMAND_HEADER_PC*);
bool TranslateCommandPCBB(COMMAND_HEADER_PC*,COMMAND_HEADER_BB*);
bool TranslateCommandPCDCGC(COMMAND_HEADER_PC*,COMMAND_HEADER_DCGC*);

////////////////////////////////////////////////////////////////////////////////
// in command-procedures.cpp and item-procedures.cpp 

int ProcedureChangeLobby(SERVER* s,LOBBY* current,LOBBY* next,CLIENT* c);
int ProcedureLoadQuest(CLIENT* c,QUEST* q,bool download);
int ProcedureCheckToJoinGame(LOBBY* l,CLIENT* c);
wchar_t* ProcedureCheckToJoinGame_ErrorString(int error);

int ProcedureInitializeClientItems(LOBBY* l,CLIENT* c);
int ProcedureSortClientItems(CLIENT* c,DWORD* itemIDs);

////////////////////////////////////////////////////////////////////////////////
// in command-input.cpp and command-input-subs.cpp 

int ReceiveCommandFromClient(CLIENT* c,void** buffer,WORD* command);
int ProcessCommands(SERVER* s,CLIENT* c, ...);
int ProcessSubcommand(SERVER* s,LOBBY* l,CLIENT* c,bool priv,int target,SUBCOMMAND* p,unsigned int num,bool hackprotect);

////////////////////////////////////////////////////////////////////////////////
// in command-output.cpp 

// the void** are arrays of void pointers to commands for each version 
int SendCommandToClient(CLIENT* c,void* data);
int SendCommandToClient(CLIENT* c,int command,int flag,void* data,int size);
int SendCommandToLobby(LOBBY* l,CLIENT* c,void** data);
int SendCommandToServer(SERVER* s,CLIENT* c,void** data);
int SendSubcommandToLobby(LOBBY* l,CLIENT* c,int command,int flag,void* sub,int size);

int CommandServerInit(CLIENT* c,bool startServer);
int CommandVerify(SERVER* s,CLIENT* c);
int CommandUpdateClientConfig(CLIENT* c);

int CommandApproveLicense(CLIENT* c);
int CommandApproveRegisteredLicense(CLIENT* c);

int CommandReconnect(CLIENT* c,DWORD address,int port);
int CommandSelectiveReconnectPC(CLIENT* c,DWORD address,int port);

int CommandBBClientInit(CLIENT* c,DWORD error);
int CommandBBSendKeyConfig(CLIENT* c);
int CommandBBSendPlayerPreview(CLIENT* c,DWORD playernum);
int CommandBBAcceptClientChecksum(CLIENT* c);
int CommandBBSendGuildCardHeader(CLIENT* c);
int CommandBBSendGuildCardData(CLIENT* c,DWORD chunk);
int CommandBBSendStreamFile(SERVER* s,CLIENT* c);
int CommandBBApprovePlayerChoice(CLIENT* c);
int CommandBBSendPlayerInfo(CLIENT* c);

int CommandPatchCheckDirectory(CLIENT* c,char* dir);
int CommandScrollingMessage(CLIENT* c,wchar_t* text);
int CommandMessageBox(CLIENT* c,wchar_t* text);
int CommandQuestInfo(CLIENT* c,wchar_t* text);

int CommandLobbyMessageBox(SERVER* s,LOBBY* li,CLIENT* c,wchar_t* text);
int CommandShipInfo(SERVER* s,LOBBY* li,CLIENT* c,wchar_t* text);
int CommandTextMessage(SERVER* s,LOBBY* li,CLIENT* c,wchar_t* text);
int CommandChat(LOBBY* li,CLIENT* c,wchar_t* text);

int CommandSendInfoBoard(LOBBY* l,CLIENT* c);
int CommandCardSearchResult(SERVER* s,CLIENT* c,CLIENT* target);
int CommandSendGuildCard(CLIENT* c,CLIENT* source);

int CommandShipSelect(SERVER* s,CLIENT* c,SHIP_SELECT_MENU* menu,DWORD* selection);
int CommandGameSelect(SERVER* s,CLIENT* c,DWORD* selection);
int CommandShipSelectAsQuestSelect(SERVER* s,CLIENT* c,SHIP_SELECT_MENU* menu,DWORD* selection);
int CommandQuestSelect(SERVER* s,CLIENT* c,QUESTLIST* ql,QUEST** selection);

int CommandSendLobbyList(SERVER* s,CLIENT* c);
int CommandBlankLobby(SERVER* s,CLIENT* c);
int CommandLobbyUpdateChallengeRank(SERVER* s,CLIENT* c);
int CommandLobbyJoin(LOBBY* l,CLIENT* c,bool notifyOthers);
int CommandLobbyAddPlayer(LOBBY* l,CLIENT* c);
int CommandLobbyDeletePlayer(LOBBY* l,int leaving);
int CommandLobbyChangeEvent(LOBBY* l);
int CommandLobbyChangeMarker(LOBBY* l);

int CommandLobbyName(CLIENT* c,wchar_t* name);
int CommandResumeGame(LOBBY* l,CLIENT* c);

int CommandPlayerStatsModify(LOBBY* l,CLIENT* c,BYTE stat,DWORD amount);
int CommandPlayerWarp(CLIENT* c,DWORD area);

int CommandDropItem(LOBBY* l,ITEM_DATA* item,bool dude,BYTE area,float x,float y,WORD request);
int CommandDropStackedItem(LOBBY* l,CLIENT* c,ITEM_DATA* item,BYTE area,float x,float y);
int CommandPickUpItem(LOBBY* l,CLIENT* c,DWORD id,BYTE area);
int CommandCreateItem(LOBBY* l,CLIENT* c,ITEM_DATA* item);
int CommandDestroyItem(LOBBY* l,CLIENT* c,DWORD itemID,DWORD amount);
int CommandBank(CLIENT* c);
int CommandShop(SERVER* s,CLIENT* c,DWORD type);
int CommandLevelUp(LOBBY* l,CLIENT* c);
int CommandGiveEXP(LOBBY* l,CLIENT* c,DWORD exp);

int CommandEp3SendCardUpdate(CLIENT* c);
int CommandEp3Rank(CLIENT* c);
int CommandEp3RankChange(SERVER* s,CLIENT* c);
int CommandEp3SendMapList(LOBBY* l);
int CommandEp3SendMapData(LOBBY* l,DWORD mapID);
int CommandEp3InitChangeState(SERVER* s,CLIENT* c,BYTE state);
int CommandEp3InitSendMapLayout(SERVER* s,CLIENT* c);
int CommandEp3InitSendNames(SERVER* s,CLIENT* c);
int CommandEp3InitSendDecks(SERVER* s,CLIENT* c);
int CommandEp3InitHandUpdate(SERVER* s,CLIENT* c,int player);
int CommandEp3InitStatUpdate(SERVER* s,CLIENT* c,int player);
int CommandEp3Init_B4_06(SERVER* s,CLIENT* c,bool game4p);
int CommandEp3Init_B4_4E(SERVER* s,CLIENT* c,int player);
int CommandEp3Init_B4_4C(SERVER* s,CLIENT* c,int player);
int CommandEp3Init_B4_4D(SERVER* s,CLIENT* c,int player);
int CommandEp3Init_B4_4F(SERVER* s,CLIENT* c,int player);
int CommandEp3Init_B4_50(SERVER* s,CLIENT* c);
int CommandEp3Init_B4_39(SERVER* s,CLIENT* c);
int CommandEp3InitBegin(SERVER* s,CLIENT* c);

int CommandLoadQuestFile(CLIENT* c,char* filename,bool downloadQuest);

int CommandInvisiblePlayer(SERVER* s,CLIENT* c);
int CommandVisiblePlayer(SERVER* s,CLIENT* c);
int CommandRevive(SERVER* s,CLIENT* c);

int CommandTime(CLIENT* c);
int CommandSave(CLIENT* c);

int CommandSimple(CLIENT* c,WORD command,DWORD sub);

