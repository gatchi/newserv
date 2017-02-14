#include <windows.h>

#include "text.h"
#include "operation.h"
#include "console.h"

#include "encryption.h"
#include "license.h"
#include "version.h"
#include "battleparamentry.h"
#include "itemraretable.h"
#include "player.h"
#include "map.h"
#include "client.h"

#include "lobby.h"

// reassigns the leader of a lobby/game to a new client, excluding the given client.
bool ChangeLeader(LOBBY* l,unsigned int leaving)
{
    unsigned int x;
    for (x = 0; x < l->maxClients; x++)
    {
        if (x == leaving) continue;
        if (l->clients[x]) break;
    }
    if (l->clients[x]) l->leaderID = x;
    return true;
}

// tells whether someone in the lobby/game is still loading (aka bursting).
bool CheckLoadingPlayers(LOBBY* l)
{
    if (!l) return false;
    unsigned int x,num = 0;
    operation_lock(l);
    for (x = 0; x < l->maxClients; x++)
    {
        if (!l->clients[x]) continue;
        if (l->clients[x]->flags & FLAG_LOADING) num++;
    }
    operation_unlock(l);
    if (num) return true;
    return false;
}

// counts the clients in a lobby.
unsigned int CountLobbyPlayers(LOBBY* l)
{
    if (!l) return 0xFFFFFFFF;
    unsigned int x,num = 0;
    for (x = 0; x < l->maxClients; x++) if (l->clients[x]) num++;
    return num;
}

// adds a client to a lobby. (returns false on error, true otherwise)
bool AddClient(LOBBY* l,CLIENT* c)
{
    unsigned int x,num;
    operation_lock(l);
    num = CountLobbyPlayers(l);
    if ((num == 0xFFFFFFFF) || (num >= l->maxClients))
    {
        operation_unlock(l);
        return false;
    }
    for (x = 0; x < l->maxClients; x++) if (!l->clients[x]) break;
    if (x >= l->maxClients)
    {
        operation_unlock(l);
        return false;
    }
    operation_lock(c);
    l->clients[x] = c;
    if (num == 0) l->leaderID = x;
    c->clientID = x;
    c->lobbyID = l->lobbyID;
    operation_unlock(c);
    operation_unlock(l);
    return true;
}

// removes a client from a lobby. (returns false if the client isn't in the lobby)
bool RemoveClient(LOBBY* l,CLIENT* c)
{
    unsigned int x;
    operation_lock(l);
    for (x = 0; x < l->maxClients; x++) if (l->clients[x] == c) break;
    if (x >= l->maxClients)
    {
        operation_unlock(l);
        return false;
    }
    operation_lock(c);
    l->clients[x] = NULL;
    c->lobbyID = 0;
    operation_unlock(c);
    ChangeLeader(l,x);
    operation_unlock(l);
    return true;
}

// atomically changes a client's lobby. this is necessary since changing lobbies
// by calling AddClient/RemoveClient can cause massive errors if two clients do
// it at the same time.
// note that this function assigns client IDs from the end, backwards (i.e. 11 is
// first, 0 is last in lobbies; blue is first, red is last in games). I use this
// to debug some in-game commands that put the client ID in odd places. to change
// this behavior back to normal, replace the two lines above the commented lines
// with the commented lines.
bool ChangeLobby(LOBBY* current,LOBBY* next,CLIENT* c)
{
    if (current == next) return true;

    unsigned int nx,cx,num;
    operation_lock(next);
    num = CountLobbyPlayers(next);
    if ((num == 0xFFFFFFFF) || (num >= next->maxClients))
    {
        operation_unlock(next);
        return false;
    }
    for (nx = next->maxClients - 1; nx >= 0; nx--) if (!next->clients[nx]) break;
    if (nx < 0)
    //for (nx = 0; nx < next->maxClients; nx++) if (!next->clients[nx]) break;
    //if (nx >= next->maxClients)
    {
        operation_unlock(next);
        return false;
    }

    operation_lock(current);
    for (cx = 0; cx < current->maxClients; cx++) if (current->clients[cx] == c) break;
    if (cx >= current->maxClients)
    {
        operation_unlock(current);
        operation_unlock(next);
        return false;
    }
    current->clients[cx] = NULL;
    ChangeLeader(current,cx);
    next->clients[nx] = c;
    if (num == 0) next->leaderID = nx;

    operation_lock(c);
    c->clientID = nx;
    c->lobbyID = next->lobbyID;
    operation_unlock(c);
    operation_unlock(current);
    operation_unlock(next);
    return true;
}

// returns the game event ID that matches the given lobby event ID, or 0 if there is no matching game event.
int ConvertLobbyEventToGameEvent(int event)
{
    if (event > 7) return 0;
    if (event == 7) return 2;
    if (event == 2) return 0;
    return event;
}

// deletes a lobby.
void DeleteLobby(LOBBY* l)
{
    operation_lock(l);
    if (l->enemies) free(l->enemies);
    if (l->rareSet) FreeItemRareSet(l->rareSet);
    if (l->items) free(l->items);
    //if (l->ep3) free(l->ep3);
    free(l);
}

////////////////////////////////////////////////////////////////////////////////

// adds a dropped item to a game
int AddItem(LOBBY* l,PLAYER_ITEM* item)
{
    operation_lock(l);
    l->items = (PLAYER_ITEM*)realloc(l->items,sizeof(PLAYER_ITEM) * (l->numItems + 1));
    if (!l->items)
    {
        l->numItems = 0;
        operation_unlock(l);
        return 698553477;
    }
    memcpy(&l->items[l->numItems],item,sizeof(PLAYER_ITEM));
    l->numItems++;
    operation_unlock(l);
    return 0;
}

// removes a dropped item from a game
int RemoveItem(LOBBY* l,DWORD itemID,PLAYER_ITEM* item)
{
    operation_lock(l);
    int index = FindItem(l,itemID);
    if (index == (-1))
    {
        operation_unlock(l);
        return 698553486;
    }
    if (item) memcpy(item,&l->items[index],sizeof(PLAYER_ITEM));
    l->numItems--;
    memcpy(&l->items[index],&l->items[index + 1],sizeof(PLAYER_ITEM) * (l->numItems - index));
    l->items = (PLAYER_ITEM*)realloc(l->items,sizeof(PLAYER_ITEM) * l->numItems);
    operation_unlock(l);
    return 0;
}

// finds and item in the current game
int FindItem(LOBBY* l,DWORD itemID)
{
    unsigned int x;
    operation_lock(l);
    for (x = 0; x < l->numItems; x++) if (l->items[x].data.itemID == itemID) break;
    if (x >= l->numItems) x = 0xFFFFFFFF;
    operation_unlock(l);
    return x;
}

// allocates an item ID in the player's item ID space, or in the game's item ID
// space if the specified player is invalid (i.e. >= 4)
DWORD GetNextItemID(LOBBY* l,DWORD player)
{
    if (player < 4)
    {
        l->nextItemID[player]++;
        return l->nextItemID[player] - 1;
    }
    l->nextGameItemID++;
    return l->nextGameItemID - 1;
}
