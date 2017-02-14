// raw item data
typedef struct {
    union {
        BYTE itemData1[12];
        WORD itemData1word[6];
        DWORD itemData1dword[3];
    };
    DWORD itemID;
    union {
        BYTE itemData2[4];
        WORD itemData2word[2];
        DWORD itemData2dword;
    };
} ITEM_DATA;

// an item in a player's inventory
typedef struct {
    WORD equipFlags;
    WORD techFlag;
    DWORD gameFlags;
    ITEM_DATA data; } PLAYER_ITEM;

// an item in a player's bank
typedef struct {
    ITEM_DATA data;
    WORD amount;
    WORD showflags; } PLAYER_BANK_ITEM;

// a player's inventory (remarkably, the format is the same in all versions of PSO)
typedef struct {
    BYTE numItems;
    BYTE hpMat;
    BYTE tpMat;
    BYTE language;
    PLAYER_ITEM items[30]; } PLAYER_INVENTORY;

// a player's bank
typedef struct {
    DWORD numItems;
    DWORD meseta;
    PLAYER_BANK_ITEM items[200]; } PLAYER_BANK;

////////////////////////////////////////////////////////////////////////////////

// simple player stats
typedef struct {
    WORD atp;
    WORD mst;
    WORD evp;
    WORD hp;
    WORD dfp;
    WORD ata;
    WORD lck; } PLAYER_STATS;

// PC/GC player appearance and stats data
typedef struct {
    PLAYER_STATS stats;
    WORD unknown1;
    DWORD unknown2[2];
    DWORD level;
    DWORD exp;
    DWORD meseta;
    char playername[16];
    DWORD unknown3[2];
    DWORD nameColor;
    BYTE extraModel;
    BYTE unused[15];
    DWORD nameColorChecksum;
    BYTE sectionID;
    BYTE charClass;
    BYTE v2flags;
    BYTE version;
    DWORD v1flags;
    WORD costume;
    WORD skin;
    WORD face;
    WORD head;
    WORD hair;
    WORD hairRed;
    WORD hairGreen;
    WORD hairBlue;
    float propX;
    float propY;
    BYTE config[0x48];
    BYTE techLevels[0x14]; } PCGC_PLAYER_DISPDATA; // 0xD0 in size 

// BB player appearance and stats data
typedef struct {
    PLAYER_STATS stats;
    WORD unknown1;
    DWORD unknown2[2];
    DWORD level;
    DWORD exp;
    DWORD meseta;
    char guildcard[16];
    DWORD unknown3[2];
    DWORD nameColor;
    BYTE extraModel;
    BYTE unused[11];
    DWORD playTime; // not actually a game field; used only by my server 
    DWORD nameColorChecksum;
    BYTE sectionID;
    BYTE charClass;
    BYTE v2flags;
    BYTE version;
    DWORD v1flags;
    WORD costume;
    WORD skin;
    WORD face;
    WORD head;
    WORD hair;
    WORD hairRed;
    WORD hairGreen;
    WORD hairBlue;
    float propX;
    float propY;
    wchar_t playername[16];
    BYTE config[0xE8];
    BYTE techLevels[0x14];
} BB_PLAYER_DISPDATA;

// BB player preview format
typedef struct {
    DWORD exp;
    DWORD level;
    char guildcard[16];
    DWORD unknown3[2];
    DWORD nameColor;
    BYTE extraModel;
    BYTE unused[15];
    DWORD nameColorChecksum;
    BYTE sectionID;
    BYTE charClass;
    BYTE v2flags;
    BYTE version;
    DWORD v1flags;
    WORD costume;
    WORD skin;
    WORD face;
    WORD head;
    WORD hair;
    WORD hairRed;
    WORD hairGreen;
    WORD hairBlue;
    float propX;
    float propY;
    wchar_t playername[16];
    DWORD playTime;
} BB_PLAYER_DISPDATA_PREVIEW;

////////////////////////////////////////////////////////////////////////////////

// GC guld card format
typedef struct {
    DWORD playerTag;
    DWORD serialNumber;
    char name[0x18];
    char desc[0x6C];
    BYTE reserved1; // should be 1 
    BYTE reserved2; // should be 1 
    BYTE sectionID;
    BYTE charClass;
} GC_GUILDCARD_DATA;

// BB guild card format
typedef struct {
    DWORD serialNumber;
    wchar_t name[0x0018];
    wchar_t teamname[0x0010];
    wchar_t desc[0x0058];
    BYTE reserved1; // should be 1 
    BYTE reserved2; // should be 1 
    BYTE sectionID;
    BYTE charClass;
} BB_GUILDCARD_DATA;

// an entry in the BB guild card file
typedef struct {
    BB_GUILDCARD_DATA data;
    BYTE unknown[0x00B4];
} BB_GUILDCARD_ENTRY;

// the format of the BB guild card file
typedef struct {
    BYTE unknown[0x1F84];
    BB_GUILDCARD_ENTRY entry[0x0068]; // that's 104 of them in decimal 
    BYTE unknown2[0x01AC];
} BB_GUILDCARD_FILE;

// PSOBB key config and team info
typedef struct {
    BYTE unknown[0x0114];        // 0000 
    BYTE keyConfig[0x016C];      // 0114 
    BYTE joystickConfig[0x0038]; // 0280 
    DWORD serialNumber;          // 02B8 
    DWORD teamID;                // 02BC 
    DWORD teamInfo[2];           // 02C0 
    WORD teamPrivilegeLevel;     // 02C8 
    WORD reserved;               // 02CA 
    wchar_t teamName[0x0010];    // 02CC 
    BYTE teamFlag[0x0800];       // 02EC 
    DWORD teamRewards[2];        // 0AEC 
} BB_KEY_TEAM_CONFIG;

// BB account data
typedef struct {
    BYTE symbolchats[0x04E0];
    BB_KEY_TEAM_CONFIG keyconfig;
    BB_GUILDCARD_FILE guildcards;
    DWORD option;
    BYTE shortcuts[0x0A40]; // chat shortcuts (@1FB4 in E7 command) 
} BB_PLAYER_ACCOUNT_DATA;

////////////////////////////////////////////////////////////////////////////////

// lobby data formats. these are used by lobby join and game join commands.

typedef struct {
    DWORD playerTag;
    DWORD guildcard;
    DWORD ipAddress;
    DWORD clientID;
    wchar_t playername[16]; } PC_PLAYER_LOBBY_DATA;

typedef struct {
    DWORD playerTag;
    DWORD guildcard;
    DWORD ipAddress;
    DWORD clientID;
    char playername[16]; } GC_PLAYER_LOBBY_DATA;

typedef struct {
    DWORD playerTag;
    DWORD guildcard;
    DWORD unknown1[5];
    DWORD clientID;
    wchar_t playername[16];
    DWORD unknown2; } BB_PLAYER_LOBBY_DATA;

////////////////////////////////////////////////////////////////////////////////

// PC player data (received in 61 command)
typedef struct {
    PLAYER_INVENTORY inventory;
    PCGC_PLAYER_DISPDATA disp;
} PC_PLAYER_DATA_FORMAT;

// GC player data (received in 61 command)
typedef struct {
    PLAYER_INVENTORY inventory;
    PCGC_PLAYER_DISPDATA disp;
    char unknown[0x134];
    char infoboard[0xAC];
    DWORD blocked[0x1E];
    DWORD autoReplyEnabled;
    char autoreply[1];
} GC_PLAYER_DATA_FORMAT;

// BB player data (received in 61 command)
typedef struct {
    PLAYER_INVENTORY inventory;
    BB_PLAYER_DISPDATA disp;
    char unused[0x174];
    wchar_t infoboard[0xAC];
    DWORD blocked[0x1E];
    DWORD autoReplyEnabled;
    wchar_t autoreply[1];
} BB_PLAYER_DATA_FORMAT;

// PC/GC lobby player data (used in lobby/game join commands)
typedef struct {
    PLAYER_INVENTORY inventory;
    PCGC_PLAYER_DISPDATA disp;
} PCGC_LOBBY_PLAYER_DATA_FORMAT;

// BB lobby player data (used in lobby/game join commands)
typedef struct {
    PLAYER_INVENTORY inventory;
    BB_PLAYER_DISPDATA disp;
} BB_LOBBY_PLAYER_DATA_FORMAT;

// complete BB player data format (used in E7 command)
typedef struct {
    PLAYER_INVENTORY inventory;      // 0000 // player 
    BB_PLAYER_DISPDATA disp;         // 034C // player 
    BYTE unknown[0x0010];            // 04DC // 
    DWORD optionFlags;               // 04EC // account 
    BYTE questData1[0x0208];         // 04F0 // player 
    PLAYER_BANK bank;                // 06F8 // player 
    DWORD serialNumber;              // 19C0 // player 
    wchar_t name[0x18];              // 19C4 // player 
    wchar_t teamname[0x10];          // 19C4 // player 
    wchar_t guildcarddesc[0x58];     // 1A14 // player 
    BYTE reserved1;                  // 1AC4 // player 
    BYTE reserved2;                  // 1AC5 // player 
    BYTE sectionID;                  // 1AC6 // player 
    BYTE charClass;                  // 1AC7 // player 
    DWORD unknown3;                  // 1AC8 // 
    BYTE symbolchats[0x04E0];        // 1ACC // account 
    BYTE shortcuts[0x0A40];          // 1FAC // account 
    wchar_t autoreply[0x00AC];       // 29EC // player 
    wchar_t infoboard[0x00AC];       // 2B44 // player 
    BYTE unknown5[0x001C];           // 2C9C // 
    BYTE challengeData[0x0140];      // 2CB8 // player 
    BYTE techMenuConfig[0x0028];     // 2DF8 // player 
    BYTE unknown6[0x002C];           // 2E20 // 
    BYTE questData2[0x0058];         // 2E4C // player 
    BB_KEY_TEAM_CONFIG keyConfig;    // 2EA4 // account 
} BB_COMPLETE_PLAYER_DATA_FORMAT; // total size: 39A0 

////////////////////////////////////////////////////////////////////////////////

// .nsc file format
typedef struct {
    char signature[0x40];
    BB_PLAYER_DISPDATA_PREVIEW preview;

    wchar_t               autoreply[0x00AC];      // player 
    PLAYER_BANK           bank;                   // player 
    BYTE                  challengeData[0x0140];  // player 
    BB_PLAYER_DISPDATA    disp;                   // player 
    wchar_t               guildcarddesc[0x58];    // player 
    wchar_t               infoboard[0x00AC];      // player 
    PLAYER_INVENTORY      inventory;              // player 
    BYTE                  questData1[0x0208];     // player 
    BYTE                  questData2[0x0058];     // player 
    BYTE                  techMenuConfig[0x0028]; // player 
} BB_SAVED_PLAYER_DATA_FORMAT;

// .nsa file format
typedef struct {
    char signature[0x40];
    DWORD                 blocked[0x001E];        // account 
    BB_GUILDCARD_FILE     guildcards;             // account 
    BB_KEY_TEAM_CONFIG    keyConfig;              // account 
    DWORD                 optionFlags;            // account 
    BYTE                  shortcuts[0x0A40];      // account 
    BYTE                  symbolchats[0x04E0];    // account 
    wchar_t               teamname[0x0010];       // account 
} BB_SAVED_ACCOUNT_DATA_FORMAT;

// complete player info stored by the server
typedef struct {
    DWORD                 loadedFromShipgateTime;

    wchar_t               autoreply[0x00AC];      // player 
    PLAYER_BANK           bank;                   // player 
    char                  bankname[0x20];
    DWORD                 blocked[0x001E];        // account 
    BYTE                  challengeData[0x0140];  // player 
    BB_PLAYER_DISPDATA    disp;                   // player 
    BYTE                  ep3config[0x2408];
    wchar_t               guildcarddesc[0x58];    // player 
    BB_GUILDCARD_FILE     guildcards;             // account 
    PLAYER_ITEM           identifyResult;
    wchar_t               infoboard[0x00AC];      // player 
    PLAYER_INVENTORY      inventory;              // player 
    BB_KEY_TEAM_CONFIG    keyConfig;              // account 
    DWORD                 optionFlags;            // account 
    BYTE                  questData1[0x0208];     // player 
    BYTE                  questData2[0x0058];     // player 
    DWORD                 serialNumber;
    BYTE                  shortcuts[0x0A40];      // account 
    BYTE                  symbolchats[0x04E0];    // account 
    wchar_t               teamname[0x0010];       // account 
    BYTE                  techMenuConfig[0x0028]; // player 
} PLAYER;

////////////////////////////////////////////////////////////////////////////////

#define ERROR_COMBINE_ITEM_SPLIT 0x5FFFFFFE // this error code is used to tell calling functions that a combine item needs to be split

// originally there was going to be a language-based header, but then I decided against it.
// these strings were already in use for that parser, so I didn't bother changing them.
#define PLAYER_FILE_SIGNATURE    "newserv player file format; 10 sections present; sequential;"
#define ACCOUNT_FILE_SIGNATURE   "newserv account file format; 7 sections present; sequential;"

void ConvertPCGCDispDataToBB(BB_PLAYER_DISPDATA* bb,PCGC_PLAYER_DISPDATA* pcgc);
void ConvertBBDispDataToPCGC(PCGC_PLAYER_DISPDATA* pcgc,BB_PLAYER_DISPDATA* bb);
void CreatePlayerPreview(BB_PLAYER_DISPDATA_PREVIEW* pre,BB_PLAYER_DISPDATA* bb);
void ApplyPlayerPreview(BB_PLAYER_DISPDATA* bb,BB_PLAYER_DISPDATA_PREVIEW* pre);

void ImportPlayerData(void* buffer,PLAYER* p,BYTE version);
void ExportLobbyPlayerData(void* buffer,PLAYER* p,BYTE version);
void ExportCompletePlayerData(BB_COMPLETE_PLAYER_DATA_FORMAT* bb,PLAYER* p);

long CalculateGuildCardChecksum(void* data,unsigned long size);
bool PlayerLoadAccountData(PLAYER* p,char* filename);
bool PlayerLoadAccountDataOldFormat(PLAYER* p,char* filename);
bool PlayerSaveAccountData(PLAYER* p,char* filename);
bool PlayerLoadPlayerData(PLAYER* p,char* filename);
bool PlayerLoadPlayerDataOldFormat(PLAYER* p,char* filename);
bool PlayerSavePlayerData(PLAYER* p,char* filename);
bool PlayerLoadBankData(PLAYER_BANK* b,char* filename);
bool PlayerSaveBankData(PLAYER_BANK* b,char* filename);
bool SwitchPlayerBank(PLAYER_BANK* b,char* saveFilename,char* loadFilename);

void ConvertToBankItem(PLAYER_BANK_ITEM* bankitem,PLAYER_ITEM* item);
void ConvertToInventoryItem(PLAYER_ITEM* item,PLAYER_BANK_ITEM* bankitem);
int AddItem(PLAYER* p,PLAYER_ITEM* item);
int AddItem(PLAYER_BANK* bank,PLAYER_BANK_ITEM* item);
int RemoveItem(PLAYER* p,DWORD itemID,DWORD amount,PLAYER_ITEM* item);
int RemoveItem(PLAYER_BANK* bank,DWORD itemID,DWORD amount,PLAYER_BANK_ITEM* item);
int FindItem(PLAYER_INVENTORY* inv,DWORD itemID);
int FindItem(PLAYER_BANK* bank,DWORD itemID);

//DWORD GetItemIdentifier(PLAYER_BANK_ITEM* pi)
//DWORD GetItemIdentifier(PLAYER_ITEM* pi)
//void PlayerUpdateInventory(PLAYER_INVENTORY* pi)
//DWORD PlayerUseItem(PLAYER* p,DWORD id)
