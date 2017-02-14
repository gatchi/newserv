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

#include "command-functions.h"

extern CFGFile* config;

OPERATION_LOCK shipgateMenuOperation = {0,0};
SHIP_SELECT_MENU shipgateServerMenu;

// called when a client connects to the shipgate server.
// this function simply sends an init comamnd and passes control off to the
// command handlers, which will do their jobs.
DWORD HandleShipgateClient(NEW_CLIENT_THREAD_DATA* nctd)
{
    SERVER* s = nctd->s;
    CLIENT* c = nctd->c;
    nctd->release = true;
    nctd = NULL;
    srand(GetTickCount());

    ConsolePrintColor("$0E> Shipgate server: new client\n");
    AddClient(s,c);

    CommandServerInit(c,true);
    ProcessCommands(s,c,0x0D,0);

    RemoveClient(s,c);
    DeleteClient(c);
    ConsolePrintColor("$0E> Shipgate server: disconnecting client\n");

    return 0;
}

