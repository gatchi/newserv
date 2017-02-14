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

#include "commonservers.h"
#include "loginserver.h"

#include "command-functions.h"

extern CFGFile* config;

char* loginServerPortNames[] = {"European_1.0_Port_GC","Login_Server_Port_BB","Login_Server_Port_PC",NULL};

////////////////////////////////////////////////////////////////////////////////

// item IDs for main menu choices
#define LOGINSERVER_MENU_GO_TO_LOBBY    0x89476122
#define LOGINSERVER_MENU_SERVER_LIST    0x48778673
#define LOGINSERVER_MENU_INFORMATION    0x46461538
#define LOGINSERVER_MENU_DISCONNECT     0x65893320

// the main menu itself
SHIP_SELECT_MENU loginMainMenu = {
    L"Main Menu",NULL,MENU_FLAG_AUTOSELECT_ONE_CHOICE | MENU_FLAG_USE_ITEM_IDS,
    {L"Go To Lobby",L"Server List",L"Information",L"Disconnect",NULL},
    {L"Join the lobby\non this server",L"Look for other\nservers",L"View information",L"Disconnect",NULL},
    {0,0,MENU_ITEM_FLAG_NOBLUEBURST | MENU_ITEM_FLAG_REQ_MESSAGEBOX,0,0},
    {LOGINSERVER_MENU_GO_TO_LOBBY,LOGINSERVER_MENU_SERVER_LIST,LOGINSERVER_MENU_INFORMATION,LOGINSERVER_MENU_DISCONNECT,0}
};

////////////////////////////////////////////////////////////////////////////////

char informationMenuFileNames[0x20][0x20];
wchar_t informationMenuTitles[0x20][0x20];
wchar_t informationMenuDescriptions[0x20][0x80];
SHIP_SELECT_MENU informationMenu;

// called for every information menu item present. each item is added to the information menu.
bool SetupLoginServer_EnumInformationFiles(CFGFile* cf,const char* name,const char* valueA,const wchar_t* valueW,DWORD* numItems)
{
    if (*numItems > 29) return false;

    long x,len;
    strcpy(informationMenuFileNames[*numItems],name);

    len = wcslen(valueW);
    for (x = 0; x < len; x++) if (valueW[x] == L'~') break;
    if (x > 0x1F) return true;
    if ((len - x - 1) > 0x7F) return true;
    informationMenu.items[*numItems] = informationMenuTitles[*numItems];
    informationMenu.descriptions[*numItems] = informationMenuDescriptions[*numItems];
    wcsncpy(informationMenu.items[*numItems],valueW,x);
    wcscpy(informationMenu.descriptions[*numItems],&valueW[x + 1]);
    tx_replace_char(informationMenu.descriptions[*numItems],L'@',L'#');
    (*numItems)++;
    return true;
}

// builds menus and prepares the information menu
int SetupLoginServer(SERVER* s)
{
    loginMainMenu.name = CFGGetStringW(config,"Short_Name");
    tx_add_color(loginMainMenu.name);

    memset(&informationMenu,0,sizeof(SHIP_SELECT_MENU));
    informationMenu.name = CFGGetStringW(config,"Short_Name");
    tx_add_color(informationMenu.name);

    DWORD numItems = 0;
    memset(informationMenuFileNames,0,0x400);
    memset(informationMenuDescriptions,0,0x1000);
    CFGFile* infoList = CFGLoadFile("system\\info\\index.ini");
    if (infoList)
    {
        CFGEnumValues(infoList,(CFGEnumRoutine)SetupLoginServer_EnumInformationFiles,(long)(&numItems));
        CFGCloseFile(infoList);
        informationMenu.items[numItems] = informationMenuTitles[numItems];
        informationMenu.descriptions[numItems] = informationMenuDescriptions[numItems];
        wcscpy(informationMenu.items[numItems],L"<- Go Back");
        wcscpy(informationMenu.descriptions[numItems],L"$C7Return to the\nMain Menu.");
    } else loginMainMenu.itemFlags[2] |= MENU_ITEM_FLAG_INVISIBLE;

    return 0;
}

// this function is called when a client connects to the login server.
DWORD HandleLoginClient(NEW_CLIENT_THREAD_DATA* nctd)
{
    SERVER* s = nctd->s;
    CLIENT* c = nctd->c;
    nctd->release = true;
    nctd = NULL;
    srand(GetTickCount());

    char filename[MAX_PATH];
    char* lobbyServerPortNames[VERSION_MAX] = {"Lobby_Server_Port_GC","Lobby_Server_Port_BB","Lobby_Server_Port_PC","Lobby_Server_Port_DC",NULL,NULL};

    // add the client to the server
    ConsolePrintColor("$0E> Login server: new client\n");
    AddClient(s,c);

    // if the client connected on 9100, then it could be a PC client; make it reconnect
    int errors = 0;
    if (c->port == CFGGetNumber(config,"American_1.0/1.1_Port_GC")) CommandSelectiveReconnectPC(c,c->localip,CFGGetNumber(config,"Login_Server_Port_PC"));

    // send an init command and get username/password
    CommandServerInit(c,true);
    ProcessCommands(s,c,0x99,0x0093,0);

    // send the Ep3 card update and rank command
    // these functions do nothing and return success if the client is not Ep3
    CommandEp3SendCardUpdate(c);
    CommandEp3Rank(c);

    // give the client the main menu....
    wchar_t* buffer;
    DWORD x,bytesread,selection = 0xFFFFFFFF;
    while ((selection == 0xFFFFFFFF) && !errors)
    {
        errors = CommandShipSelect(s,c,&loginMainMenu,&selection);
        switch (selection)
        {
          case LOGINSERVER_MENU_GO_TO_LOBBY:
            // chose Go To Lobby? send them to the lobby server
            CommandReconnect(c,c->localip,CFGGetNumber(config,lobbyServerPortNames[c->version]));
            break;
          case LOGINSERVER_MENU_SERVER_LIST:
            // chose Server List? sorry, unimplemented!
            CommandShipInfo(NULL,NULL,c,L"$C6Unsupported!");
            selection = 0xFFFFFFFF;
            break;
          case LOGINSERVER_MENU_INFORMATION:
            // chose Information? give them the Info menu
            selection = 0xFFFFFFFF;
            while ((selection == 0xFFFFFFFF) && !errors)
            {
                errors = CommandShipSelect(s,c,&informationMenu,&selection);
                if (informationMenuFileNames[selection][0])
                {
                    // if they picked a valid file, then we'll load the file and
                    // present it to them with a large message box, then wait for them
                    // to close the message box (D6).
                    sprintf(filename,"system\\info\\%s",informationMenuFileNames[selection]);
                    HANDLE file = CreateFile(filename,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
                    if (file == INVALID_HANDLE_VALUE) CommandMessageBox(c,L"$C6An information file was not found.");
                    else {
                        buffer = (wchar_t*)malloc(0x804);
                        if (!buffer) CommandMessageBox(c,L"$C6The server is out of memory.");
                        else {
                            ReadFile(file,buffer,0x800,&bytesread,NULL);
                            buffer[bytesread / 2] = 0;
                            if (buffer[0] == 0xFEFF) wcscpy(buffer,&buffer[1]);
                            for (x = 0; buffer[x]; x++) if (buffer[x] == 0x000D) wcscpy(&buffer[x],&buffer[x + 1]);
                            CommandMessageBox(c,buffer);
                            free(buffer);
                        }
                        CloseHandle(file);
                    }
                    ProcessCommands(s,c,0xD6,0);
                    selection = 0xFFFFFFFF;
                }
            }
            selection = 0xFFFFFFFF;
            break;
          case LOGINSERVER_MENU_DISCONNECT:
            // client chose to disconnect? goodbye
            c->disconnect = true;
            break;
        }
    }
    ProcessCommands(s,c,0);

    // remove and delete the client
    RemoveClient(s,c);
    DeleteClient(c);
    ConsolePrintColor("$0E> Login server: disconnecting client\n");

    return 0;
}

