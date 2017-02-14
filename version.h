// version IDs
#define VERSION_GAMECUBE       0 // PSOGC Ep1&2, Plus, Ep3 (all regions)
#define VERSION_BLUEBURST      1 // PSOBB (all regions)
#define VERSION_PC             2 // PSOPC
#define VERSION_DC             3 // PSODC (v2)
#define VERSION_PATCH          4 // PSOPC/PSOBB patch client
#define VERSION_FUZZIQER       5 // Fuzziqer Software server (client on shipgate)
#define VERSION_DCV1_QUEST     6
#define VERSION_UNKNOWN        6
#define VERSION_MAX            6

// client flags
#define FLAG_PICKY_BOXES        0x0004 // after joining a lobby, client will no longer send D6 commands when they close message boxes
#define FLAG_BOXES_DISABLED     0x0008 // client with FLAG_PICKY_BOXES has already joined a lobby
#define FLAG_EXTRA_LOBBIES      0x0010 // client can see Ep3 lobbies (this will screw up non-Ep3 clients)
#define FLAG_V4_GAMES           0x0020 // client is Ep3
#define FLAG_DC_VERSION1        0x0040 // client is DCv1
#define FLAG_LOADING            0x0080 // client is loading into a game

#define FLAG_PSOV1        (FLAG_DC_VERSION1) // PSODCv1: hidered capabilities (is v1)
#define FLAG_PSOV2_DC     0x0000 // PSODCv2: nothing special
#define FLAG_PSOV2_PC     0X0000 // PSOPC: nothing special
#define FLAG_PSOV3_10     0x0000 // PSOGC: nothing special
#define FLAG_PSOV3_12     (FLAG_PICKY_BOXES) // PSO Plus: picky message boxes
#define FLAG_PSOV3_BB     (FLAG_PICKY_BOXES | FLAG_BOXES_DISABLED) // PSOBB: picky message boxes, boxes disabled
#define FLAG_PSOV4        (FLAG_PICKY_BOXES | FLAG_EXTRA_LOBBIES | FLAG_V4_GAMES) // Ep3: picky boxes, extra lobbies, and is Ep3

WORD GetClientSubversionFlags(BYTE version,BYTE psoVersionID);
