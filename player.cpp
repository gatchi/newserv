#include <windows.h>
#include <stdio.h>

#include "text.h"

#include "version.h"
#include "player.h"

// converts PC/GC player data to BB format
void ConvertPCGCDispDataToBB(BB_PLAYER_DISPDATA* bb,PCGC_PLAYER_DISPDATA* pcgc)
{
    bb->stats.atp = pcgc->stats.atp;
    bb->stats.mst = pcgc->stats.mst;
    bb->stats.evp = pcgc->stats.evp;
    bb->stats.hp = pcgc->stats.hp;
    bb->stats.dfp = pcgc->stats.dfp;
    bb->stats.ata = pcgc->stats.ata;
    bb->stats.lck = pcgc->stats.lck;
    bb->unknown1 = pcgc->unknown1;
    bb->unknown2[0] = pcgc->unknown2[0];
    bb->unknown2[1] = pcgc->unknown2[1];
    bb->level = pcgc->level;
    bb->exp = pcgc->exp;
    bb->meseta = pcgc->meseta;
    strcpy(bb->guildcard,"         0");
    bb->unknown3[0] = pcgc->unknown3[0];
    bb->unknown3[1] = pcgc->unknown3[1];
    bb->nameColor = pcgc->nameColor;
    bb->extraModel = pcgc->extraModel;
    memcpy(&bb->unused,&pcgc->unused,15);
    bb->nameColorChecksum = pcgc->nameColorChecksum;
    bb->sectionID = pcgc->sectionID;
    bb->charClass = pcgc->charClass;
    bb->v2flags = pcgc->v2flags;
    bb->version = pcgc->version;
    bb->v1flags = pcgc->v1flags;
    bb->costume = pcgc->costume;
    bb->skin = pcgc->skin;
    bb->face = pcgc->face;
    bb->head = pcgc->head;
    bb->hair = pcgc->hair;
    bb->hairRed = pcgc->hairRed;
    bb->hairGreen = pcgc->hairGreen;
    bb->hairBlue = pcgc->hairBlue;
    bb->propX = pcgc->propX;
    bb->propY = pcgc->propY;
    tx_convert_to_unicode(bb->playername,pcgc->playername);
    tx_add_language_marker(pcgc->playername,'J');
    memcpy(&bb->config,&pcgc->config,0x48);
    memcpy(&bb->techLevels,&pcgc->techLevels,0x14);
}

// converts BB player data to PC/GC format
void ConvertBBDispDataToPCGC(PCGC_PLAYER_DISPDATA* pcgc,BB_PLAYER_DISPDATA* bb)
{
    pcgc->stats.atp = bb->stats.atp;
    pcgc->stats.mst = bb->stats.mst;
    pcgc->stats.evp = bb->stats.evp;
    pcgc->stats.hp = bb->stats.hp;
    pcgc->stats.dfp = bb->stats.dfp;
    pcgc->stats.ata = bb->stats.ata;
    pcgc->stats.lck = bb->stats.lck;
    pcgc->unknown1 = bb->unknown1;
    pcgc->unknown2[0] = bb->unknown2[0];
    pcgc->unknown2[1] = bb->unknown2[1];
    pcgc->level = bb->level;
    pcgc->exp = bb->exp;
    pcgc->meseta = bb->meseta;
    pcgc->unknown3[0] = bb->unknown3[0];
    pcgc->unknown3[1] = bb->unknown3[1];
    pcgc->nameColor = bb->nameColor;
    pcgc->extraModel = bb->extraModel;
    memcpy(&pcgc->unused,&bb->unused,15);
    pcgc->nameColorChecksum = bb->nameColorChecksum;
    pcgc->sectionID = bb->sectionID;
    pcgc->charClass = bb->charClass;
    pcgc->v2flags = bb->v2flags;
    pcgc->version = bb->version;
    pcgc->v1flags = bb->v1flags;
    pcgc->costume = bb->costume;
    pcgc->skin = bb->skin;
    pcgc->face = bb->face;
    pcgc->head = bb->head;
    pcgc->hair = bb->hair;
    pcgc->hairRed = bb->hairRed;
    pcgc->hairGreen = bb->hairGreen;
    pcgc->hairBlue = bb->hairBlue;
    pcgc->propX = bb->propX;
    pcgc->propY = bb->propY;
    tx_convert_to_sjis(pcgc->playername,bb->playername);
    tx_remove_language_marker(pcgc->playername);
    memcpy(&pcgc->config,&bb->config,0x48);
    memcpy(&pcgc->techLevels,&bb->techLevels,0x14);
}

// creates a player preview, which can then be sent to a BB client for character select
void CreatePlayerPreview(BB_PLAYER_DISPDATA_PREVIEW* pre,BB_PLAYER_DISPDATA* bb)
{
    pre->level = bb->level;
    pre->exp = bb->exp;
    strcpy(pre->guildcard,bb->guildcard);
    pre->unknown3[0] = bb->unknown3[0];
    pre->unknown3[1] = bb->unknown3[1];
    pre->nameColor = bb->nameColor;
    pre->extraModel = bb->extraModel;
    memcpy(&pre->unused,&bb->unused,11);
    pre->nameColorChecksum = bb->nameColorChecksum;
    pre->sectionID = bb->sectionID;
    pre->charClass = bb->charClass;
    pre->v2flags = bb->v2flags;
    pre->version = bb->version;
    pre->v1flags = bb->v1flags;
    pre->costume = bb->costume;
    pre->skin = bb->skin;
    pre->face = bb->face;
    pre->head = bb->head;
    pre->hair = bb->hair;
    pre->hairRed = bb->hairRed;
    pre->hairGreen = bb->hairGreen;
    pre->hairBlue = bb->hairBlue;
    pre->propX = bb->propX;
    pre->propY = bb->propY;
    wcscpy(pre->playername,bb->playername);
    pre->playTime = bb->playTime;
}

// copies the data in a player preview into a full player data structure.
// this is used when a BB player creates a character; the client sends back a
// preview structure with the new player's data.
void ApplyPlayerPreview(BB_PLAYER_DISPDATA* bb,BB_PLAYER_DISPDATA_PREVIEW* pre)
{
    bb->level = pre->level;
    bb->exp = pre->exp;
    strcpy(bb->guildcard,pre->guildcard);
    bb->unknown3[0] = pre->unknown3[0];
    bb->unknown3[1] = pre->unknown3[1];
    bb->nameColor = pre->nameColor;
    bb->extraModel = pre->extraModel;
    memcpy(&bb->unused,&pre->unused,11);
    bb->nameColorChecksum = pre->nameColorChecksum;
    bb->sectionID = pre->sectionID;
    bb->charClass = pre->charClass;
    bb->v2flags = pre->v2flags;
    bb->version = pre->version;
    bb->v1flags = pre->v1flags;
    bb->costume = pre->costume;
    bb->skin = pre->skin;
    bb->face = pre->face;
    bb->head = pre->head;
    bb->hair = pre->hair;
    bb->hairRed = pre->hairRed;
    bb->hairGreen = pre->hairGreen;
    bb->hairBlue = pre->hairBlue;
    bb->propX = pre->propX;
    bb->propY = pre->propY;
    wcscpy(bb->playername,pre->playername);
    bb->playTime = 0;//pre->playTime;
}

// reads player data from the 61/98 commands
void ImportPlayerData(void* buffer,PLAYER* p,BYTE version)
{
    PC_PLAYER_DATA_FORMAT* pc = (PC_PLAYER_DATA_FORMAT*)buffer;
    GC_PLAYER_DATA_FORMAT* gc = (GC_PLAYER_DATA_FORMAT*)buffer;
    BB_PLAYER_DATA_FORMAT* bb = (BB_PLAYER_DATA_FORMAT*)buffer;

    switch (version)
    {
      case VERSION_PC:
        memcpy(&p->inventory,&pc->inventory,sizeof(PLAYER_INVENTORY));
        ConvertPCGCDispDataToBB(&p->disp,&pc->disp);
        /*tx_convert_to_unicode(p->infoboard,pc->infoboard);
        memcpy(&p->blocked,pc->blocked,sizeof(DWORD) * 30);
        if (pc->autoReplyEnabled) tx_convert_to_unicode(p->autoreply,pc->autoreply);
        else*/ p->autoreply[0] = 0;
        break;
      case VERSION_GAMECUBE:
        memcpy(&p->inventory,&gc->inventory,sizeof(PLAYER_INVENTORY));
        ConvertPCGCDispDataToBB(&p->disp,&gc->disp);
        tx_convert_to_unicode(p->infoboard,gc->infoboard);
        memcpy(&p->blocked,gc->blocked,sizeof(DWORD) * 30);
        if (gc->autoReplyEnabled) tx_convert_to_unicode(p->autoreply,gc->autoreply);
        else p->autoreply[0] = 0;
        break;
      case VERSION_BLUEBURST:
        //memcpy(&p->inventory,&bb->inventory,sizeof(PLAYER_INVENTORY));
        //memcpy(&p->disp,&bb->disp,sizeof(BB_PLAYER_DISPDATA));
        wcscpy(p->infoboard,bb->infoboard);
        memcpy(&p->blocked,bb->blocked,sizeof(DWORD) * 30);
        if (bb->autoReplyEnabled) wcscpy(p->autoreply,bb->autoreply);
        else p->autoreply[0] = 0;
        break;
    }
}

// generates data for 65/67/68 commands (joining games/lobbies)
void ExportLobbyPlayerData(void* buffer,PLAYER* p,BYTE version)
{
    PCGC_LOBBY_PLAYER_DATA_FORMAT* pcgc = (PCGC_LOBBY_PLAYER_DATA_FORMAT*)buffer;
    BB_LOBBY_PLAYER_DATA_FORMAT* bb = (BB_LOBBY_PLAYER_DATA_FORMAT*)buffer;

    switch (version)
    {
      case VERSION_PC:
        memcpy(&pcgc->inventory,&p->inventory,sizeof(PLAYER_INVENTORY));
        ConvertBBDispDataToPCGC(&pcgc->disp,&p->disp);
        // we need to be careful; PC has less classes, so we'll substitute some here
        if (pcgc->disp.charClass == 11) pcgc->disp.charClass = 0; // fomar -> humar 
        if (pcgc->disp.charClass == 10) pcgc->disp.charClass = 1; // ramarl -> hunewearl 
        if (pcgc->disp.charClass == 9) pcgc->disp.charClass = 5; // hucaseal -> racaseal 
        // if the player is still not a valid class, make them appear as the "ninja" NPC
        if (pcgc->disp.charClass > 8)
        {
            pcgc->disp.extraModel = 0;
            pcgc->disp.v2flags |= 2;
        }
        pcgc->disp.version = 2;
        break;
      case VERSION_GAMECUBE:
        memcpy(&pcgc->inventory,&p->inventory,sizeof(PLAYER_INVENTORY));
        ConvertBBDispDataToPCGC(&pcgc->disp,&p->disp);
        break;
      case VERSION_BLUEBURST:
        memcpy(&bb->inventory,&p->inventory,sizeof(PLAYER_INVENTORY));
        memcpy(&bb->disp,&p->disp,sizeof(BB_PLAYER_DISPDATA));
        break;
    }
}

// generates player data in complete BB format (i.e. E7 command format)
void ExportCompletePlayerData(BB_COMPLETE_PLAYER_DATA_FORMAT* bb,PLAYER* p)
{
    memcpy(&bb->inventory,&p->inventory,sizeof(PLAYER_INVENTORY));
    memcpy(&bb->disp,&p->disp,sizeof(BB_PLAYER_DISPDATA));
    memset(bb->unknown,0,0x10);
    bb->optionFlags = p->optionFlags;
    memcpy(bb->questData1,&p->questData1,0x0208);
    memcpy(&bb->bank,&p->bank,sizeof(PLAYER_BANK));
    bb->serialNumber = p->serialNumber;
    wcscpy(bb->name,p->disp.playername);
    wcscpy(bb->teamname,p->teamname);
    wcscpy(bb->guildcarddesc,p->guildcarddesc);
    bb->reserved1 = 0;
    bb->reserved2 = 0;
    bb->sectionID = p->disp.sectionID;
    bb->charClass = p->disp.charClass;
    bb->unknown3 = 0;
    memcpy(bb->symbolchats,p->symbolchats,0x04E0);
    memcpy(bb->shortcuts,p->shortcuts,0x0A40);
    wcscpy(bb->autoreply,p->autoreply);
    wcscpy(bb->infoboard,p->infoboard);
    memset(bb->unknown5,0,0x1C);
    memcpy(bb->challengeData,p->challengeData,0x0140);
    memcpy(bb->techMenuConfig,p->techMenuConfig,0x0028);
    memset(bb->unknown6,0,0x2C);
    memcpy(bb->questData2,&p->questData2,0x0058);
    memcpy(&bb->keyConfig,&p->keyConfig,sizeof(BB_KEY_TEAM_CONFIG));
}

////////////////////////////////////////////////////////////////////////////////

// checksums the guild card file for BB player account data
long CalculateGuildCardChecksum(void* data,unsigned long size)
{
    unsigned long offset,y,cs = 0xFFFFFFFF;
    for (offset = 0; offset < size; offset++)
    {
        cs ^= *(unsigned char*)((long)data + offset);
        for (y = 0; y < 8; y++)
        {
            if (!(cs & 1)) cs = (cs >> 1) & 0x7FFFFFFF;
            else cs = ((cs >> 1) & 0x7FFFFFFF) ^ 0xEDB88320;
        }
    }
    return (cs ^ 0xFFFFFFFF);
}

// loads BB account data from a .nsa file (guild cards, blocked senders, key config, chat options, etc.)
bool PlayerLoadAccountData(PLAYER* p,char* filename)
{
    HANDLE file;
    DWORD bytesread;
    BB_SAVED_ACCOUNT_DATA_FORMAT account;

    file = CreateFile(filename,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    if (file == INVALID_HANDLE_VALUE) return false;
    ReadFile(file,&account,sizeof(BB_SAVED_ACCOUNT_DATA_FORMAT),&bytesread,NULL);
    CloseHandle(file);

    if (strcmp(account.signature,ACCOUNT_FILE_SIGNATURE)) return false;

    memcpy(&p->blocked,&account.blocked,sizeof(DWORD) * 30);
    memcpy(&p->guildcards,&account.guildcards,sizeof(BB_GUILDCARD_FILE));
    memcpy(&p->keyConfig,&account.keyConfig,sizeof(BB_KEY_TEAM_CONFIG));
    p->optionFlags = account.optionFlags;
    memcpy(&p->shortcuts,&account.shortcuts,0x0A40);
    memcpy(&p->symbolchats,&account.symbolchats,0x04E0);
    wcscpy(p->teamname,account.teamname);
    return true;
}

// loads old-format (.act) BB account data
bool PlayerLoadAccountDataOldFormat(PLAYER* p,char* filename)
{
    HANDLE file;
    DWORD bytesread;

    file = CreateFile(filename,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    if (file == INVALID_HANDLE_VALUE) return false;
    ReadFile(file,&p->symbolchats,0x04E0,&bytesread,NULL);
    ReadFile(file,&p->keyConfig,0x0AF0,&bytesread,NULL);
    ReadFile(file,&p->guildcards,0xD590,&bytesread,NULL);
    ReadFile(file,&p->optionFlags,0x0004,&bytesread,NULL);
    ReadFile(file,&p->shortcuts,0x0A40,&bytesread,NULL);
    CloseHandle(file);
    return true;
}

// saves BB account data to a .nsa file
bool PlayerSaveAccountData(PLAYER* p,char* filename)
{
    DWORD bytesread;
    BB_SAVED_ACCOUNT_DATA_FORMAT account;
    HANDLE file = CreateFile(filename,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);
    if (file == INVALID_HANDLE_VALUE) return false;

    strcpy(account.signature,ACCOUNT_FILE_SIGNATURE);
    memcpy(&account.blocked,&p->blocked,sizeof(DWORD) * 30);
    memcpy(&account.guildcards,&p->guildcards,sizeof(BB_GUILDCARD_FILE));
    memcpy(&account.keyConfig,&p->keyConfig,sizeof(BB_KEY_TEAM_CONFIG));
    account.optionFlags = p->optionFlags;
    memcpy(&account.shortcuts,&p->shortcuts,0x0A40);
    memcpy(&account.symbolchats,&p->symbolchats,0x04E0);
    wcscpy(account.teamname,p->teamname);

    WriteFile(file,&account,sizeof(BB_SAVED_ACCOUNT_DATA_FORMAT),&bytesread,NULL);
    CloseHandle(file);
    return true;
}

// loads BB player data from a .nsc file.
bool PlayerLoadPlayerData(PLAYER* p,char* filename)
{
    HANDLE file;
    DWORD bytesread;
    BB_SAVED_PLAYER_DATA_FORMAT bb;

    file = CreateFile(filename,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    if (file == INVALID_HANDLE_VALUE) return false;
    ReadFile(file,&bb,sizeof(BB_SAVED_PLAYER_DATA_FORMAT),&bytesread,NULL);
    CloseHandle(file);

    if (strcmp(bb.signature,PLAYER_FILE_SIGNATURE)) return false;

    wcscpy(p->autoreply,bb.autoreply);
    memcpy(&p->bank,&bb.bank,sizeof(PLAYER_BANK));
    memcpy(&p->challengeData,&bb.challengeData,0x0140);
    memcpy(&p->disp,&bb.disp,sizeof(BB_PLAYER_DISPDATA));
    wcscpy(p->guildcarddesc,bb.guildcarddesc);
    wcscpy(p->infoboard,bb.infoboard);
    memcpy(&p->inventory,&bb.inventory,sizeof(PLAYER_INVENTORY));
    memcpy(&p->questData1,&bb.questData1,0x0208);
    memcpy(&p->questData2,&bb.questData2,0x0058);
    memcpy(&p->techMenuConfig,&bb.techMenuConfig,0x0028);
    return true;
}

// loads BB player data from an old-format (.pbb) file. ignores any account data present in the character file.
bool PlayerLoadPlayerDataOldFormat(PLAYER* p,char* filename)
{
    HANDLE file;
    DWORD bytesread;
    BB_COMPLETE_PLAYER_DATA_FORMAT bb;

    file = CreateFile(filename,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    if (file == INVALID_HANDLE_VALUE) return false;
    ReadFile(file,&bb,sizeof(BB_COMPLETE_PLAYER_DATA_FORMAT),&bytesread,NULL);
    CloseHandle(file);

    wcscpy(p->autoreply,bb.autoreply);
    memcpy(&p->bank,&bb.bank,sizeof(PLAYER_BANK));
    memcpy(&p->challengeData,&bb.challengeData,0x0140);
    memcpy(&p->disp,&bb.disp,sizeof(BB_PLAYER_DISPDATA));
    wcscpy(p->guildcarddesc,bb.guildcarddesc);
    wcscpy(p->infoboard,bb.infoboard);
    memcpy(&p->inventory,&bb.inventory,sizeof(PLAYER_INVENTORY));
    memcpy(&p->questData1,&bb.questData1,0x0208);
    memcpy(&p->questData2,&bb.questData2,0x0058);
    memcpy(&p->techMenuConfig,&bb.techMenuConfig,0x0028);
    return true;
}

// saves player data to a .nsc file.
bool PlayerSavePlayerData(PLAYER* p,char* filename)
{
    DWORD bytesread;
    BB_SAVED_PLAYER_DATA_FORMAT bb;
    HANDLE file = CreateFile(filename,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);
    if (file == INVALID_HANDLE_VALUE) return false;

    strcpy(bb.signature,PLAYER_FILE_SIGNATURE);
    CreatePlayerPreview(&bb.preview,&p->disp);
    wcscpy(bb.autoreply,p->autoreply);
    memcpy(&bb.bank,&p->bank,sizeof(PLAYER_BANK));
    memcpy(&bb.challengeData,&p->challengeData,0x0140);
    memcpy(&bb.disp,&p->disp,sizeof(BB_PLAYER_DISPDATA));
    wcscpy(bb.guildcarddesc,p->guildcarddesc);
    wcscpy(bb.infoboard,p->infoboard);
    memcpy(&bb.inventory,&p->inventory,sizeof(PLAYER_INVENTORY));
    memcpy(&bb.questData1,&p->questData1,0x0208);
    memcpy(&bb.questData2,&p->questData2,0x0058);
    memcpy(&bb.techMenuConfig,&p->techMenuConfig,0x0028);

    WriteFile(file,&bb,sizeof(BB_SAVED_PLAYER_DATA_FORMAT),&bytesread,NULL);
    CloseHandle(file);
    return true;
}

// loads a bank file (.nsb)
bool PlayerLoadBankData(PLAYER_BANK* b,char* filename)
{
    HANDLE file;
    DWORD bytesread;
    file = CreateFile(filename,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    if (file == INVALID_HANDLE_VALUE) return false;
    ReadFile(file,b,sizeof(PLAYER_BANK),&bytesread,NULL);
    CloseHandle(file);

    unsigned long x;
    unsigned long id = 0x0F010000;
    for (x = 0; x < b->numItems; x++)
    {
        b->items[x].data.itemID = id;
        id++;
    }
    return true;
}

// saves a bank file (.nsb)
bool PlayerSaveBankData(PLAYER_BANK* b,char* filename)
{
    HANDLE file;
    DWORD bytesread;
    file = CreateFile(filename,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);
    if (file == INVALID_HANDLE_VALUE) return false;
    WriteFile(file,b,sizeof(PLAYER_BANK),&bytesread,NULL);
    CloseHandle(file);
    return true;
}

// switches a player's active bank
bool SwitchPlayerBank(PLAYER_BANK* b,char* saveFilename,char* loadFilename)
{
    if (!PlayerSaveBankData(b,saveFilename)) return false;
    return PlayerLoadBankData(b,loadFilename);
}

////////////////////////////////////////////////////////////////////////////////

DWORD combineItems[] = {0x00000003,0x00010003,0x00020003,0x00000103,0x00010103,0x00020103,0x00000303,0x00000403,0x00000503,0x00000603,0x00010603,0x00000703,0x00000803,0x00001003,0x00011003,0x00021003,0x00000000};
DWORD combineMaxes[] = {        10,        10,        10,        10,        10,        10,        10,        10,        10,        10,        10,        10,        10,        99,        99,        99,         0};
DWORD mesetaIdentifier = 0x00000004;

// converts an inventory item to a bank item
void ConvertToBankItem(PLAYER_BANK_ITEM* bankitem,PLAYER_ITEM* item)
{
    int x;
    memcpy(&bankitem->data,&item->data,sizeof(ITEM_DATA));
    for (x = 0; combineItems[x]; x++) if (!memcmp(&combineItems[x],item->data.itemData1,3)) break;
    if (combineItems[x]) bankitem->amount = item->data.itemData1[5];
    else bankitem->amount = 1;
    bankitem->showflags = 1;
}

// converts a bank item to an inventory item
void ConvertToInventoryItem(PLAYER_ITEM* item,PLAYER_BANK_ITEM* bankitem)
{
    memcpy(&item->data,&bankitem->data,sizeof(ITEM_DATA));
    if (item->data.itemData1[0] > 2) item->equipFlags = 0x0044;
    else item->equipFlags = 0x0050;
    item->equipFlags = 0x0001;
    item->techFlag = 0x0001;
}

// adds an item to a player's inventory
int AddItem(PLAYER* p,PLAYER_ITEM* item)
{
    int x,y;
    // is it meseta? then just add to the meseta total
    if (!memcmp(&mesetaIdentifier,item->data.itemData1,3))
    {
        x = item->data.itemData2dword;
        if ((p->disp.meseta + x) > 999999) return 586529131;
        p->disp.meseta += x;
        return 0;
    }

    // is it a combine item?
    for (x = 0; combineItems[x]; x++) if (!memcmp(&combineItems[x],item->data.itemData1,3)) break;
    if (combineItems[x])
    {
        // is there already a stack of it in the player's inventory?
        for (y = 0; y < p->inventory.numItems; y++) if (!memcmp(p->inventory.items[y].data.itemData1,item->data.itemData1,3)) break;
        if (y < p->inventory.numItems)
        {
            // if yes, add to the stack and return
            p->inventory.items[y].data.itemData1[5] += item->data.itemData1[5];
            if (p->inventory.items[y].data.itemData1[5] > combineMaxes[x]) return 586529132;
            return 0;
        }
    }

    // else, just add the item if there's room
    if (p->inventory.numItems >= 30) return 698553456;
    memcpy(&p->inventory.items[p->inventory.numItems],item,sizeof(PLAYER_ITEM));
    p->inventory.numItems++;
    return 0;
}

// adds an item to a bank
int AddItem(PLAYER_BANK* bank,PLAYER_BANK_ITEM* item)
{
    unsigned int x,y;
    // is it meseta? then just add to the meseta total
    if (!memcmp(&mesetaIdentifier,item->data.itemData1,3))
    {
        x = item->data.itemData2dword;
        if ((bank->meseta + x) > 999999) return 586529131;
        bank->meseta += x;
        return 0;
    }

    // is it a combine item?
    for (x = 0; combineItems[x]; x++) if (!memcmp(&combineItems[x],item->data.itemData1,3)) break;
    if (combineItems[x])
    {
        // is there already a stack of it in the bank?
        for (y = 0; y < bank->numItems; y++) if (!memcmp(bank->items[y].data.itemData1,item->data.itemData1,3)) break;
        if (y < bank->numItems)
        {
            // if yes, add to the stack and return
            bank->items[y].data.itemData1[5] += item->data.itemData1[5];
            bank->items[y].amount = bank->items[y].data.itemData1[5];
            if (bank->items[y].data.itemData1[5] > combineMaxes[x]) return 586529132;
            return 0;
        }
    }

    // else, just add the item if there's room
    if (bank->numItems >= 200) return 698553457;
    memcpy(&bank->items[bank->numItems],item,sizeof(PLAYER_BANK_ITEM));
    if (combineItems[x]) bank->items[y].amount = bank->items[y].data.itemData1[5];
    else bank->items[y].amount = 1;
    bank->numItems++;
    return 0;
}

// removes an item from a player's inventory.
// note: setting itemID to 0xFFFFFFFF removes meseta instead
// if you are not removing meseta, then item can be NULL, in which case the removed item is not returned
int RemoveItem(PLAYER* p,DWORD itemID,DWORD amount,PLAYER_ITEM* item)
{
    // are we removing meseta? then create a meseta item
    if (itemID == 0xFFFFFFFF)
    {
        if (amount > p->disp.meseta) return 586529131;
        memset(item,0,sizeof(PLAYER_ITEM));
        item->data.itemData1[0] = 0x04;
        item->data.itemData2dword = amount;
        p->disp.meseta -= amount;
        return 0;
    }

    // find this item
    int index = FindItem(&p->inventory,itemID);
    if (index == (-1)) return 698553466;

    // are we removing a certain amount of it? (amount == 0 means remove all of it)
    if (amount)
    {
        int x;
        // is it a combine item?
        for (x = 0; combineItems[x]; x++) if (!memcmp(&combineItems[x],p->inventory.items[index].data.itemData1,3)) break;
        if (combineItems[x])
        {
            // are we removing less than we have?
            if (amount < p->inventory.items[index].data.itemData1[5])
            {
                // if yes, then copy the item (if needed) and shorten the stack
                if (item)
                {
                    memcpy(item,&p->inventory.items[index],sizeof(PLAYER_ITEM));
                    item->data.itemData1[5] = amount;
                }
                p->inventory.items[index].data.itemData1[5] -= amount;
                return ERROR_COMBINE_ITEM_SPLIT;
            }
        }
    }

    // not a combine item, or we're removing the whole stack? then just remove the item
    if (item) memcpy(item,&p->inventory.items[index],sizeof(PLAYER_ITEM));
    p->inventory.numItems--;
    memcpy(&p->inventory.items[index],&p->inventory.items[index + 1],sizeof(PLAYER_ITEM) * (p->inventory.numItems - index));
    return 0;
}

// removes an item from a bank. works just like RemoveItem for inventories; I won't comment it
int RemoveItem(PLAYER_BANK* bank,DWORD itemID,DWORD amount,PLAYER_BANK_ITEM* item)
{
    if (itemID == 0xFFFFFFFF)
    {
        if (amount > bank->meseta) return 586529131;
        memset(item,0,sizeof(PLAYER_ITEM));
        item->data.itemData1[0] = 0x04;
        item->data.itemData2dword = amount;
        bank->meseta -= amount;
        return 0;
    }

    int index = FindItem(bank,itemID);
    if (index == (-1)) return 698553466;

    if (amount)
    {
        int x;
        for (x = 0; combineItems[x]; x++) if (!memcmp(&combineItems[x],bank->items[index].data.itemData1,3)) break;
        if (combineItems[x])
        {
            if (amount < bank->items[index].data.itemData1[5])
            {
                if (item)
                {
                    memcpy(item,&bank->items[index],sizeof(PLAYER_BANK_ITEM));
                    item->data.itemData1[5] = amount;
                    item->amount = amount;
                }
                bank->items[index].data.itemData1[5] -= amount;
                bank->items[index].amount -= amount;
                return ERROR_COMBINE_ITEM_SPLIT;
            }
        }
    }

    if (item) memcpy(item,&bank->items[index],sizeof(PLAYER_BANK_ITEM));
    bank->numItems--;
    memcpy(&bank->items[index],&bank->items[index + 1],sizeof(PLAYER_BANK_ITEM) * (bank->numItems - index));
    return 0;
}

// finds an item in a player's inventory.
int FindItem(PLAYER_INVENTORY* inv,DWORD itemID)
{
    int x;
    for (x = 0; x < inv->numItems; x++) if (inv->items[x].data.itemID == itemID) break;
    if (x >= inv->numItems) return (-1);
    return x;
}

// finds an item in a bank.
int FindItem(PLAYER_BANK* bank,DWORD itemID)
{
    unsigned int x;
    for (x = 0; x < bank->numItems; x++) if (bank->items[x].data.itemID == itemID) break;
    if (x >= bank->numItems) return (-1);
    return x;
}
