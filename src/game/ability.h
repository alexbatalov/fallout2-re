#ifndef FALLOUT_GAME_ABILITY_H_
#define FALLOUT_GAME_ABILITY_H_

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

int abil_init(Ability* ability, int initialCapacity);
int abil_resize(Ability* ability, int capacity);
int abil_free(Ability* ability);
int abil_find(Ability* ability, int a2, int* indexPtr);
int abil_search(Ability* ability, int a2);
int abil_insert(Ability* ability, AbilityData* entry);
int abil_delete(Ability* ability, int a2);
int abil_copy(Ability* dest, Ability* src);
int abil_load(File* stream, Ability* ability);
int abil_read_ability_data(Ability* ability, File* stream);
int abil_save(File* stream, Ability* ability);
int abil_write_ability_data(int length, AbilityData* entries, File* stream);

#endif /* FALLOUT_GAME_ABILITY_H_ */
