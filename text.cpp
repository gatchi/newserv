#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "text.h"

// None of these functions truly convert between SJIS and Unicode. They will
// convert English properly (and some other languages as well), but Japanese
// text will screw up horribly. I was planning to fix this later.

// Converts Unicode to SJIS.
int tx_convert_to_sjis(char* a,wchar_t* b)
{
    int x = (-1);
    do {
        *a = *b;
        a++;
        b++;
        x++;
    } while (*b);
    *a = *b;
    return x;
}

#ifndef UNICODE
int tx_convert_to_sjis(char* a,tx* b)
{
    int x = (-1);
    do {
        *a = *b;
        a++;
        b++;
        x++;
    } while (*b);
    *a = *b;
    return x;
}
#endif

// Converts SJIS to Unicode.
int tx_convert_to_unicode(wchar_t* a,char* b)
{
    int x = (-1);
    do {
        *a = *b;
        a++;
        b++;
        x++;
    } while (*b);
    *a = *b;
    return x;
}

#ifdef UNICODE
int tx_convert_to_unicode(wchar_t* a,tx* b)
{
    int x = (-1);
    do {
        *a = *b;
        a++;
        b++;
        x++;
    } while (*b);
    *a = *b;
    return x;
}
#endif

// Adds a $E or $J (or $whatever) before a string.
bool tx_add_language_marker(char* a,char e)
{
    if (a[0] != '\t') memmove(&a[2],a,strlen(a) + 1);
    else if (a[1] == 'C') memmove(&a[2],a,strlen(a) + 1);
    a[0] = '\t';
    a[1] = e;
    return true;
}

bool tx_add_language_marker(wchar_t* a,wchar_t e)
{
    if (a[0] != L'\t') memmove(&a[2],a,2 * (wcslen(a) + 1));
    else if (a[1] == L'C') memmove(&a[2],a,2 * (wcslen(a) + 1));
    a[0] = L'\t';
    a[1] = e;
    return true;
}

// Removes the $E, $J, etc. from the beginning of a string.
bool tx_remove_language_marker(char* a)
{
    if ((a[0] == '\t') && (a[1] != 'C')) strcpy(a,&a[2]);
    return true;
}

bool tx_remove_language_marker(wchar_t* a)
{
    if ((a[0] == L'\t') && (a[1] != L'C')) wcscpy(a,&a[2]);
    return true;
}

// Replaces all instances of a character in a string.
int tx_replace_char(char* a,char f,char r)
{
    while (*a)
    {
        if (*a == f) *a = r;
        a++;
    }
    return 0;
}

int tx_replace_char(wchar_t* a,wchar_t f,wchar_t r)
{
    while (*a)
    {
        if (*a == f) *a = r;
        a++;
    }
    return 0;
}

// the following functions convert all $ into tabs, all # into line breaks, and
// replace %<reps> with <repd>, for any corresponding elements of reps and repd.

char reps[] = {'d','x','p','+','1','2','3','c','l','y','X','Y','Z','?','C','R','s','%','n',0};
char repd[] = {'÷','×','¶','±','¹','²','³','¢','£','¥','¼','½','¾','¿','©','®','$','%','#',0};
wchar_t wreps[] = {L'd',L'x',L'p',L'+',L'1',L'2',L'3',L'c',L'l',L'y',L'X',L'Y',L'Z',L'?',L'C',L'R',L's',L'%',L'n',0};
wchar_t wrepd[] = {'÷','×','¶','±','¹','²','³','¢','£','¥','¼','½','¾','¿','©','®','$','%','#',0};

int tx_add_color(char* a)
{
    long x;
    while (*a)
    {
        if (*a == '$') *a = '\t';
        if (*a == '#') *a = '\n';
        if (*a == '%')
        {
            for (x = 0; reps[x]; x++) if (reps[x] == a[1]) break;
            if (reps[x])
            {
                strcpy(a,&a[1]);
                *a = repd[x];
            }
        }
        a++;
    }
    return 0;
}

int tx_add_color(wchar_t* a)
{
    unsigned long x;
    while (*a)
    {
        if (*a == L'$') *a = L'\t';
        if (*a == L'#') *a = L'\n';
        if (*a == '%')
        {
            for (x = 0; wreps[x]; x++) if (wreps[x] == a[1]) break;
            if (wreps[x])
            {
                wcscpy(a,&a[1]);
                *a = wrepd[x];
            }
        }
        a++;
    }
    return 0;
}

// Checksums an amount of data.
long checksum(void* data,int size)
{
    long cs = 0xA486F731;
    int shift = 0;
    while (size)
    {
        if (size == 1) cs ^= *(unsigned char*)data;
        else {
            shift = *(unsigned char*)((unsigned long)data + 1) % 25;
            cs ^= *(unsigned char*)data << shift;
        }
        data = (void*)((long)data + 1);
        size--;
    }
    return cs;
}

// Reads formatted data from a string. For example, the string
// "hi there" 0A 12345678 #30 'pizza'
// would become:
// 68 69 20 74 68 65 72 65 0A 12 34 56 78 1E 00 00 00 70 00 69 00 7A 00 7A 00 61 00
// to better understand how to use it, see the code below.
int tx_read_string_data(char* buffer,unsigned long length,void* data,unsigned long max)
{
    if (!length) length = strlen(buffer);
    unsigned long x,size = 0;
    int chr;
    bool read,string = false,unicodestring = false,high = true;
    unsigned char* cmdbuffer = (unsigned char*)data;
    for (x = 0; (x < length) && (size < max); x++)
    {
        read = false;
        // strings in "" are copied exactly into the data
        if (string)
        {
            if (buffer[x] == '\"') string = false;
            else {
                cmdbuffer[size] = buffer[x];
                size++;
            }
        // strings in '' are copied into the data as Unicode
        } else if (unicodestring)
        {
            if (buffer[x] == '\'') unicodestring = false;
            else {
                cmdbuffer[size] = buffer[x];
                cmdbuffer[size + 1] = 0;
                size += 2;
            }
        // #<decimal> is inserted as a DWORD
        } else if (buffer[x] == '#')
        {
            sscanf(&buffer[x + 1],"%ld",(unsigned long*)&cmdbuffer[size]);
            size += 4;
        // any hex data is parsed and added to the data
        } else {
            if ((buffer[x] >= '0') && (buffer[x] <= '9'))
            {
                read = true;
                chr |= (buffer[x] - '0');
            }
            if ((buffer[x] >= 'A') && (buffer[x] <= 'F'))
            {
                read = true;
                chr |= (buffer[x] - 'A' + 0x0A);
            }
            if ((buffer[x] >= 'a') && (buffer[x] <= 'f'))
            {
                read = true;
                chr |= (buffer[x] - 'a' + 0x0A);
            }
            if (buffer[x] == '\"') string = true;
            if (buffer[x] == '\'') unicodestring = true;
        }
        if (read)
        {
            if (high) chr = chr << 4;
            else {
                cmdbuffer[size] = chr;
                chr = 0;
                size++;
            }
            high = !high;
        }
    }
    return size;
}

// same function, but for Unicode strings (first converts the string to SJIS.... bad, I know)
int tx_read_string_data(wchar_t* buffer,unsigned long length,void* data,unsigned long max)
{
    if (!length) length = wcslen(buffer);
    char* abuffer = (char*)malloc(length + 1);
    tx_convert_to_sjis(abuffer,buffer);
    int rv = tx_read_string_data(abuffer,length,data,max);
    free(abuffer);
    return rv;
}

