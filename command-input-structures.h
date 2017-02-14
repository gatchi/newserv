////////////////////////////////////////////////////////////////////////////////
// verify license DB 

typedef struct {
    COMMAND_HEADER_DCGC header;
    char unused[0x20];
    char serialNumber[0x10];
    char accessKey[0x10];
    char unused2[0x08];
    DWORD subversion;
    char unused3[0x60];
    char password[0x10];
} COMMAND_GC_LICENSEVERIFY; // DB 

////////////////////////////////////////////////////////////////////////////////
// user login 9A 9C 9D 9E 0093 

typedef struct {
    COMMAND_HEADER_PC header;
    char unused[0x20];
    char serialNumber[0x10];
    char accessKey[0x10];
} COMMAND_PC_LOGIN_A; // 9A 

typedef struct {
    union {
        COMMAND_HEADER_PC headerpc;
        COMMAND_HEADER_DCGC headergc;
    };
    char unused[8];
    DWORD subversion;
    DWORD unused2;
    char serialNumber[0x30];
    char accessKey[0x30];
    char password[0x30];
} COMMAND_PCGC_LOGIN_C; // 9C 

typedef struct {
    union {
        COMMAND_HEADER_PC headerpc;
        COMMAND_HEADER_DCGC headergc;
    };
    char unused[0x10];
    BYTE subversion;
    BYTE unused2[0x27];
    char serialNumber[0x10];
    char accessKey[0x10];
    char unused3[0x60];
    char name[0x10];
    CLIENTCONFIG cfg;
} COMMAND_PCGC_LOGIN_E; // 9D 9E 

typedef struct {
    COMMAND_HEADER_BB header;
    char unused[0x14];
    char username[0x10];
    char unused2[0x20];
    char password[0x10];
    char unused3[0x30];
    CLIENTCONFIG_BB cfg;
} COMMAND_BB_LOGIN; // 0093 

typedef struct {
    COMMAND_HEADER_DCGC header;
    wchar_t name[0x12];
    WORD ports[4];
} COMMAND_FS_LOGIN; // 0E 

////////////////////////////////////////////////////////////////////////////////
// client checksum 96 

typedef struct {
    COMMAND_HEADER_DCGC header;
    long long checksum;
} COMMAND_GC_CLIENTCHECKSUM; // 96 

////////////////////////////////////////////////////////////////////////////////
// player data 61 98 0061 0098 

typedef struct {
    COMMAND_HEADER_PC header;
    PC_PLAYER_DATA_FORMAT data;
} COMMAND_PC_PLAYERDATA; // 61 

typedef struct {
    COMMAND_HEADER_DCGC header;
    GC_PLAYER_DATA_FORMAT data;
} COMMAND_GC_PLAYERDATA; // 61 

typedef struct {
    COMMAND_HEADER_BB header;
    BB_PLAYER_DATA_FORMAT data;
} COMMAND_BB_PLAYERDATA; // 0061 

////////////////////////////////////////////////////////////////////////////////
// menu selection 09 10 84 0009 0010 0084 

typedef struct {
    union {
        COMMAND_HEADER_PC headerpc;
        COMMAND_HEADER_DCGC headerdcgc;
    };
    DWORD menuID;
    DWORD itemID;
    union {
        wchar_t passwordpc[0];
        char passworddcgc[0];
    };
} COMMAND_DCPCGC_MENUSELECTION; // 09 10 84 

typedef struct {
    COMMAND_HEADER_BB header;
    DWORD menuID;
    DWORD itemID;
    wchar_t password[0];
} COMMAND_BB_MENUSELECTION; // 0009 0010 0084 

////////////////////////////////////////////////////////////////////////////////
// chat 06 0006 

typedef struct {
    COMMAND_HEADER_PC header;
    DWORD unused[2];
    wchar_t text[0];
} COMMAND_PC_CHAT; // 06 

typedef struct {
    COMMAND_HEADER_DCGC header;
    DWORD unused[2];
    char text[0];
} COMMAND_DCGC_CHAT; // 06 

typedef struct {
    COMMAND_HEADER_BB header;
    DWORD unused[2];
    wchar_t text[0];
} COMMAND_BB_CHAT; // 0006 

////////////////////////////////////////////////////////////////////////////////
// set info board D9 00D9 

typedef struct {
    COMMAND_HEADER_PC header;
    wchar_t text[0];
} COMMAND_PC_SETINFOBOARD; // D9 

typedef struct {
    COMMAND_HEADER_DCGC header;
    char text[0];
} COMMAND_DCGC_SETINFOBOARD; // D9 

typedef struct {
    COMMAND_HEADER_BB header;
    wchar_t text[0];
} COMMAND_BB_SETINFOBOARD; // 00D9 

////////////////////////////////////////////////////////////////////////////////
// card search 40 0040 

typedef struct {
    union {
        COMMAND_HEADER_PC headerpc;
        COMMAND_HEADER_DCGC headergc;
    };
    DWORD playerTag;
    DWORD searcherSerialNumber;
    DWORD targetSerialNumber;
} COMMAND_DCPCGC_CARDSEARCH; // 40 

typedef struct {
    COMMAND_HEADER_BB header;
    DWORD playerTag;
    DWORD searcherSerialNumber;
    DWORD targetSerialNumber;
} COMMAND_BB_CARDSEARCH; // 0040 

////////////////////////////////////////////////////////////////////////////////
// simple mail 81 

typedef struct {
    COMMAND_HEADER_DCGC header;
    DWORD unknown[6];
    DWORD targetSerialNumber;
} COMMAND_GC_SIMPLEMAIL; // 81 

////////////////////////////////////////////////////////////////////////////////
// mail autoreply enable C7 00C7 

typedef struct {
    COMMAND_HEADER_PC header;
    wchar_t text[0];
} COMMAND_PC_ENABLEAUTOREPLY; // C7 

typedef struct {
    COMMAND_HEADER_DCGC header;
    char text[0];
} COMMAND_DCGC_ENABLEAUTOREPLY; // C7 

typedef struct {
    COMMAND_HEADER_BB header;
    wchar_t text[0];
} COMMAND_BB_ENABLEAUTOREPLY; // 00C7 

////////////////////////////////////////////////////////////////////////////////
// blocked senders list C6 00C6 

typedef struct {
    union {
        COMMAND_HEADER_PC headerpc;
        COMMAND_HEADER_DCGC headergc;
    };
    DWORD blocked[0x1E];
} COMMAND_DCPCGC_BLOCKEDSENDERS; // C6 

typedef struct {
    COMMAND_HEADER_BB header;
    DWORD blocked[0x1E];
} COMMAND_BB_BLOCKEDSENDERS; // 00C6 

////////////////////////////////////////////////////////////////////////////////
// ep3 jukebox BA 

typedef struct {
    COMMAND_HEADER_DCGC header; // flag = 0x03 
    DWORD unknown[3]; // should be FFFFFFFF 00000000 
} COMMAND_GC_JUKEBOX; // BA 

////////////////////////////////////////////////////////////////////////////////
// ep3 game data command CA 

typedef struct {
    COMMAND_HEADER_DCGC header;
    SUBCOMMAND entry[0];
} COMMAND_GC_EP3GAMEDATA; // CA 

////////////////////////////////////////////////////////////////////////////////
// game command / subcommand 60 62 6C 6D C9 CB 0060 0062 006C 006D 

typedef struct {
    union {
        COMMAND_HEADER_PC headerpc;
        COMMAND_HEADER_DCGC headerdcgc;
    };
    SUBCOMMAND entry[0];
} COMMAND_DCPCGC_SUBCOMMAND; // 60 62 6C 6D C9 CB 

typedef struct {
    COMMAND_HEADER_BB header;
    SUBCOMMAND entry[0];
} COMMAND_BB_SUBCOMMAND; // 60 62 6C 6D 

////////////////////////////////////////////////////////////////////////////////
// game create C1 EC 00C1 

typedef struct {
    COMMAND_HEADER_PC header;
    DWORD unused[2];
    wchar_t name[0x10];
    wchar_t password[0x10];
    BYTE difficulty;
    BYTE battleMode;
    BYTE challengeMode;
    BYTE unused2;
} COMMAND_PC_CREATEGAME; // C1 

typedef struct {
    COMMAND_HEADER_DCGC header;
    DWORD unused[2];
    char name[0x10];
    char password[0x10];
    BYTE difficulty;
    BYTE battleMode;
    BYTE challengeMode;
    BYTE episode;
} COMMAND_DCGC_CREATEGAME; // C1 EC 

typedef struct {
    COMMAND_HEADER_BB header;
    DWORD unused[2];
    wchar_t name[0x10];
    wchar_t password[0x10];
    BYTE difficulty;
    BYTE battleMode;
    BYTE challengeMode;
    BYTE episode;
    BYTE soloMode;
    BYTE unused2[3];
} COMMAND_BB_CREATEGAME; // 00C1 

////////////////////////////////////////////////////////////////////////////////
// blue burst only commands 00E3 00E5 00E7 03DC 01ED 01E8 03E8 

typedef struct {
    COMMAND_HEADER_BB header;
    DWORD playerNum;
    DWORD unused;
} COMMAND_BB_PLAYERPREVIEWREQUEST; // 00E3 

typedef struct {
    COMMAND_HEADER_BB header;
    DWORD playernum;
    BB_PLAYER_DISPDATA_PREVIEW preview;
} COMMAND_BB_CREATECHAR; // 00E5 

typedef struct {
    COMMAND_HEADER_BB header;
    BB_COMPLETE_PLAYER_DATA_FORMAT data;
} COMMAND_BB_PLAYERSAVE; // 00E7 

typedef struct {
    COMMAND_HEADER_BB header;
    DWORD unknown;
    DWORD chunkRequested;
    DWORD cont;
} COMMAND_BB_GUILDCARDREQUEST; // 03DC 

typedef struct {
    COMMAND_HEADER_BB header;
    union {
        DWORD option; // 01ED 
        BYTE symbolchats[0x4E0]; // 02ED 
        BYTE chatshortcuts[0xA40]; // 03ED 
        BYTE keyconfig[0x16C]; // 04ED 
        BYTE padconfig[0x38]; // 05ED 
        BYTE techmenu[0x28]; // 06ED 
        BYTE customize[0xE8]; // 07ED 
    };
} COMMAND_BB_WRITEACCOUNTDATA;

/*typedef struct {
    COMMAND_HEADER_BB header;
    union {
        DWORD checksum; // 01E8 
*/

////////////////////////////////////////////////////////////////////////////////
// load gba mini game D7 

typedef struct {
    COMMAND_HEADER_DCGC header;
    char filename[0];
} COMMAND_GC_LOADMINIGAME; // D7 

////////////////////////////////////////////////////////////////////////////////
// patch server commands 04 

typedef struct {
    COMMAND_HEADER_PC header;
    DWORD unused[3];
    char username[0x10];
    char password[0x10];
} COMMAND_PATCH_LOGININFO; // 04 

