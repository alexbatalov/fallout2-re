#ifndef ABILITY_H
#define ABILITY_H

#include "db.h"

typedef struct AbilityData {
    int field_0;
    int field_4;
    int field_8;
} AbilityData;

typedef struct Ability {
    int length;
    int capacity;
    AbilityData* entries;
} Ability;

int _abil_init(Ability* ability, int initialCapacity);
int _abil_resize(Ability* ability, int capacity);
int _abil_free(Ability* ability);
int _abil_find(Ability* ability, int a2, int* indexPtr);
int _abil_search(Ability* ability, int a2);
int _abil_insert(Ability* ability, AbilityData* entry);
int _abil_delete(Ability* ability, int a2);
int _abil_copy(Ability* dest, Ability* src);
int _abil_load(File* stream, Ability* ability);
int _abil_read_ability_data(Ability* ability, File* stream);
int _abil_save(File* stream, Ability* ability);
int _abil_write_ability_data(int length, AbilityData* entries, File* stream);

#endif /* ABILITY_H */
