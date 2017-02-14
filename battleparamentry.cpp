#include <windows.h>

#include "battleparamentry.h"

// simple: loads a set of battle parameters into the specified structure.
// (really, it just reads a block of data from a file.)
bool LoadBattleParamEntriesEpisode(BATTLE_PARAM* params,char* filename)
{
    DWORD bytesread;
    HANDLE file = CreateFile(filename,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    if (!file) return false;
    ReadFile(file,params,0x180 * sizeof(BATTLE_PARAM),&bytesread,NULL);
    CloseHandle(file);
    if (bytesread < (0x180 * sizeof(BATTLE_PARAM))) return false;
    return true;
}
