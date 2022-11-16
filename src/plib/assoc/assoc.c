#include "plib/assoc/assoc.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// NOTE: I guess this marker is used as a type discriminator for implementing
// nested dictionaries. That's why every dictionary-related function starts
// with a check for this value.
#define DICTIONARY_MARKER 0xFEBAFEBA

static void* default_malloc(size_t size);
static void* default_realloc(void* ptr, size_t newSize);
static void default_free(void* ptr);
static int assoc_find(Dictionary* dictionary, const char* key, int* index);
static int assoc_read_long(FILE* stream, int* valuePtr);
static int assoc_read_assoc_array(FILE* stream, Dictionary* dictionary);
static int assoc_write_long(FILE* stream, int value);
static int assoc_write_assoc_array(FILE* stream, Dictionary* dictionary);

// 0x51E408
static MallocProc* internal_malloc = default_malloc;

// 0x51E40C
static ReallocProc* internal_realloc = default_realloc;

// 0x51E410
static FreeProc* internal_free = default_free;

// 0x4D9B90
static void* default_malloc(size_t size)
{
    return malloc(size);
}

// 0x4D9B98
static void* default_realloc(void* ptr, size_t newSize)
{
    return realloc(ptr, newSize);
}

// 0x4D9BA0
static void default_free(void* ptr)
{
    free(ptr);
}

// 0x4D9BA8
int assoc_init(Dictionary* dictionary, int initialCapacity, size_t valueSize, DictionaryIO* io)
{
    dictionary->entriesCapacity = initialCapacity;
    dictionary->valueSize = valueSize;
    dictionary->entriesLength = 0;

    if (io != NULL) {
        memcpy(&(dictionary->io), io, sizeof(*io));
    } else {
        dictionary->io.readProc = NULL;
        dictionary->io.writeProc = NULL;
        dictionary->io.field_8 = 0;
        dictionary->io.field_C = 0;
    }

    int rc = 0;

    if (initialCapacity != 0) {
        dictionary->entries = (DictionaryEntry*)internal_malloc(sizeof(*dictionary->entries) * initialCapacity);
        if (dictionary->entries == NULL) {
            rc = -1;
        }
    } else {
        dictionary->entries = NULL;
    }

    if (rc != -1) {
        dictionary->marker = DICTIONARY_MARKER;
    }

    return rc;
}

// 0x4D9C0C
int assoc_resize(Dictionary* dictionary, int newCapacity)
{
    if (dictionary->marker != DICTIONARY_MARKER) {
        return -1;
    }

    if (newCapacity < dictionary->entriesLength) {
        return -1;
    }

    DictionaryEntry* entries = (DictionaryEntry*)internal_realloc(dictionary->entries, sizeof(*dictionary->entries) * newCapacity);
    if (entries == NULL) {
        return -1;
    }

    dictionary->entriesCapacity = newCapacity;
    dictionary->entries = entries;

    return 0;
}

// 0x4D9C48
int assoc_free(Dictionary* dictionary)
{
    if (dictionary->marker != DICTIONARY_MARKER) {
        return -1;
    }

    for (int index = 0; index < dictionary->entriesLength; index++) {
        DictionaryEntry* entry = &(dictionary->entries[index]);
        if (entry->key != NULL) {
            internal_free(entry->key);
        }

        if (entry->value != NULL) {
            internal_free(entry->value);
        }
    }

    if (dictionary->entries != NULL) {
        internal_free(dictionary->entries);
    }

    memset(dictionary, 0, sizeof(*dictionary));

    return 0;
}

// Finds index for the given key.
//
// Returns 0 if key is found. Otherwise returns -1, in this case [indexPtr]
// specifies an insertion point for given key.
//
// 0x4D9CC4
static int assoc_find(Dictionary* dictionary, const char* key, int* indexPtr)
{
    if (dictionary->marker != DICTIONARY_MARKER) {
        return -1;
    }

    if (dictionary->entriesLength == 0) {
        *indexPtr = 0;
        return -1;
    }

    int r = dictionary->entriesLength - 1;
    int l = 0;
    int mid = 0;
    int cmp = 0;
    while (r >= l) {
        mid = (l + r) / 2;

        cmp = stricmp(key, dictionary->entries[mid].key);
        if (cmp == 0) {
            break;
        }

        if (cmp > 0) {
            l = l + 1;
        } else {
            r = r - 1;
        }
    }

    if (cmp == 0) {
        *indexPtr = mid;
        return 0;
    }

    if (cmp < 0) {
        *indexPtr = mid;
    } else {
        *indexPtr = mid + 1;
    }

    return -1;
}

// Returns the index of the entry for the specified key, or -1 if it's not
// present in the dictionary.
//
// 0x4D9D5C
int assoc_search(Dictionary* dictionary, const char* key)
{
    if (dictionary->marker != DICTIONARY_MARKER) {
        return -1;
    }

    int index;
    if (assoc_find(dictionary, key, &index) != 0) {
        return -1;
    }

    return index;
}

// Adds key-value pair to the dictionary if the specified key is not already
// present.
//
// Returns 0 on success, or -1 on any error (including key already exists
// error).
//
// 0x4D9D88
int assoc_insert(Dictionary* dictionary, const char* key, const void* value)
{
    if (dictionary->marker != DICTIONARY_MARKER) {
        return -1;
    }

    int newElementIndex;
    if (assoc_find(dictionary, key, &newElementIndex) == 0) {
        // Element for this key is already exists.
        return -1;
    }

    if (dictionary->entriesLength == dictionary->entriesCapacity) {
        // Dictionary reached it's capacity and needs to be enlarged.
        if (assoc_resize(dictionary, 2 * (dictionary->entriesCapacity + 1)) == -1) {
            return -1;
        }
    }

    // Make a copy of the key.
    char* keyCopy = (char*)internal_malloc(strlen(key) + 1);
    if (keyCopy == NULL) {
        return -1;
    }

    strcpy(keyCopy, key);

    // Make a copy of the value.
    void* valueCopy = NULL;
    if (value != NULL && dictionary->valueSize != 0) {
        valueCopy = internal_malloc(dictionary->valueSize);
        if (valueCopy == NULL) {
            internal_free(keyCopy);
            return -1;
        }
    }

    if (valueCopy != NULL && dictionary->valueSize != 0) {
        memcpy(valueCopy, value, dictionary->valueSize);
    }

    // Starting at the end of entries array loop backwards and move entries down
    // one by one until we reach insertion point.
    for (int index = dictionary->entriesLength; index > newElementIndex; index--) {
        DictionaryEntry* src = &(dictionary->entries[index - 1]);
        DictionaryEntry* dest = &(dictionary->entries[index]);
        memcpy(dest, src, sizeof(*dictionary->entries));
    }

    DictionaryEntry* entry = &(dictionary->entries[newElementIndex]);
    entry->key = keyCopy;
    entry->value = valueCopy;

    dictionary->entriesLength++;

    return 0;
}

// Removes key-value pair from the dictionary if specified key is present in
// the dictionary.
//
// Returns 0 on success, -1 on any error (including key not present error).
//
// 0x4D9EE8
int assoc_delete(Dictionary* dictionary, const char* key)
{
    if (dictionary->marker != DICTIONARY_MARKER) {
        return -1;
    }

    int indexToRemove;
    if (assoc_find(dictionary, key, &indexToRemove) == -1) {
        return -1;
    }

    DictionaryEntry* entry = &(dictionary->entries[indexToRemove]);

    // Free key and value (which are copies).
    internal_free(entry->key);
    if (entry->value != NULL) {
        internal_free(entry->value);
    }

    dictionary->entriesLength--;

    // Starting from the index of the entry we've just removed, loop thru the
    // remaining of the array and move entries up one by one.
    for (int index = indexToRemove; index < dictionary->entriesLength; index++) {
        DictionaryEntry* src = &(dictionary->entries[index + 1]);
        DictionaryEntry* dest = &(dictionary->entries[index]);
        memcpy(dest, src, sizeof(*dictionary->entries));
    }

    return 0;
}

// NOTE: Unused.
//
// 0x4D9F84
int assoc_copy(Dictionary* dest, Dictionary* src)
{
    if (src->marker != DICTIONARY_MARKER) {
        return -1;
    }

    if (assoc_init(dest, src->entriesCapacity, src->valueSize, &(src->io)) != 0) {
        // FIXME: Should return -1, as we were unable to initialize dictionary.
        return 0;
    }

    for (int index = 0; index < src->entriesLength; index++) {
        DictionaryEntry* entry = &(src->entries[index]);
        if (assoc_insert(dest, entry->key, entry->value) == -1) {
            return -1;
        }
    }

    return 0;
}

// NOTE: Unused.
//
// 0x4DA090
static int assoc_read_long(FILE* stream, int* valuePtr)
{
    int ch;
    int value;

    ch = fgetc(stream);
    if (ch == -1) {
        return -1;
    }

    value = (ch & 0xFF);

    ch = fgetc(stream);
    if (ch == -1) {
        return -1;
    }

    value = (value << 8) | (ch & 0xFF);

    ch = fgetc(stream);
    if (ch == -1) {
        return -1;
    }

    value = (value << 8) | (ch & 0xFF);

    ch = fgetc(stream);
    if (ch == -1) {
        return -1;
    }

    value = (value << 8) | (ch & 0xFF);

    *valuePtr = value;

    return 0;
}

// NOTE: Unused.
//
// 0x4DA0F4
static int assoc_read_assoc_array(FILE* stream, Dictionary* dictionary)
{
    int value;

    if (assoc_read_long(stream, &value) != 0) return -1;
    dictionary->entriesLength = value;

    if (assoc_read_long(stream, &value) != 0) return -1;
    dictionary->entriesCapacity = value;

    if (assoc_read_long(stream, &value) != 0) return -1;
    dictionary->valueSize = value;

    if (assoc_read_long(stream, &value) != 0) return -1;
    // FIXME: Reading pointer.
    dictionary->entries = (DictionaryEntry*)value;

    return 0;
}

// NOTE: Unused.
//
// 0x4DA158
int assoc_load(FILE* stream, Dictionary* dictionary, int a3)
{
    if (dictionary->marker != DICTIONARY_MARKER) {
        return -1;
    }

    for (int index = 0; index < dictionary->entriesLength; index++) {
        DictionaryEntry* entry = &(dictionary->entries[index]);
        if (entry->key != NULL) {
            internal_free(entry->key);
        }

        if (entry->value != NULL) {
            internal_free(entry->value);
        }
    }

    if (dictionary->entries != NULL) {
        internal_free(dictionary->entries);
    }

    if (assoc_read_assoc_array(stream, dictionary) != 0) {
        return -1;
    }

    dictionary->entries = NULL;

    if (dictionary->entriesCapacity <= 0) {
        return 0;
    }

    dictionary->entries = (DictionaryEntry*)internal_malloc(sizeof(*dictionary->entries) * dictionary->entriesCapacity);
    if (dictionary->entries == NULL) {
        return -1;
    }

    for (int index = 0; index < dictionary->entriesLength; index++) {
        DictionaryEntry* entry = &(dictionary->entries[index]);
        entry->key = NULL;
        entry->value = NULL;
    }

    if (dictionary->entriesLength <= 0) {
        return 0;
    }

    for (int index = 0; index < dictionary->entriesLength; index++) {
        DictionaryEntry* entry = &(dictionary->entries[index]);
        int keyLength = fgetc(stream);
        if (keyLength == -1) {
            return -1;
        }

        entry->key = (char*)internal_malloc(keyLength + 1);
        if (entry->key == NULL) {
            return -1;
        }

        if (fgets(entry->key, keyLength, stream) == NULL) {
            return -1;
        }

        if (dictionary->valueSize != 0) {
            entry->value = internal_malloc(dictionary->valueSize);
            if (entry->value == NULL) {
                return -1;
            }

            if (dictionary->io.readProc != NULL) {
                if (dictionary->io.readProc(stream, entry->value, dictionary->valueSize, a3) != 0) {
                    return -1;
                }
            } else {
                if (fread(entry->value, dictionary->valueSize, 1, stream) != 1) {
                    return -1;
                }
            }
        }
    }

    return 0;
}

// NOTE: Unused.
//
// 0x4DA2EC
static int assoc_write_long(FILE* stream, int value)
{
    if (fputc((value >> 24) & 0xFF, stream) == -1) return -1;
    if (fputc((value >> 16) & 0xFF, stream) == -1) return -1;
    if (fputc((value >> 8) & 0xFF, stream) == -1) return -1;
    if (fputc(value & 0xFF, stream) == -1) return -1;

    return 0;
}

// NOTE: Unused.
//
// 0x4DA360
static int assoc_write_assoc_array(FILE* stream, Dictionary* dictionary)
{
    if (assoc_write_long(stream, dictionary->entriesLength) != 0) return -1;
    if (assoc_write_long(stream, dictionary->entriesCapacity) != 0) return -1;
    if (assoc_write_long(stream, dictionary->valueSize) != 0) return -1;
    // FIXME: Writing pointer.
    if (assoc_write_long(stream, (int)dictionary->entries) != 0) return -1;

    return 0;
}

// NOTE: Unused.
//
// 0x4DA3A4
int assoc_save(FILE* stream, Dictionary* dictionary, int a3)
{
    if (dictionary->marker != DICTIONARY_MARKER) {
        return -1;
    }

    if (assoc_write_assoc_array(stream, dictionary) != 0) {
        return -1;
    }

    for (int index = 0; index < dictionary->entriesLength; index++) {
        DictionaryEntry* entry = &(dictionary->entries[index]);
        int keyLength = strlen(entry->key);
        if (fputc(keyLength, stream) == -1) {
            return -1;
        }

        if (fputs(entry->key, stream) == -1) {
            return -1;
        }

        if (dictionary->io.writeProc != NULL) {
            if (dictionary->valueSize != 0) {
                if (dictionary->io.writeProc(stream, entry->value, dictionary->valueSize, a3) != 0) {
                    return -1;
                }
            }
        } else {
            if (dictionary->valueSize != 0) {
                if (fwrite(entry->value, dictionary->valueSize, 1, stream) != 1) {
                    return -1;
                }
            }
        }
    }

    return 0;
}

// 0x4DA498
void assoc_register_mem(MallocProc* mallocProc, ReallocProc* reallocProc, FreeProc* freeProc)
{
    if (mallocProc != NULL && reallocProc != NULL && freeProc != NULL) {
        internal_malloc = mallocProc;
        internal_realloc = reallocProc;
        internal_free = freeProc;
    } else {
        internal_malloc = default_malloc;
        internal_realloc = default_realloc;
        internal_free = default_free;
    }
}
