// if the callback returns FALSE, the listen thread will close the new socket as if it were rejected 
typedef bool (*LISTEN_THREAD_CALLBACK)(DWORD local,DWORD remote,WORD port,int newsocket,long param);

typedef struct {
    OPERATION_LOCK operation;
    DWORD addr;
    WORD port;
    int socket;
    DWORD threadID;
    HANDLE thread;
    LISTEN_THREAD_CALLBACK callback;
    long param;
} LISTEN_THREAD;

bool BeginListenThread(LISTEN_THREAD*);
bool EndListenThread(LISTEN_THREAD*,DWORD);
