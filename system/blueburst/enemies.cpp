#include <windows.h>
#include <stdio.h>

typedef struct {
    char name[0x20];
    int serverID;
    int expID;
    int itemID;
    int episode;
    WORD hpValue[4];
    DWORD expValue[4];
    DWORD dropChance[4]; // by difficulty 
    BYTE dropType[4]; // by difficulty 
    DWORD dropRareChance[4][10]; // by difficulty, section ID 
    BYTE dropRareItemCode[4][10][3]; // by difficulty, section ID 
} DUDE_INFO;

typedef struct {
    int numDudes;
    DUDE_INFO dudes[0];
} DUDE_INFO_SET;

DUDE_INFO_SET* LoadDudeInfo(char* filename)
{
    DWORD bytesread;
    HANDLE file = CreateFile(filename,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    if (file == INVALID_HANDLE_VALUE) return NULL;
    DWORD numDudes = 0;
    numDudes = GetFileSize(file,NULL) / sizeof(DUDE_INFO);
    DUDE_INFO_SET* dudes = (DUDE_INFO_SET*)malloc(sizeof(DUDE_INFO_SET) + (numDudes * sizeof(DUDE_INFO)));
    if (!dudes) return NULL;
    dudes->numDudes = numDudes;
    ReadFile(file,dudes->dudes,numDudes * sizeof(DUDE_INFO),&bytesread,NULL);
    CloseHandle(file);
    return dudes;
}

void FreeDudeInfo(DUDE_INFO_SET* dudes) { if (dudes) free(dudes); }

DUDE_INFO* FindDudeInfoServer(DUDE_INFO_SET* dudes,int serverID)
{
    int x;
    for (x = 0; x < dudes->numDudes; x++) if (dudes->dudes[x].serverID == serverID) return &dudes->dudes[x];
    return NULL;
}

DUDE_INFO* FindDudeInfoEXP(DUDE_INFO_SET* dudes,int expID)
{
    int x;
    for (x = 0; x < dudes->numDudes; x++) if (dudes->dudes[x].expID == expID) return &dudes->dudes[x];
    return NULL;
}

DUDE_INFO* FindDudeInfoItem(DUDE_INFO_SET* dudes,int itemID)
{
    int x;
    for (x = 0; x < dudes->numDudes; x++) if (dudes->dudes[x].itemID == itemID) return &dudes->dudes[x];
    return NULL;
}

int main(int argc,char* argv[])
{
    DUDE_INFO_SET* dudes = LoadDudeInfo("enemies.nsi");
    if (!dudes)
    {
        printf("> error; couldn't load enemies.nsi\n");
        return 0;
    }

    DWORD x;
    if (argc > 1)
    {
        FILE* f = fopen(argv[1],"wt");
        fprintf(f,"enemies.nsi\n\n");
        for (x = 0; x < dudes->numDudes; x++)
        {
            if ((x & 0x1F) == 0) fprintf(f,"srvid itemid expid ep exp0     hp0  hp1  hp2  hp3  name\n");
            fprintf(f,"%02X    %02X     %02X    %d  %08X %04X %04X %04X %04X %s\n",dudes->dudes[x].serverID,dudes->dudes[x].itemID,dudes->dudes[x].expID,dudes->dudes[x].episode,dudes->dudes[x].expValue[0],dudes->dudes[x].hpValue[0],dudes->dudes[x].hpValue[1],dudes->dudes[x].hpValue[2],dudes->dudes[x].hpValue[3],dudes->dudes[x].name);
            x++;
        }
        fclose(f);
    } else {
        printf("\nenemies.nsi\n\n");
        for (x = 0; x < dudes->numDudes; x++)
        {
            if ((x & 0x1F) == 0) printf("srvid itemid expid ep exp0     hp0  hp1  hp2  hp3  name\n");
            printf("%02X    %02X     %02X    %d  %08X %04X %04X %04X %04X %s\n",dudes->dudes[x].serverID,dudes->dudes[x].itemID,dudes->dudes[x].expID,dudes->dudes[x].episode,dudes->dudes[x].expValue[0],dudes->dudes[x].hpValue[0],dudes->dudes[x].hpValue[1],dudes->dudes[x].hpValue[2],dudes->dudes[x].hpValue[3],dudes->dudes[x].name);
            x++;
        }
    }
    FreeDudeInfo(dudes);
    system("PAUSE>NUL");
    return 0;
}

