// information on a single level for a single class
typedef struct {
    BYTE atp; // atp to add on level up
    BYTE mst; // mst to add on level up
    BYTE evp; // evp to add on level up
    BYTE hp; // hp to add on level up
    BYTE dfp; // dfp to add on level up
    BYTE ata; // ata to add on level up
    BYTE unknown[2];
    DWORD exp; // EXP value of this level
} LEVEL_INFO; // 0x0C in size 

// level table format (PlyLevelTbl.prs, I believe -- the server calls it levels.pbb)
typedef struct {
    PLAYER_STATS baseStats[12];
    DWORD unknown[12];
    LEVEL_INFO levels[12][200];
} LEVEL_TABLE;

LEVEL_TABLE* LoadLevelTable(char* filename,bool compressed);
void FreeLevelTable(LEVEL_TABLE* table);

PLAYER_STATS* GetBaseStats(LEVEL_TABLE* table,int charClass);
LEVEL_INFO* GetLevelInfo(LEVEL_TABLE* table,int charClass,int level);

void ApplyLevelInfoToPlayerStats(LEVEL_INFO* level,PLAYER_STATS* stats);
