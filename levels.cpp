#include <windows.h>
#include <stdio.h>

#include "prs.h"

#include "player.h"
#include "levels.h"

// Loads the level-up data from a file (usually levels.pbb). This data tells how
// much each stat should increase when a player levels up, and how much EXP is
// required for each level.
LEVEL_TABLE* LoadLevelTable(char* filename,bool compressed)
{
    DWORD bytesread;
    HANDLE file = CreateFile(filename,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    if (!file) return NULL;
    LEVEL_TABLE* table = (LEVEL_TABLE*)malloc(sizeof(LEVEL_TABLE));
    if (!table)
    {
        CloseHandle(file);
        return NULL;
    }
    if (compressed)
    {
        DWORD encsize,decsize;
        void *compressed_data,*decompressed_data;
        encsize = GetFileSize(file,NULL);
        compressed_data = malloc(encsize);
        if (!compressed_data)
        {
            free(table);
            CloseHandle(file);
            return NULL;
        }
        ReadFile(file,compressed_data,encsize,&bytesread,NULL);
        CloseHandle(file);
        decsize = prs_decompress_size(compressed_data);
        decompressed_data = malloc(decsize);
        if (!decompressed_data)
        {
            free(table);
            free(compressed_data);
            return NULL;
        }
        prs_decompress(compressed_data,decompressed_data);
        free(compressed_data);
        memcpy(table,decompressed_data,sizeof(LEVEL_TABLE));
        free(decompressed_data);
    } else {
        ReadFile(file,table,sizeof(LEVEL_TABLE),&bytesread,NULL);
        CloseHandle(file);
    }
    return table;
}

// Frees a loaded level table.
void FreeLevelTable(LEVEL_TABLE* table) { free(table); }

// Finds a class' base statistics (i.e. level 1 stats)
PLAYER_STATS* GetBaseStats(LEVEL_TABLE* table,int charClass)
{
    if (charClass >= 12) return NULL;
    return &table->baseStats[charClass];
}

// Gets the level-up data for a certain class and level.
LEVEL_INFO* GetLevelInfo(LEVEL_TABLE* table,int charClass,int level)
{
    if ((charClass >= 12) || (level >= 200)) return NULL;
    return &table->levels[charClass][level];
}

// Levels up a character by adding the level-up bonuses to the player's stats.
void ApplyLevelInfoToPlayerStats(LEVEL_INFO* level,PLAYER_STATS* stats)
{
    stats->ata += level->ata;
    stats->atp += level->atp;
    stats->dfp += level->dfp;
    stats->evp += level->evp;
    stats->hp += level->hp;
    stats->mst += level->mst;
}

