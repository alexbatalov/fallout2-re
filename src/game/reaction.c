#include "game/reaction.h"

#include "game/scripts.h"

// 0x4A29D0
int reaction_set(Object* critter, int value)
{
    scriptSetLocalVar(critter->sid, 0, value);
    return 0;
}

// NOTE: Unused.
//
// 0x4A29E4
int level_to_reaction()
{
    return 0;
}

// 0x4A29E8
int reaction_lookup_internal(int a1)
{
    if (a1 > 10) {
        return NPC_REACTION_GOOD;
    } else if (a1 > -10) {
        return NPC_REACTION_NEUTRAL;
    } else if (a1 > -25) {
        return NPC_REACTION_BAD;
    } else if (a1 > -50) {
        return NPC_REACTION_BAD;
    } else if (a1 > -75) {
        return NPC_REACTION_BAD;
    } else {
        return NPC_REACTION_BAD;
    }
}

// NOTE: Unused.
//
// 0x4A2A2C
int reaction_to_level_internal(int sid, int reaction)
{
    int level;

    if (reaction <= -75) {
        level = -4;
    } else if (reaction <= -50) {
        level = -3;
    } else if (reaction <= -25) {
        level = -2;
    } else if (reaction <= -10) {
        level = -1;
    } else if (reaction <= 10) {
        level = 0;
    } else if (reaction <= 25) {
        level = 1;
    } else if (reaction <= 50) {
        level = 2;
    } else if (reaction <= 75) {
        level = 3;
    } else {
        level = 4;
    }

    scriptSetLocalVar(sid, 1, level);

    return reaction_lookup_internal(reaction);
}

// NOTE: From OS X binary.
int reaction_to_level(int a1)
{
    return reaction_lookup_internal(a1);
}

// 0x4A2B28
int reaction_get(Object* critter)
{
    int reactionValue;

    if (scriptGetLocalVar(critter->sid, 0, &reactionValue) == -1) {
        return -1;
    }

    return reactionValue;
}

// NOTE: From OS X binary.
int reaction_roll()
{
    return 0;
}

// TODO: Accepts 3 parameters, check `op_reaction_influence`.
//
// 0x4A29F0
int reaction_influence()
{
    return 0;
}
