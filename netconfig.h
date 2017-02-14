typedef struct {
    char name[0x40];
    char valueA[0x100];
    wchar_t valueW[0x100]; } CFGString;

typedef struct {
    unsigned long numncs;
    CFGString* ncs; } CFGFile;

typedef bool (*CFGEnumRoutine)(CFGFile* cf,const char* name,const char* valueA,const wchar_t* valueW,long param);

CFGString* CFGAddEntry(CFGFile* cf);
CFGFile* CFGLoadFile(char* filename);
void CFGCloseFile(CFGFile* cf);

long CFGParseNumber(const char* str);
bool CFGIsValuePresent(CFGFile* cf,char* name);
long CFGGetNumber(CFGFile* cf,char* name);
char* CFGGetStringA(CFGFile* cf,char* name);
char* CFGGetStringSafeA(CFGFile* cf,char* name);
wchar_t* CFGGetStringW(CFGFile* cf,char* name);
wchar_t* CFGGetStringSafeW(CFGFile* cf,char* name);

bool CFGEnumValues(CFGFile* cf,CFGEnumRoutine func,long param);

