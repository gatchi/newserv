#include <windows.h>

#include "text.h"
#include "operation.h"
#include "console.h"

#include "netconfig.h"

#include "version.h"
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

extern CFGFile* config;

// returns the PSO version that supposedly connects on the given port.
int PortVersion(int port)
{
    char* names[]  = {"Japanese_1.0_Port_GC","Japanese_1.1_Port_GC","Japanese_Ep3_Port_GC","American_1.0/1.1_Port_GC","American_Ep3_Port_GC","European_1.0_Port_GC","European_1.1_Port_GC","European_Ep3_Port_GC","Patch_Server_Port_PC","Patch_Server_Port_BB","Auth_Server_Port_PC","Block_Server_Port_BB","Auth_Server_Port_GC","Block_Server_Port_1_BB","Block_Server_Port_2_BB","Lobby_Server_Port_PC","Lobby_Server_Port_GC","Lobby_Server_Port_BB","Login_Server_Port_PC","Login_Server_Port_GC","Login_Server_Port_BB","Fuzziqer_Shipgate_Port",NULL};
    int versions[] = {VERSION_GAMECUBE      ,VERSION_GAMECUBE      ,VERSION_GAMECUBE      ,VERSION_GAMECUBE          ,VERSION_GAMECUBE      ,VERSION_GAMECUBE      ,VERSION_GAMECUBE      ,VERSION_GAMECUBE      ,VERSION_PATCH         ,VERSION_PATCH         ,VERSION_PC           ,VERSION_BLUEBURST     ,VERSION_GAMECUBE     ,VERSION_BLUEBURST       ,VERSION_BLUEBURST       ,VERSION_PC            ,VERSION_GAMECUBE      ,VERSION_BLUEBURST     ,VERSION_PC            ,VERSION_GAMECUBE      ,VERSION_BLUEBURST     ,VERSION_FUZZIQER        ,VERSION_UNKNOWN};

    int x;
    for (x = 0; names[x]; x++) if (port == CFGGetNumber(config,names[x])) break;
    return versions[x];
}

// this function is called when a client connects to a server. It creates a CLIENT
// structure, fills in some basic information, and starts a thread specified by
// the server to handle this client.
bool HandleConnection(DWORD local,DWORD remote,WORD port,int newsocket,SERVER* server)
{
    NEW_CLIENT_THREAD_DATA nctd;

    DWORD now = GetTickCount();
    CLIENT* c = (CLIENT*)malloc(sizeof(CLIENT));
    if (!c) return false;
    memset(c,0,sizeof(CLIENT));
    operation_lock(c);
    wcscpy(c->name,L"[ Unnamed ]");
    c->version = PortVersion(port);
    c->localip = local;
    c->remoteip = remote;
    c->port = port;
    c->socket = newsocket;
    c->lastrecv = now;
    c->lastsend = now;
    nctd.s = server;
    nctd.c = c;
    nctd.release = false;
    c->thread = CreateThread(NULL,0,server->clientThread,&nctd,0,&c->threadID);
    if (!c->thread)
    {
        operation_unlock(c);
        free(c);
        return false;
    }
    while (!nctd.release) Sleep(0);
    operation_unlock(c);
    return true;
}
