// an entry in a battleparamentry file
typedef struct {
    WORD atp; // attack power
    WORD psv; // perseverance (intelligence?)
    WORD evp; // evasion
    WORD hp; // hit points
    WORD dfp; // defense
    WORD ata; // accuracy
    WORD lck; // luck
    BYTE unknown[14];
    DWORD exp; // exp value
    DWORD difficulty;
} BATTLE_PARAM;

bool LoadBattleParamEntriesEpisode(BATTLE_PARAM* params,char* filename);
