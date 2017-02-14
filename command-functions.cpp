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
#include "command-input-structures.h"

// Translates a command from BB to DC/GC format
bool TranslateCommandBBDCGC(COMMAND_HEADER_BB* a,COMMAND_HEADER_DCGC* b)
{
    memcpy((void*)((DWORD)b + sizeof(COMMAND_HEADER_DCGC)),
           (void*)((DWORD)a + sizeof(COMMAND_HEADER_BB)),
           a->size - sizeof(COMMAND_HEADER_BB));
    b->size = a->size - 4;
    b->command = a->command;
    b->flag = a->flag;
    return true;
}

// Translates a command from BB to PC format
bool TranslateCommandBBPC(COMMAND_HEADER_BB* a,COMMAND_HEADER_PC* b)
{
    memcpy((void*)((DWORD)b + sizeof(COMMAND_HEADER_PC)),
           (void*)((DWORD)a + sizeof(COMMAND_HEADER_BB)),
           a->size - sizeof(COMMAND_HEADER_BB));
    b->size = a->size - 4;
    b->command = a->command;
    b->flag = a->flag;
    return true;
}

// Translates a command from DC/GC to BB format
bool TranslateCommandDCGCBB(COMMAND_HEADER_DCGC* a,COMMAND_HEADER_BB* b)
{
    memcpy((void*)((DWORD)b + sizeof(COMMAND_HEADER_BB)),
           (void*)((DWORD)a + sizeof(COMMAND_HEADER_DCGC)),
           a->size - sizeof(COMMAND_HEADER_DCGC));
    b->size = a->size + 4;
    b->command = a->command;
    b->flag = a->flag;
    return true;
}

// Translates a command from DC/GC to PC format
bool TranslateCommandDCGCPC(COMMAND_HEADER_DCGC* a,COMMAND_HEADER_PC* b)
{
    memcpy((void*)((DWORD)b + sizeof(COMMAND_HEADER_PC)),
           (void*)((DWORD)a + sizeof(COMMAND_HEADER_DCGC)),
           a->size - sizeof(COMMAND_HEADER_DCGC));
    b->size = a->size;
    b->command = a->command;
    b->flag = a->flag;
    return true;
}

// Translates a command from PC to BB format
bool TranslateCommandPCBB(COMMAND_HEADER_PC* a,COMMAND_HEADER_BB* b)
{
    memcpy((void*)((DWORD)b + sizeof(COMMAND_HEADER_BB)),
           (void*)((DWORD)a + sizeof(COMMAND_HEADER_PC)),
           a->size - sizeof(COMMAND_HEADER_PC));
    b->size = a->size + 4;
    b->command = a->command;
    b->flag = a->flag;
    return true;
}

// Translates a command from PC to DC/GC format
bool TranslateCommandPCDCGC(COMMAND_HEADER_PC* a,COMMAND_HEADER_DCGC* b)
{
    memcpy((void*)((DWORD)b + sizeof(COMMAND_HEADER_DCGC)),
           (void*)((DWORD)a + sizeof(COMMAND_HEADER_PC)),
           a->size - sizeof(COMMAND_HEADER_PC));
    b->size = a->size;
    b->command = a->command;
    b->flag = a->flag;
    return true;
}

