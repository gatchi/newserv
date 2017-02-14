// quest categories (used to filter quests at the quest selection menu)
#define QUEST_CATEGORY_RETRIEVAL      0x0001
#define QUEST_CATEGORY_EXTERMINATION  0x0002
#define QUEST_CATEGORY_EVENT          0x0004
#define QUEST_CATEGORY_SHOP           0x0008
#define QUEST_CATEGORY_VR             0x0010
#define QUEST_CATEGORY_TOWER          0x0020
#define QUEST_CATEGORY_EP1            0x0040 // for govt quests 
#define QUEST_CATEGORY_EP2            0x0080 // for govt quests 
#define QUEST_CATEGORY_EP4            0x0100 // for govt quests 
#define QUEST_CATEGORY_DOWNLOAD       0x0200
#define QUEST_CATEGORY_JOINABLE       0x8000 // actually a flag, but whatever: it means the quest is joinable while in progress
#define QUEST_CATEGORY_ANY            0xFFFF // use this when we don't care about category

// modes in which the quest can be played
#define QUEST_MODE_NORMAL      0x01
#define QUEST_MODE_BATTLE      0x02
#define QUEST_MODE_CHALLENGE   0x04
#define QUEST_MODE_SOLO        0x08
#define QUEST_MODE_GOVT        0x10
#define QUEST_MODE_DOWNLOAD    0x20
#define QUEST_MODE_ANY         0xFF

// a single quest entry:
typedef struct {
    BYTE mode; // online, battle, challenge, solo, govt, or download 
    BYTE episode; // 01 = ep1, 02 = ep2, 03 = ep4, FF = ep3, 00 = don't filter by episode (for FilterQuestList) 
    WORD category; // retrieval, extermination, etc. 
    BYTE versionFlags;
    char filebase[7]; // base name for quest files (we append v<versionnumber>.<bin/dat>)
    DWORD questID;
    wchar_t name[0x1C];
    wchar_t shortdesc[0x60];
    wchar_t description[0x100];
} QUEST;

// a list of quests:
typedef struct {
    DWORD flags;
    DWORD numQuests;
    QUEST quests[0];
} QUESTLIST;

void QuestFileName(char* target,char* dirprefix,char* base,int version,bool bin);

int CreateQuestList(char* filename,char* dirname,QUESTLIST** ql);
void DestroyQuestList(QUESTLIST* ql);
QUESTLIST* FilterQuestList(QUESTLIST* ql,BYTE version,BYTE mode,BYTE episode,WORD category);
QUEST* FindQuest(QUESTLIST* ql,DWORD questID);
QUEST* FindQuestPName(QUESTLIST* ql,wchar_t* name);
bool ConvertToDownloadQuest(QUEST* src,QUEST* dst,BYTE version);

