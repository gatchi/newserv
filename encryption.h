// encryption types supported
#define CRYPT_GAMECUBE   0 // PSOGC encryption
#define CRYPT_BLUEBURST  1 // PSOBB encryption
#define CRYPT_PC         2 // PSOPC encryption
#define CRYPT_SCHTHACK   3 // Schthack shipgate protocol encryption (removed; Crono probably wouldn't want me releasing this)
#define CRYPT_FUZZIQER   4 // FS shipgate protocol encryption (removed; it was never used)

// generic encryption instance that can be used for all types of encryption supported
typedef struct {
    BYTE type; // type of instance
    DWORD keys[1042]; // keystream
    DWORD pc_posn; // position in key stream (used by CRYPT_PC)
    DWORD* gc_block_ptr; // position in key stream (used by CRYPT_GAMECUBE)
    DWORD* gc_block_end_ptr; // position of end of key stream (used by CRYPT_GAMECUBE)
    DWORD gc_seed; // seed used to generate key stream (used by all except CRYPT_BLEUBURST)
    DWORD bb_posn; // position in key stream (used by CRYPT_BLUEBURST)
    DWORD bb_seed[12]; // seed used to generate key stream (used by CRYPT_BLUEBURST)
} CRYPT_SETUP;

// PSOPC crypt functions
void CRYPT_PC_CreateKeys(CRYPT_SETUP*,DWORD);
void CRYPT_PC_CryptData(CRYPT_SETUP*,void*,DWORD);
void CRYPT_PC_DEBUG_PrintKeys(CRYPT_SETUP*,wchar_t*);

// PSOGC crypt functions
void CRYPT_GC_CreateKeys(CRYPT_SETUP*,DWORD);
void CRYPT_GC_CryptData(CRYPT_SETUP*,void*,DWORD);
void CRYPT_GC_DEBUG_PrintKeys(CRYPT_SETUP*,wchar_t*);

// PSOBB crypt functions
void CRYPT_BB_Decrypt(CRYPT_SETUP*,void*,DWORD);
void CRYPT_BB_Encrypt(CRYPT_SETUP*,void*,DWORD);
void CRYPT_BB_CreateKeys(CRYPT_SETUP*,void*);
void CRYPT_BB_DEBUG_PrintKeys(CRYPT_SETUP*,wchar_t*);

// Schthack shipgate crypt functions
void CRYPT_SS_Decrypt(CRYPT_SETUP*,void*,DWORD);
void CRYPT_SS_Encrypt(CRYPT_SETUP*,void*,DWORD);
void CRYPT_SS_CreateKeys(CRYPT_SETUP*,void*);
void CRYPT_SS_DEBUG_PrintKeys(CRYPT_SETUP*,wchar_t*);

// FS shipgate crypt functions
void CRYPT_FS_CreateKeys(CRYPT_SETUP*,DWORD);
void CRYPT_FS_CryptData(CRYPT_SETUP*,void*,DWORD);
void CRYPT_FS_DEBUG_PrintKeys(CRYPT_SETUP*,wchar_t*);

// generic functions (all the above are internal to the encryption library; call these functions rather than the version-specific ones)
void CRYPT_CreateKeys(CRYPT_SETUP*,void*,unsigned char);
void CRYPT_CryptData(CRYPT_SETUP*,void*,DWORD,bool);
void CRYPT_DEBUG_PrintKeys(CRYPT_SETUP*,wchar_t*);

// data print functions. should be in console.h, but I'm too lazy to move them
void CRYPT_PrintData(void*,DWORD);
void CRYPT_PrintData(HANDLE,void*,DWORD);
