// data passed to new client threads when clients connect
typedef struct {
    SERVER* s;
    CLIENT* c;
    bool release;
} NEW_CLIENT_THREAD_DATA;

bool HandleConnection(DWORD local,DWORD remote,WORD port,int newsocket,SERVER* server);
