#include <windows.h>
#include <stdio.h>

#include "text.h"
#include "operation.h"
#include "console.h"

#include "encryption.h"
#include "netconfig.h"

#include "player.h"
#include "quest.h"
#include "battleparamentry.h"
#include "itemraretable.h"
#include "levels.h"
#include "map.h"

#include "license.h"
#include "version.h"
#include "privilege.h"
#include "client.h"

#include "listenthread.h"
#include "lobby.h"
#include "server.h"

#include "chatcommands.h"

#include "command-functions.h"
#include "command-input-structures.h"

// Many of these functions are dupicated for each version of PSO. I will not
// comment most of them, since it's probably fairly obvious what they do. Each
// function is called according to the client's version and the ID of the command received.

extern CFGFile* config;
extern LICENSE_LIST* licenses;
extern QUESTLIST* quests;
extern LEVEL_TABLE* leveltable;
extern BATTLE_PARAM battleParamTable[2][3][4][0x60];
extern OPERATION_LOCK sendCommandConsoleLock;
extern char* loginServerPortNames[];

extern wchar_t* sectionIDNames[];

////////////////////////////////////////////////////////////////////////////////
// Login commands 

int ProcessCommand_Login_DoLicenseCheck(CLIENT* c,DWORD what)
{
    int rs = VerifyLicense(licenses,&c->license,what);
    if ((rs == LICENSE_RESULT_NOTFOUND) && !CFGIsValuePresent(config,"Allow_Unregistered_Users"))
    {
        CommandMessageBox(c,L"$C6You are not registered on this server.");
        c->disconnect = true;
        return 5;
    }
    if (rs == LICENSE_RESULT_INVALID_CALL)
    {
        CommandMessageBox(c,L"$C6Internal license system error!");
        c->disconnect = true;
        return 5;
    }
    if (rs == LICENSE_RESULT_WRONGPASS)
    {
        CommandMessageBox(c,L"$C6Incorrect password.");
        c->disconnect = true;
        return 5;
    }
    if (rs == LICENSE_RESULT_BANNED)
    {
        CommandMessageBox(c,L"$C6Your license has been banned.");
        c->disconnect = true;
        return 5;
    }
    return 0;
}

int ProcessCommand_GC_VerifyLicense(SERVER* s,CLIENT* c,COMMAND_GC_LICENSEVERIFY* p) // DB
{
    if (p->header.size != 0xE0) return 13;
    sscanf(p->serialNumber,"%8lX",&c->license.serialNumber);
    strncpy(c->license.accessKey,p->accessKey,12);
    strncpy(c->license.password2,p->password,8);
    c->flags |= GetClientSubversionFlags(c->version,p->subversion);
    int rs = ProcessCommand_Login_DoLicenseCheck(c,LICENSE_VERIFY_GAMECUBE | LICENSE_VERIFY_CHECK_PASSWORD);
    if (rs) return rs;
    return CommandSimple(c,0x009A,0x00000001);
}

int ProcessCommand_PC_Login_A(SERVER* s,CLIENT* c,COMMAND_PC_LOGIN_A* p) // 9A
{
    sscanf(p->serialNumber,"%8lX",&c->license.serialNumber);
    strncpy(c->license.accessKey,p->accessKey,12);
    int rs = ProcessCommand_Login_DoLicenseCheck(c,((c->version == VERSION_GAMECUBE) ? LICENSE_VERIFY_GAMECUBE : LICENSE_VERIFY_PC));
    if (rs) return rs;
    return CommandSimple(c,0x009C,0x00000001);
}

int ProcessCommand_PCGC_Login_C(SERVER* s,CLIENT* c,COMMAND_PCGC_LOGIN_C* p) // 9C
{
    sscanf(p->serialNumber,"%8lX",&c->license.serialNumber);
    strncpy(c->license.accessKey,p->accessKey,12);
    strncpy(c->license.password2,p->password,8);
    c->flags |= GetClientSubversionFlags(c->version,p->subversion);
    int rs = ProcessCommand_Login_DoLicenseCheck(c,((c->version == VERSION_GAMECUBE) ? LICENSE_VERIFY_GAMECUBE : LICENSE_VERIFY_PC) | LICENSE_VERIFY_CHECK_PASSWORD);
    if (rs) return rs;
    return CommandSimple(c,0x009C,0x00000001);
}

int ProcessCommand_PCGC_Login_E(SERVER* s,CLIENT* c,COMMAND_PCGC_LOGIN_E* p) // 9D/9E
{
    sscanf(p->serialNumber,"%8lX",&c->license.serialNumber);
    strncpy(c->license.accessKey,p->accessKey,12);
    memcpy(&c->cfg,&p->cfg,sizeof(CLIENTCONFIG));
    c->flags |= GetClientSubversionFlags(c->version,p->subversion);
    int rs = ProcessCommand_Login_DoLicenseCheck(c,((c->version == VERSION_GAMECUBE) ? LICENSE_VERIFY_GAMECUBE : LICENSE_VERIFY_PC));
    if (rs) return rs;
    if (c->cfg.magic != 0x48615467)
    {
        memset(&c->cfg,0,sizeof(CLIENTCONFIG_BB));
        c->cfg.magic = 0x48615467;
    }
    return CommandUpdateClientConfig(c);
}

int ProcessCommand_BB_Login(SERVER* s,CLIENT* c,COMMAND_BB_LOGIN* p) // 93
{
    if (p->header.size != 0xB4) return 13;
    strncpy(c->license.username,p->username,0x10);
    strncpy(c->license.password,p->password,0x10);
    memcpy(&c->cfg,&p->cfg,sizeof(CLIENTCONFIG_BB));
    c->flags |= FLAG_PSOV3_BB;
    int rs = ProcessCommand_Login_DoLicenseCheck(c,LICENSE_VERIFY_BLUEBURST | LICENSE_VERIFY_CHECK_PASSWORD);
    if (rs) return rs;
    if (c->cfg.magic != 0x48615467)
    {
        memset(&c->cfg,0,sizeof(CLIENTCONFIG_BB));
        c->cfg.magic = 0x48615467;
    } else c->cfg.bbGameState++;
    return CommandBBClientInit(c,BB_LOGIN_ERROR_NONE);
}

int ProcessCommand_FS_Login(SERVER* s,CLIENT* c,COMMAND_FS_LOGIN* p) // FS shipgate login command
{
    operation_lock(c);
    wcscpy(c->name,p->name);
    memcpy(c->cfg.ports,p->ports,8);
    operation_unlock(c);
    return CommandSimple(c,0x0E,0x00);
}

int ProcessCommand_DCPCGC_ClientChecksum(SERVER* s,CLIENT* c,void*) { return CommandSimple(c,0x0097,0x00000001); } // 96

int ProcessCommand_DCPCGC_TimeRequest(SERVER* s,CLIENT* c,void*) { return CommandTime(c); } // B1

////////////////////////////////////////////////////////////////////////////////
// Ep3 commands. Note that these commands are not at all functional. The command
// handlers that partially worked were lost in a dead hard drive, unfortunately.

int ProcessCommand_GC_Ep3Jukebox(SERVER* s,CLIENT* c,COMMAND_GC_JUKEBOX* p)
{
    if (p->header.size < 0x0C) return 13;
    LOBBY* l = FindLobby(s,c->lobbyID);
    if (!l) return 0;
    if (!(l->flags & LOBBY_FLAG_EP3)) return 854936;
    p->header.flag = 0x03;
    p->unknown[0] = 0xFFFFFFFF;
    p->unknown[1] = 0x00000000;
    void* data[] = {p,NULL,NULL,NULL,NULL,NULL};
    return SendCommandToLobby(l,NULL,data);
}

int ProcessCommand_GC_Ep3MenuChallenge(SERVER* s,CLIENT* c,void* p) { return CommandSimple(c,0xDC,0x00); } // DC

int ProcessCommand_GC_Ep3ServerDataCommand(SERVER* s,CLIENT* c,COMMAND_GC_EP3GAMEDATA* p) // CA
{
    LOBBY* l = FindLobby(s,c->lobbyID);
    if (!l) return 854951;
    if (!(l->flags & LOBBY_FLAG_GAME) && !(l->flags & LOBBY_FLAG_EP3)) return 854952;
    if (p->entry[0].byte[0] != 0xB3) return 854953;

    switch (p->entry[1].byte[0])
    {
      // phase 1: map select 
      case 0x40:
        CommandEp3SendMapList(l);
        break;
      case 0x41:
        CommandEp3SendMapData(l,p->entry[4].dword);
        break;
      /*// phase 2: deck/name entry 
      case 0x13:
        ti = FindTeam(s,c->teamID);
        memcpy(&ti->ep3game,(void*)((DWORD)c->bufferin + 0x14),0x2AC);
        CommandEp3InitChangeState(s,c,1);
        break;
      case 0x1B:
        ti = FindTeam(s,c->teamID);
        memcpy(&ti->ep3names[*(BYTE*)((DWORD)c->bufferin + 0x24)],(void*)((DWORD)c->bufferin + 0x14),0x14); // NOTICE: may be 0x26 instead of 0x24 
        CommandEp3InitSendNames(s,c);
        break;
      case 0x14:
        ti = FindTeam(s,c->teamID);
        memcpy(&ti->ep3decks[*(BYTE*)((DWORD)c->bufferin + 0x14)],(void*)((DWORD)c->bufferin + 0x18),0x58); // NOTICE: may be 0x16 instead of 0x14 
        Ep3FillHand(&ti->ep3game,&ti->ep3decks[*(BYTE*)((DWORD)c->bufferin + 0x14)],&ti->ep3pcs[*(BYTE*)((DWORD)c->bufferin + 0x14)]);
        //Ep3RollDice(&ti->ep3game,&ti->ep3pcs[*(BYTE*)((DWORD)c->bufferin + 0x14)]);
        CommandEp3InitSendDecks(s,c);
        CommandEp3InitSendMapLayout(s,c);
        for (x = 0, param = 0; x < 4; x++) if ((ti->ep3decks[x].clientID != 0xFFFFFFFF) && (ti->ep3names[x].clientID != 0xFF)) param++;
        if (param >= ti->ep3game.numPlayers) CommandEp3InitChangeState(s,c,3);
        break;
      // phase 3: hands & game states 
      case 0x1D:
        ti = FindTeam(s,c->teamID);
        Ep3ReprocessMap(&ti->ep3game);
        CommandEp3SendMapData(s,c,ti->ep3game.mapID);
        for (y = 0, x = 0; x < 4; x++)
        {
            if ((ti->ep3decks[x].clientID == 0xFFFFFFFF) || (ti->ep3names[x].clientID == 0xFF)) continue;
            Ep3EquipCard(&ti->ep3game,&ti->ep3decks[x],&ti->ep3pcs[x],0); // equip SC card 
            CommandEp3InitHandUpdate(s,c,x);
            CommandEp3InitStatUpdate(s,c,x);
            y++;
        }
        CommandEp3Init_B4_06(s,c,(y == 4) ? true : false);
        CommandEp3InitSendMapLayout(s,c);
        for (x = 0; x < 4; x++)
        {
            if ((ti->ep3decks[x].clientID == 0xFFFFFFFF) || (ti->ep3names[x].clientID == 0xFF)) continue;
            CommandEp3Init_B4_4E(s,c,x);
            CommandEp3Init_B4_4C(s,c,x);
            CommandEp3Init_B4_4D(s,c,x);
            CommandEp3Init_B4_4F(s,c,x);
        }
        CommandEp3InitSendDecks(s,c);
        CommandEp3InitSendMapLayout(s,c);
        for (x = 0; x < 4; x++)
        {
            if ((ti->ep3decks[x].clientID == 0xFFFFFFFF) || (ti->ep3names[x].clientID == 0xFF)) continue;
            CommandEp3InitHandUpdate(s,c,x);
        }
        CommandEp3InitSendNames(s,c);
        CommandEp3InitChangeState(s,c,4);
        CommandEp3Init_B4_50(s,c);
        CommandEp3InitSendMapLayout(s,c);
        CommandEp3Init_B4_39(s,c); // MISSING: 60 00 AC 00 B4 2A 00 00 39 56 00 08 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
        CommandEp3InitBegin(s,c);
        break; */
      default:
        ConsolePrintColor("$0C> > Unknown Ep3 server data request: %02X\n",p->entry[1].byte[0]);
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// menu commands

int ProcessCommand_DCPCGC_MenuSelection(SERVER* s,CLIENT* c,COMMAND_DCPCGC_MENUSELECTION* p) // 10
{
    if ((c->version == VERSION_PC) && (p->headerpc.size < 0x0C)) return 13;
    else if (p->headerdcgc.size < 0x0C) return 13;
    if (c->version == VERSION_PC) c->lastMenuSelectionType = (p->headerpc.command == 0x09) ? 1 : 0;
    else c->lastMenuSelectionType = (p->headerdcgc.command == 0x09) ? 1 : 0;
    c->lastMenuSelection = p->itemID;
    if (c->version == VERSION_PC)
    {
        if (p->headerpc.size > 0x0C) wcsncpy(c->lastMenuSelectionPassword,p->passwordpc,0x10);
    } else {
        if (p->headerdcgc.size > 0x0C) tx_convert_to_unicode(c->lastMenuSelectionPassword,p->passworddcgc);
    }
    return 0;
}

int ProcessCommand_BB_MenuSelection(SERVER* s,CLIENT* c,COMMAND_BB_MENUSELECTION* p) // 10
{
    if (p->header.size < 0x10) return 0x10;
    c->lastMenuSelectionType = (p->header.command == 0x09) ? 1 : 0;
    c->lastMenuSelection = p->itemID;
    if (p->header.size > 0x10) wcsncpy(c->lastMenuSelectionPassword,p->password,0x10);
    return 0;
}

int ProcessCommand_DCPCGC_ChangeLobby(SERVER* s,CLIENT* c,COMMAND_DCPCGC_MENUSELECTION* p) // 84
{
    if ((c->version == VERSION_PC) && (p->headerpc.size != 0x0C)) return 13;
    else if (p->headerdcgc.size != 0x0C) return 13;
    LOBBY* next = FindLobby(s,p->itemID);
    if (next)
    {
        if ((next->flags & LOBBY_FLAG_EP3) && !(c->flags & FLAG_V4_GAMES)) return CommandLobbyMessageBox(NULL,NULL,c,L"$C6Can't change lobby\n\n$C7The lobby is for\nEpisode 3 only.");
        return ProcedureChangeLobby(s,FindLobby(s,c->lobbyID),next,c);
    }
    return CommandLobbyMessageBox(NULL,NULL,c,L"$C6Can't change lobby\n\n$C7The lobby does not exist.");
}

int ProcessCommand_BB_ChangeLobby(SERVER* s,CLIENT* c,COMMAND_BB_MENUSELECTION* p) // 84
{
    if (p->header.size != 0x10) return 13;
    LOBBY* next = FindLobby(s,p->itemID);
    if (next)
    {
        if (next->flags & LOBBY_FLAG_EP3) return CommandLobbyMessageBox(NULL,NULL,c,L"$C6Can't change lobby\n\n$C7The lobby is for\nEpisode 3 only.");
        return ProcedureChangeLobby(s,FindLobby(s,c->lobbyID),next,c);
    }
    return CommandLobbyMessageBox(NULL,NULL,c,L"$C6Can't change lobby\n\n$C7The lobby does not exist.");
}

int ProcessCommand_GameListRequest(SERVER* s,CLIENT* c,void*) // 08
{
    DWORD selection = 0;
    int errors = CommandGameSelect(s,c,&selection);
    if (errors) return errors;
    if (!selection) return 0;

    LOBBY* l = FindLobby(s,selection);
    if (!l) return CommandLobbyMessageBox(NULL,NULL,c,L"$C6This game is no\nlonger active.");;

    errors = ProcedureCheckToJoinGame(l,c);
    if (errors) return CommandLobbyMessageBox(NULL,NULL,c,ProcedureCheckToJoinGame_ErrorString(errors));

    int rv = ProcedureChangeLobby(s,FindLobby(s,c->lobbyID),l,c);
    if (rv) return rv;
    c->flags |= FLAG_LOADING;
    if (c->version == VERSION_BLUEBURST) ProcedureInitializeClientItems(l,c);
    return 0;
}

int ProcessCommand_ChangeShipRequest(SERVER* s,CLIENT* c,void*) // A0
{
    int rv = CommandMessageBox(c,L""); // we do this to avoid the "log window in message box" bug
    if (rv) return rv;
    return CommandReconnect(c,c->localip,CFGGetNumber(config,loginServerPortNames[c->version]));
}

int ProcessCommand_ChangeBlockRequest(SERVER* s,CLIENT* c,void*) // A1
{
    int rv = CommandMessageBox(c,L""); // we do this to avoid the "log window in message box" bug
    if (rv) return rv;
    return CommandReconnect(c,c->localip,CFGGetNumber(config,loginServerPortNames[c->version]));
}

////////////////////////////////////////////////////////////////////////////////
// Quest commands 

SHIP_SELECT_MENU quest_categories_mode[4] = {
    {L"Categories",NULL,MENU_FLAG_USE_ITEM_IDS,
      {L"Retrieval",L"Extermination",L"Events",L"Shops",L"Virtual Reality",L"Control Tower"},
      {L"$E$C6Quests that involve\nretrieving an object",L"$E$C6Quests that involve\ndestroying all\nmonsters",L"$E$C6Quests that are part\nof an event",L"$E$C6Quests that contain\nshops",L"$E$C6Quests that are\ndone in a simulator",L"$E$C6Quests that take\nplace at the Control\nTower"},
      {0,0,0,0,0,0},
      {QUEST_CATEGORY_RETRIEVAL,QUEST_CATEGORY_EXTERMINATION,QUEST_CATEGORY_EVENT,QUEST_CATEGORY_SHOP,QUEST_CATEGORY_VR,QUEST_CATEGORY_TOWER}},
    {L"Battle",NULL,MENU_FLAG_USE_ITEM_IDS,{L"Battle"},{L"$E$C6Battle mode rule\nsets"},{0},{QUEST_CATEGORY_ANY}},
    {L"Challenge",NULL,MENU_FLAG_USE_ITEM_IDS,{L"Challenge"},{L"$E$C6Challenge mode\nquests"},{0},{QUEST_CATEGORY_ANY}},
    {L"Solo Quests",NULL,MENU_FLAG_USE_ITEM_IDS,{L"Solo Quests"},{L"$E$C6Quests that require\na single player"},{0},{QUEST_CATEGORY_ANY}}};

SHIP_SELECT_MENU quest_categories_govt =
    {L"Government",NULL,MENU_FLAG_USE_ITEM_IDS,
      {L"Hero in Red",L"The Military's Hero",L"The Meteor Impact Incident"},
      {L"$E$CG-Red Ring Rico-\n$C6Quests that follow\nthe Episode 1\nstoryline",L"$E$CG-Heathcliff Flowen-\n$C6Quests that follow\nthe Episode 2\nstoryline",L"$E$C6Quests that follow\nthe Episode 4\nstoryline"},
      {0,0,0},
      {QUEST_CATEGORY_EP1,QUEST_CATEGORY_EP2,QUEST_CATEGORY_EP4}};

SHIP_SELECT_MENU quest_categories_download =
    {L"Categories",NULL,MENU_FLAG_USE_ITEM_IDS,
      {L"Retrieval",L"Extermination",L"Events",L"Shops",L"Virtual Reality",L"Control Tower",L"Download"},
      {L"$E$C6Quests that involve\nretrieving an object",L"$E$C6Quests that involve\ndestroying all\nmonsters",L"$E$C6Quests that are part\nof an event",L"$E$C6Quests that contain\nshops",L"$E$C6Quests that are\ndone in a simulator",L"$E$C6Quests that take\nplace at the Control\nTower",L"$E$C6Quests to download\nto your Memory Card"},
      {0,0,0,0,0,0,0},
      {QUEST_CATEGORY_RETRIEVAL,QUEST_CATEGORY_EXTERMINATION,QUEST_CATEGORY_EVENT,QUEST_CATEGORY_SHOP,QUEST_CATEGORY_VR,QUEST_CATEGORY_TOWER,QUEST_CATEGORY_DOWNLOAD}};

int ProcessCommand_QuestListRequest(SERVER* s,CLIENT* c,COMMAND_HEADER_BB* p) // A2
{
    if (!quests) return CommandLobbyMessageBox(NULL,NULL,c,L"$C6Quests are not available.");

    LOBBY* l = FindLobby(s,c->lobbyID);
    if (!l) return 8395;
    if (!(l->flags & LOBBY_FLAG_GAME)) return 8396;

    bool govt = false;
    SHIP_SELECT_MENU* menu = NULL;
    if (l->mode < 4) menu = &quest_categories_mode[l->mode];
    if (c->version == VERSION_BLUEBURST) if (p->flag) govt = true;
    if (govt) menu = &quest_categories_govt;
    if (!menu) return 8397;

    unsigned int x,errors;
    DWORD selection;
    errors = CommandShipSelectAsQuestSelect(s,c,menu,&selection);
    if (errors) return errors;

    QUEST* q = NULL;
    QUESTLIST* ql = FilterQuestList(quests,l->version,l->mode,(govt ? 0 : l->episode),selection);
    if (!ql) errors = CommandLobbyMessageBox(NULL,NULL,c,L"$C6There are no quests\navailable in that\ncategory.");
    else errors = CommandQuestSelect(s,c,ql,&q);

    if (q)
    {
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
        SendSubcommandToLobby(l,NULL,0x1D,0x00,NULL,0);
    }

    return 0;
}

int ProcessCommand_CancelQuestSelect(SERVER* s,CLIENT* c,void*) // A3
{
    c->lastMenuSelectionType = (-1); // this is to back out of quest select 
    return 0;
}

// the ping command (1D) is used by this server to start loading a quest. when
// the leader selects a quest, each player receives a 1D, to which it replies
// immediately with another 1D, which initiates the quest load process in that
// client's thread.
int ProcessCommand_PingReply(SERVER* s,CLIENT* c,void*) // 1D
{
    QUEST* q;
    int errors = 0;
    if (c->questLoading)
    {
        q = FindQuest(quests,c->questLoading);
        if (!q) return 78641;
        errors = ProcedureLoadQuest(c,q,false);
        c->questLoading = 0;
    }
    return errors;
}

int ProcessCommand_QuestReady(SERVER* s,CLIENT* c,void*) // AC
{
    LOBBY* l = FindLobby(s,c->lobbyID);
    if (!l) return 78642;
    operation_lock(l);

    unsigned int x;
    for (x = 0; x < l->maxClients; x++)
    {
        if (!l->clients[x]) continue;
        if (l->clients[x]->questLoading) break;
    }
    operation_unlock(l);
    if (x >= l->maxClients) return SendSubcommandToLobby(l,NULL,0xAC,0x00,NULL,0);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// player data commands 

int ProcessCommand_PC_PlayerData(SERVER* s,CLIENT* c,COMMAND_PC_PLAYERDATA* p) // 61/98
{
    operation_lock(c);
    ImportPlayerData(&p->data,&c->playerInfo,VERSION_PC);
    operation_unlock(c);
    if (p->header.command == 0x98)
    {
        LOBBY* l = FindLobby(s,c->lobbyID);
        if (l) return ProcedureChangeLobby(s,l,NULL,c);
    }
    return 0;
}

int ProcessCommand_GC_PlayerData(SERVER* s,CLIENT* c,COMMAND_GC_PLAYERDATA* p) // 61/98
{
    operation_lock(c);
    ImportPlayerData(&p->data,&c->playerInfo,VERSION_GAMECUBE);
    operation_unlock(c);
    if (p->header.command == 0x98)
    {
        LOBBY* l = FindLobby(s,c->lobbyID);
        if (l) return ProcedureChangeLobby(s,l,NULL,c);
    }
    return 0;
}

int ProcessCommand_BB_PlayerData(SERVER* s,CLIENT* c,COMMAND_BB_PLAYERDATA* p) // 61/98
{
    operation_lock(c);
    ImportPlayerData(&p->data,&c->playerInfo,VERSION_BLUEBURST);
    operation_unlock(c);
    if (p->header.command == 0x98)
    {
        LOBBY* l = FindLobby(s,c->lobbyID);
        if (l) return ProcedureChangeLobby(s,l,NULL,c);
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// subcommands 

int ProcessCommand_GC_Ep3GameCommand(SERVER* s,CLIENT* c,COMMAND_DCPCGC_SUBCOMMAND* p) // C9/CB
{
    c->lastMenuSelectionType = (-1); // this is to back out of game select 
    LOBBY* l = FindLobby(s,c->lobbyID);
    if (!l) return 4583;
    if (!(l->flags & LOBBY_FLAG_EP3)) return 4584;

    int x,rv = 0,rv2;
    operation_lock(l);
    for (x = 0; x < 12; x++)
    {
        if (!l->clients[x]) continue;
        if (l->clients[x] == c) continue;
        if (!(l->clients[x]->flags & FLAG_V4_GAMES)) continue;
        rv2 = SendCommandToClient(l->clients[x],p);
        if (rv2) rv = rv2;
    }
    operation_unlock(l);
    return rv;
}

int ProcessCommand_DCGC_GameCommand(SERVER* s,CLIENT* c,COMMAND_DCPCGC_SUBCOMMAND* p) // 60/62/6C/6D
{
    c->lastMenuSelectionType = (-1); // this is to back out of game select 
    LOBBY* l = FindLobby(s,c->lobbyID);
    if (!l) return 4593;

    bool priv = false;
    if ((p->headerdcgc.command == 0x62) || (p->headerdcgc.command == 0x6D)) priv = true;

    int res,num = (p->headerdcgc.size - 4) / 4;
    res = ProcessSubcommand(s,l,c,priv,p->headerdcgc.flag,p->entry,num,(l->flags & LOBBY_FLAG_HACKPROTECT));
    if (res == SUBCOMMAND_ERROR_ABSORBED) return 0;
    if (res) return res;

    if (priv)
    {
        if (!(l->flags & LOBBY_FLAG_GAME) && (p->headerdcgc.flag >= 12)) return 4594;
        if ((l->flags & LOBBY_FLAG_GAME) && (p->headerdcgc.flag >= 4)) return 4594;
        CLIENT* target = l->clients[p->headerdcgc.flag];
        return SendCommandToClient(target,p->headerdcgc.command,p->headerdcgc.flag,&p->entry[0],p->headerdcgc.size - 4);
    }
    return SendSubcommandToLobby(l,c,p->headerdcgc.command,p->headerdcgc.flag,p->entry,p->headerdcgc.size - 4);
}

int ProcessCommand_PC_GameCommand(SERVER* s,CLIENT* c,COMMAND_DCPCGC_SUBCOMMAND* p) // 60/62/6C/6D
{
    c->lastMenuSelectionType = (-1); // this is to back out of game select 
    LOBBY* l = FindLobby(s,c->lobbyID);
    if (!l) return 4593;

    bool priv = false;
    if ((p->headerpc.command == 0x62) || (p->headerpc.command == 0x6D)) priv = true;

    int res,num = (p->headerpc.size - 4) / 4;
    res = ProcessSubcommand(s,l,c,priv,p->headerpc.flag,p->entry,num,(l->flags & LOBBY_FLAG_HACKPROTECT));
    if (res == SUBCOMMAND_ERROR_ABSORBED) return 0;
    if (res) return res;

    if (priv)
    {
        if (!(l->flags & LOBBY_FLAG_GAME) && (p->headerpc.flag >= 12)) return 4594;
        if ((l->flags & LOBBY_FLAG_GAME) && (p->headerpc.flag >= 4)) return 4594;
        CLIENT* target = l->clients[p->headerpc.flag];
        return SendCommandToClient(target,p->headerpc.command,p->headerpc.flag,&p->entry[0],p->headerpc.size - 4);
    }
    return SendSubcommandToLobby(l,c,p->headerpc.command,p->headerpc.flag,p->entry,p->headerpc.size - 4);
}

int ProcessCommand_BB_GameCommand(SERVER* s,CLIENT* c,COMMAND_BB_SUBCOMMAND* p) // 60/62/6C/6D
{
    c->lastMenuSelectionType = (-1); // this is to back out of game select 
    LOBBY* l = FindLobby(s,c->lobbyID);
    if (!l) return 4593;

    bool priv = false;
    if ((p->header.command == 0x62) || (p->header.command == 0x6D)) priv = true;

    int res,num = (p->header.size - 8) / 4;
    res = ProcessSubcommand(s,l,c,priv,p->header.flag,p->entry,num,(l->flags & LOBBY_FLAG_HACKPROTECT));
    if (res == SUBCOMMAND_ERROR_ABSORBED) return 0;
    if (res) return res;

    if (priv)
    {
        if (!(l->flags & LOBBY_FLAG_GAME) && (p->header.flag >= 12)) return 4594;
        if ((l->flags & LOBBY_FLAG_GAME) && (p->header.flag >= 4)) return 4594;
        CLIENT* target = l->clients[p->header.flag];
        return SendCommandToClient(target,p->header.command,p->header.flag,&p->entry[0],p->header.size - 8);
    }
    return SendSubcommandToLobby(l,c,p->header.command,p->header.flag,p->entry,p->header.size - 8);
}

////////////////////////////////////////////////////////////////////////////////
// chat commands 

int ProcessCommand_Chat(SERVER* s,CLIENT* c,wchar_t* text)
{
    tx_remove_language_marker(text);

    wchar_t* marker = CFGGetStringW(config,"Chat_Command_Marker");
    if (marker) if (text[0] == marker[0]) return ProcessChatCommand(s,c,&text[1]);

    if (c->stfu) return 0;

    LOBBY* l;
    l = FindLobby(s,c->lobbyID);
    if (!l) return 897359544;
    return CommandChat(l,c,text);
}

int ProcessCommand_PC_Chat(SERVER* s,CLIENT* c,COMMAND_PC_CHAT* p) { return ProcessCommand_Chat(s,c,p->text); } // 06

int ProcessCommand_DCGC_Chat(SERVER* s,CLIENT* c,COMMAND_DCGC_CHAT* p) // 06
{
    // text from DC/GC clients must be converted to Unicode first.
    wchar_t textbuffer[0x200];
    tx_convert_to_unicode(textbuffer,p->text);
    return ProcessCommand_Chat(s,c,textbuffer);
}

int ProcessCommand_BB_Chat(SERVER* s,CLIENT* c,COMMAND_BB_CHAT* p) { return ProcessCommand_Chat(s,c,p->text); } // 06

////////////////////////////////////////////////////////////////////////////////
// BB commands

int ProcessCommand_BB_KeyConfigRequest(SERVER* s,CLIENT* c,void*) { return CommandBBSendKeyConfig(c); }

int ProcessCommand_BB_PlayerPreviewRequest(SERVER* s,CLIENT* c,COMMAND_BB_PLAYERPREVIEWREQUEST* p)
{
    if (c->cfg.bbGameState == BBSTATE_CHOOSE_CHAR)
    {
        c->cfg.bbplayernum = p->playerNum;
        c->cfg.bbGameState++;
        int rv = CommandBBClientInit(c,BB_LOGIN_ERROR_NONE);
        if (rv) return rv;
        return CommandBBApprovePlayerChoice(c);
    }
    return CommandBBSendPlayerPreview(c,p->playerNum);
}

int ProcessCommand_BB_ClientChecksum(SERVER* s,CLIENT* c,COMMAND_HEADER_BB* p)
{
    if (p->command == 0x01E8) return CommandBBAcceptClientChecksum(c);
    if (p->command == 0x03E8) return CommandBBSendGuildCardHeader(c);
    return 5784365;
}

int ProcessCommand_BB_GuildCardDataRequest(SERVER* s,CLIENT* c,COMMAND_BB_GUILDCARDREQUEST* p)
{
    if (p->cont) return CommandBBSendGuildCardData(c,p->chunkRequested);
    return 0;
}

int ProcessCommand_BB_StreamFileRequest(SERVER* s,CLIENT* c,COMMAND_HEADER_BB* p)
{
    if (p->command == 0x04EB) return CommandBBSendStreamFile(s,c);
    return 0;
}

int ProcessCommand_BB_CreateCharacter(SERVER* s,CLIENT* c,COMMAND_BB_CREATECHAR* p)
{
    char targetFile[MAX_PATH],targetFileB[MAX_PATH],sourceFile[MAX_PATH],sourceFile2[MAX_PATH];

    if (c->playerInfo.disp.playername[0]) return CommandMessageBox(c,L"$C6You are not on the Block Server,\nand therefore cannot create a character now.");

    c->cfg.bbplayernum = p->playernum;
    sprintf(c->playerInfo.bankname,"player%ld",p->playernum + 1);
    sprintf(targetFile, "system\\players\\player_%s_%ld.nsc",c->license.username,p->playernum + 1);
    sprintf(targetFileB,"system\\players\\bank_%s_%s.nsb",c->license.username,c->playerInfo.bankname);
    sprintf(sourceFile, "system\\blueburst\\player_class_%d.nsc",p->preview.charClass);
    sprintf(sourceFile2,"system\\blueburst\\char%d.pbb",p->preview.charClass);

    int errors = !PlayerLoadPlayerData(&c->playerInfo,sourceFile);
    if (errors) errors = !PlayerLoadPlayerDataOldFormat(&c->playerInfo,sourceFile2);
    if (errors)
    {
        CommandMessageBox(c,L"$C6New character could not be created.\n\nA server file is missing.");
        return 8984;
    }
    ApplyPlayerPreview(&c->playerInfo.disp,&p->preview);
    PLAYER_STATS* stats = GetBaseStats(leveltable,c->playerInfo.disp.charClass);
    if (!stats) memset(&c->playerInfo.disp.stats,0,sizeof(PLAYER_STATS));
    else memcpy(&c->playerInfo.disp.stats,stats,sizeof(PLAYER_STATS));
    if (!PlayerSavePlayerData(&c->playerInfo,targetFile))
    {
        CommandMessageBox(c,L"$C6New character could not be created.\n\nThe disk is full or write-protected.");
        return 8985;
    }
    if (!PlayerSaveBankData(&c->playerInfo.bank,targetFileB))
    {
        DeleteFile(targetFile);
        CommandMessageBox(c,L"$C6New bank data could not be created.\n\nThe disk is full or write-protected.");
        return 8986;
    }

    c->cfg.bbplayernum = p->playernum;
    int rv = CommandBBClientInit(c,0x00000000);
    if (rv) return rv;

    return CommandBBApprovePlayerChoice(c);
}

int ProcessCommand_BB_ChangeAccountData(SERVER* s,CLIENT* c,COMMAND_BB_WRITEACCOUNTDATA* p)
{
    wchar_t buffer[0x100];
    switch (p->header.command)
    {
      case 0x01ED:
        c->playerInfo.optionFlags = p->option;
        break;
      case 0x02ED:
        memcpy(c->playerInfo.symbolchats,p->symbolchats,0x04E0);
        break;
      case 0x03ED:
        memcpy(c->playerInfo.shortcuts,p->chatshortcuts,0x0A40);
        break;
      case 0x04ED:
        memcpy(&c->playerInfo.keyConfig.keyConfig,p->keyconfig,0x016C);
        break;
      case 0x05ED:
        memcpy(&c->playerInfo.keyConfig.joystickConfig,p->padconfig,0x0038);
        break;
      case 0x06ED:
        memcpy(&c->playerInfo.techMenuConfig,p->techmenu,0x0028);
        break;
      case 0x07ED:
        memcpy(c->playerInfo.disp.config,p->customize,0xE8);
        break;
      default:
        swprintf(buffer,L"$C4Unknown account command\n%04X %04X %08X\n",p->header.size,p->header.command,p->header.flag);
        CommandTextMessage(NULL,NULL,c,buffer);
        ConsolePrintColor("$0C> > Unknown account data command %04X %04X %08X\n",p->header.size,p->header.command,p->header.flag);
        return 0;
    }
    return 0;
}

int ProcessCommand_BB_ReturnPlayerData(SERVER* s,CLIENT* c,COMMAND_BB_PLAYERSAVE* p) // E7
{
    // we only trust the player's quest data and challenge data.
    if (p->header.size < 0x399C) return 48654564;
    memcpy(&c->playerInfo.challengeData,&p->data.challengeData,0x0140);
    memcpy(&c->playerInfo.questData1,&p->data.questData1,0x0208);
    memcpy(&c->playerInfo.questData2,&p->data.questData2,0x0058);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Lobby commands 

int ProcessCommand_PC_ChangeArrow(SERVER* s,CLIENT* c,COMMAND_HEADER_PC* p) // 89
{
    c->lobbyarrow = p->flag;
    return CommandLobbyChangeMarker(FindLobby(s,c->lobbyID));
}

int ProcessCommand_DCGC_ChangeArrow(SERVER* s,CLIENT* c,COMMAND_HEADER_DCGC* p) // 89
{
    c->lobbyarrow = p->flag;
    return CommandLobbyChangeMarker(FindLobby(s,c->lobbyID));
}

int ProcessCommand_BB_ChangeArrow(SERVER* s,CLIENT* c,COMMAND_HEADER_BB* p) // 89
{
    c->lobbyarrow = p->flag;
    return CommandLobbyChangeMarker(FindLobby(s,c->lobbyID));
}

int ProcessCommand_CardSearch(SERVER* s,CLIENT* c,DWORD targetID)
{
    unsigned int x;
    CLIENT* target;

    operation_lock(s);
    for (x = 0; x < s->numClients; x++) if (s->clients[x]->license.serialNumber == targetID) break;
    if (x < s->numClients) target = s->clients[x];
    operation_unlock(s);

    int res = 0;
    if (target)
    {
        operation_lock(target);
        res = CommandCardSearchResult(s,c,target);
        operation_unlock(target);
    }
    return res;
}

int ProcessCommand_DCPCGC_CardSearch(SERVER* s,CLIENT* c,COMMAND_DCPCGC_CARDSEARCH * p) { return ProcessCommand_CardSearch(s,c,p->targetSerialNumber); } // 40

int ProcessCommand_BB_CardSearch(SERVER* s,CLIENT* c,COMMAND_BB_CARDSEARCH * p) { return ProcessCommand_CardSearch(s,c,p->targetSerialNumber); } // 40

////////////////////////////////////////////////////////////////////////////////
// Info board commands 

int ProcessCommand_InfoBoardRequest(SERVER* s,CLIENT* c,void*) // D8
{
    LOBBY* l = FindLobby(s,c->lobbyID);
    if (!l) return 4594;
    return CommandSendInfoBoard(l,c);
}

int ProcessCommand_WriteInfoBoard(CLIENT* c,wchar_t* text)
{
    wcscpy(c->playerInfo.infoboard,text);
    return 0;
}

int ProcessCommand_PC_WriteInfoBoard(SERVER* s,CLIENT* c,COMMAND_PC_SETINFOBOARD* p) { return ProcessCommand_WriteInfoBoard(c,p->text); } // D9

int ProcessCommand_DCGC_WriteInfoBoard(SERVER* s,CLIENT* c,COMMAND_DCGC_SETINFOBOARD* p) // D9
{
    wchar_t textbuffer[0x100];
    tx_convert_to_unicode(textbuffer,p->text);
    return ProcessCommand_WriteInfoBoard(c,textbuffer);
}

int ProcessCommand_BB_WriteInfoBoard(SERVER* s,CLIENT* c,COMMAND_BB_SETINFOBOARD* p) { return ProcessCommand_WriteInfoBoard(c,p->text); } // D9

int ProcessCommand_SetAutoReply(CLIENT* c,wchar_t* text)
{
    if (!text) c->playerInfo.autoreply[0] = 0;
    else wcscpy(c->playerInfo.autoreply,text);
    return 0;
}

int ProcessCommand_PC_SetAutoReply(SERVER* s,CLIENT* c,COMMAND_PC_ENABLEAUTOREPLY* p) { return ProcessCommand_SetAutoReply(c,(p->header.size > 4) ? p->text : NULL); }

int ProcessCommand_DCGC_SetAutoReply(SERVER* s,CLIENT* c,COMMAND_DCGC_ENABLEAUTOREPLY* p)
{
    wchar_t textbuffer[0x100];
    if (p->header.size > 4)
    {
        tx_convert_to_unicode(textbuffer,p->text);
        return ProcessCommand_SetAutoReply(c,textbuffer);
    }
    return ProcessCommand_SetAutoReply(c,NULL);
}

int ProcessCommand_BB_SetAutoReply(SERVER* s,CLIENT* c,COMMAND_BB_ENABLEAUTOREPLY* p) { return ProcessCommand_SetAutoReply(c,(p->header.size > 8) ? p->text : NULL); }

int ProcessCommand_DisableAutoReply(SERVER* s,CLIENT* c,void*) { return ProcessCommand_SetAutoReply(c,NULL); }

int ProcessCommand_SetBlockedList(CLIENT* c,DWORD* blocked)
{
    memcpy(c->playerInfo.blocked,blocked,0x78);
    return 0;
}

int ProcessCommand_DCPCGC_SetBlockedList(SERVER* s,CLIENT* c,COMMAND_DCPCGC_BLOCKEDSENDERS* p) { return ProcessCommand_SetBlockedList(c,p->blocked); }
int ProcessCommand_BB_SetBlockedList(SERVER* s,CLIENT* c,COMMAND_BB_BLOCKEDSENDERS* p) { return ProcessCommand_SetBlockedList(c,p->blocked); }

////////////////////////////////////////////////////////////////////////////////
// Game commands 

// maximums for each variation index in each game
DWORD ProcessCommand_CreateGame_VariationMaxes_Online[3][0x20] = {
    {1,1,1,5,1,5,3,2,3,2,3,2,3,2,3,2,3,2,3,2,3,2,1,1,1,1,1,1,1,1,1,1},
    {1,1,2,1,2,1,2,1,2,1,1,3,1,3,1,3,2,2,1,3,2,2,2,2,1,1,1,1,1,1,1,1},
    {1,1,1,3,1,3,1,3,1,3,1,3,3,1,1,3,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}};

DWORD ProcessCommand_CreateGame_VariationMaxes_Solo[3][0x20] = {
    {1,1,1,3,1,3,3,1,3,1,3,1,3,2,3,2,3,2,3,2,3,2,1,1,1,1,1,1,1,1,1,1},
    {1,1,2,1,2,1,2,1,2,1,1,3,1,3,1,3,2,2,1,3,2,1,2,1,1,1,1,1,1,1,1,1},
    {1,1,1,3,1,3,1,3,1,3,1,3,3,1,1,3,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}};

LOBBY* ProcessCommand_CreateGame_BasicSetup(LOBBY* current,CLIENT* c,int ep,int diff,int battle,int chal,int solo)
{
    if (ep == 0) ep = 0xFF;
    if (((ep != 0xFF) && (ep > 3)) || (ep == 0)) return NULL;

    int x;
    char difficultyCheckName[0x20],filename[MAX_PATH];

    LOBBY* l = (LOBBY*)malloc(sizeof(LOBBY));
    if (!l) return NULL;
    memset(l,0,sizeof(LOBBY));
    operation_lock(l);

    l->version = c->version;
    l->sectionID = c->playerInfo.disp.sectionID;
    l->episode = ep;
    l->difficulty = diff;
    if (battle) l->mode = 1;
    if (chal) l->mode = 2;
    if (solo) l->mode = 3;
    l->event = ConvertLobbyEventToGameEvent(current->event);
    l->block = 0xFF;
    l->maxClients = 4;
    l->flags = LOBBY_FLAG_GAME | (current->flags & LOBBY_FLAG_HACKPROTECT) | ((l->episode != 0xFF) ? 0 : LOBBY_FLAG_EP3);
    sprintf(difficultyCheckName,"Minimum_Level_%d%d",(l->flags & LOBBY_FLAG_EP3) ? 0 : l->episode,l->difficulty);
    l->minlevel = CFGGetNumber(config,difficultyCheckName) - 1;
    l->maxlevel = 0xFFFFFFFF;
    l->rareSet = LoadItemRareSet("system\\blueburst\\ItemRT.rel",l->episode - 1,l->difficulty,l->sectionID);

    if (l->version == VERSION_BLUEBURST)
    {
        for (x = 0; x < 4; x++) l->nextItemID[x] = (0x00200000 * x) + 0x00010000;
        l->nextGameItemID = 0x00810000;
        l->enemies = (GAME_ENEMY_ENTRY*)malloc(0x0B50 * sizeof(GAME_ENEMY_ENTRY));
        if (!l->enemies)
        {
            free(l);
            return NULL;
        }
        if (l->mode == 3)
        {
            l->battleParamTable = &battleParamTable[1][l->episode - 1][l->difficulty][0];
            for (x = 0; x < 0x20; x++) l->variations[x] = rand() % ProcessCommand_CreateGame_VariationMaxes_Solo[(ep - 1)][x];
            for (x = 0; x < 0x10; x++)
            {
                sprintf(filename,"system\\blueburst\\map\\s%X%X%X%X.dat",l->episode,x,(int)l->variations[x * 2],(int)l->variations[(x * 2) + 1]);
                if (!LoadMapData(l->enemies,&l->numEnemies,filename,l->episode,l->difficulty,l->battleParamTable,false))
                {
                    sprintf(filename,"system\\blueburst\\map\\m%X%X%X%X.dat",l->episode,x,(int)l->variations[x * 2],(int)l->variations[(x * 2) + 1]);
                    LoadMapData(l->enemies,&l->numEnemies,filename,l->episode,l->difficulty,l->battleParamTable,false);
                }
            }
        } else {
            l->battleParamTable = &battleParamTable[0][l->episode - 1][l->difficulty][0];
            for (x = 0; x < 0x20; x++) l->variations[x] = rand() % ProcessCommand_CreateGame_VariationMaxes_Online[(ep - 1)][x];
            for (x = 0; x < 0x10; x++)
            {
                sprintf(filename,"system\\blueburst\\map\\m%X%X%X%X.dat",l->episode,x,(int)l->variations[x * 2],(int)l->variations[(x * 2) + 1]);
                LoadMapData(l->enemies,&l->numEnemies,filename,l->episode,l->difficulty,l->battleParamTable,false);
            }
        }
    }
    operation_unlock(l);
    return l;
}

int ProcessCommand_PC_CreateGame(SERVER* s,CLIENT* c,COMMAND_PC_CREATEGAME* p) // C1
{
    LOBBY* current = FindLobby(s,c->lobbyID);
    if (!current) return 468734779;

    LOBBY* l = ProcessCommand_CreateGame_BasicSetup(current,c,1,p->difficulty,p->battleMode,p->challengeMode,0);
    if (!l) return 5;
    if (l->minlevel > c->playerInfo.disp.level)
    {
        free(l);
        return CommandLobbyMessageBox(NULL,NULL,c,L"$C6Your level is too\nlow to make games\nof that difficulty.");
    }
    wcsncpy(l->name,p->name,0x10);
    wcsncpy(l->password,p->password,0x10);
    if (!AddLobby(s,l))
    {
        free(l);
        return 468734780;
    }

    int rv = ProcedureChangeLobby(s,current,l,c);
    if (rv) return rv;
    c->flags |= FLAG_LOADING;
    return 0;
}

int ProcessCommand_DCGC_CreateGame(SERVER* s,CLIENT* c,COMMAND_DCGC_CREATEGAME* p) // C1/EC
{
    LOBBY* current = FindLobby(s,c->lobbyID);
    if (!current) return 468734779;

    LOBBY* l = ProcessCommand_CreateGame_BasicSetup(current,c,((c->version == VERSION_DC) ? 1 : ((c->flags & FLAG_V4_GAMES) ? 0xFF : p->episode)),p->difficulty,p->battleMode,p->challengeMode,0);
    if (!l) return 5;
    if (l->minlevel > c->playerInfo.disp.level)
    {
        free(l);
        return CommandLobbyMessageBox(NULL,NULL,c,L"$C6Your level is too\nlow to make games\nof that difficulty.");
    }
    tx_convert_to_unicode(l->name,p->name);
    tx_convert_to_unicode(l->password,p->password);
    if (!AddLobby(s,l))
    {
        free(l);
        return 468734780;
    }

    int rv = ProcedureChangeLobby(s,current,l,c);
    if (rv) return rv;
    c->flags |= FLAG_LOADING;
    return 0;
}

int ProcessCommand_BB_CreateGame(SERVER* s,CLIENT* c,COMMAND_BB_CREATEGAME* p) // C1
{
    LOBBY* current = FindLobby(s,c->lobbyID);
    if (!current) return 468734779;

    LOBBY* l = ProcessCommand_CreateGame_BasicSetup(current,c,p->episode,p->difficulty,p->battleMode,p->challengeMode,p->soloMode);
    if (!l) return 5;
    if (l->minlevel > c->playerInfo.disp.level)
    {
        free(l);
        return CommandLobbyMessageBox(NULL,NULL,c,L"$C6Your level is too\nlow to make games\nof that difficulty.");
    }
    wcsncpy(l->name,p->name,0x10);
    wcsncpy(l->password,p->password,0x10);
    if (!AddLobby(s,l))
    {
        free(l);
        return 468734780;
    }

    int rv = ProcedureChangeLobby(s,current,l,c);
    if (rv) return rv;
    c->flags |= FLAG_LOADING;
    ProcedureInitializeClientItems(l,c);
    return 0;
}

int ProcessCommand_LobbyName(SERVER* s,CLIENT* c,void*) // 8A
{
    LOBBY* l = FindLobby(s,c->lobbyID);
    if (!l) return 75893365;
    return CommandLobbyName(c,l->name);
}

int ProcessCommand_ClientReady(SERVER* s,CLIENT* c,void*) // 6F
{
    LOBBY* l = FindLobby(s,c->lobbyID);
    if (!l) return 75893368;
    c->flags &= (~FLAG_LOADING);

    CommandResumeGame(l,c); // tell the other players to stop waiting for the new player to load 
    CommandTime(c); // tell the new player the time 
    CommandSimple(c,0x0095,0x00000000); // get character info 

    wchar_t buffer[0x180],levelstring[0x10];
    //if (l->rareSet) CommandTextMessage(NULL,NULL,c,L"$C6The item rare set\nfor this game could\nnot be loaded.\nThere will be no\nrare items.");

    if (l->maxlevel == 0xFFFFFFFF) swprintf(levelstring,L"%d+",l->minlevel + 1);
    else swprintf(levelstring,L"%d-%d",l->minlevel + 1,l->maxlevel + 1);
    swprintf(buffer,L"$C6Lobby ID: %08X\nLevels: %s\nSection ID: %s\nAnti-hack: %s\nCheat mode: %s",l->lobbyID,levelstring,sectionIDNames[l->sectionID],(l->flags & LOBBY_FLAG_HACKPROTECT) ? L"on" : L"off",(l->flags & LOBBY_FLAG_CHEAT) ? L"on" : L"off");

    return CommandTextMessage(NULL,NULL,c,buffer);
}

////////////////////////////////////////////////////////////////////////////////
// Team commands 

int ProcessCommand_BB_TeamCommand(SERVER* s,CLIENT* c,COMMAND_HEADER_BB* bb) // EA
{
    if (bb->command == 0x01EA) return CommandLobbyMessageBox(NULL,NULL,c,L"$C6Teams are not supported.");
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Patch server commands 

int ProcessCommand_Patch_EncrOK(SERVER* s,CLIENT* c,void*)
{
    CommandSimple(c,0x0004,0x00000000); // this command requests the user's login information 
    return 0;
}

int ProcessCommand_Patch_LoginInfo(SERVER* s,CLIENT* c,COMMAND_PATCH_LOGININFO* p)
{
    strncpy(c->license.username,p->username,0x10);
    strncpy(c->license.password,p->password,0x10);

    wchar_t buffer[0x200];
    int result = VerifyLicense(licenses,&c->license,LICENSE_VERIFY_BLUEBURST | LICENSE_VERIFY_CHECK_PASSWORD);
    wcscpy(buffer,L"$C7NewServ Patch Server v1.0\n\nPlease note that this server is for private use only.\nThis server is not affiliated with, sponsored by, or in any\nother way connected to SEGA or Sonic Team, and is owned\nand operated completely independently.\n\nLicense check: ");
    if (result == LICENSE_RESULT_INVALID_CALL) wcscat(buffer,L"Internal error");
    if (result == LICENSE_RESULT_OK)           wcscat(buffer,L"OK");
    if (result == LICENSE_RESULT_NOTFOUND)     wcscat(buffer,L"Unregistered");
    if (result == LICENSE_RESULT_WRONGPASS)    wcscat(buffer,L"Wrong password");
    if (result == LICENSE_RESULT_BANNED)       wcscat(buffer,L"Banned");
    CommandMessageBox(c,buffer);
    CommandPatchCheckDirectory(c,".");
    CommandPatchCheckDirectory(c,"data");
    CommandPatchCheckDirectory(c,"scene");
    CommandSimple(c,0x000A,0x00000000);
    CommandSimple(c,0x000A,0x00000000);
    CommandSimple(c,0x000A,0x00000000);
    CommandSimple(c,0x0012,0x00000000); // this command terminates the patch connection successfully 
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Command pointer arrays 

int ProcessCommandIgnored(SERVER* s,CLIENT* c,void*) { return 0; }
int ProcessCommandErrorHandler(SERVER* s,CLIENT* c,void* buffer)
{
    COMMAND_HEADER_BB* bb = (COMMAND_HEADER_BB*)buffer;
    COMMAND_HEADER_PC* pc = (COMMAND_HEADER_PC*)buffer;
    COMMAND_HEADER_DCGC* dcgc = (COMMAND_HEADER_DCGC*)buffer;
    switch (c->version)
    {
      case VERSION_GAMECUBE:
      case VERSION_DC:
      case VERSION_FUZZIQER:
        ConsolePrintColor("$0C> > Unknown command [DC/GC/FS]: %02X %02X %04X\n",dcgc->command,dcgc->flag,dcgc->size);
        break;
      case VERSION_BLUEBURST:
        ConsolePrintColor("$0C> > Unknown command [BB]: %08X %04X %04X\n",bb->size,bb->command,bb->flag);
        break;
      case VERSION_PC:
      case VERSION_PATCH:
        ConsolePrintColor("$0C> > Unknown command [PC/Patch]: %04X %02X %02X\n",pc->size,pc->command,pc->flag);
    }
    return 87953;
}

// The entries in these arrays correspond to the ID of the command received. For
// instance, if a command 6C is received, the function at position 0x6C in the
// array corresponding to the client's version is called.
void* ProcessCommandHandlers[VERSION_MAX][0x100] = {
{ // VERSION_GAMECUBE 
NULL,NULL,NULL,NULL,NULL,(void*)ProcessCommandIgnored,(void*)ProcessCommand_DCGC_Chat,NULL,(void*)ProcessCommand_GameListRequest,(void*)ProcessCommand_DCPCGC_MenuSelection,NULL,NULL,NULL,NULL,NULL,NULL,//00 
(void*)ProcessCommand_DCPCGC_MenuSelection,NULL,NULL,(void*)ProcessCommandIgnored,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,(void*)ProcessCommand_PingReply,NULL,NULL,//10 
NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,//20 
NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,//30 
(void*)ProcessCommand_DCPCGC_CardSearch,NULL,NULL,NULL,(void*)ProcessCommandIgnored,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,//40 
NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,//50 
(void*)ProcessCommand_DCGC_GameCommand,(void*)ProcessCommand_GC_PlayerData,(void*)ProcessCommand_DCGC_GameCommand,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,(void*)ProcessCommand_DCGC_GameCommand,(void*)ProcessCommand_DCGC_GameCommand,NULL,(void*)ProcessCommand_ClientReady,//60 
NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,//70 
NULL,NULL,NULL,NULL,(void*)ProcessCommand_DCPCGC_ChangeLobby,NULL,NULL,NULL,NULL,(void*)ProcessCommand_DCGC_ChangeArrow,(void*)ProcessCommand_LobbyName,NULL,NULL,NULL,NULL,NULL,//80 
NULL,NULL,NULL,NULL,NULL,NULL,(void*)ProcessCommand_DCPCGC_ClientChecksum,NULL,(void*)ProcessCommand_GC_PlayerData,(void*)ProcessCommandIgnored,NULL,NULL,(void*)ProcessCommand_PCGC_Login_C,(void*)ProcessCommand_PCGC_Login_E,(void*)ProcessCommand_PCGC_Login_E,NULL,//90 
(void*)ProcessCommand_ChangeShipRequest,(void*)ProcessCommand_ChangeBlockRequest,(void*)ProcessCommand_QuestListRequest,NULL,NULL,NULL,NULL,NULL,NULL,(void*)ProcessCommand_CancelQuestSelect,NULL,NULL,(void*)ProcessCommand_QuestReady,NULL,NULL,NULL,//A0 
NULL,(void*)ProcessCommand_DCPCGC_TimeRequest,NULL,NULL,NULL,NULL,NULL,(void*)ProcessCommandIgnored,(void*)ProcessCommandIgnored,NULL,(void*)ProcessCommand_GC_Ep3Jukebox,NULL,NULL,NULL,NULL,NULL,//B0 
NULL,(void*)ProcessCommand_DCGC_CreateGame,NULL,NULL,NULL,NULL,(void*)ProcessCommand_DCPCGC_SetBlockedList,(void*)ProcessCommand_DCGC_SetAutoReply,(void*)ProcessCommand_DisableAutoReply,(void*)ProcessCommand_GC_Ep3GameCommand,(void*)ProcessCommand_GC_Ep3ServerDataCommand,(void*)ProcessCommand_GC_Ep3GameCommand,NULL,NULL,NULL,NULL,//C0 
NULL,NULL,NULL,NULL,NULL,NULL,(void*)ProcessCommandIgnored,NULL,(void*)ProcessCommand_InfoBoardRequest,(void*)ProcessCommand_DCGC_WriteInfoBoard,NULL,(void*)ProcessCommand_GC_VerifyLicense,(void*)ProcessCommand_GC_Ep3MenuChallenge,NULL,NULL,NULL,//D0 
NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,(void*)ProcessCommand_DCGC_CreateGame,NULL,NULL,NULL,//E0 
},{ // VERSION_BLUEBURST 
NULL,NULL,NULL,NULL,NULL,(void*)ProcessCommandIgnored,(void*)ProcessCommand_BB_Chat,NULL,(void*)ProcessCommand_GameListRequest,(void*)ProcessCommand_BB_MenuSelection,NULL,NULL,NULL,NULL,NULL,NULL,//00 
(void*)ProcessCommand_BB_MenuSelection,NULL,NULL,(void*)ProcessCommandIgnored,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,(void*)ProcessCommand_PingReply,NULL,NULL,//10 
NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,//20 
NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,//30 
(void*)ProcessCommand_BB_CardSearch,NULL,NULL,NULL,(void*)ProcessCommandIgnored,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,//40 
NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,//50 
(void*)ProcessCommand_BB_GameCommand,(void*)ProcessCommand_BB_PlayerData,(void*)ProcessCommand_BB_GameCommand,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,(void*)ProcessCommand_BB_GameCommand,(void*)ProcessCommand_BB_GameCommand,NULL,(void*)ProcessCommand_ClientReady,//60 
NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,//70 
NULL,NULL,NULL,NULL,(void*)ProcessCommand_BB_ChangeLobby,NULL,NULL,NULL,NULL,(void*)ProcessCommand_BB_ChangeArrow,(void*)ProcessCommand_LobbyName,NULL,NULL,NULL,NULL,NULL,//80 
NULL,NULL,NULL,(void*)ProcessCommand_BB_Login,NULL,NULL,NULL,NULL,(void*)ProcessCommand_BB_PlayerData,(void*)ProcessCommandIgnored,NULL,NULL,NULL,NULL,NULL,NULL,//90 
(void*)ProcessCommand_ChangeShipRequest,(void*)ProcessCommand_ChangeBlockRequest,(void*)ProcessCommand_QuestListRequest,NULL,NULL,NULL,NULL,NULL,NULL,(void*)ProcessCommand_CancelQuestSelect,NULL,NULL,(void*)ProcessCommand_QuestReady,NULL,NULL,NULL,//A0 
NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,//B0 
NULL,(void*)ProcessCommand_BB_CreateGame,NULL,NULL,NULL,NULL,(void*)ProcessCommand_BB_SetBlockedList,(void*)ProcessCommand_BB_SetAutoReply,(void*)ProcessCommand_DisableAutoReply,NULL,NULL,NULL,NULL,NULL,NULL,NULL,//C0 
NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,(void*)ProcessCommand_InfoBoardRequest,(void*)ProcessCommand_BB_WriteInfoBoard,NULL,NULL,(void*)ProcessCommand_BB_GuildCardDataRequest,NULL,NULL,NULL,//D0 
(void*)ProcessCommand_BB_KeyConfigRequest,NULL,NULL,(void*)ProcessCommand_BB_PlayerPreviewRequest,NULL,(void*)ProcessCommand_BB_CreateCharacter,NULL,(void*)ProcessCommand_BB_ReturnPlayerData,(void*)ProcessCommand_BB_ClientChecksum,NULL,(void*)ProcessCommand_BB_TeamCommand,(void*)ProcessCommand_BB_StreamFileRequest,(void*)ProcessCommandIgnored,(void*)ProcessCommand_BB_ChangeAccountData,NULL,NULL,//E0 
},{ // VERSION_PC 
NULL,NULL,NULL,NULL,NULL,(void*)ProcessCommandIgnored,(void*)ProcessCommand_PC_Chat,NULL,(void*)ProcessCommand_GameListRequest,(void*)ProcessCommand_DCPCGC_MenuSelection,NULL,NULL,NULL,NULL,NULL,NULL,//00 
(void*)ProcessCommand_DCPCGC_MenuSelection,NULL,NULL,(void*)ProcessCommandIgnored,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,(void*)ProcessCommand_PingReply,NULL,NULL,//10 
NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,//20 
NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,//30 
(void*)ProcessCommand_DCPCGC_CardSearch,NULL,NULL,NULL,(void*)ProcessCommandIgnored,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,//40 
NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,//50 
(void*)ProcessCommand_PC_GameCommand,(void*)ProcessCommand_PC_PlayerData,(void*)ProcessCommand_PC_GameCommand,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,(void*)ProcessCommand_PC_GameCommand,(void*)ProcessCommand_PC_GameCommand,NULL,(void*)ProcessCommand_ClientReady,//60 
NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,//70 
NULL,NULL,NULL,NULL,(void*)ProcessCommand_DCPCGC_ChangeLobby,NULL,NULL,NULL,NULL,(void*)ProcessCommand_PC_ChangeArrow,(void*)ProcessCommand_LobbyName,NULL,NULL,NULL,NULL,NULL,//80 
NULL,NULL,NULL,NULL,NULL,NULL,(void*)ProcessCommand_DCPCGC_ClientChecksum,NULL,(void*)ProcessCommand_PC_PlayerData,(void*)ProcessCommandIgnored,(void*)ProcessCommand_PC_Login_A,NULL,(void*)ProcessCommand_PCGC_Login_C,(void*)ProcessCommand_PCGC_Login_E,(void*)ProcessCommand_PCGC_Login_E,NULL,//90 
(void*)ProcessCommand_ChangeShipRequest,(void*)ProcessCommand_ChangeBlockRequest,(void*)ProcessCommand_QuestListRequest,NULL,NULL,NULL,NULL,NULL,NULL,(void*)ProcessCommand_CancelQuestSelect,NULL,NULL,(void*)ProcessCommand_QuestReady,NULL,NULL,NULL,//A0 
NULL,(void*)ProcessCommand_DCPCGC_TimeRequest,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,//B0 
NULL,(void*)ProcessCommand_PC_CreateGame,NULL,NULL,NULL,NULL,(void*)ProcessCommand_DCPCGC_SetBlockedList,(void*)ProcessCommand_BB_SetAutoReply,(void*)ProcessCommand_DisableAutoReply,NULL,NULL,NULL,NULL,NULL,NULL,NULL,//C0 
NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,(void*)ProcessCommand_InfoBoardRequest,(void*)ProcessCommand_PC_WriteInfoBoard,NULL,NULL,NULL,NULL,NULL,NULL,//D0 
},{ // VERSION_DC 
NULL,NULL,NULL,NULL,NULL,(void*)ProcessCommandIgnored,(void*)ProcessCommand_DCGC_Chat,NULL,(void*)ProcessCommand_GameListRequest,(void*)ProcessCommand_DCPCGC_MenuSelection,NULL,NULL,NULL,NULL,NULL,NULL,//00 
(void*)ProcessCommand_DCPCGC_MenuSelection,NULL,NULL,(void*)ProcessCommandIgnored,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,(void*)ProcessCommand_PingReply,NULL,NULL,//10 
NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,//20 
NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,//30 
(void*)ProcessCommand_DCPCGC_CardSearch,NULL,NULL,NULL,(void*)ProcessCommandIgnored,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,//40 
NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,//50 
(void*)ProcessCommand_DCGC_GameCommand,NULL,(void*)ProcessCommand_DCGC_GameCommand,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,(void*)ProcessCommand_DCGC_GameCommand,(void*)ProcessCommand_DCGC_GameCommand,NULL,(void*)ProcessCommand_ClientReady,//60 
NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,//70 
NULL,NULL,NULL,NULL,(void*)ProcessCommand_DCPCGC_ChangeLobby,NULL,NULL,NULL,NULL,(void*)ProcessCommand_DCGC_ChangeArrow,(void*)ProcessCommand_LobbyName,NULL,NULL,NULL,NULL,NULL,//80 
NULL,NULL,NULL,NULL,NULL,NULL,(void*)ProcessCommand_DCPCGC_ClientChecksum,NULL,NULL,(void*)ProcessCommandIgnored,NULL,NULL,NULL,NULL,NULL,NULL,//90 
(void*)ProcessCommand_ChangeShipRequest,(void*)ProcessCommand_ChangeBlockRequest,(void*)ProcessCommand_QuestListRequest,NULL,NULL,NULL,NULL,NULL,NULL,(void*)ProcessCommand_CancelQuestSelect,NULL,NULL,(void*)ProcessCommand_QuestReady,NULL,NULL,NULL,//A0 
NULL,(void*)ProcessCommand_DCPCGC_TimeRequest,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,//B0 
NULL,(void*)ProcessCommand_DCGC_CreateGame,NULL,NULL,NULL,NULL,(void*)ProcessCommand_DCPCGC_SetBlockedList,(void*)ProcessCommand_DCGC_SetAutoReply,(void*)ProcessCommand_DisableAutoReply,NULL,NULL,NULL,NULL,NULL,NULL,NULL,//C0 
NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,(void*)ProcessCommand_InfoBoardRequest,(void*)ProcessCommand_DCGC_WriteInfoBoard,NULL,NULL,NULL,NULL,NULL,NULL,//D0 
},{ // VERSION_PATCH 
NULL,NULL,(void*)ProcessCommand_Patch_EncrOK,NULL,(void*)ProcessCommand_Patch_LoginInfo,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,//00 
NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,//10 
},{ // VERSION_FUZZIQER 
NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,(void*)ProcessCommand_FS_Login,NULL,//00 
NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,//10 
NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,//20 
}};

// Receives a command from a client, decrypts it, and calls the appropriate handler
int ReceiveCommandFromClient(CLIENT* c,void** buffer,WORD* command)
{
    int rv,headerSize = ((c->version == VERSION_BLUEBURST) ? 8 : 4);
    BYTE header[headerSize];
    COMMAND_HEADER_PC* pc = (COMMAND_HEADER_PC*)header;
    COMMAND_HEADER_DCGC* gc = (COMMAND_HEADER_DCGC*)header;
    COMMAND_HEADER_BB* bb = (COMMAND_HEADER_BB*)header;
    DWORD datasize,sendsize;

    if (!c) return 5;
    if (c->disconnect) return 8;

    *buffer = NULL;
    switch (c->version)
    {
      case VERSION_GAMECUBE:
      case VERSION_DC:
      case VERSION_FUZZIQER:
        rv = ReceiveClient(c,header,headerSize);
        if (c->encrypt) CRYPT_CryptData(&c->cryptin,header,headerSize,false);
        *command = gc->command;
        datasize = gc->size;
        if (c->encrypt) sendsize = ((datasize + 3) & ~3);
        break;
      case VERSION_PC:
      case VERSION_PATCH:
        rv = ReceiveClient(c,header,headerSize);
        if (c->encrypt) CRYPT_CryptData(&c->cryptin,header,headerSize,false);
        *command = pc->command;
        datasize = pc->size;
        if (c->encrypt) sendsize = ((datasize + 3) & ~3);
        break;
      case VERSION_BLUEBURST:
        rv = ReceiveClient(c,header,headerSize);
        if (c->encrypt) CRYPT_CryptData(&c->cryptin,header,headerSize,false);
        *command = bb->command;
        datasize = bb->size;
        if (c->encrypt) sendsize = ((datasize + 7) & ~7);
    }

    if (rv) return rv;

    void* encbuffer = malloc(sendsize);
    if (!encbuffer) return 1;

    memcpy(encbuffer,header,headerSize);
    rv = ReceiveClient(c,(void*)((DWORD)encbuffer + headerSize),sendsize - headerSize);
    if (c->encrypt) CRYPT_CryptData(&c->cryptin,(void*)((DWORD)encbuffer + headerSize),sendsize - headerSize,false);

    if (CFGIsValuePresent(config,"Show_Client_Data"))
    {
        operation_lock(&sendCommandConsoleLock);
        ConsolePrintColor("$0A> Receiving from %S [version %d] [%04X bytes]:\n",c->name,c->version,datasize);
        if (datasize < (unsigned)CFGGetNumber(config,"Maximum_Display_Command_Size_Recv")) CRYPT_PrintData(encbuffer,datasize);
        operation_unlock(&sendCommandConsoleLock);
    }

    *buffer = encbuffer;
    if (!rv) c->lastrecv = GetTickCount();
    return rv;
}

// continually receives commands from a client and processes them until the client
// disconnects, an error occurs, or one of the given commands is reached. For example:
// ProcessCommands(s,c,0xA3,0x10,0x09,0x00) would receive commands until an A3, 10, or 09
// was received (and would process the last received command) or an error occured.
int ProcessCommands(SERVER* s,CLIENT* c, ...)
{
    int error = 0;
    va_list va;
    void* buffer;
    WORD command,stop;
    while (!error && !c->disconnect)
    {
        error = ReceiveCommandFromClient(c,&buffer,&command);
        if (error) break;
        if (ProcessCommandHandlers[c->version][command & 0xFF]) error = ((int (*)(SERVER*,CLIENT*,void*))ProcessCommandHandlers[c->version][command & 0xFF])(s,c,buffer);
        else error = ProcessCommandErrorHandler(s,c,buffer);
        free(buffer);
        va_start(va,c);
        do {
            stop = va_arg(va,int);
            if (command == stop) break;
        } while (stop);
        va_end(va);
        if (stop) break;
    }
    return error;
}

