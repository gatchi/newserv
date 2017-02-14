#include <windows.h>

#include "text.h"
#include "operation.h"
#include "console.h"
#include "network-addresses.h"

#include "encryption.h"
#include "netconfig.h"
#include "updates.h"

#include "player.h"
#include "quest.h"
#include "battleparamentry.h"
#include "itemraretable.h"
#include "levels.h"
#include "map.h"

#include "license.h"
#include "client.h"

#include "listenthread.h"
#include "lobby.h"
#include "items.h"
#include "server.h"

#include "dns-server.h"
#include "commonservers.h"
#include "patchserver.h"
#include "blockserver.h"
#include "loginserver.h"
#include "lobbyserver.h"

#include "newserv.h"

CFGFile* config;
UPDATE_LIST* updatelist;
QUESTLIST* quests;
LICENSE_LIST* licenses;
LEVEL_TABLE* leveltable;
BATTLE_PARAM battleParamTable[2][3][4][0x60]; // online/offline, episode, diff, monster 
in_addr local_addresses[24];
in_addr external_connect;
in_addr local_connect;

int main(int argc,char* argv[])
{
    ConsolePrintColor("$0F> Fuzziqer Software NewServ beta 5\n\n");

    // load config file (die if it's not found)
    ConsolePrintColor("$0E> Loading configuration file: ");
    config = CFGLoadFile("system\\config.ini");
    if (!config)
    {
        ConsolePrintColor("$0Cfailed!\n");
        ConsolePrintColor("$0C> > Configuration file must be present for server to work.\n");
        return (-1);
    } else ConsolePrintColor("$0Aok\n");

    // load update list
    ConsolePrintColor("$0E> Loading update list: ");
    updatelist = LoadUpdateList("system\\update.ini");
    if (!updatelist) ConsolePrintColor("$0Cfailed!\n");
    else ConsolePrintColor("$0A%d files\n",updatelist->numFiles);

    // load license list (die if it's not found)
    ConsolePrintColor("$0E> Loading license list: ");
    licenses = LoadLicenseList("system\\licenses.nsi");
    if (!licenses)
    {
        ConsolePrintColor("$0Cfailed!\n");
        ConsolePrintColor("$0C> > License list must be present for server to work.\n");
        return (-1);
    } else ConsolePrintColor("$0A%d licenses\n",licenses->numLicenses);

    // load monster information (battleparamentry files)
    DWORD numFailures = 0;
    ConsolePrintColor("$0E> Loading battle parameters: ");
    if (!LoadBattleParamEntriesEpisode(&battleParamTable[0][0][0][0],"system\\blueburst\\BattleParamEntry_on.dat")) numFailures++;
    if (!LoadBattleParamEntriesEpisode(&battleParamTable[0][1][0][0],"system\\blueburst\\BattleParamEntry_lab_on.dat")) numFailures++;
    if (!LoadBattleParamEntriesEpisode(&battleParamTable[0][2][0][0],"system\\blueburst\\BattleParamEntry_ep4_on.dat")) numFailures++;
    if (!LoadBattleParamEntriesEpisode(&battleParamTable[1][0][0][0],"system\\blueburst\\BattleParamEntry.dat")) numFailures++;
    if (!LoadBattleParamEntriesEpisode(&battleParamTable[1][1][0][0],"system\\blueburst\\BattleParamEntry_lab.dat")) numFailures++;
    if (!LoadBattleParamEntriesEpisode(&battleParamTable[1][2][0][0],"system\\blueburst\\BattleParamEntry_ep4.dat")) numFailures++;
    if (numFailures) ConsolePrintColor("$0Cfailed!\n");
    else ConsolePrintColor("$0Aok\n");

    // load non-rare item info
    ConsolePrintColor("$0E> Loading non-rare item information: ");
    if (SetupNonRareSystem()) ConsolePrintColor("$0Cfailed!\n");
    else ConsolePrintColor("$0Aok\n");

    // load level info
    ConsolePrintColor("$0E> Loading player level information: ");
    leveltable = LoadLevelTable("system\\blueburst\\PlyLevelTbl.prs",true);
    if (!leveltable) ConsolePrintColor("$0Cfailed!\n");
    else ConsolePrintColor("$0Aok\n");

    // build quest lists
    ConsolePrintColor("$0E> Loading quest list: ");
    if (CreateQuestList("system\\quests\\index.ini","system\\quests\\",&quests)) ConsolePrintColor("$0Cfailed!\n");
    else ConsolePrintColor("$0A%d quests\n",quests->numQuests);

    // start Winsock
    ConsolePrintColor("$0E> Starting network subsystem: ");
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2,2),&wsadata))
    {
        ConsolePrintColor("$0Cfailed!\n");
        return (-1);
    } else ConsolePrintColor("$0Aok\n");

    ConsolePrintColor("\n");

    // find our network addresses and set the default local and external connect addresses
    long x = WSGetLocalAddressList(local_addresses);
    local_connect.s_addr = WSResolveAddress(CFGGetStringSafeA(config,"Local_Address"));
    external_connect.s_addr = WSResolveAddress(CFGGetStringSafeA(config,"External_Address"));
    if (!WSAddLocalAddress(local_addresses,&local_connect)) ConsolePrintColor("$0C> Error: could not add local address to address list\n");
    if (!WSAddLocalAddress(local_addresses,&external_connect)) ConsolePrintColor("$0C> Error: could not add external address to address list\n");

    ConsolePrintColor("$0E> IP addresses found:\n>");
    for (x = 0; local_addresses[x].s_addr; x++)
    {
        if (local_addresses[x].s_addr == external_connect.s_addr) ConsolePrintColor("$0F %s",inet_ntoa(local_addresses[x]));
        else if (local_addresses[x].s_addr == local_connect.s_addr) ConsolePrintColor("$0B %s",inet_ntoa(local_addresses[x]));
        else ConsolePrintColor("$0E %s",inet_ntoa(local_addresses[x]));
    }
    ConsolePrintColor("\n\n");

    // start all our servers!
    SERVER *blockserver = NULL;
    SERVER *loginserver = NULL;
    SERVER *lobbyserver = NULL;
    SERVER *patchserver = NULL;

    ConsolePrintColor("$0E> Starting DNS server: ");
    if (DNSStartServer()) ConsolePrintColor("$0Aok\n");
    else ConsolePrintColor("$0Cfailed!\n");

    ConsolePrintColor("$0E> Starting patch server: ");
    patchserver = StartServer("Patch Server",(LPTHREAD_START_ROUTINE)HandlePatchClient,(LISTEN_THREAD_CALLBACK)HandleConnection,CFGGetNumber(config,"Patch_Server_Port_PC"),CFGGetNumber(config,"Patch_Server_Port_BB"),0);
    if (patchserver) ConsolePrintColor("$0Aok\n");
    else ConsolePrintColor("$0Cfailed!\n");

    ConsolePrintColor("$0E> Starting block server: ");
    blockserver = StartServer("Block Server",(LPTHREAD_START_ROUTINE)HandleBlockClient,(LISTEN_THREAD_CALLBACK)HandleConnection,CFGGetNumber(config,"Block_Server_Port_BB"),CFGGetNumber(config,"Block_Server_Port_1_BB"),CFGGetNumber(config,"Block_Server_Port_2_BB"),0);
    if (blockserver) ConsolePrintColor("$0Aok\n");
    else ConsolePrintColor("$0Cfailed!\n");

    ConsolePrintColor("$0E> Starting login server: ");
    loginserver = StartServer("Login Server",(LPTHREAD_START_ROUTINE)HandleLoginClient,(LISTEN_THREAD_CALLBACK)HandleConnection,CFGGetNumber(config,"Japanese_1.0_Port_GC"),CFGGetNumber(config,"Japanese_1.1_Port_GC"),CFGGetNumber(config,"Japanese_Ep3_Port_GC"),CFGGetNumber(config,"American_1.0/1.1_Port_GC"),CFGGetNumber(config,"American_Ep3_Port_GC"),CFGGetNumber(config,"European_1.0_Port_GC"),CFGGetNumber(config,"European_1.1_Port_GC"),CFGGetNumber(config,"European_Ep3_Port_GC"),CFGGetNumber(config,"Login_Server_Port_PC"),CFGGetNumber(config,"Login_Server_Port_BB"),0);
    if (loginserver)
    {
        if (SetupLoginServer(loginserver))
        {
            StopServer(loginserver);
            ConsolePrintColor("$0Cfailed!\n");
        } else ConsolePrintColor("$0Aok\n");
    } else ConsolePrintColor("$0Cfailed!\n");

    ConsolePrintColor("$0E> Starting lobby server: ");
    lobbyserver = StartServer("Lobby Server",(LPTHREAD_START_ROUTINE)HandleLobbyClient,(LISTEN_THREAD_CALLBACK)HandleConnection,CFGGetNumber(config,"Lobby_Server_Port_PC"),CFGGetNumber(config,"Lobby_Server_Port_GC"),CFGGetNumber(config,"Lobby_Server_Port_BB"),0);
    if (lobbyserver)
    {
        if (SetupLobbyServer(lobbyserver))
        {
            StopServer(lobbyserver);
            ConsolePrintColor("$0Cfailed!\n");
        } else ConsolePrintColor("$0Aok\n");
    } else ConsolePrintColor("$0Cfailed!\n");

    // and now, we wait. there should be some kind of termination condition here
    // (like user presses CTRL+SHIFT+ESCAPE or something). I haven't put this
    // in because I always just close it by clicking the close box on the console.
    ConsolePrintColor("\n");
    WaitForSingleObject(GetCurrentThread(),INFINITE);

    // stop servers and free everything
    ConsolePrintColor("$0E> Stopping lobby server\n");
    StopServer(lobbyserver);
    ConsolePrintColor("$0E> Stopping login server\n");
    StopServer(loginserver);
    ConsolePrintColor("$0E> Stopping block server\n");
    StopServer(blockserver);
    ConsolePrintColor("$0E> Stopping patch server\n");
    StopServer(patchserver);
    ConsolePrintColor("$0E> Stopping DNS server\n");
    DNSStopServer();
    ConsolePrintColor("$0E> Releasing data from memory\n");
    DestroyLicenseList(licenses);
    DestroyQuestList(quests);
    DestroyUpdateList(updatelist);
    CFGCloseFile(config);
    ConsolePrintColor("$0E> Stopping network system\n");
    WSACleanup();
    return 0;
}

