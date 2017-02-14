#include <windows.h>

#include "console.h"

#include "battleparamentry.h"
#include "map.h"

// loads a map (.dat file).
int LoadMapData(GAME_ENEMY_ENTRY* enemies,DWORD* numEnemies,char* filename,DWORD episode,DWORD diff,BATTLE_PARAM* battleParamTable,bool alt_enemies)
{
    DWORD bytesread;
    HANDLE file = CreateFile(filename,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    if (file == INVALID_HANDLE_VALUE) return 0;

    int size = GetFileSize(file,NULL);
    ENEMY_ENTRY* data = (ENEMY_ENTRY*)malloc(size);
    if (!data)
    {
        CloseHandle(file);
        return 0;
    }
    ReadFile(file,data,size,&bytesread,NULL);
    CloseHandle(file);
    int errors = ParseMapData(enemies,numEnemies,episode,diff,battleParamTable,data,size / 0x48,alt_enemies);
    free(data);
    if (errors) return 0;
    return size / 0x48;
}

// fetches the EXP and rare item values for each monster in a loaded map
int ParseMapData(GAME_ENEMY_ENTRY* enemies,DWORD* numEnemies,DWORD episode,DWORD diff,BATTLE_PARAM* battleParamTable,ENEMY_ENTRY* map,DWORD numEntries,bool alt_enemies)
{
    unsigned long x,y;
    for (y = 0; y < numEntries; y++)
    {
        if ((*numEnemies) >= 0xB50) break;
        memset(&enemies[(*numEnemies)],0,sizeof(GAME_ENEMY_ENTRY));
        switch (map[y].base)
        {
          case 0x40: // Hildebear and Hildetorr
            enemies[(*numEnemies)].rt_index = 0x01 + (map[y].skin & 0x01);
            enemies[(*numEnemies)].exp = battleParamTable[0x49 + (map[y].skin & 0x01)].exp;
            break;
          case 0x41: // Rappies
            if (episode == 3) // Del Rappy and Sand Rappy
            {
                enemies[(*numEnemies)].rt_index = 17 + (map[y].skin & 0x01);
                if (alt_enemies) enemies[(*numEnemies)].exp = battleParamTable[0x17 + (map[y].skin & 0x01)].exp;
                else enemies[(*numEnemies)].exp = battleParamTable[0x05 + (map[y].skin & 0x01)].exp;
            } else { // Rag Rappy and Al Rappy (Love for Episode II)
                if (map[y].skin & 0x01)  enemies[(*numEnemies)].rt_index = 0xFF; // No clue what rappy it could be... yet.
                else enemies[(*numEnemies)].rt_index = 5;
                enemies[(*numEnemies)].exp = battleParamTable[0x18 + (map[y].skin & 0x01)].exp;
            }
            break;
          case 0x42: // Monest + 30 Mothmants
            enemies[(*numEnemies)].exp = battleParamTable[0x01].exp;
            enemies[(*numEnemies)].rt_index = 4;
            for (x = 0; x < 30; x++)
            {
                if ((*numEnemies) >= 0xB50) break;
                (*numEnemies)++;
                enemies[(*numEnemies)].rt_index = 3;
                enemies[(*numEnemies)].exp = battleParamTable[0x00].exp;
            }
            break;
          case 0x43: // Savage Wolf and Barbarous Wolf
            enemies[(*numEnemies)].rt_index = 7 + ((map[y].reserved[10] & 0x800000) ? 1 : 0);
            enemies[(*numEnemies)].exp = battleParamTable[0x02 + ((map[y].reserved[10] & 0x800000) ? 1 : 0)].exp;
            break;
          case 0x44: // Booma family
            enemies[(*numEnemies)].rt_index = 9 + (map[y].skin % 3);
            enemies[(*numEnemies)].exp = battleParamTable[0x4B + (map[y].skin % 3)].exp;
            break;
          case 0x60: // Grass Assassin
            enemies[(*numEnemies)].rt_index = 12;
            enemies[(*numEnemies)].exp = battleParamTable[0x4E].exp;
            break;
          case 0x61: // Del Lily, Poison Lily, Nar Lily
            if ((episode == 2) && (alt_enemies))
            {
                enemies[(*numEnemies)].rt_index = 83;
                enemies[(*numEnemies)].exp = battleParamTable[0x25].exp;
            } else {
                enemies[(*numEnemies)].rt_index = 13 + ((map[y].reserved[10] & 0x800000) ? 1 : 0);
                enemies[(*numEnemies)].exp = battleParamTable[0x04 + ((map[y].reserved[10] & 0x800000) ? 1 : 0)].exp;
            }
            break;
          case 0x62: // Nano Dragon
            enemies[(*numEnemies)].rt_index = 15;
            enemies[(*numEnemies)].exp = battleParamTable[0x1A].exp;
            break;
          case 0x63: // Shark family
            enemies[(*numEnemies)].rt_index = 16 + (map[y].skin % 3);
            enemies[(*numEnemies)].exp = battleParamTable[0x4F + (map[y].skin % 3)].exp;
            break;
          case 0x64: // Slime + 4 clones
            enemies[(*numEnemies)].rt_index = 19 + ((map[y].reserved[10] & 0x800000) ? 1 : 0);
            enemies[(*numEnemies)].exp = battleParamTable[0x2F + ((map[y].reserved[10] & 0x800000) ? 0 : 1)].exp;
            for (x = 0; x < 4; x++)
            {
                if ((*numEnemies) >= 0xB50) break;
                (*numEnemies)++;
                enemies[(*numEnemies)].rt_index = 19;
                enemies[(*numEnemies)].exp = battleParamTable[0x30].exp;
            }
            break;
          case 0x65: // Pan Arms, Migium, Hidoom
            for (x = 0; x < 3; x++)
            {
                enemies[(*numEnemies) + x].rt_index = 21 + x;
                enemies[(*numEnemies) + x].exp = battleParamTable[0x31 + x].exp;
            }

            (*numEnemies) += 2;
            break;
          case 0x80: // Dubchic and Gilchic
            enemies[(*numEnemies)].exp = battleParamTable[0x1B + (map[y].skin & 0x01)].exp;
            if (map[y].skin & 0x01) enemies[(*numEnemies)].rt_index = 50;
            else enemies[(*numEnemies)].rt_index = 24;
            break;
          case 0x81: // Garanz
            enemies[(*numEnemies)].rt_index = 25;
            enemies[(*numEnemies)].exp = battleParamTable[0x1D].exp;
            break;
          case 0x82: // Sinow Beat and Gold
            enemies[(*numEnemies)].rt_index = 26 + ((map[y].reserved[10] & 0x800000) ? 1 : 0);
            if (map[y].reserved[10] & 0x800000) enemies[(*numEnemies)].exp = battleParamTable[0x13].exp;
            else enemies[(*numEnemies)].exp = battleParamTable[0x06].exp;
            if (map[y].numClones == 0) map[y].numClones = 4; // only if no clone # present
            break;
          case 0x83: // Canadine
            enemies[(*numEnemies)].rt_index = 28;
            enemies[(*numEnemies)].exp = battleParamTable[0x07].exp;
            break;
          case 0x84: // Canadine Group
            enemies[(*numEnemies)].rt_index = 29;
            enemies[(*numEnemies)].exp = battleParamTable[0x09].exp;
            for (x = 1; x < 9; x++)
            {
                enemies[(*numEnemies) + x].rt_index = 28;
                enemies[(*numEnemies) + x].exp = battleParamTable[0x08].exp;
            }
            (*numEnemies) += 8;
            break;
          case 0x85: // Dubwitch
            break;
          case 0xA0: // Delsaber
            enemies[(*numEnemies)].rt_index = 30;
            enemies[(*numEnemies)].exp = battleParamTable[0x52].exp;
            break;
          case 0xA1: // Chaos Sorcerer + 2 Bits
            enemies[(*numEnemies)].rt_index = 31;
            enemies[(*numEnemies)].exp = battleParamTable[0x0A].exp;
            (*numEnemies) += 2;
            break;
          case 0xA2: // Dark Gunner
            enemies[(*numEnemies)].rt_index = 34;
            enemies[(*numEnemies)].exp = battleParamTable[0x1E].exp;
            break;
          case 0xA4: // Chaos Bringer
            enemies[(*numEnemies)].rt_index = 36;
            enemies[(*numEnemies)].exp = battleParamTable[0x0D].exp;
            break;
          case 0xA5: // Dark Belra
            enemies[(*numEnemies)].rt_index = 37;
            enemies[(*numEnemies)].exp = battleParamTable[0x0E].exp;
            break;
          case 0xA6: // Dimenian family
            enemies[(*numEnemies)].rt_index = 41 + (map[y].skin % 3);
            enemies[(*numEnemies)].exp = battleParamTable[0x53 + (map[y].skin % 3)].exp;
            break;
          case 0xA7: // Bulclaw + 4 claws
            enemies[(*numEnemies)].rt_index = 40;
            enemies[(*numEnemies)].exp = battleParamTable[0x1F].exp;
            for (x = 1; x < 5; x++)
            {
                enemies[(*numEnemies) + x].rt_index = 38;
                enemies[(*numEnemies) + x].exp = battleParamTable[0x20].exp;
            }
            (*numEnemies) += 4;
            break;
          case 0xA8: // Claw
            enemies[(*numEnemies)].rt_index = 38;
            enemies[(*numEnemies)].exp = battleParamTable[0x20].exp;
            break;
          case 0xC0: // Dragon or Gal Gryphon
            if (episode == 1)
            {
                enemies[(*numEnemies)].rt_index = 44;
                enemies[(*numEnemies)].exp = battleParamTable[0x12].exp;
            } else if (episode == 0x02)
            {
                enemies[(*numEnemies)].rt_index = 77;
                enemies[(*numEnemies)].exp = battleParamTable[0x1E].exp;
            }
            break;
          case 0xC1: // De Rol Le
            enemies[(*numEnemies)].rt_index = 45;
            enemies[(*numEnemies)].exp = battleParamTable[0x0F].exp;
            break;
          case 0xC2: // Vol Opt form 1
            break;
          case 0xC5: // Vol Opt form 2
            enemies[(*numEnemies)].rt_index = 46;
            enemies[(*numEnemies)].exp = battleParamTable[0x25].exp;
            break;
          case 0xC8: // Dark Falz + 510 Helpers
            enemies[(*numEnemies)].rt_index = 47;
            if (diff) enemies[(*numEnemies)].exp = battleParamTable[0x38].exp; // Form 2
            else enemies[(*numEnemies)].exp = battleParamTable[0x37].exp;
            for (x = 1; x < 511; x++)
            {
                //enemies[(*numEnemies) + x].base = 200;
                enemies[(*numEnemies) + x].exp = battleParamTable[0x35].exp;
            }
            (*numEnemies) += 510;
            break;
          case 0xCA: // Olga Flow
            enemies[(*numEnemies)].rt_index = 78;
            enemies[(*numEnemies)].exp = battleParamTable[0x2C].exp;
            (*numEnemies) += 512;
            break;
          case 0xCB: // Barba Ray
            enemies[(*numEnemies)].rt_index = 73;
            enemies[(*numEnemies)].exp = battleParamTable[0x0F].exp;
            (*numEnemies) += 47;
            break;
          case 0xCC: // Gol Dragon
            enemies[(*numEnemies)].rt_index = 76;
            enemies[(*numEnemies)].exp = battleParamTable[0x12].exp;
            (*numEnemies) += 5;
            break;
          case 0xD4: // Sinow Berill & Spigell
            enemies[(*numEnemies)].rt_index = 62 + ((map[y].reserved[10] & 0x800000) ? 1 : 0);
            enemies[(*numEnemies)].exp = battleParamTable[(map[y].reserved[10] & 0x800000) ? 0x13 : 0x06].exp;
            (*numEnemies) += 4; // Add 4 clones which are never used...
            break;
          case 0xD5: // Merillia & Meriltas
            enemies[(*numEnemies)].rt_index = 52 + (map[y].skin & 0x01);
            enemies[(*numEnemies)].exp = battleParamTable[0x4B + (map[y].skin & 0x01)].exp;
            break;
          case 0xD6: // Mericus, Merikle, & Mericarol
            enemies[(*numEnemies)].rt_index = 56 + (map[y].skin % 3);
            if (map[y].skin) enemies[(*numEnemies)].exp = battleParamTable[0x44 + (map[y].skin % 3)].exp;
            else enemies[(*numEnemies)].exp = battleParamTable[0x3A].exp;
            break;
          case 0xD7: // Ul Gibbon and Zol Gibbon
            enemies[(*numEnemies)].rt_index = 59 + (map[y].skin & 0x01);
            enemies[(*numEnemies)].exp = battleParamTable[0x3B + (map[y].skin & 0x01)].exp;
            break;
          case 0xD8: // Gibbles
            enemies[(*numEnemies)].rt_index = 61;
            enemies[(*numEnemies)].exp = battleParamTable[0x3D].exp;
            break;
          case 0xD9: // Gee
            enemies[(*numEnemies)].rt_index = 54;
            enemies[(*numEnemies)].exp = battleParamTable[0x07].exp;
            break;
          case 0xDA: // Gi Gue
            enemies[(*numEnemies)].rt_index = 55;
            enemies[(*numEnemies)].exp = battleParamTable[0x1A].exp;
            break;
          case 0xDB: // Deldepth
            enemies[(*numEnemies)].rt_index = 71;
            enemies[(*numEnemies)].exp = battleParamTable[0x30].exp;
            break;
          case 0xDC: // Delbiter
            enemies[(*numEnemies)].rt_index = 72;
            enemies[(*numEnemies)].exp = battleParamTable[0x0D].exp;
            break;
          case 0xDD: // Dolmolm and Dolmdarl
            enemies[(*numEnemies)].rt_index = 64 + (map[y].skin & 0x01);
            enemies[(*numEnemies)].exp = battleParamTable[0x4F + (map[y].skin & 0x01)].exp;
            break;
          case 0xDE: // Morfos
            enemies[(*numEnemies)].rt_index = 66;
            enemies[(*numEnemies)].exp = battleParamTable[0x40].exp;
            break;
          case 0xDF: // Recobox & Recons
            enemies[(*numEnemies)].rt_index = 67;
            enemies[(*numEnemies)].exp = battleParamTable[0x41].exp;
            for (x = 1; x <= map[y].numClones; x++)
            {
                enemies[(*numEnemies) + x].rt_index = 68;
                enemies[(*numEnemies) + x].exp = battleParamTable[0x42].exp;
            }
            break;
          case 0xE0: // Epsilon, Sinow Zoa and Zele
            if ((episode == 0x02) && (alt_enemies))
            {
                enemies[(*numEnemies)].rt_index = 84;
                enemies[(*numEnemies)].exp = battleParamTable[0x23].exp;
                (*numEnemies) += 4;
            } else {
                enemies[(*numEnemies)].rt_index = 69 + (map[y].skin & 0x01);
                enemies[(*numEnemies)].exp = battleParamTable[0x43 + (map[y].skin & 0x01)].exp;
            }
            break;
          case 0xE1: // Ill Gill
            enemies[(*numEnemies)].rt_index = 82;
            enemies[(*numEnemies)].exp = battleParamTable[0x26].exp;
            break;
          case 0x0110: // Astark
            enemies[(*numEnemies)].rt_index = 1;
            enemies[(*numEnemies)].exp = battleParamTable[0x09].exp;
            break;
          case 0x0111: // Satellite Lizard and Yowie
            enemies[(*numEnemies)].rt_index = 2 + ((map[y].reserved[10] & 0x800000) ? 0 : 1);
            enemies[(*numEnemies)].exp = battleParamTable[0x0D + ((map[y].reserved[10] & 0x800000) ? 1 : 0) + (alt_enemies ? 0x10 : 0)].exp;
            break;
          case 0x0112: // Merissa A/AA
            enemies[(*numEnemies)].rt_index = 4 + (map[y].skin & 0x01);
            enemies[(*numEnemies)].exp = battleParamTable[0x19 + (map[y].skin & 0x01)].exp;
            break;
          case 0x0113: // Girtablulu
            enemies[(*numEnemies)].rt_index = 6;
            enemies[(*numEnemies)].exp = battleParamTable[0x1F].exp;
            break;
          case 0x0114: // Zu and Pazuzu
            enemies[(*numEnemies)].rt_index = 7 + (map[y].skin & 0x01);
            enemies[(*numEnemies)].exp = battleParamTable[0x0B + (map[y].skin & 0x01) + (alt_enemies ? 0x14: 0x00)].exp;
            break;
          case 0x0115: // Boota family
            enemies[(*numEnemies)].rt_index = 9 + (map[y].skin % 3);
            if (map[y].skin & 2) enemies[(*numEnemies)].exp = battleParamTable[0x03].exp;
            else enemies[(*numEnemies)].exp = battleParamTable[0x00 + (map[y].skin % 3)].exp;
            break;
          case 0x0116: // Dorphon and Eclair
            enemies[(*numEnemies)].rt_index = 12 + (map[y].skin & 0x01);
            enemies[(*numEnemies)].exp = battleParamTable[0x0F + (map[y].skin & 0x01)].exp;
            break;
          case 0x0117: // Goran family
            if (map[y].skin & 0x02) enemies[(*numEnemies)].rt_index = 15;
            else if (map[y].skin & 0x01) enemies[(*numEnemies)].rt_index = 16;
            else enemies[(*numEnemies)].rt_index = 14;
            enemies[(*numEnemies)].exp = battleParamTable[0x11 + (map[y].skin % 3)].exp;
            break;
          case 0x0119: // Saint Million, Shambertin, and Kondrieu
            if (map[y].reserved[10] & 0x800000) enemies[(*numEnemies)].rt_index = 21;
            else enemies[(*numEnemies)].rt_index = 19 + (map[y].skin & 0x01);
            enemies[(*numEnemies)].exp = battleParamTable[0x22].exp;
            break;
          default:
            enemies[(*numEnemies)].exp = 0xFFFFFFFF;
            ConsolePrintColor("$0C> > > Warning: Unknown enemy type %08X %08X\n",map[y].base,map[y].skin);
            break;
        }
        if (map[y].numClones) (*numEnemies) += map[y].numClones;
        (*numEnemies)++;
    }
    return 0;
}

