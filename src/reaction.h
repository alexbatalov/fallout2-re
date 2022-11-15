#ifndef REACTION_H
#define REACTION_H

#include "game/object_types.h"

typedef enum NpcReaction {
    NPC_REACTION_BAD,
    NPC_REACTION_NEUTRAL,
    NPC_REACTION_GOOD,
} NpcReaction;

int reactionSetValue(Object* critter, int a2);
int level_to_reaction();
int reactionTranslateValue(int a1);
int _reaction_influence_();
int reaction_to_level_internal(int sid, int reaction);
int reactionGetValue(Object* critter);

#endif /* REACTION_H */
