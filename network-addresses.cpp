#include <windows.h>
#include <winsock2.h>
#include <stdio.h>

#include "network-addresses.h"

// does DNS on a domain name.
DWORD WSResolveAddress(char* address)
{
    if ((address[0] >= '0') && (address[0] <= '9')) return inet_addr(address);
    hostent* localHost = gethostbyname(address);
    if (localHost == NULL) return 0;
    return (*(struct in_addr*)*localHost->h_addr_list).s_addr;
}

// gets a list of local addresses. Winsock apparently behaves slightly differently
// than Unix here; this is why the server doesn't work well in Wine.
// note that local_addr must be an array of in_addr at least 24 long.
DWORD WSGetLocalAddressList(in_addr* local_addr)
{
    hostent* localHost;
    DWORD x;

    DWORD nic = 0;
    ZeroMemory(local_addr,sizeof(in_addr) * 24);
    localHost = gethostbyname("");
    for (x = 0; x < 23; x++)
    {
        if (localHost->h_addr_list[x] == NULL) break;
        local_addr[x] = *(in_addr*)(localHost->h_addr_list[x]);
        if (WSIsLocalAddressType(local_addr[x].s_addr) && local_addr[x].s_net != 127) nic++;
    }
    memmove(&local_addr[1],&local_addr[0],4 * 23);
    local_addr[0].s_addr = inet_addr("127.0.0.1");
    return nic;
}

// adds an address to the local address list.
bool WSAddLocalAddress(in_addr* local_addr,in_addr* add)
{
    long x;
    for (x = 0; (local_addr[x].s_addr) && (local_addr[x].s_addr != add->s_addr); x++);
    if (x > 22) return false;
    if (local_addr[x].s_addr) return true;
    local_addr[x].s_addr = add->s_addr;
    return true;
}

// finds the local address that a socket is connected to
DWORD WSGetConnectedAddress(int s)
{
    sockaddr_in si,sir;
    int namelen = sizeof(sockaddr_in);
    if (getsockname(s,(SOCKADDR*)&si,&namelen) == SOCKET_ERROR) return 0;
    if (getpeername(s,(SOCKADDR*)&sir,&namelen) == SOCKET_ERROR) return 0;
    if (!WSIsLocalAddressType(sir.sin_addr.s_addr)) return 0;
    return si.sin_addr.s_addr;
}

// determines the "closest" (numerically) address to the given address.
// this allows hosts on the local network to "see" a local server, and hosts
// on the Internet to "see" a remote server.
DWORD WSDetermineBestConnectAddress(in_addr* local_addr,DWORD daddr)
{
    in_addr test;
    long x,best = 0;
    long matches[24] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    test.s_addr = daddr;
    for (x = 0; local_addr[x].s_addr; x++)
    {
        if (local_addr[x].s_net == test.s_net) matches[x]++;
        else continue;
        if (local_addr[x].s_host == test.s_host) matches[x]++;
        else continue;
        if (local_addr[x].s_lh == test.s_lh) matches[x]++;
        else continue;
        if (local_addr[x].s_impno == test.s_impno) matches[x]++;
        else continue;
    }
    for (x = 1; x < 24; x++) if (matches[x] > matches[best]) best = x;
    return local_addr[best].s_addr;
}

// crude method of telling whether an address is local. this is actually a fairly terrible way to do it.
bool WSIsLocalAddressType(DWORD daddr)
{
    in_addr addr;
    addr.s_addr = daddr;
    if ((addr.s_net != 127) && (addr.s_net != 1) && (addr.s_net != 172) && (addr.s_net != 10) && (addr.s_net != 192)) return false;
    return true;
}

