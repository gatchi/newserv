#include <windows.h>
#include <stdio.h>

#include "netconfig.h"

#include "updates.h"

// Checksums an amount of data.
long CalculateUpdateChecksum(void* data,unsigned long size)
{
    unsigned long offset,y,cs = 0xFFFFFFFF;
    for (offset = 0; offset < size; offset++)
    {
        cs ^= *(unsigned char*)((long)data + offset);
        for (y = 0; y < 8; y++)
        {
            if (!(cs & 1)) cs = (cs >> 1) & 0x7FFFFFFF;
            else cs = ((cs >> 1) & 0x7FFFFFFF) ^ 0xAFBE89A4;
        }
    }
    return (cs ^ 0xFFFFFFFF);
}

// Creates an index of files to update from the given configuration file.
bool LoadUpdateList_EnumRoutine(CFGFile* cf,const char* name,const char* valueA,const wchar_t* valueW,UPDATE_LIST** list)
{
    DWORD num;
    if (*list) num = (*list)->numFiles;
    else num = 0;
    *list = (UPDATE_LIST*)realloc(*list,sizeof(UPDATE_LIST) + ((num + 1) * sizeof(UPDATE_FILE)));
    if (!(*list)) return false;
    if (strlen(name) > 0x3F) return false;
    strcpy((*list)->files[num].filename,name);
    (*list)->files[num].flags = 0;
    sscanf(valueA,"%lX",&((*list)->files[num].flags));

    HANDLE file = CreateFile((*list)->files[num].filename,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    if (file == INVALID_HANDLE_VALUE) return false;

    DWORD bytesread;
    (*list)->files[num].size = GetFileSize(file,NULL);
    void* data = malloc((*list)->files[num].size);
    if (!data)
    {
        CloseHandle(file);
        return false;
    }
    ReadFile(file,data,(*list)->files[num].size,&bytesread,NULL);
    (*list)->files[num].checksum = CalculateUpdateChecksum(data,(*list)->files[num].size);
    free(data);

    (*list)->numFiles = num + 1;

    return true;
}

// Loads the update list from the given file.
UPDATE_LIST* LoadUpdateList(char* filename)
{
    UPDATE_LIST* list = NULL;

    CFGFile* cfg;
    cfg = CFGLoadFile(filename);
    if (!cfg) return NULL;
    bool success = CFGEnumValues(cfg,(CFGEnumRoutine)LoadUpdateList_EnumRoutine,(long)(&list));
    CFGCloseFile(cfg);

    if (!success)
    {
        if (list) free(list);
        list = NULL;
    }

    return list;
}

// Frees an allocated update list.
void DestroyUpdateList(UPDATE_LIST* list) { free(list); }

