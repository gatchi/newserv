#include <windows.h>
#include <stdio.h>

#include "text.h"

#include "netconfig.h"

#include "quest.h"

// generates a quest file name. quite simple.
void QuestFileName(char* target,char* dirprefix,char* base,int version,bool bin) { sprintf(target,"%s%sv%d.%s",dirprefix,base,version,(bin ? "bin" : "dat")); }

// parses quest entries in a quest list config file.
bool CreateQuestList_EnumRoutine(CFGFile* cf,const char* name,const char* valueA,const wchar_t* valueW,QUESTLIST** ql)
{
    if (!strcmp(name,"QuestFilePrefix"))
    {
        if (!(*ql))
        {
            *ql = (QUESTLIST*)malloc(sizeof(QUESTLIST) + (sizeof(QUEST)));
            (*ql)->numQuests = 1;
        } else {
            (*ql)->numQuests++;
            *ql = (QUESTLIST*)realloc(*ql,sizeof(QUESTLIST) + ((*ql)->numQuests * sizeof(QUEST)));
        }
        if (!(*ql)) return false;
        strcpy((*ql)->quests[(*ql)->numQuests - 1].filebase,valueA);
    }
    if (!strcmp(name,"QuestMode")) (*ql)->quests[(*ql)->numQuests - 1].mode = CFGParseNumber(valueA);
    if (!strcmp(name,"QuestEpisode")) (*ql)->quests[(*ql)->numQuests - 1].episode = CFGParseNumber(valueA);
    if (!strcmp(name,"QuestCategory")) (*ql)->quests[(*ql)->numQuests - 1].category = CFGParseNumber(valueA);
    if (!strcmp(name,"QuestName")) wcscpy((*ql)->quests[(*ql)->numQuests - 1].name,valueW);
    if (!strcmp(name,"QuestShortDescription"))
    {
        wcscpy((*ql)->quests[(*ql)->numQuests - 1].shortdesc,valueW);
        tx_replace_char((*ql)->quests[(*ql)->numQuests - 1].shortdesc,'@','#');
    }
    if (!strcmp(name,"QuestLongDescription"))
    {
        wcscpy((*ql)->quests[(*ql)->numQuests - 1].description,valueW);
        tx_replace_char((*ql)->quests[(*ql)->numQuests - 1].description,'@','#');
    }
    return true;
}

// builds a quest list from a quest list config file.
int CreateQuestList(char* filename,char* dirname,QUESTLIST** ql)
{
    *ql = NULL;

    CFGFile* cfg;
    cfg = CFGLoadFile(filename);
    if (!cfg) return (-1);
    CFGEnumValues(cfg,(CFGEnumRoutine)CreateQuestList_EnumRoutine,(long)ql);
    CFGCloseFile(cfg);

    unsigned int x,y;
    HANDLE file;
    char filename2[MAX_PATH];
    for (x = 0; x < (*ql)->numQuests; x++)
    {
        (*ql)->quests[x].versionFlags = 0x00;
        for (y = 0; y < 8; y++)
        {
            QuestFileName(filename2,dirname,(*ql)->quests[x].filebase,y,true);
            file = CreateFile(filename2,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
            if (file == INVALID_HANDLE_VALUE) continue;
            (*ql)->quests[x].versionFlags |= (1 << y);
            CloseHandle(file);
        }
        do {
            (*ql)->quests[x].questID = (rand() << 16) | rand();
            for (y = 0; y < x; y++) if ((*ql)->quests[x].questID == (*ql)->quests[y].questID) break;
        } while (y < x);
    }

    return 0;
}

// frees a quest list.
void DestroyQuestList(QUESTLIST* ql) { free(ql); }

// generates a new quest list which contains only quests qith the selected version, mode, episode, and category flags.
// to skip a criterion:
//   to allow all modes: set mode to 0xFF
//   to allow all episodes: set episode to 0
//   to allow all categories: set category to 0xFFFF
QUESTLIST* FilterQuestList(QUESTLIST* ql,BYTE version,BYTE mode,BYTE episode,WORD category)
{
    if (!ql) return NULL;

    unsigned int x,numresults = 0;
    QUESTLIST* result = NULL;
    for (x = 0; x < ql->numQuests; x++)
    {
        if (!(ql->quests[x].versionFlags & (1 << version))) continue; // excluded by version 
        if ((mode != 0xFF) && (ql->quests[x].mode != mode)) continue; // excluded by mode 
        if (!(ql->quests[x].category & category)) continue; // excluded by category 
        if (episode && (ql->quests[x].episode != episode)) continue; // excluded by episode (if present) 
        result = (QUESTLIST*)realloc(result,sizeof(QUESTLIST) + ((numresults + 1) * sizeof(QUEST)));
        if (!result) return NULL;
        memcpy(&result->quests[numresults],&ql->quests[x],sizeof(QUEST));
        numresults++;
    }
    if (result) result->numQuests = numresults;
    return result;
}

// finds a quest in a quest list by ID
QUEST* FindQuest(QUESTLIST* ql,DWORD questID)
{
    unsigned int x;
    QUEST* ret = NULL;
    for (x = 0; x < ql->numQuests; x++) if (ql->quests[x].questID == questID) break;
    if (x < ql->numQuests) ret = &ql->quests[x];
    return ret;
}

// finds a quest in a quest list by partial name or acronym. for example,
// searching for "ttf" would find "Towards the Future", as would searching for
// "Towards" or even "tow"
QUEST* FindQuestPName(QUESTLIST* ql,wchar_t* name)
{
    DWORD x,y,z;

    wchar_t ab[0x14];
    wchar_t temp[0x20];
    if (wcslen(name) > 0x1B) return NULL;
    for (x = 0; x < ql->numQuests; x++)
    {
        if (!wcsicmp(ql->quests[x].name,name)) return &ql->quests[x];

        ZeroMemory(ab,0x20);

        for (y = 0; y < wcslen(ql->quests[x].name); y++) if (ql->quests[x].name[x] == L':') break;
        if (y < wcslen(ql->quests[x].name))
        {
            wcscpy(temp,ql->quests[x].name);
            temp[y] = 0;
            if (!wcsicmp(temp,name)) return &ql->quests[x];
            wcscpy(temp,&temp[y + 1]);
        } else wcscpy(temp,ql->quests[x].name);

        ab[0] = temp[0];
        for (y = 0, z = 1; y < wcslen(temp); y++)
        {
            if (temp[y] == ' ')
            {
                ab[z] = temp[y + 1];
                z++;
            }
        }

        if (!wcsicmp(ab,name)) return &ql->quests[x];
    }
    return NULL;
}
