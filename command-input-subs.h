typedef struct {
    BYTE command;
    BYTE size;
    WORD enemyID2;
    WORD enemyID;
    WORD damage;
    DWORD flags;
} COMMAND_BB_SUBCOMMAND_0A;

typedef struct {
    BYTE command;
    BYTE size;
    BYTE clientID;
    BYTE unused;
    DWORD itemID;
    DWORD amount; // unused except by 63 
} COMMAND_BB_SUBCOMMAND_25_26_27_63;

typedef struct {
    BYTE command;
    BYTE size;
    BYTE clientID;
    BYTE unused;
    WORD unused2; // should be 1 
    WORD area;
    DWORD itemID;
    float x;
    float y;
    float z;
} COMMAND_BB_SUBCOMMAND_2A;

typedef struct {
    BYTE command;
    BYTE size;
    BYTE clientID;
    BYTE unused;
    DWORD itemID;
    BYTE area;
    BYTE unused2[3];
} COMMAND_BB_SUBCOMMAND_5A;

typedef struct {
    BYTE command;
    BYTE size;
    WORD unused;
    BYTE area;
    BYTE monsterID;
    WORD requestID;
    float x;
    float y;
    DWORD unknown[2];
} COMMAND_BB_SUBCOMMAND_60;

typedef struct {
    BYTE command;
    BYTE size;
    WORD unused;
    BYTE area;
    BYTE unused2;
    WORD requestID;
    float x;
    float y;
    DWORD unknown[6];
} COMMAND_BB_SUBCOMMAND_A2;

typedef struct {
    BYTE command;
    BYTE unused[3];
    DWORD shopType;
} COMMAND_BB_SUBCOMMAND_B5;

typedef struct {
    BYTE command;
    BYTE size;
    BYTE unused[2];
    DWORD itemID;
    BYTE unused2;
    BYTE shopItemIndex;
    BYTE amount;
    BYTE unused3;
} COMMAND_BB_SUBCOMMAND_B7;

typedef struct {
    BYTE command;
    BYTE unused[3];
    DWORD shopType;
} COMMAND_BB_SUBCOMMAND_BB_C4;

typedef struct {
    BYTE subcommand;
    BYTE subsize;
    WORD unused;
    DWORD itemID;
    DWORD amount;
    BYTE action;
    BYTE itemAmount;
    WORD unused2;
} COMMAND_BB_SUBCOMMAND_BD;

typedef struct {
    BYTE command;
    BYTE size;
    BYTE clientID;
    BYTE unused;
    DWORD itemID;
    DWORD amount;
} COMMAND_BB_SUBCOMMAND_C0;

typedef struct {
    BYTE command;
    BYTE size;
    BYTE clientID;
    BYTE unused;
    WORD area;
    WORD unused2;
    float x;
    float y;
    DWORD itemID;
    DWORD amount;
} COMMAND_BB_SUBCOMMAND_C3;

typedef struct {
    BYTE command;
    BYTE size;
    WORD unused;
    DWORD itemIDs[30];
} COMMAND_BB_SUBCOMMAND_C4;

typedef struct {
    BYTE command;
    BYTE size;
    WORD enemyID2;
    WORD enemyID;
    WORD killer;
    DWORD unused;
} COMMAND_BB_SUBCOMMAND_C8;

// these flags tell when a subcommand should be blocked
#define SUBCOMMAND_ALLOW_ALL            0x00000000 // always allow this subcommand
#define SUBCOMMAND_CHECK_SIZE           0x00000001 // check the sub-size of this subcommand
#define SUBCOMMAND_CHECK_CLIENT         0x00000002 // check the originating client ID (to make sure this is not impersonating another user)
#define SUBCOMMAND_CHECK_SIZE_CLIENT    0x00000003 // check the size and client ID
#define SUBCOMMAND_GAME_LOADING         0x00000004 // only allow this command when a game is loading
#define SUBCOMMAND_GAME_ONLY            0x00000100 // only allow this command in a game
#define SUBCOMMAND_CHECK_SIZE_GAME      0x00000101 // check size, and only allow this command in a game
#define SUBCOMMAND_GAME_LOADING_ONLY    0x00000104 // only allow this command in a game while another player is loading
#define SUBCOMMAND_LOBBY_ONLY           0x00000200 // only allow this command in a lobby

typedef struct {
    DWORD minSize;
    DWORD maxSize;
    void* function;
    DWORD flags;
} SUBCOMMAND_DESCRIPTOR;
