////////////////////////////////////////////////////////////////////////////////
// encryption init 02 17 0003 

typedef struct {
    COMMAND_HEADER_PC header;
    char copyright[0x40];
    DWORD serverkey;
    DWORD clientkey;
    char aftermessage[0];
} COMMAND_PCPATCH_ENCRYPTIONINIT; // 02 17 

typedef struct {
    COMMAND_HEADER_DCGC header;
    char copyright[0x40];
    DWORD serverkey;
    DWORD clientkey;
    char aftermessage[0];
} COMMAND_GC_ENCRYPTIONINIT; // 02 17 

typedef struct {
    COMMAND_HEADER_BB header;
    char copyright[0x60];
    BYTE serverkey[0x30];
    BYTE clientkey[0x30];
    char aftermessage[0];
} COMMAND_BB_ENCRYPTIONINIT; // 00E6 

////////////////////////////////////////////////////////////////////////////////
// set security data 04 00E6 

typedef struct {
    union {
        COMMAND_HEADER_DCGC headerdcgc;
        COMMAND_HEADER_PC headerpc;
    };
    DWORD playerTag;
    DWORD serialNumber;
    CLIENTCONFIG cfg;
} COMMAND_DCPCGC_SETSECURITY; // 04 

typedef struct {
    COMMAND_HEADER_BB header;
    DWORD error; // see below 
    DWORD playerTag;
    DWORD serialNumber;
    DWORD teamID; // just randomize it; teams aren't supported 
    CLIENTCONFIG_BB cfg;
    DWORD caps; // should be 0x00000102 
} COMMAND_BB_SETSECURITY; // 00E6 

////////////////////////////////////////////////////////////////////////////////
// reconnect 19 0019 

typedef struct {
    union {
        COMMAND_HEADER_PC headerpc;
        COMMAND_HEADER_DCGC headerdcgc;
    };
    DWORD address;
    WORD port;
    WORD unused;
} COMMAND_DCPCGC_RECONNECT; // 19 

typedef struct {
    COMMAND_HEADER_BB header;
    DWORD address;
    WORD port;
    WORD unused;
} COMMAND_BB_RECONNECT; // 0019 

////////////////////////////////////////////////////////////////////////////////
// bb key config 00E2 

typedef struct {
    COMMAND_HEADER_BB header;
    BB_KEY_TEAM_CONFIG keyconfig;
} COMMAND_BB_KEYCONFIG; // 00E2 

////////////////////////////////////////////////////////////////////////////////
// bb no player preview present 00E4 

typedef struct {
    COMMAND_HEADER_BB header;
    DWORD playernum;
    DWORD error; // 0x00000002 
} COMMAND_BB_NOPLAYERPREVIEW; // 00E4 

////////////////////////////////////////////////////////////////////////////////
// bb player preview 00E5

typedef struct {
    COMMAND_HEADER_BB header;
    DWORD playernum;
    BB_PLAYER_DISPDATA_PREVIEW preview;
} COMMAND_BB_PLAYERPREVIEW; // 00E5 

////////////////////////////////////////////////////////////////////////////////
// bb player data 00E7 

typedef struct {
    COMMAND_HEADER_BB header;
    BB_COMPLETE_PLAYER_DATA_FORMAT data;
} COMMAND_BB_PLAYERSAVE; // 00E7 

////////////////////////////////////////////////////////////////////////////////
// bb accept client checksum (from 01E8) 02E8 

typedef struct {
    COMMAND_HEADER_BB header;
    DWORD verify; // should be 1 
    DWORD unused;
} COMMAND_BB_ACCEPTCLIENTCHECKSUM; // 02DC 

////////////////////////////////////////////////////////////////////////////////
// bb guild card header 01DC 

typedef struct {
    COMMAND_HEADER_BB header;
    DWORD unknown; // should be 1 
    DWORD filesize; // 0x0000490 
    DWORD checksum;
} COMMAND_BB_GUILDCARDHEADER; // 01DC 

////////////////////////////////////////////////////////////////////////////////
// bb guild card chunk 02DC 

typedef struct {
    COMMAND_HEADER_BB header;
    DWORD unknown;
    DWORD chunkID;
    BYTE data[0];
} COMMAND_BB_GUILDCARDCHUNK; // 02DC 

////////////////////////////////////////////////////////////////////////////////
// bb stream file index 01EB 

typedef struct {
    WORD size;
    WORD command;
    DWORD num;
    struct {
        DWORD size;
        DWORD checksum;
        DWORD offset;
        char filename[0x40];
    } entry[0];
} COMMAND_BB_STREAMFILEINDEX; // 01EB 

////////////////////////////////////////////////////////////////////////////////
// bb stream file chunk 02EB 

typedef struct {
    COMMAND_HEADER_BB header;
    DWORD chunknum;
    BYTE data[0];
} COMMAND_BB_STREAMFILECHUNK; // 02EB 

////////////////////////////////////////////////////////////////////////////////
// patch check directory 09 

typedef struct {
    COMMAND_HEADER_PC header;
    char dir[0x40];
} COMMAND_PATCH_CHECKDIR; // 001A 

////////////////////////////////////////////////////////////////////////////////
// large message box 13 1A D5 001A 
// room name 8A 008A 
// quest description A3 00A3 

typedef struct {
    COMMAND_HEADER_PC header;
    wchar_t text[0];
} COMMAND_PC_LARGEMESSAGE; // 13 1A 8A A3 D5 

typedef struct {
    COMMAND_HEADER_DCGC header;
    char text[0];
} COMMAND_GC_LARGEMESSAGE; // 1A 8A A3 D5 

typedef struct {
    COMMAND_HEADER_BB header;
    wchar_t text[0];
} COMMAND_BB_LARGEMESSAGE; // 001A 008A 00A3 

////////////////////////////////////////////////////////////////////////////////
// lobby message box 01 0001 
// chat 06 0006 
// ship info 11 0011 
// text message B0 00B0 
// bb scrolling message 00EE 

typedef struct {
    COMMAND_HEADER_PC header;
    DWORD unused;
    DWORD serialNumber;
    wchar_t text[0];
} COMMAND_PC_MESSAGE; // 01 06 11 B0 

typedef struct {
    COMMAND_HEADER_DCGC header;
    DWORD unused;
    DWORD serialNumber;
    char text[0];
} COMMAND_GC_MESSAGE; // 01 06 11 B0 

typedef struct {
    COMMAND_HEADER_BB header;
    DWORD unused;
    DWORD serialNumber;
    wchar_t text[0];
} COMMAND_BB_MESSAGE; // 0001 0006 0011 00B0 00EE 

////////////////////////////////////////////////////////////////////////////////
// info board D8 00D8 

typedef struct { // this format is probably incorrect 
    COMMAND_HEADER_PC header;
    struct {
        wchar_t name[0x10];
        wchar_t message[0xAC];
    } entry[0];
} COMMAND_PC_INFOBOARD; // D8 

typedef struct {
    COMMAND_HEADER_DCGC header;
    struct {
        char name[0x10];
        char message[0xAC];
    } entry[0];
} COMMAND_GC_INFOBOARD; // D8 

typedef struct {
    COMMAND_HEADER_BB header;
    struct {
        wchar_t name[0x10];
        wchar_t message[0xAC];
    } entry[0];
} COMMAND_BB_INFOBOARD; // 00D8 

////////////////////////////////////////////////////////////////////////////////
// card search result 40 0040 

typedef struct {
    COMMAND_HEADER_DCGC header;
    DWORD playerTag;
    DWORD searcherSerialNumber;
    DWORD targetSerialNumber;
    COMMAND_DCPCGC_RECONNECT usedReconnectCommand;
    char locationString[0x44];
    DWORD menuID;
    DWORD lobbyID;
    char unused[0x3C];
    wchar_t name[0x20];
} COMMAND_DCGC_CARDSEARCHRESULT; // 40 

typedef struct {
    COMMAND_HEADER_BB header;
    DWORD playerTag;
    DWORD searcherSerialNumber;
    DWORD targetSerialNumber;
    COMMAND_BB_RECONNECT usedReconnectCommand;
    wchar_t locationString[0x44];
    DWORD menuID;
    DWORD lobbyID;
    char unused[0x3C];
    wchar_t name[0x20];
} COMMAND_BB_CARDSEARCHRESULT; // 0040 

////////////////////////////////////////////////////////////////////////////////
// send guild card 60/06 

typedef struct {
    COMMAND_HEADER_DCGC header;
    BYTE subcommand;
    BYTE subsize;
    WORD unused;
    GC_GUILDCARD_DATA data;
} COMMAND_GC_SENDGUILDCARD;

typedef struct {
    COMMAND_HEADER_BB header;
    BYTE subcommand;
    BYTE subsize;
    WORD unused;
    BB_GUILDCARD_DATA data;
} COMMAND_BB_SENDGUILDCARD;

////////////////////////////////////////////////////////////////////////////////
// ship select 07 1F 0007 001F 

typedef struct {
    COMMAND_HEADER_PC header;
    struct {
        DWORD menuID;
        DWORD itemID;
        WORD flags; // should be 0x0F04 
        wchar_t text[17];
    } entry[0];
} COMMAND_PC_SHIPSELECT; // 07 1F 

typedef struct {
    COMMAND_HEADER_DCGC header;
    struct {
        DWORD menuID;
        DWORD itemID;
        WORD flags; // should be 0x0F04 
        char text[18];
    } entry[0];
} COMMAND_GC_SHIPSELECT; // 07 1F 

typedef struct {
    COMMAND_HEADER_BB header;
    struct {
        DWORD menuID;
        DWORD itemID;
        WORD flags; // should be 0x0004 
        wchar_t text[17];
    } entry[0];
} COMMAND_BB_SHIPSELECT; // 0007 001F 

////////////////////////////////////////////////////////////////////////////////
// lobby list 83 0083 

typedef struct {
    union {
        COMMAND_HEADER_PC headerpc;
        COMMAND_HEADER_DCGC headerdcgc;
    };
    struct {
        DWORD menuID;
        DWORD itemID;
        DWORD unused; // should be 0x00000000 
    } entry[0];
} COMMAND_DCPCGC_LOBBYLIST; // 83 

typedef struct {
    COMMAND_HEADER_BB header;
    struct {
        DWORD menuID;
        DWORD itemID;
        DWORD unused; // should be 0x00000000 
    } entry[0];
} COMMAND_BB_LOBBYLIST; // 0083 

////////////////////////////////////////////////////////////////////////////////
// challenge rank C5 00C5 

typedef struct {
    COMMAND_HEADER_PC header;
    struct {
        DWORD serialNumber;
        BYTE data[0x0118];
    } entry[0];
} COMMAND_PC_CHALLENGERANK; // C5 

typedef struct {
    COMMAND_HEADER_DCGC header;
    struct {
        DWORD serialNumber;
        BYTE data[0x0118];
    } entry[0];
} COMMAND_GC_CHALLENGERANK; // C5 

typedef struct {
    COMMAND_HEADER_BB header;
    struct {
        DWORD clientID;
        BYTE data[0x0154];
    } entry[0];
} COMMAND_BB_CHALLENGERANK; // 00C5 

////////////////////////////////////////////////////////////////////////////////
// join lobby 65 67 68 0065 0067 0068 

typedef struct {
    COMMAND_HEADER_PC header;
    BYTE clientID;
    BYTE leaderID;
    BYTE disableUDP;
    BYTE lobbyNumber;
    WORD blockNumber;
    WORD event;
    DWORD unused; // should be 0 
    struct {
        PC_PLAYER_LOBBY_DATA lobbyData;
        PCGC_LOBBY_PLAYER_DATA_FORMAT data;
    } entry[0];
} COMMAND_PC_JOINLOBBY; // 65 67 68 

typedef struct {
    COMMAND_HEADER_DCGC header;
    BYTE clientID;
    BYTE leaderID;
    BYTE disableUDP;
    BYTE lobbyNumber;
    WORD blockNumber;
    WORD event;
    DWORD unused; // should be 0 
    struct {
        GC_PLAYER_LOBBY_DATA lobbyData;
        PCGC_LOBBY_PLAYER_DATA_FORMAT data;
    } entry[0];
} COMMAND_GC_JOINLOBBY; // 65 67 68 

typedef struct {
    COMMAND_HEADER_BB header;
    BYTE clientID;
    BYTE leaderID;
    BYTE unused;
    BYTE lobbyNumber;
    WORD blockNumber;
    WORD event;
    DWORD unused2;
    struct {
        BB_PLAYER_LOBBY_DATA lobbyData;
        BB_LOBBY_PLAYER_DATA_FORMAT data;
    } entry[0];
} COMMAND_BB_JOINLOBBY; // 0065 0067 0068 

////////////////////////////////////////////////////////////////////////////////
// leave lobby 66 69 0066 0069 

typedef struct {
    union {
        COMMAND_HEADER_PC headerpc;
        COMMAND_HEADER_DCGC headerdcgc;
    };
    BYTE clientID;
    BYTE leaderID;
    WORD unused; // should be 0 
} COMMAND_DCPCGC_LEAVELOBBY; // 66 69 

typedef struct {
    COMMAND_HEADER_BB header; // flag should be the same as clientID 
    BYTE clientID;
    BYTE leaderID;
    WORD unused; // should be 0 
} COMMAND_BB_LEAVELOBBY; // 0066 0069 

////////////////////////////////////////////////////////////////////////////////
// arrow update 88 0088 

typedef struct {
    union {
        COMMAND_HEADER_PC headerpc;
        COMMAND_HEADER_DCGC headerdcgc;
    };
    struct {
        DWORD playerTag;
        DWORD serialNumber;
        DWORD arrowColor;
    } entry[0];
} COMMAND_DCPCGC_ARROWUPDATE; // 88 

typedef struct {
    COMMAND_HEADER_BB header;
    struct {
        DWORD playerTag;
        DWORD serialNumber;
        DWORD arrowColor;
    } entry[0];
} COMMAND_BB_ARROWUPDATE; // 0088 

////////////////////////////////////////////////////////////////////////////////
// game select 08 0008 

typedef struct {
    COMMAND_HEADER_PC header;
    struct {
        DWORD menuID;
        DWORD gameID;
        BYTE difficultyTag; // (s->teams[x]->episode == 0xFF ? 0x0A : s->teams[x]->difficulty + 0x22);
        BYTE numPlayers;
        wchar_t name[0x10];
        BYTE episode;
        BYTE flags;
    } entry[0];
} COMMAND_PC_GAMESELECT; // 08 

typedef struct {
    COMMAND_HEADER_DCGC header;
    struct {
        DWORD menuID;
        DWORD gameID;
        BYTE difficultyTag; // (s->teams[x]->episode == 0xFF ? 0x0A : s->teams[x]->difficulty + 0x22);
        BYTE numPlayers;
        char name[0x10];
        BYTE episode;
        BYTE flags; // ep3: =(password[0] ? 2 : 0) // ep12: =((episode << 6) | (mode << 4) | (password[0] ? 2 : 0))
    } entry[0];
} COMMAND_GC_GAMESELECT; // 08 

typedef struct {
    COMMAND_HEADER_BB header;
    struct {
        DWORD menuID;
        DWORD gameID;
        BYTE difficultyTag; // (s->teams[x]->episode == 0xFF ? 0x0A : s->teams[x]->difficulty + 0x22);
        BYTE numPlayers;
        wchar_t name[0x10];
        BYTE episode; // =0x40+episode 
        BYTE flags; // =((mode < 3 ? (mode << 4) : 0) | (password[0] ? 2 : 0) | (mode == 3 ? 4 : 0));
    } entry[0];
} COMMAND_BB_GAMESELECT; // 0008 

////////////////////////////////////////////////////////////////////////////////
// join game 64 0064 

typedef struct {
    COMMAND_HEADER_PC header; // flag = numPlayers 
    DWORD variations[0x20];
    PC_PLAYER_LOBBY_DATA lobbyData[4];
    BYTE clientID;
    BYTE leaderID;
    BYTE unused; // may be disableUDP; set it to 0x00 
    BYTE difficulty;
    BYTE battleMode;
    BYTE event;
    BYTE sectionID;
    BYTE challengeMode;
    DWORD gameID; // actually random number for rare monster selection; whatever 
    DWORD episode; // for PSOPC, this must be 0x00000100 
} COMMAND_PC_JOINGAME; // 64 

typedef struct {
    COMMAND_HEADER_DCGC header; // flag = numPlayers 
    DWORD variations[0x20];
    GC_PLAYER_LOBBY_DATA lobbyData[4];
    BYTE clientID;
    BYTE leaderID;
    BYTE unused; // may be disableUDP; set it to 0x01 
    BYTE difficulty;
    BYTE battleMode;
    BYTE event;
    BYTE sectionID;
    BYTE challengeMode;
    DWORD gameID; // actually random number for rare monster selection; whatever 
    DWORD episode;
    struct {
        PLAYER_INVENTORY inventory;
        PCGC_PLAYER_DISPDATA disp;
    } entry[4]; // only used on ep3 
} COMMAND_GC_JOINGAME; // 64 

typedef struct {
    COMMAND_HEADER_BB header; // flag = numPlayers 
    DWORD variations[0x20];
    BB_PLAYER_LOBBY_DATA lobbyData[4];
    BYTE clientID;
    BYTE leaderID;
    BYTE unused; // may be disableUDP; set it to 0x01 
    BYTE difficulty;
    BYTE battleMode;
    BYTE event;
    BYTE sectionID;
    BYTE challengeMode;
    DWORD gameID; // actually random number for rare monster selection; whatever 
    BYTE episode;
    BYTE unused2;
    BYTE solomode;
    BYTE unused3;
} COMMAND_BB_JOINGAME; // 0064 

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
// BB game commands 0060 

typedef struct {
    COMMAND_HEADER_BB header;
    BYTE subcommand;
    BYTE subsize;
    WORD clientID;
    DWORD itemID;
    DWORD amount;
} COMMAND_BB_DESTROY_ITEM; // 29 

typedef struct {
    COMMAND_HEADER_BB header;
    BYTE subcommand;
    BYTE subsize;
    WORD clientID;
    WORD clientID2;
    WORD area;
    DWORD itemID;
} COMMAND_BB_PICK_UP_ITEM; // 59 

typedef struct {
    COMMAND_HEADER_BB header;
    BYTE subcommand;
    BYTE subsize;
    WORD unused;
    WORD area;
    WORD unused2;
    float x;
    float y;
    ITEM_DATA data;
} COMMAND_BB_DROP_STACK_ITEM; // 5D 

typedef struct {
    COMMAND_HEADER_BB header;
    BYTE subcommand;
    BYTE subsize;
    WORD unused;
    BYTE area;
    BYTE dude;
    WORD requestID;
    float x;
    float y;
    DWORD unused2;
    ITEM_DATA data;
} COMMAND_BB_DROP_ITEM; // 5F 

typedef struct {
    COMMAND_HEADER_BB header;
    BYTE subcommand;
    BYTE subsize;
    WORD unused;
    BYTE shopType;
    BYTE numItems;
    WORD unused2;
    ITEM_DATA items[0];
} COMMAND_BB_SHOP; // B6 

typedef struct {
    COMMAND_HEADER_BB header;
    BYTE subcommand;
    BYTE unused[3];
    DWORD size; // same as size in header 
    DWORD checksum; // can be random; client won't notice 
    DWORD numItems;
    DWORD meseta;
    PLAYER_BANK_ITEM items[0];
} COMMAND_BB_BANK; // BC 

typedef struct {
    COMMAND_HEADER_BB header;
    BYTE subcommand;
    BYTE subsize;
    WORD clientID;
    ITEM_DATA item;
    DWORD unused;
} COMMAND_BB_CREATE_ITEM; // BE 

////////////////////////////////////////////////////////////////////////////////
// ep3 card update B8 

typedef struct {
    COMMAND_HEADER_DCGC header;
    DWORD size;
    BYTE data[0];
} COMMAND_GC_CARDUPDATE; // B8 

////////////////////////////////////////////////////////////////////////////////
// ep3 rank update B7 

typedef struct {
    COMMAND_HEADER_DCGC header;
    DWORD rank;
    char rankText[0x0C];
    DWORD meseta;
    DWORD maxMeseta;
    DWORD jukeboxSongsUnlocked;
} COMMAND_GC_RANKUPDATE; // B7 

////////////////////////////////////////////////////////////////////////////////
// quest select A2 A4 00A2 

typedef struct {
    COMMAND_HEADER_PC header;
    struct {
        DWORD menuID;
        DWORD questID;
        wchar_t name[0x20];
        wchar_t shortDesc[0x70];
    } entry[0];
} COMMAND_PC_QUESTSELECT; // A2 A4 

typedef struct {
    COMMAND_HEADER_DCGC header;
    struct {
        DWORD menuID;
        DWORD questID;
        char name[0x20];
        char shortDesc[0x70];
    } entry[0];
} COMMAND_GC_QUESTSELECT; // A2 A4 

typedef struct {
    COMMAND_HEADER_BB header;
    struct {
        DWORD menuID;
        DWORD questID;
        wchar_t name[0x20];
        wchar_t shortDesc[0x7A];
    } entry[0];
} COMMAND_BB_QUESTSELECT; // 00A2 00A4 

////////////////////////////////////////////////////////////////////////////////
// open quest file 44 A6 0044 00A6 

typedef struct {
    union {
        COMMAND_HEADER_PC headerpc;
        COMMAND_HEADER_DCGC headergc;
    };
    char name[0x20];
    WORD unused;
    WORD flags;
    char filename[0x10];
    DWORD filesize;
} COMMAND_PCGC_OPENQUESTFILE; // 44 A6 

typedef struct {
    COMMAND_HEADER_BB header;
    BYTE unused[0x22];
    WORD flags; // 3 for Ep3 download quest, 2 otherwise 
    char filename[0x10];
    DWORD filesize;
    char name[0x18];
} COMMAND_BB_OPENQUESTFILE; // 0044 00A6 

////////////////////////////////////////////////////////////////////////////////
// send quest file chunk 13 A7 0013 00A7 

typedef struct {
    union {
        COMMAND_HEADER_PC headerpc;
        COMMAND_HEADER_DCGC headerdcgc;
    };
    char filename[0x10];
    BYTE data[0x400];
    DWORD datasize;
} COMMAND_DCPCGC_QUESTFILECHUNK; // 13 A7 

typedef struct {
    COMMAND_HEADER_BB header;
    char filename[0x10];
    BYTE data[0x400];
    DWORD datasize;
} COMMAND_BB_QUESTFILECHUNK; // 0013 00A7 

////////////////////////////////////////////////////////////////////////////////
// time B1 00B1 

typedef struct {
    union {
        COMMAND_HEADER_PC headerpc;
        COMMAND_HEADER_DCGC headerdcgc;
    };
    char time[0]; // GetLocalTime(&time); sprintf(time,"%d:%d:%d: %d:%d:%d.000",time.wYear,time.wMonth,time.wDay,time.wHour,time.wMinute,time.wSecond); 
} COMMAND_DCPCGC_TIME; // B1 

typedef struct {
    COMMAND_HEADER_BB header;
    char time[0]; // GetLocalTime(&time); sprintf(time,"%d:%d:%d: %d:%d:%d.000",time.wYear,time.wMonth,time.wDay,time.wHour,time.wMinute,time.wSecond); 
} COMMAND_BB_TIME; // 00B1 

