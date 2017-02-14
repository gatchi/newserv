#include <windows.h>
#include <stdio.h>

#include "operation.h"
#include "console.h"
#include "text.h"
#include "netconfig.h"

#include "encryption.h"

#include "player.h"
#include "quest.h"
#include "battleparamentry.h"
#include "itemraretable.h"
#include "map.h"

#include "license.h"
#include "client.h"

#include "listenthread.h"
#include "lobby.h"

#include "items.h"

extern CFGFile* config;

////////////////////////////////////////////////////////////////////////////////

// called when a player uses a tech disk.
int ClientUseItem_TechDisk(LOBBY* l,CLIENT* c,PLAYER_ITEM* item,long eqWeapon,long eqArmor,long eqShield,long eqMag)
{
    c->playerInfo.disp.techLevels[item->data.itemData1[4]] = item->data.itemData1[2];
    return 0;
}

// called when a player uses a material. 
int ClientUseItem_Material(LOBBY* l,CLIENT* c,PLAYER_ITEM* item,long eqWeapon,long eqArmor,long eqShield,long eqMag)
{
    switch (item->data.itemData1[2])
    {
      case 0: // Power Material 
        c->playerInfo.disp.stats.atp += 2;
        break;
      case 1: // Mind Material 
        c->playerInfo.disp.stats.mst += 2;
        break;
      case 2: // Evade Material 
        c->playerInfo.disp.stats.evp += 2;
        break;
      case 3: // HP Material 
        c->playerInfo.inventory.hpMat += 2;
        break;
      case 4: // TP Material 
        c->playerInfo.inventory.tpMat += 2;
        break;
      case 5: // Def Material 
        c->playerInfo.disp.stats.dfp += 2;
        break;
      case 6: // Luck Material 
        c->playerInfo.disp.stats.lck += 2;
        break;
      default:
        return 5743;
    }
    return 0;
}

// called when a player uses a grinder.
int ClientUseItem_Grinder(LOBBY* l,CLIENT* c,PLAYER_ITEM* item,long eqWeapon,long eqArmor,long eqShield,long eqMag)
{
    if (eqWeapon < 0) return 57849365;
    if ((item->data.itemData1[2] > 2) && (l->flags & LOBBY_FLAG_HACKPROTECT)) return 5473;
    c->playerInfo.inventory.items[eqWeapon].data.itemData1[3] += (item->data.itemData1[2] + 1);
    // CheckMaxGrind() 
    return 0;
}

/* these items all need some kind of special handling that hasn't been implemented yet.

030B04 = TP Material (?)
030C00 = Cell Of MAG 502
030C01 = Cell Of MAG 213
030C02 = Parts Of RoboChao 
030C03 = Heart Of Opa Opa 
030C04 = Heart Of Pian 
030C05 = Heart Of Chao 

030D00 = Sorcerer's Right Arm
030D01 = S-beat's Arms 
030D02 = P-arm's Arms 
030D03 = Delsabre's Right Arm 
030D04 = C-bringer's Right Arm 
030D05 = Delsabre's Left Arm 
030D06 = S-red's Arms 
030D07 = Dragon's Claw 
030D08 = Hildebear's Head 
030D09 = Hildeblue's Head 
030D0A = Parts of Baranz 
030D0B = Belra's Right Arms
030D0C = GIGUE'S ARMS
030D0D = S-BERILL'S ARMS
030D0E = G-ASSASIN'S ARMS
030D0F = BOOMA'S RIGHT ARMS
030D10 = GOBOOMA'S RIGHT ARMS
030D11 = GIGOBOOMA'S RIGHT ARMS
030D12 = GAL WIND
030D13 = RAPPY'S WING

030E00 = BERILL PHOTON
030E01 = PARASITIC GENE FLOW
030E02 = MAGICSTONE IRITISTA
030E03 = BLUE BLACK STONE 
030E04 = SYNCESTA 
030E05 = MAGIC WATER
030E06 = PARASITIC CELL TYPE D
030E07 = MAGIC ROCK HEART KEY 
030E08 = MAGIC ROCK MOOLA
030E09 = STAR AMPLIFIER
030E0A = BOOK OF HITOGATA
030E0B = HEART OF CHU CHU
030E0C = PART OF EGG BLASTER
030E0D = HEART OF ANGLE
030E0E = HEART OF DEVIL
030E0F = KIT OF HAMBERGER
030E10 = PANTHER'S SPIRIT
030E11 = KIT OF MARK3
030E12 = KIT OF MASTER SYSTEM
030E13 = KIT OF GENESIS
030E14 = KIT OF SEGA SATURN
030E15 = KIT OF DREAMCAST
030E16 = AMP. RESTA
030E17 = AMP. ANTI
030E18 = AMP. SHIFTA
030E19 = AMP. DEBAND
030E1A = AMP.
030E1B = AMP.
030E1C = AMP.
030E1D = AMP.
030E1E = AMP.
030E1F = AMP.
030E20 = AMP.
030E21 = AMP.
030E22 = AMP.
030E23 = AMP.
030E24 = AMP.
030E25 = AMP.
030E26 = HEART OF KAPUKAPU
030E27 = PROTON BOOSTER
030F00 = ADD SLOT
031000 = PHOTON DROP
031001 = PHOTON SPHERE
031002 = PHOTON CRYSTAL
031100 = BOOK OF KATANA 1
031101 = BOOK OF KATANA 2
031102 = BOOK OF KATANA 3
031200 = WEAPONS BRONZE BADGE
031201 = WEAPONS SILVER BADGE
031202 = WEAPONS GOLD BADGE
031203 = WEAPONS CRYSTAL BADGE
031204 = WEAPONS STEEL BADGE
031205 = WEAPONS ALUMINUM BADGE
031206 = WEAPONS LEATHER BADGE
031207 = WEAPONS BONE BADGE
031208 = LETTER OF APPRECATION
031209 = AUTOGRAPH ALBUM
03120A = VALENTINE'S CHOCOLATE 
03120B = NEWYEAR'S CARD
03120C = CRISMAS CARD
03120D = BIRTHDAY CARD
03120E = PROOF OF SONIC TEAM
03120F = SPECIAL EVENT TICKET
031300 = PRESENT
031400 = CHOCOLATE
031401 = CANDY
031402 = CAKE
031403 = SILVER BADGE
031404 = GOLD BADGE
031405 = CRYSTAL BADGE
031406 = IRON BADGE
031407 = ALUMINUM BADGE
031408 = LEATHER BADGE
031409 = BONE BADGE
03140A = BONQUET
03140B = DECOCTION
031500 = CRISMAS PRESENT
031501 = EASTER EGG
031502 = JACK-O'S-LANTERN
031700 = HUNTERS REPORT
031701 = HUNTERS REPORT RANK A
031702 = HUNTERS REPORT RANK B
031703 = HUNTERS REPORT RANK C
031704 = HUNTERS REPORT RANK F
031705 = HUNTERS REPORT
031705 = HUNTERS REPORT
031705 = HUNTERS REPORT
031705 = HUNTERS REPORT
031705 = HUNTERS REPORT
031705 = HUNTERS REPORT
031705 = HUNTERS REPORT
031705 = HUNTERS REPORT
031705 = HUNTERS REPORT
031802 = Dragon Scale
031803 = Heaven Striker Coat
031807 = Rappys Beak
031802 = Dragon Scale */

// This is called when an unknown item is used. It unwraps the item if it's a present.
int ClientUseItem_DefaultHandler(LOBBY* l,CLIENT* c,PLAYER_ITEM* item,long eqWeapon,long eqArmor,long eqShield,long eqMag)
{
    if ((item->data.itemData1[0] == 2) && (item->data.itemData2[2] & 0x40))
    {
        item->data.itemData2[2] &= 0xBF;
        return ITEM_ERROR_DO_NOT_DELETE;
    }
    if ((item->data.itemData1[0] != 2) && (item->data.itemData1[4] & 0x40))
    {
        item->data.itemData1[4] &= 0xBF;
        return ITEM_ERROR_DO_NOT_DELETE;
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////

// item handler definitions. A handler can be installed by adding an entry to this
// list with the first two bytes of the item code (in reverse order) and the handler function.
// see above for item handler examples.
struct {
    WORD itemcode;
    int (*handler)(LOBBY*,CLIENT*,PLAYER_ITEM*,long,long,long,long);
} ClientUseItem_HandlerDefinitions[] = {
    {0x0003,NULL},
    {0x0103,NULL},
    {0x0203,ClientUseItem_TechDisk},
    {0x0303,NULL},
    {0x0403,NULL},
    {0x0503,NULL},
    {0x0603,NULL},
    {0x0703,NULL},
    {0x0803,NULL},
    {0x0903,NULL},
    {0x0A03,ClientUseItem_Grinder},
    {0x0B03,ClientUseItem_Material},
    {0x1603,NULL},
    {0x0000,NULL},
};

// this function is called when a player uses an item. it determines the equipped
// weapon, armor, shield, and mag, then calls the appropriate item use handler.
int ClientUseItem(LOBBY* l,CLIENT* c,PLAYER_ITEM* item)
{
    long x,y;
    long eqWeapon = 0xFFFFFFFF,eqArmor = 0xFFFFFFFF,eqShield = 0xFFFFFFFF,eqMag = 0xFFFFFFFF;
    for (y = 0; y < c->playerInfo.inventory.numItems; y++)
    {
        if (c->playerInfo.inventory.items[y].equipFlags & 0x0008)
        {
            if (c->playerInfo.inventory.items[y].data.itemData1[0] == 0) eqWeapon = y;
            if ((c->playerInfo.inventory.items[y].data.itemData1[0] == 1) && (c->playerInfo.inventory.items[y].data.itemData1[1] == 1)) eqArmor = y;
            if ((c->playerInfo.inventory.items[y].data.itemData1[0] == 1) && (c->playerInfo.inventory.items[y].data.itemData1[1] == 2)) eqShield = y;
            if (c->playerInfo.inventory.items[y].data.itemData1[0] == 2) eqMag = y;
        }
    }

    for (x = 0; ClientUseItem_HandlerDefinitions[x].itemcode; x++) if (ClientUseItem_HandlerDefinitions[x].itemcode == item->data.itemData1word[0]) break;
    if (!ClientUseItem_HandlerDefinitions[x].itemcode) return ClientUseItem_DefaultHandler(l,c,item,eqWeapon,eqArmor,eqShield,eqMag);
    if (!ClientUseItem_HandlerDefinitions[x].handler) return 0;
    return ClientUseItem_HandlerDefinitions[x].handler(l,c,item,eqWeapon,eqArmor,eqShield,eqMag);
}

////////////////////////////////////////////////////////////////////////////////

// variables that determine how non-rare items drop
DWORD NonRareItemCategory_Enemy[0x10];
DWORD NonRareItemCategory_Box[0x10];
BYTE NonRareUnitTypes[4][0x40];

// reads the non-rare item preferences from the config file.
int SetupNonRareSystem()
{
    memset(NonRareItemCategory_Enemy,0,sizeof(DWORD) * 0x10);
    memset(NonRareItemCategory_Box,0,sizeof(DWORD) * 0x10);
    memset(NonRareUnitTypes,0,sizeof(DWORD) * 0x80);

    long long max_enemy = 0;
    long long max_box = 0;
    char name[0x40];
    long x;
    for (x = 0; x < 0x10; x++)
    {
        sprintf(name,"NonRareItemCategory_Enemy_%02lX",x);
        if (!CFGIsValuePresent(config,name)) NonRareItemCategory_Enemy[x] = 0;
        else NonRareItemCategory_Enemy[x] = CFGGetNumber(config,name);
        max_enemy += NonRareItemCategory_Enemy[x];
        sprintf(name,"NonRareItemCategory_Box_%02lX",x);
        if (!CFGIsValuePresent(config,name)) NonRareItemCategory_Box[x] = 0;
        else NonRareItemCategory_Box[x] = CFGGetNumber(config,name);
        max_box += NonRareItemCategory_Enemy[x];
    }
    for (x = 0; x < 4; x++)
    {
        sprintf(name,"NonRareUnitTypes_%02lX",x);
        memset(NonRareUnitTypes[x],0xFF,0x40);
        if (CFGIsValuePresent(config,name)) tx_read_string_data(CFGGetStringA(config,name),0,NonRareUnitTypes[x],0x40);
    }
    if ((max_enemy > 0x00000000FFFFFFFF) || (max_box > 0x00000000FFFFFFFF)) return 1;
    return 0;
}

// decides what type of item to drop for a monstear death or box open event
int DecideNonRareType(bool box,DWORD det)
{
    int type = (-1);
    if (!det) det = (rand() << 30) ^ (rand() << 15) ^ rand();
    DWORD max = 0x00000000;
    DWORD nextmax;

    while (max < det)
    {
        type++;
        nextmax = (box ? NonRareItemCategory_Box[type] : NonRareItemCategory_Enemy[type]);
        if (!nextmax) return type;
        max += nextmax;
    }
    return type;
}

// generates a non-rare item.
bool CreateNonRareItem(ITEM_DATA* data,bool box,int ep,int diff,int area,int secid)
{
    long x,y;

    // change the area if it's invalid (data for the bosses are actually in other areas)
    memset(data,0,sizeof(ITEM_DATA));
    if (area > 10)
    {
        if (ep == 1)
        {
                 if (area == 11) area = 3; // dragon
            else if (area == 12) area = 6; // de rol le
            else if (area == 13) area = 8; // vol opt
            else if (area == 14) area = 10; // dark falz
            else                 area = 1; // unknown area -> forest 1
        } else if (ep == 2)
        {
                 if (area == 12) area = 9; // gal gryphon
            else if (area == 13) area = 10; // olga flow
            else if (area == 14) area = 3; // barba ray
            else if (area == 15) area = 6; // gol dragon
            else                 area = 10; // tower
        } else if (ep == 3) area = 1;
    }

    // picks a random non-rare item type, then gives it appropriate random stats
    // modify some of the constants in this section to change the system's parameters
    int type = DecideNonRareType(box,0);
    switch (type)
    {
      case 0x00: // material 
        data->itemData1[0] = 0x03;
        data->itemData1[1] = 0x0B;
        data->itemData1[2] = rand() % 7;
        break;
      case 0x01: // equipment 
        switch (rand() % 4)
        {
          case 0x00: // weapon 
            data->itemData1[1] = 1 + (rand() % 12); // random normal class
            data->itemData1[2] = diff + (rand() % 3); // special type
            if ((data->itemData1[1] > 0x09) && (data->itemData1[2] > 0x04)) data->itemData1[2] = 0x04; // no special classes above 4
            data->itemData1[4] = 0x80; // untekked 
            if (data->itemData1[2] < 0x04) data->itemData1[4] |= (rand() % 41); // give a special 
            for (x = 0, y = 0; (x < 5) && (y < 3); x++) // percentages
            {
                if ((rand() % 11) < 2) // 1/11 chance of getting each type of percentage
                {
                    data->itemData1[6 + (y * 2)] = x + 1;
                    data->itemData1[7 + (y * 2)] = (rand() % 11) * 5;
                    y++;
                }
            }
            break;
          case 0x01: // armor 
            data->itemData1[0] = 0x01;
            data->itemData1[1] = 0x01;
            data->itemData1[2] = (6 * diff) + (rand() % ((area / 2) + 2)); // standard type based on difficulty and area
            if (data->itemData1[2] > 0x17) data->itemData1[2] = 0x17; // no standard types above 0x17
            if ((rand() % 10) < 2) // +/-
            {
                data->itemData1[4] = rand() % 6;
                data->itemData1[6] = rand() % 3;
            }
            data->itemData1[5] = rand() % 5; // slots 
            break;
          case 0x02: // shield 
            data->itemData1[0] = 0x01;
            data->itemData1[1] = 0x02;
            data->itemData1[2] = (5 * diff) + (rand() % ((area / 2) + 2)); // standard type based on difficulty and area
            if (data->itemData1[2] > 0x14) data->itemData1[2] = 0x14; // no standard types above 0x14
            if ((rand() % 11) < 2) // +/-
            {
                data->itemData1[4] = rand() % 6;
                data->itemData1[6] = rand() % 6;
            }
            break;
          case 0x03: // unit 
            x = NonRareUnitTypes[diff][rand() % 24]; // random nonrare unit type
            if (x == 0xFF) return false; // 0xFF -> no item drops
            data->itemData1[0] = 0x01;
            data->itemData1[1] = 0x03;
            data->itemData1[2] = x;
            break;
        }
        break;
      case 0x02: // technique 
        data->itemData1[0] = 0x03;
        data->itemData1[1] = 0x02;
        data->itemData1[4] = rand() % 19; // tech type
        if ((data->itemData1[4] != 14) && (data->itemData1[4] != 17)) // if not ryuker or reverser, give it a level
        {
            if (data->itemData1[4] == 16) // if not anti, give it a level between 1 and 30
            {
                if (area > 3) data->itemData1[2] = diff + (rand() % ((area - 1) / 2));
                else data->itemData1[2] = diff + (rand() % 1);
                if (data->itemData1[2] > 6) data->itemData1[2] = 6;
            } else data->itemData1[2] = (5 * diff) + (rand() % ((area * 3) / 2)); // else between 1 and 7
        }
        break;
      case 0x03: // scape doll 
        data->itemData1[0] = 0x03;
        data->itemData1[1] = 0x09;
        data->itemData1[2] = 0x00;
        break;
      case 0x04: // grinder 
        data->itemData1[0] = 0x03;
        data->itemData1[1] = 0x0A;
        data->itemData1[2] = rand() % 3; // mono, di, tri
        break;
      case 0x05: // consumable 
        data->itemData1[0] = 0x03;
        data->itemData1[5] = 0x01;
        switch (rand() % 2)
        {
          case 0x00: // antidote / antiparalysis
            data->itemData1[1] = 6;
            data->itemData1[2] = rand() & 1;
            break;
          case 0x01: // telepipe / trap vision
            data->itemData1[1] = 7 + rand() & 1;
            break;
          case 0x02: // sol / moon / star atomizer
            data->itemData1[1] = 3 + rand() % 3;
            break;
        }
        break;
      case 0x06: // consumable 
        data->itemData1[0] = 0x03;
        data->itemData1[5] = 0x01;
        data->itemData1[1] = rand() & 1; // mate or fluid
        if (diff == 0) data->itemData1[2] = rand() & 1; // only mono and di on normal
        else if (diff == 3) data->itemData1[2] = 1 + rand() & 1; // only di and tri on ultimate
        else data->itemData1[2] = rand() % 3; // else, any of the three
        break;
      case 0x07: // meseta
        data->itemData1[0] = 0x04;
        data->itemData2dword = (90 * diff) + ((rand() % 20) * (area * 2)); // meseta amount
        break;
      default:
        return false;
    }
    return true;
}
