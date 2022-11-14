#include "int/memdbg.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void defaultOutput(const char* string);
static int debug_printf(const char* format, ...);
static void error(const char* func, size_t size, const char* file, int line);
static void* defaultMalloc(size_t size);
static void* defaultRealloc(void* ptr, size_t size);
static void defaultFree(void* ptr);

// 0x519588
static MemoryManagerPrintErrorProc* outputFunc = defaultOutput;

// 0x51958C
static MallocProc* mallocPtr = defaultMalloc;

// 0x519590
static ReallocProc* reallocPtr = defaultRealloc;

// 0x519594
static FreeProc* freePtr = defaultFree;

// 0x4845B0
static void defaultOutput(const char* string)
{
    printf("%s", string);
}

// NOTE: Unused.
//
// 0x4845C0
void memoryRegisterDebug(MemoryManagerPrintErrorProc* func)
{
    outputFunc = func;
}

// 0x4845C8
static int debug_printf(const char* format, ...)
{
    // 0x631F7C
    static char buf[256];

    int length = 0;

    if (outputFunc != NULL) {
        va_list args;
        va_start(args, format);
        length = vsprintf(buf, format, args);
        va_end(args);

        outputFunc(buf);
    }

    return length;
}

// 0x484610
static void error(const char* func, size_t size, const char* file, int line)
{
    debug_printf("%s: Error allocating block of size %ld (%x), %s %d\n", func, size, size, file, line);
    exit(1);
}

// 0x48462C
static void* defaultMalloc(size_t size)
{
    return malloc(size);
}

// 0x484634
static void* defaultRealloc(void* ptr, size_t size)
{
    return realloc(ptr, size);
}

// 0x48463C
static void defaultFree(void* ptr)
{
    free(ptr);
}

// 0x484644
void memoryRegisterAlloc(MallocProc* mallocProc, ReallocProc* reallocProc, FreeProc* freeProc)
{
    mallocPtr = mallocProc;
    reallocPtr = reallocProc;
    freePtr = freeProc;
}

// 0x484660
void* mymalloc(size_t size, const char* file, int line)
{
    void* ptr = mallocPtr(size);
    if (ptr == NULL) {
        error("malloc", size, file, line);
    }

    return ptr;
}

// 0x4846B4
void* myrealloc(void* ptr, size_t size, const char* file, int line)
{
    ptr = reallocPtr(ptr, size);
    if (ptr == NULL) {
        error("realloc", size, file, line);
    }

    return ptr;
}

// 0x484688
void myfree(void* ptr, const char* file, int line)
{
    if (ptr == NULL) {
        debug_printf("free: free of a null ptr, %s %d\n", file, line);
        exit(1);
    }

    freePtr(ptr);
}

// 0x4846D8
void* mycalloc(int count, int size, const char* file, int line)
{
    void* ptr = mallocPtr(count * size);
    if (ptr == NULL) {
        error("calloc", size, file, line);
    }

    memset(ptr, 0, count * size);

    return ptr;
}

// 0x484710
char* mystrdup(const char* string, const char* file, int line)
{
    size_t size = strlen(string) + 1;
    char* copy = (char*)mallocPtr(size);
    if (copy == NULL) {
        error("strdup", size, file, line);
    }

    strcpy(copy, string);

    return copy;
}
