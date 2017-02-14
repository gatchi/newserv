// privilege bits. A privilege mask is a DWORD, each bit corresponding to one of these privileges:
#define PRIVILEGE_KICK_USER             0x00000001 // user can kick other users.
#define PRIVILEGE_BAN_USER              0x00000002 // user can set bans on other users.
#define PRIVILEGE_STFU_USER             0x00000004 // user can prevent other users from chatting.
#define PRIVILEGE_CHANGE_LOBBY_INFO     0x00000010 // user can change lobby info (type, name, etc.)
#define PRIVILEGE_CHANGE_EVENT          0x00000020 // user can change lobby events
#define PRIVILEGE_ANNOUNCE              0x00000040 // user can announce messages to the entire server
#define PRIVILEGE_FREE_JOIN_GAMES       0x00000100 // user can join games regardless of passwords or level restrictions
#define PRIVILEGE_UNLOCK_GAMES          0x00000200 // user can unlock password-protected games from the lobby
#define PRIVILEGE_HIGHER_LEVEL          0x01000000 // user can use these privileges on people who also have these privileges (i.e. can ban users who can also ban users)
#define PRIVILEGE_IMMUNITY              0x02000000 // unused, as far as I can tell

#define PRIVILEGE_MODERATE              0x00000007 // recommended privilege level for moderators
#define PRIVILEGE_ADMINISTRATE          0x00000377 // recommended privilege level for administrators
#define PRIVILEGE_ROOT                  0x01000377 // recommended privilege level for root admin
