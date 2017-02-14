#include <windows.h>

#include "text.h"
#include "operation.h"
#include "console.h"

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

// Starts a server with the given name, handler routine, and callback, listening on the given ports (terminated with 0)
// Example: SERVER* httpServer = StartServer("HTTP Server",HTTPHandlerRoutine,HTTPConnectCallback,80,81,0)
// This example starts an HTTP server on ports 80 and 81.
SERVER* StartServer(char* serverName,LPTHREAD_START_ROUTINE HandlerRoutine,LISTEN_THREAD_CALLBACK HandleConnection, ...)
{
    int x,y,port;
    va_list va;

    SERVER* server = (SERVER*)malloc(sizeof(SERVER));
    if (!server) return NULL;
    memset(server,0,sizeof(SERVER));
    operation_lock(server);
    strcpy(server->name,serverName);
    server->maxClients = 0xFFFFFFFF;
    server->clientThread = HandlerRoutine;
    va_start(va,HandleConnection);
    for (x = 0; (port = va_arg(va,int)) && (x < 0x10); x++)
    {
        operation_lock(&server->threads[x]);
        server->threads[x].addr = INADDR_ANY;
        server->threads[x].port = port;
        server->threads[x].callback = HandleConnection;
        server->threads[x].param = (long)server;
        operation_unlock(&server->threads[x]);
        if (!BeginListenThread(&server->threads[x])) break;
    }
    va_end(va);
    if (port)
    {
        for (y = 0; y < x; y++) EndListenThread(&server->threads[x],100);
        free(server);
        server = NULL;
    } else operation_unlock(server);
    return server;
}

// Stops a server, disconnects all clients, and frees anything associated with it.
bool StopServer(SERVER* s)
{
    unsigned int x;
    operation_lock(s);
    for (x = 0; x < 0x10; x++) if (s->threads[x].thread) EndListenThread(&s->threads[x],100);
    for (x = 0; x < s->numLobbies; x++) DeleteLobby(s->lobbies[x]);
    free(s->lobbies);
    for (x = 0; x < s->numClients; x++) DeleteClient(s->clients[x]);
    free(s->clients);
    free(s);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

// finds a client's index in the list of connected clients to the server.
// the server should be operation_locked before calling this function! 
DWORD FindClient(SERVER* s,CLIENT* c)
{
    if (!s || !c) return 0xFFFFFFFF;

    unsigned int x;
    operation_lock(s);
    for (x = 0; x < s->numClients; x++) if (s->clients[x] == c) break;
    if (x >= s->numClients) x = 0xFFFFFFFF;
    operation_unlock(s);
    return x;
}

// finds a client on the server given the serial number (Guild Card number)
CLIENT* FindClient(SERVER* s,DWORD serialNumber)
{
    DWORD x;
    CLIENT* c = NULL;
    operation_lock(s);
    for (x = 0; x < s->numClients; x++) if (s->clients[x]->license.serialNumber == serialNumber) c = s->clients[x];
    operation_unlock(s);
    return c;
}

// adds a client to a server
bool AddClient(SERVER* s,CLIENT* c)
{
    if (!s || !c) return false;

    operation_lock(s);
    s->clients = (CLIENT**)realloc(s->clients,sizeof(CLIENT*) * (s->numClients + 1));
    if (!s->clients)
    {
        operation_unlock(s);
        return false;
    }
    s->clients[s->numClients] = c;
    s->numClients++;
    operation_unlock(s);
    return true;
}

// removes a client from a server
bool RemoveClient(SERVER* s,CLIENT* c)
{
    if (!s || !c) return false;

    operation_lock(s);
    unsigned int index = FindClient(s,c);
    if ((index < 0) || (index >= s->numClients))
    {
        operation_unlock(s);
        return false;
    }
    s->numClients--;
    memcpy(&s->clients[index],&s->clients[index + 1],sizeof(CLIENT*) * (s->numClients - index));
    s->clients = (CLIENT**)realloc(s->clients,sizeof(CLIENT*) * s->numClients);
    operation_unlock(s);
    if (!s->clients) return false;
    return true;
}

// finds a lobby with room, and adds the client to that lobby
bool AddClientToAvailableLobby(SERVER* s,CLIENT* c)
{
    unsigned int x,y;
    operation_lock(s);
    for (x = 0; x < s->numLobbies; x++)
    {
        if (!(s->lobbies[x]->flags & LOBBY_FLAG_PUBLIC)) continue;
        if (AddClient(s->lobbies[x],c)) break;
    }
    y = s->numLobbies; // do this in case s->numLobbies changes right after we operation_unlock s 
    operation_unlock(s);
    if (x < y) return true;
    return false;
}

////////////////////////////////////////////////////////////////////////////////

// finds a lobby's index on the server
DWORD FindLobby(SERVER* s,LOBBY* l)
{
    if (!s || !l) return 0xFFFFFFFF;

    unsigned int x;
    operation_lock(s);
    for (x = 0; x < s->numLobbies; x++) if (s->lobbies[x] == l) break;
    if (x >= s->numLobbies) x = 0xFFFFFFFF;
    operation_unlock(s);
    return x;
}

// finds a lobby based on lobby ID
LOBBY* FindLobby(SERVER* s,DWORD lobbyID)
{
    unsigned int x;
    LOBBY* ret = NULL;
    operation_lock(s);
    for (x = 0; x < s->numLobbies; x++) if (s->lobbies[x]->lobbyID == lobbyID) break;
    if (x < s->numLobbies) ret = s->lobbies[x];
    operation_unlock(s);
    return ret;
}

// finds a lobby by its block number. not recommended, since lobbies can have the same block number.
LOBBY* FindLobbyByBlockNumber(SERVER* s,DWORD block)
{
    unsigned int x;
    LOBBY* ret = NULL;
    operation_lock(s);
    for (x = 0; x < s->numLobbies; x++)
    {
        if (s->lobbies[x]->flags & LOBBY_FLAG_GAME) continue;
        if (s->lobbies[x]->block == block) break;
    }
    if (x < s->numLobbies) ret = s->lobbies[x];
    operation_unlock(s);
    return ret;
}

// finds a named lobby (that is, a game)
LOBBY* FindLobbyByName(SERVER* s,wchar_t* name,bool casesensitive)
{
    unsigned int x;
    LOBBY* ret = NULL;
    operation_lock(s);
    if (casesensitive)
    {
        for (x = 0; x < s->numLobbies; x++) if (!wcscmp(s->lobbies[x]->name,name)) break;
    } else {
        for (x = 0; x < s->numLobbies; x++) if (!wcsicmp(s->lobbies[x]->name,name)) break;
    }
    if (x < s->numLobbies) ret = s->lobbies[x];
    operation_unlock(s);
    return ret;
}

// adds a lobby to the server
bool AddLobby(SERVER* s,LOBBY* l)
{
    if (!s || !l) return false;

    operation_lock(s);
    operation_lock(l);
    do l->lobbyID = (rand() << 16) | rand();
    while (FindLobby(s,l->lobbyID) && l->lobbyID);
    operation_unlock(l);

    s->lobbies = (LOBBY**)realloc(s->lobbies,sizeof(LOBBY*) * (s->numLobbies + 1));
    if (!s->lobbies)
    {
        operation_unlock(s);
        return false;
    }
    s->lobbies[s->numLobbies] = l;
    s->numLobbies++;
    operation_unlock(s);
    return true;
}

// removes a lobby from the server
bool RemoveLobby(SERVER* s,LOBBY* l)
{
    if (!s || !l) return false;

    unsigned int x;
    operation_lock(l);
    for (x = 0; x < l->maxClients; x++) if (l->clients[x]) break;
    if (x < l->maxClients)
    {
        operation_unlock(l);
        return false;
    }

    operation_lock(s);
    unsigned int index = FindLobby(s,l);
    if ((index < 0) || (index >= s->numLobbies))
    {
        operation_unlock(s);
        operation_unlock(l);
        return false;
    }
    s->numLobbies--;
    memcpy(&s->lobbies[index],&s->lobbies[index + 1],sizeof(LOBBY*) * (s->numLobbies - index));
    s->lobbies = (LOBBY**)realloc(s->lobbies,sizeof(LOBBY*) * s->numLobbies);
    operation_unlock(s);
    operation_unlock(l);
    if (!s->lobbies) return false;
    return true;
}

