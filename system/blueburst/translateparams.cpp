#include <windows.h>
#include <stdio.h>

#include "../../netconfig.h"

////////////////////////////////////////////////////////////////////////////////

// the format for entries in BattleParamEntry files
typedef struct {
    WORD atp;
    WORD psv; // perseverance (intelligence?)
    WORD evp;
    WORD hp;
    WORD dfp;
    WORD ata;
    WORD lck;
    BYTE unknown[14];
    DWORD exp;
    DWORD difficulty; } DUDEINFO;

// online/offline enemy info, by episode and difficulty
DUDEINFO dudeinfoon[3][4][0x60];
DUDEINFO dudeinfooff[3][4][0x60];

// loads an episode's worth of enemy data
bool LoadMonsterEXPsEpisode(bool on,BYTE episode,wchar_t* filename)
{
    HANDLE file;
    DWORD bytesread;
    file = CreateFileW(filename,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    if (!file) return false;
    if (on) ReadFile(file,dudeinfoon[episode - 1],0x180 * sizeof(DUDEINFO),&bytesread,NULL);
    else ReadFile(file,dudeinfooff[episode - 1],0x180 * sizeof(DUDEINFO),&bytesread,NULL);
    CloseHandle(file);
    if (bytesread < (0x180 * sizeof(DUDEINFO))) return false;
    return true;
}

// loads all enemy data
bool LoadMonsterEXPs()
{
    if (!LoadMonsterEXPsEpisode(true,1,L"BattleParamEntry_on.dat")) return false;
    if (!LoadMonsterEXPsEpisode(true,2,L"BattleParamEntry_lab_on.dat")) return false;
    if (!LoadMonsterEXPsEpisode(true,3,L"BattleParamEntry_ep4_on.dat")) return false;
    if (!LoadMonsterEXPsEpisode(false,1,L"BattleParamEntry.dat")) return false;
    if (!LoadMonsterEXPsEpisode(false,2,L"BattleParamEntry_lab.dat")) return false;
    if (!LoadMonsterEXPsEpisode(false,3,L"BattleParamEntry_ep4.dat")) return false;
    return true;
}

// retrieves a monster's EXP or HP
DWORD MonsterEXP(BYTE episode,BYTE difficulty,BYTE type) { return dudeinfoon[episode - 1][difficulty][type].exp; }
DWORD MonsterHP(BYTE episode,BYTE difficulty,BYTE type) { return dudeinfoon[episode - 1][difficulty][type].hp; }

////////////////////////////////////////////////////////////////////////////////

// the format of a file entry in a GSL archive
typedef struct {
    char name[0x20];
    DWORD offset;
    DWORD size;
    DWORD unused[2]; } GSL_ENTRY;

// GSLs consist of 256 (0x100) file entries, then the file data. that's it. the HANDLE is not part of the actual archive header.
typedef struct {
    GSL_ENTRY e[0x100];
    HANDLE file; } GSL_HEADER;

// byteswaps a DWORD the long way
DWORD byteswap(DWORD e) { return (((e >> 24) & 0xFF) | (((e >> 16) & 0xFF) << 8) | (((e >> 8) & 0xFF) << 16) | ((e & 0xFF) << 24)); }

// finds a file entry in a loaded GSL archive
GSL_ENTRY* GSL_FindEntry(GSL_HEADER* gsl,char* filename)
{
    int x;
    if (!gsl) return NULL;
    for (x = 0; x < 0x100; x++) if (!strcmp(gsl->e[x].name,filename)) break;
    if (x >= 0x100) return NULL;
    return &gsl->e[x];
}

// opens (loads) a GSL archive
GSL_HEADER* GSL_OpenArchive(wchar_t* filename)
{
    HANDLE file;
    DWORD x;
    file = CreateFileW(filename,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    if (file == INVALID_HANDLE_VALUE) return NULL;
    GSL_HEADER* gsl = (GSL_HEADER*)malloc(sizeof(GSL_HEADER));
    if (!gsl)
    {
        free(gsl);
        return NULL;
    }
    ReadFile(file,gsl,0x3000,&x,NULL);
    gsl->file = file;
    for (x = 0; x < 0x100; x++)
    {
        gsl->e[x].offset = byteswap(gsl->e[x].offset);
        gsl->e[x].size = byteswap(gsl->e[x].size);
    }
    return gsl;
}

// reads an entire file from a GSL archive
bool GSL_ReadFile(GSL_HEADER* gsl,char* filename,void* buffer)
{
    DWORD bytesread;
    if (!gsl) return false;
    GSL_ENTRY* e = GSL_FindEntry(gsl,filename);
    if (!e) return false;
    SetFilePointer(gsl->file,e->offset * 0x800,NULL,FILE_BEGIN);
    ReadFile(gsl->file,buffer,e->size,&bytesread,NULL);
    return true;
}

// gets a file's size inside a GSL archive
DWORD GSL_GetFileSize(GSL_HEADER* gsl,char* filename)
{
    GSL_ENTRY* ge = GSL_FindEntry(gsl,filename);
    if (!ge) return 0;
    return ge->size;
}

// closes (unloads) a GSL archive
void GSL_CloseArchive(GSL_HEADER* gsl)
{
    if (gsl)
    {
        CloseHandle(gsl->file);
        free(gsl);
    }
}

////////////////////////////////////////////////////////////////////////////////

// an entry in PSO's ItemRT*.rel files
typedef struct {
    BYTE prob;
    BYTE item[3]; } DROPITEM;

// the format of the ItemRT*.rel files
typedef struct {
    DROPITEM dude[3][4][10][50];
    DROPITEM box[3][4][10][30];
    BYTE area[3][4][10][30]; } DROPDATA;

DROPDATA dropdata;

// loads all ItemRT*.rel files for episodes 1 and 2 from itemraretable.pbb (also known as ItemRT.gsl, from PSOGC)
bool LoadDropInfoFile()
{
    HANDLE file;
    DWORD size,bytesread;
    int w,x,y,z,a;
    BYTE data[0x800];
    char filename[0x20];
    wchar_t buffer[0x20];

    GSL_HEADER* gsl = GSL_OpenArchive(L"itemraretable.pbb");
    if (!gsl) return false;

    char* prefixes[] = {"ItemRT","ItemRTl"};
    char diffs[] = "nhvu";
    for (w = 0; w < 2; w++) // episode 
    {
        for (x = 0; x < 4; x++) // difficulty 
        {
            for (y = 0; y < 10; y++) // sec id 
            {
                sprintf(filename,"%s%c%d.rel",prefixes[w],diffs[x],y);
                if (GSL_GetFileSize(gsl,filename) > 0x800) return false;
                if (!GSL_ReadFile(gsl,filename,data)) return false;

                for (z = 0; z < 50; z++) // monster id 
                    memcpy(&dropdata.dude[w][x][y][z],&data[(z * 4) + 4],4);
                for (z = 0; z < 30; z++) // box id 
                {
                    memcpy(&dropdata.box[w][x][y][z],&data[(z * 4) + 0x01B2],4);
                    dropdata.area[w][x][y][z] = data[z + 0x0194];
                }
            }
        }
    }
    GSL_CloseArchive(gsl);
    return true;
}

// converts a byte-format (PSO-format) drop rate into an unsigned long.
// to use these drop rates, pick a random 32-bit number. if your random number is
// (unsigned) less than the 32-bit drop rate, then the rare item should drop.
unsigned long ExpandDropRate(BYTE pc)
{
    unsigned int shift = ((pc >> 3) & 0x1F) - 4;
    if (shift < 0) shift = 0;
    return ((2 << shift) * ((pc & 7) + 7));
}

////////////////////////////////////////////////////////////////////////////////

// this section goes through dropchart.ini, droptype.ini, and enemies.ini to build enemies.nsi.

// procedure for battle parameter file builder:
// - read line
// - copy name, exp ID, item ID, drop chances (by difficulty)
// - fetch nonrare drop type (by server ID in droptype.ini)
// - fetch drop rare chances and items (by item ID in itemraretable.pbb)
// - fetch exp value (use episode and exp ID; look in battleparamentry files)

// enemy info format used in enemies.nsi
typedef struct {
    char name[0x20];
    int serverID; // serverID is this enemy's index in the server's enemy list
    int expID; // expID is this enemy's index in the BattleParamEntry files
    int itemID; // itemID is this enemy's index in the ItemRT* files
    int episode;
    WORD hpValue[4]; // by difficulty 
    DWORD expValue[4]; // by difficulty 
    DWORD dropChance[4]; // by difficulty 
    BYTE dropType[4]; // by difficulty 
    DWORD dropRareChance[4][10]; // by difficulty, then section ID 
    BYTE dropRareItemCode[4][10][3]; // by difficulty, then section ID 
} DUDE_INFO;

CFGFile *enemiesini,*droptypeini,*dropchartini;

// this function is called for every entry in enemies.ini.
bool EnumRoutine(CFGFile* cf,char* name,char* valueA,wchar_t* valueW,HANDLE file)
{
    wchar_t wbuffer[0x200];
    char buffer[0x200];
    DUDE_INFO dude;
    int x,y,diff;
    DWORD bytesread;

    // erase current enemy info structure
    memset(&dude,0,sizeof(DUDE_INFO));

    // get the enemy ID from the name, then copy the enemy's name from the value
    sscanf(name,"Dude%2X",&dude.serverID);
    for (x = 0; (valueA[x] != ',') && valueA[x]; x++) dude.name[x] = valueA[x];

    // after the name comes a comma, the episode that this enemy is in, another comma, and the contents of the sscanf below (you can figure it out)
    dude.episode = valueA[x + 1] - 0x0030;
    sscanf(&valueA[x + 3],"%2X,%2X,%f,%f,%f,%f",&dude.expID,&dude.itemID,&dude.dropChance[0],&dude.dropChance[1],&dude.dropChance[2],&dude.dropChance[3]);

    // if drop chances are zero past normal difficulty, use the previous difficulty's chance
    for (x = 1; x < 4; x++) if (!dude.dropChance[x]) dude.dropChance[x] = dude.dropChance[x - 1];

    // dropChance is currently a double in a DWORD's spot, so we'll convert it from a double (out of 100) to a DWORD (out of 0xFFFFFFFF)
    for (x = 0; x < 4; x++) dude.dropChance[x] = (DWORD)((*(float*)(&dude.dropChance[x]) * (double)0xFFFFFFFF) / 100.0f);

    // in each difficulty....
    for (diff = 0; diff < 4; diff++)
    {
        // find this enemy's drop type entry
        sprintf(buffer,"Drop_Item_%02X_%02X",diff,dude.serverID);
        if (CFGIsValuePresent(droptypeini,buffer)) sscanf(CFGGetStringA(droptypeini,buffer),"%2X",&dude.dropType[diff]);

        // if this is not an Ep4 enemy, then we have its HP and EXP loaded already, as well as rare item data
        if (dude.episode < 3)
        {
            dude.hpValue[diff] = MonsterHP(dude.episode,diff,dude.expID);
            dude.expValue[diff] = MonsterEXP(dude.episode,diff,dude.expID);
            for (x = 0; x < 10; x++)
            {
                dude.dropRareChance[diff][x] = ExpandDropRate(dropdata.dude[dude.episode - 1][diff][x][dude.itemID - 1].prob);
                memcpy(dude.dropRareItemCode[diff][x],dropdata.dude[dude.episode - 1][diff][x][dude.itemID - 1].item,3);
            }
        }
    }

    // similarly to drop chances, if drop types are missing beyond normal, copy them from the previous difficulty
    for (x = 1; x < 4; x++) if (!dude.dropType[x]) dude.dropType[x] = dude.dropType[x - 1];

    // and save it
    WriteFile(file,&dude,sizeof(dude),&bytesread,NULL);
    return true;
}

// this function is called for every entry in the Ep4 drop chart (dropchart.ini). it skips over entries that describe box rares.
// this function should likely be rewritten. in fact, this program should be rewritten, or the entire enemy library should be rewritten.
bool EnumRoutine_DropChart_Dudes(CFGFile* cf,char* name,char* valueA,wchar_t* valueW,DUDE_INFO* dudes)
{
    if (memcmp(name,L"Drop_",10)) return true;

    int x,y,episode,diff,dudeID;
    BYTE itemcode[3];
    unsigned long chance;
    double fchance;

    sscanf(name,"Drop_%2X_%2X_%2X",&episode,&diff,&dudeID);

    for (x = 0; valueA[x] && (valueA[x] != ','); x++);
    valueA[x] = 0;
    sscanf(&valueA[x + 1],"%2X%2X%2X",&itemcode[0],&itemcode[1],&itemcode[2]);
    x += 8;
    for (y = x; valueA[y]; y++) if (valueA[y] == '/') break;
    if (valueA[y])
    {
        sscanf(&valueA[x],"%d/%d",&x,&y);
        fchance = (double)x / (double)y;
    } else {
        sscanf(&valueA[x],"%f",&fchance);
    }
    chance = (int)(fchance * (double)0xFFFFFFFF);

    bool doIDs[10];
    memset(doIDs,0,sizeof(doIDs));
    if (!strcmp(valueA,"all")) for (x = 0; x < 10; x++) doIDs[x] = true;
    else for (x = 0; valueA[x]; x++) doIDs[valueA[x] - '0'] = true;

    DUDE_INFO dude;
    for (y = 0; ; y++) if (dudes[y].serverID == dudeID) break;
    memcpy(&dude,&dudes[y],sizeof(DUDE_INFO));
    for (x = 0; x < 10; x++)
    {
        if (doIDs[x])
        {
            dude.dropRareChance[diff][x] = chance;
            memcpy(dude.dropRareItemCode[diff][x],itemcode,3);
        }
    }
    memcpy(&dudes[y],&dude,sizeof(DUDE_INFO));

    return true;
}

////////////////////////////////////////////////////////////////////////////////

// This section gathers all the information on box rares together into one array of BOXRARE_INFOs.

typedef struct {
    BYTE episode;
    BYTE difficulty;
    BYTE secid;
    BYTE area;
    DWORD dropRareChance; // by difficulty, section ID 
    BYTE dropRareItemCode[3]; // by difficulty, section ID 
    BYTE unused;
} BOXRARE_INFO;

int numBoxRares;
BOXRARE_INFO boxrares[0x800];//[3][4][10][30];

void ConsolidateBoxRares()
{
    int v,w,x,y,z;
    z = 0;
    for (v = 0; v < 2; v++)
    {
        for (w = 0; w < 4; w++)
        {
            for (x = 0; x < 10; x++)
            {
                for (y = 0; y < 30; y++)
                {
                    if (dropdata.area[v][w][x][y] != 0xFF)
                    {
                        boxrares[z].episode = v;
                        boxrares[z].difficulty = w;
                        boxrares[z].secid = x;
                        boxrares[z].area = dropdata.area[v][w][x][y];
                        boxrares[z].dropRareChance = ExpandDropRate(dropdata.box[v][w][x][y].prob);
                        memcpy(boxrares[z].dropRareItemCode,dropdata.box[v][w][x][y].item,3);
                        if (boxrares[z].dropRareChance) z++;
                    }
                }
            }
        }
    }
    numBoxRares = z;
}

bool EnumRoutine_DropChart_Boxes(CFGFile* cf,const char* name,const char* valueA,const wchar_t* valueW,long param)
{
    if (memcmp(name,L"BoxRares_",18) && memcmp(name,L"BoxRare_",16)) return true;

    int v,w,x,y,episode,diff,area;
    BYTE itemcode[3];
    unsigned long chance;
    double fchance;

    if (!memcmp(name,"BoxRare_",16)) sscanf(name,"BoxRare_%2X_%2X_%2X",&episode,&diff,&area);
    if (!memcmp(name,"BoxRares_",18))
    {
        sscanf(name,"BoxRares_%2X_%2X",&episode,&diff);
        area = 0xFF;
    }

    sscanf(valueA,"%2X%2X%2X",&itemcode[0],&itemcode[1],&itemcode[2]);
    x = 7;
    for (y = x; valueA[y]; y++) if (valueA[y] == '/') break;
    if (valueA[y])
    {
        sscanf(&valueA[x],"%d/%d",&x,&y);
        fchance = (double)x / (double)y;
    } else {
        sscanf(&valueA[x],"%f",&fchance);
    }
    //chance = (int)(fchance * (double)0x80000000) << 1;

    for (x = 0; x < 10; x++)
    {
        /*for (v = 0; v < 30; v++) if (!boxrares[episode][diff][x][v].area) break;
        boxrares[episode][diff][x][v].episode = episode;
        boxrares[episode][diff][x][v].difficulty = diff;
        boxrares[episode][diff][x][v].secid = x;
        boxrares[episode][diff][x][v].area = area;
        boxrares[episode][diff][x][v].dropRareChance = (DWORD)(fchance * (double)0xFFFFFFFF);
        memcpy(boxrares[episode][diff][x][v].dropRareItemCode,itemcode,3); */
        boxrares[numBoxRares].episode = episode;
        boxrares[numBoxRares].difficulty = diff;
        boxrares[numBoxRares].secid = x;
        boxrares[numBoxRares].area = area;
        boxrares[numBoxRares].dropRareChance = (DWORD)(fchance * (double)0xFFFFFFFF);
        memcpy(boxrares[numBoxRares].dropRareItemCode,itemcode,3);
        numBoxRares++;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////

int __stdcall WinMain(HINSTANCE,HINSTANCE,char* filename,int)
{
    // load all data files
    dropchartini = CFGLoadFile("dropchart.ini");
    enemiesini = CFGLoadFile("enemies.ini");
    droptypeini = CFGLoadFile("droptype.ini");
    LoadDropInfoFile();
    LoadMonsterEXPs();

    // create output files
    HANDLE enem = CreateFile("enemies.pbb",GENERIC_READ | GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);
    HANDLE boxr = CreateFile("boxrares.pbb",GENERIC_READ | GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);

    // create enemy list
    CFGEnumValues(enemiesini,(CFGEnumRoutine)EnumRoutine,(long)enem);

    // reload enemy list generated by EnumRoutine
    DWORD bytesread;
    int size = GetFileSize(enem,NULL);
    DUDE_INFO* dudes = (DUDE_INFO*)malloc(size);
    SetFilePointer(enem,0,NULL,FILE_BEGIN);
    ReadFile(enem,dudes,size,&bytesread,NULL);

    // apply drop chart to enemy list
    CFGEnumValues(dropchartini,(CFGEnumRoutine)EnumRoutine_DropChart_Dudes,(long)dudes);

    // process box rare information
    ConsolidateBoxRares();
    CFGEnumValues(dropchartini,(CFGEnumRoutine)EnumRoutine_DropChart_Boxes,(long)dudes);

    // save enemy list
    SetFilePointer(enem,0,NULL,FILE_BEGIN);
    WriteFile(enem,dudes,size,&bytesread,NULL);
    CloseHandle(enem);

    // save box rare list
    WriteFile(boxr,&boxrares,numBoxRares * sizeof(BOXRARE_INFO),&bytesread,NULL);
    CloseHandle(boxr);

    return 0;
}

