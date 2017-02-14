#include <windows.h>
#include <windows.h>
#include <stdio.h>

// Reads a hex byte from the given string. Simple and buggy.
int ParseHexByte(char* a)
{
    int value = 0;
    if ((a[0] >= '0') && (a[0] <= '9')) value += ((a[0] - '0') * 0x10);
    if ((a[0] >= 'A') && (a[0] <= 'F')) value += (((a[0] - 'A') + 0x0A) * 0x10);
    if ((a[0] >= 'a') && (a[0] <= 'f')) value += (((a[0] - 'a') + 0x0A) * 0x10);
    if ((a[1] >= '0') && (a[1] <= '9')) value += (a[1] - '0');
    if ((a[1] >= 'A') && (a[1] <= 'F')) value += ((a[1] - 'A') + 0x0A);
    if ((a[1] >= 'a') && (a[1] <= 'f')) value += ((a[1] - 'a') + 0x0A);
    return value;
}

// Escape codes didn't work to change the color, so I wrote a color version of printf.
// Basically, use "$XX" (where XX is the color code, in hex) to change the color of your text as it's printed.
int ConsolePrintColor(const char* format, ...)
{
    char buffer[0x400];
    va_list va;
    va_start(va,format);
    int rv = vsprintf(buffer,format,va);
    va_end(va);

    int x;
    for (x = 0; buffer[x]; x++)
    {
        if (buffer[x] == '$')
        {
            x += 2;
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),ParseHexByte(&buffer[x - 1]));
        } else putchar(buffer[x]);
    }

    return rv;
}

