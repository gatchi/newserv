// credentials to verify for VerifyLicense and similar functions
#define LICENSE_VERIFY_BLUEBURST        0x00000001 // verify BB username
#define LICENSE_VERIFY_GAMECUBE         0x00000002 // verify GC serial number/access key
#define LICENSE_VERIFY_PC               0x00000004 // verify PC serial number/access key
#define LICENSE_VERIFY_SERIALNUMBER     0x00000008 // verify serial number only
#define LICENSE_VERIFY_CHECK_PASSWORD   0x00000010 // verify password also (this flag must be combined with one of the above)

// credentials to search for using EnumLicenses (can be combined)
#define LICENSE_CHECK_USERNAME          0x00000001 // search by username
#define LICENSE_CHECK_PASSWORD          0x00000002 // search by password
#define LICENSE_CHECK_SERIALNUMBER      0x00000004 // search by serial number
#define LICENSE_CHECK_ACCESSKEY         0x00000008 // search by access key
#define LICENSE_CHECK_GC_PASSWORD       0x00000010 // search by GC password
#define LICENSE_CHECK_PRIVILEGES        0x00000020 // search by privileges

// errors
#define LICENSE_RESULT_INVALID_CALL   (-1) // invalid arguments
#define LICENSE_RESULT_OK             0 // no error
#define LICENSE_RESULT_NOTFOUND       1 // license not found
#define LICENSE_RESULT_WRONGPASS      2 // wrong password
#define LICENSE_RESULT_BANNED         3 // license is banned

// a license.
typedef struct {
    char username[20]; // BB username (max. 16 chars; should technically be Unicode)
    char password[20]; // BB password (max. 16 chars)
    unsigned long serialNumber; // PC/GC serial number. MUST BE PRESENT FOR BB LICENSES TOO; this is also the player's guild card number.
    char accessKey[16]; // PC/GC access key. (to log in using PC on a GC license, just enter the first 8 characters of the GC access key)
    char password2[12]; // GC password
    long privileges; // privilege level
    FILETIME banTime; // end time of ban (zero = not banned)
} LICENSE;

// a loaded list of licenses
typedef struct {
    OPERATION_LOCK operation;
    char filename[260];
    unsigned long numLicenses;
    LICENSE* licenses;
} LICENSE_LIST;

typedef int (*LicenseEnumProc)(LICENSE_LIST* list,DWORD index,LICENSE* l,long param); // returns <0: run that one again, =0: stop enumerating, >0: continue

LICENSE_LIST* LoadLicenseList(char* filename);
bool SaveLicenseList(LICENSE_LIST* li);
void DestroyLicenseList(LICENSE_LIST* li);

int VerifyLicense(LICENSE_LIST* li,LICENSE* l,long verifyWhat);
int SetUserBan(LICENSE_LIST* li,LICENSE* l,long verifyWhat,DWORD seconds);

unsigned int FindLicense(LICENSE_LIST* li,unsigned long serialNumber);
bool AddLicense(LICENSE_LIST* li,LICENSE* l);
bool DeleteLicense(LICENSE_LIST* li,long index);

int EnumLicenses(LICENSE_LIST* list,DWORD filter,LICENSE* criteria,LicenseEnumProc proc,long param);

