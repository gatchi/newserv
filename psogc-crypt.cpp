#include <windows.h>
#include <stdio.h>

#include "encryption.h"

/* All source in this file was reversed from the original PPC ASM by
 * Fuzziqer Software, 2004 */

////////////////////////////////////////////////////////////////////////////////
// GameCube Encryption Source 

void CRYPT_GC_MixKeys(CRYPT_SETUP* cs)
{
    DWORD r0,r3,r4,*r5,*r6,*r7;
    r3 = (DWORD)cs;

    cs->gc_block_ptr = cs->keys;
    r5 = cs->keys;
    r6 = &(cs->keys[489]);
    r7 = cs->keys;

    while (r6 != cs->gc_block_end_ptr)
    {
        r0 = *(DWORD*)r6;
        r6++;
        r4 = *(DWORD*)r5;
        r0 ^= r4;
        *(DWORD*)r5 = r0;
        r5++;
    }

    while (r5 != cs->gc_block_end_ptr)
    {
        r0 = *(DWORD*)r7;
        r7++;
        r4 = *(DWORD*)r5;
        r0 ^= r4;
        *(DWORD*)r5 = r0;
        r5++;
    }
}

DWORD CRYPT_GC_GetNextKey(CRYPT_SETUP* cs)
{
    cs->gc_block_ptr++;
    if (cs->gc_block_ptr == cs->gc_block_end_ptr) CRYPT_GC_MixKeys(cs);
    return *cs->gc_block_ptr;
}

void CRYPT_GC_CreateKeys(CRYPT_SETUP* cs,DWORD seed)
{
    DWORD x,y,basekey,*source1,*source2,*source3;
    basekey = 0;

    cs->gc_seed = seed;

    cs->gc_block_end_ptr = &(cs->keys[521]);
    cs->gc_block_ptr = (DWORD*)cs->keys;

    for  (x = 0; x <= 16; x++)
    {
        for (y = 0; y < 32; y++)
        {
            seed = seed * 0x5D588B65;
            basekey = basekey >> 1;
            seed++;
            if (seed & 0x80000000) basekey = basekey | 0x80000000;
            else basekey = basekey & 0x7FFFFFFF;
        }
        *cs->gc_block_ptr = basekey;
        cs->gc_block_ptr = (DWORD*)((DWORD)cs->gc_block_ptr + 4);
    }
    source1 = &(cs->keys[0]);
    source2 = &(cs->keys[1]);
    cs->gc_block_ptr = (DWORD*)((DWORD)cs->gc_block_ptr - 4);
    (*cs->gc_block_ptr) = (((cs->keys[0] >> 9) ^ (*cs->gc_block_ptr << 23)) ^ cs->keys[15]);//cs->keys[15]);
    source3 = cs->gc_block_ptr;
    cs->gc_block_ptr = (DWORD*)((DWORD)cs->gc_block_ptr + 4);
    while (cs->gc_block_ptr != cs->gc_block_end_ptr)
    {
        *cs->gc_block_ptr = (*source3 ^ (((*source1 << 23) & 0xFF800000) ^ ((*source2 >> 9) & 0x007FFFFF)));
        cs->gc_block_ptr = (DWORD*)((DWORD)(cs->gc_block_ptr) + 4);
        source1 = (DWORD*)((DWORD)source1 + 4);
        source2 = (DWORD*)((DWORD)source2 + 4);
        source3 = (DWORD*)((DWORD)source3 + 4);
    }
    CRYPT_GC_MixKeys(cs);
    CRYPT_GC_MixKeys(cs);
    CRYPT_GC_MixKeys(cs);
    cs->gc_block_ptr = &(cs->keys[520]);
}

void CRYPT_GC_CryptData(CRYPT_SETUP* c,void* data,DWORD size)
{
    DWORD *address_start,*address_end;
    address_start = (DWORD*)data;
    address_end = (DWORD*)((DWORD)data + size);
    while (address_start < address_end)
    {
        *address_start = *address_start ^ CRYPT_GC_GetNextKey(c);
        address_start++;
    }
}

void CRYPT_GC_DEBUG_PrintKeys(CRYPT_SETUP* cs,wchar_t* title)
{
    DWORD x,y;
    wprintf(L"\n%s\n### ###+0000 ###+0001 ###+0002 ###+0003 ###+0004 ###+0005 ###+0006 ###+0007\n",title);
    for (x = 0; x < 66; x++)
    {
        wprintf(L"%03d",x * 8);
        for (y = 0; y < 8; y++) wprintf(L" %08X",cs->keys[(x * 8) + y]);
        wprintf(L"\n");
    }
}

