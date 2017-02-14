#include <windows.h>
#include <stdio.h>

#include "text.h"
#include "operation.h"
#include "console.h"

#include "netconfig.h"
#include "quest.h"

#include "encryption.h"
#include "license.h"
#include "version.h"
#include "battleparamentry.h"
#include "itemraretable.h"
#include "player.h"
#include "map.h"
#include "client.h"

#include "listenthread.h"
#include "lobby.h"
#include "server.h"

#include "commonservers.h"
#include "loginserver.h"

#include "command-functions.h"

extern CFGFile* config;

// creates the lobbies used by players on the lobby server
int SetupLobbyServer(SERVER* s)
{
    int x,y;
    LOBBY* l;
    for (x = 0; x < 20; x++)
    {
        l = (LOBBY*)malloc(sizeof(LOBBY));
        if (!l) break;
        memset(l,0,sizeof(LOBBY));
        l->block = x + 1;
        l->maxlevel = 0xFFFFFFFF;
        swprintf(l->name,L"LOBBY%d",x + 1);
        l->flags = LOBBY_FLAG_HACKPROTECT | LOBBY_FLAG_PUBLIC | LOBBY_FLAG_DEFAULT | ((x > 14) ? LOBBY_FLAG_EP3 : 0);
        l->maxClients = 12;
        l->type = x;
        if (!AddLobby(s,l))
        {
            free(l);
            break;
        }
    }
    if (x < 20)
    {
        for (y = 0; y < x; y++) free(s->lobbies[x]);
        free(s->lobbies);
        return 1;
    }

    return 0;
}

// thie function is called when a player connects to the lobby server
DWORD HandleLobbyClient(NEW_CLIENT_THREAD_DATA* nctd)
{
    SERVER* s = nctd->s;
    CLIENT* c = nctd->c;
    nctd->release = true;
    nctd = NULL;
    srand(GetTickCount());

    char filename[MAX_PATH],filename2[MAX_PATH],bankname[MAX_PATH];
    int result;

    // add the client to the server
    ConsolePrintColor("$0E> Lobby server: new client\n");
    AddClient(s,c);

    // send an init command and get username/password
    CommandServerInit(c,true);
    ProcessCommands(s,c,0x9D,0x9E,0x0093,0);

    int errors = 0;
    if (c->version == VERSION_BLUEBURST)
    {
        // if the client is BB, then get its player and account data and send it to them
        //errors = GetPlayerInfoFromShipgate(...);
        //if (errors)
        //{
            sprintf(filename,"system\\players\\account_%s.nsa",c->license.username);
            sprintf(filename2,"system\\players\\%s.act",c->license.username);
            result = PlayerLoadAccountData(&c->playerInfo,filename);
            if (!result) result = PlayerLoadAccountDataOldFormat(&c->playerInfo,filename2);
            if (!result) result = PlayerLoadAccountData(&c->playerInfo,"system\\blueburst\\default.nsa");
            if (!result) result = PlayerLoadAccountDataOldFormat(&c->playerInfo,"system\\blueburst\\default.act");
            if (!result)
            {
                CommandMessageBox(c,L"$C6Your account data could not be found.");
                errors = 4534632;
            } else {
                sprintf(c->playerInfo.bankname,"player%d",c->cfg.bbplayernum + 1);
                sprintf(filename,"system\\players\\player_%s_%d.nsc",c->license.username,c->cfg.bbplayernum + 1);
                sprintf(filename2,"system\\players\\%s-player-%d.pbb",c->license.username,c->cfg.bbplayernum);
                sprintf(bankname,"system\\players\\bank_%s_%s.nsb",c->license.username,c->playerInfo.bankname);
                errors = !PlayerLoadPlayerData(&c->playerInfo,filename);
                if (errors) errors = !PlayerLoadPlayerDataOldFormat(&c->playerInfo,filename2);
                if (errors) CommandMessageBox(c,L"$C6Your player data could not be found.");
                else {
                    errors = !PlayerLoadBankData(&c->playerInfo.bank,bankname);
                    if (errors) errors = !PlayerLoadBankData(&c->playerInfo.bank,"system\\blueburst\\default.nsb");
                    if (errors) CommandMessageBox(c,L"$C6Warning: Your bank data could not be loaded.");
                }
            }
        //}
    }
    c->playTimeBegin = GetTickCount();

    // send the lobby list and add them to an open lobby, if possible
    LOBBY* l;
    if (!errors)
    {
        CommandSendLobbyList(s,c);
        if (c->version == VERSION_BLUEBURST) CommandBBSendPlayerInfo(c);
        CommandSimple(c,0x0095,0x00000000); // this is GetCharacterInfo(), in effect 
        ProcessCommands(s,c,0x0061,0);
        ConsolePrintColor("$0E> Lobby server: client joining lobby\n");
        if (ProcedureChangeLobby(s,NULL,NULL,c))
        {
            ConsolePrintColor("$0E> Lobby server error: lobbies full\n");
            CommandMessageBox(c,L"$C6All lobbies are currently full.");
        } else {
            ConsolePrintColor("$0E> Lobby server: client leaving lobby with error code %d\n",ProcessCommands(s,c,0));
            l = FindLobby(s,c->lobbyID);
            if (l) ProcedureChangeLobby(s,l,NULL,c);
        }
    }

    c->playerInfo.disp.playTime += ((GetTickCount() - c->playTimeBegin) / 1000);

    // if the client is BB, save the player and account data
    if (c->version == VERSION_BLUEBURST)
    {
        //errors = SendPlayerInfoToShipgate(...);
        //if (errors)
        //{
            sprintf(filename,"system\\players\\account_%s.nsa",c->license.username);
            PlayerSaveAccountData(&c->playerInfo,filename);
            sprintf(filename,"system\\players\\player_%s_%d.nsc",c->license.username,c->cfg.bbplayernum + 1);
            PlayerSavePlayerData(&c->playerInfo,filename);
            sprintf(filename,"system\\players\\bank_%s_%s.nsb",c->license.username,c->playerInfo.bankname);
            PlayerSaveBankData(&c->playerInfo.bank,filename);
        //}
    }

    // remove and delete the client
    RemoveClient(s,c);
    DeleteClient(c);
    ConsolePrintColor("$0E> Lobby server: disconnecting client\n");

    return 0;
}

