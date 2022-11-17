#ifndef FALLOUT_GAME_PERK_H_
#define FALLOUT_GAME_PERK_H_

#include <stdbool.h>

#include "plib/db/db.h"
#include "game/object_types.h"
#include "game/perk_defs.h"

typedef struct PerkRankData {
    int ranks[PERK_COUNT];
} PerkRankData;

int perk_init();
void perk_reset();
void perk_exit();
int perk_load(File* stream);
int perk_save(File* stream);
PerkRankData* perkGetLevelData(Object* critter);
int perk_add(Object* critter, int perk);
int perk_add_force(Object* critter, int perk);
int perk_sub(Object* critter, int perk);
int perk_make_list(Object* critter, int* perks);
int perk_level(Object* critter, int perk);
char* perk_name(int perk);
char* perk_description(int perk);
int perk_skilldex_fid(int perk);
void perk_add_effect(Object* critter, int perk);
void perk_remove_effect(Object* critter, int perk);
int perk_adjust_skill(Object* critter, int skill);

// Returns true if perk is valid.
static inline bool perkIsValid(int perk)
{
    return perk >= 0 && perk < PERK_COUNT;
}

// Returns true if critter has at least one rank in specified perk.
//
// NOTE: Most perks have only 1 rank, which means dude either have perk, or
// not.
//
// On the other hand, there are several places in editor, where they made two
// consequtive calls to [perk_level], first to check for presence, then get
// the actual value for displaying. So a macro could exist, or this very
// function, but due to similarity to [perk_level] it could have been
// collapsed by compiler.
static inline bool perkHasRank(Object* critter, int perk)
{
    return perk_level(critter, perk) != 0;
}

#endif /* FALLOUT_GAME_PERK_H_ */
