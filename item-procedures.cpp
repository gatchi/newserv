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

#include "command-functions.h"

// Gives a player's items their initial IDs when joining a game.
int ProcedureInitializeClientItems(LOBBY* l,CLIENT* c)
{
    long x;
    for (x = 0; x < c->playerInfo.inventory.numItems; x++) c->playerInfo.inventory.items[x].data.itemID = GetNextItemID(l,c->clientID);
    return 0;
}

// Reorders a client's inventory according to the given list of 30 item IDs.
int ProcedureSortClientItems(CLIENT* c,DWORD* itemIDs)
{
    PLAYER_INVENTORY sorted;
    memset(&sorted,0,sizeof(PLAYER_INVENTORY));

    long x,y;
    operation_lock(c);
    for (x = 0; x < 30; x++)
    {
        if (itemIDs[x] == 0xFFFFFFFF) sorted.items[x].data.itemID = 0xFFFFFFFF;
        else {
            y = FindItem(&c->playerInfo.inventory,itemIDs[x]);
            if (y == (-1))
            {
                operation_unlock(c);
                return 854934;
            }
            memcpy(&sorted.items[x],&c->playerInfo.inventory.items[y],sizeof(PLAYER_ITEM));
        }
    }

    sorted.numItems = c->playerInfo.inventory.numItems;
    sorted.hpMat = c->playerInfo.inventory.hpMat;
    sorted.tpMat = c->playerInfo.inventory.tpMat;
    sorted.language = c->playerInfo.inventory.language;
    memcpy(&c->playerInfo.inventory,&sorted,sizeof(PLAYER_INVENTORY));
    operation_unlock(c);

    return 0;
}

