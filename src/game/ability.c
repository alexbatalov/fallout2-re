#include "game/ability.h"

#include <string.h>

#include "plib/gnw/memory.h"

// 0x410010
int abil_init(Ability* ability, int initialCapacity)
{
    ability->length = 0;
    ability->capacity = 0;
    ability->entries = NULL;

    if (initialCapacity != 0) {
        ability->entries = (AbilityData*)mem_malloc(sizeof(*ability->entries) * initialCapacity);
        if (ability->entries != NULL) {
            ability->length = 0;
            ability->capacity = initialCapacity;
            return 0;
        }
    }

    return -1;
}

// 0x410058
int abil_resize(Ability* ability, int capacity)
{
    AbilityData* entries;

    if (capacity >= ability->length) {
        entries = (AbilityData*)mem_realloc(ability->entries, sizeof(*ability->entries) * capacity);
        if (entries != NULL) {
            ability->entries = entries;
            ability->capacity = capacity;
            return 0;
        }
    }

    return -1;
}

// 0x410094
int abil_free(Ability* ability)
{
    if (ability->entries != NULL) {
        mem_free(ability->entries);
    }
    return 0;
}

// 0x4100A8
int abil_find(Ability* ability, int a2, int* indexPtr)
{
    int right;
    int left;
    int mid;
    int cmp;
    int rc;

    if (ability->length == 0) {
        rc = -1;
        *indexPtr = 0;
    } else {
        right = ability->length - 1;
        left = 0;

        while (right >= left) {
            mid = (right + left) >> 1;

            if (a2 == ability->entries[mid].field_0) {
                cmp = 0;
                break;
            }

            if (a2 < ability->entries[mid].field_0) {
                right = mid - 1;
                cmp = -1;
            } else {
                left = mid + 1;
                cmp = 1;
            }
        }

        if (cmp == 0) {
            *indexPtr = mid;
            rc = 0;
        } else {
            if (cmp < 0) {
                *indexPtr = mid;
            } else {
                *indexPtr = mid + 1;
            }
            rc = -1;
        }
    }

    return rc;
}

// 0x410130
int abil_search(Ability* ability, int a2)
{
    int index;

    if (abil_find(ability, a2, &index) == 0) {
        return index;
    }

    return -1;
}

// 0x410154
int abil_insert(Ability* ability, AbilityData* entry)
{
    int index;

    if (abil_find(ability, entry->field_0, &index) == 0) {
        return -1;
    }

    if (ability->capacity == ability->length && abil_resize(ability, ability->capacity * 2) == -1) {
        return -1;
    }

    if (sizeof(*ability->entries) * (ability->length - index) != 0) {
        memmove((unsigned char*)ability->entries + sizeof(*ability->entries) * index + sizeof(*ability->entries),
            (unsigned char*)ability->entries + sizeof(*ability->entries) * index,
            sizeof(*ability->entries) * (ability->length - index));
    }

    ability->entries[index].field_0 = entry->field_0;
    ability->entries[index].field_4 = entry->field_4;
    ability->entries[index].field_8 = entry->field_8;
    ability->length++;

    return 0;
}

// 0x41021C
int abil_delete(Ability* ability, int a2)
{
    int index;

    if (abil_find(ability, a2, &index) == -1) {
        return -1;
    }

    ability->length--;

    if (sizeof(*ability->entries) * (ability->length - index) != 0) {
        memmove((unsigned char*)ability->entries + sizeof(*ability->entries) * index,
            (unsigned char*)ability->entries + sizeof(*ability->entries) * index + sizeof(*ability->entries),
            sizeof(*ability->entries) * (ability->length - index));
    }

    return 0;
}

// 0x41026C
int abil_copy(Ability* dest, Ability* src)
{
    int index;

    if (abil_init(dest, src->capacity) == -1) {
        return -1;
    }

    for (index = 0; index < src->length; index++) {
        if (abil_insert(dest, &(src->entries[index])) == -1) {
            return -1;
        }
    }

    return 0;
}

// 0x4102B0
int abil_load(File* stream, Ability* ability)
{
    if (db_freadInt(stream, &(ability->length)) == -1
        || db_freadInt(stream, &(ability->capacity)) == -1
        || abil_read_ability_data(ability, stream) == -1) return -1;
    return 0;
}

// 0x4102EC
int abil_read_ability_data(Ability* ability, File* stream)
{
    int index;

    ability->entries = (AbilityData*)mem_malloc(sizeof(*ability->entries) * ability->capacity);
    if (ability->entries == NULL) {
        return -1;
    }

    for (index = 0; index < ability->length; index++) {
        if (db_freadInt(stream, &(ability->entries[index].field_0)) == -1
            || db_freadInt(stream, &(ability->entries[index].field_4)) == -1
            || db_freadInt(stream, &(ability->entries[index].field_8)) == -1) {
            return -1;
        }
    }

    return 0;
}

// 0x410378
int abil_save(File* stream, Ability* ability)
{
    if (db_fwriteInt(stream, ability->length) == -1
        || db_fwriteInt(stream, ability->capacity) == -1
        || abil_write_ability_data(ability->length, ability->entries, stream) == -1) {
        return -1;
    }

    return 0;
}

// 0x4103B8
int abil_write_ability_data(int length, AbilityData* entries, File* stream)
{
    int index;

    for (index = 0; index < length; index++) {
        if (db_fwriteInt(stream, entries[index].field_0) == -1
            || db_fwriteInt(stream, entries[index].field_4) == -1
            || db_fwriteInt(stream, entries[index].field_8) == -1) {
            return -1;
        }
    }

    return 0;
}
