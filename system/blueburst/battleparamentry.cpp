#include <windows.h>
#include <stdio.h>

// format of an entry in a battleparamentry file
typedef struct {
    WORD atp;
    WORD psv; // perseverance (intelligence?)
    WORD evp;
    WORD hp;
    WORD dfp;
    WORD ata;
    WORD lck;
    BYTE unknown[14];
    DWORD exp;
    DWORD unknown2;
} DUDEEXP;

// this program is simple; it just opens a battleparamentry file and reads entries from it until the end, with a key every 32 entries.
// not very difficult.
int main(int argc,char* argv[])
{
    DUDEEXP exp;
    DWORD x = 0,bytesread = 0x24;
    HANDLE file = CreateFile(argv[1],GENERIC_READ,0,NULL,OPEN_EXISTING,0,NULL);
    if (argc > 2)
    {
        FILE* f = fopen(argv[2],"wt");
        fprintf(f,"\n%s\n\n",argv[1]);
        while (bytesread == 0x24)
        {
            if ((x & ~0x1F) == x) fprintf(f,"[OFFSET] X    ATP  PSV  EVP  HP   DFP  ATA  LCK  ========================================= EXP      unknown2\n");
            ReadFile(file,&exp,0x24,&bytesread,NULL);
            fprintf(f,"%08X %04X %04X %04X %04X %04X %04X %04X %04X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %08X %08X\n",x * 0x24,x,exp.atp,exp.psv,exp.evp,exp.hp,exp.dfp,exp.ata,exp.lck,exp.unknown[0],exp.unknown[1],exp.unknown[2],exp.unknown[3],exp.unknown[4],exp.unknown[5],exp.unknown[6],exp.unknown[7],exp.unknown[8],exp.unknown[9],exp.unknown[10],exp.unknown[11],exp.unknown[12],exp.unknown[13],exp.exp,exp.unknown2);
            x++;
        }
        fclose(f);
    } else {
        printf("\n%s\n\n",argv[1]);
        while (bytesread == 0x24)
        {
            if ((x & ~0x1F) == x) printf("X    ATP  PSV  EVP  HP   DFP  ATA  LCK  EXP      unknown2\n");
            ReadFile(file,&exp,0x24,&bytesread,NULL);
            printf("%04X %04X %04X %04X %04X %04X %04X %04X %08X %08X\n",x,exp.atp,exp.psv,exp.evp,exp.hp,exp.dfp,exp.ata,exp.lck,exp.exp,exp.unknown2);
            x++;
        }
    }
    CloseHandle(file);
    return 0;
}

