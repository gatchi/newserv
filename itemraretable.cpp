#include <windows.h>
#include <stdio.h>

#include "itemraretable.h"

ITEM_RARE_SET* LoadItemRareSet(char* filename,int episode,int difficulty,int secid)
{
    DWORD bytesread;
    HANDLE file = CreateFile(filename,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    if (file == INVALID_HANDLE_VALUE) return NULL;
    ITEM_RARE_SET* set = (ITEM_RARE_SET*)malloc(sizeof(ITEM_RARE_SET));
    if (!set)
    {
        CloseHandle(file);
        return NULL;
    }
    int offset = (episode * 0x6400) + (difficulty * 0x1900) + (secid * 0x0280);
    SetFilePointer(file,offset,NULL,FILE_BEGIN);
    ReadFile(file,set,sizeof(ITEM_RARE_SET),&bytesread,NULL);
    CloseHandle(file);

    return set;
}

void FreeItemRareSet(ITEM_RARE_SET* set) { free(set); }

bool DecideIfItemIsRare(BYTE pc)
{
    unsigned int shift = ((pc >> 3) & 0x1F) - 4;
    if (shift < 0) shift = 0;
    unsigned long rate = ((2 << shift) * ((pc & 7) + 7));

    unsigned long x = (((long)rand() << 30) ^ ((long)rand() << 15) ^ (long)rand());
    if (x < rate) return true;
    return false;
}

