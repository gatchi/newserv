#include <windows.h>

#include "text.h"
#include "operation.h"
#include "console.h"

#include "encryption.h"
#include "license.h"
#include "player.h"
#include "client.h"

// sends data to the client. really, it's just a wrapper for send()
int SendClient(CLIENT* c,void* data,int size)
{
    int posn = 0;
    int lastsend;
    while (posn < size)
    {
        lastsend = send(c->socket,(char*)((DWORD)data + posn),size - posn,0);
        if ((lastsend == SOCKET_ERROR) || (lastsend == 0)) return (-1);
        posn += lastsend;
    }
    return 0;
}

// receives data from the client. really, it's just a wrapper for recv()
int ReceiveClient(CLIENT* c,void* data,int size)
{
    int posn = 0;
    int lastrecv;
    while (posn < size)
    {
        lastrecv = recv(c->socket,(char*)((DWORD)data + posn),size - posn,0);
        if (lastrecv == 0) return (-1);
        if (lastrecv == SOCKET_ERROR)
        {
            if (WSAGetLastError() != WSAEWOULDBLOCK) return (-1);
            else Sleep(0);
        } else posn += lastrecv;
    }
    return 0;
}

// frees a client and associated memory
int DeleteClient(CLIENT* c)
{
    operation_lock(c);
    //if (c->playerInfo.account) free(c->playerInfo.account);
    closesocket(c->socket);
    CloseHandle(c->thread);
    free(c);
    return 0;
}
