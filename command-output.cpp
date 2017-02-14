#include <windows.h>
#include <stdio.h>

#include "text.h"
#include "operation.h"
#include "console.h"

#include "netconfig.h"
#include "updates.h"
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
#include "command-output-structures.h"

extern CFGFile* config;
extern UPDATE_LIST* updatelist;

////////////////////////////////////////////////////////////////////////////////
// Sending routines 

// a lock for the console, so only one client can print data blocks to it at a time
OPERATION_LOCK sendCommandConsoleLock = {0,0};

// encrypts and sends a command to a client
int SendCommandToClient(CLIENT* c,void* data)
{
    if (!c || !data) return 5;
    COMMAND_HEADER_PC* pc = (COMMAND_HEADER_PC*)data;
    COMMAND_HEADER_DCGC* gc = (COMMAND_HEADER_DCGC*)data;
    COMMAND_HEADER_BB* bb = (COMMAND_HEADER_BB*)data;
    DWORD datasize,sendsize;

    operation_lock(c);
    switch (c->version)
    {
      case VERSION_GAMECUBE:
      case VERSION_DC:
      case VERSION_FUZZIQER:
        datasize = gc->size;
        if (c->encrypt) sendsize = ((datasize + 3) & ~3);
        else sendsize = datasize;
        break;
      case VERSION_PC:
      case VERSION_PATCH:
        datasize = pc->size;
        if (c->encrypt) sendsize = ((datasize + 3) & ~3);
        else sendsize = datasize;
        break;
      case VERSION_BLUEBURST:
        datasize = bb->size;
        if (c->encrypt) sendsize = ((datasize + 7) & ~7);
        else sendsize = datasize;
        break;
      default:
        return 20;
    }

    // print the data if it's small enough and data display is enabled
    if (CFGIsValuePresent(config,"Show_Client_Data"))
    {
        operation_lock(&sendCommandConsoleLock);
        ConsolePrintColor("$0A> Sending to %S [version %d] [%04X bytes]:\n",c->name,c->version,datasize);
        if (datasize < (unsigned)CFGGetNumber(config,"Maximum_Display_Command_Size_Send")) CRYPT_PrintData(data,datasize);
        operation_unlock(&sendCommandConsoleLock);
    }

    // copy the command into a separate buffer
    void* encbuffer = malloc(sendsize);
    if (!encbuffer)
    {
        operation_unlock(c);
        return 1;
    }
    memcpy(encbuffer,data,datasize);
    memset((void*)((DWORD)encbuffer + datasize),0,sendsize - datasize);

    // encrypt, send, and clean up
    if (c->encrypt) CRYPT_CryptData(&c->cryptout,encbuffer,sendsize,true);
    int rv = SendClient(c,encbuffer,sendsize);
    if (!rv) c->lastsend = GetTickCount();
    free(encbuffer);
    operation_unlock(c);
    return rv;
}

// appends a command header to a data block and sends to a client 
int SendCommandToClient(CLIENT* c,int command,int flag,void* data,int size)
{
    COMMAND_DCPCGC_SUBCOMMAND* dcpcgc;
    COMMAND_BB_SUBCOMMAND* bb;
    int rv;
    switch (c->version)
    {
      case VERSION_GAMECUBE:
      case VERSION_DC:
      case VERSION_FUZZIQER:
        dcpcgc = (COMMAND_DCPCGC_SUBCOMMAND*)malloc(4 + size);
        if (!dcpcgc) return 5;
        dcpcgc->headerdcgc.command = command;
        dcpcgc->headerdcgc.flag = flag;
        dcpcgc->headerdcgc.size = 4 + size;
        memcpy(dcpcgc->entry,data,size);
        rv = SendCommandToClient(c,dcpcgc);
        free(dcpcgc);
        break;
      case VERSION_PC:
      case VERSION_PATCH:
        dcpcgc = (COMMAND_DCPCGC_SUBCOMMAND*)malloc(4 + size);
        if (!dcpcgc) return 5;
        dcpcgc->headerpc.command = command;
        dcpcgc->headerpc.flag = flag;
        dcpcgc->headerpc.size = 4 + size;
        memcpy(dcpcgc->entry,data,size);
        rv = SendCommandToClient(c,dcpcgc);
        free(dcpcgc);
        break;
      case VERSION_BLUEBURST:
        bb = (COMMAND_BB_SUBCOMMAND*)malloc(8 + size);
        if (!bb) return 5;
        bb->header.command = command;
        bb->header.flag = flag;
        bb->header.size = 8 + size;
        memcpy(bb->entry,data,size);
        rv = SendCommandToClient(c,bb);
        free(bb);
        break;
      default:
        return 20;
    }
    return rv;
}

// sends a command to everyone in a lobby/game, except the given client.
// if c is NULL, then everyone in the lobby/game receives the command.
int SendCommandToLobby(LOBBY* l,CLIENT* c,void** data)
{
    int x,rv = 0,rv2;
    if (!l || !data) return 5;
    operation_lock(l);
    for (x = 0; x < 12; x++)
    {
        if (!l->clients[x]) continue;
        if (c && (l->clients[x] == c)) continue;
        if (!data[l->clients[x]->version]) rv2 = 7853325;
        else rv2 = SendCommandToClient(l->clients[x],data[l->clients[x]->version]);
        if (rv2) rv = rv2;
    }
    operation_unlock(l);
    return rv;
}

// sends a command to everyone on a server, except the given client.
// again, c can be NULL.
int SendCommandToServer(SERVER* s,CLIENT* c,void** data)
{
    unsigned int x,rv = 0,rv2;
    if (!s || !data) return 5;
    operation_lock(s);
    for (x = 0; x < s->numClients; x++)
    {
        if (c && (s->clients[x] == c)) continue;
        if (!data[s->clients[x]->version]) rv2 = 7853325;
        else rv2 = SendCommandToClient(s->clients[x],data[s->clients[x]->version]);
        if (rv2 != 0) rv = rv2;
    }
    operation_unlock(s);
    return rv;
}

// Similar to SendCommandToClient, it appends headers to a data block and sends the command to a lobby
// except the given client.
int SendSubcommandToLobby(LOBBY* l,CLIENT* c,int command,int flag,void* sub,int size)
{
    COMMAND_DCPCGC_SUBCOMMAND* pc = (COMMAND_DCPCGC_SUBCOMMAND*)malloc(4 + size);
    if (!pc) return 1;
    COMMAND_DCPCGC_SUBCOMMAND* dcgc = (COMMAND_DCPCGC_SUBCOMMAND*)malloc(4 + size);
    if (!dcgc)
    {
        free(pc);
        return 1;
    }
    COMMAND_BB_SUBCOMMAND* bb = (COMMAND_BB_SUBCOMMAND*)malloc(8 + size);
    if (!bb)
    {
        free(dcgc);
        free(pc);
        return 1;
    }
    pc->headerpc.size = 4 + size;
    pc->headerpc.command = command;
    pc->headerpc.flag = flag;
    memcpy(pc->entry,sub,size);
    if (!TranslateCommandPCDCGC(&pc->headerpc,&dcgc->headerdcgc) || !TranslateCommandPCBB(&pc->headerpc,&bb->header))
    {
        free(bb);
        free(dcgc);
        free(pc);
        return 486786415;
    }
    void* data[VERSION_MAX] = {dcgc,bb,pc,dcgc,NULL,NULL};
    int errors = SendCommandToLobby(l,c,data);
    free(bb);
    free(dcgc);
    free(pc);
    return errors;
}

////////////////////////////////////////////////////////////////////////////////

// command sending functions follow. in general, they're written in such a way that
// you don't need to think about anything, even the client's version, before
// calling them. for this reason, some of them are quite complex. many are split
// into several functions, one for each version of PSO, named with suffixes like
// _GC, _BB, and the like. in these cases, the function without the suffix simply
// calls the appropriate function for the client's version. thus, if you change
// something in one of the version-specific functions, you may have to change it
// in all of them.

////////////////////////////////////////////////////////////////////////////////
// CommandServerInit: this function sends the command that initializes encryption

// strings needed for various functions
char* CommandServerInit_anticopyright = "This server is in no way affiliated, sponsored, or supported by SEGA Enterprises or SONICTEAM. The preceding message exists only in order to remain compatible with programs that expect it.";
char* CommandServerInit_dcPortMap = "DreamCast Port Map. Copyright SEGA Enterprises. 1999";
char* CommandServerInit_dcLobbyServer = "DreamCast Lobby Server. Copyright SEGA Enterprises. 1999";
char* CommandServerInit_bb = "Phantasy Star Online Blue Burst Game Server. Copyright 1999-2004 SONICTEAM.";
char* CommandServerInit_patch = "Patch Server. Copyright SonicTeam, LTD. 2001";

//int CommandServerInit_DC(CLIENT* c,bool startServer)

int CommandServerInit_PC(CLIENT* c,bool startserver)
{
    COMMAND_PCPATCH_ENCRYPTIONINIT* p = (COMMAND_PCPATCH_ENCRYPTIONINIT*)malloc(0x108);
    if (!p) return 1;
    p->header.command = 0x17;
    p->header.flag = 0;
    p->header.size = 0x0108;
    // it's possible that PC doesn't use the DC lobby server message; we'll have to test this to find out
    strcpy(p->copyright,startserver ? CommandServerInit_dcPortMap : CommandServerInit_dcLobbyServer);
    p->serverkey = (rand() << 16) | rand();
    p->clientkey = (rand() << 16) | rand();
    strcpy(p->aftermessage,CommandServerInit_anticopyright);
    CRYPT_CreateKeys(&c->cryptout,&p->serverkey,CRYPT_PC);
    CRYPT_CreateKeys(&c->cryptin,&p->clientkey,CRYPT_PC);
    int rv = SendCommandToClient(c,p);
    free(p);
    c->encrypt = true;
    return rv;
}

int CommandServerInit_GC(CLIENT* c,bool startserver)
{
    COMMAND_GC_ENCRYPTIONINIT* p = (COMMAND_GC_ENCRYPTIONINIT*)malloc(0x110);
    p->header.command = (startserver ? 0x17 : 0x02);
    p->header.flag = 0;
    p->header.size = 0x0108;
    strcpy(p->copyright,startserver ? CommandServerInit_dcPortMap : CommandServerInit_dcLobbyServer);
    p->serverkey = (rand() << 16) | rand();
    p->clientkey = (rand() << 16) | rand();
    strcpy(p->aftermessage,CommandServerInit_anticopyright);
    CRYPT_CreateKeys(&c->cryptout,&p->serverkey,CRYPT_GAMECUBE);
    CRYPT_CreateKeys(&c->cryptin,&p->clientkey,CRYPT_GAMECUBE);
    int rv = SendCommandToClient(c,p);
    free(p);
    c->encrypt = true;
    return rv;
}

int CommandServerInit_BB(CLIENT* c,bool)
{
    int x;
    COMMAND_BB_ENCRYPTIONINIT* p = (COMMAND_BB_ENCRYPTIONINIT*)malloc(0x184);
    if (!p) return 1;
    memset(p,0,0x0184);
    p->header.command = 0x03;
    p->header.flag = 0;
    p->header.size = 0x0184;
    strcpy(p->copyright,CommandServerInit_bb);
    for (x = 0; x < 0x30; x++)
    {
        p->serverkey[x] = rand();
        p->clientkey[x] = rand();
    }
    strcpy(p->aftermessage,CommandServerInit_anticopyright);
    CRYPT_CreateKeys(&c->cryptout,p->serverkey,CRYPT_BLUEBURST);
    CRYPT_CreateKeys(&c->cryptin,p->clientkey,CRYPT_BLUEBURST);
    int rv = SendCommandToClient(c,p);
    free(p);
    c->encrypt = true;
    return rv;
}

int CommandServerInit_Patch(CLIENT* c,bool startServer)
{
    COMMAND_PCPATCH_ENCRYPTIONINIT p;
    p.header.command = 0x02;
    p.header.flag = 0;
    p.header.size = 0x4C;
    strcpy(p.copyright,CommandServerInit_patch);
    p.serverkey = (rand() << 16) | rand();
    p.clientkey = (rand() << 16) | rand();
    CRYPT_CreateKeys(&c->cryptout,&p.serverkey,CRYPT_PC);
    CRYPT_CreateKeys(&c->cryptin,&p.clientkey,CRYPT_PC);
    int rv = SendCommandToClient(c,&p);
    c->encrypt = true;
    return rv;
}

int (*CommandServerInit_Functions[])(CLIENT* c,bool startServer) = {CommandServerInit_GC,CommandServerInit_BB,CommandServerInit_PC,/*CommandServerInit_DC*/NULL,CommandServerInit_Patch,/*CommandServerInit_Fuzziqer*/NULL};
int CommandServerInit(CLIENT* c,bool startserver)
{
    if (!c) return 5;
    if (!CommandServerInit_Functions[c->version]) return 20;
    return CommandServerInit_Functions[c->version](c,startserver);
}

// For non-BB clients, updates the client's guild card and security data
// (BB clients use CommandBBClientInit)
int CommandUpdateClientConfig(CLIENT* c)
{
    if (c->version == VERSION_BLUEBURST) return 541973568;
    COMMAND_DCPCGC_SETSECURITY p;
    if (c->version == VERSION_PC)
    {
        p.headerpc.size = 0x002C;
        p.headerpc.command = 0x04;
        p.headerpc.flag = 0x00;
    } else {
        p.headerdcgc.command = 0x04;
        p.headerdcgc.flag = 0x00;
        p.headerdcgc.size = 0x002C;
    }
    p.playerTag = 0x00000100;
    p.serialNumber = c->license.serialNumber;
    memcpy(&p.cfg,&c->cfg,sizeof(CLIENTCONFIG));
    return SendCommandToClient(c,&p);
}

//int CommandApproveLicense(CLIENT* c)
//int CommandApproveRegisteredLicense(CLIENT* c)
//int CommandGetCharacterInfo(SERVER* s,CLIENT* c)

////////////////////////////////////////////////////////////////////////////////
// CommandReconnect: sends the command that tells PSO to reconnect to a different address and port.

int CommandReconnect_DCPCGC(CLIENT* c,DWORD address,int port)
{
    COMMAND_DCPCGC_RECONNECT p;
    if (c->version == VERSION_PC)
    {
        p.headerpc.size = 0x000C;
        p.headerpc.command = 0x19;
        p.headerpc.flag = 0x00;
    } else {
        p.headerdcgc.command = 0x19;
        p.headerdcgc.flag = 0x00;
        p.headerdcgc.size = 0x000C;
    }
    p.address = address;
    p.port = port;
    return SendCommandToClient(c,&p);
}

int CommandReconnect_BB(CLIENT* c,DWORD address,int port)
{
    COMMAND_BB_RECONNECT p;
    p.header.size = 0x0010;
    p.header.command = 0x0019;
    p.header.flag = 0x00000000;
    p.address = address;
    p.port = port;
    return SendCommandToClient(c,&p);
}

int (*CommandReconnect_Functions[])(CLIENT* c,DWORD address,int port) = {CommandReconnect_DCPCGC,CommandReconnect_BB,CommandReconnect_DCPCGC,CommandReconnect_DCPCGC,NULL,NULL};
int CommandReconnect(CLIENT* c,DWORD address,int port)
{
    if (!c) return 5;
    if (!CommandReconnect_Functions[c->version]) return 20;
    return CommandReconnect_Functions[c->version](c,address,port);
}

// sends the command (first used by Schthack) that separates PC and GC users that connect on the same port
int CommandSelectiveReconnectPC(CLIENT* c,DWORD address,int port)
{
    if (!c) return 5;
    if (c->version == VERSION_BLUEBURST) return 89356594;
    COMMAND_DCPCGC_RECONNECT* p = (COMMAND_DCPCGC_RECONNECT*)malloc(0xB0);
    if (!p) return 1;
    p->headerdcgc.command = 0xB0;
    p->headerdcgc.flag = 0x00;
    p->headerdcgc.size = 0x0019;
    p->address = address;
    p->port = port;
    COMMAND_HEADER_DCGC* p2 = (COMMAND_HEADER_DCGC*)((DWORD)p + 0x19);
    p2->command = 0xB0;
    p2->flag = 0x00;
    p2->size = 0x0097;
    int rv = SendCommandToClient(c,p);
    free(p);
    return rv;
}

////////////////////////////////////////////////////////////////////////////////
// BB only commands 

// sends the command that signals an error or updates the client's guild card number and security data
int CommandBBClientInit(CLIENT* c,DWORD error)
{
    if (!c) return 5;
    COMMAND_BB_SETSECURITY p;
    p.header.size = sizeof(COMMAND_BB_SETSECURITY);
    p.header.command = 0x00E6;
    p.header.flag = 0x00000000;
    p.error = error;
    p.playerTag = 0x00010000;
    p.serialNumber = c->license.serialNumber;
    p.teamID = (rand() << 16) | rand();//0xFFFFFFFF;
    memcpy(&p.cfg,&c->cfg,sizeof(CLIENTCONFIG_BB));
    p.caps = 0x00000102;
    return SendCommandToClient(c,&p);
}

// sends the client's key config
int CommandBBSendKeyConfig(CLIENT* c)
{
    if (!c) return 5;
    if (c->version != VERSION_BLUEBURST) return 8;

    COMMAND_BB_KEYCONFIG p;
    p.header.size = sizeof(COMMAND_BB_KEYCONFIG);
    p.header.command = 0x00E2;
    p.header.flag = 0x00000000;
    memcpy(&p.keyconfig,&c->playerInfo.keyConfig,sizeof(BB_KEY_TEAM_CONFIG));
    return SendCommandToClient(c,&p);
}

// sends a player preview. these are used by the caracter select and character creation mechanism.
// see ApplyPlayerPreview for more information
int CommandBBSendPlayerPreview(CLIENT* c,DWORD playernum)
{
    if (!c) return 5;

    COMMAND_BB_PLAYERPREVIEW p;
    COMMAND_BB_NOPLAYERPREVIEW p2;
    char signature[0x40];
    bool oldformat = false;

    DWORD bytesread;
    char filename[MAX_PATH];
    sprintf(filename,"system\\players\\player_%s_%ld.nsc",c->license.username,playernum + 1);
    HANDLE file = CreateFile(filename,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        oldformat = true;
        sprintf(filename,"system\\players\\%s-player-%ld.pbb",c->license.username,playernum);
        file = CreateFile(filename,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    }
    if (file == INVALID_HANDLE_VALUE)
    {
        p2.header.size = 0x0010;
        p2.header.command = 0x00E4;
        p2.header.flag = 0x00000000;
        p2.playernum = playernum;
        p2.error = 2;
        return SendCommandToClient(c,&p2);
    } else {
        if (oldformat)
        {
            p.header.size = 0x0088;
            p.header.command = 0x00E5;
            p.header.flag = 0x00000000;
            p.playernum = playernum;
            SetFilePointer(file,0x0364,NULL,FILE_BEGIN);
            ReadFile(file,&p.preview.level,0x0004,&bytesread,NULL);
            ReadFile(file,&p.preview.exp,0x0004,&bytesread,NULL);
            SetFilePointer(file,0x0370,NULL,FILE_BEGIN);
            ReadFile(file,p.preview.guildcard,0x74,&bytesread,NULL);
        } else {
            ReadFile(file,signature,0x40,&bytesread,NULL);
            if (!strcmp(signature,PLAYER_FILE_SIGNATURE))
            {
                p.header.size = 0x0088;
                p.header.command = 0x00E5;
                p.header.flag = 0x00000000;
                p.playernum = playernum;
                ReadFile(file,&p.preview,sizeof(BB_PLAYER_DISPDATA_PREVIEW),&bytesread,NULL);
                if (bytesread != sizeof(BB_PLAYER_DISPDATA_PREVIEW))
                {
                    CloseHandle(file);
                    return 8905;
                }
            } else {
                p2.header.size = 0x0010;
                p2.header.command = 0x00E4;
                p2.header.flag = 0x00000000;
                p2.playernum = playernum;
                p2.error = 1;
                CloseHandle(file);
                return SendCommandToClient(c,&p2);
            }
        }
        CloseHandle(file);
        return SendCommandToClient(c,&p);
    }
    return 5438493;
}

// sent in response to the client's 01E8 command
int CommandBBAcceptClientChecksum(CLIENT* c)
{
    if (!c) return 5;
    COMMAND_BB_ACCEPTCLIENTCHECKSUM p;
    p.header.size = 0x000C;
    p.header.command = 0x02E8;
    p.header.flag = 0x00000000;
    p.verify = 2;
    return SendCommandToClient(c,&p);
}

// sends the "I'm about to send your guild card file" command
int CommandBBSendGuildCardHeader(CLIENT* c)
{
    if (!c) return 5;
    if (c->version != VERSION_BLUEBURST) return 5;
    COMMAND_BB_GUILDCARDHEADER h;
    h.header.size = 0x0014;
    h.header.command = 0x01DC;
    h.header.flag = 0x00000000;
    h.unknown = 1;
    h.filesize = sizeof(BB_GUILDCARD_FILE);
    h.checksum = CalculateGuildCardChecksum(&c->playerInfo.guildcards,sizeof(BB_GUILDCARD_FILE));//0x0EA14A8B;
    return SendCommandToClient(c,&h);
}

// sends a chunk of guild card data
int CommandBBSendGuildCardData(CLIENT* c,DWORD chunk)
{
    if (!c) return 5;
    unsigned int beginOffset = (chunk * 0x6800);
    if (beginOffset >= sizeof(BB_GUILDCARD_FILE)) return 849365;
    unsigned int dataSize = sizeof(BB_GUILDCARD_FILE) - beginOffset;
    if (dataSize > 0x6800) dataSize = 0x6800;

    COMMAND_BB_GUILDCARDCHUNK* p = (COMMAND_BB_GUILDCARDCHUNK*)malloc(0x6810);
    if (!p) return 1;
    p->header.size = dataSize + 0x0010;
    p->header.command = 0x02DC;
    p->header.flag = 0x00000000;
    p->unknown = 0;
    p->chunkID = chunk;
    memcpy(p->data,(void*)((DWORD)(&c->playerInfo.guildcards) + beginOffset),dataSize);
    int rv = SendCommandToClient(c,p);
    free(p);
    return rv;
}

// sends the game data (battleparamentry files, etc.)
int CommandBBSendStreamFile(SERVER* s,CLIENT* c)
{
    if (!s || !c) return 5;

    int errors;
    DWORD x,size,bytesread;
    HANDLE file;

    file = CreateFile("system\\blueburst\\streamfile.ind",GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    if (file == INVALID_HANDLE_VALUE) return 537;
    size = GetFileSize(file,NULL);
    COMMAND_BB_STREAMFILEINDEX* pi = (COMMAND_BB_STREAMFILEINDEX*)malloc(size + 4);
    if (!pi)
    {
        CloseHandle(file);
        return 1;
    }
    ReadFile(file,(void*)((DWORD)pi + 4),size,&bytesread,NULL);
    CloseHandle(file);
    if (bytesread != size)
    {
        free(pi);
        return 539;
    }
    pi->size = size + 4;
    pi->command = 0x01EB;
    errors = SendCommandToClient(c,pi);
    if (errors)
    {
        free(pi);
        return errors;
    }

    COMMAND_BB_STREAMFILECHUNK* pc = (COMMAND_BB_STREAMFILECHUNK*)malloc(0x680C);
    DWORD dataInBuffer = 0,fileDataRemaining,thisReadSize,chunknum = 0;
    char filename[MAX_PATH];

    if (!pc)
    {
        free(pi);
        return 1;
    }

    for (x = 0; (x < pi->num) && !errors; x++)
    {
        strcpy(filename,"system\\blueburst\\");
        strcat(filename,pi->entry[x].filename);
        file = CreateFile(filename,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
        if (file == INVALID_HANDLE_VALUE)
        {
            errors = 5387441;
            break;
        }
        fileDataRemaining = pi->entry[x].size;
        while (fileDataRemaining && !errors)
        {
            thisReadSize = 0x6800 - dataInBuffer;
            if (thisReadSize > fileDataRemaining) thisReadSize = fileDataRemaining;
            ReadFile(file,&pc->data[dataInBuffer],thisReadSize,&bytesread,NULL);
            if (bytesread != thisReadSize)
            {
                errors = 5387442;
                break;
            }
            dataInBuffer += thisReadSize;
            fileDataRemaining -= thisReadSize;
            if (dataInBuffer == 0x6800)
            {
                errors = ProcessCommands(s,c,0x0005,0x03EB,0);
                pc->header.size = (dataInBuffer + 15) & ~3;
                pc->header.command = 0x02EB;
                pc->header.flag = 0x00000000;
                pc->chunknum = chunknum;
                if (!errors) errors = SendCommandToClient(c,pc);
                dataInBuffer = 0;
                chunknum++;
            }
        }
        CloseHandle(file);
    }

    if ((dataInBuffer > 0) && !errors)
    {
        errors = ProcessCommands(s,c,0x0005,0x03EB,0);
        pc->header.size = (dataInBuffer + 15) & ~3;
        pc->header.command = 0x02EB;
        pc->header.flag = 0x00000000;
        pc->chunknum = chunknum;
        if (!errors) SendCommandToClient(c,pc);
    }
    free(pc);
    free(pi);
    return errors;
}

// accepts the player's choice at char select
int CommandBBApprovePlayerChoice(CLIENT* c)
{
    if (!c) return 5;
    COMMAND_BB_ACCEPTCLIENTCHECKSUM p;
    p.header.size = 0x0010;
    p.header.command = 0x00E4;
    p.header.flag = 0x00000000;
    p.verify = c->cfg.bbplayernum;
    p.unused = 1;
    return SendCommandToClient(c,&p);
}

// sends player data to the client (usually sent right before entering lobby)
int CommandBBSendPlayerInfo(CLIENT* c)
{
    if (!c) return 5;
    COMMAND_BB_PLAYERSAVE p;
    p.header.size = 0x399C;
    p.header.command = 0x00E7;
    p.header.flag = 0x00000000;
    ExportCompletePlayerData(&p.data,&c->playerInfo);
    return SendCommandToClient(c,&p);
}

////////////////////////////////////////////////////////////////////////////////
// message functions 

//int CommandScrollingMessage(CLIENT* c,wchar_t* text) // for BB, this is command EE with the same format as 1A/D5

int CommandPatchCheckDirectory(CLIENT* c,char* dir)
{
    if (!c) return 5;
    COMMAND_PATCH_CHECKDIR p;
    memset(&p,0,0x44);
    p.header.size = 0x0044;
    p.header.command = 0x09;
    p.header.flag = 0;
    strcpy(p.dir,dir);
    return SendCommandToClient(c,&p);
}

////////////////////////////////////////////////////////////////////////////////
// Message functions (CommandMessageBox, CommandLobbyName, CommandQuestInfo)
// CommandMessageBox: this function displays a large message box (1A/D5) on the client's screen.
// CommandLobbyName: this function tells the client the current lobby's/game's name.
// CommandQuestInfo: displays the quest info box when a client is selecting a quest.

int CommandLargeBasicMessage_PCPatch(CLIENT* c,int command,wchar_t* text)
{
    COMMAND_PC_LARGEMESSAGE* p = (COMMAND_PC_LARGEMESSAGE*)malloc(0x808);
    if (!p) return 1;
    p->header.command = command;
    p->header.flag = 0;
    p->header.size = 0x0008 + (((2 * wcslen(text)) + 5) & ~3);
    wcscpy(p->text,text);
    tx_add_color(p->text);
    int rv = SendCommandToClient(c,p);
    free(p);
    return rv;
}

int CommandLargeBasicMessage_DCGC(CLIENT* c,int command,wchar_t* text)
{
    COMMAND_GC_LARGEMESSAGE* p = (COMMAND_GC_LARGEMESSAGE*)malloc(0x408);
    if (!p) return 1;
    p->header.command = command;
    p->header.flag = 0;
    p->header.size = 0x0004 + ((tx_convert_to_sjis(p->text,text) + 5) & ~3);
    tx_add_color(p->text);
    int rv = SendCommandToClient(c,p);
    free(p);
    return rv;
}

int CommandLargeBasicMessage_BB(CLIENT* c,int command,wchar_t* text)
{
    COMMAND_BB_LARGEMESSAGE* p = (COMMAND_BB_LARGEMESSAGE*)malloc(0x80C);
    if (!p) return 1;
    p->header.command = command;
    p->header.flag = 0;
    p->header.size = 0x0008 + (((2 * wcslen(text)) + 5) & ~3);
    wcscpy(p->text,text);
    tx_add_color(p->text);
    int rv = SendCommandToClient(c,p);
    free(p);
    return rv;
}

int (*CommandLargeBasicMessage_Functions[])(CLIENT* c,int command,wchar_t* text) = {CommandLargeBasicMessage_DCGC,CommandLargeBasicMessage_BB,CommandLargeBasicMessage_PCPatch,CommandLargeBasicMessage_DCGC,CommandLargeBasicMessage_PCPatch,NULL};
int CommandLargeBasicMessage(CLIENT* c,int command,wchar_t* text)
{
    if (!c || !text) return 5;
    if (!CommandLargeBasicMessage_Functions[c->version]) return 20;
    return CommandLargeBasicMessage_Functions[c->version](c,command,text);
}

int CommandMessageBox(CLIENT* c,wchar_t* text) { return CommandLargeBasicMessage(c,((c->version == VERSION_PATCH) ? 0x13 : 0x1A),text); }
int CommandLobbyName(CLIENT* c,wchar_t* text) { return CommandLargeBasicMessage(c,0x8A,text); }
int CommandQuestInfo(CLIENT* c,wchar_t* text) { return CommandLargeBasicMessage(c,0xA3,text); }

////////////////////////////////////////////////////////////////////////////////
// message commands (CommandLobbyMessageBox CommandShipInfo CommandTextMessage )
// these use a slightly different format than the above.
// these functions are designed to broadcast messages, so the version-specific functions
// simply generate data for specific versions, and do not send anything.
// CommandLobbyMessageBox: shows a small message box in the lower-left corner of the screen. used when errors occur joining a game, for example.
// CommandShipInfo: shows a small message box in the lower-right corner of the screen. on GC (and maybe other versions) the client cannot make this disappear if used in the lobby.
// CommandTextMessage: sends a text message that appears near the middle of the screen. using colors may help it stand out (i.e. $C6 for yellow)

//void* DataCommandBasicMessage_DC(CLIENT* c,wchar_t* text)

void* DataCommandBasicMessage_PC(int command,wchar_t* text)
{
    COMMAND_PC_MESSAGE* p = (COMMAND_PC_MESSAGE*)malloc(0x810);
    if (!p) return NULL;
    p->header.command = command;
    p->header.flag = 0;
    p->header.size = 0x000C + (((2 * wcslen(text)) + 5) & ~3);
    p->unused = 0;
    p->serialNumber = 0;
    wcscpy(p->text,text);
    tx_add_color(p->text);
    return p;
}

void* DataCommandBasicMessage_GC(int command,wchar_t* text)
{
    COMMAND_GC_MESSAGE* p = (COMMAND_GC_MESSAGE*)malloc(0x408);
    if (!p) return NULL;
    p->header.command = command;
    p->header.flag = 0;
    p->header.size = 0x000C + ((tx_convert_to_sjis(p->text,text) + 5) & ~3);
    p->unused = 0;
    p->serialNumber = 0;
    tx_add_color(p->text);
    return p;
}

void* DataCommandBasicMessage_BB(int command,wchar_t* text)
{
    COMMAND_BB_MESSAGE* p = (COMMAND_BB_MESSAGE*)malloc(0x80C);
    if (!p) return NULL;
    p->header.command = command;
    p->header.flag = 0;
    p->header.size = 0x0010 + (((2 * wcslen(text)) + 5) & ~3);
    p->unused = 0;
    p->serialNumber = 0;
    wcscpy(p->text,text);
    tx_add_color(p->text);
    return p;
}

void* (*DataCommandBasicMessage_Functions[])(int command,wchar_t* text) = {DataCommandBasicMessage_GC,DataCommandBasicMessage_BB,DataCommandBasicMessage_PC,/*DataCommandBasicMessage_DC*/NULL,NULL,NULL};
int CommandBasicMessage(SERVER* s,LOBBY* li,CLIENT* c,int command,wchar_t* text)
{
    if ((!s && !li && !c) || !text) return 5;
    void* data[VERSION_MAX];
    int x;
    for (x = 0; x < VERSION_MAX; x++)
    {
        if (!DataCommandBasicMessage_Functions[x]) data[x] = NULL;
        else data[x] = DataCommandBasicMessage_Functions[x](command,text);
    }
    int errors;
    if (s) errors = SendCommandToServer(s,NULL,data);
    else if (li) errors = SendCommandToLobby(li,NULL,data);
    else errors = SendCommandToClient(c,data[c->version]);
    for (x = 0; x < VERSION_MAX; x++) if (data[x]) free(data[x]);
    return errors;
}

int CommandLobbyMessageBox(SERVER* s,LOBBY* li,CLIENT* c,wchar_t* text) { return CommandBasicMessage(s,li,c,0x0001,text); }
int CommandShipInfo(SERVER* s,LOBBY* li,CLIENT* c,wchar_t* text) { return CommandBasicMessage(s,li,c,0x0011,text); }
int CommandTextMessage(SERVER* s,LOBBY* li,CLIENT* c,wchar_t* text) { return CommandBasicMessage(s,li,c,0x00B0,text); }

////////////////////////////////////////////////////////////////////////////////
// CommandChat: sends a message from a specific player to the lobby

void* DataCommandChat_PC(DWORD serialNumber,wchar_t* name,wchar_t* text)
{
    wchar_t textbuffer[0x200];
    wcscpy(textbuffer,text);
    tx_add_language_marker(textbuffer,L'J');

    COMMAND_PC_MESSAGE* p = (COMMAND_PC_MESSAGE*)malloc(0x810);
    if (!p) return NULL;
    p->header.command = 0x06;
    p->header.flag = 0;
    p->unused = 0;
    p->serialNumber = serialNumber;
    wcscpy(p->text,name);
    tx_remove_language_marker(p->text);
    wcscat(p->text,L"\t");
    wcscat(p->text,textbuffer);
    tx_add_color(p->text);
    p->header.size = 0x000C + (((2 * wcslen(p->text)) + 5) & ~3);
    return p;
}

void* DataCommandChat_DCGC(DWORD serialNumber,wchar_t* name,wchar_t* text)
{
    wchar_t textbuffer[0x200];
    wcscpy(textbuffer,text);
    tx_add_language_marker(textbuffer,L'J');

    COMMAND_GC_MESSAGE* p = (COMMAND_GC_MESSAGE*)malloc(0x410);
    if (!p) return NULL;
    p->header.command = 0x06;
    p->header.flag = 0;
    p->unused = 0;
    p->serialNumber = serialNumber;
    tx_convert_to_sjis(p->text,name);
    tx_remove_language_marker(p->text);
    tx_convert_to_sjis(&p->text[strlen(p->text)],"\t");
    tx_convert_to_sjis(&p->text[strlen(p->text)],textbuffer);
    tx_add_color(p->text);
    p->header.size = 0x000C + ((strlen(p->text) + 5) & ~3);
    return p;
}

void* DataCommandChat_BB(DWORD serialNumber,wchar_t* name,wchar_t* text)
{
    wchar_t textbuffer[0x200];
    wcscpy(textbuffer,text);
    tx_add_language_marker(textbuffer,L'J');

    COMMAND_BB_MESSAGE* p = (COMMAND_BB_MESSAGE*)malloc(0x814);
    if (!p) return NULL;
    p->header.command = 0x0006;
    p->header.flag = 0;
    p->unused = 0;
    p->serialNumber = serialNumber;
    wcscpy(p->text,name);
    tx_add_language_marker(p->text,L'J');
    wcscat(p->text,L"\t");
    wcscat(p->text,textbuffer);
    tx_add_color(p->text);
    p->header.size = 0x0010 + (((2 * wcslen(p->text)) + 5) & ~3);
    return p;
}

void* (*DataCommandChat_Functions[])(DWORD serialNumber,wchar_t* name,wchar_t* text) = {DataCommandChat_DCGC,DataCommandChat_BB,DataCommandChat_PC,DataCommandChat_DCGC,NULL,NULL};
int CommandChat(LOBBY* li,CLIENT* c,wchar_t* text)
{
    if ((!li && !c) || !text) return 5;
    void* data[VERSION_MAX];
    int x;
    for (x = 0; x < VERSION_MAX; x++)
    {
        if (!DataCommandChat_Functions[x]) data[x] = NULL;
        else data[x] = DataCommandChat_Functions[x](c->license.serialNumber,c->playerInfo.disp.playername,text);
    }
    int errors;
    errors = SendCommandToLobby(li,NULL,data);
    for (x = 0; x < VERSION_MAX; x++) if (data[x]) free(data[x]);
    return errors;
}

////////////////////////////////////////////////////////////////////////////////
// CommandSendInfoBoard: sends the info board to a player.

//int CommandSendInfoBoard_DC(LOBBY* l,CLIENT* c)

int CommandSendInfoBoard_PC(LOBBY* l,CLIENT* c)
{
    COMMAND_PC_INFOBOARD* p = (COMMAND_PC_INFOBOARD*)malloc(4);
    if (!p) return 1;
    p->header.size = 0x0004;
    p->header.command = 0xD8;
    p->header.flag = 0x00;

    unsigned int x;
    operation_lock(l);
    for (x = 0; x < l->maxClients; x++)
    {
        if (!l->clients[x]) continue; // there's no client here 
        p->header.flag++;
        p->header.size += 0x0178;
        p = (COMMAND_PC_INFOBOARD*)realloc(p,p->header.size);
        if (!p)
        {
            operation_unlock(l);
            return 1;
        }
        memset(p->entry[p->header.flag - 1].name,0,0x20);
        memset(p->entry[p->header.flag - 1].message,0,0x0158);
        wcscpy(p->entry[p->header.flag - 1].name,l->clients[x]->playerInfo.disp.playername);
        wcscpy(p->entry[p->header.flag - 1].message,l->clients[x]->playerInfo.infoboard);
        tx_add_color(p->entry[p->header.flag - 1].message);
    }
    operation_unlock(l);
    int errors = SendCommandToClient(c,p);
    free(p);
    return errors;
}

int CommandSendInfoBoard_GC(LOBBY* l,CLIENT* c)
{
    COMMAND_GC_INFOBOARD* p = (COMMAND_GC_INFOBOARD*)malloc(4);
    if (!p) return 1;
    p->header.size = 0x0004;
    p->header.command = 0xD8;
    p->header.flag = 0x00;

    unsigned int x;
    operation_lock(l);
    for (x = 0; x < l->maxClients; x++)
    {
        if (!l->clients[x]) continue; // there's no client here 
        p->header.flag++;
        p->header.size += 0xBC;
        p = (COMMAND_GC_INFOBOARD*)realloc(p,p->header.size);
        if (!p)
        {
            operation_unlock(l);
            return 1;
        }
        memset(p->entry[p->header.flag - 1].name,0,0x10);
        memset(p->entry[p->header.flag - 1].message,0,0xAC);
        tx_convert_to_sjis(p->entry[p->header.flag - 1].name,l->clients[x]->playerInfo.disp.playername);
        tx_convert_to_sjis(p->entry[p->header.flag - 1].message,l->clients[x]->playerInfo.infoboard);
        tx_add_color(p->entry[p->header.flag - 1].message);
    }
    operation_unlock(l);
    int errors = SendCommandToClient(c,p);
    free(p);
    return errors;
}

int CommandSendInfoBoard_BB(LOBBY* l,CLIENT* c)
{
    COMMAND_BB_INFOBOARD* p = (COMMAND_BB_INFOBOARD*)malloc(8);
    if (!p) return 1;
    p->header.size = 0x0008;
    p->header.command = 0x00D8;
    p->header.flag = 0x00000000;

    unsigned int x;
    operation_lock(l);
    for (x = 0; x < l->maxClients; x++)
    {
        if (!l->clients[x]) continue; // there's no client here 
        p->header.flag++;
        p->header.size += 0x0178;
        p = (COMMAND_BB_INFOBOARD*)realloc(p,p->header.size);
        if (!p)
        {
            operation_unlock(l);
            return 1;
        }
        memset(p->entry[p->header.flag - 1].name,0,0x20);
        memset(p->entry[p->header.flag - 1].message,0,0x0158);
        wcscpy(p->entry[p->header.flag - 1].name,l->clients[x]->playerInfo.disp.playername);
        wcscpy(p->entry[p->header.flag - 1].message,l->clients[x]->playerInfo.infoboard);
        tx_add_color(p->entry[p->header.flag - 1].message);
    }
    operation_unlock(l);
    int errors = SendCommandToClient(c,p);
    free(p);
    return errors;
}

int (*CommandSendInfoBoard_Functions[])(LOBBY* l,CLIENT* c) = {CommandSendInfoBoard_GC,CommandSendInfoBoard_BB,CommandSendInfoBoard_PC,/*CommandSendInfoBoard_DC*/NULL,NULL,NULL};
int CommandSendInfoBoard(LOBBY* l,CLIENT* c)
{
    if (!l || !c) return 5;
    if (!CommandSendInfoBoard_Functions[c->version]) return 20;
    return CommandSendInfoBoard_Functions[c->version](l,c);
}

////////////////////////////////////////////////////////////////////////////////
// CommandCardSearchResult: sends a guild card search result to a player.
// note that this function does not perform a search, only generates and sends the result.

int CommandCardSearchResult_DCGC(SERVER* s,LOBBY* l,CLIENT* c,CLIENT* target)
{
    operation_lock(l);
    COMMAND_DCGC_CARDSEARCHRESULT p;
    p.header.command = 0x40;
    p.header.flag = 0x00;
    p.header.size = 0x00C4;
    p.playerTag = 0x00000100;
    p.searcherSerialNumber = c->license.serialNumber;
    p.targetSerialNumber = target->license.serialNumber;
    p.usedReconnectCommand.headerdcgc.command = 0x19;
    p.usedReconnectCommand.headerdcgc.flag = 0x00;
    p.usedReconnectCommand.headerdcgc.size = 0x000C;
    p.usedReconnectCommand.address = target->localip;
    p.usedReconnectCommand.port = target->port;
    if (l->flags & LOBBY_FLAG_GAME) sprintf(p.locationString,"%S,Block 00,,%s",l->name,CFGGetStringA(config,"Short_Name"));
    else sprintf(p.locationString,"Block 00,,%s",CFGGetStringA(config,"Short_Name"));
    p.menuID = 0xFFFFFFFF;
    p.lobbyID = target->lobbyID;
    memset(p.unused,0,0x3C);
    wcscpy(p.name,target->playerInfo.disp.playername);
    operation_unlock(l);
    return SendCommandToClient(c,&p);
}

//int CommandCardSearchResult_PC(SERVER* s,LOBBY* l,CLIENT* c,CLIENT* target)

int CommandCardSearchResult_BB(SERVER* s,LOBBY* l,CLIENT* c,CLIENT* target)
{
    operation_lock(l);
    COMMAND_BB_CARDSEARCHRESULT p;
    p.header.command = 0x0040;
    p.header.flag = 0x00000000;
    p.header.size = 0x0130;
    p.playerTag = 0x00000100;
    p.searcherSerialNumber = c->license.serialNumber;
    p.targetSerialNumber = target->license.serialNumber;
    p.usedReconnectCommand.header.command = 0x0019;
    p.usedReconnectCommand.header.flag = 0x00000000;
    p.usedReconnectCommand.header.size = 0x0010;
    p.usedReconnectCommand.address = target->localip;
    p.usedReconnectCommand.port = target->port;
    if (l->flags & LOBBY_FLAG_GAME) swprintf(p.locationString,L"%s,Block 00,,%s",l->name,CFGGetStringW(config,"Short_Name"));
    else swprintf(p.locationString,L"Block 00,,%s",CFGGetStringW(config,"Short_Name"));
    p.menuID = 0xFFFFFFFF;
    p.lobbyID = target->lobbyID;
    memset(p.unused,0,0x3C);
    wcscpy(p.name,target->playerInfo.disp.playername);
    operation_unlock(l);
    return SendCommandToClient(c,&p);
}

int (*CommandCardSearchResult_Functions[])(SERVER* s,LOBBY* l,CLIENT* c,CLIENT* target) = {CommandCardSearchResult_DCGC,CommandCardSearchResult_BB,/*CommandCardSearchResult_PC*/NULL,CommandCardSearchResult_DCGC,NULL,NULL};
int CommandCardSearchResult(SERVER* s,CLIENT* c,CLIENT* target)
{
    if (!s || !c || !target) return 5;
    LOBBY* l = FindLobby(s,target->lobbyID);
    if (!l) return 0;
    if (!CommandCardSearchResult_Functions[c->version]) return 20;
    return CommandCardSearchResult_Functions[c->version](s,l,c,target);
}

////////////////////////////////////////////////////////////////////////////////
// CommandSendGuildCard: generates a guild card for the source player and sends it to the destination player

//int CommandSendGuildCard_DC(CLIENT* c,CLIENT* source)
//int CommandSendGuildCard_PC(CLIENT* c,CLIENT* source)

int CommandSendGuildCard_GC(CLIENT* c,CLIENT* source)
{
    operation_lock(source);
    COMMAND_GC_SENDGUILDCARD p;
    memset(&p,0,sizeof(COMMAND_GC_SENDGUILDCARD));
    p.header.command = 0x0060;
    p.header.flag = 0x00000000;
    p.header.size = 0x0098;
    p.subcommand = 0x06;
    p.subsize = 0x25;
    p.unused = 0x0000;
    p.data.playerTag = 0x00000100;
    p.data.serialNumber = source->license.serialNumber;
    tx_convert_to_sjis(p.data.name,source->playerInfo.disp.playername);
    tx_remove_language_marker(p.data.name);
    tx_convert_to_sjis(p.data.desc,source->playerInfo.guildcarddesc);
    p.data.reserved1 = 1;
    p.data.reserved2 = 0;
    p.data.sectionID = source->playerInfo.disp.sectionID;
    p.data.charClass = source->playerInfo.disp.charClass;
    operation_unlock(source);
    return SendCommandToClient(c,&p);
}

int CommandSendGuildCard_BB(CLIENT* c,CLIENT* source)
{
    operation_lock(source);
    COMMAND_BB_SENDGUILDCARD p;
    memset(&p,0,sizeof(COMMAND_BB_SENDGUILDCARD));
    p.header.command = 0x0062;
    p.header.flag = c->clientID;
    p.header.size = 0x0114;
    p.subcommand = 0x06;
    p.subsize = 0x43;
    p.unused = 0x0000;
    //p.data.playerTag = 0x00000100;
    p.data.serialNumber = source->license.serialNumber;
    wcscpy(p.data.name,source->playerInfo.disp.playername);
    wcscpy(p.data.teamname,source->playerInfo.teamname);
    wcscpy(p.data.desc,source->playerInfo.guildcarddesc);
    p.data.reserved1 = 1;
    p.data.reserved2 = 1;
    p.data.sectionID = source->playerInfo.disp.sectionID;
    p.data.charClass = source->playerInfo.disp.charClass;
    operation_unlock(source);
    return SendCommandToClient(c,&p);
}

int (*CommandSendGuildCard_Functions[])(CLIENT* c,CLIENT* source) = {CommandSendGuildCard_GC,CommandSendGuildCard_BB,/*CommandSendGuildCard_PC*/NULL,/*CommandSendGuildCard_DCGC*/NULL,NULL,NULL};
int CommandSendGuildCard(CLIENT* c,CLIENT* source)
{
    if (!c || !source) return 5;
    if (!CommandSendGuildCard_Functions[c->version]) return 20;
    return CommandSendGuildCard_Functions[c->version](c,source);
}

////////////////////////////////////////////////////////////////////////////////
// CommandShipSelect: presents the player with a ship select menu.
// see the SHIP_SELECT_MENU structure for more information on customizing these menus and on how this function returns menu choices.

//int CommandShipSelect_DC(CLIENT* c,bool startServer)

int CommandShipSelect_PC(CLIENT* c,SHIP_SELECT_MENU* menu,DWORD* selection)
{
    COMMAND_PC_SHIPSELECT* p = (COMMAND_PC_SHIPSELECT*)malloc(0x30); // 4 + 0x2C(entries) 
    if (!p) return 1;
    p->header.size = 0x0030;
    p->header.command = (menu->flags & MENU_FLAG_IS_INFO_MENU) ? 0x1F : 0x07;
    p->header.flag = 0x00;
    p->entry[0].menuID = 0xFFFFFFFF;
    p->entry[0].itemID = 0xFFFFFFFF;
    p->entry[0].flags = 0x0004;
    wcscpy(p->entry[0].text,menu->name);

    int x,numitems = 0;
    for (x = 0; x < 30; x++)
    {
        if (!menu->items[x]) continue; // there's no item here 
        if (menu->itemFlags[x] & MENU_ITEM_FLAG_INVISIBLE) continue; // item is invisible 
        if ((menu->itemFlags[x] & MENU_ITEM_FLAG_REQ_MESSAGEBOX) && (c->flags & FLAG_BOXES_DISABLED)) continue; // item requires message boxes, which this client has disabled 
        if (((menu->itemFlags[x] & MENU_ITEM_FLAG_VERSION_MASK) >> c->version) & 1) continue; // item is not available on this client's version 
        p->header.flag++;
        p->header.size += 0x2C;
        p = (COMMAND_PC_SHIPSELECT*)realloc(p,p->header.size);
        if (!p) return 1;
        p->entry[numitems + 1].menuID = 0xFFFFFFFF;
        p->entry[numitems + 1].itemID = (menu->flags & MENU_FLAG_USE_ITEM_IDS) ? menu->itemIDs[x] : x;
        p->entry[numitems + 1].flags = 0x0F04;
        wcscpy(p->entry[numitems + 1].text,menu->items[x]);
        numitems++;
    }
    if ((numitems == 1) && (menu->flags & MENU_FLAG_AUTOSELECT_ONE_CHOICE))
    {
        *selection = (menu->flags & MENU_FLAG_USE_ITEM_IDS) ? menu->itemIDs[p->entry[1].itemID] : p->entry[1].itemID;
        free(p);
        return 0;
    } else {
        int errors = SendCommandToClient(c,p);
        free(p);
        return errors;
    }
    return 5438493;
}

int CommandShipSelect_GC(CLIENT* c,SHIP_SELECT_MENU* menu,DWORD* selection)
{
    COMMAND_GC_SHIPSELECT* p = (COMMAND_GC_SHIPSELECT*)malloc(0x20); // 4 + 0x2C(entries) 
    if (!p) return 1;
    p->header.command = (menu->flags & MENU_FLAG_IS_INFO_MENU) ? 0x1F : 0x07;
    p->header.flag = 0x00;
    p->header.size = 0x0020;
    p->entry[0].menuID = 0xFFFFFFFF;
    p->entry[0].itemID = 0xFFFFFFFF;
    p->entry[0].flags = 0x0004;
    tx_convert_to_sjis(p->entry[0].text,menu->name);

    int x,numitems = 0;
    for (x = 0; x < 30; x++)
    {
        if (!menu->items[x]) continue; // there's no item here 
        if (menu->itemFlags[x] & MENU_ITEM_FLAG_INVISIBLE) continue; // item is invisible 
        if ((menu->itemFlags[x] & MENU_ITEM_FLAG_REQ_MESSAGEBOX) && (c->flags & FLAG_BOXES_DISABLED)) continue; // item requires message boxes, which this client has disabled 
        if (((menu->itemFlags[x] & MENU_ITEM_FLAG_VERSION_MASK) >> c->version) & 1) continue; // item is not available on this client's version 
        p->header.flag++;
        p->header.size += 0x1C;
        p = (COMMAND_GC_SHIPSELECT*)realloc(p,p->header.size);
        if (!p) return 1;
        p->entry[numitems + 1].menuID = 0xFFFFFFFF;
        p->entry[numitems + 1].itemID = (menu->flags & MENU_FLAG_USE_ITEM_IDS) ? menu->itemIDs[x] : x;
        p->entry[numitems + 1].flags = 0x0F04;
        tx_convert_to_sjis(p->entry[numitems + 1].text,menu->items[x]);
        numitems++;
    }
    if ((numitems == 1) && (menu->flags & MENU_FLAG_AUTOSELECT_ONE_CHOICE))
    {
        *selection = (menu->flags & MENU_FLAG_USE_ITEM_IDS) ? menu->itemIDs[p->entry[1].itemID] : p->entry[1].itemID;
        free(p);
        return 0;
    } else {
        int errors = SendCommandToClient(c,p);
        free(p);
        return errors;
    }
    return 5438493;
}

int CommandShipSelect_BB(CLIENT* c,SHIP_SELECT_MENU* menu,DWORD* selection)
{
    COMMAND_BB_SHIPSELECT* p = (COMMAND_BB_SHIPSELECT*)malloc(0x34); // 4 + 0x2C(entries) 
    if (!p) return 1;
    p->header.size = 0x0034;
    p->header.command = (menu->flags & MENU_FLAG_IS_INFO_MENU) ? 0x001F : 0x0007;
    p->header.flag = 0x00000000;
    p->entry[0].menuID = 0xFFFFFFFF;
    p->entry[0].itemID = 0xFFFFFFFF;
    p->entry[0].flags = 0x0004;
    wcscpy(p->entry[0].text,menu->name);

    int x,numitems = 0;
    for (x = 0; x < 30; x++)
    {
        if (!menu->items[x]) continue; // there's no item here 
        if (menu->itemFlags[x] & MENU_ITEM_FLAG_INVISIBLE) continue; // item is invisible 
        if ((menu->itemFlags[x] & MENU_ITEM_FLAG_REQ_MESSAGEBOX) && (c->flags & FLAG_BOXES_DISABLED)) continue; // item requires message boxes, which this client has disabled 
        if (((menu->itemFlags[x] & MENU_ITEM_FLAG_VERSION_MASK) >> c->version) & 1) continue; // item is not available on this client's version 
        p->header.flag++;
        p->header.size += 0x2C;
        p = (COMMAND_BB_SHIPSELECT*)realloc(p,p->header.size);
        if (!p) return 1;
        p->entry[numitems + 1].menuID = 0xFFFFFFFF;
        p->entry[numitems + 1].itemID = (menu->flags & MENU_FLAG_USE_ITEM_IDS) ? menu->itemIDs[x] : x;
        p->entry[numitems + 1].flags = 0x0F04;
        wcscpy(p->entry[numitems + 1].text,menu->items[x]);
        numitems++;
    }
    if ((numitems == 1) && (menu->flags & MENU_FLAG_AUTOSELECT_ONE_CHOICE))
    {
        *selection = (menu->flags & MENU_FLAG_USE_ITEM_IDS) ? menu->itemIDs[p->entry[1].itemID] : p->entry[1].itemID;
        free(p);
        return 0;
    } else {
        int errors = SendCommandToClient(c,p);
        free(p);
        return errors;
    }
    return 5438493;
}

int CommandShipSelect_GetResponse(SERVER* s,CLIENT* c,SHIP_SELECT_MENU* menu,DWORD* selection)
{
    unsigned int x,errors = 0;
    wchar_t infobuffer[0x200];
    while (!errors && (*selection == 0xFFFFFFFF))
    {
        errors = ProcessCommands(s,c,0x09,0x10,0);
        if (!(menu->flags & MENU_FLAG_USE_ITEM_IDS) && (c->lastMenuSelection > 30)) return 7584;
        if (c->lastMenuSelectionType) // info request 
        {
            if (menu->flags & MENU_FLAG_USE_ITEM_IDS) // lastMenuSelection is an item ID 
            {
                for (x = 0; x < 30; x++) if (menu->itemIDs[x] == c->lastMenuSelection) break;
                if (x < 30) // is it in the menu? if so, get its description 
                {
                    if (menu->descriptions[x]) wcscpy(infobuffer,menu->descriptions[x]);
                    else menu->GetDescription(c->lastMenuSelection,infobuffer);
                } else infobuffer[0] = 0; // if not, no description
            } else { // lastMenuSelection is an item index 
                if (menu->descriptions[c->lastMenuSelection]) wcscpy(infobuffer,menu->descriptions[c->lastMenuSelection]);
                else menu->GetDescription(c->lastMenuSelection,infobuffer);
            }
            if (infobuffer[0]) CommandShipInfo(NULL,NULL,c,infobuffer); // if we got a description, show it 
        } else *selection = c->lastMenuSelection;
    }
    return errors;
}

int (*CommandShipSelect_Functions[])(CLIENT* c,SHIP_SELECT_MENU* menu,DWORD* selection) = {CommandShipSelect_GC,CommandShipSelect_BB,CommandShipSelect_PC,/*CommandShipSelect_DC*/NULL,NULL,NULL};
int CommandShipSelect(SERVER* s,CLIENT* c,SHIP_SELECT_MENU* menu,DWORD* selection)
{
    if (!s || !c || !menu || !selection) return 5;
    *selection = 0xFFFFFFFF;
    if (!CommandShipSelect_Functions[c->version]) return 20;
    int errors = CommandShipSelect_Functions[c->version](c,menu,selection);
    if (errors || (*selection != 0xFFFFFFFF)) return errors;
    return CommandShipSelect_GetResponse(s,c,menu,selection);
}

////////////////////////////////////////////////////////////////////////////////
// CommandGameSelect: presents the player with a Game Select menu. returns the selection in the same way as CommandShipSelect.

//int CommandGameSelect_DC(CLIENT* c,bool startServer)

int CommandGameSelect_PC(SERVER* s,CLIENT* c)
{
    COMMAND_PC_GAMESELECT* p = (COMMAND_PC_GAMESELECT*)malloc(0x30); // 4 + 0x2C(entries) 
    if (!p) return 1;
    memset(p,0,0x30);
    p->header.size = 0x0030;
    p->header.command = 0x08;
    p->header.flag = 0x00;
    p->entry[0].menuID = 0xFFFFFFFF;
    p->entry[0].gameID = 0xFFFFFFFF;

    operation_lock(s);
    tx_convert_to_unicode(p->entry[0].name,s->name);

    unsigned int x;
    for (x = 0; x < s->numLobbies; x++)
    {
        if (s->lobbies[x]->flags & LOBBY_FLAG_DEFAULT) continue;
        if (s->lobbies[x]->version != c->version) continue;
        p->header.size += 0x2C;
        p->header.flag++;
        p = (COMMAND_PC_GAMESELECT*)realloc(p,p->header.size);
        if (!p)
        {
            operation_unlock(s);
            return 1;
        }
        p->entry[p->header.flag].menuID = 0xFFFFFFFF;
        p->entry[p->header.flag].gameID = s->lobbies[x]->lobbyID;
        p->entry[p->header.flag].difficultyTag = (s->lobbies[x]->difficulty + 0x22);
        operation_lock(s->lobbies[x]);
        p->entry[p->header.flag].numPlayers = CountLobbyPlayers(s->lobbies[x]);
        operation_unlock(s->lobbies[x]);
        p->entry[p->header.flag].episode = 0x00;//s->lobbies[x]->episode + 0x40;
        p->entry[p->header.flag].flags = (s->lobbies[x]->mode << 4) | (s->lobbies[x]->password[0] ? 2 : 0);
        wcscpy(p->entry[p->header.flag].name,s->lobbies[x]->name);
    }
    operation_unlock(s);
    int errors = SendCommandToClient(c,p);
    free(p);
    return errors;
}

int CommandGameSelect_GC(SERVER* s,CLIENT* c)
{
    COMMAND_GC_GAMESELECT* p = (COMMAND_GC_GAMESELECT*)malloc(0x20); // 4 + 0x2C(entries) 
    if (!p) return 1;
    memset(p,0,0x20);
    p->header.size = 0x0020;
    p->header.command = 0x08;
    p->header.flag = 0x00;
    p->entry[0].menuID = 0xFFFFFFFF;
    p->entry[0].gameID = 0xFFFFFFFF;
    p->entry[0].flags = 0x0004;

    operation_lock(s);
    strcpy(p->entry[0].name,s->name);

    unsigned int x;
    for (x = 0; x < s->numLobbies; x++)
    {
        if (s->lobbies[x]->flags & LOBBY_FLAG_DEFAULT) continue;
        //if (s->lobbies[x]->version != c->version) continue;
        p->header.size += 0x1C;
        p->header.flag++;
        p = (COMMAND_GC_GAMESELECT*)realloc(p,p->header.size);
        if (!p)
        {
            operation_unlock(s);
            return 1;
        }
        p->entry[p->header.flag].menuID = 0xFFFFFFFF;
        p->entry[p->header.flag].gameID = s->lobbies[x]->lobbyID;
        p->entry[p->header.flag].difficultyTag = ((s->lobbies[x]->flags & LOBBY_FLAG_EP3) ? 0x0A : s->lobbies[x]->difficulty + 0x22);
        operation_lock(s->lobbies[x]);
        p->entry[p->header.flag].numPlayers = CountLobbyPlayers(s->lobbies[x]);
        operation_unlock(s->lobbies[x]);
        p->entry[p->header.flag].episode = 0x00; // unused by PSOGC? weird! 
        if (s->lobbies[x]->flags & LOBBY_FLAG_EP3) p->entry[p->header.flag].flags = (s->lobbies[x]->password[0] ? 2 : 0);
        else p->entry[p->header.flag].flags = ((s->lobbies[x]->episode << 6) | (s->lobbies[x]->mode << 4) | (s->lobbies[x]->password[0] ? 2 : 0));
        tx_convert_to_sjis(p->entry[p->header.flag].name,s->lobbies[x]->name);
    }
    operation_unlock(s);
    int errors = SendCommandToClient(c,p);
    free(p);
    return errors;
}

int CommandGameSelect_BB(SERVER* s,CLIENT* c)
{
    COMMAND_BB_GAMESELECT* p = (COMMAND_BB_GAMESELECT*)malloc(0x34); // 4 + 0x2C(entries) 
    if (!p) return 1;
    memset(p,0,0x34);
    p->header.size = 0x0034;
    p->header.command = 0x08;
    p->header.flag = 0x00;
    p->entry[0].menuID = 0xFFFFFFFF;
    p->entry[0].gameID = 0xFFFFFFFF;
    p->entry[0].flags = 0x0004;

    operation_lock(s);
    tx_convert_to_unicode(p->entry[0].name,s->name);

    unsigned int x;
    for (x = 0; x < s->numLobbies; x++)
    {
        if (s->lobbies[x]->flags & LOBBY_FLAG_DEFAULT) continue;
        if (s->lobbies[x]->version != c->version) continue;
        p->header.size += 0x2C;
        p->header.flag++;
        p = (COMMAND_BB_GAMESELECT*)realloc(p,p->header.size);
        if (!p)
        {
            operation_unlock(s);
            return 1;
        }
        p->entry[p->header.flag].menuID = 0xFFFFFFFF;
        p->entry[p->header.flag].gameID = s->lobbies[x]->lobbyID;
        p->entry[p->header.flag].difficultyTag = (s->lobbies[x]->flags & LOBBY_FLAG_GAME) ? (s->lobbies[x]->difficulty + 0x22) : 0x0C;
        operation_lock(s->lobbies[x]);
        p->entry[p->header.flag].numPlayers = CountLobbyPlayers(s->lobbies[x]);
        operation_unlock(s->lobbies[x]);
        p->entry[p->header.flag].episode = (s->lobbies[x]->maxClients << 4) | s->lobbies[x]->episode;
        p->entry[p->header.flag].flags = ((s->lobbies[x]->mode % 3) << 4) | (s->lobbies[x]->password[0] ? 2 : 0) | ((s->lobbies[x]->mode == 3) ? 4 : 0);
        wcscpy(p->entry[p->header.flag].name,s->lobbies[x]->name);
    }
    operation_unlock(s);
    int errors = SendCommandToClient(c,p);
    free(p);
    return errors;
}

int CommandGameSelect_GetResponse(SERVER* s,CLIENT* c,DWORD* selection)
{
    int errors = 0;
    wchar_t infobuffer[0x200];
    LOBBY* l;
    while (!errors && !(*selection))
    {
        errors = ProcessCommands(s,c,0x09,0x10,0x60,0x62,0x6C,0x6D,0xC9,0xCB,0);
        if (c->lastMenuSelectionType > 0) // info request 
        {
            l = FindLobby(s,c->lastMenuSelection);
            if (!l) CommandShipInfo(NULL,NULL,c,L"$C6This game is no\nlonger active."); // if we got a description, show it 
            else {
                swprintf(infobuffer,L"$C7%s\nGame ID: %08X\nVersion %d",l->name,l->lobbyID,l->version);
                CommandShipInfo(NULL,NULL,c,infobuffer);
            }
        } else if (c->lastMenuSelectionType == 0) *selection = c->lastMenuSelection;
        else return 0; // client backed out of game select 
    }
    return errors;
}

int (*CommandGameSelect_Functions[])(SERVER* s,CLIENT* c) = {CommandGameSelect_GC,CommandGameSelect_BB,CommandGameSelect_PC,/*CommandGameSelect_DC*/NULL,NULL,NULL};
int CommandGameSelect(SERVER* s,CLIENT* c,DWORD* selection)
{
    if (!s || !c || !selection) return 5;
    *selection = 0;
    if (!CommandGameSelect_Functions[c->version]) return 20;
    int errors = CommandGameSelect_Functions[c->version](s,c);
    if (errors) return errors;
    return CommandGameSelect_GetResponse(s,c,selection);
}

////////////////////////////////////////////////////////////////////////////////
// CommandShipSelectAsQuestSelect: presents the user with a quest select menu based on a ship select menu structure.

//int CommandShipSelectAsQuestSelect_DC(CLIENT* c,bool startServer)

int CommandShipSelectAsQuestSelect_PC(CLIENT* c,SHIP_SELECT_MENU* menu)
{
    COMMAND_PC_QUESTSELECT* p = (COMMAND_PC_QUESTSELECT*)malloc(4);
    if (!p) return 1;
    p->header.size = 4;
    p->header.command = (menu->flags & MENU_FLAG_IS_DLQ_MENU) ? 0xA4 : 0xA2;
    p->header.flag = 0x00;

    int x;
    for (x = 0; x < 30; x++)
    {
        if (!menu->items[x]) continue; // there's no item here 
        if (menu->itemFlags[x] & MENU_ITEM_FLAG_INVISIBLE) continue; // item is invisible 
        if ((menu->itemFlags[x] & MENU_ITEM_FLAG_REQ_MESSAGEBOX) && (c->flags & FLAG_BOXES_DISABLED)) continue; // item requires message boxes, which this client has disabled 
        if (((menu->itemFlags[x] & MENU_ITEM_FLAG_VERSION_MASK) >> c->version) & 1) continue; // item is not available on this client's version 
        p->header.size += 0x128;
        p = (COMMAND_PC_QUESTSELECT*)realloc(p,p->header.size);
        if (!p) return 1;
        p->entry[p->header.flag].menuID = 0xFFFFFFFF;
        p->entry[p->header.flag].questID = (menu->flags & MENU_FLAG_USE_ITEM_IDS) ? menu->itemIDs[x] : x;
        wcscpy(p->entry[p->header.flag].name,menu->items[x]);
        if (menu->descriptions[x]) wcscpy(p->entry[p->header.flag].shortDesc,menu->descriptions[x]);
        else if (menu->GetDescription) menu->GetDescription(c->lastMenuSelection,p->entry[p->header.flag].shortDesc);
        else p->entry[p->header.flag].shortDesc[0] = 0;
        tx_add_color(p->entry[p->header.flag].shortDesc);
        p->header.flag++;
    }
    int errors = SendCommandToClient(c,p);
    free(p);
    return errors;
}

int CommandShipSelectAsQuestSelect_GC(CLIENT* c,SHIP_SELECT_MENU* menu)
{
    COMMAND_GC_QUESTSELECT* p = (COMMAND_GC_QUESTSELECT*)malloc(4);
    if (!p) return 1;
    p->header.size = 4;
    p->header.command = (menu->flags & MENU_FLAG_IS_DLQ_MENU) ? 0xA4 : 0xA2;
    p->header.flag = 0x00;

    int x;
    wchar_t descbuffer[0x70];
    for (x = 0; x < 30; x++)
    {
        if (!menu->items[x]) continue; // there's no item here 
        if (menu->itemFlags[x] & MENU_ITEM_FLAG_INVISIBLE) continue; // item is invisible 
        if ((menu->itemFlags[x] & MENU_ITEM_FLAG_REQ_MESSAGEBOX) && (c->flags & FLAG_BOXES_DISABLED)) continue; // item requires message boxes, which this client has disabled 
        if (((menu->itemFlags[x] & MENU_ITEM_FLAG_VERSION_MASK) >> c->version) & 1) continue; // item is not available on this client's version 
        p->header.size += 0x98;
        p = (COMMAND_GC_QUESTSELECT*)realloc(p,p->header.size);
        if (!p) return 1;
        p->entry[p->header.flag].menuID = 0xFFFFFFFF;
        p->entry[p->header.flag].questID = (menu->flags & MENU_FLAG_USE_ITEM_IDS) ? menu->itemIDs[x] : x;
        tx_convert_to_sjis(p->entry[p->header.flag].name,menu->items[x]);
        if (menu->descriptions[x]) tx_convert_to_sjis(p->entry[p->header.flag].shortDesc,menu->descriptions[x]);
        else if (menu->GetDescription)
        {
            menu->GetDescription(c->lastMenuSelection,descbuffer);
            tx_convert_to_sjis(p->entry[p->header.flag].shortDesc,descbuffer);
        } else p->entry[p->header.flag].shortDesc[0] = 0;
        tx_add_color(p->entry[p->header.flag].shortDesc);
        p->header.flag++;
    }
    int errors = SendCommandToClient(c,p);
    free(p);
    return errors;
}

int CommandShipSelectAsQuestSelect_BB(CLIENT* c,SHIP_SELECT_MENU* menu)
{
    COMMAND_BB_QUESTSELECT* p = (COMMAND_BB_QUESTSELECT*)malloc(8);
    if (!p) return 1;
    p->header.size = 8;
    p->header.command = (menu->flags & MENU_FLAG_IS_DLQ_MENU) ? 0x00A4 : 0x00A2;
    p->header.flag = 0x00;

    int x;
    for (x = 0; x < 30; x++)
    {
        if (!menu->items[x]) continue; // there's no item here 
        if (menu->itemFlags[x] & MENU_ITEM_FLAG_INVISIBLE) continue; // item is invisible 
        if ((menu->itemFlags[x] & MENU_ITEM_FLAG_REQ_MESSAGEBOX) && (c->flags & FLAG_BOXES_DISABLED)) continue; // item requires message boxes, which this client has disabled 
        if (((menu->itemFlags[x] & MENU_ITEM_FLAG_VERSION_MASK) >> c->version) & 1) continue; // item is not available on this client's version 
        p->header.size += 0x13C;
        p = (COMMAND_BB_QUESTSELECT*)realloc(p,p->header.size);
        if (!p) return 1;
        p->entry[p->header.flag].menuID = 0xFFFFFFFF;
        p->entry[p->header.flag].questID = (menu->flags & MENU_FLAG_USE_ITEM_IDS) ? menu->itemIDs[x] : x;
        wcscpy(p->entry[p->header.flag].name,menu->items[x]);
        if (menu->descriptions[x]) wcscpy(p->entry[p->header.flag].shortDesc,menu->descriptions[x]);
        else if (menu->GetDescription) menu->GetDescription(c->lastMenuSelection,p->entry[p->header.flag].shortDesc);
        else p->entry[p->header.flag].shortDesc[0] = 0;
        tx_add_color(p->entry[p->header.flag].shortDesc);
        p->header.flag++;
    }
    int errors = SendCommandToClient(c,p);
    free(p);
    return errors;
}

int CommandShipSelectAsQuestSelect_GetResponse(SERVER* s,CLIENT* c,SHIP_SELECT_MENU* menu,DWORD* selection)
{
    int errors = ProcessCommands(s,c,0x09,0x10,0);
    if (!(menu->flags & MENU_FLAG_USE_ITEM_IDS) && (c->lastMenuSelection > 30)) return 7584;
    if (c->lastMenuSelectionType != (-1)) *selection = c->lastMenuSelection;
    return errors;
}

int (*CommandShipSelectAsQuestSelect_Functions[])(CLIENT* c,SHIP_SELECT_MENU* menu) = {CommandShipSelectAsQuestSelect_GC,CommandShipSelectAsQuestSelect_BB,CommandShipSelectAsQuestSelect_PC,/*CommandShipSelectAsQuestSelect_DC*/NULL,NULL,NULL};
int CommandShipSelectAsQuestSelect(SERVER* s,CLIENT* c,SHIP_SELECT_MENU* menu,DWORD* selection)
{
    if (!s || !c || !menu || !selection) return 5;
    *selection = 0xFFFFFFFF;
    if (!CommandShipSelectAsQuestSelect_Functions[c->version]) return 20;
    int errors = CommandShipSelectAsQuestSelect_Functions[c->version](c,menu);
    if (errors) return errors;
    return CommandShipSelectAsQuestSelect_GetResponse(s,c,menu,selection);
}

////////////////////////////////////////////////////////////////////////////////
// CommandQuestSelect: presents the user with a quest select menu based on a quest list.

//int CommandQuestSelect_DC(CLIENT* c,bool startServer)

int CommandQuestSelect_PC(CLIENT* c,QUESTLIST* menu)
{
    DWORD size = 4 + (menu->numQuests * 0x128);
    COMMAND_PC_QUESTSELECT* p = (COMMAND_PC_QUESTSELECT*)malloc(size);
    if (!p) return 1;
    p->header.size = size;
    p->header.command = (menu->flags & MENU_FLAG_IS_DLQ_MENU) ? 0xA4 : 0xA2;
    p->header.flag = menu->numQuests;

    unsigned int x;
    for (x = 0; x < menu->numQuests; x++)
    {
        p->entry[x].menuID = 0xFFFFFFFF;
        p->entry[x].questID = menu->quests[x].questID;
        wcscpy(p->entry[x].name,menu->quests[x].name);
        if (menu->quests[x].shortdesc) wcscpy(p->entry[x].shortDesc,menu->quests[x].shortdesc);
        else p->entry[x].shortDesc[0] = 0;
        tx_add_color(p->entry[x].shortDesc);
    }
    int errors = SendCommandToClient(c,p);
    free(p);
    return errors;
}

int CommandQuestSelect_GC(CLIENT* c,QUESTLIST* menu)
{
    DWORD size = 4 + (menu->numQuests * 0x98);
    COMMAND_GC_QUESTSELECT* p = (COMMAND_GC_QUESTSELECT*)malloc(size);
    if (!p) return 1;
    p->header.size = size;
    p->header.command = (menu->flags & MENU_FLAG_IS_DLQ_MENU) ? 0xA4 : 0xA2;
    p->header.flag = menu->numQuests;

    unsigned int x;
    for (x = 0; x < menu->numQuests; x++)
    {
        p->entry[x].menuID = 0xFFFFFFFF;
        p->entry[x].questID = menu->quests[x].questID;
        tx_convert_to_sjis(p->entry[x].name,menu->quests[x].name);
        if (menu->quests[x].shortdesc) tx_convert_to_sjis(p->entry[x].shortDesc,menu->quests[x].shortdesc);
        else p->entry[x].shortDesc[0] = 0;
        tx_add_color(p->entry[x].shortDesc);
    }
    int errors = SendCommandToClient(c,p);
    free(p);
    return errors;
}

int CommandQuestSelect_BB(CLIENT* c,QUESTLIST* menu)
{
    DWORD size = 8 + (menu->numQuests * 0x13C);
    COMMAND_BB_QUESTSELECT* p = (COMMAND_BB_QUESTSELECT*)malloc(size);
    if (!p) return 1;
    p->header.size = size;
    p->header.command = (menu->flags & MENU_FLAG_IS_DLQ_MENU) ? 0xA4 : 0xA2;
    p->header.flag = menu->numQuests;

    unsigned int x;
    for (x = 0; x < menu->numQuests; x++)
    {
        p->entry[x].menuID = 0xFFFFFFFF;
        p->entry[x].questID = menu->quests[x].questID;
        wcscpy(p->entry[x].name,menu->quests[x].name);
        if (menu->quests[x].shortdesc) wcscpy(p->entry[x].shortDesc,menu->quests[x].shortdesc);
        else p->entry[x].shortDesc[0] = 0;
        tx_add_color(p->entry[x].shortDesc);
    }
    int errors = SendCommandToClient(c,p);
    free(p);
    return errors;
}

int CommandQuestSelect_GetResponse(SERVER* s,CLIENT* c,QUESTLIST* menu,QUEST** selection)
{
    int errors = 0;
    while (!errors && !(*selection))
    {
        errors = ProcessCommands(s,c,0x09,0x10,0xA9,0);
        if (c->lastMenuSelectionType > 0) // info request 
        {
            *selection = FindQuest(menu,c->lastMenuSelection);
            if (*selection) CommandQuestInfo(c,(*selection)->description);
            else return 48674;
            *selection = NULL;
        } else if (c->lastMenuSelectionType == 0) *selection = FindQuest(menu,c->lastMenuSelection);
        else return 0; // client backed out of quest select 
    }
    return errors;
}

int (*CommandQuestSelect_Functions[])(CLIENT* c,QUESTLIST* menu) = {CommandQuestSelect_GC,CommandQuestSelect_BB,CommandQuestSelect_PC,/*CommandQuestSelect_DC*/NULL,NULL,NULL};
int CommandQuestSelect(SERVER* s,CLIENT* c,QUESTLIST* menu,QUEST** selection)
{
    if (!s || !c || !menu || !selection) return 5;
    *selection = NULL;
    if (!CommandQuestSelect_Functions[c->version]) return 20;
    int errors = CommandQuestSelect_Functions[c->version](c,menu);
    if (errors) return errors;
    return CommandQuestSelect_GetResponse(s,c,menu,selection);
}

////////////////////////////////////////////////////////////////////////////////
// CommandSendLobbyList: sends the lobby list. this command is deprecated, as PSO
// expects it to be exactly how this server sends it, and does not react if it's
// different, except by changing the lobby IDs.

int CommandSendLobbyList_DCPCGC(SERVER* s,CLIENT* c)
{
    unsigned int x,num = 0;
    COMMAND_DCPCGC_LOBBYLIST* p = (COMMAND_DCPCGC_LOBBYLIST*)malloc(0x200);
    operation_lock(s);
    if (c->version == VERSION_PC)
    {
        p->headerpc.size = 0x0004;
        p->headerpc.command = 0x83;
        p->headerpc.flag = 0x00;
    } else {
        p->headerdcgc.command = 0x83;
        p->headerdcgc.flag = 0x00;
        p->headerdcgc.size = 0x0004;
    }
    for (x = 0; x < s->numLobbies; x++)
    {
        if (!(s->lobbies[x]->flags & LOBBY_FLAG_DEFAULT)) continue;
        if (!(c->flags & FLAG_V4_GAMES) && (s->lobbies[x]->flags & LOBBY_FLAG_EP3)) continue;
        p->entry[num].menuID = 0xFFFFFFFF;
        p->entry[num].itemID = s->lobbies[x]->lobbyID;
        p->entry[num].unused = 0;
        if (c->version == VERSION_PC)
        {
            p->headerpc.size += 0x000C;
            p->headerpc.flag++;
        } else {
            p->headerdcgc.flag++;
            p->headerdcgc.size += 0x000C;
        }
        num++;
    }
    operation_unlock(s);
    int rv = SendCommandToClient(c,p);
    free(p);
    return rv;
}

int CommandSendLobbyList_BB(SERVER* s,CLIENT* c)
{
    unsigned int x;
    COMMAND_BB_LOBBYLIST* p = (COMMAND_BB_LOBBYLIST*)malloc(0x200);
    operation_lock(s);
    p->header.size = 0x0004;
    p->header.command = 0x0083;
    p->header.flag = 0x00000000;
    for (x = 0; x < s->numLobbies; x++)
    {
        if (!(s->lobbies[x]->flags & LOBBY_FLAG_DEFAULT)) continue;
        if (s->lobbies[x]->flags & LOBBY_FLAG_EP3) continue;
        p->entry[p->header.flag].menuID = 0xFFFFFFFF;
        p->entry[p->header.flag].itemID = s->lobbies[x]->lobbyID;
        p->entry[p->header.flag].unused = 0;
        p->header.size += 0x000C;
        p->header.flag++;
    }
    operation_unlock(s);
    int rv = SendCommandToClient(c,p);
    free(p);
    return rv;
}

int (*CommandSendLobbyList_Functions[])(SERVER* s,CLIENT* c) = {CommandSendLobbyList_DCPCGC,CommandSendLobbyList_BB,CommandSendLobbyList_DCPCGC,CommandSendLobbyList_DCPCGC,NULL,NULL};
int CommandSendLobbyList(SERVER* s,CLIENT* c)
{
    if (!s || !c) return 5;
    if (!CommandSendLobbyList_Functions[c->version]) return 20;
    return CommandSendLobbyList_Functions[c->version](s,c);
}

//int CommandLobbyUpdateChallengeRank(SERVER* s,CLIENT* c)

////////////////////////////////////////////////////////////////////////////////
// CommandLobbyJoin: tells a client that it's joining a game or lobby.
// the game join and lobby join commands are quite different; since the server has
// just one structure for both, this function automatically decides which command
// to use based on the lobby's flags.

//int CommandGameJoin_DC(LOBBY* l,CLIENT* c)

int CommandGameJoin_PC(LOBBY* l,CLIENT* c)
{
    int x;
    COMMAND_PC_JOINGAME* p = (COMMAND_PC_JOINGAME*)malloc(0x0154);
    if (!p) return 1;
    p->header.command = 0x64;
    p->header.flag = 0x00;
    p->header.size = 0x0154;
    operation_lock(l);
    memcpy(p->variations,l->variations,0x80);
    for (x = 0; x < 4; x++)
    {
        if (!l->clients[x]) memset(&p->lobbyData[x],0,sizeof(PC_PLAYER_LOBBY_DATA));
        else {
            p->lobbyData[x].playerTag = 0x00000100;
            p->lobbyData[x].guildcard = l->clients[x]->license.serialNumber;
            p->lobbyData[x].ipAddress = 0xFFFFFFFF;
            p->lobbyData[x].clientID = c->clientID;
            wcsncpy(p->lobbyData[x].playername,l->clients[x]->playerInfo.disp.playername,0x10);
            p->header.flag++;
        }
    }
    p->clientID = c->clientID;
    p->leaderID = l->leaderID;
    p->unused = 0x00;
    p->difficulty = l->difficulty;
    p->battleMode = (l->mode == 1) ? 1 : 0;
    p->event = l->event;
    p->sectionID = l->sectionID;
    p->challengeMode = (l->mode == 2) ? 1 : 0;
    p->gameID = l->lobbyID;
    p->episode = 0x00000100;
    operation_unlock(l);

    int rv = SendCommandToClient(c,p);
    free(p);
    return rv;
}

int CommandGameJoin_GC(LOBBY* l,CLIENT* c)
{
    int x;
    int size = (l->flags & LOBBY_FLAG_EP3) ? 0x1184 : 0x0114;
    COMMAND_GC_JOINGAME* p = (COMMAND_GC_JOINGAME*)malloc(size);
    if (!p) return 1;
    p->header.command = 0x64;
    p->header.flag = 0x00;
    p->header.size = size;
    operation_lock(l);
    memcpy(p->variations,l->variations,0x80);
    for (x = 0; x < 4; x++)
    {
        if (!l->clients[x]) memset(&p->lobbyData[x],0,sizeof(GC_PLAYER_LOBBY_DATA));
        else {
            p->lobbyData[x].playerTag = 0x00000100;
            p->lobbyData[x].guildcard = l->clients[x]->license.serialNumber;
            p->lobbyData[x].ipAddress = 0xFFFFFFFF;
            p->lobbyData[x].clientID = c->clientID;
            tx_convert_to_sjis(p->lobbyData[x].playername,l->clients[x]->playerInfo.disp.playername);
            if (l->flags & LOBBY_FLAG_EP3) ExportLobbyPlayerData(&p->entry[x].inventory,&l->clients[x]->playerInfo,VERSION_GAMECUBE);
            p->header.flag++;
        }
    }
    p->clientID = c->clientID;
    p->leaderID = l->leaderID;
    p->unused = 0x01;
    p->difficulty = l->difficulty;
    p->battleMode = (l->mode == 1) ? 1 : 0;
    p->event = l->event;
    p->sectionID = l->sectionID;
    p->challengeMode = (l->mode == 2) ? 1 : 0;
    p->gameID = l->lobbyID;
    p->episode = l->episode;
    operation_unlock(l);

    int rv = SendCommandToClient(c,p);
    free(p);
    return rv;
}

int CommandGameJoin_BB(LOBBY* l,CLIENT* c)
{
    int x,y;
    COMMAND_BB_JOINGAME* p = (COMMAND_BB_JOINGAME*)malloc(0x01A8);
    if (!p) return 1;
    p->header.command = 0x0064;
    p->header.flag = 0x00000000;
    p->header.size = 0x01A8;
    operation_lock(l);
    memcpy(p->variations,l->variations,0x80);
    for (x = 0; x < 4; x++)
    {
        if (!l->clients[x]) memset(&p->lobbyData[x],0,sizeof(BB_PLAYER_LOBBY_DATA));
        else {
            p->lobbyData[x].playerTag = 0x00000100;
            p->lobbyData[x].guildcard = l->clients[x]->license.serialNumber;
            for (y = 0; y < 5; y++) p->lobbyData[x].unknown1[y] = 0;
            p->lobbyData[x].clientID = c->clientID;
            wcsncpy(p->lobbyData[x].playername,l->clients[x]->playerInfo.disp.playername,0x10);
            p->lobbyData[x].unknown2 = 0;
            p->header.flag++;
        }
    }
    p->clientID = c->clientID;
    p->leaderID = l->leaderID;
    p->unused = 0x01;
    p->difficulty = l->difficulty;
    p->battleMode = (l->mode == 1) ? 1 : 0;
    p->event = l->event;
    p->sectionID = l->sectionID;
    p->challengeMode = (l->mode == 2) ? 1 : 0;
    p->gameID = l->lobbyID;
    p->episode = l->episode;
    p->unused2 = 0x01;
    p->solomode = (l->mode == 3) ? 1 : 0;
    p->unused3 = 0x00;
    operation_unlock(l);

    int rv = SendCommandToClient(c,p);
    free(p);
    return rv;
}

//int CommandLobbyJoin_DC(LOBBY* l,CLIENT* c)

int CommandLobbyJoin_PC(LOBBY* l,CLIENT* c)
{
    unsigned int x;
    COMMAND_PC_JOINLOBBY* p = (COMMAND_PC_JOINLOBBY*)malloc(0x0010);
    if (!p) return 1;
    p->header.command = 0x67;
    p->header.flag = 0x00;
    p->header.size = 0x0010;
    p->clientID = c->clientID;
    p->leaderID = l->leaderID;
    p->disableUDP = 0x01;
    p->lobbyNumber = (l->type > 14) ? (l->block - 1) : l->type; // CompatibleLobbyNumber
    p->blockNumber = l->block;
    p->event = l->event;
    p->unused = 0;

    operation_lock(l);
    for (x = 0; x < l->maxClients; x++)
    {
        if (!l->clients[x]) continue;
        p->header.size += (sizeof(PC_PLAYER_LOBBY_DATA) + sizeof(PLAYER_INVENTORY) + sizeof(PCGC_PLAYER_DISPDATA));
        p = (COMMAND_PC_JOINLOBBY*)realloc(p,p->header.size);
        operation_lock(l->clients[x]);
        p->entry[p->header.flag].lobbyData.playerTag = 0x00000100;
        p->entry[p->header.flag].lobbyData.guildcard = l->clients[x]->license.serialNumber;
        p->entry[p->header.flag].lobbyData.ipAddress = 0xFFFFFFFF;
        p->entry[p->header.flag].lobbyData.clientID = l->clients[x]->clientID;
        wcscpy(p->entry[p->header.flag].lobbyData.playername,l->clients[x]->playerInfo.disp.playername);
        ExportLobbyPlayerData(&p->entry[p->header.flag].data,&l->clients[x]->playerInfo,VERSION_PC);
        operation_unlock(l->clients[x]);
        p->header.flag++;
    }
    operation_unlock(l);

    int rv = SendCommandToClient(c,p);
    free(p);
    return rv;
}

int CommandLobbyJoin_GC(LOBBY* l,CLIENT* c)
{
    unsigned int x;
    COMMAND_GC_JOINLOBBY* p = (COMMAND_GC_JOINLOBBY*)malloc(0x0010);
    if (!p) return 1;
    p->header.command = 0x67;
    p->header.flag = 0x00;
    p->header.size = 0x0010;
    p->clientID = c->clientID;
    p->leaderID = l->leaderID;
    p->disableUDP = 0x01;
    p->lobbyNumber = l->type;
    if ((l->flags & LOBBY_FLAG_EP3) && (l->type > 0x14) && (l->type < 0xE9)) p->lobbyNumber = l->block - 1;
    if (!(l->flags & LOBBY_FLAG_EP3) && (l->type > 0x11) && (l->type != 0x67) && (l->type != 0xD4) && (l->type < 0xFC)) p->lobbyNumber = l->block - 1;
    p->blockNumber = l->block;
    p->event = l->event;
    p->unused = 0;

    operation_lock(l);
    for (x = 0; x < l->maxClients; x++)
    {
        if (!l->clients[x]) continue;
        p->header.size += (sizeof(GC_PLAYER_LOBBY_DATA) + sizeof(PLAYER_INVENTORY) + sizeof(PCGC_PLAYER_DISPDATA));
        p = (COMMAND_GC_JOINLOBBY*)realloc(p,p->header.size);
        operation_lock(l->clients[x]);
        p->entry[p->header.flag].lobbyData.playerTag = 0x00000100;
        p->entry[p->header.flag].lobbyData.guildcard = l->clients[x]->license.serialNumber;
        p->entry[p->header.flag].lobbyData.ipAddress = 0xFFFFFFFF;
        p->entry[p->header.flag].lobbyData.clientID = l->clients[x]->clientID;
        tx_convert_to_sjis(p->entry[p->header.flag].lobbyData.playername,l->clients[x]->playerInfo.disp.playername);
        ExportLobbyPlayerData(&p->entry[p->header.flag].data,&l->clients[x]->playerInfo,VERSION_GAMECUBE);
        operation_unlock(l->clients[x]);
        p->header.flag++;
    }
    operation_unlock(l);

    int rv = SendCommandToClient(c,p);
    free(p);
    return rv;
}

int CommandLobbyJoin_BB(LOBBY* l,CLIENT* c)
{
    unsigned int x;
    COMMAND_BB_JOINLOBBY* p = (COMMAND_BB_JOINLOBBY*)malloc(0x0014);
    if (!p) return 1;
    p->header.command = 0x67;
    p->header.flag = 0x00;
    p->header.size = 0x0014;
    p->clientID = c->clientID;
    p->leaderID = l->leaderID;
    p->unused = 0;
    p->lobbyNumber = (l->type > 14) ? (l->block - 1) : l->type; // CompatibleLobbyNumber
    p->blockNumber = l->block;
    p->event = l->event;
    p->unused2 = 0;

    operation_lock(l);
    for (x = 0; x < l->maxClients; x++)
    {
        if (!l->clients[x]) continue;
        p->header.size += (sizeof(BB_PLAYER_LOBBY_DATA) + sizeof(PLAYER_INVENTORY) + sizeof(BB_PLAYER_DISPDATA));
        p = (COMMAND_BB_JOINLOBBY*)realloc(p,p->header.size);
        operation_lock(l->clients[x]);
        memset(&p->entry[p->header.flag].lobbyData,0,sizeof(BB_PLAYER_LOBBY_DATA));
        p->entry[p->header.flag].lobbyData.playerTag = 0x00000100;
        p->entry[p->header.flag].lobbyData.guildcard = l->clients[x]->license.serialNumber;
        p->entry[p->header.flag].lobbyData.clientID = l->clients[x]->clientID;
        wcscpy(p->entry[p->header.flag].lobbyData.playername,l->clients[x]->playerInfo.disp.playername);
        ExportLobbyPlayerData(&p->entry[p->header.flag].data,&l->clients[x]->playerInfo,VERSION_BLUEBURST);
        operation_unlock(l->clients[x]);
        p->header.flag++;
    }
    operation_unlock(l);

    int rv = SendCommandToClient(c,p);
    free(p);
    return rv;
}

int (*CommandLobbyJoin_LobbyFunctions[])(LOBBY* l,CLIENT* c) = {CommandLobbyJoin_GC,CommandLobbyJoin_BB,CommandLobbyJoin_PC,/*CommandLobbyJoin_DC*/NULL,NULL,NULL};
int (*CommandLobbyJoin_GameFunctions[])(LOBBY* l,CLIENT* c) = {CommandGameJoin_GC,CommandGameJoin_BB,CommandGameJoin_PC,/*CommandGameJoin_DC*/NULL,NULL,NULL};
int CommandLobbyJoin(LOBBY* l,CLIENT* c,bool notifyOthers)
{
    if (!l || !c) return 5;
    int (**functions)(LOBBY* l,CLIENT* c);
    if (l->flags & LOBBY_FLAG_GAME) functions = CommandLobbyJoin_GameFunctions;
    else functions = CommandLobbyJoin_LobbyFunctions;
    if (!functions[c->version]) return 20;
    int errors = functions[c->version](l,c);
    if (errors) return errors;
    if (notifyOthers) errors = CommandLobbyAddPlayer(l,c);
    return errors;
}

////////////////////////////////////////////////////////////////////////////////
// CommandLobbyAddPlayer: notifies all players in a lobby that a new player is joining.
// this command, unlike the previous, is virtually the same between games and lobbies.

//void* DataCommandLobbyAddPlayer_DC(LOBBY* li,CLIENT* c)

void* DataCommandLobbyAddPlayer_PC(LOBBY* l,CLIENT* c)
{
    COMMAND_PC_JOINLOBBY* p = (COMMAND_PC_JOINLOBBY*)malloc(0x0010 + sizeof(PC_PLAYER_LOBBY_DATA) + sizeof(PLAYER_INVENTORY) + sizeof(PCGC_PLAYER_DISPDATA));
    p->header.command = (l->flags & LOBBY_FLAG_GAME) ? 0x65 : 0x68;
    p->header.flag = 0x01;
    p->header.size = 0x0010 + sizeof(PC_PLAYER_LOBBY_DATA) + sizeof(PLAYER_INVENTORY) + sizeof(PCGC_PLAYER_DISPDATA);
    p->clientID = 0xFF;
    p->leaderID = l->leaderID;
    p->disableUDP = 0x01;
    p->lobbyNumber = l->type; // CompatibleLobbyNumber
    p->blockNumber = l->block;
    p->event = l->event;
    p->unused = 0;
    operation_lock(c);
    p->entry[0].lobbyData.playerTag = 0x00000100;
    p->entry[0].lobbyData.guildcard = c->license.serialNumber;
    p->entry[0].lobbyData.ipAddress = 0xFFFFFFFF;
    p->entry[0].lobbyData.clientID = c->clientID;
    wcscpy(p->entry[0].lobbyData.playername,c->playerInfo.disp.playername);
    ExportLobbyPlayerData(&p->entry[0].data,&c->playerInfo,VERSION_PC);
    operation_unlock(c);
    return p;
}

void* DataCommandLobbyAddPlayer_GC(LOBBY* l,CLIENT* c)
{
    COMMAND_GC_JOINLOBBY* p = (COMMAND_GC_JOINLOBBY*)malloc(0x0010 + sizeof(GC_PLAYER_LOBBY_DATA) + sizeof(PLAYER_INVENTORY) + sizeof(PCGC_PLAYER_DISPDATA));
    p->header.command = (l->flags & LOBBY_FLAG_GAME) ? 0x65 : 0x68;
    p->header.flag = 0x01;
    p->header.size = 0x0010 + sizeof(GC_PLAYER_LOBBY_DATA) + sizeof(PLAYER_INVENTORY) + sizeof(PCGC_PLAYER_DISPDATA);
    p->clientID = 0xFF;
    p->leaderID = l->leaderID;
    p->disableUDP = 0x01;
    p->lobbyNumber = l->type; // CompatibleLobbyNumber
    p->blockNumber = l->block;
    p->event = l->event;
    p->unused = 0;
    operation_lock(c);
    p->entry[0].lobbyData.playerTag = 0x00000100;
    p->entry[0].lobbyData.guildcard = c->license.serialNumber;
    p->entry[0].lobbyData.ipAddress = 0xFFFFFFFF;
    p->entry[0].lobbyData.clientID = c->clientID;
    tx_convert_to_sjis(p->entry[0].lobbyData.playername,c->playerInfo.disp.playername);
    ExportLobbyPlayerData(&p->entry[0].data,&c->playerInfo,VERSION_GAMECUBE);
    operation_unlock(c);
    return p;
}

void* DataCommandLobbyAddPlayer_BB(LOBBY* l,CLIENT* c)
{
    COMMAND_BB_JOINLOBBY* p = (COMMAND_BB_JOINLOBBY*)malloc(0x0014 + sizeof(BB_PLAYER_LOBBY_DATA) + sizeof(PLAYER_INVENTORY) + sizeof(BB_PLAYER_DISPDATA));
    p->header.command = (l->flags & LOBBY_FLAG_GAME) ? 0x0065 : 0x0068;
    p->header.flag = 0x01;
    p->header.size = 0x0014 + sizeof(BB_PLAYER_LOBBY_DATA) + sizeof(PLAYER_INVENTORY) + sizeof(BB_PLAYER_DISPDATA);
    p->clientID = 0xFF;
    p->leaderID = l->leaderID;
    p->unused = 0;
    p->lobbyNumber = l->type; // CompatibleLobbyNumber
    p->blockNumber = l->block;
    p->event = l->event;
    p->unused2 = 0;
    memset(&p->entry[0].lobbyData,0,sizeof(BB_PLAYER_LOBBY_DATA));
    operation_lock(c);
    p->entry[0].lobbyData.playerTag = 0x00000100;
    p->entry[0].lobbyData.guildcard = c->license.serialNumber;
    p->entry[0].lobbyData.clientID = c->clientID;
    wcscpy(p->entry[0].lobbyData.playername,c->playerInfo.disp.playername);
    ExportLobbyPlayerData(&p->entry[0].data,&c->playerInfo,VERSION_BLUEBURST);
    operation_unlock(c);
    return p;
}

void* (*DataCommandLobbyAddPlayer_Functions[])(LOBBY* li,CLIENT* c) = {DataCommandLobbyAddPlayer_GC,DataCommandLobbyAddPlayer_BB,DataCommandLobbyAddPlayer_PC,/*DataCommandLobbyAddPlayer_DC*/NULL,NULL,NULL};
int CommandLobbyAddPlayer(LOBBY* l,CLIENT* c)
{
    if (!l || !c) return 5;
    void* data[VERSION_MAX];
    int x;
    for (x = 0; x < VERSION_MAX; x++)
    {
        if (!DataCommandLobbyAddPlayer_Functions[x]) data[x] = NULL;
        else data[x] = DataCommandLobbyAddPlayer_Functions[x](l,c);
    }
    int errors = SendCommandToLobby(l,c,data);
    for (x = 0; x < VERSION_MAX; x++) if (data[x]) free(data[x]);
    return errors;
}

////////////////////////////////////////////////////////////////////////////////
// CommandLobbyDeletePlayer: removes a player from a lobby or game.

void* DataCommandLobbyDeletePlayer_PC(LOBBY* l,int leaving)
{
    COMMAND_DCPCGC_LEAVELOBBY* p = (COMMAND_DCPCGC_LEAVELOBBY*)malloc(0x0008);
    p->headerpc.command = (l->flags & LOBBY_FLAG_GAME) ? 0x66 : 0x69;
    p->headerpc.flag = leaving;
    p->headerpc.size = 0x0008;
    p->clientID = leaving;
    p->leaderID = l->leaderID;
    return p;
}

void* DataCommandLobbyDeletePlayer_DCGC(LOBBY* l,int leaving)
{
    COMMAND_DCPCGC_LEAVELOBBY* p = (COMMAND_DCPCGC_LEAVELOBBY*)malloc(0x0008);
    p->headerdcgc.command = (l->flags & LOBBY_FLAG_GAME) ? 0x66 : 0x69;
    p->headerdcgc.flag = leaving;
    p->headerdcgc.size = 0x0008;
    p->clientID = leaving;
    p->leaderID = l->leaderID;
    return p;
}

void* DataCommandLobbyDeletePlayer_BB(LOBBY* l,int leaving)
{
    COMMAND_BB_LEAVELOBBY* p = (COMMAND_BB_LEAVELOBBY*)malloc(0x000C);
    p->header.command = (l->flags & LOBBY_FLAG_GAME) ? 0x66 : 0x69;
    p->header.flag = leaving;
    p->header.size = 0x000C;
    p->clientID = leaving;
    p->leaderID = l->leaderID;
    return p;
}

void* (*DataCommandLobbyDeletePlayer_Functions[])(LOBBY* li,int leaving) = {DataCommandLobbyDeletePlayer_DCGC,DataCommandLobbyDeletePlayer_BB,DataCommandLobbyDeletePlayer_PC,DataCommandLobbyDeletePlayer_DCGC,NULL,NULL};
int CommandLobbyDeletePlayer(LOBBY* l,int leaving)
{
    if (!l) return 5;
    void* data[VERSION_MAX];
    int x;
    for (x = 0; x < VERSION_MAX; x++)
    {
        if (!DataCommandLobbyDeletePlayer_Functions[x]) data[x] = NULL;
        else data[x] = DataCommandLobbyDeletePlayer_Functions[x](l,leaving);
    }
    int errors = SendCommandToLobby(l,NULL,data);
    for (x = 0; x < VERSION_MAX; x++) if (data[x]) free(data[x]);
    return errors;
}

//int CommandLobbyChangeEvent(LOBBY* l,LOBBY* li)

////////////////////////////////////////////////////////////////////////////////
// CommandLobbyChangeMarker: sends the list of all players' lobby marker (arrow) colors to all players in the lobby

void* DataCommandLobbyChangeMarker_PC(LOBBY* l)
{
    unsigned int x;
    COMMAND_DCPCGC_ARROWUPDATE* p = (COMMAND_DCPCGC_ARROWUPDATE*)malloc(0x0004);
    p->headerpc.command = 0x88;
    p->headerpc.flag = 0x00;
    p->headerpc.size = 0x0004;
    operation_lock(l);
    for (x = 0; x < l->maxClients; x++)
    {
        if (!l->clients[x]) continue;
        p->headerpc.size += 12;
        p = (COMMAND_DCPCGC_ARROWUPDATE*)realloc(p,p->headerpc.size);
        if (!p) return NULL;
        p->entry[p->headerpc.flag].playerTag = 0x00000100;
        p->entry[p->headerpc.flag].serialNumber = l->clients[x]->license.serialNumber;
        p->entry[p->headerpc.flag].arrowColor = l->clients[x]->lobbyarrow;
        p->headerpc.flag++;
    }
    operation_unlock(l);
    return p;
}

void* DataCommandLobbyChangeMarker_DCGC(LOBBY* l)
{
    unsigned int x;
    COMMAND_DCPCGC_ARROWUPDATE* p = (COMMAND_DCPCGC_ARROWUPDATE*)malloc(0x0004);
    p->headerdcgc.command = 0x88;
    p->headerdcgc.flag = 0x00;
    p->headerdcgc.size = 0x0004;
    operation_lock(l);
    for (x = 0; x < l->maxClients; x++)
    {
        if (!l->clients[x]) continue;
        p->headerdcgc.size += 12;
        p = (COMMAND_DCPCGC_ARROWUPDATE*)realloc(p,p->headerdcgc.size);
        if (!p) return NULL;
        p->entry[p->headerdcgc.flag].playerTag = 0x00000100;
        p->entry[p->headerdcgc.flag].serialNumber = l->clients[x]->license.serialNumber;
        p->entry[p->headerdcgc.flag].arrowColor = l->clients[x]->lobbyarrow;
        p->headerdcgc.flag++;
    }
    operation_unlock(l);
    return p;
}

void* DataCommandLobbyChangeMarker_BB(LOBBY* l)
{
    unsigned int x;
    COMMAND_BB_ARROWUPDATE* p = (COMMAND_BB_ARROWUPDATE*)malloc(0x0008);
    p->header.command = 0x88;
    p->header.flag = 0x00;
    p->header.size = 0x0008;
    operation_lock(l);
    for (x = 0; x < l->maxClients; x++)
    {
        if (!l->clients[x]) continue;
        p->header.size += 12;
        p = (COMMAND_BB_ARROWUPDATE*)realloc(p,p->header.size);
        if (!p) return NULL;
        p->entry[p->header.flag].playerTag = 0x00000100;
        p->entry[p->header.flag].serialNumber = l->clients[x]->license.serialNumber;
        p->entry[p->header.flag].arrowColor = l->clients[x]->lobbyarrow;
        p->header.flag++;
    }
    operation_unlock(l);
    return p;
}

void* (*DataCommandLobbyChangeMarker_Functions[])(LOBBY* li) = {DataCommandLobbyChangeMarker_DCGC,DataCommandLobbyChangeMarker_BB,DataCommandLobbyChangeMarker_PC,DataCommandLobbyChangeMarker_DCGC,NULL,NULL};
int CommandLobbyChangeMarker(LOBBY* l)
{
    if (!l) return 5;
    void* data[VERSION_MAX];
    int x;
    for (x = 0; x < VERSION_MAX; x++)
    {
        if (!DataCommandLobbyChangeMarker_Functions[x]) data[x] = NULL;
        else data[x] = DataCommandLobbyChangeMarker_Functions[x](l);
    }
    int errors = SendCommandToLobby(l,NULL,data);
    for (x = 0; x < VERSION_MAX; x++) if (data[x]) free(data[x]);
    return errors;
}

// tells each player that the joining player is done joining, and the game can resume
int CommandResumeGame(LOBBY* l,CLIENT* c)
{
    SUBCOMMAND command;
    command.dword = 0x081C0372;
    return SendSubcommandToLobby(l,c,0x60,0x00,&command,4);
}

////////////////////////////////////////////////////////////////////////////////
// Game/cheat commands 

// sends an HP/TP/Meseta modifying command (see flag definitions in command-functions.h)
int CommandPlayerStatsModify(LOBBY* l,CLIENT* c,BYTE stat,DWORD amount)
{
    long x;
    SUBCOMMAND sub[20];
    if (amount > 2550) return 49354644;
    for (x = 0; amount > 0; x++)
    {
        sub[x * 2].byte[0] = 0x9A;
        sub[x * 2].byte[1] = 0x02;
        sub[x * 2].byte[2] = c->clientID;
        sub[x * 2].byte[3] = 0x00;
        sub[(x * 2) + 1].byte[0] = 0x00;
        sub[(x * 2) + 1].byte[1] = 0x00;
        sub[(x * 2) + 1].byte[2] = stat;
        sub[(x * 2) + 1].byte[3] = (amount > 0xFF) ? 0xFF : amount;
        amount -= (amount > 0xFF) ? 0xFF : amount;
    }
    if (!x) return 0;
    return SendSubcommandToLobby(l,NULL,0x60,0,sub,x * 8);
}

// sends a player to the given area.
int CommandPlayerWarp(CLIENT* c,DWORD area)
{
    SUBCOMMAND cmds[2];
    cmds[0].byte[0] = 0x94;
    cmds[0].byte[1] = 0x02;
    cmds[0].byte[2] = c->clientID;
    cmds[0].byte[3] = 0x00;
    cmds[1].dword = area;
    return SendCommandToClient(c,0x62,c->clientID,cmds,8);
}

////////////////////////////////////////////////////////////////////////////////
// BB game commands 

// notifies other players of a dropped item from a box or enemy
int CommandDropItem(LOBBY* l,ITEM_DATA* item,bool dude,BYTE area,float x,float y,WORD request)
{
    COMMAND_BB_DROP_ITEM p;
    p.header.size = 0x0030;
    p.header.command = 0x0060;
    p.header.flag = 0x00000000;
    p.subcommand = 0x5F;
    p.subsize = 0x0A;
    p.unused = 0x0000;
    p.area = area;
    p.dude = (dude ? 0x01 : 0x02);
    p.requestID = request;
    p.x = x;
    p.y = y;
    p.unused2 = 0;
    memcpy(&p.data,item,sizeof(ITEM_DATA));

    void* data[] = {NULL,&p,NULL,NULL,NULL,NULL};
    return SendCommandToLobby(l,NULL,data);
}

// notifies other players that a stack was split and part of it dropped (a new item was created)
int CommandDropStackedItem(LOBBY* l,CLIENT* c,ITEM_DATA* item,BYTE area,float x,float y)
{
    COMMAND_BB_DROP_STACK_ITEM p;
    p.header.size = 0x002C;
    p.header.command = 0x0060;
    p.header.flag = 0x00000000;
    p.subcommand = 0x5D;
    p.subsize = 0x09;
    p.unused = 0;
    p.area = area;
    p.unused2 = 0;
    p.x = x;
    p.y = y;
    memcpy(&p.data,item,sizeof(ITEM_DATA));

    void* data[] = {NULL,&p,NULL,NULL,NULL,NULL};
    return SendCommandToLobby(l,NULL,data);
}

// notifies other players that an item was picked up
int CommandPickUpItem(LOBBY* l,CLIENT* c,DWORD id,BYTE area)
{
    COMMAND_BB_PICK_UP_ITEM p;
    memset(&p,0,sizeof(COMMAND_BB_PICK_UP_ITEM));
    p.header.size = 0x0014;
    p.header.command = 0x0060;
    p.header.flag = 0x00000000;
    p.subcommand = 0x59;
    p.subsize = 0x03;
    p.clientID = c->clientID;
    p.clientID2 = c->clientID;
    p.area = area;
    p.itemID = id;

    void* data[] = {NULL,&p,NULL,NULL,NULL,NULL};
    return SendCommandToLobby(l,NULL,data);
}

// creates an item in a player's inventory (used for withdrawing items from the bank)
int CommandCreateItem(LOBBY* l,CLIENT* c,ITEM_DATA* item)
{
    COMMAND_BB_CREATE_ITEM p;
    p.header.size = 0x0024;
    p.header.command = 0x0060;
    p.header.flag = 0x00000000;
    p.subcommand = 0xBE;
    p.subsize = 0x07;
    p.clientID = c->clientID;
    memcpy(&p.item,item,sizeof(ITEM_DATA));
    p.unused = 0;
    void* data[] = {NULL,&p,NULL,NULL,NULL,NULL};
    return SendCommandToLobby(l,NULL,data);
}

// destroys an item
int CommandDestroyItem(LOBBY* l,CLIENT* c,DWORD itemID,DWORD amount)
{
    COMMAND_BB_DESTROY_ITEM p;
    p.header.size = 0x0014;
    p.header.command = 0x0060;
    p.header.flag = 0x00000000;
    p.subcommand = 0x29;
    p.subsize = 0x03;
    p.clientID = c->clientID;
    p.itemID = itemID;
    p.amount = amount;
    void* data[] = {NULL,&p,NULL,NULL,NULL,NULL};
    return SendCommandToLobby(l,c,data);
}

// sends the player his/her bank data
int CommandBank(CLIENT* c)
{
    DWORD size = sizeof(COMMAND_BB_BANK) + (c->playerInfo.bank.numItems * sizeof(PLAYER_BANK_ITEM));
    COMMAND_BB_BANK* p = (COMMAND_BB_BANK*)malloc(size);
    if (!p) return 5;
    p->header.size = size;
    p->header.command = 0x006C;
    p->header.flag = 0x00000000;
    p->subcommand = 0xBC;
    p->size = size;
    p->checksum = rand() | (rand() << 16);
    p->numItems = c->playerInfo.bank.numItems;
    p->meseta = c->playerInfo.bank.meseta;
    memcpy(p->items,c->playerInfo.bank.items,sizeof(PLAYER_BANK_ITEM) * c->playerInfo.bank.numItems);
    int rv = SendCommandToClient(c,p);
    free(p);
    return rv;
}

//int CommandShop(SERVER* s,CLIENT* c,DWORD type)

// levels up a player
int CommandLevelUp(LOBBY* l,CLIENT* c)
{
    PLAYER_STATS stats;
    memcpy(&stats,&c->playerInfo.disp.stats,sizeof(PLAYER_STATS));

    long x;
    for (x = 0; x < c->playerInfo.inventory.numItems; x++)
    {
        if ((c->playerInfo.inventory.items[x].equipFlags & 0x08) && (c->playerInfo.inventory.items[x].data.itemData1[0] == 0x02))
        {
            stats.dfp += (c->playerInfo.inventory.items[x].data.itemData1word[2] / 100);
            stats.atp += (c->playerInfo.inventory.items[x].data.itemData1word[3] / 50);
            stats.ata += (c->playerInfo.inventory.items[x].data.itemData1word[4] / 200);
            stats.mst += (c->playerInfo.inventory.items[x].data.itemData1word[5] / 50);
        }
    }

    SUBCOMMAND p[5];
    p[0].byte[0] = 0x30;
    p[0].byte[1] = 0x05;
    p[0].word[1] = c->clientID;
    p[1].word[0] = stats.atp;
    p[1].word[1] = stats.mst;
    p[2].word[0] = stats.evp;
    p[2].word[1] = stats.hp;
    p[3].word[0] = stats.dfp;
    p[3].word[1] = stats.ata;
    p[4].dword = c->playerInfo.disp.level;
    return SendSubcommandToLobby(l,NULL,0x60,0x00,p,0x14);
}

// gives a player EXP
int CommandGiveEXP(LOBBY* l,CLIENT* c,DWORD exp)
{
    SUBCOMMAND p[2];
    p[0].byte[0] = 0xBF;
    p[0].byte[1] = 0x02;
    p[0].word[1] = c->clientID;
    p[1].dword = exp;
    return SendSubcommandToLobby(l,NULL,0x60,0x00,p,8);
}

////////////////////////////////////////////////////////////////////////////////
// ep3 only commands 

// sends the (PRS-compressed) card list to the client
// note that this function does not fail if the client is not Ep3; it just does nothing.
int CommandEp3SendCardUpdate(CLIENT* c)
{
    if (!c) return 5;
    if (!(c->flags & FLAG_V4_GAMES)) return 0;
    int errors = 0;
    DWORD pksize,size,bytesread;
    HANDLE file = CreateFile("system\\ep3\\cardupdate.mnr",GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        CommandMessageBox(c,L"$C6Card update file not found.");
        return 5;
    }
    size = GetFileSize(file,NULL);
    pksize = (13 + size) & 0xFFFFFFFC;
    COMMAND_GC_CARDUPDATE* p = (COMMAND_GC_CARDUPDATE*)malloc(pksize);
    p->header.command = 0xB8;
    p->header.flag = 0x00;
    p->header.size = pksize;
    p->size = size;
    ReadFile(file,p->data,size,&bytesread,NULL);
    CloseHandle(file);
    if (bytesread != size) errors = 2;
    if (!errors) errors = SendCommandToClient(c,p);
    free(p);
    return errors;
}

// sends the client a generic rank
// this function also does not fail if the client is not Ep3
int CommandEp3Rank(CLIENT* c)
{
    if (!c) return 5;
    if (!(c->flags & FLAG_V4_GAMES)) return 0;
    int x;
    COMMAND_GC_RANKUPDATE p;
    p.header.command = 0xB7;
    p.header.flag = 0x00;
    p.header.size = 0x0020;
    p.rank = 0;
    for (x = 0; x < 12; x++) p.rankText[x] = 0;
    p.meseta = 0x00FFFFFF;
    p.maxMeseta = 0x00FFFFFF;
    p.jukeboxSongsUnlocked = 0xFFFFFFFF;
    return SendCommandToClient(c,&p);
}

// sends the map list (used for battle setup) to all players in a game
int CommandEp3SendMapList(LOBBY* l)
{
    if (!(l->flags & LOBBY_FLAG_EP3)) return 39745;
    DWORD size,bytesread;
    HANDLE file = CreateFile("system\\ep3\\maplist.mnr",GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        CommandLobbyMessageBox(NULL,l,NULL,L"A game data file\nwas not found.");
        return 39746;
    }
    size = GetFileSize(file,NULL);

    COMMAND_DCPCGC_SUBCOMMAND* p = (COMMAND_DCPCGC_SUBCOMMAND*)malloc((23 + size) & 0xFFFFFFFC);
    if (!p)
    {
        CloseHandle(file);
        return 1;
    }
    p->headerdcgc.command = 0x6C;
    p->headerdcgc.flag = 0x00;
    p->headerdcgc.size = (23 + size) & 0xFFFFFFFC;
    p->entry[0].dword = 0x000000B6;
    p->entry[1].dword = (23 + size) & 0xFFFFFFFC;
    p->entry[2].dword = 0x00000040;
    p->entry[3].dword = size;
    ReadFile(file,&p->entry[4],size,&bytesread,NULL);
    CloseHandle(file);

    void* data[] = {p,NULL,NULL,NULL,NULL,NULL};
    int rv = SendCommandToLobby(l,NULL,data);
    free(p);
    return rv;
}

// sends the map data for the chosen map to all players in the game
int CommandEp3SendMapData(LOBBY* l,DWORD mapID)
{
    if (!(l->flags & LOBBY_FLAG_EP3)) return 39745;
    //if (!l->ep3) return 39746;

    char filename[MAX_PATH];
    DWORD size,bytesread;
    sprintf(filename,"system\\ep3\\map%08lX.mnm",mapID);
    HANDLE file = CreateFile(filename,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    if (file == INVALID_HANDLE_VALUE)
    {
        CommandLobbyMessageBox(NULL,l,NULL,L"A game data file\nwas not found.");
        return 39846;
    }
    size = GetFileSize(file,NULL);

    COMMAND_DCPCGC_SUBCOMMAND* p = (COMMAND_DCPCGC_SUBCOMMAND*)malloc((23 + size) & 0xFFFFFFFC);
    if (!p)
    {
        CloseHandle(file);
        return 1;
    }
    p->headerdcgc.command = 0x6C;
    p->headerdcgc.flag = 0x00;
    p->headerdcgc.size = (19 + size) & 0xFFFFFFFC;
    p->entry[0].dword = 0x000000B6;
    p->entry[1].dword = (19 + size) & 0xFFFFFFFC;
    p->entry[2].dword = 0x00000041;
    ReadFile(file,&p->entry[3],size,&bytesread,NULL);
    CloseHandle(file);

    void* data[] = {p,NULL,NULL,NULL,NULL,NULL};
    int rv = SendCommandToLobby(l,NULL,data);
    free(p);
    return rv;
}

//int CommandEp3InitChangeState(SERVER* s,CLIENT* c,BYTE state)
//int CommandEp3InitSendMapLayout(SERVER* s,CLIENT* c)
//int CommandEp3InitSendNames(SERVER* s,CLIENT* c)
//int CommandEp3InitSendDecks(SERVER* s,CLIENT* c)
//int CommandEp3InitHandUpdate(SERVER* s,CLIENT* c,int player)
//int CommandEp3InitStatUpdate(SERVER* s,CLIENT* c,int player)
//int CommandEp3Init_B4_06(SERVER* s,CLIENT* c,bool game4p)
//int CommandEp3Init_B4_4E(SERVER* s,CLIENT* c,int player)
//int CommandEp3Init_B4_4C(SERVER* s,CLIENT* c,int player)
//int CommandEp3Init_B4_4D(SERVER* s,CLIENT* c,int player)
//int CommandEp3Init_B4_4F(SERVER* s,CLIENT* c,int player)
//int CommandEp3Init_B4_50(SERVER* s,CLIENT* c)
//int CommandEp3Init_B4_39(SERVER* s,CLIENT* c)
//int CommandEp3InitBegin(SERVER* s,CLIENT* c)

////////////////////////////////////////////////////////////////////////////////
// CommandLoadQuestFile: sends a quest file to the client.
// the _OpenFile functions send the begin command (44/A6), and the _SendChunk functions send a chunk of data (13/A7).

//int CommandLoadQuestFile_OpenFile_DC(CLIENT* c,bool startServer)

int CommandLoadQuestFile_OpenFile_PC(CLIENT* c,char* filename,DWORD size,bool download)
{
    COMMAND_PCGC_OPENQUESTFILE p;
    p.headerpc.size = 0x003C;
    p.headerpc.command = (download ? 0xA6 : 0x44);
    p.headerpc.flag = 0x00;
    strcpy(p.name,"Quest: ");
    strcat(p.name,filename);
    p.unused = 0;
    p.flags = 2;
    strcpy(p.filename,filename);
    p.filesize = size;
    return SendCommandToClient(c,&p);
}

int CommandLoadQuestFile_OpenFile_GC(CLIENT* c,char* filename,DWORD size,bool download)
{
    COMMAND_PCGC_OPENQUESTFILE p;
    memset(&p,0,sizeof(COMMAND_PCGC_OPENQUESTFILE));
    p.headergc.command = (download ? 0xA6 : 0x44);
    p.headergc.flag = 0x00;
    p.headergc.size = 0x003C;
    strcpy(p.name,"Quest: ");
    strcat(p.name,filename);
    p.unused = 0;
    p.flags = 2;
    strcpy(p.filename,filename);
    p.filesize = size;
    return SendCommandToClient(c,&p);
}

int CommandLoadQuestFile_OpenFile_BB(CLIENT* c,char* filename,DWORD size,bool download)
{
    COMMAND_BB_OPENQUESTFILE p;
    memset(&p,0,sizeof(COMMAND_BB_OPENQUESTFILE));
    p.header.size = 0x0058;
    p.header.command = (download ? 0x00A6 : 0x0044);
    p.header.flag = 0x00000000;
    p.flags = 2;
    strcpy(p.filename,filename);
    p.filesize = size;
    strcpy(p.name,"Quest: ");
    strcat(p.name,filename);
    return SendCommandToClient(c,&p);
}

//int CommandLoadQuestFile_SendChunk_DC(CLIENT* c,bool startServer)

int CommandLoadQuestFile_SendChunk_PC(CLIENT* c,char* filename,int chunknum,void* data,DWORD size,bool download)
{
    if (size > 0x400) return 8593654;
    COMMAND_DCPCGC_QUESTFILECHUNK p;
    p.headerpc.size = 0x0418;
    p.headerpc.command = (download ? 0xA7 : 0x13);
    p.headerpc.flag = chunknum;
    strcpy(p.filename,filename);
    memcpy(p.data,data,size);
    p.datasize = size;
    return SendCommandToClient(c,&p);
}

int CommandLoadQuestFile_SendChunk_DCGC(CLIENT* c,char* filename,int chunknum,void* data,DWORD size,bool download)
{
    if (size > 0x400) return 8593654;
    COMMAND_DCPCGC_QUESTFILECHUNK p;
    p.headerdcgc.command = (download ? 0xA7 : 0x13);
    p.headerdcgc.flag = chunknum;
    p.headerdcgc.size = 0x0418;
    strcpy(p.filename,filename);
    memcpy(p.data,data,size);
    p.datasize = size;
    return SendCommandToClient(c,&p);
}

int CommandLoadQuestFile_SendChunk_BB(CLIENT* c,char* filename,int chunknum,void* data,DWORD size,bool download)
{
    if (size > 0x400) return 8593654;
    COMMAND_BB_QUESTFILECHUNK p;
    p.header.size = 0x041C;
    p.header.command = (download ? 0xA7 : 0x13);
    p.header.flag = chunknum;
    strcpy(p.filename,filename);
    memcpy(p.data,data,size);
    p.datasize = size;
    return SendCommandToClient(c,&p);
}

int (*CommandLoadQuestFile_OpenFile_Functions[])(CLIENT* c,char* filename,DWORD size,bool download) = {CommandLoadQuestFile_OpenFile_GC,CommandLoadQuestFile_OpenFile_BB,CommandLoadQuestFile_OpenFile_PC,/*CommandLoadQuestFile_OpenFile_DC*/NULL,NULL,NULL};
int (*CommandLoadQuestFile_SendChunk_Functions[])(CLIENT* c,char* filename,int chunknum,void* data,DWORD size,bool download) = {CommandLoadQuestFile_SendChunk_DCGC,CommandLoadQuestFile_SendChunk_BB,CommandLoadQuestFile_SendChunk_PC,CommandLoadQuestFile_SendChunk_DCGC,NULL,NULL};

int CommandLoadQuestFile(CLIENT* c,char* filename,bool downloadQuest)
{
    if (!CommandLoadQuestFile_OpenFile_Functions[c->version]) return 20;
    if (!CommandLoadQuestFile_SendChunk_Functions[c->version]) return 20;

    int x;
    char basename[MAX_PATH];
    for (x = strlen(filename); x > 0; x--) if (filename[x] == '\\') break;
    if (filename[x] == '\\') x++;
    strcpy(basename,&filename[x]);

    HANDLE file = CreateFile(filename,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    if (file == INVALID_HANDLE_VALUE) return 57489436;

    DWORD size = GetFileSize(file,NULL);
    int errors = CommandLoadQuestFile_OpenFile_Functions[c->version](c,basename,size,downloadQuest);
    if (errors)
    {
        CloseHandle(file);
        return errors;
    }

    BYTE data[0x400];
    DWORD bytesread;
    int chunknum = 0;
    while (size)
    {
        ReadFile(file,data,0x400,&bytesread,NULL);
        errors = CommandLoadQuestFile_SendChunk_Functions[c->version](c,basename,chunknum,data,bytesread,downloadQuest);
        if (errors)
        {
            CloseHandle(file);
            return errors;
        }
        chunknum++;
        size -= bytesread;
    }

    CloseHandle(file);
    return 0;
}

//int CommandInvisiblePlayer(SERVER* s,CLIENT* c)
//int CommandVisiblePlayer(SERVER* s,CLIENT* c)
//int CommandRevive(SERVER* s,CLIENT* c)

// Sends the server's current time to the client
int CommandTime(CLIENT* c)
{
    if (!c) return 5;
    BYTE data[0x40],data2[0x40];
    COMMAND_DCPCGC_TIME* p = (COMMAND_DCPCGC_TIME*)data;
    COMMAND_BB_TIME* p2 = (COMMAND_BB_TIME*)data2;
    SYSTEMTIME time;
    GetLocalTime(&time);
    if (c->version == VERSION_BLUEBURST)
    {
        sprintf(p2->time,"%d:%d:%d: %d:%d:%d.000",time.wYear,time.wMonth,time.wDay,time.wHour,time.wMinute,time.wSecond);
        p2->header.size = 0x0008 + ((strlen(p2->time) + 5) & 0xFFFFFFFC);
        p2->header.command = 0x00B1;
        p2->header.flag = 0x00000000;
        return SendCommandToClient(c,p2);
    } else {
        sprintf(p->time,"%d:%d:%d: %d:%d:%d.000",time.wYear,time.wMonth,time.wDay,time.wHour,time.wMinute,time.wSecond);
        if (c->version == VERSION_PC)
        {
            p->headerpc.size = 0x04 + ((strlen(p->time) + 5) & 0xFFFFFFFC);
            p->headerpc.command = 0xB1;
            p->headerpc.flag = 0x00;
        } else {
            p->headerdcgc.command = 0xB1;
            p->headerdcgc.flag = 0x00;
            p->headerdcgc.size = 0x04 + ((strlen(p->time) + 5) & 0xFFFFFFFC);
        }
        return SendCommandToClient(c,p);
    }
    return 756042590;
}

// sends a simple command (i.e. a command that is only a header)
int CommandSimple(CLIENT* c,WORD command,DWORD sub)
{
    if (!c) return 5;
    COMMAND_HEADER_PC pc;
    COMMAND_HEADER_DCGC gc;
    COMMAND_HEADER_BB bb;
    switch (c->version)
    {
      case VERSION_GAMECUBE:
      case VERSION_DC:
      case VERSION_FUZZIQER:
        gc.command = command;
        gc.flag = sub;
        gc.size = 0x0004;
        return SendCommandToClient(c,&gc);
        break;
      case VERSION_PC:
      case VERSION_PATCH:
        pc.size = 0x0004;
        pc.command = command;
        pc.flag = sub;
        return SendCommandToClient(c,&pc);
        break;
      case VERSION_BLUEBURST:
        bb.size = 0x0004;
        bb.command = command;
        bb.flag = sub;
        return SendCommandToClient(c,&bb);
    }
    return 8493;
}
