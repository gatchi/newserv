// These flags determine who is allowed to use a chat command, and when.
#define COMMAND_REQUIRE_NOT_GAMECUBE        0x00000001 // command can't be used in PSOGC
#define COMMAND_REQUIRE_NOT_BLUEBURST       0x00000002 // command can't be used in PSOBB
#define COMMAND_REQUIRE_NOT_PC              0x00000004 // command can't be used in PSOPC
#define COMMAND_REQUIRE_NOT_DC              0x00000008 // command can't be used in PSODC
#define COMMAND_REQUIRE_LOBBY               0x00000010 // player must be in the lobby
#define COMMAND_REQUIRE_GAME                0x00000020 // player must be in a game
#define COMMAND_REQUIRE_LEADER              0x00000040 // player must be the leader of the game
#define COMMAND_REQUIRE_LEADER_GAME         0x00000080 // player must be the leader in a game or private lobby, otherwise use privilege mask 
#define COMMAND_REQUIRE_PRIVILEGE_MASK      0x00000100 // use privilege mask (see privilege.h)
#define COMMAND_REQUIRE_CHEAT_MODE          0x00000200 // cheat mode must be enabled in the game
#define COMMAND_REQUIRE_DISALLOW            0x80000000 // unused, apparently

#define COMMAND_REQUIRE_VERSION_GAMECUBE        0x0000000E // require PSOGC
#define COMMAND_REQUIRE_VERSION_BLUEBURST       0x0000000D // require PSOBB
#define COMMAND_REQUIRE_VERSION_PC              0x0000000B // require PSOPC
#define COMMAND_REQUIRE_VERSION_DC              0x00000007 // require PSODC

#define CHAT_COMMAND_BAD_ARGUMENTS          7 // just an error code

typedef struct {
    wchar_t* command;
    int (*function)(SERVER*,LOBBY*,CLIENT*,int,wchar_t**,wchar_t*);
    long requirementFlags;
    long requiredPrivilege;
    wchar_t* helpMessage;
} ChatCommandEntry;

int ProcessChatCommand(SERVER* s,CLIENT* c,wchar_t* text);
