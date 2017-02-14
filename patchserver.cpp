#include <windows.h>

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
#include "patchserver.h"

#include "command-functions.h"

extern CFGFile* config;

// this function is called when a client connects on the patch server.
// the patch server's operation is handled entirely by the command handlers;
// all we need to do is send an init command and pass control to the command handlers.
DWORD HandlePatchClient(NEW_CLIENT_THREAD_DATA* nctd)
{
    SERVER* s = nctd->s;
    CLIENT* c = nctd->c;
    nctd->release = true;
    nctd = NULL;

    ConsolePrintColor("$0E> Patch server: new client\n");

    CommandServerInit(c,false); // bool startserver doesn't matter for patch clients 
    ProcessCommands(s,c,0);

    ConsolePrintColor("$0E> Patch server: disconnecting client\n");

    DeleteClient(c);

    return 0;
}

