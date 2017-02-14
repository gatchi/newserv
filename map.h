// an enemy entry in a map file
typedef struct {
    DWORD base;
    WORD reserved0;
    WORD numClones;
    DWORD reserved[11];
    float reserved12;
    DWORD reserved13;
    DWORD reserved14;
    DWORD skin;
    DWORD reserved15;
} ENEMY_ENTRY;

// an enemy entry as loaded by the game
typedef struct {
    WORD unused;
    BYTE hitFlags;
    BYTE lastHit;
    DWORD exp;
    DWORD rt_index;
} GAME_ENEMY_ENTRY;

int LoadMapData(GAME_ENEMY_ENTRY* enemies,DWORD* numEnemies,char* filename,DWORD episode,DWORD diff,BATTLE_PARAM* battleParamTable,bool alt_enemies);
int ParseMapData(GAME_ENEMY_ENTRY* enemies,DWORD* numEnemies,DWORD episode,DWORD diff,BATTLE_PARAM* battleParamTable,ENEMY_ENTRY* map,DWORD numEntries,bool alt_enemies);

