// the SERVER structure. every server within this project has these components.
typedef struct {
    OPERATION_LOCK operation;

    char name[0x20];
    DWORD maxClients;
    DWORD numClients;
    CLIENT** clients;

    LISTEN_THREAD threads[0x10];
    LPTHREAD_START_ROUTINE clientThread;

    DWORD numLobbies;
    LOBBY** lobbies;
} SERVER;

SERVER* StartServer(char* serverName,LPTHREAD_START_ROUTINE HandlerRoutine,LISTEN_THREAD_CALLBACK HandleConnection, ...);
bool StopServer(SERVER* s);

DWORD FindClient(SERVER* s,CLIENT* c);
CLIENT* FindClient(SERVER* s,DWORD serialNumber);
CLIENT* FindClient(SERVER* s,wchar_t* name);
CLIENT* FindClientPartialName(SERVER* s,wchar_t* name);
bool AddClient(SERVER* s,CLIENT* c);
bool RemoveClient(SERVER* s,CLIENT* c);
bool AddClientToAvailableLobby(SERVER* s,CLIENT* c);

LOBBY* FindLobby(SERVER* s,DWORD lobbyID);
LOBBY* FindLobbyByBlockNumber(SERVER* s,DWORD block);
LOBBY* FindLobbyByName(SERVER* s,wchar_t* name,bool casesensitive);
bool AddLobby(SERVER* s,LOBBY* l);
bool RemoveLobby(SERVER* s,LOBBY* l);

/* void LoadLobbyInfo(SERVER* s);
void SaveLobbyInfo(SERVER* s); */

