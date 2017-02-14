#include <windows.h>
#include <stdio.h>

/* drop rates are encoded into a single byte like this:

bits: SSSSSNNN

rate = (N + 7) << (S - 4) */

// compresses a DWORD drop rate into a byte
BYTE CompressDropRate(DWORD pc)
{
    DWORD x,y;
    BYTE exprate,factor,shift;

    for (shift = 31, y = pc; shift > 0; shift--) if ((pc >> shift) & 1) break;
    shift -= 4;
    factor = (pc >> (shift + 1)) & 15;
    if (factor >= 14) factor = 13;
    factor -= 7; // 6; 
    exprate = factor | ((shift + 4) << 3);

    return exprate;
}

// expands a byte drop rate into a DWORD
// this function was reverse-engineered from PSOGC
DWORD ExpandDropRate(BYTE pc)
{
    int shift = ((pc >> 3) & 0x1F) - 4;
    if (shift < 0) shift = 0;
    return ((2 << shift) * ((pc & 7) + 7));
}

// the usage string should tell it all.
int main(int argc,char* argv[])
{
    BYTE rate;
    long long exprate;
    double floatrate,numer,denom;

    if (argc < 2)
    {
        printf("> fuzziqer software drop rate calculator\n\n");
        printf("> to reverse a drop rate index, run the program as: %s <index>\n",argv[0]);
        printf("> to create a drop rate index, run the program as: %s *<num>,<denom>\n\n",argv[0]);
        printf("> examples:\n");
        printf("> > to find out what the drop rate 5F means:\n");
        printf("    %s 5F\n",argv[0]);
        printf("> > to convert 548/20384 into a PSO-style drop rate:\n");
        printf("    %s *548,20384\n",argv[0]);
        return 0;
    }

    if (argv[1][0] != '*')
    {
        sscanf(argv[1],"%2X",&rate);
        exprate = ExpandDropRate(rate);
        floatrate = ((double)exprate / 0x100000000);
        printf("> rate %02X is (%08X / 100000000), or %lf\n",rate,(long)exprate,floatrate);
    } else {
        sscanf(&argv[1][1],"%lf,%lf",&numer,&denom);
        exprate = (long)((numer / denom) * (double)0x100000000);
        printf("> rate (%lf / %lf) or (%08X / 100000000) or (%lf) is ",numer,denom,(long)exprate,numer / denom);
        rate = CompressDropRate(exprate);
        printf("%02X\n",rate);
    }

    return 0;
}

