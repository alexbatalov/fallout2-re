#ifndef FALLOUT_GAME_REACTION_H_
#define FALLOUT_GAME_REACTION_H_

#include "game/object_types.h"

typedef enum NpcReaction {
    NPC_REACTION_BAD,
    NPC_REACTION_NEUTRAL,
    NPC_REACTION_GOOD,
} NpcReaction;

int reaction_set(Object* critter, int value);
int level_to_reaction();
int reaction_lookup_internal(int a1);
int reaction_to_level_internal(int sid, int reaction);
int reaction_to_level(int a1);
int reaction_get(Object* critter);
int reaction_roll();
int reaction_influence();

#endif /* FALLOUT_GAME_REACTION_H_ */
