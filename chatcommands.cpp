#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "text.h"
#include "operation.h"
#include "console.h"

#include "netconfig.h"
#include "quest.h"

#include "encryption.h"
#include "license.h"
#include "privilege.h"
#include "version.h"
#include "battleparamentry.h"
#include "itemraretable.h"
#include "player.h"
#include "map.h"
#include "client.h"

#include "listenthread.h"
#include "lobby.h"
#include "server.h"

#include "command-functions.h"
#include "chatcommands.h"

// In this file, I'll omit commenting each chat command function.... it's probably
// pretty obvious which is which, and how each works. I'll comment the dispatcher
// at the end, though.

extern QUESTLIST* quests;
extern LICENSE_LIST* licenses;
extern char informationMenuFileNames[0x20][0x20];
extern SHIP_SELECT_MENU informationMenu;

////////////////////////////////////////////////////////////////////////////////

wchar_t* sectionIDNames[]  = {L"Viridia",L"Greennill",L"Skyly",L"Bluefull",L"Purplenum",L"Pinkal",L"Redria",L"Oran",L"Yellowboze",L"Whitill",NULL};
wchar_t* lobbyEventNames[] = {L"none",L"xmas",L"none",L"val",L"easter",L"hallo",L"sonic",L"newyear",L"spring",L"white",L"wedding",L"fall",L"s-summer",L"s-spring",L"summer",NULL};
wchar_t* lobbyTypeNames[]  = {L"normal",L"inormal",L"ipc",L"iball",L"cave1",L"cave2u",L"dragon",L"derolle",L"volopt",L"darkfalz",L"planet",L"clouds",L"cave",L"jungle",L"forest2-2",L"forest2-1",L"windpower",L"overview",L"seaside",L"some?",L"dmorgue",L"caelum",L"digital",L"boss1",L"boss2",L"boss3",L"knight",L"sky",L"morgue",NULL};
BYTE lobbyTypeNumbers[]    = {0x00     ,0x0F      ,0x10  ,0x11    ,0xD4    ,0x67     ,0xFC     ,0xFD      ,0xFE     ,0xFF       ,0xE9     ,0xEA     ,0xED   ,0xEE     ,0xEF        ,0xF0        ,0xF1        ,0xF2       ,0xF3      ,0xF4    ,0xF5      ,0xF6     ,0xF8      ,0xF9    ,0xFA    ,0xFB    ,0xFC     ,0xFE  ,0xFF     ,0x00};
wchar_t* techNames[]       = {L"foie",L"gifoie",L"rafoie",L"barta",L"gibarta",L"rabarta",L"zonde",L"gizonde",L"razonde",L"grants",L"deband",L"jellen",L"zalure",L"shifta",L"ryuker",L"resta",L"anti",L"reverser",L"megid",NULL};
wchar_t* npcNames[]        = {L"ninja",L"rico",L"sonic",L"knuckles",L"tails",L"flowen",L"elly",NULL};

////////////////////////////////////////////////////////////////////////////////
// Message commands 

int ChatCommandLobbyInfo(SERVER* s,LOBBY* l,CLIENT* c,int argc,wchar_t** argv,wchar_t* text)
{
    wchar_t buffer[0x180];
    wchar_t levelstring[0x40];
    if (l->flags & LOBBY_FLAG_GAME)
    {
        if (l->maxlevel == 0xFFFFFFFF) swprintf(levelstring,L"Levels: %d+",l->minlevel + 1);
        else swprintf(levelstring,L"Levels: %d-%d",l->minlevel + 1,l->maxlevel + 1);

        swprintf(buffer,L"$C6Lobby ID: %08X\n%s\nSection ID: %s\nAnti-hack: %s\nCheat mode: %s",l->lobbyID,levelstring,sectionIDNames[l->sectionID],(l->flags & LOBBY_FLAG_HACKPROTECT) ? L"on" : L"off",(l->flags & LOBBY_FLAG_CHEAT) ? L"on" : L"off");
    } else {
        swprintf(buffer,L"$C6Lobby ID: %08X\nAnti-hack: %s",l->lobbyID,(l->flags & LOBBY_FLAG_HACKPROTECT) ? L"on" : L"off");
    }
    return CommandTextMessage(NULL,NULL,c,buffer);
}

int ChatCommandAX(SERVER* s,LOBBY* l,CLIENT* c,int argc,wchar_t** argv,wchar_t* text)
{
    ConsolePrintColor("$0F> %010u notes: %s\n",c->license.serialNumber,text);
    return 0;
}

int ChatCommandAnnounce(SERVER* s,LOBBY* l,CLIENT* c,int argc,wchar_t** argv,wchar_t* text) { return CommandTextMessage(s,NULL,NULL,text); }

int ChatCommandArrow(SERVER* s,LOBBY* l,CLIENT* c,int argc,wchar_t** argv,wchar_t* text)
{
    if (argc < 2) return CHAT_COMMAND_BAD_ARGUMENTS;

    swscanf(text,L"%hhd",&c->lobbyarrow);
    return CommandLobbyChangeMarker(FindLobby(s,c->lobbyID));
}

int ChatCommandInformation(SERVER* s,LOBBY* l,CLIENT* c,int argc,wchar_t** argv,wchar_t* text)
{
    char filename[MAX_PATH];
    wchar_t* buffer;
    DWORD x,bytesread,selection = 0xFFFFFFFF;
    int errors = CommandShipSelect(s,c,&informationMenu,&selection);
    if (informationMenuFileNames[selection][0] && !errors)
    {
        sprintf(filename,"system\\info\\%s",informationMenuFileNames[selection]);
        HANDLE file = CreateFile(filename,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
        if (file == INVALID_HANDLE_VALUE) CommandMessageBox(c,L"$C6An information file was not found.");
        else {
            buffer = (wchar_t*)malloc(0x804);
            if (!buffer) CommandMessageBox(c,L"$C6The server is out of memory.");
            else {
                ReadFile(file,buffer,0x800,&bytesread,NULL);
                buffer[bytesread / 2] = 0;
                if (buffer[0] == 0xFEFF) wcscpy(buffer,&buffer[1]);
                for (x = 0; buffer[x]; x++) if (buffer[x] == 0x000D) wcscpy(&buffer[x],&buffer[x + 1]);
                CommandMessageBox(c,buffer);
                free(buffer);
            }
            CloseHandle(file);
        }
        selection = 0xFFFFFFFF;
    } else return CommandLobbyMessageBox(NULL,NULL,c,L"$C7Press Enter\nto continue.");
    return errors;
}

////////////////////////////////////////////////////////////////////////////////
// Lobby commands 

int ChatCommandHack(SERVER* s,LOBBY* l,CLIENT* c,int argc,wchar_t** argv,wchar_t* text)
{
    if (l->flags & LOBBY_FLAG_HACKPROTECT) l->flags &= ~LOBBY_FLAG_HACKPROTECT;
    else l->flags |= LOBBY_FLAG_HACKPROTECT;

    wchar_t buffer[0x40];
    swprintf(buffer,L"Hack protection is %s",(l->flags & LOBBY_FLAG_HACKPROTECT) ? L"on" : L"off");
    return CommandTextMessage(NULL,l,NULL,buffer);
}

int ChatCommandCheat(SERVER* s,LOBBY* l,CLIENT* c,int argc,wchar_t** argv,wchar_t* text)
{
    if (l->flags & LOBBY_FLAG_CHEAT) l->flags &= ~LOBBY_FLAG_CHEAT;
    else l->flags |= LOBBY_FLAG_CHEAT;

    wchar_t buffer[0x40];
    swprintf(buffer,L"Cheat mode is %s",(l->flags & LOBBY_FLAG_CHEAT) ? L"on" : L"off");
    return CommandTextMessage(NULL,l,NULL,buffer);
}

int ChatCommandLobbyEvent(SERVER* s,LOBBY* l,CLIENT* c,int argc,wchar_t** argv,wchar_t* text)
{
    if (argc < 2) return CHAT_COMMAND_BAD_ARGUMENTS;

    long x;
    for (x = 0; lobbyEventNames[x]; x++) if (!wcsicmp(lobbyEventNames[x],text)) break;
    if (!lobbyEventNames[x]) swscanf(text,L"%ld",&x);
    operation_lock(l);
    l->event = x;
    operation_unlock(l);
    return SendSubcommandToLobby(l,NULL,0xDA,x,NULL,0);
}

int ChatCommandLobbyEventAll(SERVER* s,LOBBY* l,CLIENT* c,int argc,wchar_t** argv,wchar_t* text)
{
    if (argc < 2) return CHAT_COMMAND_BAD_ARGUMENTS;

    unsigned long x,y;
    for (x = 0; lobbyEventNames[x]; x++) if (!wcsicmp(lobbyEventNames[x],text)) break;
    if (!lobbyEventNames[x]) swscanf(text,L"%ld",&x);
    operation_lock(s);
    for (y = 0; y < s->numLobbies; y++)
    {
        if (s->lobbies[y]->flags & LOBBY_FLAG_GAME) continue;
        if (!(s->lobbies[y]->flags & LOBBY_FLAG_DEFAULT)) continue;
        operation_lock(s->lobbies[y]);
        s->lobbies[y]->event = x;
        SendSubcommandToLobby(s->lobbies[y],NULL,0xDA,x,NULL,0);
        operation_unlock(s->lobbies[y]);
    }
    operation_unlock(s);
    return 0;
}

int ChatCommandLobbyType(SERVER* s,LOBBY* l,CLIENT* c,int argc,wchar_t** argv,wchar_t* text)
{
    if (argc < 2) return CHAT_COMMAND_BAD_ARGUMENTS;

    unsigned long x;
    for (x = 0; lobbyTypeNames[x]; x++) if (!wcsicmp(lobbyTypeNames[x],text)) break;
    if (!lobbyTypeNames[x]) swscanf(text,L"%ld",&x);
    else x = lobbyTypeNumbers[x];
    if (l->flags & LOBBY_FLAG_EP3)
    {
        if (x < 20) x = l->block - 1;
    } else {
        if (x < 15) x = l->block - 1;
    }
    operation_lock(l);
    l->type = x;
    for (x = 0; x < l->maxClients; x++) if (l->clients[x]) CommandLobbyJoin(l,l->clients[x],false);
    operation_unlock(l);
    return 0;
}

int ChatCommandChangeLobby(SERVER* s,LOBBY* l,CLIENT* c,int argc,wchar_t** argv,wchar_t* text)
{
    if (argc < 2) return CHAT_COMMAND_BAD_ARGUMENTS;

    DWORD lobbyID;
    long errors;
    swscanf(text,L"%X",&lobbyID);
    LOBBY* next = FindLobby(s,lobbyID);
    if (!next) // not by ID 
    {
        swscanf(text,L"%d",&lobbyID);
        next = FindLobbyByBlockNumber(s,lobbyID);
    }
    if (!next) next = FindLobbyByName(s,text,false);
    if (!next) return CommandTextMessage(NULL,NULL,c,L"$C6Lobby not found");
    if (l->lobbyID == next->lobbyID) return CommandTextMessage(NULL,NULL,c,L"$C6You are currently\nin that lobby.");

    if (next->flags & LOBBY_FLAG_GAME)
    {
        errors = ProcedureCheckToJoinGame(next,c);
        if (errors) return CommandLobbyMessageBox(NULL,NULL,c,ProcedureCheckToJoinGame_ErrorString(errors));

        int rv = ProcedureChangeLobby(s,l,next,c);
        if (rv) return rv;
        c->flags |= FLAG_LOADING;
        if (c->version == VERSION_BLUEBURST) ProcedureInitializeClientItems(l,c);
    } else {
        if ((next->flags & LOBBY_FLAG_EP3) && !(c->flags & FLAG_V4_GAMES)) return CommandTextMessage(NULL,NULL,c,L"$C6That lobby is for\nEpisode 3 only.");
        return ProcedureChangeLobby(s,l,next,c);
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Game commands 

int ChatCommandLock(SERVER* s,LOBBY* l,CLIENT* c,int argc,wchar_t** argv,wchar_t* text)
{
    wchar_t message[0x30];
    if (argc == 1)
    {
        l->password[0] = 0;
        return CommandTextMessage(NULL,l,NULL,L"$C6Game unlocked");
    } else if (argc > 1)
    {
        wcscpy(l->password,text);
        swprintf(message,L"$C6Game locked; password:\n%s",l->password);
        return CommandTextMessage(NULL,l,NULL,message);
    }
    return CHAT_COMMAND_BAD_ARGUMENTS;
}

int ChatCommandUnlock(SERVER* s,LOBBY* l,CLIENT* c,int argc,wchar_t** argv,wchar_t* text)
{
    if (argc < 2) return CHAT_COMMAND_BAD_ARGUMENTS;

    DWORD lobbyID;
    swscanf(text,L"%8X",&lobbyID);
    LOBBY* l2 = FindLobby(s,lobbyID);
    if (!l2) return CommandTextMessage(NULL,l,NULL,L"$C6Lobby not found");

    l2->password[0] = 0;
    return CommandTextMessage(NULL,NULL,c,L"$C6Lobby unlocked");
}

int ChatCommandItemStatistics(SERVER* s,LOBBY* l,CLIENT* c,int argc,wchar_t** argv,wchar_t* text)
{
    unsigned int x;
    wchar_t buffer[0x100];
    swprintf(buffer,L"Inventory: %d\nBank: %d\nGround: %d",c->playerInfo.inventory.numItems,c->playerInfo.bank.numItems,l->numItems);
    CommandTextMessage(NULL,NULL,c,buffer);
    ConsolePrintColor("$0ENext item IDs: %08X %08X %08X %08X %08X\n",l->nextItemID[0],l->nextItemID[1],l->nextItemID[2],l->nextItemID[3],l->nextGameItemID);
    ConsolePrintColor("$0EYOUR INVENTORY: %d items, %d meseta\n",c->playerInfo.inventory.numItems,c->playerInfo.disp.meseta);
    for (x = 0; x < c->playerInfo.inventory.numItems; x++) ConsolePrintColor("$0E  [%d: %08X] %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X\n",x,c->playerInfo.inventory.items[x].data.itemID,c->playerInfo.inventory.items[x].data.itemData1[0],c->playerInfo.inventory.items[x].data.itemData1[1],c->playerInfo.inventory.items[x].data.itemData1[2],c->playerInfo.inventory.items[x].data.itemData1[3],c->playerInfo.inventory.items[x].data.itemData1[4],c->playerInfo.inventory.items[x].data.itemData1[5],c->playerInfo.inventory.items[x].data.itemData1[6],c->playerInfo.inventory.items[x].data.itemData1[7],c->playerInfo.inventory.items[x].data.itemData1[8],c->playerInfo.inventory.items[x].data.itemData1[9],c->playerInfo.inventory.items[x].data.itemData1[10],c->playerInfo.inventory.items[x].data.itemData1[11],c->playerInfo.inventory.items[x].data.itemData2[0],c->playerInfo.inventory.items[x].data.itemData2[1],c->playerInfo.inventory.items[x].data.itemData2[2],c->playerInfo.inventory.items[x].data.itemData2[3]);
    ConsolePrintColor("$0EYOUR BANK: %d items, %d meseta\n",c->playerInfo.bank.numItems,c->playerInfo.bank.meseta);
    for (x = 0; x < c->playerInfo.bank.numItems; x++) ConsolePrintColor("$0E  [%d: %08X] %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %d\n",x,c->playerInfo.bank.items[x].data.itemID,c->playerInfo.bank.items[x].data.itemData1[0],c->playerInfo.bank.items[x].data.itemData1[1],c->playerInfo.bank.items[x].data.itemData1[2],c->playerInfo.bank.items[x].data.itemData1[3],c->playerInfo.bank.items[x].data.itemData1[4],c->playerInfo.bank.items[x].data.itemData1[5],c->playerInfo.bank.items[x].data.itemData1[6],c->playerInfo.bank.items[x].data.itemData1[7],c->playerInfo.bank.items[x].data.itemData1[8],c->playerInfo.bank.items[x].data.itemData1[9],c->playerInfo.bank.items[x].data.itemData1[10],c->playerInfo.bank.items[x].data.itemData1[11],c->playerInfo.bank.items[x].data.itemData2[0],c->playerInfo.bank.items[x].data.itemData2[1],c->playerInfo.bank.items[x].data.itemData2[2],c->playerInfo.bank.items[x].data.itemData2[3],c->playerInfo.bank.items[x].amount);
    ConsolePrintColor("$0ETHE FLOOR: %d items\n",l->numItems);
    for (x = 0; x < l->numItems; x++) ConsolePrintColor("$0E  [%d: %08X] %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X\n",x,l->items[x].data.itemID,l->items[x].data.itemData1[0],l->items[x].data.itemData1[1],l->items[x].data.itemData1[2],l->items[x].data.itemData1[3],l->items[x].data.itemData1[4],l->items[x].data.itemData1[5],l->items[x].data.itemData1[6],l->items[x].data.itemData1[7],l->items[x].data.itemData1[8],l->items[x].data.itemData1[9],l->items[x].data.itemData1[10],l->items[x].data.itemData1[11],l->items[x].data.itemData2[0],l->items[x].data.itemData2[1],l->items[x].data.itemData2[2],l->items[x].data.itemData2[3]);
    return 0;
}

int ChatCommandQuest(SERVER* s,LOBBY* l,CLIENT* c,int argc,wchar_t** argv,wchar_t* text)
{
    if (argc < 2) return CHAT_COMMAND_BAD_ARGUMENTS;

    QUEST* q = FindQuestPName(quests,text);
    if (!q) return CommandTextMessage(NULL,NULL,c,L"$C6Quest not found");

    if (!(q->versionFlags & (1 << l->version))) return CommandTextMessage(NULL,NULL,c,L"$C6Quest is not\navailable for this\nversion of PSO");
    if (l->flags & LOBBY_FLAG_HACKPROTECT)
    {
        if (q->episode != l->episode) return CommandTextMessage(NULL,NULL,c,L"$C6Quest is for a\ndifferent episode");
        if (q->mode != l->mode) return CommandTextMessage(NULL,NULL,c,L"$C6Quest is for a\ndifferent game mode");
    }

    unsigned long x;
    operation_lock(l);
    for (x = 0; x < l->maxClients; x++)
    {
        if (!l->clients[x]) continue;
        if (l->clients[x]->area) break;
    }
    operation_unlock(l);
    if (x < l->maxClients) return CommandTextMessage(NULL,NULL,c,L"$C6All players must be\non Pioneer 2 to\nbegin a quest.");

    operation_lock(l);
    if (q->category & QUEST_CATEGORY_JOINABLE) l->flags |= LOBBY_FLAG_OPEN_QUESTING;
    else l->flags |= LOBBY_FLAG_QUESTING;
    l->questLoading = q->questID;
    for (x = 0; x < l->maxClients; x++)
    {
        if (!l->clients[x]) continue;
        operation_lock(l->clients[x]);
        l->clients[x]->questLoading = q->questID;
        l->clients[x]->flags |= FLAG_LOADING;
        operation_unlock(l->clients[x]);
    }
    operation_unlock(l);
    return SendSubcommandToLobby(l,NULL,0x1D,0x00,NULL,0);
}

int ChatCommandMinLevel(SERVER* s,LOBBY* l,CLIENT* c,int argc,wchar_t** argv,wchar_t* text)
{
    if (argc < 2) return CHAT_COMMAND_BAD_ARGUMENTS;

    operation_lock(l);
    swscanf(text,L"%ld",&l->minlevel);
    operation_unlock(l);

    wchar_t message[0x30];
    swprintf(message,L"$C6Minimum level set to\n%d",l->minlevel);
    l->minlevel--;
    return CommandTextMessage(NULL,l,NULL,message);
}

int ChatCommandMaxLevel(SERVER* s,LOBBY* l,CLIENT* c,int argc,wchar_t** argv,wchar_t* text)
{
    if (argc < 2) return CHAT_COMMAND_BAD_ARGUMENTS;

    operation_lock(l);
    swscanf(text,L"%ld",&l->maxlevel);
    operation_unlock(l);

    wchar_t message[0x30];
    swprintf(message,L"$C6Maximum level set to\n%d",l->maxlevel);
    l->maxlevel--;
    return CommandTextMessage(NULL,l,NULL,message);
}

////////////////////////////////////////////////////////////////////////////////
// Character commands 

int ChatCommandEdit(SERVER* s,LOBBY* l,CLIENT* c,int argc,wchar_t** argv,wchar_t* text)
{
    if (argc < 3) return CHAT_COMMAND_BAD_ARGUMENTS;
    BB_PLAYER_DISPDATA olddisp;

    long x,y;
    memcpy(&olddisp,&c->playerInfo.disp,sizeof(BB_PLAYER_DISPDATA));
    if (!wcsicmp(argv[1],L"atp"))    swscanf(argv[2],L"%hd",&c->playerInfo.disp.stats.atp);
    if (!wcsicmp(argv[1],L"mst"))    swscanf(argv[2],L"%hd",&c->playerInfo.disp.stats.mst);
    if (!wcsicmp(argv[1],L"evp"))    swscanf(argv[2],L"%hd",&c->playerInfo.disp.stats.evp);
    if (!wcsicmp(argv[1],L"hp"))     swscanf(argv[2],L"%hd",&c->playerInfo.disp.stats.hp);
    if (!wcsicmp(argv[1],L"dfp"))    swscanf(argv[2],L"%hd",&c->playerInfo.disp.stats.dfp);
    if (!wcsicmp(argv[1],L"ata"))    swscanf(argv[2],L"%hd",&c->playerInfo.disp.stats.ata);
    if (!wcsicmp(argv[1],L"lck"))    swscanf(argv[2],L"%hd",&c->playerInfo.disp.stats.lck);
    if (!wcsicmp(argv[1],L"meseta")) swscanf(argv[2],L"%ld",&c->playerInfo.disp.meseta);
    if (!wcsicmp(argv[1],L"exp"))    swscanf(argv[2],L"%ld",&c->playerInfo.disp.exp);
    if (!wcsicmp(argv[1],L"level"))
    {
        swscanf(argv[2],L"%ld",&c->playerInfo.disp.level);
        c->playerInfo.disp.level--;
    }
    if (!wcsicmp(argv[1],L"namecolor")) swscanf(argv[2],L"%8X",&c->playerInfo.disp.nameColor);
    if (!wcsicmp(argv[1],L"secid")) for (x = 0; sectionIDNames[x]; x++) if (!wcsicmp(argv[2],sectionIDNames[x])) c->playerInfo.disp.sectionID = x;
    if (!wcsicmp(argv[1],L"name"))
    {
        wcsncpy(c->playerInfo.disp.playername,argv[2],10);
        tx_add_language_marker(c->playerInfo.disp.playername,L'J');
    }
    if (!wcsicmp(argv[1],L"npc"))
    {
        for (x = 0; npcNames[x]; x++) if (!wcsicmp(argv[2],npcNames[x])) c->playerInfo.disp.extraModel = x;
        if (!wcsicmp(argv[2],L"none")) c->playerInfo.disp.v2flags &= 0xFD;
        else c->playerInfo.disp.v2flags |= 0x02;
    }
    if (!wcsicmp(argv[1],L"tech") && (argc > 3))
    {
        swscanf(argv[3],L"%ld",&y);
        if (!wcsicmp(argv[2],L"all"))
        {
            for (x = 0; x < 0x14; x++) c->playerInfo.disp.techLevels[x] = y - 1;
        } else {
            for (x = 0; techNames[x]; x++) if (!wcsicmp(argv[2],techNames[x])) break;
            if (techNames[x]) c->playerInfo.disp.techLevels[x] = y - 1;
            else return 8493454;
        }
    }
    if (memcmp(&olddisp,&c->playerInfo.disp,sizeof(BB_PLAYER_DISPDATA)))
    {
        CommandLobbyDeletePlayer(l,c->clientID);
        CommandBBSendPlayerInfo(c);
        CommandLobbyJoin(l,c,true);
    }
    return 0;
}

int ChatCommandChangeChar(SERVER* s,LOBBY* l,CLIENT* c,int argc,wchar_t** argv,wchar_t* text)
{
    if (argc < 2) return CHAT_COMMAND_BAD_ARGUMENTS;

    DWORD nextPlayerID;
    char filename[MAX_PATH],filename2[MAX_PATH],bankname[MAX_PATH];
    swscanf(argv[1],L"%ld",&nextPlayerID);
    nextPlayerID--;
    if (nextPlayerID > 3) return CommandTextMessage(NULL,NULL,c,L"$C6Player slots range\nfrom 1 to 4.");

    sprintf(filename,"system\\players\\player_%s_%d.nsc",c->license.username,c->cfg.bbplayernum + 1);
    sprintf(bankname,"system\\players\\bank_%s_%s.nsb",c->license.username,c->playerInfo.bankname);
    if (!PlayerSavePlayerData(&c->playerInfo,filename)) return CommandTextMessage(NULL,NULL,c,L"$C6Your player data could\nnot be saved.");
    if (!PlayerSaveBankData(&c->playerInfo.bank,bankname)) return CommandTextMessage(NULL,NULL,c,L"$C6Your bank data could\nnot be saved.");

    sprintf(c->playerInfo.bankname,"player%ld",nextPlayerID + 1);
    sprintf(filename,"system\\players\\player_%s_%ld.nsc",c->license.username,nextPlayerID + 1);
    sprintf(filename2,"system\\players\\%s-player-%ld.pbb",c->license.username,nextPlayerID);
    sprintf(bankname,"system\\players\\bank_%s_%s.nsb",c->license.username,c->playerInfo.bankname);
    if (!PlayerLoadPlayerData(&c->playerInfo,filename))
    {
        if (!PlayerLoadPlayerDataOldFormat(&c->playerInfo,filename2)) return CommandTextMessage(NULL,NULL,c,L"$C6Your player data could\nnot be loaded.");
    }
    if (!PlayerLoadBankData(&c->playerInfo.bank,bankname))
    {
        if (!PlayerLoadBankData(&c->playerInfo.bank,"system\\blueburst\\default.nsb")) CommandTextMessage(NULL,NULL,c,L"$C6Warning: bank data could\nnot be loaded.");
    }

    c->cfg.bbplayernum = nextPlayerID;
    int errors = CommandLobbyDeletePlayer(l,c->clientID);
    if (errors) return errors;
    errors = CommandBBSendPlayerInfo(c);
    if (errors) return errors;
    return CommandLobbyJoin(l,c,true);
}

int ChatCommandChangeBank(SERVER* s,LOBBY* l,CLIENT* c,int argc,wchar_t** argv,wchar_t* text)
{
    if (argc < 2) return CHAT_COMMAND_BAD_ARGUMENTS;

    char bankname[MAX_PATH];
    wchar_t buffer[0x100];

    sprintf(bankname,"system\\players\\bank_%s_%s.nsb",c->license.username,c->playerInfo.bankname);
    if (!PlayerSaveBankData(&c->playerInfo.bank,bankname)) return CommandTextMessage(NULL,NULL,c,L"$C6Your bank data could\nnot be saved.");

    sprintf(bankname,"system\\players\\bank_%s_%S.nsb",c->license.username,text);
    if (!PlayerLoadBankData(&c->playerInfo.bank,bankname)) return CommandTextMessage(NULL,NULL,c,L"$C6Bank not found");

    tx_convert_to_sjis(c->playerInfo.bankname,text);
    swprintf(buffer,L"$C6You are using bank:\n%s",text);
    return CommandTextMessage(NULL,NULL,c,buffer);
}

int ChatCommandBBChar(SERVER* s,LOBBY* l,CLIENT* c,int argc,wchar_t** argv,wchar_t* text)
{
    if (argc < 4) return CHAT_COMMAND_BAD_ARGUMENTS;

    LICENSE li;
    tx_convert_to_sjis(li.username,argv[1]);
    tx_convert_to_sjis(li.password,argv[2]);

    DWORD nextPlayerID;
    char filename[MAX_PATH];
    swscanf(argv[3],L"%ld",&nextPlayerID);
    nextPlayerID--;
    if (nextPlayerID > 3) return CommandTextMessage(NULL,NULL,c,L"$C6Player slots range\nfrom 1 to 4.");

    if (VerifyLicense(licenses,&li,LICENSE_VERIFY_BLUEBURST | LICENSE_VERIFY_CHECK_PASSWORD) == LICENSE_RESULT_OK)
    {
        CommandSimple(c,0x0095,0x00000000); // this is GetCharacterInfo(), in effect 
        ProcessCommands(s,c,0x0061,0);
        sprintf(filename,"system\\players\\player_%s_%ld.nsc",li.username,nextPlayerID + 1);
        if (!PlayerSavePlayerData(&c->playerInfo,filename)) return CommandTextMessage(NULL,NULL,c,L"$C6Your player data could\nnot be saved.");
        sprintf(filename,"system\\players\\bank_%s_player%ld.nsb",li.username,nextPlayerID + 1);
        if (!PlayerSaveBankData(&c->playerInfo.bank,filename)) return CommandTextMessage(NULL,NULL,c,L"$C6Your bank data could\nnot be saved.");
    } else return CommandTextMessage(NULL,NULL,c,L"$C6License verification\nfailed.");
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Administration commands 

int ChatCommandSTFU(SERVER* s,LOBBY* l,CLIENT* c,int argc,wchar_t** argv,wchar_t* text)
{
    if (argc < 2) return CHAT_COMMAND_BAD_ARGUMENTS;

    wchar_t buffer[0x100];
    DWORD serial;
    swscanf(text,L"%ld",&serial);
    CLIENT* target = FindClient(s,serial);
    if (target)
    {
        if ((target->license.privileges & PRIVILEGE_MODERATE) && !(c->license.privileges & PRIVILEGE_HIGHER_LEVEL)) return CommandTextMessage(NULL,NULL,c,L"$C6You do not have\nsufficient privileges.");
        target->stfu = !target->stfu;
        if (target->stfu) swprintf(buffer,L"$C6%s STFUed",target->playerInfo.disp.playername);
        else swprintf(buffer,L"$C6%s unSTFUed",target->playerInfo.disp.playername);
        return CommandTextMessage(NULL,l,NULL,buffer);
    }
    return CommandTextMessage(NULL,NULL,c,L"$C6Target not found");
}

int ChatCommandKick(SERVER* s,LOBBY* l,CLIENT* c,int argc,wchar_t** argv,wchar_t* text)
{
    if (argc < 2) return CHAT_COMMAND_BAD_ARGUMENTS;

    wchar_t buffer[0x100];
    DWORD serial;
    swscanf(text,L"%ld",&serial);
    CLIENT* target = FindClient(s,serial);
    if (target)
    {
        if ((target->license.privileges & PRIVILEGE_MODERATE) && !(c->license.privileges & PRIVILEGE_HIGHER_LEVEL)) return CommandTextMessage(NULL,NULL,c,L"$C6You do not have\nsufficient privileges.");
        CommandMessageBox(target,L"$C6You were kicked by a moderator.");
        target->disconnect = true;
        swprintf(buffer,L"$C6%s kicked",target->playerInfo.disp.playername);
        return CommandTextMessage(NULL,l,NULL,buffer);
    }
    return CommandTextMessage(NULL,NULL,c,L"$C6Target not found");
}

int ChatCommandBan(SERVER* s,LOBBY* l,CLIENT* c,int argc,wchar_t** argv,wchar_t* text)
{
    if (argc < 4) return CHAT_COMMAND_BAD_ARGUMENTS;

    wchar_t buffer[0x100];
    DWORD serial,seconds;
    swscanf(argv[1],L"%ld",&serial);
    CLIENT* target = FindClient(s,serial);
    if (target)
    {
        if ((target->license.privileges & PRIVILEGE_MODERATE) && !(c->license.privileges & PRIVILEGE_HIGHER_LEVEL)) return CommandTextMessage(NULL,NULL,c,L"$C6You do not have\nsufficient privileges.");
        swscanf(argv[2],L"%ld",&seconds);
        if (argv[3][0] == 'm') seconds *= 60;
        if (argv[3][0] == 'h') seconds *= 3600;
        if (argv[3][0] == 'd') seconds *= 86400;
        if (argv[3][0] == 'w') seconds *= 604800;
        SetUserBan(licenses,&target->license,LICENSE_VERIFY_BLUEBURST,seconds);
        target->disconnect = true;
        CommandMessageBox(target,L"$C6You were banned by a moderator.");
        swprintf(buffer,L"$C6%s banned",target->playerInfo.disp.playername);
        return CommandTextMessage(NULL,l,NULL,buffer);
    }
    return CommandTextMessage(NULL,NULL,c,L"$C6Target not found");
}

////////////////////////////////////////////////////////////////////////////////
// Cheat commands 

int ChatCommandWarp(SERVER* s,LOBBY* l,CLIENT* c,int argc,wchar_t** argv,wchar_t* text)
{
    if (argc < 2) return CHAT_COMMAND_BAD_ARGUMENTS;

    DWORD area;
    swscanf(text,L"%ld",&area);
    if (!l->episode) return 5783;
    if ((l->episode == 1) && (area > 17)) return CommandTextMessage(NULL,NULL,c,L"$C6Area numbers must be\n17 or less.");
    if ((l->episode == 2) && (area > 17)) return CommandTextMessage(NULL,NULL,c,L"$C6Area numbers must be\n17 or less.");
    if ((l->episode == 3) && (area > 10)) return CommandTextMessage(NULL,NULL,c,L"$C6Area numbers must be\n10 or less.");
    if (l->episode > 3) return 5784;
    if (c->area == area) return 0;
    return CommandPlayerWarp(c,area);
}

int ChatCommandMove(SERVER* s,LOBBY* l,CLIENT* c,int argc,wchar_t** argv,wchar_t* text)
{
    if (argc < 2) return CHAT_COMMAND_BAD_ARGUMENTS;

    DWORD area;
    swscanf(text,L"%ld",&area);
    if (!l->episode) return 5783;
    if ((l->episode == 1) && (area > 17)) return CommandTextMessage(NULL,NULL,c,L"$C6Area numbers must be\n17 or less.");
    if ((l->episode == 2) && (area > 17)) return CommandTextMessage(NULL,NULL,c,L"$C6Area numbers must be\n17 or less.");
    if ((l->episode == 3) && (area > 10)) return CommandTextMessage(NULL,NULL,c,L"$C6Area numbers must be\n10 or less.");
    if (l->episode > 3) return 5784;

    unsigned long x;
    operation_lock(l);
    for (x = 0; x < l->maxClients; x++)
    {
        if (!l->clients[x]) continue;
        if (l->clients[x]->area == area) continue;
        CommandPlayerWarp(l->clients[x],area);
    }
    operation_unlock(l);
    return 0;
}

int ChatCommandInfiniteHP(SERVER* s,LOBBY* l,CLIENT* c,int argc,wchar_t** argv,wchar_t* text)
{
    c->infhp = !c->infhp;
    if (c->infhp) return CommandTextMessage(NULL,NULL,c,L"$C6Infinite HP enabled");
    return CommandTextMessage(NULL,NULL,c,L"$C6Infinite HP disabled");
}

int ChatCommandInfiniteTP(SERVER* s,LOBBY* l,CLIENT* c,int argc,wchar_t** argv,wchar_t* text)
{
    c->inftp = !c->inftp;
    if (c->inftp) return CommandTextMessage(NULL,NULL,c,L"$C6Infinite TP enabled");
    return CommandTextMessage(NULL,NULL,c,L"$C6Infinite TP disabled");
}

int ChatCommandItem(SERVER* s,LOBBY* l,CLIENT* c,int argc,wchar_t** argv,wchar_t* text)
{
    if (argc < 2) return CHAT_COMMAND_BAD_ARGUMENTS;

    int rv = 0;
    int size = wcslen(text);
    if (size < 0x10) size = 0x10;
    void* data = malloc(size);
    if (!data) return 5;
    memset(data,0,size);
    int datasize = tx_read_string_data(text,size,data,size);
    if (datasize > 0x10) rv = CommandTextMessage(NULL,NULL,c,L"$C6Item codes must be\n16 bytes or less.");
    else if (datasize < 2) rv = CommandTextMessage(NULL,NULL,c,L"$C6Item codes must be\n2 bytes or more.");
    else {
        memcpy(&l->item.data.itemData1,data,12);
        memcpy(&l->item.data.itemData2,(void*)((DWORD)data + 12),4);
    }
    free(data);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////

// Each chat command has a name, function, flags, required privileges, and a usage string.
// The usage string is displayed if the command is used improperly.
// The required privileges are only enforced if the flags include COMMAND_REQUIRE_PRIVILEGE_MASK or COMMAND_REQUIRE_LEADER_GAME.
// The rest should be self-explanatory. For more information, see the flag definitions in chatcommands.h.
ChatCommandEntry ChatCommands[] = {
    {L"allevent"  ,ChatCommandLobbyEventAll ,COMMAND_REQUIRE_LOBBY | COMMAND_REQUIRE_PRIVILEGE_MASK,PRIVILEGE_CHANGE_EVENT     ,L"usage:\nallevent <name/ID>"},
    {L"ann"       ,ChatCommandAnnounce      ,COMMAND_REQUIRE_PRIVILEGE_MASK,PRIVILEGE_ANNOUNCE                                 ,L"usage:\nann <message>"},
    {L"arrow"     ,ChatCommandArrow         ,COMMAND_REQUIRE_LOBBY,0                                                           ,L"usage:\narrow <color>"},
    {L"ax"        ,ChatCommandAX            ,COMMAND_REQUIRE_PRIVILEGE_MASK,PRIVILEGE_ANNOUNCE                                 ,L"usage:\nax <message>"},
    {L"ban"       ,ChatCommandBan           ,COMMAND_REQUIRE_PRIVILEGE_MASK,PRIVILEGE_BAN_USER                                 ,L"usage:\nban <serialNumber>"},
    //{L"banip"     ,ChatCommandBanIP         ,COMMAND_REQUIRE_PRIVILEGE_MASK,PRIVILEGE_BAN_USER                                 ,L"usage:\nipban <serialNumber>"},
    {L"bbchar"    ,ChatCommandBBChar        ,COMMAND_REQUIRE_LOBBY | COMMAND_REQUIRE_NOT_BLUEBURST,0                           ,L"usage:\nbbchar <user> <pass> <1-4>"},
    {L"changebank",ChatCommandChangeBank    ,COMMAND_REQUIRE_VERSION_BLUEBURST,0                                               ,L"usage:\nchangebank <bank name>"},
    {L"changechar",ChatCommandChangeChar    ,COMMAND_REQUIRE_LOBBY | COMMAND_REQUIRE_VERSION_BLUEBURST,0                       ,L"usage:\nchangechar <1-4>"},
    {L"cheat"     ,ChatCommandCheat         ,COMMAND_REQUIRE_GAME | COMMAND_REQUIRE_LEADER,0                                   ,L"usage:\nduh"},
    {L"edit"      ,ChatCommandEdit          ,COMMAND_REQUIRE_LOBBY | COMMAND_REQUIRE_VERSION_BLUEBURST,0                       ,L"usage:\nedit <stat> <value>"},
    {L"event"     ,ChatCommandLobbyEvent    ,COMMAND_REQUIRE_LOBBY | COMMAND_REQUIRE_PRIVILEGE_MASK,PRIVILEGE_CHANGE_EVENT     ,L"usage:\nevent <name/ID>"},
    {L"hack"      ,ChatCommandHack          ,COMMAND_REQUIRE_LEADER_GAME,PRIVILEGE_CHANGE_LOBBY_INFO                           ,L"usage:\nduh"},
    {L"infhp"     ,ChatCommandInfiniteHP    ,COMMAND_REQUIRE_GAME | COMMAND_REQUIRE_CHEAT_MODE,0                               ,L"usage:\nduh"},
    {L"info"      ,ChatCommandInformation   ,0,0                                                                               ,L"usage:\nduh"},
    {L"inftp"     ,ChatCommandInfiniteTP    ,COMMAND_REQUIRE_GAME | COMMAND_REQUIRE_CHEAT_MODE,0                               ,L"usage:\nduh"},
    {L"item"      ,ChatCommandItem          ,COMMAND_REQUIRE_GAME | COMMAND_REQUIRE_CHEAT_MODE | COMMAND_REQUIRE_LEADER,0      ,L"usage:\nitem <item code>"},
    {L"istat"     ,ChatCommandItemStatistics,COMMAND_REQUIRE_GAME,0                                                            ,L"usage:\nduh"},
    {L"kick"      ,ChatCommandKick          ,COMMAND_REQUIRE_PRIVILEGE_MASK,PRIVILEGE_KICK_USER                                ,L"usage:\nkick <serialNumber>"},
    {L"li"        ,ChatCommandLobbyInfo     ,0,0                                                                               ,L"usage:\nduh."},
    {L"lobby"     ,ChatCommandChangeLobby   ,COMMAND_REQUIRE_LOBBY,0                                                           ,L"usage:\nlobby <name/number>"},
    {L"lock"      ,ChatCommandLock          ,COMMAND_REQUIRE_GAME | COMMAND_REQUIRE_LEADER,0                                   ,L"usage:\nlock [password]\nomit password to\nunlock game"},
    {L"maxlevel"  ,ChatCommandMaxLevel      ,COMMAND_REQUIRE_GAME | COMMAND_REQUIRE_LEADER,0                                   ,L"usage:\nmaxlevel <level>"},
    {L"minlevel"  ,ChatCommandMinLevel      ,COMMAND_REQUIRE_GAME | COMMAND_REQUIRE_LEADER,0                                   ,L"usage:\nminlevel <level>"},
    {L"move"      ,ChatCommandMove          ,COMMAND_REQUIRE_GAME | COMMAND_REQUIRE_CHEAT_MODE | COMMAND_REQUIRE_LEADER,0      ,L"usage:\nmove <area>"},
    {L"quest"     ,ChatCommandQuest         ,COMMAND_REQUIRE_GAME | COMMAND_REQUIRE_LEADER,0                                   ,L"usage:\nquest <partialName>"},
    {L"stfu"      ,ChatCommandSTFU          ,COMMAND_REQUIRE_PRIVILEGE_MASK,PRIVILEGE_STFU_USER                                ,L"usage:\nstfu <serialNumber>"},
    {L"type"      ,ChatCommandLobbyType     ,COMMAND_REQUIRE_LOBBY | COMMAND_REQUIRE_PRIVILEGE_MASK,PRIVILEGE_CHANGE_LOBBY_INFO,L"usage:\ntype <name/ID>"},
    {L"unlock"    ,ChatCommandUnlock        ,COMMAND_REQUIRE_LOBBY | COMMAND_REQUIRE_PRIVILEGE_MASK,PRIVILEGE_UNLOCK_GAMES     ,L"usage:\nunlock <gameID>"},
    {L"warp"      ,ChatCommandWarp          ,COMMAND_REQUIRE_GAME | COMMAND_REQUIRE_CHEAT_MODE,0                               ,L"usage:\nwarp <area>"},
    {NULL         ,NULL                     ,0,0,                                                                              NULL}
};

// this function is called every time any player sends a chat beginning with a dollar sign.
// It is this function's responsibility to see if the chat is a command, and to
// execute the command and block the chat if it is.
int ProcessChatCommand(SERVER* s,CLIENT* c,wchar_t* text)
{
    unsigned int x,y;
    int argc;
    wchar_t** argv;
    wchar_t buffer[0x100];

    // disallow slashes/backslashes in commands
    for (x = 0; x < wcslen(text); x++) if ((text[x] == '/') || (text[x] == '\\')) return CommandTextMessage(NULL,NULL,c,L"$C6Chat commands cannot\ncontain backslashes.");

    // match this message with a command
    for (y = 0; ChatCommands[y].command; y++) if (!_wcsnicmp(ChatCommands[y].command,text,wcslen(ChatCommands[y].command))) break;

    // if it matches a command, verify conditions
    if (ChatCommands[y].command)
    {
        // check version
        if ((c->version == VERSION_GAMECUBE) && (ChatCommands[y].requirementFlags & COMMAND_REQUIRE_NOT_GAMECUBE)) return CommandTextMessage(NULL,NULL,c,L"$C6This command is for\na different version\nof PSO.");
        if ((c->version == VERSION_BLUEBURST) && (ChatCommands[y].requirementFlags & COMMAND_REQUIRE_NOT_BLUEBURST)) return CommandTextMessage(NULL,NULL,c,L"$C6This command is for\na different version\nof PSO.");
        if ((c->version == VERSION_PC) && (ChatCommands[y].requirementFlags & COMMAND_REQUIRE_NOT_PC)) return CommandTextMessage(NULL,NULL,c,L"$C6This command is for\na different version\nof PSO.");
        if ((c->version == VERSION_DC) && (ChatCommands[y].requirementFlags & COMMAND_REQUIRE_NOT_DC)) return CommandTextMessage(NULL,NULL,c,L"$C6This command is for\na different version\nof PSO.");

        // check lobby/game mode and leader
        LOBBY* l = FindLobby(s,c->lobbyID);
        if (!l) return 84727953;
        if ((l->flags & LOBBY_FLAG_GAME) && (ChatCommands[y].requirementFlags & COMMAND_REQUIRE_LOBBY)) return CommandTextMessage(NULL,NULL,c,L"$C6You must be in a\nlobby to use this\ncommand.");
        if (!(l->flags & LOBBY_FLAG_GAME) && (ChatCommands[y].requirementFlags & COMMAND_REQUIRE_GAME)) return CommandTextMessage(NULL,NULL,c,L"$C6You must be in a\ngame to use this\ncommand.");
        if ((l->leaderID != c->clientID) && (ChatCommands[y].requirementFlags & COMMAND_REQUIRE_LEADER)) return CommandTextMessage(NULL,NULL,c,L"$C6You must be the\ngame leader to use\nthis command.");

        // check priileges and cheat mode
        if (((c->license.privileges & ChatCommands[y].requiredPrivilege) != ChatCommands[y].requiredPrivilege) && (ChatCommands[y].requirementFlags & COMMAND_REQUIRE_PRIVILEGE_MASK))  return CommandTextMessage(NULL,NULL,c,L"$C6You do not have\npermission to use\nthis command.");
        if (!(l->flags & LOBBY_FLAG_CHEAT) && (ChatCommands[y].requirementFlags & COMMAND_REQUIRE_CHEAT_MODE)) return CommandTextMessage(NULL,NULL,c,L"$C6Cheats must be enabled\nfor you to use this\ncommand.");
        if (ChatCommands[y].requirementFlags & COMMAND_REQUIRE_LEADER_GAME)
        {
            if (l->flags & LOBBY_FLAG_DEFAULT)
            {
                if ((c->license.privileges & ChatCommands[y].requiredPrivilege) != ChatCommands[y].requiredPrivilege)  return CommandTextMessage(NULL,NULL,c,L"$C6You do not have\npermission to use\nthis command.");
            } else {
                if (l->leaderID != c->clientID) return CommandTextMessage(NULL,NULL,c,L"$C6You must be the\nlobby leader to use\nthis command.");
            }
        }

        // else, prepare an argument list and call the chat command function
        argv = CommandLineToArgvW(text,&argc);
        for (x = 0; text[x] && (text[x] != L' '); x++);
        for (; text[x] && (text[x] == L' '); x++);
        int rv = ChatCommands[y].function(s,l,c,argc,argv,&text[x]);
        GlobalFree(argv);

        // the function returned 'bad arguments'? then show the usage message
        if (rv == CHAT_COMMAND_BAD_ARGUMENTS) return CommandTextMessage(NULL,NULL,c,ChatCommands[y].helpMessage);

        // a different error occurred? give the error code
        if (rv)
        {
            swprintf(buffer,L"$C6Failed with error code:\n%u",rv);
            CommandTextMessage(NULL,NULL,c,buffer);
        }
    }
    return 0;
}

