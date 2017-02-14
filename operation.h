// Structure used to lock resources (spinlock) 
typedef struct {
    void* pcOriginalRequester;
    unsigned long time;
    unsigned long threadID;
    unsigned long lockCount;
} OPERATION_LOCK;

extern "C" {
    void __stdcall operation_lock(void* oper);
    void __stdcall operation_unlock(void* oper);
};
