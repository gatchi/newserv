#include <windows.h>
#include <stdio.h>

#include "encryption.h"
#include "psobb-table.h"

void L_CRYPT_BB_InitKey(unsigned char *data)
{
	unsigned x;
	for (x = 0; x < 48; x += 3)
	{
		data[x] ^= 0x19;
		data[x + 1] ^= 0x16;
		data[x + 2] ^= 0x18;
	}
}

void CRYPT_BB_Decrypt(CRYPT_SETUP* pcry,void* vdata,DWORD length)
{
    unsigned char* data = (unsigned char*)vdata;
	DWORD eax,ecx,edx,ebx,ebp,esi,edi;

    edx = 0;
    ecx = 0;
    eax = 0;
    while (edx < length)
	{
        ebx = *(DWORD *) &data[edx];
        ebx = ebx ^ pcry->keys[5];
        ebp = ((pcry->keys[(ebx >> 0x18) + 0x12]+pcry->keys[((ebx >> 0x10)& 0xff) + 0x112])
            ^ pcry->keys[((ebx >> 0x8)& 0xff) + 0x212]) + pcry->keys[(ebx & 0xff) + 0x312];
        ebp = ebp ^ pcry->keys[4];
		ebp ^= *(DWORD *) &data[edx+4];
        edi = ((pcry->keys[(ebp >> 0x18) + 0x12]+pcry->keys[((ebp >> 0x10)& 0xff) + 0x112])
            ^ pcry->keys[((ebp >> 0x8)& 0xff) + 0x212]) + pcry->keys[(ebp & 0xff) + 0x312];
        edi = edi ^ pcry->keys[3];
        ebx = ebx ^ edi;
        esi = ((pcry->keys[(ebx >> 0x18) + 0x12]+pcry->keys[((ebx >> 0x10)& 0xff) + 0x112])
            ^ pcry->keys[((ebx >> 0x8)& 0xff) + 0x212]) + pcry->keys[(ebx & 0xff) + 0x312];
        ebp = ebp ^ esi ^ pcry->keys[2];
        edi = ((pcry->keys[(ebp >> 0x18) + 0x12]+pcry->keys[((ebp >> 0x10)& 0xff) + 0x112])
            ^ pcry->keys[((ebp >> 0x8)& 0xff) + 0x212]) + pcry->keys[(ebp & 0xff) + 0x312];
        edi = edi ^ pcry->keys[1];
        ebp = ebp ^ pcry->keys[0];
        ebx = ebx ^ edi;
		*(DWORD *) &data[edx] = ebp;
		*(DWORD *) &data[edx+4] = ebx;
        edx = edx+8;
	}
}


void CRYPT_BB_Encrypt(CRYPT_SETUP* pcry,void* vdata,DWORD length)
{
    unsigned char* data = (unsigned char*)vdata;
	unsigned eax, ecx, edx, ebx, ebp, esi, edi;

    edx = 0;
    ecx = 0;
    eax = 0;
    while (edx < length)
	{
        ebx = *(DWORD *) &data[edx];
        ebx = ebx ^ pcry->keys[0];
        ebp = ((pcry->keys[(ebx >> 0x18) + 0x12]+pcry->keys[((ebx >> 0x10)& 0xff) + 0x112])
            ^ pcry->keys[((ebx >> 0x8)& 0xff) + 0x212]) + pcry->keys[(ebx & 0xff) + 0x312];
        ebp = ebp ^ pcry->keys[1];
		ebp ^= *(DWORD *) &data[edx+4];
        edi = ((pcry->keys[(ebp >> 0x18) + 0x12]+pcry->keys[((ebp >> 0x10)& 0xff) + 0x112])
            ^ pcry->keys[((ebp >> 0x8)& 0xff) + 0x212]) + pcry->keys[(ebp & 0xff) + 0x312];
        edi = edi ^ pcry->keys[2];
        ebx = ebx ^ edi;
        esi = ((pcry->keys[(ebx >> 0x18) + 0x12]+pcry->keys[((ebx >> 0x10)& 0xff) + 0x112])
            ^ pcry->keys[((ebx >> 0x8)& 0xff) + 0x212]) + pcry->keys[(ebx & 0xff) + 0x312];
        ebp = ebp ^ esi ^ pcry->keys[3];
        edi = ((pcry->keys[(ebp >> 0x18) + 0x12]+pcry->keys[((ebp >> 0x10)& 0xff) + 0x112])
            ^ pcry->keys[((ebp >> 0x8)& 0xff) + 0x212]) + pcry->keys[(ebp & 0xff) + 0x312];
        edi = edi ^ pcry->keys[4];
        ebp = ebp ^ pcry->keys[5];
        ebx = ebx ^ edi;
		*(DWORD *) &data[edx] = ebp;
		*(DWORD *) &data[edx+4] = ebx;
        edx = edx+8;
	}
}

void CRYPT_BB_CreateKeys(CRYPT_SETUP* pcry,void* salt)
{
	DWORD eax, ecx, edx, ebx, ebp, esi, edi, ou, x;
	unsigned char s[48];

	pcry->bb_posn = 0;

	memcpy(pcry->bb_seed,salt,48);
	memcpy(s, salt, sizeof(s));
	L_CRYPT_BB_InitKey(s);

    pcry->keys[0] = 0x243F6A88;
    pcry->keys[1] = 0x85A308D3;
    pcry->keys[2] = 0x13198A2E;
    pcry->keys[3] = 0x03707344;
    pcry->keys[4] = 0xA4093822;
    pcry->keys[5] = 0x299F31D0;
    pcry->keys[6] = 0x082EFA98;
    pcry->keys[7] = 0xEC4E6C89;
    pcry->keys[8] = 0x452821E6;
    pcry->keys[9] = 0x38D01377;
    pcry->keys[10] = 0xBE5466CF;
    pcry->keys[11] = 0x34E90C6C;
    pcry->keys[12] = 0xC0AC29B7;
    pcry->keys[13] = 0xC97C50DD;
    pcry->keys[14] = 0x3F84D5B5;
    pcry->keys[15] = 0xB5470917;
    pcry->keys[16] = 0x9216D5D9;
    pcry->keys[17] = 0x8979FB1B;
	memcpy(&pcry->keys[18], bbtable, sizeof(bbtable));

    ecx=0;
    ebx=0;

    while (ebx < 0x12)
	{
		ebp=((DWORD) (s[ecx])) << 0x18;
		eax=ecx+1;
		edx=eax-((eax / 48)*48);
		eax=(((DWORD) (s[edx])) << 0x10) & 0xFF0000;
		ebp=(ebp | eax) & 0xffff00ff;
		eax=ecx+2;
		edx=eax-((eax / 48)*48);
		eax=(((DWORD) (s[edx])) << 0x8) & 0xFF00;
		ebp=(ebp | eax) & 0xffffff00;
		eax=ecx+3;
		ecx=ecx+4;
		edx=eax-((eax / 48)*48);
		eax=(DWORD) (s[edx]);
		ebp=ebp | eax;
		eax=ecx;
		edx=eax-((eax / 48)*48);
		pcry->keys[ebx]=pcry->keys[ebx] ^ ebp;
		ecx=edx;
		ebx++;
	}

	ebp=0;
    esi=0;
    ecx=0;
    edi=0;
    ebx=0;
    edx=0x48;

    while (edi < edx)
	{
		esi=esi ^ pcry->keys[0];
		eax=esi >> 0x18;
		ebx=(esi >> 0x10) & 0xff;
		eax=pcry->keys[eax+0x12]+pcry->keys[ebx+0x112];
		ebx=(esi >> 8) & 0xFF;
		eax=eax ^ pcry->keys[ebx+0x212];
		ebx=esi & 0xff;
		eax=eax + pcry->keys[ebx+0x312];

		eax=eax ^ pcry->keys[1];
		ecx= ecx ^ eax;
		ebx=ecx >> 0x18;
		eax=(ecx >> 0x10) & 0xFF;
		ebx=pcry->keys[ebx+0x12]+pcry->keys[eax+0x112];
		eax=(ecx >> 8) & 0xff;
		ebx=ebx ^ pcry->keys[eax+0x212];
		eax=ecx & 0xff;
		ebx=ebx + pcry->keys[eax+0x312];

		for (x = 0; x <= 5; x++)
		{
			ebx=ebx ^ pcry->keys[(x*2)+2];
			esi= esi ^ ebx;
			ebx=esi >> 0x18;
			eax=(esi >> 0x10) & 0xFF;
			ebx=pcry->keys[ebx+0x12]+pcry->keys[eax+0x112];
			eax=(esi >> 8) & 0xff;
			ebx=ebx ^ pcry->keys[eax+0x212];
			eax=esi & 0xff;
			ebx=ebx + pcry->keys[eax+0x312];

			ebx=ebx ^ pcry->keys[(x*2)+3];
			ecx= ecx ^ ebx;
			ebx=ecx >> 0x18;
			eax=(ecx >> 0x10) & 0xFF;
			ebx=pcry->keys[ebx+0x12]+pcry->keys[eax+0x112];
			eax=(ecx >> 8) & 0xff;
			ebx=ebx ^ pcry->keys[eax+0x212];
			eax=ecx & 0xff;
			ebx=ebx + pcry->keys[eax+0x312];
		}

		ebx=ebx ^ pcry->keys[14];
		esi= esi ^ ebx;
		eax=esi >> 0x18;
		ebx=(esi >> 0x10) & 0xFF;
		eax=pcry->keys[eax+0x12]+pcry->keys[ebx+0x112];
		ebx=(esi >> 8) & 0xff;
		eax=eax ^ pcry->keys[ebx+0x212];
		ebx=esi & 0xff;
		eax=eax + pcry->keys[ebx+0x312];

		eax=eax ^ pcry->keys[15];
		eax= ecx ^ eax;
		ecx=eax >> 0x18;
		ebx=(eax >> 0x10) & 0xFF;
		ecx=pcry->keys[ecx+0x12]+pcry->keys[ebx+0x112];
		ebx=(eax >> 8) & 0xff;
		ecx=ecx ^ pcry->keys[ebx+0x212];
		ebx=eax & 0xff;
		ecx=ecx + pcry->keys[ebx+0x312];

		ecx=ecx ^ pcry->keys[16];
		ecx=ecx ^ esi;
		esi= pcry->keys[17];
		esi=esi ^ eax;
		pcry->keys[(edi / 4)]=esi;
		pcry->keys[(edi / 4)+1]=ecx;
		edi=edi+8;
	}


    eax=0;
    edx=0;
    ou=0;
    while (ou < 0x1000)
	{
        edi=0x48;
        edx=0x448;

        while (edi < edx)
		{
			esi=esi ^ pcry->keys[0];
			eax=esi >> 0x18;
			ebx=(esi >> 0x10) & 0xff;
			eax=pcry->keys[eax+0x12]+pcry->keys[ebx+0x112];
			ebx=(esi >> 8) & 0xFF;
			eax=eax ^ pcry->keys[ebx+0x212];
			ebx=esi & 0xff;
			eax=eax + pcry->keys[ebx+0x312];

			eax=eax ^ pcry->keys[1];
			ecx= ecx ^ eax;
			ebx=ecx >> 0x18;
			eax=(ecx >> 0x10) & 0xFF;
			ebx=pcry->keys[ebx+0x12]+pcry->keys[eax+0x112];
			eax=(ecx >> 8) & 0xff;
			ebx=ebx ^ pcry->keys[eax+0x212];
			eax=ecx & 0xff;
			ebx=ebx + pcry->keys[eax+0x312];

			for (x = 0; x <= 5; x++)
			{
				ebx=ebx ^ pcry->keys[(x*2)+2];
				esi= esi ^ ebx;
				ebx=esi >> 0x18;
				eax=(esi >> 0x10) & 0xFF;
				ebx=pcry->keys[ebx+0x12]+pcry->keys[eax+0x112];
				eax=(esi >> 8) & 0xff;
				ebx=ebx ^ pcry->keys[eax+0x212];
				eax=esi & 0xff;
				ebx=ebx + pcry->keys[eax+0x312];

				ebx=ebx ^ pcry->keys[(x*2)+3];
				ecx= ecx ^ ebx;
				ebx=ecx >> 0x18;
				eax=(ecx >> 0x10) & 0xFF;
				ebx=pcry->keys[ebx+0x12]+pcry->keys[eax+0x112];
				eax=(ecx >> 8) & 0xff;
				ebx=ebx ^ pcry->keys[eax+0x212];
				eax=ecx & 0xff;
				ebx=ebx + pcry->keys[eax+0x312];
			}

			ebx=ebx ^ pcry->keys[14];
			esi= esi ^ ebx;
			eax=esi >> 0x18;
			ebx=(esi >> 0x10) & 0xFF;
			eax=pcry->keys[eax+0x12]+pcry->keys[ebx+0x112];
			ebx=(esi >> 8) & 0xff;
			eax=eax ^ pcry->keys[ebx+0x212];
			ebx=esi & 0xff;
			eax=eax + pcry->keys[ebx+0x312];

			eax=eax ^ pcry->keys[15];
			eax= ecx ^ eax;
			ecx=eax >> 0x18;
			ebx=(eax >> 0x10) & 0xFF;
			ecx=pcry->keys[ecx+0x12]+pcry->keys[ebx+0x112];
			ebx=(eax >> 8) & 0xff;
			ecx=ecx ^ pcry->keys[ebx+0x212];
			ebx=eax & 0xff;
			ecx=ecx + pcry->keys[ebx+0x312];

			ecx=ecx ^ pcry->keys[16];
			ecx=ecx ^ esi;
			esi= pcry->keys[17];
			esi=esi ^ eax;
			pcry->keys[(ou / 4)+(edi / 4)]=esi;
			pcry->keys[(ou / 4)+(edi / 4)+1]=ecx;
			edi=edi+8;
		}
        ou=ou+0x400;
	}
}

void CRYPT_BB_DEBUG_PrintKeys(CRYPT_SETUP* cs,wchar_t* title)
{
    DWORD x,y;
    wprintf(L"\n%s\n### ###+0000 ###+0001 ###+0002 ###+0003 ###+0004 ###+0005 ###+0006 ###+0007\n",title);
    for (x = 0; x < 131; x++)
    {
        wprintf(L"%03d",x * 8);
        for (y = 0; y < 8; y++) wprintf(L" %08X",cs->keys[(x * 8) + y]);
        wprintf(L"\n");
    }
}

