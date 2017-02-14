#include <windows.h>
#include <stdio.h>

#include "text.h"
#include "operation.h"
#include "console.h"

#include "netconfig.h"
#include "quest.h"

#include "encryption.h"
#include "license.h"
#include "battleparamentry.h"
#include "itemraretable.h"
#include "player.h"
#include "map.h"
#include "client.h"

#include "listenthread.h"
#include "lobby.h"
#include "server.h"

#include "commonservers.h"
#include "blockserver.h"

#include "command-structures.h"
#include "command-functions.h"

extern CFGFile* config;

// called when a client connects to the block server
DWORD HandleBlockClient(NEW_CLIENT_THREAD_DATA* nctd)
{
    SERVER* s = nctd->s;
    CLIENT* c = nctd->c;
    nctd->release = true;
    nctd = NULL;
    srand(GetTickCount());

    char filename[MAX_PATH],filename2[MAX_PATH];
    bool result;

    // add the client to the server
    ConsolePrintColor("$0E> Block server: new client\n");
    AddClient(s,c);

    // send an init command and get username/password
    CommandServerInit(c,false);
    ProcessCommands(s,c,0x0093,0);

    // then do what needs to be done, depending on this client's state
    switch (c->cfg.bbGameState)
    {
      case BBSTATE_INITIAL_LOGIN:
        // first login? send them to the other port
        CommandReconnect(c,c->localip,CFGGetNumber(config,"Block_Server_Port_1_BB"));
        break;
      case BBSTATE_DOWNLOAD_DATA:
        // download data? send them their account data and player previews
        sprintf(filename,"system\\players\\account_%s.nsa",c->license.username);
        sprintf(filename2,"system\\players\\%s.act",c->license.username);
        result = PlayerLoadAccountData(&c->playerInfo,filename);
        if (!result) result = PlayerLoadAccountDataOldFormat(&c->playerInfo,filename2);
        if (!result) result = PlayerLoadAccountData(&c->playerInfo,"system\\blueburst\\default.nsa");
        if (!result) result = PlayerLoadAccountDataOldFormat(&c->playerInfo,"system\\blueburst\\default.act");
        if (!result) CommandMessageBox(c,L"$C6Your account data could not be found.");
        else ProcessCommands(s,c,0);
        break;
      case BBSTATE_CHOOSE_CHAR:
        // choose character? just wait; the command handlers will handle it
        ProcessCommands(s,c,0x00E3,0x00EC,0);
        break;
      case BBSTATE_SAVE_CHAR:
        // create character / use dressing room? just wait; the command handlers will handle it
        ProcessCommands(s,c,0);
        break;
      default:
        // unknown state? send them to the login server
        CommandReconnect(c,c->localip,CFGGetNumber(config,"Login_Server_Port_BB"));
    }

    // when we're done, remove and delete the client.
    RemoveClient(s,c);
    DeleteClient(c);
    ConsolePrintColor("$0E> Block server: disconnecting client\n");

    return 0;
}

