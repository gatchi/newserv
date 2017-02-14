#include <windows.h>
#include <winsock2.h>

#include "operation.h"
#include "listenthread.h"

typedef bool (*LISTEN_THREAD_CALLBACK)(DWORD local,DWORD remote,WORD port,int newsocket,long param);

// thread routine that listens on a socket, and calls the callback function when a client connects.
// to terminate this thread, simply close the listening socket and set it to SOCKET_ERROR.
// accept() will fail immediately when the socket is closed.
// NOTE: this behavior may be different in Unix sockets! This program has only been
// tested with Winsock, where this little trick works fine.
DWORD ListenThreadRoutine(LISTEN_THREAD* lt)
{
    int asock,conn_size = sizeof(sockaddr_in),local_size;
    sockaddr_in connection,local;

    while (lt->socket != SOCKET_ERROR)
    {
        asock = accept(lt->socket,(sockaddr*)(&connection),&conn_size);
        if (asock != SOCKET_ERROR)
        {
            local_size = sizeof(sockaddr_in);
            getsockname(asock,(SOCKADDR*)(&local),&local_size);
            if (!lt->callback(local.sin_addr.s_addr,connection.sin_addr.s_addr,lt->port,asock,lt->param)) closesocket(asock);
        }
    }
    return 0;
}

// starts a thread listening on the specified port (fill in addr, port, callback,
// and param members of the structure after initializing it to 0 with memset/ZeroMemory).
// addr can be INADDR_ANY to listen on all interfaces.
bool BeginListenThread(LISTEN_THREAD* lt)
{
    int errors;
    operation_lock(lt);

    lt->socket = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if (lt->socket == (signed)INVALID_SOCKET) return false;

    BYTE yes = true;
    errors = setsockopt(lt->socket,SOL_SOCKET,SO_REUSEADDR,(char*)(&yes),1);
    if (errors != NO_ERROR)
    {
        closesocket(lt->socket);
        return false;
    }

    sockaddr_in service;
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = lt->addr;
    service.sin_port = htons(lt->port);
    errors = bind(lt->socket,(SOCKADDR*)(&service),sizeof(service));
    if (errors == SOCKET_ERROR)
    {
        closesocket(lt->socket);
        return false;
    }

    errors = listen(lt->socket,10);
    if (errors == SOCKET_ERROR)
    {
        closesocket(lt->socket);
        return false;
    }

    lt->thread = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)ListenThreadRoutine,lt,0,&lt->threadID);
    if (!lt->thread)
    {
        closesocket(lt->socket);
        return false;
    }

    operation_unlock(lt);
    return true;
}

// ends a listening thread. see the ListenThreadRoutine comment to see why this works.
bool EndListenThread(LISTEN_THREAD* lt,DWORD time)
{
    operation_lock(lt);
    closesocket(lt->socket);
    lt->socket = SOCKET_ERROR;
    if (WaitForSingleObject(lt->thread,time) == WAIT_TIMEOUT) TerminateThread(lt->thread,0);
    CloseHandle(lt->thread);
    lt->thread = NULL;
    lt->threadID = 0;
    operation_unlock(lt);
    return true;
}

