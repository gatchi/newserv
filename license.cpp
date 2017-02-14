#include <windows.h>
#include <windows.h>
#include <stdio.h>

#include "operation.h"
#include "license.h"

LICENSE_LIST* LoadLicenseList(char* filename)
{
    LICENSE_LIST* li = NULL;
    unsigned long numLicenses;
    DWORD bytesread;
    HANDLE file = CreateFile(filename,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
    if (file == INVALID_HANDLE_VALUE) return NULL;

    ReadFile(file,&numLicenses,4,&bytesread,NULL);
    if (bytesread != 4)
    {
        CloseHandle(file);
        return NULL;
    }
    li = (LICENSE_LIST*)malloc(sizeof(LICENSE_LIST));
    if (!li)
    {
        CloseHandle(file);
        return NULL;
    }
    memset(li,0,sizeof(LICENSE_LIST));
    li->licenses = (LICENSE*)malloc(numLicenses * sizeof(LICENSE));
    if (!li->licenses)
    {
        free(li);
        CloseHandle(file);
        return NULL;
    }
    ReadFile(file,li->licenses,numLicenses * sizeof(LICENSE),&bytesread,NULL);
    CloseHandle(file);
    if (bytesread != (numLicenses * sizeof(LICENSE)))
    {
        free(li->licenses);
        free(li);
        return NULL;
    }

    strcpy(li->filename,filename);
    li->numLicenses = numLicenses;
    return li;
}

bool SaveLicenseList(LICENSE_LIST* li)
{
    if (!li) return false;

    DWORD bytesread;
    HANDLE file = CreateFile(li->filename,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);
    if (file == INVALID_HANDLE_VALUE) return false;
    operation_lock(li);
    WriteFile(file,&li->numLicenses,4,&bytesread,NULL);
    if (bytesread != 4)
    {
        operation_unlock(li);
        CloseHandle(file);
        return false;
    }
    WriteFile(file,li->licenses,li->numLicenses * sizeof(LICENSE),&bytesread,NULL);
    operation_unlock(li);
    CloseHandle(file);
    if (bytesread != (li->numLicenses * sizeof(LICENSE))) return false;
    return true;
}

void DestroyLicenseList(LICENSE_LIST* li)
{
    operation_lock(li);
    if (li)
    {
        if (li->licenses) free(li->licenses);
        free(li);
    }
}

int VerifyLicense(LICENSE_LIST* li,LICENSE* l,long verifyWhat)
{
    FILETIME now;
    unsigned long x;
    if (!li || !l) return LICENSE_RESULT_INVALID_CALL;
    operation_lock(li);

    if (verifyWhat & LICENSE_VERIFY_BLUEBURST)
    {
        for (x = 0; x < li->numLicenses; x++) if (!strcmp(li->licenses[x].username,l->username)) break;
        if (x >= li->numLicenses)
        {
            operation_unlock(li);
            return LICENSE_RESULT_NOTFOUND;
        }
        if ((verifyWhat & LICENSE_VERIFY_CHECK_PASSWORD) && strcmp(li->licenses[x].password,l->password))
        {
            operation_unlock(li);
            return LICENSE_RESULT_WRONGPASS;
        }
    }
    if (verifyWhat & LICENSE_VERIFY_GAMECUBE)
    {
        for (x = 0; x < li->numLicenses; x++) if ((li->licenses[x].serialNumber == l->serialNumber) && !memcmp(li->licenses[x].accessKey,l->accessKey,12)) break;
        if (x >= li->numLicenses)
        {
            operation_unlock(li);
            return LICENSE_RESULT_NOTFOUND;
        }
        if ((verifyWhat & LICENSE_VERIFY_CHECK_PASSWORD) && strcmp(li->licenses[x].password2,l->password2))
        {
            operation_unlock(li);
            return LICENSE_RESULT_WRONGPASS;
        }
    }
    if (verifyWhat & LICENSE_VERIFY_PC)
    {
        for (x = 0; x < li->numLicenses; x++) if ((li->licenses[x].serialNumber == l->serialNumber) && !memcmp(li->licenses[x].accessKey,l->accessKey,8)) break;
        if (x >= li->numLicenses)
        {
            operation_unlock(li);
            return LICENSE_RESULT_NOTFOUND;
        }
        if ((verifyWhat & LICENSE_VERIFY_CHECK_PASSWORD) && strcmp(li->licenses[x].password2,l->password2))
        {
            operation_unlock(li);
            return LICENSE_RESULT_WRONGPASS;
        }
    }
    if (verifyWhat & LICENSE_VERIFY_SERIALNUMBER)
    {
        for (x = 0; x < li->numLicenses; x++) if (li->licenses[x].serialNumber == l->serialNumber) break;
        if (x >= li->numLicenses)
        {
            operation_unlock(li);
            return LICENSE_RESULT_NOTFOUND;
        }
        if ((verifyWhat & LICENSE_VERIFY_CHECK_PASSWORD) && strcmp(li->licenses[x].password2,l->password2))
        {
            operation_unlock(li);
            return LICENSE_RESULT_WRONGPASS;
        }
    }

    if (li->licenses[x].banTime.dwLowDateTime || li->licenses[x].banTime.dwHighDateTime)
    {
        GetSystemTimeAsFileTime(&now);
        if (CompareFileTime(&li->licenses[x].banTime,&now) <= 0)
        {
            memset(&li->licenses[x].banTime,0,sizeof(FILETIME));
            operation_unlock(li);
            SaveLicenseList(li);
        } else {
            operation_unlock(li);
            return LICENSE_RESULT_BANNED;
        }
    } else {
        memcpy(l,&li->licenses[x],sizeof(LICENSE));
        operation_unlock(li);
    }
    return LICENSE_RESULT_OK;
}

int SetUserBan(LICENSE_LIST* li,LICENSE* l,long verifyWhat,DWORD seconds)
{
    unsigned long x;
    if (!li || !l) return LICENSE_RESULT_INVALID_CALL;
    operation_lock(li);
    if (verifyWhat & LICENSE_VERIFY_BLUEBURST)
    {
        for (x = 0; x < li->numLicenses; x++) if (!strcmp(li->licenses[x].username,l->username)) break;
        if (x >= li->numLicenses)
        {
            operation_unlock(li);
            return LICENSE_RESULT_NOTFOUND;
        }
        if ((verifyWhat & LICENSE_VERIFY_CHECK_PASSWORD) && strcmp(li->licenses[x].password,l->password))
        {
            operation_unlock(li);
            return LICENSE_RESULT_WRONGPASS;
        }
    }
    if (verifyWhat & LICENSE_VERIFY_GAMECUBE)
    {
        for (x = 0; x < li->numLicenses; x++) if ((li->licenses[x].serialNumber == l->serialNumber) && !memcmp(li->licenses[x].accessKey,l->accessKey,12)) break;
        if (x >= li->numLicenses)
        {
            operation_unlock(li);
            return LICENSE_RESULT_NOTFOUND;
        }
        if ((verifyWhat & LICENSE_VERIFY_CHECK_PASSWORD) && strcmp(li->licenses[x].password2,l->password2))
        {
            operation_unlock(li);
            return LICENSE_RESULT_WRONGPASS;
        }
    }
    if (verifyWhat & LICENSE_VERIFY_PC)
    {
        for (x = 0; x < li->numLicenses; x++) if ((li->licenses[x].serialNumber == l->serialNumber) && !memcmp(li->licenses[x].accessKey,l->accessKey,8)) break;
        if (x >= li->numLicenses)
        {
            operation_unlock(li);
            return LICENSE_RESULT_NOTFOUND;
        }
        if ((verifyWhat & LICENSE_VERIFY_CHECK_PASSWORD) && strcmp(li->licenses[x].password2,l->password2))
        {
            operation_unlock(li);
            return LICENSE_RESULT_WRONGPASS;
        }
    }
    if (verifyWhat & LICENSE_VERIFY_SERIALNUMBER)
    {
        for (x = 0; x < li->numLicenses; x++) if (li->licenses[x].serialNumber == l->serialNumber) break;
        if (x >= li->numLicenses)
        {
            operation_unlock(li);
            return LICENSE_RESULT_NOTFOUND;
        }
        if ((verifyWhat & LICENSE_VERIFY_CHECK_PASSWORD) && strcmp(li->licenses[x].password2,l->password2))
        {
            operation_unlock(li);
            return LICENSE_RESULT_WRONGPASS;
        }
    }

    LARGE_INTEGER now;
    GetSystemTimeAsFileTime((FILETIME*)&now);
    now.QuadPart += UInt32x32To64(seconds,10000000);

    memcpy(&li->licenses[x].banTime,&now,sizeof(FILETIME));
    memcpy(&l->banTime,&now,sizeof(FILETIME));
    operation_unlock(li);
    if (SaveLicenseList(li)) return LICENSE_RESULT_BANNED;
    return LICENSE_RESULT_OK;
}

unsigned int FindLicense(LICENSE_LIST* li,unsigned long serialNumber)
{
    unsigned long x;
    if (!li) return 0xFFFFFFFF;
    operation_lock(li);
    for (x = 0; x < li->numLicenses; x++) if (li->licenses[x].serialNumber == serialNumber) break;
    operation_unlock(li);
    return ((x < li->numLicenses) ? x : 0xFFFFFFFF);
}

bool AddLicense(LICENSE_LIST* li,LICENSE* l)
{
    if (!li || !l) return false;

    operation_lock(li);
    LICENSE* t = (LICENSE*)malloc(sizeof(LICENSE) * (li->numLicenses + 1));
    if (!t)
    {
        operation_unlock(li);
        return false;
    }
    memcpy(t,li->licenses,li->numLicenses * sizeof(LICENSE));
    memcpy(&t[li->numLicenses],l,sizeof(LICENSE));
    free(li->licenses);
    li->licenses = t;
    operation_unlock(li);
    return true;
}

bool DeleteLicense(LICENSE_LIST* li,unsigned long index)
{
    if (!li) return false;
    if ((index < 0) || (index >= li->numLicenses)) return false;

    operation_lock(li);
    LICENSE* t = (LICENSE*)malloc(sizeof(LICENSE) * (li->numLicenses - 1));
    if (!t)
    {
        operation_unlock(li);
        return false;
    }
    memcpy(t,li->licenses,index * sizeof(LICENSE));
    memcpy(&t[index],&li->licenses[index + 1],sizeof(LICENSE) * (li->numLicenses - index - 1));
    free(li->licenses);
    li->licenses = t;
    operation_unlock(li);
    return true;
}

int EnumLicenses(LICENSE_LIST* list,DWORD filter,LICENSE* criteria,LicenseEnumProc proc,long param)
{
    DWORD x,numCalls = 0;
    long result;
    operation_lock(list);
    for (x = 0; x < list->numLicenses; x++)
    {
        if ((filter & LICENSE_CHECK_USERNAME) && (strcmp(criteria->username,list->licenses[x].username))) continue;
        if ((filter & LICENSE_CHECK_PASSWORD) && (strcmp(criteria->password,list->licenses[x].password))) continue;
        if ((filter & LICENSE_CHECK_SERIALNUMBER) && (criteria->serialNumber != list->licenses[x].serialNumber)) continue;
        if ((filter & LICENSE_CHECK_ACCESSKEY) && (strcmp(criteria->accessKey,list->licenses[x].accessKey))) continue;
        if ((filter & LICENSE_CHECK_GC_PASSWORD) && (strcmp(criteria->password2,list->licenses[x].password2))) continue;
        if ((filter & LICENSE_CHECK_PRIVILEGES) && (criteria->privileges != list->licenses[x].privileges)) continue;
        numCalls++;
        result = proc(list,x,&list->licenses[x],param);
        if (result < 0) x--;
        if (result == 0) break;
    }
    operation_unlock(list);
    return numCalls;
}

