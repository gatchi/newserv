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
#include "privilege.h"
#include "battleparamentry.h"
#include "itemraretable.h"
#include "player.h"
#include "map.h"
#include "client.h"

#include "listenthread.h"
#include "lobby.h"
#include "server.h"

#include "command-functions.h"

// changes the player's lobby safely (i.e. without risk of stranding the player outside a lobby)
int ProcedureChangeLobby(SERVER* s,LOBBY* current,LOBBY* next,CLIENT* c)
{
    int errors = 0;
    int prevClientID = c->clientID;
    if (next && current) // changing lobbies 
    {
        //ConsolePrintColor("$0E> > ProcedureChangeLobby: changing lobbies for client %S\n",c->name);
        if (ChangeLobby(current,next,c))
        {
            errors = CommandLobbyDeletePlayer(current,prevClientID);
            if (!errors) errors = CommandLobbyJoin(next,c,true);
        } else return CommandLobbyMessageBox(NULL,NULL,c,L"$C6Can\'t change lobby\n\n$C7The lobby might be full.");
    } else if (!next && current) // leaving a lobby 
    {
        //ConsolePrintColor("$0E> > ProcedureChangeLobby: leaving lobbies for client %S\n",c->name);
        if (!RemoveClient(current,c)) return 80046;
        errors = CommandLobbyDeletePlayer(current,prevClientID);
    } else if (next && !current) // joining a lobby 
    {
        //ConsolePrintColor("$0E> > ProcedureChangeLobby: joining lobbies for client %S\n",c->name);
        if (!AddClient(next,c)) return 80047;
        errors = CommandLobbyJoin(next,c,true);
    } else { // joining an available lobby 
        //ConsolePrintColor("$0E> > ProcedureChangeLobby: joining open lobbies for client %S\n",c->name);
        if (!AddClientToAvailableLobby(s,c)) return 80048;
        next = FindLobby(s,c->lobbyID);
        if (!next) return 80049;
        CommandLobbyJoin(next,c,true);
    }
    c->area = 0;
    if (!current) return errors;

    if (!(current->flags & LOBBY_FLAG_DEFAULT))
    {
        if (RemoveLobby(s,current))
        {
            //ConsolePrintColor("$0E> ProcedureChangeLobby: lobby deleted\n");
            DeleteLobby(current);
        }
    }
    return errors;
}

// checks to see if a player can join a game
int ProcedureCheckToJoinGame(LOBBY* l,CLIENT* c)
{
    if (CountLobbyPlayers(l) > l->maxClients) return JOINGAME_ERROR_FULL;
    if (l->version != c->version) return JOINGAME_ERROR_VERSION;
    if (!(l->flags & LOBBY_FLAG_EP3) != !(c->flags & LOBBY_FLAG_EP3)) return JOINGAME_ERROR_VERSION;
    if (l->flags & LOBBY_FLAG_QUESTING) return JOINGAME_ERROR_QUEST;
    if (CheckLoadingPlayers(l)) return JOINGAME_ERROR_LOADING;
    if (l->mode == 3) return JOINGAME_ERROR_SOLO;
    if (!(c->license.privileges & PRIVILEGE_FREE_JOIN_GAMES))
    {
        if (l->password[0] && wcsncmp(l->password,c->lastMenuSelectionPassword,0x10)) return JOINGAME_ERROR_PASSWORD;
        if (c->playerInfo.disp.level < l->minlevel) return JOINGAME_ERROR_LEVEL_TOO_LOW;
        if (c->playerInfo.disp.level > l->maxlevel) return JOINGAME_ERROR_LEVEL_TOO_HIGH;
    }
    return 0;
}

// converts an error code from the previous function into a string
wchar_t* ProcedureCheckToJoinGame_ErrorString(int error)
{
    switch (error)
    {
        case JOINGAME_ERROR_FULL: return L"$C6This game is full.";
        case JOINGAME_ERROR_VERSION: return L"$C6This game is for\na different version\nof PSO.";
        case JOINGAME_ERROR_QUEST: return L"$C6A quest is in progress\nin this game.";
        case JOINGAME_ERROR_LOADING: return L"$C6A player is currently\nloading.";
        case JOINGAME_ERROR_SOLO: return L"$C6You cannot join a\nSolo Mode game.";
        case JOINGAME_ERROR_PASSWORD: return L"$C6Incorrect password.";
        case JOINGAME_ERROR_LEVEL_TOO_LOW: return L"$C6Your level is too\nlow to join this\ngame.";
        case JOINGAME_ERROR_LEVEL_TOO_HIGH: return L"$C6Your level is too\nhigh to join this\ngame.";
        default: return L"$C6Unknown error.";
    }
    return L"$C6Unknown error.";
}

// Loads a quest.
// The "quest loading progress bar jumps backward" bug is due to this function.
// Normally, PSO quests are loaded simultaneously, that is, both the .bin and .dat
// files are download at the same time. Due to the way this server is written, it
// would be tedious to do that, so I load them sequentially instead. PSO isn't "aware"
// of the second file while the first is loading, so it can't factor that into
// the progress bar.
int ProcedureLoadQuest(CLIENT* c,QUEST* q,bool download)
{
    char filename[MAX_PATH];
    QuestFileName(filename,"system\\quests\\",q->filebase,c->version,true);
    int errors = CommandLoadQuestFile(c,filename,download);
    if (errors) return errors;
    QuestFileName(filename,"system\\quests\\",q->filebase,c->version,false);
    return CommandLoadQuestFile(c,filename,download);
}

