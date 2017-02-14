typedef struct {
    BYTE probability;
    BYTE itemCode[3];
} ITEM_RARE_DROP;

typedef struct {
    ITEM_RARE_DROP rares[0x65];    // 0000 - 0194 in file
    BYTE boxAreas[0x1E];           // 0194 - 01B2 in file
    ITEM_RARE_DROP boxRares[0x1E]; // 01B2 - 022A in file
    BYTE unused[0x56];
} ITEM_RARE_SET; // 0x280 in size; describes one difficulty, section ID, and episode 

ITEM_RARE_SET* LoadItemRareSet(char* filename,int episode,int difficulty,int secid);
void FreeItemRareSet(ITEM_RARE_SET* set);
bool DecideIfItemIsRare(BYTE pc);
