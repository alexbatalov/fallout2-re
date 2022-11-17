#include "game/gmemory.h"

#include "plib/db/db.h"
#include "plib/assoc/assoc.h"
#include "plib/gnw/memory.h"
#include "int/memdbg.h"

// NOTE: Unused.
//
// 0x44B200
void* localmymalloc(size_t size)
{
    return mymalloc(size, __FILE__, __LINE__); // "gmemory.c", 22
}

// NOTE: Unused.
//
// 0x44B214
void* localmyrealloc(void* ptr, size_t size)
{
    return myrealloc(ptr, size, __FILE__, __LINE__); // "gmemory.c", 26
}

// NOTE: Unused.
//
// 0x44B228
void localmyfree(void* ptr)
{
    myfree(ptr, __FILE__, __LINE__); // "gmemory.c", 30
}

// NOTE: Unused.
//
// 0x44B23C
char* localmystrdup(const char* string)
{
    return mystrdup(string, __FILE__, __LINE__); // "gmemory.c", 34
}

// 0x44B250
int gmemory_init()
{
    assoc_register_mem(mem_malloc, mem_realloc, mem_free);
    db_register_mem(mem_malloc, mem_strdup, mem_free);
    memoryRegisterAlloc(gmalloc, grealloc, gfree);

    return 0;
}

// 0x44B294
void* gmalloc(size_t size)
{
    return mem_malloc(size);
}

// 0x44B29C
void* grealloc(void* ptr, size_t newSize)
{
    return mem_realloc(ptr, newSize);
}

// 0x44B2A4
void gfree(void* ptr)
{
    mem_free(ptr);
}
