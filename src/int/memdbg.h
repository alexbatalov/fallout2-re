#ifndef FALLOUT_INT_MEMDBG_H_
#define FALLOUT_INT_MEMDBG_H_

#include "memory_defs.h"

typedef void(MemoryManagerPrintErrorProc)(const char* string);

void memoryRegisterAlloc(MallocProc* mallocProc, ReallocProc* reallocProc, FreeProc* freeProc);
void* mymalloc(size_t size, const char* file, int line);
void* myrealloc(void* ptr, size_t size, const char* file, int line);
void myfree(void* ptr, const char* file, int line);
void* mycalloc(int count, int size, const char* file, int line);
char* mystrdup(const char* string, const char* file, int line);

#endif /* FALLOUT_INT_MEMDBG_H_ */
