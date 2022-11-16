#ifndef FALLOUT_PLIB_GNW_MEMORY_H_
#define FALLOUT_PLIB_GNW_MEMORY_H_

#include "memory_defs.h"

char* mem_strdup(const char* string);
void* mem_malloc(size_t size);
void* mem_realloc(void* ptr, size_t size);
void mem_free(void* ptr);
void mem_check();
void mem_register_func(MallocProc* mallocFunc, ReallocProc* reallocFunc, FreeProc* freeFunc);

#endif /* FALLOUT_PLIB_GNW_MEMORY_H_ */
