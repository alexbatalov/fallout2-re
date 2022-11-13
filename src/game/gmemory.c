#include "game/gmemory.h"

#include "db.h"
#include "dictionary.h"
#include "memory.h"
#include "memory_manager.h"

// NOTE: Unused.
//
// 0x44B200
void* localmymalloc(size_t size)
{
    return internal_malloc_safe(size, __FILE__, __LINE__); // "gmemory.c", 22
}

// NOTE: Unused.
//
// 0x44B214
void* localmyrealloc(void* ptr, size_t size)
{
    return internal_realloc_safe(ptr, size, __FILE__, __LINE__); // "gmemory.c", 26
}

// NOTE: Unused.
//
// 0x44B228
void localmyfree(void* ptr)
{
    internal_free_safe(ptr, __FILE__, __LINE__); // "gmemory.c", 30
}

// NOTE: Unused.
//
// 0x44B23C
char* localmystrdup(const char* string)
{
    return strdup_safe(string, __FILE__, __LINE__); // "gmemory.c", 34
}

// 0x44B250
int gmemory_init()
{
    dictionarySetMemoryProcs(internal_malloc, internal_realloc, internal_free);
    _db_register_mem(internal_malloc, internal_strdup, internal_free);
    memoryManagerSetProcs(gmalloc, grealloc, gfree);

    return 0;
}

// 0x44B294
void* gmalloc(size_t size)
{
    return internal_malloc(size);
}

// 0x44B29C
void* grealloc(void* ptr, size_t newSize)
{
    return internal_realloc(ptr, newSize);
}

// 0x44B2A4
void gfree(void* ptr)
{
    internal_free(ptr);
}
