#include <windows.h>
#include <stdio.h>

#include "text.h"
#include "operation.h"
#include "console.h"

#include "netconfig.h"
#include "quest.h"
#include "battleparamentry.h"
#include "itemraretable.h"
#include "map.h"

#include "encryption.h"
#include "license.h"
#include "privilege.h"
#include "version.h"
#include "player.h"
#include "levels.h"
#include "client.h"

#include "listenthread.h"
#include "lobby.h"
#include "items.h"
#include "server.h"

#include "command-functions.h"
#include "command-input-subs.h"

// The functions in this file are called when a BB client sends a game command
// (60, 62, 6C, or 6D) that must be handled by the server.

extern CFGFile* config;
extern LICENSE_LIST* licenses;
extern QUESTLIST* quests;
extern LEVEL_TABLE* leveltable;

////////////////////////////////////////////////////////////////////////////////
// Chat commands and the like 

// client requests to send a quild card
int ProcessSubcommand_SendGuildCard(SERVER*,LOBBY* l,CLIENT* c,bool priv,int target,SUBCOMMAND* p,int num,bool hackprotect)
{
    if (!priv) return 8764578;
    if (c->version == VERSION_GAMECUBE)
    {
        if (num < 0x25) return SUBCOMMAND_ERROR_SIZE_MISMATCH;
        tx_convert_to_unicode(c->playerInfo.guildcarddesc,(char*)(&p[9]));
    }
    if (!l->clients[target]) return 0;
    int rv = CommandSendGuildCard(l->clients[target],c);
    if (rv) return rv;
    return SUBCOMMAND_ERROR_ABSORBED;
}

// client sends a symbol chat
int ProcessSubcommand_SymbolChat(SERVER*,LOBBY*,CLIENT* c,bool,int,SUBCOMMAND* p,int num,bool hackprotect)
{
    if (c->stfu) return SUBCOMMAND_ERROR_ABSORBED;
    if (!hackprotect) return 0;
    if (p->byte[1] != num) return SUBCOMMAND_ERROR_SIZE_MISMATCH;
    if (p->byte[1] < 2) return SUBCOMMAND_ERROR_SIZE_MISMATCH;
    if (p[1].byte[0] != c->clientID) return SUBCOMMAND_ERROR_IMPERSONATION;
    return 0;
}

// client sends a word select chat
int ProcessSubcommand_WordSelect(SERVER*,LOBBY*,CLIENT* c,bool,int,SUBCOMMAND* p,int num,bool hackprotect)
{
    int x;
    if (c->stfu) return SUBCOMMAND_ERROR_ABSORBED;
    if (!hackprotect) return 0;
    if (p->byte[1] != num) return SUBCOMMAND_ERROR_SIZE_MISMATCH;
    if (p->byte[1] < 8) return SUBCOMMAND_ERROR_SIZE_MISMATCH;
    p->byte[2] = p->byte[3] = p->byte[(c->version == VERSION_BLUEBURST) ? 2 : 3];
    if (p->byte[2] != c->clientID) return SUBCOMMAND_ERROR_IMPERSONATION;
    for (x = 1; x < 8; x++)
    {
        if ((p[x].word[0] > 0x1863) && (p[x].word[0] != 0xFFFF)) return SUBCOMMAND_ERROR_INVALID_COMMAND;
        if ((p[x].word[1] > 0x1863) && (p[x].word[1] != 0xFFFF)) return SUBCOMMAND_ERROR_INVALID_COMMAND;
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Game commands used by cheat mechanisms

// need to process changing areas since we keep track of where players are
int ProcessSubcommand_ChangeArea(SERVER*,LOBBY* l,CLIENT* c,bool,int,SUBCOMMAND* p,int num,bool hackprotect)
{
    c->area = p[1].dword;
    return 0;
}

// when a player is hit by a monster, heal them if infinite HP is enabled
int ProcessSubcommand_HitByMonster(SERVER*,LOBBY* l,CLIENT* c,bool,int,SUBCOMMAND* p,int num,bool hackprotect)
{
    if ((l->flags & LOBBY_FLAG_CHEAT) && c->infhp) CommandPlayerStatsModify(l,c,HP_ADD,1020);
    return 0;
}

// when a player casts a tech, restore TP if infinite TP is enabled
int ProcessSubcommand_UsedTech(SERVER*,LOBBY* l,CLIENT* c,bool,int,SUBCOMMAND* p,int num,bool hackprotect)
{
    if ((l->flags & LOBBY_FLAG_CHEAT) && c->inftp) CommandPlayerStatsModify(l,c,TP_ADD,255);
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// BB Item commands 

// player drops an item
int ProcessSubcommand_DropItem(SERVER*,LOBBY* l,CLIENT* c,bool,int,COMMAND_BB_SUBCOMMAND_2A* p,int num,bool hackprotect)
{
    if (l->version != VERSION_BLUEBURST) return 0;
    if (p->size != 6) return 84935;

    int error;
    PLAYER_ITEM item;
    operation_lock(c);
    error = RemoveItem(&c->playerInfo,p->itemID,0,&item);
    operation_unlock(c);
    if (error) return error;

    operation_lock(l);
    error = AddItem(l,&item);
    operation_unlock(l);
    return error;
}

// player splits a stack and drops part of it
int ProcessSubcommand_DropStackItem(SERVER*,LOBBY* l,CLIENT* c,bool,int,COMMAND_BB_SUBCOMMAND_C3* p,int num,bool hackprotect)
{
    if (l->version != VERSION_BLUEBURST) return 0;
    if (p->size != 6) return 84935;

    int error;
    PLAYER_ITEM item;
    operation_lock(c);
    error = RemoveItem(&c->playerInfo,p->itemID,p->amount,&item);
    operation_unlock(c);
    if (error == ERROR_COMBINE_ITEM_SPLIT)
    {
        item.data.itemID = GetNextItemID(l,c->clientID);
        error = 0;
    }
    if (error) return error;

    operation_lock(l);
    error = AddItem(l,&item);
    operation_unlock(l);
    if (error) return error;

    error = CommandDropStackedItem(l,c,&item.data,0,p->x,p->y);
    if (error) return error;
    return SUBCOMMAND_ERROR_ABSORBED;
}

// player requests to pick up an item
int ProcessSubcommand_PickUpItem(SERVER*,LOBBY* l,CLIENT* c,bool,int,COMMAND_BB_SUBCOMMAND_5A* p,int num,bool hackprotect)
{
    if (l->version != VERSION_BLUEBURST) return 0;
    if (p->size != 3) return 84935;

    int error;
    PLAYER_ITEM item;
    operation_lock(l);
    error = RemoveItem(l,p->itemID,&item);
    operation_unlock(l);
    if (error) return error;

    operation_lock(c);
    error = AddItem(&c->playerInfo,&item);
    operation_unlock(c);
    if (error) return error;

    error = CommandPickUpItem(l,c,p->itemID,p->area);
    if (error) return error;
    return SUBCOMMAND_ERROR_ABSORBED;
}

// player equips an item
int ProcessSubcommand_EquipItem(SERVER*,LOBBY* l,CLIENT* c,bool,int,COMMAND_BB_SUBCOMMAND_25_26_27_63* p,int num,bool hackprotect)
{
    if (l->version != VERSION_BLUEBURST) return 0;
    if (p->size != 3) return 84935;

    unsigned int index;
    operation_lock(c);
    index = FindItem(&c->playerInfo.inventory,p->itemID);
    if (index == 0xFFFFFFFF)
    {
        operation_unlock(c);
        return 48395;
    }
    c->playerInfo.inventory.items[index].gameFlags |= 0x00000008;
    operation_unlock(c);
    return 0;
}

// player unequips an item
int ProcessSubcommand_UnequipItem(SERVER*,LOBBY* l,CLIENT* c,bool,int,COMMAND_BB_SUBCOMMAND_25_26_27_63* p,int num,bool hackprotect)
{
    if (l->version != VERSION_BLUEBURST) return 0;
    if (p->size != 3) return 84935;

    unsigned int index;
    operation_lock(c);
    index = FindItem(&c->playerInfo.inventory,p->itemID);
    if (index == 0xFFFFFFFF)
    {
        operation_unlock(c);
        return 48396;
    }
    c->playerInfo.inventory.items[index].gameFlags &= 0xFFFFFFF7;
    operation_unlock(c);
    return 0;
}

// player uses an item (see ClientUseItem for specific item handlers)
int ProcessSubcommand_UseItem(SERVER*,LOBBY* l,CLIENT* c,bool,int,COMMAND_BB_SUBCOMMAND_25_26_27_63* p,int num,bool hackprotect)
{
    if (l->version != VERSION_BLUEBURST) return 0;
    if (p->size != 2) return 84935;

    long index = FindItem(&c->playerInfo.inventory,p->itemID);
    if (index == (-1)) return 84936;
    long errors = ClientUseItem(l,c,&c->playerInfo.inventory.items[index]);

    if (!errors)
    {
        operation_lock(c);
        errors = RemoveItem(&c->playerInfo,p->itemID,1,NULL);
        operation_unlock(c);
    } else if (errors == ITEM_ERROR_DO_NOT_DELETE) errors = 0;
    return errors;
}

// player opens the bank window
int ProcessSubcommand_OpenBank(SERVER*,LOBBY* l,CLIENT* c,bool,int,void*,int,bool)
{
    if (l->version != VERSION_BLUEBURST) return 89534;
    int err = CommandBank(c);
    if (err) return err;
    return SUBCOMMAND_ERROR_ABSORBED;
}

// player performs some bank action
int ProcessSubcommand_BankAction(SERVER*,LOBBY* l,CLIENT* c,bool,int,COMMAND_BB_SUBCOMMAND_BD* p,int num,bool hackprotect)
{
    PLAYER_ITEM item;
    PLAYER_BANK_ITEM bankitem;

    if (l->version != VERSION_BLUEBURST) return 89534;

    int rv = 0;
    operation_lock(c);
    if (p->action == 0) // deposit 
    {
        if (p->itemID == 0xFFFFFFFF) // meseta 
        {
            if (p->amount > c->playerInfo.disp.meseta) return 543265423;
            if ((c->playerInfo.bank.meseta + p->amount) > 999999) return 543265424;
            c->playerInfo.bank.meseta += p->amount;
            c->playerInfo.disp.meseta -= p->amount;
        } else { // item 
            rv = RemoveItem(&c->playerInfo,p->itemID,p->itemAmount,&item);
            if (rv == ERROR_COMBINE_ITEM_SPLIT) rv = 0;
            if (rv)
            {
                operation_unlock(c);
                return rv;
            }
            ConvertToBankItem(&bankitem,&item);
            rv = AddItem(&c->playerInfo.bank,&bankitem);
            if (rv)
            {
                operation_unlock(c);
                return rv;
            }
            rv = CommandDestroyItem(l,c,p->itemID,p->amount);
        }
    } else if (p->action == 1) // take 
    {
        if (p->itemID == 0xFFFFFFFF) // meseta 
        {
            if (p->amount > c->playerInfo.bank.meseta) return 543265425;
            if ((c->playerInfo.disp.meseta + p->amount) > 999999) return 543265426;
            c->playerInfo.bank.meseta -= p->amount;
            c->playerInfo.disp.meseta += p->amount;
        } else { // item 
            rv = RemoveItem(&c->playerInfo.bank,p->itemID,p->itemAmount,&bankitem);
            if (rv == ERROR_COMBINE_ITEM_SPLIT) rv = 0;
            if (rv)
            {
                operation_unlock(c);
                return rv;
            }
            ConvertToInventoryItem(&item,&bankitem);
            item.data.itemID = GetNextItemID(l,0xFFFFFFFF);
            rv = AddItem(&c->playerInfo,&item);
            if (rv)
            {
                operation_unlock(c);
                return rv;
            }
            rv = CommandCreateItem(l,c,&item.data);
        }
    }
    operation_unlock(c);
    if (rv) return rv;
    return SUBCOMMAND_ERROR_ABSORBED;
}

// player sorts the items in their inventory
int ProcessSubcommand_SortItems(SERVER*,LOBBY* l,CLIENT* c,bool,int,COMMAND_BB_SUBCOMMAND_C4* p,int num,bool hackprotect)
{
    if (l->version != VERSION_BLUEBURST) return 84394;
    if (p->size != 0x1F) return 84935;

    int rv = ProcedureSortClientItems(c,p->itemIDs);
    if (rv) return rv;
    return SUBCOMMAND_ERROR_ABSORBED;
}

////////////////////////////////////////////////////////////////////////////////
// BB EXP/Drop Item commands 

// enemy killed; leader sends drop item request
int ProcessSubcommand_DudeDropItem(SERVER*,LOBBY* l,CLIENT* c,bool,int,COMMAND_BB_SUBCOMMAND_60* p,int num,bool hackprotect)
{
    if (l->version != VERSION_BLUEBURST) return 0;

    PLAYER_ITEM item;

    bool isRareItem = false;
    bool drop = true;
    if (l->item.data.itemData1dword[0])
    {
        memcpy(&item,&l->item,sizeof(PLAYER_ITEM));
        l->item.data.itemData1dword[0] = 0;
    } else {
        if (l->rareSet)
        {
            if (p->monsterID <= 0x65)
            {
                if (DecideIfItemIsRare(l->rareSet->rares[p->monsterID].probability)) isRareItem = true;
            }
        }
        if (isRareItem)
        {
            memset(&item,0,sizeof(PLAYER_ITEM));
            memcpy(item.data.itemData1,l->rareSet->rares[p->monsterID].itemCode,3);
            //RandPercentages();
            if (item.data.itemData1[0] == 0) item.data.itemData1[4] |= 0x80; // make it untekked, if it's a weapon
        } else {
            memset(&item,0,sizeof(PLAYER_ITEM));
            drop = CreateNonRareItem(&item.data,false,l->episode,l->difficulty,p->area,l->sectionID);
            //RandPercentages();
        }
    }
    if (!drop) return SUBCOMMAND_ERROR_ABSORBED;
    item.data.itemID = GetNextItemID(l,0xFFFFFFFF);

    int rv = AddItem(l,&item);
    if (rv) return rv;

    rv = CommandDropItem(l,&item.data,false,p->area,p->x,p->y,p->requestID);
    if (rv) return rv;
    return SUBCOMMAND_ERROR_ABSORBED;
}

// box broken; leader sends drop item request
int ProcessSubcommand_BoxDropItem(SERVER*,LOBBY* l,CLIENT* c,bool,int,COMMAND_BB_SUBCOMMAND_A2* p,int num,bool hackprotect)
{
    if (l->version != VERSION_BLUEBURST) return 0;

    PLAYER_ITEM item;

    long x;
    bool isRareItem = false;
    bool drop = true;
    if (l->item.data.itemData1dword[0])
    {
        memcpy(&item,&l->item,sizeof(PLAYER_ITEM));
        l->item.data.itemData1dword[0] = 0;
    } else {
        if (l->rareSet)
        {
            for (x = 0; x < 30; x++)
            {
                if (l->rareSet->boxAreas[x] != p->area) continue;
                if (DecideIfItemIsRare(l->rareSet->boxRares[x].probability))
                {
                    isRareItem = true;
                    break;
                }
            }
        }
        if (isRareItem)
        {
            CommandTextMessage(NULL,l,NULL,L"$C6Rare item from box!");
            memset(&item,0,sizeof(PLAYER_ITEM));
            memcpy(item.data.itemData1,l->rareSet->boxRares[x].itemCode,3);
            //RandPercentages();
            if (item.data.itemData1[0] == 0) item.data.itemData1[4] |= 0x80; // untekked 
        } else {
            memset(&item,0,sizeof(PLAYER_ITEM));
            drop = CreateNonRareItem(&item.data,true,l->episode,l->difficulty,p->area,l->sectionID);
            //RandPercentages();
        }
    }
    if (!drop) return SUBCOMMAND_ERROR_ABSORBED;
    item.data.itemID = GetNextItemID(l,0xFFFFFFFF);

    int rv = AddItem(l,&item);
    if (rv) return rv;

    rv = CommandDropItem(l,&item.data,false,p->area,p->x,p->y,p->requestID);
    if (rv) return rv;
    return SUBCOMMAND_ERROR_ABSORBED;
}

// monster hit by player
int ProcessSubcommand_MonsterHit(SERVER*,LOBBY* l,CLIENT* c,bool,int,COMMAND_BB_SUBCOMMAND_0A* p,int num,bool hackprotect)
{
    if (l->version != VERSION_BLUEBURST) return 0;
    if (p->enemyID >= l->numEnemies) return 568473;
    if (l->enemies[p->enemyID].hitFlags & 0x80) return 568474;
    //ConsolePrintColor("$0E> > game enemy handler: hitting monster %04X for %d damage\n",p->enemyID,p->damage);
    operation_lock(l);
    l->enemies[p->enemyID].hitFlags |= (1 << c->clientID);
    l->enemies[p->enemyID].lastHit = c->clientID;
    operation_unlock(l);
    return 0;
}

// monster killed by player
int ProcessSubcommand_MonsterKilled(SERVER*,LOBBY* l,CLIENT* c,bool,int,COMMAND_BB_SUBCOMMAND_C8* p,int num,bool hackprotect)
{
    if (l->version != VERSION_BLUEBURST) return 0;
    if (p->enemyID >= l->numEnemies) return 568473;
    if (l->enemies[p->enemyID].hitFlags & 0x80) return 568474;
    //ConsolePrintColor("$0E> > game enemy handler: killing monster %04X for %d EXP\n",p->enemyID,l->enemies[p->enemyID].exp);

    if (l->enemies[p->enemyID].exp == 0xFFFFFFFF) return CommandTextMessage(NULL,l,NULL,L"$C6Unknown enemy type killed");

    unsigned long x,exp;
    LEVEL_INFO* level;
    bool leveledUp;
    operation_lock(l);
    l->enemies[p->enemyID].hitFlags |= 0x80;
    for (x = 0; x < l->maxClients; x++)
    {
        if (!l->clients[x]) continue; // no player 
        if (!((l->enemies[p->enemyID].hitFlags >> x) & 1)) continue; // player did not hit this enemy 
        if (l->clients[x]->playerInfo.disp.level >= 199) continue; // player is level 200 or higher 
        if (l->enemies[p->enemyID].lastHit == l->clients[x]->clientID) exp = l->enemies[p->enemyID].exp;
        else exp = ((l->enemies[p->enemyID].exp * 77) / 100);

        operation_lock(l->clients[x]);
        l->clients[x]->playerInfo.disp.exp += exp;
        CommandGiveEXP(l,l->clients[x],exp);

        leveledUp = false;
        do {
            level = GetLevelInfo(leveltable,l->clients[x]->playerInfo.disp.charClass,l->clients[x]->playerInfo.disp.level + 1);
            if (l->clients[x]->playerInfo.disp.exp >= level->exp)
            {
                leveledUp = true;
                ApplyLevelInfoToPlayerStats(level,&l->clients[x]->playerInfo.disp.stats);
                l->clients[x]->playerInfo.disp.level++;
            }
        } while ((l->clients[x]->playerInfo.disp.exp >= level->exp) && (l->clients[x]->playerInfo.disp.level < 199));
        if (leveledUp) CommandLevelUp(l,c);
        operation_unlock(l->clients[x]);
    }
    operation_unlock(l);
    return 0;
}

// destroy item (sent when there are too many items on the ground)
int ProcessSubcommand_DestroyItem(SERVER*,LOBBY* l,CLIENT* c,bool,int,COMMAND_BB_SUBCOMMAND_25_26_27_63* p,int num,bool hackprotect)
{
    if (l->version != VERSION_BLUEBURST) return 0;

    operation_lock(l);
    int rv = RemoveItem(l,p->itemID,NULL);
    operation_unlock(l);
    return rv;
}

// player requests to tekk an items
int ProcessSubcommand_IdentifyItem(SERVER*,LOBBY* l,CLIENT* c,bool,int,COMMAND_BB_SUBCOMMAND_25_26_27_63* p,int num,bool hackprotect)
{
    if (l->version != VERSION_BLUEBURST) return 0;

    unsigned long x;
    x = FindItem(&c->playerInfo.inventory,p->itemID);
    if (x == 0xFFFFFFFF) return 4895364;
    if (c->playerInfo.inventory.items[x].data.itemData1[0] != 0) return 4895365;

    c->playerInfo.disp.meseta -= 100;
    memcpy(&c->playerInfo.identifyResult,&c->playerInfo.inventory.items[x],sizeof(PLAYER_ITEM));
    c->playerInfo.identifyResult.data.itemData1[4] &= 0x7F;
    SUBCOMMAND k[6];
    k[0].byte[0] = 0xB9;
    k[0].byte[1] = 0x06;
    k[0].word[1] = c->clientID;
    memcpy(&k[1],&c->playerInfo.identifyResult.data,sizeof(ITEM_DATA));
    int rv = SendSubcommandToLobby(l,NULL,0x60,0x00,k,0x18);
    if (rv) return rv;
    return SUBCOMMAND_ERROR_ABSORBED;
}

// player accepts the tekk
int ProcessSubcommand_AcceptIdentifiedItem(SERVER*,LOBBY* l,CLIENT* c,bool,int,COMMAND_BB_SUBCOMMAND_25_26_27_63* p,int num,bool hackprotect)
{
    if (l->version != VERSION_BLUEBURST) return 0;

    unsigned long x;
    x = FindItem(&c->playerInfo.inventory,p->itemID);
    if (x == 0xFFFFFFFF) return 4895364;
    memcpy(&c->playerInfo.inventory.items[x],&c->playerInfo.identifyResult,sizeof(ITEM_DATA));
    // HERE! what do we send to the other clients? 
    return SUBCOMMAND_ERROR_ABSORBED;
}

////////////////////////////////////////////////////////////////////////////////

// used for commands that are allowed, but the server takes no action on
int ProcessSubcommandIgnored(SERVER*,LOBBY*,CLIENT*,bool,int,SUBCOMMAND*,int,bool) { return 0; }

// used for commands that the server takes no action on and that should not be echoed to other players in the game/lobby
int ProcessSubcommandAbsorbed(SERVER*,LOBBY*,CLIENT*,bool,int,SUBCOMMAND*,int,bool) { return SUBCOMMAND_ERROR_ABSORBED; }

// used for invalid commands. normally, clients should be disconnected - to restore this behavior, change the return value back to SUBCOMMAND_ERROR_INVALID_COMMAND.
int ProcessSubcommandInvalid(SERVER*,LOBBY*,CLIENT*,bool priv,int target,SUBCOMMAND* p,int num,bool)
{
    if (priv) ConsolePrintColor("$0C> > Invalid subcommand: %02X (%d of them) (private to player %d)\n",p->byte[0],num,target);
    else ConsolePrintColor("$0C> > Invalid subcommand: %02X (%d of them)\n",p->byte[0],num);
    return 0;//SUBCOMMAND_ERROR_INVALID_COMMAND;
}

// used when an error occurs (unknown commands, etc). normally, clients should be disconnected - to restore this behavior, change the return value back to SUBCOMMAND_ERROR_INVALID_COMMAND.
int ProcessSubcommandErrorHandler(SERVER*,LOBBY*,CLIENT*,bool priv,int target,SUBCOMMAND* p,int num,bool)
{
    if (priv) ConsolePrintColor("$0C> > Unknown subcommand: %02X (%d of them) (private to player %d)\n",p->byte[0],num,target);
    else ConsolePrintColor("$0C> > Unknown subcommand: %02X (%d of them)\n",p->byte[0],num);
    return 0;//SUBCOMMAND_ERROR_INVALID_COMMAND;
}

////////////////////////////////////////////////////////////////////////////////

// Subcommands are described by four fields: the minimum size and maximum size (in DWORDs),
// the handler function, and flags that tell when to allow the command. See command-input-subs.h
// for more information on flags. The maximum size is not enforced if it's zero.
SUBCOMMAND_DESCRIPTOR subcommands[0x100] = {
    {0x00,0x00,(void*)ProcessSubcommandInvalid}, // 00 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 01 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 02 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 03 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 04 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_GAME  }, // 05 
    {0x00,0x00,(void*)ProcessSubcommand_SendGuildCard,SUBCOMMAND_CHECK_SIZE       }, // 06 
    {0x11,0x11,(void*)ProcessSubcommand_SymbolChat   ,SUBCOMMAND_CHECK_SIZE       }, // 07 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 08 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 09 
    {0x03,0x03,(void*)ProcessSubcommand_MonsterHit   ,SUBCOMMAND_CHECK_SIZE_GAME  }, // 0A 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_GAME  }, // 0B 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_GAME  }, // 0C 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE       }, // 0D 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 0E 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 0F 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 10 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 11 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_GAME  }, // 12 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_GAME  }, // 13 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_GAME  }, // 14 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_GAME  }, // 15 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 16 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_GAME  }, // 17 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_GAME  }, // 18 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_GAME  }, // 19 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 1A 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 1B 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 1C 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 1D 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 1E 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE       }, // 1F 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE       }, // 20 
    {0x02,0x02,(void*)ProcessSubcommand_ChangeArea   ,SUBCOMMAND_CHECK_SIZE_GAME  }, // 21 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_CLIENT}, // 22 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_CLIENT}, // 23 
    {0x05,0x05,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE       }, // 24 
    {0x00,0x00,(void*)ProcessSubcommand_EquipItem    ,SUBCOMMAND_CHECK_SIZE_CLIENT}, // 25 
    {0x00,0x00,(void*)ProcessSubcommand_UnequipItem  ,SUBCOMMAND_CHECK_SIZE_CLIENT}, // 26 
    {0x02,0x02,(void*)ProcessSubcommand_UseItem      ,SUBCOMMAND_CHECK_SIZE_CLIENT}, // 27 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 28 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_GAME  }, // 29 
    {0x00,0x00,(void*)ProcessSubcommand_DropItem     ,SUBCOMMAND_CHECK_SIZE_GAME  }, // 2A 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 2B 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE       }, // 2C 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_GAME  }, // 2D 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 2E 
    {0x00,0x00,(void*)ProcessSubcommand_HitByMonster ,SUBCOMMAND_CHECK_SIZE_CLIENT}, // 2F 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 30 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_GAME  }, // 31 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_GAME  }, // 32 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 33 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 34 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 35 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_GAME_ONLY        }, // 36 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 37 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 38 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 39 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_GAME  }, // 3A 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE       }, // 3B 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 3C 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 3D 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE       }, // 3E 
    {0x06,0x06,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE       }, // 3F 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE       }, // 40 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 41 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE       }, // 42 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_CLIENT}, // 43 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_CLIENT}, // 44 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_CLIENT}, // 45 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_CLIENT}, // 46 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_CLIENT}, // 47 
    {0x00,0x00,(void*)ProcessSubcommand_UsedTech     ,SUBCOMMAND_CHECK_SIZE_CLIENT}, // 48 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_CLIENT}, // 49 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_CLIENT}, // 4A 
    {0x00,0x00,(void*)ProcessSubcommand_HitByMonster ,SUBCOMMAND_CHECK_SIZE_CLIENT}, // 4B 
    {0x00,0x00,(void*)ProcessSubcommand_HitByMonster ,SUBCOMMAND_CHECK_SIZE_CLIENT}, // 4C 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_CLIENT}, // 4D 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_CLIENT}, // 4E 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_CLIENT}, // 4F 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_CLIENT}, // 50 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 51 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE       }, // 52 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_GAME  }, // 53 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 54 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_CLIENT}, // 55 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_CLIENT}, // 56 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_CLIENT}, // 57 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 58 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_GAME  }, // 59 
    {0x00,0x00,(void*)ProcessSubcommand_PickUpItem   ,SUBCOMMAND_CHECK_SIZE_GAME  }, // 5A 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 5B 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 5C 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 5D 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 5E 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 5F 
    {0x06,0x06,(void*)ProcessSubcommand_DudeDropItem ,SUBCOMMAND_CHECK_SIZE_GAME  }, // 60 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 61 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 62 
    {0x03,0x03,(void*)ProcessSubcommand_DestroyItem  ,SUBCOMMAND_CHECK_SIZE_GAME  }, // 63 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 64 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 65 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 66 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_GAME  }, // 67 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_GAME  }, // 68 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 69 
    {0x02,0x02,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_GAME  }, // 6A 
    {0x05,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_GAME_LOADING_ONLY}, // 6B 
    {0x05,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_GAME_LOADING_ONLY}, // 6C 
    {0x05,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_GAME_LOADING_ONLY}, // 6D 
    {0x05,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_GAME_LOADING_ONLY}, // 6E 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 6F 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 70 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_GAME_LOADING_ONLY}, // 71 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 72 
    {0x00,0x00,(void*)ProcessSubcommandInvalid       ,SUBCOMMAND_ALLOW_ALL        }, // 73 
    {0x00,0x00,(void*)ProcessSubcommand_WordSelect   ,SUBCOMMAND_CHECK_SIZE       }, // 74 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_GAME  }, // 75 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_GAME  }, // 76 
    {0x03,0x03,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE       }, // 77 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 78 
    {0x06,0x06,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE       }, // 79 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 7A 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 7B 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 7C 
    {0x06,0x06,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_GAME  }, // 7D 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 7E 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 7F 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 80 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 81 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 82 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 83 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_GAME  }, // 84 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_GAME  }, // 85 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 86 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 87 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_GAME  }, // 88 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_GAME  }, // 89 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 8A 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 8B 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 8C 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_CLIENT}, // 8D 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 8E 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 8F 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 90 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 91 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 92 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 93 
    {0x02,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 94 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 95 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 96 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 97 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 98 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 99 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 9A 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 9B 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 9C 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 9D 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 9E 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // 9F 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // A0 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // A1 
    {0x0A,0x0A,(void*)ProcessSubcommand_BoxDropItem  ,SUBCOMMAND_CHECK_SIZE_GAME  }, // A2 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // A3 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // A4 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // A5 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // A6 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // A7 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // A8 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // A9 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // AA 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_CLIENT}, // AB 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // AC 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // AD 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_CLIENT}, // AE 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_CLIENT}, // AF 
    {0x00,0x00,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_CLIENT}, // B0 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // B1 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // B2 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // B3 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // B4 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // B5 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // B6 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // B7 
    {0x02,0x02,(void*)ProcessSubcommand_IdentifyItem ,SUBCOMMAND_CHECK_SIZE_GAME  }, // B8 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // B9 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // BA 
    {0x00,0x00,(void*)ProcessSubcommand_OpenBank     ,SUBCOMMAND_GAME_ONLY        }, // BB 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // BC 
    {0x04,0x00,(void*)ProcessSubcommand_BankAction   ,SUBCOMMAND_CHECK_SIZE_GAME  }, // BD 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // BE 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // BF 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // C0 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // C1 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // C2 
    {0x00,0x00,(void*)ProcessSubcommand_DropStackItem,SUBCOMMAND_CHECK_SIZE_GAME  }, // C3 
    {0x00,0x00,(void*)ProcessSubcommand_SortItems    ,SUBCOMMAND_CHECK_SIZE_GAME  }, // C4 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // C5 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // C6 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // C7 
    {0x03,0x03,(void*)ProcessSubcommand_MonsterKilled,SUBCOMMAND_CHECK_SIZE_GAME  }, // C8 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // C9 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // CA 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // CB 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // CC 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // CD 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // CE 
    {0x0D,0x0D,(void*)ProcessSubcommandIgnored       ,SUBCOMMAND_CHECK_SIZE_GAME  }, // CF 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // D0 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // D1 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // D2 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // D3 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // D4 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // D5 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // D6 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // D7 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // D8 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // D9 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // DA 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // DB 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // DC 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // DD 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // DE 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // DF 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // E0 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // E1 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // E2 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // E3 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // E4 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // E5 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // E6 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // E7 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // E8 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // E9 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // EA 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // EB 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // EC 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // ED 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // EE 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // EF 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // F0 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // F1 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // F2 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // F3 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // F4 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // F5 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // F6 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // F7 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // F8 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // F9 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // FA 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // FB 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // FC 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // FD 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }, // FE 
    {0x00,0x00,NULL                                  ,SUBCOMMAND_ALLOW_ALL        }  // FF 
};

// error-checks a subcommand's structure and calls the appropriate handler.
int ProcessSubcommand(SERVER* s,LOBBY* l,CLIENT* c,bool priv,int target,SUBCOMMAND* p,unsigned int num,bool hackprotect)
{
    int error;
    int cmd = p->byte[0] & 0xFF;

    if (hackprotect)
    {
        if ((subcommands[cmd].flags & SUBCOMMAND_CHECK_SIZE) && (p->byte[1] != num)) return SUBCOMMAND_ERROR_SIZE_MISMATCH;
        if ((subcommands[cmd].flags & SUBCOMMAND_CHECK_CLIENT) && !(l->flags & LOBBY_FLAG_GAME) && (p->byte[2] != c->clientID)) return SUBCOMMAND_ERROR_IMPERSONATION;
        if ((subcommands[cmd].flags & SUBCOMMAND_GAME_ONLY) && !(l->flags & LOBBY_FLAG_GAME)) return SUBCOMMAND_ERROR_INVALID_COMMAND;
        if ((subcommands[cmd].flags & SUBCOMMAND_LOBBY_ONLY) && (l->flags & LOBBY_FLAG_GAME)) return SUBCOMMAND_ERROR_INVALID_COMMAND;
        if ((subcommands[cmd].flags & SUBCOMMAND_GAME_LOADING) && (!CheckLoadingPlayers(l))) return SUBCOMMAND_ERROR_INVALID_COMMAND;
    }

    if (num < subcommands[cmd].minSize) return SUBCOMMAND_ERROR_SIZE_MISMATCH;
    if (subcommands[cmd].maxSize && (num > subcommands[cmd].maxSize)) return SUBCOMMAND_ERROR_SIZE_MISMATCH;
    if (subcommands[cmd].function) error = ((int (*)(SERVER*,LOBBY*,CLIENT*,bool,int,SUBCOMMAND*,int,bool))subcommands[cmd].function)(s,l,c,priv,target,p,num,hackprotect);
    else error = ProcessSubcommandErrorHandler(s,l,c,priv,target,p,num,hackprotect);
    return error;
}

