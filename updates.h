// an index entry of a file to be updated
typedef struct {
    DWORD checksum;
    DWORD size;
    DWORD flags;
    char filename[0x40];
} UPDATE_FILE;

// a list of files to be updated
typedef struct {
    DWORD numFiles;
    UPDATE_FILE files[0];
} UPDATE_LIST;

long CalculateUpdateChecksum(void* data,unsigned long size);
UPDATE_LIST* LoadUpdateList(char* filename);
void DestroyUpdateList(UPDATE_LIST* list);

