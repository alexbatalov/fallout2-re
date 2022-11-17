#ifndef FALLOUT_GAME_TRAIT_H_
#define FALLOUT_GAME_TRAIT_H_

#include <stdbool.h>

#include "plib/db/db.h"
#include "game/trait_defs.h"

int trait_init();
void trait_reset();
void trait_exit();
int trait_load(File* stream);
int trait_save(File* stream);
void trait_set(int trait1, int trait2);
void trait_get(int* trait1, int* trait2);
char* trait_name(int trait);
char* trait_description(int trait);
int trait_pic(int trait);
bool trait_level(int trait);
int trait_adjust_stat(int stat);
int trait_adjust_skill(int skill);

#endif /* FALLOUT_GAME_TRAIT_H_ */
