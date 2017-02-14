#include <windows.h>
#include <stdio.h>

// an entry in the streamfile index
typedef struct {
    DWORD size;
    DWORD crc;
    DWORD offset;
    char name[0x40]; } entry;

// the streamfile index format
typedef struct {
    DWORD num;
    entry entries[1]; } index;

int __stdcall WinMain(HINSTANCE,HINSTANCE,char*,int)
{
    void* data;
    DWORD x,y,z,size,bytesread;
    HANDLE dst,src;
    index* i;

    // load the index
    printf("> opening streamfile.ind\n");
    dst = CreateFile("streamfile.ind",GENERIC_READ,0,NULL,OPEN_EXISTING,0,NULL);
    if (dst == INVALID_HANDLE_VALUE)
    {
        printf("> could not open streamfile.ind\n");
        system("PAUSE>NUL");
        return 0;
    }
    size = GetFileSize(dst,NULL);
    i = (index*)malloc(size);
    ReadFile(dst,i,size,&bytesread,NULL);
    CloseHandle(dst);

    // open the streamfile data
    src = CreateFile("streamfile.pbb",GENERIC_READ,0,NULL,OPEN_EXISTING,0,NULL);
    if (src == INVALID_HANDLE_VALUE)
    {
        printf("> could not open streamfile.pbb\n");
        system("PAUSE>NUL");
        return 0;
    }

    // for each file, open a destination file with the corresponding name and copy its data from the streamfile
    for (x = 0; x < i->num; x++)
    {
        dst = CreateFile(i->entries[x].name,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);
        data = malloc(i->entries[x].size);
        SetFilePointer(src,i->entries[x].offset,NULL,FILE_BEGIN);
        ReadFile(src,data,i->entries[x].size,&bytesread,NULL);
        WriteFile(dst,data,i->entries[x].size,&bytesread,NULL);
        CloseHandle(dst);
        free(data);
    }

    // close the streamfile data
    free(i);
    CloseHandle(src);
    printf("> done");
    system("PAUSE>NUL");
    return 0;
}

