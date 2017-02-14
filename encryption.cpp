#include <windows.h>
#include <stdio.h>
#include "encryption.h"
#include "console.h"

// I realize that these functions could be better implemented with function pointers.
// At the time I wrote this library, I did not know how to use function pointers, and this works just fine.

// Initializes the stream cipher context using the given key and encryption type.
void CRYPT_CreateKeys(CRYPT_SETUP* cs,void* key,BYTE type)
{
    ZeroMemory(cs,sizeof(CRYPT_SETUP));
    cs->type = type;
    switch (cs->type)
    {
      case CRYPT_PC:
        CRYPT_PC_CreateKeys(cs,*(DWORD*)key);
        break;
      case CRYPT_GAMECUBE:
        CRYPT_GC_CreateKeys(cs,*(DWORD*)key);
        break;
      case CRYPT_BLUEBURST:
        CRYPT_BB_CreateKeys(cs,key);
        break;
      //case CRYPT_SCHTHACK:
        //CRYPT_SS_CreateKeys(cs,key);
        //break;
      //case CRYPT_FUZZIQER:
        //CRYPT_FS_CreateKeys(cs,*(DWORD*)key);
        //break;
    }
}

// Encrypts or decrypts data using the givent stream cipher context.
void CRYPT_CryptData(CRYPT_SETUP* cs,void* data,DWORD size,bool encrypting)
{
    switch (cs->type)
    {
      case CRYPT_PC:
        CRYPT_PC_CryptData(cs,data,size);
        break;
      case CRYPT_GAMECUBE:
        CRYPT_GC_CryptData(cs,data,size);
        break;
      case CRYPT_BLUEBURST:
        if (encrypting) CRYPT_BB_Encrypt(cs,data,size);
        else CRYPT_BB_Decrypt(cs,data,size);
        break;
      //case CRYPT_SCHTHACK:
        //if (encrypting) CRYPT_SS_Encrypt(cs,data,size);
        //else CRYPT_SS_Decrypt(cs,data,size);
        //break;
      //case CRYPT_FUZZIQER:
        //CRYPT_FS_CryptData(cs,data,size);
        //break;
    }
}

// Prints the key stream. This should never be necessary (hence the 'debug')
void CRYPT_DEBUG_PrintKeys(CRYPT_SETUP* cs,wchar_t* title)
{
    switch (cs->type)
    {
      case CRYPT_PC:
        CRYPT_PC_DEBUG_PrintKeys(cs,title);
        break;
      case CRYPT_GAMECUBE:
        CRYPT_GC_DEBUG_PrintKeys(cs,title);
        break;
      case CRYPT_BLUEBURST:
        CRYPT_BB_DEBUG_PrintKeys(cs,title);
        break;
      //case CRYPT_SCHTHACK:
        //CRYPT_SS_DEBUG_PrintKeys(cs,title);
        //break;
      //case CRYPT_FUZZIQER:
        //CRYPT_FS_DEBUG_PrintKeys(cs,title);
        //break;
    }
}

// Prints a block of data to the console in a nice hex/ascii view.
// This really should be in console.cpp but I never got around to moving/renaming it.
void CRYPT_PrintData(void* ds,DWORD data_size)
{
    BYTE* data_source = (BYTE*)ds;
    DWORD x,y,off;
    char buffer[17];
    buffer[16] = 0;
    off = 0;
    printf("00000000 | ");
    for (x = 0; x < data_size; x++)
    {
        if (off == 16)
        {
            for (y = 0; y < 16; y++)
            {
                if (data_source[x + y - 16] < 0x20) buffer[y] = ' ';
                else buffer[y] = data_source[x + y - 16];
            }
            printf("| %s\n%08lX | ",buffer,x);
            off = 0;
        }
        printf("%02X ",data_source[x]);
        off++;
    }
    buffer[off] = 0;
    for (y = 0; y < off; y++) buffer[y] = data_source[x - off + y];
    for (y = 0; y < off; y++) if (buffer[y] < 0x20) buffer[y] = ' ';
    for (y = 0; y < 16 - off; y++) printf("   ");
    printf("| %s\n\n",buffer);
}

// Prints a block of data to a file in a nice hex/ascii view.
void CRYPT_PrintData(FILE* f,void* ds,DWORD data_size)
{
    BYTE* data_source = (BYTE*)ds;
    DWORD x,y,off;
    wchar_t buffer[17];
    buffer[16] = 0;
    off = 0;
    fwprintf(f,L"00000000 | ");
    for (x = 0; x < data_size; x++)
    {
        if (off == 16)
        {
            for (y = 0; y < 16; y++)
            {
                if (data_source[x + y - 16] < 0x20) buffer[y] = L' ';
                else buffer[y] = data_source[x + y - 16];
            }
            fwprintf(f,L"| %s\n%08X | ",buffer,x);
            off = 0;
        }
        fwprintf(f,L"%02X ",data_source[x]);
        off++;
    }
    buffer[off] = 0;
    for (y = 0; y < off; y++) buffer[y] = data_source[x - off + y];
    for (y = 0; y < off; y++) if (buffer[y] < 0x20) buffer[y] = L' ';
    for (y = 0; y < 16 - off; y++) fwprintf(f,L"   ");
    fwprintf(f,L"| %s\n\n",buffer);
}

// Prints a block of data to a file and the console in a nice hex/ascii view.
void CRYPT_SplitPrintData(FILE* f,void* ds,DWORD data_size)
{
    if (!f)
    {
        CRYPT_PrintData(ds,data_size);
        return;
    }

    BYTE* data_source = (BYTE*)ds;
    DWORD x,y,off;
    wchar_t buffer[17];
    buffer[16] = 0;
    off = 0;
    wprintf(L"00000000 | ");
    fwprintf(f,L"00000000 | ");
    for (x = 0; x < data_size; x++)
    {
        if (off == 16)
        {
            for (y = 0; y < 16; y++)
            {
                if (data_source[x + y - 16] < 0x20) buffer[y] = L' ';
                else buffer[y] = data_source[x + y - 16];
            }
            wprintf(L"| %s\n%08X | ",buffer,x);
            fwprintf(f,L"| %s\n%08X | ",buffer,x);
            off = 0;
        }
        wprintf(L"%02X ",data_source[x]);
        fwprintf(f,L"%02X ",data_source[x]);
        off++;
    }
    buffer[off] = 0;
    for (y = 0; y < off; y++) buffer[y] = data_source[x - off + y];
    for (y = 0; y < off; y++) if (buffer[y] < 0x20) buffer[y] = L' ';
    for (y = 0; y < 16 - off; y++)
    {
        wprintf(L"   ");
        fwprintf(f,L"   ");
    }
    wprintf(L"| %s\n\n",buffer);
    fwprintf(f,L"| %s\n\n",buffer);
}

