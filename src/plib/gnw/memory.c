#include "plib/gnw/memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "plib/gnw/debug.h"
#include "window_manager.h"

// A special value that denotes a beginning of a memory block data.
#define MEMORY_BLOCK_HEADER_GUARD 0xFEEDFACE

// A special value that denotes an ending of a memory block data.
#define MEMORY_BLOCK_FOOTER_GUARD 0xBEEFCAFE

// A header of a memory block.
typedef struct MemoryBlockHeader {
    // Size of the memory block including header and footer.
    size_t size;

    // See [MEMORY_BLOCK_HEADER_GUARD].
    int guard;
} MemoryBlockHeader;

// A footer of a memory block.
typedef struct MemoryBlockFooter {
    // See [MEMORY_BLOCK_FOOTER_GUARD].
    int guard;
} MemoryBlockFooter;

static void* my_malloc(size_t size);
static void* my_realloc(void* ptr, size_t size);
static void my_free(void* ptr);
static void* mem_prep_block(void* block, size_t size);
static void mem_check_block(void* block);

// 0x51DED0
static MallocProc* p_malloc = my_malloc;

// 0x51DED4
static ReallocProc* p_realloc = my_realloc;

// 0x51DED8
static FreeProc* p_free = my_free;

// 0x51DEDC
static int num_blocks = 0;

// 0x51DEE0
static int max_blocks = 0;

// 0x51DEE4
static size_t mem_allocated = 0;

// 0x51DEE8
static size_t max_allocated = 0;

// 0x4C5A80
char* mem_strdup(const char* string)
{
    char* copy = NULL;
    if (string != NULL) {
        copy = (char*)p_malloc(strlen(string) + 1);
        strcpy(copy, string);
    }
    return copy;
}

// 0x4C5AD0
void* mem_malloc(size_t size)
{
    return p_malloc(size);
}

// 0x4C5AD8
static void* my_malloc(size_t size)
{
    void* ptr = NULL;

    if (size != 0) {
        size += sizeof(MemoryBlockHeader) + sizeof(MemoryBlockFooter);

        unsigned char* block = (unsigned char*)malloc(size);
        if (block != NULL) {
            // NOTE: Uninline.
            ptr = mem_prep_block(block, size);

            num_blocks++;
            if (num_blocks > max_blocks) {
                max_blocks = num_blocks;
            }

            mem_allocated += size;
            if (mem_allocated > max_allocated) {
                max_allocated = mem_allocated;
            }
        }
    }

    return ptr;
}

// 0x4C5B50
void* mem_realloc(void* ptr, size_t size)
{
    return p_realloc(ptr, size);
}

// 0x4C5B58
static void* my_realloc(void* ptr, size_t size)
{
    if (ptr != NULL) {
        unsigned char* block = (unsigned char*)ptr - sizeof(MemoryBlockHeader);

        MemoryBlockHeader* header = (MemoryBlockHeader*)block;
        size_t oldSize = header->size;

        mem_allocated -= oldSize;

        mem_check_block(block);

        if (size != 0) {
            size += sizeof(MemoryBlockHeader) + sizeof(MemoryBlockFooter);
        }

        unsigned char* newBlock = (unsigned char*)realloc(block, size);
        if (newBlock != NULL) {
            mem_allocated += size;
            if (mem_allocated > max_allocated) {
                max_allocated = mem_allocated;
            }

            // NOTE: Uninline.
            ptr = mem_prep_block(newBlock, size);
        } else {
            if (size != 0) {
                mem_allocated += oldSize;

                debug_printf("%s,%u: ", __FILE__, __LINE__); // "Memory.c", 195
                debug_printf("Realloc failure.\n");
            } else {
                num_blocks--;
            }
            ptr = NULL;
        }
    } else {
        ptr = p_malloc(size);
    }

    return ptr;
}

// 0x4C5C24
void mem_free(void* ptr)
{
    p_free(ptr);
}

// 0x4C5C2C
static void my_free(void* ptr)
{
    if (ptr != NULL) {
        void* block = (unsigned char*)ptr - sizeof(MemoryBlockHeader);
        MemoryBlockHeader* header = (MemoryBlockHeader*)block;

        mem_check_block(block);

        mem_allocated -= header->size;
        num_blocks--;

        free(block);
    }
}

// NOTE: Not used.
//
// 0x4C5C5C
void mem_check()
{
    if (p_malloc == my_malloc) {
        debug_printf("Current memory allocated: %6d blocks, %9u bytes total\n", num_blocks, mem_allocated);
        debug_printf("Max memory allocated:     %6d blocks, %9u bytes total\n", max_blocks, max_allocated);
    }
}

// NOTE: Unused.
//
// 0x4C5CA8
void mem_register_func(MallocProc* mallocFunc, ReallocProc* reallocFunc, FreeProc* freeFunc)
{
    if (!gWindowSystemInitialized) {
        p_malloc = mallocFunc;
        p_realloc = reallocFunc;
        p_free = freeFunc;
    }
}

// NOTE: Inlined.
//
// 0x4C5CC4
static void* mem_prep_block(void* block, size_t size)
{
    MemoryBlockHeader* header;
    MemoryBlockFooter* footer;

    header = (MemoryBlockHeader*)block;
    header->guard = MEMORY_BLOCK_HEADER_GUARD;
    header->size = size;

    footer = (MemoryBlockFooter*)((unsigned char*)block + size - sizeof(*footer));
    footer->guard = MEMORY_BLOCK_FOOTER_GUARD;

    return (unsigned char*)block + sizeof(*header);
}

// Validates integrity of the memory block.
//
// [block] is a pointer to the the memory block itself, not it's data.
//
// 0x4C5CE4
static void mem_check_block(void* block)
{
    MemoryBlockHeader* header = (MemoryBlockHeader*)block;
    if (header->guard != MEMORY_BLOCK_HEADER_GUARD) {
        debug_printf("Memory header stomped.\n");
    }

    MemoryBlockFooter* footer = (MemoryBlockFooter*)((unsigned char*)block + header->size - sizeof(MemoryBlockFooter));
    if (footer->guard != MEMORY_BLOCK_FOOTER_GUARD) {
        debug_printf("Memory footer stomped.\n");
    }
}
