#include <windows.h>
#include <stdio.h>

#include "netconfig.h"

/* Add an entry to a comfiguration file. This function is only useful for
 * temporary entries, since configuration files cannot be saved. This is used
 * only as an internal procedure. */
CFGString* CFGAddEntry(CFGFile* cf)
{
    cf->numncs++;
    cf->ncs = (CFGString*)realloc(cf->ncs,sizeof(CFGString) * cf->numncs);
    if (!cf->ncs) return NULL;
    memset(&cf->ncs[cf->numncs - 1],0,sizeof(CFGString));
    return &cf->ncs[cf->numncs - 1];
}

/* Load a configuration file. The function fails if the file doesn't exist or something. */
CFGFile* CFGLoadFile(char* filename)
{
    /* open the file first */
    FILE* f = fopen(filename,"rt");
    if (!f) return NULL;
    CFGFile* cf = (CFGFile*)malloc(sizeof(CFGFile));
    if (!cf)
    {
        fclose(f);
        return NULL;
    }
    memset(cf,0,sizeof(CFGFile));

    /* loop over all lines in the file; max line length: 0x400 chars */
    char line[0x400];
    int x,y,length;
    CFGString* ts;
    while (fgets(line,0x400,f))
    {
        /* eliminate all nonstandard characters */
        while (line[0] & 0x80) strcpy(line,&line[1]);

        /* end the line at the first \r, \n, or # (for comments) */
        for (x = 0; line[x]; x++) if ((line[x] == '#') || (line[x] == '\r') || (line[x] == '\n')) break;
        line[x] = 0;
        length = x;

        /* find last non-whitespace character at the end of the line */
        for (x--; x >= 0; x--) if ((line[x] != ' ') && (line[x] != '\t')) break;
        if (x < 0) continue; /* line is all whitespace: skip it */
        line[x + 1] = 0; /* delete whitespace at the end */
        length = x + 1;

        /* create a new entry and fill it in */
        ts = CFGAddEntry(cf);
        if (!ts)
        {
            /* error! clean up and return NULL */
            CFGCloseFile(cf);
            fclose(f);
            return NULL;
        }

        /* fill in the name (until the first whitespace) */
        for (x = 0; (line[x] != 0) && (line[x] != ' ') && (line[x] != '\t'); x++) ts->name[x] = line[x];
        ts->name[x] = 0;

        /* skip all whitespace until the value and fill it in */
        for (; (line[x] == ' ') || (line[x] == '\t'); x++);
        for (y = 0; line[x] != 0; x++, y++) ts->valueA[y] = ts->valueW[y] = line[x];
    }

    /* close the file and return */
    fclose(f);
    return cf;
}

/* free memory associated with a CFGFile */
void CFGCloseFile(CFGFile* cf)
{
    free(cf->ncs);
    free(cf);
}

/* parses a number in decimal (or hexadecimal, if prefixed with 0x (case-insensitive)) */
long CFGParseNumber(const char* str)
{
    long number;
    if ((str[0] == '0') && ((str[1] == 'x') || (str[1] == 'X'))) sscanf(&str[2],"%lx",&number);
    else if ((str[0] == '0') && ((str[1] == 'o') || (str[1] == 'O'))) sscanf(&str[2],"%lo",&number);
    else sscanf(str,"%ld",&number);
    return number;
}

/* returns 1 if there's an entry with the given name, 0 otherwise. */
bool CFGIsValuePresent(CFGFile* cf,char* name)
{
    DWORD x;
    for (x = 0; x < cf->numncs; x++) if (!strcmp(name,cf->ncs[x].name)) return 1;
    return 0;
}

/* returns the number represented by the specified name. if the name does not exist,
 * this function returns 0. therefore, it is NOT SAFE to use this without CFGIsValuePresent(). */
long CFGGetNumber(CFGFile* cf,char* name)
{
    char* str = CFGGetStringA(cf,name);
    if (!str) return 0;
    return CFGParseNumber(str);
}

/* returns the ANSI representation of the string represented by name, or NULL if it's nonexistant */
char* CFGGetStringA(CFGFile* cf,char* name)
{
    DWORD x;
    for (x = 0; x < cf->numncs; x++) if (!strcmp(name,cf->ncs[x].name)) return cf->ncs[x].valueA;
    return NULL;
}

/* returns the ANSI representation of the string represented by name, or "[not found]" if it's nonexistant */
char* CFGGetStringSafeA(CFGFile* cf,char* name)
{
    DWORD x;
    for (x = 0; x < cf->numncs; x++) if (!strcmp(name,cf->ncs[x].name)) return cf->ncs[x].valueA;
    return "[not found]";
}

/* returns the Unicode representation of the string represented by name, or NULL if it's nonexistant */
wchar_t* CFGGetStringW(CFGFile* cf,char* name)
{
    DWORD x;
    for (x = 0; x < cf->numncs; x++) if (!strcmp(name,cf->ncs[x].name)) return cf->ncs[x].valueW;
    return NULL;
}

/* returns the Unicode representation of the string represented by name, or "[not found]" if it's nonexistant */
wchar_t* CFGGetStringSafeW(CFGFile* cf,char* name)
{
    DWORD x;
    for (x = 0; x < cf->numncs; x++) if (!strcmp(name,cf->ncs[x].name)) return cf->ncs[x].valueW;
    return L"[not found]";
}

/* Enumerates each value in the specified CFGFile, calling a CFGEnumRoutine for each one.
 * See CFGEnumRoutine in netconfig.h for mor information. */
bool CFGEnumValues(CFGFile* cf,CFGEnumRoutine func,long param)
{
    unsigned int x,y;
    for (x = 0; x < cf->numncs; )
    {
        y = func(cf,cf->ncs[x].name,cf->ncs[x].valueA,cf->ncs[x].valueW,param);
        if (y) x += y;
        else return 0;
    }
    return 1;
}

