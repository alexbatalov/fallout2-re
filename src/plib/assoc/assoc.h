#ifndef FALLOUT_PLIB_ASSOC_ASSOC_H_
#define FALLOUT_PLIB_ASSOC_ASSOC_H_

#include <stdio.h>

#include "memory_defs.h"

typedef int(DictionaryReadProc)(FILE* stream, void* buffer, unsigned int size, int a4);
typedef int(DictionaryWriteProc)(FILE* stream, void* buffer, unsigned int size, int a4);

// NOTE: Last unnamed fields are likely seek, tell, and filelength.
typedef struct DictionaryIO {
    DictionaryReadProc* readProc;
    DictionaryWriteProc* writeProc;
    int field_8;
    int field_C;
    int field_10;
} DictionaryIO;

// A tuple containing individual key-value pair of a dictionary.
typedef struct DictionaryEntry {
    char* key;
    void* value;
} DictionaryEntry;

// A collection of key/value pairs.
//
// The keys in dictionary are always strings. Internally dictionary entries
// are kept sorted by the key. Both keys and values are copied when new entry
// is added to dictionary. For this reason the size of the value's type is
// provided during dictionary initialization.
typedef struct Dictionary {
    int marker;

    // The number of key/value pairs in the dictionary.
    int entriesLength;

    // The capacity of key/value pairs in [entries] array.
    int entriesCapacity;

    // The size of the dictionary values in bytes.
    size_t valueSize;

    // IO callbacks.
    DictionaryIO io;

    // The array of key-value pairs.
    DictionaryEntry* entries;
} Dictionary;

int assoc_init(Dictionary* dictionary, int initialCapacity, size_t valueSize, DictionaryIO* io);
int assoc_resize(Dictionary* dictionary, int newCapacity);
int assoc_free(Dictionary* dictionary);
int assoc_search(Dictionary* dictionary, const char* key);
int assoc_insert(Dictionary* dictionary, const char* key, const void* value);
int assoc_delete(Dictionary* dictionary, const char* key);
int assoc_copy(Dictionary* dest, Dictionary* src);
int assoc_load(FILE* stream, Dictionary* dictionary, int a3);
int assoc_save(FILE* stream, Dictionary* dictionary, int a3);
void assoc_register_mem(MallocProc* mallocProc, ReallocProc* reallocProc, FreeProc* freeProc);

#endif /* FALLOUT_PLIB_ASSOC_ASSOC_H_ */
