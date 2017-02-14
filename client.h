typedef struct {
    DWORD magic; // must be set to 0x48615467 
    BYTE bbGameState; // status of client connecting on BB 
    BYTE bbplayernum; // selected char 
    WORD flags; // just in case we lose them somehow between connections 
    WORD ports[4]; // used by shipgate clients 
    DWORD unused[4];
} CLIENTCONFIG;

typedef struct {
    DWORD magic; // must be set to 0x48615467 
    BYTE bbGameState; // status of client connecting on BB 
    BYTE bbplayernum; // selected char 
    WORD flags; // just in case we lose them somehow between connections 
    WORD ports[4]; // used by shipgate clients 
    DWORD unused[4];
    DWORD unusedBBOnly[2];
} CLIENTCONFIG_BB;

typedef struct {
    OPERATION_LOCK operation;

    // license & account 
    LICENSE license;
    wchar_t name[0x20];
    CLIENTCONFIG_BB cfg;
    WORD version; // use VERSION_ constants above 
    WORD flags; // use FLAG_ constants above 

    // encryption 
    bool encrypt;
    CRYPT_SETUP cryptin;
    CRYPT_SETUP cryptout;

    // network 
    unsigned long localip;
    unsigned long remoteip;
    int port;
    int socket;
    unsigned long nextConnectionAddress;
    int nextConnectionPort;
    bool disconnect;
    DWORD threadID;
    HANDLE thread;

    // timing & menus 
    DWORD playTimeBegin; // time of connection (used for incrementing play time on BB) 
    DWORD lastrecv; // time of last data received 
    DWORD lastsend; // time of last data sent 
    DWORD lastMenuSelection;
    int lastMenuSelectionType; // 0: selection, 1: info 
    wchar_t lastMenuSelectionPassword[0x14];
    DWORD questLoading;

    // lobby/positioning 
    DWORD area; // which area is the client in? 
    DWORD lobbyID; // which lobby is this person in? 
    BYTE clientID; // which client number is thie person? 
    BYTE lobbyarrow; // lobby arrow color ID 
    PLAYER playerInfo; // player data, duh 

    // miscellaneous (used by chat commands) 
    DWORD nextexp; // next EXP value to give 
    bool infhp,inftp; // cheats enabled 
    bool stfu; // if true, player can't chat 
    DWORD chatlock; // serial number of ventriloquist's dummy 
} CLIENT;

int SendClient(CLIENT*,void*,int); // alias of winsock send() 
int ReceiveClient(CLIENT*,void*,int); // alias of winsock recv() 

int DeleteClient(CLIENT*);

//wchar_t* VersionName(BYTE);

//void SetClientVersionFlags(CLIENT*,BYTE);

//int ErrorCheckClient(CLIENT*);
//wchar_t* ErrorCheckClientName(int);
