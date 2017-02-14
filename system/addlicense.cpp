#include <windows.h>
#include <stdio.h>

#include "../license.h"

// add a license
// addlicense.exe file:<filename> add bbuser:<username> bbpass:<password> serial:<serialNumber> access:<accesskey> pass:<gcpassword> priv:<privileges>

// delete a license
// addlicense.exe file:<filename> delete find:<options>

// modify a license
// addlicense.exe file:<filename> modify find:<options> <newoptions>

// unban a license
// addlicense.exe file:<filename> unban find:<options>

// display a license
// addlicense.exe file:<filename> info find:<options>

// parses a command line option.
DWORD parseoption(LICENSE* l,char* option)
{
    if (!memcmp(option,"bbuser:",7))
    {
        strcpy(l->username,&option[7]);
        return LICENSE_CHECK_USERNAME;
    } else if (!memcmp(option,"bbpass:",7))
    {
        strcpy(l->password,&option[7]);
        return LICENSE_CHECK_PASSWORD;
    } else if (!memcmp(option,"serial:",7))
    {
        if ((option[7] == '0') && ((option[8] == 'x') || (option[8] == 'X'))) sscanf(&option[9],"%X",&l->serialNumber);
        else sscanf(&option[7],"%d",&l->serialNumber);
        return LICENSE_CHECK_SERIALNUMBER;
    } else if (!memcmp(option,"access:",7))
    {
        strcpy(l->accessKey,&option[7]);
        return LICENSE_CHECK_ACCESSKEY;
    } else if (!memcmp(option,"pass:",5))
    {
        strcpy(l->password2,&option[5]);
        return LICENSE_CHECK_GC_PASSWORD;
    } else if (!memcmp(option,"priv:",5))
    {
        if ((option[5] == '0') && ((option[6] == 'x') || (option[6] == 'X'))) sscanf(&option[7],"%X",&l->privileges);
        else sscanf(&option[5],"%d",&l->privileges);
        return LICENSE_CHECK_PRIVILEGES;
    }
    return 0;
}

DWORD scanFlags = 0,replaceFlags = 0;

int deletelicenseenumproc(LICENSE_LIST* list,int index,LICENSE* l,long param)
{
    bool success;
    if (DeleteLicense(list,index))
    {
        printf("> > license deleted: %08X\n",l->serialNumber);
        return (-1);
    }
    printf("> > license could not be deleted: %08X\n",l->serialNumber);
    return 1;
}

int modifylicenseenumproc(LICENSE_LIST* list,int index,LICENSE* l,LICENSE* lnew)
{
    bool success;
    printf("> > license modified: %08X\n",l->serialNumber);
    if (replaceFlags & LICENSE_CHECK_USERNAME) strcpy(l->username,lnew->username);
    if (replaceFlags & LICENSE_CHECK_PASSWORD) strcpy(l->password,lnew->password);
    if (replaceFlags & LICENSE_CHECK_SERIALNUMBER) l->serialNumber = lnew->serialNumber;
    if (replaceFlags & LICENSE_CHECK_ACCESSKEY) strcpy(l->accessKey,lnew->accessKey);
    if (replaceFlags & LICENSE_CHECK_GC_PASSWORD) strcpy(l->password2,lnew->password2);
    if (replaceFlags & LICENSE_CHECK_PRIVILEGES) l->privileges = lnew->privileges;
    return 1;
}

int unbanlicenseenumproc(LICENSE_LIST* list,int index,LICENSE* l,long param)
{
    bool success;
    memset(&l->banTime,0,sizeof(FILETIME));
    printf("> > license unbanned: %08X\n",l->serialNumber);
    return 1;
}

int infolicenseenumproc(LICENSE_LIST* list,int index,LICENSE* l,long param)
{
    bool success;
    printf("> blue burst user/pass: %s %s\n",l->username,l->password);
    printf("> gc serial/access/password: %08X %s %s\n",l->serialNumber,l->accessKey,l->password2);
    printf("> privilege flags/ban time: %08X %08X%08X\n\n",l->privileges,l->banTime.dwHighDateTime,l->banTime.dwLowDateTime);
    return 1;
}

int main(int argc,char* argv[])
{
    printf("> fuzziqer software newserv license editor\n\n");

    char filename[MAX_PATH];
    DWORD x,flagsTemp;
    LICENSE_LIST* list;
    LICENSE lold,lnew;
    memset(&lold,0,sizeof(LICENSE));
    memset(&lnew,0,sizeof(LICENSE));

    bool result;
    DWORD action = 0; // 1 = add, 2 = delete, 3 = modify, 4 = unban, 5 = get info
    DWORD numFailures = 0,numChanges;
    for (x = 1; x < argc; x++)
    {
        result = true;
             if (!memcmp(argv[x],"add",3)) action = 1;
        else if (!memcmp(argv[x],"delete",6)) action = 2;
        else if (!memcmp(argv[x],"modify",6)) action = 3;
        else if (!memcmp(argv[x],"unban",5)) action = 4;
        else if (!memcmp(argv[x],"info",4)) action = 5;
        else if (!memcmp(argv[x],"file:",5)) strcpy(filename,&argv[x][5]);
        else if (!memcmp(argv[x],"find:",5))
        {
            flagsTemp = parseoption(&lold,&argv[x][5]);
            if (!flagsTemp)
            {
                printf("> > error: unknown find option: %s\n",&argv[x][5]);
                numFailures++;
            } else scanFlags |= flagsTemp;
        } else {
            flagsTemp = parseoption(&lnew,argv[x]);
            if (!flagsTemp)
            {
                printf("> > error: unknown option: %s\n",argv[x]);
                numFailures++;
            } else replaceFlags |= flagsTemp;
        }
    }

    list = LoadLicenseList(filename);
    if (!list)
    {
        printf("> > warning: license file not found, creating a new one\n");
        list = CreateLicenseList();
        if (!list)
        {
            printf("> > use the [file:<filename>] option to specify the file name\n");
            numFailures++;
        } else strcpy(list->filename,filename);
    }
    if (numFailures) return (-1);

    switch (action)
    {
      case 1: // add license
        if (AddLicense(list,&lnew)) printf("> > license added\n");
        else printf("> > error!\n");
        break;
      case 2: // delete license
        if (!scanFlags) printf("> > error: no scan flags specified\n> > use at least one [find:] directive\n");
        else {
            numChanges = EnumLicenses(list,scanFlags,&lold,(LicenseEnumProc)deletelicenseenumproc,0);
            printf("> > %d licenses deleted\n",numChanges);
        }
        break;
      case 3: // modify license
        if (!scanFlags) printf("> > error: no scan flags specified\n> > use at least one [find:] directive\n");
        else {
            numChanges = EnumLicenses(list,scanFlags,&lold,(LicenseEnumProc)modifylicenseenumproc,(long)(&lnew));
            printf("> > %d licenses modified\n",numChanges);
        }
        break;
      case 4: // unban license
        if (!scanFlags) printf("> > error: no scan flags specified\n> > use at least one [find:] directive\n");
        else {
            numChanges = EnumLicenses(list,scanFlags,&lold,(LicenseEnumProc)unbanlicenseenumproc,0);
            printf("> > %d licenses unbanned\n",numChanges);
        }
        break;
      case 5: // show license
        if (!scanFlags) printf("> > error: no scan flags specified\n> > use at least one [find:] directive\n");
        else {
            numChanges = EnumLicenses(list,scanFlags,&lold,(LicenseEnumProc)infolicenseenumproc,0);
            printf("> > %d licenses listed\n",numChanges);
        }
        break;
    }
    if (!SaveLicenseList(list)) printf("> > error: couldn't save the license list\n");

    system("PAUSE>NUL");

    return 0;
}

