#include "combat.h"

#include "actions.h"
#include "animation.h"
#include "art.h"
#include "color.h"
#include "combat_ai.h"
#include "core.h"
#include "critter.h"
#include "db.h"
#include "debug.h"
#include "display_monitor.h"
#include "draw.h"
#include "elevator.h"
#include "game.h"
#include "game_config.h"
#include "game_mouse.h"
#include "game_sound.h"
#include "interface.h"
#include "item.h"
#include "loadsave.h"
#include "map.h"
#include "memory.h"
#include "object.h"
#include "perk.h"
#include "pipboy.h"
#include "proto.h"
#include "queue.h"
#include "random.h"
#include "scripts.h"
#include "skill.h"
#include "stat.h"
#include "text_font.h"
#include "tile.h"
#include "trait.h"
#include "window_manager.h"

#include <stdio.h>

// 0x500B50
char byte_500B50[] = ".";

// 0x51093C
int dword_51093C = 0;

// 0x510940
int dword_510940 = 0;

// 0x510944
unsigned int gCombatState = COMBAT_STATE_0x02;

// 0x510948
STRUCT_510948* off_510948 = NULL;

// 0x51094C
STRUCT_664980* off_51094C = NULL;

// 0x510950
bool dword_510950 = false;

// Accuracy modifiers for hit locations.
//
// 0x510954
const int dword_510954[HIT_LOCATION_COUNT] = {
    -40,
    -30,
    -30,
    0,
    -20,
    -20,
    -60,
    -30,
    0,
};

// Critical hit tables for every kill type.
//
// 0x510978
CriticalHitDescription gCriticalHitTables[KILL_TYPE_COUNT][HIT_LOCATION_COUNT][CRTICIAL_EFFECT_COUNT] = {
    // KILL_TYPE_MAN
    {
        // HIT_LOCATION_HEAD
        {
            { 4, 0, -1, 0, 0, 5001, 5000 },
            { 4, DAM_BYPASS, STAT_ENDURANCE, 0, DAM_KNOCKED_OUT, 5002, 5003 },
            { 5, DAM_BYPASS, STAT_ENDURANCE, -3, DAM_KNOCKED_OUT, 5002, 5003 },
            { 5, DAM_KNOCKED_DOWN | DAM_BYPASS, STAT_ENDURANCE, -3, DAM_KNOCKED_OUT, 5004, 5003 },
            { 6, DAM_KNOCKED_OUT | DAM_BYPASS, STAT_LUCK, 0, DAM_BLIND, 5005, 5006 },
            { 6, DAM_DEAD, -1, 0, 0, 5007, 5000 },
        },
        // HIT_LOCATION_LEFT_ARM
        {
            { 3, 0, -1, 0, 0, 5008, 5000 },
            { 3, DAM_LOSE_TURN, -1, 0, 0, 5009, 5000 },
            { 4, 0, STAT_ENDURANCE, -3, DAM_CRIP_ARM_LEFT, 5010, 5011 },
            { 4, DAM_CRIP_ARM_LEFT | DAM_BYPASS, -1, 0, 0, 5012, 5000 },
            { 4, DAM_CRIP_ARM_LEFT | DAM_BYPASS, -1, 0, 0, 5012, 5000 },
            { 4, DAM_CRIP_ARM_LEFT | DAM_BYPASS, -1, 0, 0, 5013, 5000 },
        },
        // HIT_LOCATION_RIGHT_ARM
        {
            { 3, 0, -1, 0, 0, 5008, 5000 },
            { 3, DAM_LOSE_TURN, -1, 0, 0, 5009, 5000 },
            { 4, 0, STAT_ENDURANCE, -3, DAM_CRIP_ARM_RIGHT, 5014, 5000 },
            { 4, DAM_CRIP_ARM_RIGHT | DAM_BYPASS, -1, 0, 0, 5015, 5000 },
            { 4, DAM_CRIP_ARM_RIGHT | DAM_BYPASS, -1, 0, 0, 5015, 5000 },
            { 4, DAM_CRIP_ARM_RIGHT | DAM_BYPASS, -1, 0, 0, 5013, 5000 },
        },
        // HIT_LOCATION_TORSO
        {
            { 3, 0, -1, 0, 0, 5016, 5000 },
            { 3, DAM_BYPASS, -1, 0, 0, 5017, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 5019, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 5019, 5000 },
            { 6, DAM_KNOCKED_OUT | DAM_BYPASS, -1, 0, 0, 5020, 5000 },
            { 6, DAM_DEAD, -1, 0, 0, 5021, 5000 },
        },
        // HIT_LOCATION_RIGHT_LEG
        {
            { 3, DAM_KNOCKED_DOWN, -1, 0, 0, 5023, 5000 },
            { 3, DAM_KNOCKED_DOWN, STAT_ENDURANCE, 0, DAM_CRIP_LEG_RIGHT, 5023, 5024 },
            { 4, DAM_KNOCKED_DOWN, STAT_ENDURANCE, -3, DAM_CRIP_LEG_RIGHT, 5023, 5024 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_RIGHT | DAM_BYPASS, -1, 0, 0, 5025, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_RIGHT | DAM_BYPASS, STAT_ENDURANCE, 0, DAM_KNOCKED_OUT, 5025, 5026 },
            { 4, DAM_KNOCKED_OUT | DAM_CRIP_LEG_RIGHT | DAM_BYPASS, -1, 0, 0, 5026, 5000 },
        },
        // HIT_LOCATION_LEFT_LEG
        {
            { 3, DAM_KNOCKED_DOWN, -1, 0, 0, 5023, 5000 },
            { 3, DAM_KNOCKED_DOWN, STAT_ENDURANCE, 0, DAM_CRIP_LEG_LEFT, 5023, 5024 },
            { 4, DAM_KNOCKED_DOWN, STAT_ENDURANCE, -3, DAM_CRIP_LEG_LEFT, 5023, 5024 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_LEFT | DAM_BYPASS, -1, 0, 0, 5025, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_LEFT | DAM_BYPASS, STAT_ENDURANCE, 0, DAM_KNOCKED_OUT, 5025, 5026 },
            { 4, DAM_KNOCKED_OUT | DAM_CRIP_LEG_LEFT | DAM_BYPASS, -1, 0, 0, 5026, 5000 },
        },
        // HIT_LOCATION_EYES
        {
            { 4, 0, STAT_LUCK, 4, DAM_BLIND, 5027, 5028 },
            { 4, DAM_BYPASS, STAT_LUCK, 3, DAM_BLIND, 5029, 5028 },
            { 6, DAM_BYPASS, STAT_LUCK, 2, DAM_BLIND, 5029, 5028 },
            { 6, DAM_BLIND | DAM_BYPASS | DAM_LOSE_TURN, -1, 0, 0, 5030, 5000 },
            { 8, DAM_KNOCKED_OUT | DAM_BLIND | DAM_BYPASS, -1, 0, 0, 5031, 5000 },
            { 8, DAM_DEAD, -1, 0, 0, 5032, 5000 },
        },
        // HIT_LOCATION_GROIN
        {
            { 3, 0, -1, 0, 0, 5033, 5000 },
            { 3, DAM_BYPASS, STAT_ENDURANCE, -3, DAM_KNOCKED_DOWN, 5034, 5035 },
            { 3, DAM_KNOCKED_DOWN, STAT_ENDURANCE, -3, DAM_KNOCKED_OUT, 5035, 5036 },
            { 3, DAM_KNOCKED_OUT, -1, 0, 0, 5036, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, STAT_ENDURANCE, 0, DAM_KNOCKED_OUT, 5035, 5036 },
            { 4, DAM_KNOCKED_OUT | DAM_BYPASS, -1, 0, 0, 5037, 5000 },
        },
        // HIT_LOCATION_UNCALLED
        {
            { 3, 0, -1, 0, 0, 5016, 5000 },
            { 3, DAM_BYPASS, -1, 0, 0, 5017, 5000 },
            { 4, 0, -1, 0, 0, 5018, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 5019, 5000 },
            { 6, DAM_KNOCKED_OUT | DAM_BYPASS, -1, 0, 0, 5020, 5000 },
            { 6, DAM_DEAD, -1, 0, 0, 5021, 5000 },
        },
    },
    // KILL_TYPE_WOMAN
    {
        // HIT_LOCATION_HEAD
        {
            { 4, 0, -1, 0, 0, 5101, 5100 },
            { 4, DAM_BYPASS, STAT_ENDURANCE, 0, DAM_KNOCKED_OUT, 5102, 5103 },
            { 6, DAM_BYPASS, STAT_ENDURANCE, -3, DAM_KNOCKED_OUT, 5102, 5103 },
            { 6, DAM_KNOCKED_DOWN | DAM_BYPASS, STAT_ENDURANCE, -3, DAM_KNOCKED_OUT, 5104, 5103 },
            { 6, DAM_KNOCKED_OUT | DAM_BYPASS, STAT_LUCK, 0, DAM_BLIND, 5105, 5106 },
            { 6, DAM_DEAD, -1, 0, 0, 5107, 5000 },
        },
        // HIT_LOCATION_LEFT_ARM
        {
            { 3, 0, -1, 0, 0, 5108, 5100 },
            { 3, DAM_LOSE_TURN, -1, 0, 0, 5109, 5100 },
            { 4, 0, STAT_ENDURANCE, -2, DAM_CRIP_ARM_LEFT, 5110, 5111 },
            { 4, 0, STAT_ENDURANCE, -4, DAM_CRIP_ARM_LEFT, 5110, 5111 },
            { 4, DAM_CRIP_ARM_LEFT | DAM_BYPASS, -1, 0, 0, 5112, 5100 },
            { 4, DAM_CRIP_ARM_LEFT | DAM_BYPASS, -1, 0, 0, 5113, 5100 },
        },
        // HIT_LOCATION_RIGHT_ARM
        {
            { 3, 0, -1, 0, 0, 5108, 5100 },
            { 3, DAM_LOSE_TURN, -1, 0, 0, 5109, 5100 },
            { 4, 0, STAT_ENDURANCE, -2, DAM_CRIP_ARM_RIGHT, 5114, 5100 },
            { 4, 0, STAT_ENDURANCE, -4, DAM_CRIP_ARM_RIGHT, 5114, 5100 },
            { 4, DAM_CRIP_ARM_RIGHT | DAM_BYPASS, -1, 0, 0, 5115, 5100 },
            { 4, DAM_CRIP_ARM_RIGHT | DAM_BYPASS, -1, 0, 0, 5113, 5100 },
        },
        // HIT_LOCATION_TORSO
        {
            { 3, 0, -1, 0, 0, 5116, 5100 },
            { 3, DAM_BYPASS, -1, 0, 0, 5117, 5100 },
            { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 5119, 5100 },
            { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 5119, 5100 },
            { 6, DAM_KNOCKED_OUT | DAM_BYPASS, -1, 0, 0, 5120, 5100 },
            { 6, DAM_DEAD, -1, 0, 0, 5121, 5100 },
        },
        // HIT_LOCATION_RIGHT_LEG
        {
            { 3, DAM_KNOCKED_DOWN, -1, 0, 0, 5123, 5100 },
            { 4, DAM_KNOCKED_DOWN, STAT_ENDURANCE, 0, DAM_CRIP_LEG_RIGHT, 5123, 5124 },
            { 4, DAM_KNOCKED_DOWN, STAT_ENDURANCE, -3, DAM_CRIP_LEG_RIGHT, 5123, 5124 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_RIGHT | DAM_BYPASS, -1, 0, 0, 5125, 5100 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_RIGHT | DAM_BYPASS, STAT_ENDURANCE, 0, DAM_KNOCKED_OUT, 5125, 5126 },
            { 4, DAM_KNOCKED_OUT | DAM_CRIP_LEG_RIGHT | DAM_BYPASS, -1, 0, 0, 5126, 5100 },
        },
        // HIT_LOCATION_LEFT_LEG
        {
            { 3, DAM_KNOCKED_DOWN, -1, 0, 0, 5123, 5100 },
            { 4, DAM_KNOCKED_DOWN, STAT_ENDURANCE, 0, DAM_CRIP_LEG_LEFT, 5123, 5124 },
            { 4, DAM_KNOCKED_DOWN, STAT_ENDURANCE, -3, DAM_CRIP_LEG_LEFT, 5123, 5124 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_LEFT | DAM_BYPASS, -1, 0, 0, 5125, 5100 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_LEFT | DAM_BYPASS, STAT_ENDURANCE, 0, DAM_KNOCKED_OUT, 5125, 5126 },
            { 4, DAM_KNOCKED_OUT | DAM_CRIP_LEG_LEFT | DAM_BYPASS, -1, 0, 0, 5126, 5100 },
        },
        // HIT_LOCATION_EYES
        {
            { 4, 0, STAT_LUCK, 4, DAM_BLIND, 5127, 5128 },
            { 4, DAM_BYPASS, STAT_LUCK, 3, DAM_BLIND, 5129, 5128 },
            { 6, DAM_BYPASS, STAT_LUCK, 2, DAM_BLIND, 5129, 5128 },
            { 6, DAM_BLIND | DAM_BYPASS | DAM_LOSE_TURN, -1, 0, 0, 5130, 5100 },
            { 8, DAM_KNOCKED_OUT | DAM_BLIND | DAM_BYPASS, -1, 0, 0, 5131, 5100 },
            { 8, DAM_DEAD, -1, 0, 0, 5132, 5100 },
        },
        // HIT_LOCATION_GROIN
        {
            { 3, 0, -1, 0, 0, 5133, 5100 },
            { 3, 0, STAT_ENDURANCE, 0, DAM_KNOCKED_DOWN, 5133, 5134 },
            { 3, DAM_KNOCKED_DOWN, STAT_ENDURANCE, 0, DAM_KNOCKED_OUT, 5134, 5135 },
            { 3, DAM_KNOCKED_OUT, -1, 0, 0, 5135, 5100 },
            { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, STAT_ENDURANCE, 0, DAM_KNOCKED_OUT, 5134, 5135 },
            { 4, DAM_KNOCKED_OUT | DAM_BYPASS, -1, 0, 0, 5135, 5100 },
        },
        // HIT_LOCATION_UNCALLED
        {
            { 3, 0, -1, 0, 0, 5116, 5100 },
            { 3, DAM_BYPASS, -1, 0, 0, 5117, 5100 },
            { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 5119, 5100 },
            { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 5119, 5100 },
            { 6, DAM_KNOCKED_OUT | DAM_BYPASS, -1, 0, 0, 5120, 5100 },
            { 6, DAM_DEAD, -1, 0, 0, 5121, 5100 },
        },
    },
    // KILL_TYPE_CHILD
    {
        // HIT_LOCATION_HEAD
        {
            { 4, 0, STAT_ENDURANCE, 0, DAM_KNOCKED_OUT, 5200, 5201 },
            { 4, DAM_BYPASS, STAT_ENDURANCE, -2, DAM_KNOCKED_OUT, 5202, 5203 },
            { 4, DAM_BYPASS, STAT_ENDURANCE, -2, DAM_KNOCKED_OUT, 5202, 5203 },
            { 6, DAM_KNOCKED_OUT | DAM_BYPASS, -1, 0, 0, 5203, 5000 },
            { 6, DAM_KNOCKED_OUT | DAM_BYPASS, -1, 0, 0, 5203, 5000 },
            { 6, DAM_DEAD, -1, 0, 0, 5204, 5000 },
        },
        // HIT_LOCATION_LEFT_ARM
        {
            { 3, 0, -1, 0, 0, 5205, 5000 },
            { 4, DAM_LOSE_TURN, STAT_ENDURANCE, 0, DAM_CRIP_ARM_LEFT, 5206, 5207 },
            { 4, DAM_LOSE_TURN, STAT_ENDURANCE, -2, DAM_CRIP_ARM_LEFT, 5206, 5207 },
            { 4, DAM_CRIP_ARM_LEFT | DAM_BYPASS, -1, 0, 0, 5208, 5000 },
            { 4, DAM_CRIP_ARM_LEFT | DAM_BYPASS, -1, 0, 0, 5208, 5000 },
            { 4, DAM_CRIP_ARM_LEFT | DAM_BYPASS, -1, 0, 0, 5208, 5000 },
        },
        // HIT_LOCATION_RIGHT_ARM
        {
            { 3, 0, -1, 0, 0, 5209, 5000 },
            { 4, DAM_LOSE_TURN, STAT_ENDURANCE, 0, DAM_CRIP_ARM_RIGHT, 5206, 5207 },
            { 4, DAM_LOSE_TURN, STAT_ENDURANCE, -2, DAM_CRIP_ARM_RIGHT, 5206, 5207 },
            { 4, DAM_CRIP_ARM_RIGHT | DAM_BYPASS, -1, 0, 0, 5208, 5000 },
            { 4, DAM_CRIP_ARM_RIGHT | DAM_BYPASS, -1, 0, 0, 5208, 5000 },
            { 4, DAM_CRIP_ARM_RIGHT | DAM_BYPASS, -1, 0, 0, 5208, 5000 },
        },
        // HIT_LOCATION_TORSO
        {
            { 3, 0, -1, 0, 0, 5210, 5000 },
            { 4, DAM_BYPASS, -1, 0, 0, 5211, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 5212, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 5212, 5000 },
            { 4, DAM_KNOCKED_OUT | DAM_BYPASS, -1, 0, 0, 5213, 5000 },
            { 6, DAM_DEAD, -1, 0, 0, 5214, 5000 },
        },
        // HIT_LOCATION_RIGHT_LEG
        {
            { 3, 0, -1, 0, 0, 5215, 5000 },
            { 3, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_RIGHT, -1, 0, DAM_CRIP_ARM_RIGHT | DAM_BLIND | DAM_ON_FIRE | DAM_EXPLODE, 5000, 0 },
            { 3, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_RIGHT, -1, 0, DAM_CRIP_ARM_RIGHT | DAM_BLIND | DAM_ON_FIRE | DAM_EXPLODE, 5000, 0 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_RIGHT | DAM_BYPASS, -1, 0, 0, 5217, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_RIGHT | DAM_BYPASS, -1, 0, 0, 5217, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_RIGHT | DAM_BYPASS, -1, 0, 0, 5217, 5000 },
        },
        // HIT_LOCATION_LEFT_LEG
        {
            { 3, 0, -1, 0, 0, 5215, 5000 },
            { 3, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_LEFT, -1, 0, DAM_CRIP_ARM_RIGHT | DAM_BLIND | DAM_ON_FIRE | DAM_EXPLODE, 5000, 0 },
            { 3, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_LEFT, -1, 0, DAM_CRIP_ARM_RIGHT | DAM_BLIND | DAM_ON_FIRE | DAM_EXPLODE, 5000, 0 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_LEFT | DAM_BYPASS, -1, 0, 0, 5217, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_LEFT | DAM_BYPASS, -1, 0, 0, 5217, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_LEFT | DAM_BYPASS, -1, 0, 0, 5217, 5000 },
        },
        // HIT_LOCATION_EYES
        {
            { 4, 0, STAT_LUCK, 5, DAM_BLIND, 5218, 5219 },
            { 4, DAM_BYPASS, STAT_LUCK, 2, DAM_BLIND, 5220, 5221 },
            { 6, DAM_BYPASS, STAT_LUCK, -1, DAM_BLIND, 5220, 5221 },
            { 6, DAM_BLIND | DAM_BYPASS | DAM_LOSE_TURN, -1, 0, 0, 5222, 5000 },
            { 8, DAM_KNOCKED_OUT | DAM_BLIND | DAM_BYPASS, -1, 0, 0, 5223, 5000 },
            { 8, DAM_DEAD, -1, 0, 0, 5224, 5000 },
        },
        // HIT_LOCATION_GROIN
        {
            { 3, 0, -1, 0, 0, 5225, 5000 },
            { 3, 0, -1, 0, 0, 5225, 5000 },
            { 3, DAM_KNOCKED_DOWN, -1, 0, 0, 5226, 5000 },
            { 3, DAM_KNOCKED_DOWN, -1, 0, 0, 5226, 5000 },
            { 3, DAM_KNOCKED_DOWN, -1, 0, 0, 5226, 5000 },
            { 4, DAM_KNOCKED_DOWN, -1, 0, 0, 5226, 5000 },
        },
        // HIT_LOCATION_UNCALLED
        {
            { 3, 0, -1, 0, 0, 5210, 5000 },
            { 3, DAM_BYPASS, -1, 0, 0, 5211, 5000 },
            { 4, DAM_BYPASS, -1, 0, 0, 5211, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 5212, 5000 },
            { 4, DAM_KNOCKED_OUT | DAM_BYPASS, -1, 0, 0, 5213, 5000 },
            { 6, DAM_DEAD, -1, 0, 0, 5214, 5000 },
        },
    },
    // KILL_TYPE_SUPER_MUTANT
    {
        // HIT_LOCATION_HEAD
        {
            { 4, 0, -1, 0, 0, 5300, 5000 },
            { 4, DAM_BYPASS, STAT_ENDURANCE, -1, DAM_KNOCKED_DOWN, 5301, 5302 },
            { 5, DAM_BYPASS, STAT_ENDURANCE, -4, DAM_KNOCKED_DOWN, 5301, 5302 },
            { 5, DAM_KNOCKED_DOWN | DAM_BYPASS, STAT_ENDURANCE, -3, DAM_KNOCKED_OUT, 5302, 5303 },
            { 6, DAM_KNOCKED_DOWN | DAM_BYPASS, STAT_ENDURANCE, 0, DAM_KNOCKED_OUT, 5302, 5303 },
            { 6, DAM_DEAD, -1, 0, 0, 5304, 5000 },
        },
        // HIT_LOCATION_LEFT_ARM
        {
            { 3, 0, -1, 0, 0, 5300, 5000 },
            { 3, 0, STAT_AGILITY, 0, DAM_LOSE_TURN, 5300, 5306 },
            { 4, DAM_BYPASS | DAM_LOSE_TURN, STAT_ENDURANCE, -1, DAM_CRIP_ARM_LEFT, 5307, 5308 },
            { 4, DAM_BYPASS | DAM_LOSE_TURN, STAT_ENDURANCE, -3, DAM_CRIP_ARM_LEFT, 5307, 5308 },
            { 4, DAM_CRIP_ARM_LEFT | DAM_BYPASS, -1, 0, 0, 5308, 5000 },
            { 4, DAM_CRIP_ARM_LEFT | DAM_BYPASS, -1, 0, 0, 5308, 5000 },
        },
        // HIT_LOCATION_RIGHT_ARM
        {
            { 3, 0, -1, 0, 0, 5300, 5000 },
            { 3, 0, STAT_AGILITY, 0, DAM_LOSE_TURN, 5300, 5006 },
            { 4, DAM_BYPASS | DAM_LOSE_TURN, STAT_ENDURANCE, -1, DAM_CRIP_ARM_RIGHT, 5307, 5309 },
            { 4, DAM_BYPASS | DAM_LOSE_TURN, STAT_ENDURANCE, -3, DAM_CRIP_ARM_RIGHT, 5307, 5309 },
            { 4, DAM_CRIP_ARM_RIGHT | DAM_BYPASS, -1, 0, 0, 5309, 5000 },
            { 4, DAM_CRIP_ARM_RIGHT | DAM_BYPASS, -1, 0, 0, 5309, 5000 },
        },
        // HIT_LOCATION_TORSO
        {
            { 3, 0, -1, 0, 0, 5300, 5000 },
            { 3, DAM_BYPASS, -1, 0, 0, 5301, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 5302, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 5302, 5000 },
            { 4, DAM_KNOCKED_OUT | DAM_BYPASS, -1, 0, 0, 5310, 5000 },
            { 6, DAM_DEAD, -1, 0, 0, 5311, 5000 },
        },
        // HIT_LOCATION_RIGHT_LEG
        {
            { 3, 0, -1, 0, 0, 5300, 5000 },
            { 3, 0, STAT_AGILITY, 0, DAM_KNOCKED_DOWN, 5300, 5312 },
            { 4, DAM_KNOCKED_DOWN, STAT_ENDURANCE, -3, DAM_CRIP_LEG_RIGHT, 5312, 5313 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_RIGHT, -1, 0, 0, 5313, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_RIGHT | DAM_BYPASS, -1, 0, 0, 5314, 5000 },
            { 4, DAM_KNOCKED_OUT | DAM_CRIP_LEG_RIGHT | DAM_BYPASS, -1, 0, 0, 5315, 5000 },
        },
        // HIT_LOCATION_LEFT_LEG
        {
            { 3, 0, -1, 0, 0, 5300, 5000 },
            { 3, 0, STAT_AGILITY, 0, DAM_KNOCKED_DOWN, 5300, 5312 },
            { 4, DAM_KNOCKED_DOWN, STAT_ENDURANCE, -3, DAM_CRIP_LEG_LEFT, 5312, 5313 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_LEFT, -1, 0, 0, 5313, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_LEFT | DAM_BYPASS, -1, 0, 0, 5314, 5000 },
            { 4, DAM_KNOCKED_OUT | DAM_CRIP_LEG_LEFT | DAM_BYPASS, -1, 0, 0, 5315, 5000 },
        },
        // HIT_LOCATION_EYES
        {
            { 4, 0, -1, 0, 0, 5300, 5000 },
            { 4, DAM_BYPASS, STAT_LUCK, 5, DAM_BLIND, 5316, 5317 },
            { 6, DAM_BYPASS, STAT_LUCK, 3, DAM_BLIND, 5316, 5317 },
            { 6, DAM_BYPASS | DAM_LOSE_TURN, STAT_LUCK, 0, DAM_BLIND, 5318, 5319 },
            { 8, DAM_KNOCKED_OUT | DAM_BLIND | DAM_BYPASS, -1, 0, 0, 5320, 5000 },
            { 8, DAM_DEAD, -1, 0, 0, 5321, 5000 },
        },
        // HIT_LOCATION_GROIN
        {
            { 3, 0, -1, 0, 0, 5300, 5000 },
            { 3, 0, STAT_LUCK, 0, DAM_BYPASS, 5300, 5017 },
            { 3, DAM_BYPASS, STAT_ENDURANCE, -2, DAM_KNOCKED_DOWN, 5301, 5302 },
            { 3, DAM_KNOCKED_DOWN, -1, 0, 0, 5312, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, STAT_ENDURANCE, 0, DAM_KNOCKED_OUT, 5302, 5303 },
            { 4, DAM_KNOCKED_OUT | DAM_BYPASS, -1, 0, 0, 5303, 5000 },
        },
        // HIT_LOCATION_UNCALLED
        {
            { 3, 0, -1, 0, 0, 5300, 5000 },
            { 3, DAM_BYPASS, -1, 0, 0, 5301, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 5302, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 5302, 5000 },
            { 4, DAM_KNOCKED_OUT | DAM_BYPASS, -1, 0, 0, 5310, 5000 },
            { 6, DAM_DEAD, -1, 0, 0, 5311, 5000 },
        },
    },
    // KILL_TYPE_GHOUL
    {
        // HIT_LOCATION_HEAD
        {
            { 4, 0, -1, 0, 0, 5001, 5000 },
            { 4, DAM_BYPASS, STAT_ENDURANCE, 0, DAM_KNOCKED_OUT, 5400, 5003 },
            { 5, DAM_BYPASS, STAT_ENDURANCE, -1, DAM_KNOCKED_OUT, 5400, 5003 },
            { 5, DAM_KNOCKED_DOWN | DAM_BYPASS, STAT_ENDURANCE, -2, DAM_KNOCKED_OUT, 5004, 5005 },
            { 6, DAM_KNOCKED_OUT | DAM_BYPASS, STAT_STRENGTH, 0, 0, 5005, 5000 },
            { 6, DAM_DEAD, -1, 0, 0, 5401, 5000 },
        },
        // HIT_LOCATION_LEFT_ARM
        {
            { 3, 0, -1, 0, 0, 5016, 5000 },
            { 3, 0, STAT_AGILITY, 0, DAM_DROP | DAM_LOSE_TURN, 5001, 5402 },
            { 4, DAM_DROP | DAM_LOSE_TURN, STAT_ENDURANCE, 0, DAM_CRIP_ARM_LEFT, 5402, 5012 },
            { 4, DAM_BYPASS | DAM_DROP | DAM_LOSE_TURN, STAT_ENDURANCE, 0, DAM_CRIP_ARM_LEFT, 5403, 5404 },
            { 4, DAM_CRIP_ARM_LEFT | DAM_BYPASS | DAM_DROP, -1, 0, 0, 5404, 5000 },
            { 4, DAM_CRIP_ARM_LEFT | DAM_BYPASS | DAM_DROP, -1, 0, 0, 5404, 5000 },
        },
        // HIT_LOCATION_RIGHT_ARM
        {
            { 3, 0, -1, 0, 0, 5016, 5000 },
            { 3, 0, STAT_AGILITY, 0, DAM_DROP | DAM_LOSE_TURN, 5001, 5402 },
            { 4, DAM_DROP | DAM_LOSE_TURN, STAT_ENDURANCE, 0, DAM_CRIP_ARM_RIGHT, 5402, 5015 },
            { 4, DAM_BYPASS | DAM_DROP | DAM_LOSE_TURN, STAT_ENDURANCE, 0, DAM_CRIP_ARM_RIGHT, 5403, 5404 },
            { 4, DAM_CRIP_ARM_RIGHT | DAM_BYPASS | DAM_DROP, -1, 0, 0, 5404, 5000 },
            { 4, DAM_CRIP_ARM_RIGHT | DAM_BYPASS | DAM_DROP, -1, 0, 0, 5404, 5000 },
        },
        // HIT_LOCATION_TORSO
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, DAM_BYPASS, -1, 0, 0, 5017, 5000 },
            { 3, 0, -1, 0, 0, 5018, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 5004, 5000 },
            { 4, DAM_KNOCKED_OUT | DAM_BYPASS, -1, 0, 0, 5003, 5000 },
            { 6, DAM_DEAD, -1, 0, 0, 5007, 5000 },
        },
        // HIT_LOCATION_RIGHT_LEG
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, 0, STAT_AGILITY, 0, DAM_KNOCKED_DOWN, 5001, 5023 },
            { 4, DAM_KNOCKED_DOWN, STAT_ENDURANCE, 0, DAM_CRIP_LEG_RIGHT, 5023, 5024 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_RIGHT, -1, 0, 0, 5024, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_RIGHT | DAM_BYPASS, STAT_ENDURANCE, 0, DAM_KNOCKED_OUT, 5024, 5026 },
            { 4, DAM_KNOCKED_OUT | DAM_CRIP_LEG_RIGHT | DAM_BYPASS, -1, 0, 0, 5026, 5000 },
        },
        // HIT_LOCATION_LEFT_LEG
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, 0, STAT_AGILITY, 0, DAM_KNOCKED_DOWN, 5001, 5023 },
            { 4, DAM_KNOCKED_DOWN, STAT_ENDURANCE, 0, DAM_CRIP_LEG_LEFT, 5023, 5024 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_LEFT, -1, 0, 0, 5024, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_LEFT | DAM_BYPASS, STAT_ENDURANCE, 0, DAM_KNOCKED_OUT, 5024, 5026 },
            { 4, DAM_KNOCKED_OUT | DAM_CRIP_LEG_LEFT | DAM_BYPASS, -1, 0, 0, 5026, 5000 },
        },
        // HIT_LOCATION_EYES
        {
            { 4, 0, STAT_LUCK, 3, DAM_BLIND, 5001, 5405 },
            { 4, DAM_BYPASS, STAT_LUCK, 0, DAM_BLIND, 5406, 5407 },
            { 6, DAM_BYPASS, STAT_LUCK, -3, DAM_BLIND, 5406, 5407 },
            { 6, DAM_BLIND | DAM_BYPASS | DAM_LOSE_TURN, -1, 0, 0, 5030, 5000 },
            { 8, DAM_KNOCKED_OUT | DAM_BLIND | DAM_BYPASS, -1, 0, 0, 5031, 5000 },
            { 8, DAM_DEAD, -1, 0, 0, 5408, 5000 },
        },
        // HIT_LOCATION_GROIN
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, 0, STAT_LUCK, 0, DAM_BYPASS, 5001, 5033 },
            { 3, DAM_BYPASS, STAT_ENDURANCE, 0, DAM_KNOCKED_DOWN, 5033, 5035 },
            { 3, DAM_KNOCKED_DOWN, -1, 0, 0, 5004, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, STAT_ENDURANCE, 0, DAM_KNOCKED_OUT, 5035, 5036 },
            { 4, DAM_KNOCKED_OUT | DAM_BYPASS, -1, 0, 0, 5036, 5000 },
        },
        // HIT_LOCATION_UNCALLED
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, DAM_BYPASS, -1, 0, 0, 5017, 5000 },
            { 3, 0, -1, 0, 0, 5018, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 5004, 5000 },
            { 4, DAM_KNOCKED_OUT | DAM_BYPASS, -1, 0, 0, 5003, 5000 },
            { 6, DAM_DEAD, -1, 0, 0, 5007, 5000 },
        },
    },
    // KILL_TYPE_BRAHMIN
    {
        // HIT_LOCATION_HEAD
        {
            { 4, 0, -1, 0, 0, 5016, 5000 },
            { 4, 0, -1, 0, 0, 5016, 5000 },
            { 5, 0, STAT_ENDURANCE, 2, DAM_KNOCKED_DOWN, 5016, 5500 },
            { 5, 0, STAT_ENDURANCE, -1, DAM_KNOCKED_DOWN, 5016, 5500 },
            { 6, DAM_KNOCKED_OUT, STAT_STRENGTH, 0, 0, 5501, 5000 },
            { 6, DAM_DEAD, -1, 0, 0, 5502, 5000 },
        },
        // HIT_LOCATION_LEFT_ARM
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 4, 0, STAT_ENDURANCE, 0, DAM_CRIP_LEG_LEFT, 5016, 5503 },
            { 4, 0, STAT_ENDURANCE, 0, DAM_CRIP_LEG_LEFT, 5016, 5503 },
            { 4, DAM_CRIP_LEG_LEFT, -1, 0, 0, 5503, 5000 },
            { 4, DAM_CRIP_LEG_LEFT, -1, 0, 0, 5503, 5000 },
        },
        // HIT_LOCATION_RIGHT_ARM
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 4, 0, STAT_ENDURANCE, 0, DAM_CRIP_LEG_RIGHT, 5016, 5503 },
            { 4, 0, STAT_ENDURANCE, 0, DAM_CRIP_LEG_RIGHT, 5016, 5503 },
            { 4, DAM_CRIP_LEG_RIGHT, -1, 0, 0, 5503, 5000 },
            { 4, DAM_CRIP_LEG_RIGHT, -1, 0, 0, 5503, 5000 },
        },
        // HIT_LOCATION_TORSO
        {
            { 3, 0, -1, 0, 0, 5504, 5000 },
            { 3, 0, -1, 0, 0, 5504, 5000 },
            { 4, 0, -1, 0, 0, 5504, 5000 },
            { 4, DAM_BYPASS, -1, 0, 0, 5505, 5000 },
            { 4, DAM_BYPASS, -1, 0, 0, 5505, 5000 },
            { 6, DAM_DEAD, -1, 0, 0, 5506, 5000 },
        },
        // HIT_LOCATION_RIGHT_LEG
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 4, 0, STAT_ENDURANCE, 0, DAM_CRIP_LEG_RIGHT, 5016, 5503 },
            { 4, 0, STAT_ENDURANCE, 0, DAM_CRIP_LEG_RIGHT, 5016, 5503 },
            { 4, DAM_CRIP_LEG_RIGHT, -1, 0, 0, 5503, 5000 },
            { 4, DAM_CRIP_LEG_RIGHT, -1, 0, 0, 5503, 5000 },
        },
        // HIT_LOCATION_LEFT_LEG
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 4, 0, STAT_ENDURANCE, 0, DAM_CRIP_LEG_LEFT, 5016, 5503 },
            { 4, 0, STAT_ENDURANCE, 0, DAM_CRIP_LEG_LEFT, 5016, 5503 },
            { 4, DAM_CRIP_LEG_LEFT, -1, 0, 0, 5503, 5000 },
            { 4, DAM_CRIP_LEG_LEFT, -1, 0, 0, 5503, 5000 },
        },
        // HIT_LOCATION_EYES
        {
            { 4, 0, -1, 0, 0, 5001, 5000 },
            { 4, DAM_BYPASS, STAT_LUCK, 0, DAM_BLIND, 5029, 5507 },
            { 6, DAM_BYPASS, STAT_LUCK, -3, DAM_BLIND, 5029, 5507 },
            { 6, DAM_BLIND | DAM_BYPASS | DAM_LOSE_TURN, -1, 0, 0, 5508, 5000 },
            { 8, DAM_KNOCKED_OUT | DAM_BLIND | DAM_BYPASS, -1, 0, 0, 5509, 5000 },
            { 8, DAM_DEAD, -1, 0, 0, 5510, 5000 },
        },
        // HIT_LOCATION_GROIN
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, DAM_BYPASS, -1, 0, 0, 5511, 5000 },
            { 3, DAM_BYPASS, -1, 0, 0, 5511, 5000 },
            { 4, DAM_BYPASS, -1, 0, 0, 5512, 5000 },
            { 4, DAM_BYPASS, -1, 0, 0, 5512, 5000 },
            { 6, DAM_BYPASS, -1, 0, 0, 5513, 5000 },
        },
        // HIT_LOCATION_UNCALLED
        {
            { 3, 0, -1, 0, 0, 5504, 5000 },
            { 3, 0, -1, 0, 0, 5504, 5000 },
            { 4, 0, -1, 0, 0, 5504, 5000 },
            { 4, DAM_BYPASS, -1, 0, 0, 5505, 5000 },
            { 4, DAM_BYPASS, -1, 0, 0, 5505, 5000 },
            { 6, DAM_DEAD, -1, 0, 0, 5506, 5000 },
        },
    },
    // KILL_TYPE_RADSCORPION
    {
        // HIT_LOCATION_HEAD
        {
            { 4, 0, -1, 0, 0, 5001, 5000 },
            { 4, 0, STAT_ENDURANCE, 3, DAM_KNOCKED_DOWN, 5001, 5600 },
            { 5, 0, STAT_ENDURANCE, 0, DAM_KNOCKED_DOWN, 5001, 5600 },
            { 5, 0, STAT_ENDURANCE, -3, DAM_KNOCKED_DOWN, 5001, 5600 },
            { 6, DAM_KNOCKED_DOWN, -1, 0, 0, 5600, 5000 },
            { 6, DAM_DEAD, -1, 0, 0, 5601, 5000 },
        },
        // HIT_LOCATION_LEFT_ARM
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 4, 0, -1, 0, 0, 5016, 5000 },
            { 4, 0, STAT_ENDURANCE, 0, DAM_CRIP_ARM_LEFT, 5016, 5602 },
            { 4, DAM_CRIP_ARM_LEFT, -1, 0, 0, 5602, 5000 },
            { 4, DAM_CRIP_ARM_LEFT, -1, 0, 0, 5602, 5000 },
        },
        // HIT_LOCATION_RIGHT_ARM
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 4, 0, -1, 0, 0, 5016, 5000 },
            { 4, 0, STAT_ENDURANCE, 2, DAM_CRIP_ARM_RIGHT, 5016, 5603 },
            { 4, 0, STAT_ENDURANCE, 0, DAM_CRIP_ARM_RIGHT, 5016, 5603 },
            { 4, DAM_CRIP_ARM_RIGHT, -1, 0, 0, 5603, 5000 },
        },
        // HIT_LOCATION_TORSO
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, DAM_BYPASS, -1, 0, 0, 5604, 5000 },
            { 4, 0, -1, 0, 0, 5016, 5000 },
            { 4, DAM_BYPASS, -1, 0, 0, 5605, 5000 },
            { 4, DAM_BYPASS, STAT_AGILITY, 0, DAM_KNOCKED_DOWN, 5605, 5606 },
            { 4, DAM_DEAD, -1, 0, 0, 5607, 5000 },
        },
        // HIT_LOCATION_RIGHT_LEG
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, 0, STAT_AGILITY, 2, 0, 5001, 5600 },
            { 4, DAM_KNOCKED_DOWN, STAT_ENDURANCE, 0, DAM_CRIP_LEG_RIGHT, 5600, 5608 },
            { 4, DAM_CRIP_LEG_RIGHT | DAM_BYPASS, -1, 0, 0, 5609, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_RIGHT | DAM_BYPASS, -1, 0, 0, 5608, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_RIGHT | DAM_BYPASS, -1, 0, 0, 5608, 5000 },
        },
        // HIT_LOCATION_LEFT_LEG
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, 0, STAT_AGILITY, 2, 0, 5001, 5600 },
            { 4, DAM_KNOCKED_DOWN, STAT_ENDURANCE, 0, DAM_CRIP_LEG_LEFT, 5600, 5008 },
            { 4, DAM_CRIP_LEG_LEFT | DAM_BYPASS, -1, 0, 0, 5609, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_LEFT | DAM_BYPASS, -1, 0, 0, 5608, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_LEFT | DAM_BYPASS, -1, 0, 0, 5608, 5000 },
        },
        // HIT_LOCATION_EYES
        {
            { 4, 0, -1, 0, 0, 5001, 5000 },
            { 4, 0, STAT_AGILITY, 3, DAM_BLIND, 5001, 5610 },
            { 6, 0, STAT_AGILITY, 0, DAM_BLIND, 5016, 5610 },
            { 6, 0, STAT_AGILITY, -3, DAM_BLIND, 5016, 5610 },
            { 8, 0, STAT_AGILITY, -3, DAM_BLIND, 5611, 5612 },
            { 8, DAM_DEAD, -1, 0, 0, 5613, 5000 },
        },
        // HIT_LOCATION_GROIN
        {
            { 3, 0, -1, 0, 0, 5614, 5000 },
            { 3, 0, -1, 0, 0, 5614, 5000 },
            { 4, 0, -1, 0, 0, 5614, 5000 },
            { 4, DAM_KNOCKED_OUT, -1, 0, 0, 5615, 5000 },
            { 4, DAM_KNOCKED_OUT, -1, 0, 0, 5615, 5000 },
            { 4, DAM_DEAD, -1, 0, 0, 5616, 5000 },
        },
        // HIT_LOCATION_UNCALLED
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, DAM_BYPASS, -1, 0, 0, 5604, 5000 },
            { 4, 0, -1, 0, 0, 5016, 5000 },
            { 4, DAM_BYPASS, -1, 0, 0, 5605, 5000 },
            { 4, DAM_BYPASS, STAT_AGILITY, 0, DAM_KNOCKED_DOWN, 5605, 5606 },
            { 4, DAM_DEAD, -1, 0, 0, 5607, 5000 },
        },
    },
    // KILL_TYPE_RAT
    {
        // HIT_LOCATION_HEAD
        {
            { 4, DAM_BYPASS, -1, 0, 0, 5700, 5000 },
            { 4, DAM_BYPASS, -1, 0, 0, 5700, 5000 },
            { 4, DAM_DEAD, -1, 0, 0, 5701, 5000 },
            { 4, DAM_DEAD, -1, 0, 0, 5701, 5000 },
            { 6, DAM_DEAD, -1, 0, 0, 5701, 5000 },
            { 6, DAM_DEAD, -1, 0, 0, 5701, 5000 },
        },
        // HIT_LOCATION_LEFT_ARM
        {
            { 3, DAM_CRIP_ARM_LEFT, -1, 0, 0, 5703, 5000 },
            { 3, DAM_CRIP_ARM_LEFT, -1, 0, 0, 5703, 5000 },
            { 3, DAM_CRIP_ARM_LEFT, -1, 0, 0, 5703, 5000 },
            { 4, DAM_CRIP_ARM_LEFT, -1, 0, 0, 5703, 5000 },
            { 4, DAM_CRIP_ARM_LEFT, -1, 0, 0, 5703, 5000 },
            { 4, DAM_CRIP_ARM_LEFT, -1, 0, 0, 5703, 5000 },
        },
        // HIT_LOCATION_RIGHT_ARM
        {
            { 3, DAM_CRIP_ARM_RIGHT, -1, 0, 0, 5705, 5000 },
            { 3, DAM_CRIP_ARM_RIGHT, -1, 0, 0, 5705, 5000 },
            { 3, DAM_CRIP_ARM_RIGHT, -1, 0, 0, 5705, 5000 },
            { 4, DAM_CRIP_ARM_RIGHT, -1, 0, 0, 5705, 5000 },
            { 4, DAM_CRIP_ARM_RIGHT, -1, 0, 0, 5705, 5000 },
            { 4, DAM_CRIP_ARM_RIGHT, -1, 0, 0, 5705, 5000 },
        },
        // HIT_LOCATION_TORSO
        {
            { 3, 0, -1, 0, 0, 5706, 5000 },
            { 3, DAM_KNOCKED_DOWN, -1, 0, 0, 5707, 5000 },
            { 3, DAM_KNOCKED_DOWN, -1, 0, 0, 5707, 5000 },
            { 4, DAM_KNOCKED_DOWN, -1, 0, 0, 5707, 5000 },
            { 4, DAM_KNOCKED_DOWN, -1, 0, 0, 5707, 5000 },
            { 4, DAM_DEAD, -1, 0, 0, 5708, 5000 },
        },
        // HIT_LOCATION_RIGHT_LEG
        {
            { 3, DAM_CRIP_LEG_RIGHT, -1, 0, 0, 5709, 5000 },
            { 3, DAM_CRIP_LEG_RIGHT, -1, 0, 0, 5709, 5000 },
            { 3, DAM_CRIP_LEG_RIGHT, -1, 0, 0, 5709, 5000 },
            { 4, DAM_CRIP_LEG_RIGHT, -1, 0, 0, 5709, 5000 },
            { 4, DAM_CRIP_LEG_RIGHT, -1, 0, 0, 5709, 5000 },
            { 4, DAM_CRIP_LEG_RIGHT, -1, 0, 0, 5709, 5000 },
        },
        // HIT_LOCATION_LEFT_LEG
        {
            { 3, DAM_CRIP_LEG_LEFT, -1, 0, 0, 5710, 5000 },
            { 3, DAM_CRIP_LEG_LEFT, -1, 0, 0, 5710, 5000 },
            { 3, DAM_CRIP_LEG_LEFT, -1, 0, 0, 5710, 5000 },
            { 4, DAM_CRIP_LEG_LEFT, -1, 0, 0, 5710, 5000 },
            { 4, DAM_CRIP_LEG_LEFT, -1, 0, 0, 5710, 5000 },
            { 4, DAM_CRIP_LEG_LEFT, -1, 0, 0, 5710, 5000 },
        },
        // HIT_LOCATION_EYES
        {
            { 4, DAM_BYPASS, -1, 0, 0, 5711, 5000 },
            { 4, DAM_DEAD, -1, 0, 0, 5712, 5000 },
            { 4, DAM_DEAD, -1, 0, 0, 5712, 5000 },
            { 6, DAM_DEAD, -1, 0, 0, 5712, 5000 },
            { 6, DAM_DEAD, -1, 0, 0, 5712, 5000 },
            { 6, DAM_DEAD, -1, 0, 0, 5712, 5000 },
        },
        // HIT_LOCATION_GROIN
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 4, DAM_BYPASS, -1, 0, 0, 5711, 5000 },
            { 4, DAM_BYPASS, -1, 0, 0, 5711, 5000 },
            { 6, DAM_DEAD, -1, 0, 0, 5712, 5000 },
            { 6, DAM_DEAD, -1, 0, 0, 5712, 5000 },
        },
        // HIT_LOCATION_UNCALLED
        {
            { 3, 0, -1, 0, 0, 5706, 5000 },
            { 3, DAM_KNOCKED_DOWN, -1, 0, 0, 5707, 5000 },
            { 3, DAM_KNOCKED_DOWN, -1, 0, 0, 5707, 5000 },
            { 4, DAM_KNOCKED_DOWN, -1, 0, 0, 5707, 5000 },
            { 4, DAM_KNOCKED_DOWN, -1, 0, 0, 5707, 5000 },
            { 4, DAM_DEAD, -1, 0, 0, 5708, 5000 },
        },
    },
    // KILL_TYPE_FLOATER
    {
        // HIT_LOCATION_HEAD
        {
            { 4, 0, -1, 0, 0, 5001, 5000 },
            { 4, 0, STAT_AGILITY, 0, DAM_KNOCKED_DOWN, 5001, 5800 },
            { 5, 0, STAT_AGILITY, -3, DAM_KNOCKED_DOWN, 5016, 5800 },
            { 5, DAM_KNOCKED_DOWN, STAT_ENDURANCE, 0, DAM_KNOCKED_OUT, 5800, 5801 },
            { 6, DAM_KNOCKED_DOWN, STAT_ENDURANCE, -3, DAM_KNOCKED_OUT, 5800, 5801 },
            { 6, DAM_DEAD, -1, 0, 0, 5802, 5000 },
        },
        // HIT_LOCATION_LEFT_ARM
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, 0, STAT_ENDURANCE, 0, DAM_LOSE_TURN, 5001, 5803 },
            { 4, 0, STAT_ENDURANCE, -2, DAM_LOSE_TURN, 5001, 5803 },
            { 3, DAM_BYPASS | DAM_LOSE_TURN, -1, 0, 0, 5804, 5000 },
            { 4, DAM_BYPASS | DAM_LOSE_TURN, -1, 0, 0, 5804, 5000 },
            { 4, DAM_DEAD, -1, 0, 0, 5805, 5000 },
        },
        // HIT_LOCATION_RIGHT_ARM
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, 0, STAT_ENDURANCE, 0, DAM_LOSE_TURN, 5001, 5803 },
            { 4, 0, STAT_ENDURANCE, -2, DAM_LOSE_TURN, 5001, 5803 },
            { 3, DAM_BYPASS | DAM_LOSE_TURN, -1, 0, 0, 5804, 5000 },
            { 4, DAM_BYPASS | DAM_LOSE_TURN, -1, 0, 0, 5804, 5000 },
            { 4, DAM_DEAD, -1, 0, 0, 5805, 5000 },
        },
        // HIT_LOCATION_TORSO
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, 0, STAT_AGILITY, 0, DAM_KNOCKED_DOWN, 5001, 5800 },
            { 3, 0, STAT_AGILITY, -2, DAM_KNOCKED_DOWN, 5001, 5800 },
            { 4, DAM_KNOCKED_DOWN, -1, 0, 0, 5800, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 5804, 5000 },
            { 4, DAM_DEAD, -1, 0, 0, 5805, 5000 },
        },
        // HIT_LOCATION_RIGHT_LEG
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, 0, STAT_AGILITY, 1, DAM_KNOCKED_DOWN, 5001, 5800 },
            { 4, 0, STAT_AGILITY, -1, DAM_KNOCKED_DOWN, 5001, 5800 },
            { 4, DAM_KNOCKED_DOWN, STAT_ENDURANCE, -1, DAM_CRIP_LEG_LEFT | DAM_CRIP_LEG_RIGHT, 5800, 5806 },
            { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, STAT_ENDURANCE, -3, DAM_CRIP_LEG_LEFT | DAM_CRIP_LEG_RIGHT, 5804, 5806 },
            { 6, DAM_DEAD | DAM_ON_FIRE, -1, 0, 0, 5807, 5000 },
        },
        // HIT_LOCATION_LEFT_LEG
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 4, DAM_LOSE_TURN, -1, 0, 0, 5803, 5000 },
            { 4, DAM_LOSE_TURN, -1, 0, 0, 5803, 5000 },
            { 4, DAM_CRIP_ARM_LEFT | DAM_CRIP_ARM_RIGHT | DAM_LOSE_TURN, -1, 0, 0, 5808, 5000 },
            { 4, DAM_CRIP_ARM_LEFT | DAM_CRIP_ARM_RIGHT | DAM_LOSE_TURN, -1, 0, 0, 5808, 5000 },
        },
        // HIT_LOCATION_EYES
        {
            { 4, 0, -1, 0, 0, 5001, 5000 },
            { 4, DAM_BYPASS, -1, 0, 0, 5809, 5000 },
            { 5, 0, STAT_ENDURANCE, 0, DAM_BLIND, 5016, 5810 },
            { 5, DAM_BYPASS, STAT_ENDURANCE, -3, DAM_BLIND, 5809, 5810 },
            { 6, DAM_BLIND | DAM_BYPASS, -1, 0, 0, 5810, 5000 },
            { 6, DAM_KNOCKED_DOWN | DAM_BLIND | DAM_BYPASS, -1, 0, 0, 5801, 5000 },
        },
        // HIT_LOCATION_GROIN
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, 0, STAT_ENDURANCE, 0, DAM_KNOCKED_DOWN, 5001, 5800 },
            { 3, 0, STAT_ENDURANCE, -3, DAM_KNOCKED_DOWN, 5001, 5800 },
            { 3, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 5800, 5000 },
            { 3, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 5800, 5000 },
        },
        // HIT_LOCATION_UNCALLED
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, 0, STAT_AGILITY, 0, DAM_KNOCKED_DOWN, 5001, 5800 },
            { 3, 0, STAT_AGILITY, -2, DAM_KNOCKED_DOWN, 5001, 5800 },
            { 4, DAM_KNOCKED_DOWN, -1, 0, 0, 5800, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 5804, 5000 },
            { 4, DAM_DEAD, -1, 0, 0, 5805, 5000 },
        },
    },
    // KILL_TYPE_CENTAUR
    {
        // HIT_LOCATION_HEAD
        {
            { 4, 0, -1, 0, 0, 5016, 5000 },
            { 4, 0, STAT_ENDURANCE, 0, DAM_KNOCKED_DOWN | DAM_LOSE_TURN, 5016, 5900 },
            { 5, 0, STAT_ENDURANCE, -3, DAM_KNOCKED_DOWN | DAM_LOSE_TURN, 5016, 5900 },
            { 5, DAM_BYPASS, STAT_ENDURANCE, -3, DAM_KNOCKED_DOWN | DAM_LOSE_TURN, 5901, 5900 },
            { 6, DAM_BYPASS, STAT_ENDURANCE, -3, DAM_KNOCKED_DOWN | DAM_LOSE_TURN, 5901, 5900 },
            { 6, DAM_DEAD, -1, 0, 0, 5902, 5000 },
        },
        // HIT_LOCATION_LEFT_ARM
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 4, 0, STAT_ENDURANCE, 0, DAM_LOSE_TURN, 5016, 5903 },
            { 4, 0, STAT_ENDURANCE, 0, DAM_CRIP_ARM_LEFT, 5016, 5904 },
            { 4, DAM_CRIP_ARM_LEFT, -1, 0, 0, 5904, 5000 },
            { 4, DAM_CRIP_ARM_LEFT | DAM_BYPASS, -1, 0, 0, 5905, 5000 },
        },
        // HIT_LOCATION_RIGHT_ARM
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 4, 0, STAT_ENDURANCE, 0, DAM_LOSE_TURN, 5016, 5903 },
            { 4, 0, STAT_ENDURANCE, 0, DAM_CRIP_ARM_RIGHT, 5016, 5904 },
            { 4, DAM_CRIP_ARM_RIGHT, -1, 0, 0, 5904, 5000 },
            { 4, DAM_CRIP_ARM_RIGHT | DAM_BYPASS, -1, 0, 0, 5905, 5000 },
        },
        // HIT_LOCATION_TORSO
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 4, 0, -1, 0, 0, 5001, 5000 },
            { 4, DAM_BYPASS, -1, 0, 0, 5901, 5000 },
            { 4, DAM_BYPASS, STAT_ENDURANCE, 2, 0, 5901, 5900 },
            { 5, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 5900, 5000 },
            { 5, DAM_DEAD, -1, 0, 0, 5902, 5000 },
        },
        // HIT_LOCATION_RIGHT_LEG
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, DAM_KNOCKED_DOWN, -1, 0, 0, 5900, 5000 },
            { 4, DAM_KNOCKED_DOWN, STAT_ENDURANCE, 0, DAM_CRIP_LEG_RIGHT, 5900, 5906 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_RIGHT, -1, 0, 0, 5906, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_RIGHT, -1, 0, 0, 5906, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_RIGHT | DAM_LOSE_TURN, -1, 0, 0, 5907, 5000 },
        },
        // HIT_LOCATION_LEFT_LEG
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, DAM_KNOCKED_DOWN, -1, 0, 0, 5900, 5000 },
            { 4, DAM_KNOCKED_DOWN, STAT_ENDURANCE, 0, DAM_CRIP_LEG_LEFT, 5900, 5906 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_LEFT, -1, 0, 0, 5906, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_LEFT, -1, 0, 0, 5906, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_LEFT | DAM_LOSE_TURN, -1, 0, 0, 5907, 5000 },
        },
        // HIT_LOCATION_EYES
        {
            { 4, 0, -1, 0, 0, 5001, 5000 },
            { 4, 0, STAT_ENDURANCE, 1, DAM_BLIND, 5001, 5908 },
            { 6, DAM_BYPASS, STAT_ENDURANCE, -1, DAM_BLIND, 5901, 5908 },
            { 6, DAM_BLIND | DAM_BYPASS | DAM_LOSE_TURN, -1, 0, 0, 5909, 5000 },
            { 8, DAM_KNOCKED_OUT | DAM_BLIND | DAM_BYPASS, -1, 0, 0, 5910, 5000 },
            { 8, DAM_DEAD, -1, 0, 0, 5911, 5000 },
        },
        // HIT_LOCATION_GROIN
        {
            { 2, 0, -1, 0, 0, 5912, 5000 },
            { 2, 0, -1, 0, 0, 5912, 5000 },
            { 2, 0, -1, 0, 0, 5912, 5000 },
            { 2, 0, -1, 0, 0, 5912, 5000 },
            { 2, 0, -1, 0, 0, 5912, 5000 },
            { 2, 0, -1, 0, 0, 5912, 5000 },
        },
        // HIT_LOCATION_UNCALLED
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 4, 0, -1, 0, 0, 5001, 5000 },
            { 4, DAM_BYPASS, -1, 0, 0, 5901, 5000 },
            { 4, DAM_BYPASS, STAT_ENDURANCE, 2, 0, 5901, 5900 },
            { 5, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 5900, 5000 },
            { 5, DAM_DEAD, -1, 0, 0, 5902, 5000 },
        },
    },
    // KILL_TYPE_ROBOT
    {
        // HIT_LOCATION_HEAD
        {
            { 4, 0, -1, 0, 0, 6000, 5000 },
            { 4, 0, -1, 0, 0, 6000, 5000 },
            { 5, 0, -1, 0, 0, 6000, 5000 },
            { 5, DAM_KNOCKED_DOWN, -1, 0, 0, 6001, 5000 },
            { 6, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 6002, 5000 },
            { 6, DAM_DEAD, -1, 0, 0, 6003, 5000 },
        },
        // HIT_LOCATION_LEFT_ARM
        {
            { 3, 0, -1, 0, 0, 6000, 5000 },
            { 3, 0, -1, 0, 0, 6000, 5000 },
            { 3, 0, STAT_ENDURANCE, 0, DAM_CRIP_ARM_LEFT, 6000, 6004 },
            { 3, 0, STAT_ENDURANCE, -3, DAM_CRIP_ARM_LEFT, 6000, 6004 },
            { 4, DAM_CRIP_ARM_LEFT, -1, 0, 0, 6004, 5000 },
            { 4, DAM_CRIP_ARM_LEFT, STAT_ENDURANCE, 0, DAM_CRIP_ARM_RIGHT, 6004, 6005 },
        },
        // HIT_LOCATION_RIGHT_ARM
        {
            { 3, 0, -1, 0, 0, 6000, 5000 },
            { 3, 0, -1, 0, 0, 6000, 5000 },
            { 3, 0, STAT_ENDURANCE, 0, DAM_CRIP_ARM_RIGHT, 6000, 6004 },
            { 3, 0, STAT_ENDURANCE, -3, DAM_CRIP_ARM_RIGHT, 6000, 6004 },
            { 4, DAM_CRIP_ARM_RIGHT, -1, 0, 0, 6004, 5000 },
            { 4, DAM_CRIP_ARM_RIGHT, STAT_ENDURANCE, 0, DAM_CRIP_ARM_LEFT, 6004, 6005 },
        },
        // HIT_LOCATION_TORSO
        {
            { 3, 0, -1, 0, 0, 6000, 5000 },
            { 3, DAM_BYPASS, -1, 0, 0, 6006, 5000 },
            { 4, 0, -1, 0, 0, 6007, 5000 },
            { 4, DAM_BYPASS, -1, 0, 0, 6008, 5000 },
            { 6, DAM_BYPASS, -1, 0, 0, 6009, 5000 },
            { 6, DAM_DEAD, -1, 0, 0, 6010, 5000 },
        },
        // HIT_LOCATION_RIGHT_LEG
        {
            { 3, 0, -1, 0, 0, 6000, 5000 },
            { 4, 0, -1, 0, 0, 6007, 5000 },
            { 3, 0, STAT_ENDURANCE, 0, DAM_CRIP_LEG_RIGHT, 6000, 6004 },
            { 4, 0, STAT_ENDURANCE, -4, DAM_CRIP_LEG_RIGHT, 6007, 6004 },
            { 4, DAM_CRIP_LEG_RIGHT, STAT_ENDURANCE, 0, DAM_KNOCKED_DOWN, 6004, 6011 },
            { 4, DAM_CRIP_LEG_RIGHT, STAT_ENDURANCE, -3, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_LEFT, 6004, 6012 },
        },
        // HIT_LOCATION_LEFT_LEG
        {
            { 3, 0, -1, 0, 0, 6000, 5000 },
            { 4, 0, -1, 0, 0, 6007, 5000 },
            { 3, 0, STAT_ENDURANCE, 0, DAM_CRIP_LEG_LEFT, 6000, 6004 },
            { 4, 0, STAT_ENDURANCE, -4, DAM_CRIP_LEG_LEFT, 6007, 6004 },
            { 4, DAM_CRIP_LEG_LEFT, STAT_ENDURANCE, 0, DAM_KNOCKED_DOWN, 6004, 6011 },
            { 4, DAM_CRIP_LEG_LEFT, STAT_ENDURANCE, -3, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_RIGHT, 6004, 6012 },
        },
        // HIT_LOCATION_EYES
        {
            { 3, 0, -1, 0, 0, 6000, 5000 },
            { 3, 0, STAT_ENDURANCE, 0, DAM_BLIND, 6000, 6013 },
            { 3, 0, STAT_ENDURANCE, -2, DAM_BLIND, 6000, 6013 },
            { 3, 0, STAT_ENDURANCE, -4, DAM_BLIND, 6000, 6013 },
            { 3, 0, STAT_ENDURANCE, -6, DAM_BLIND, 6000, 6013 },
            { 3, DAM_BLIND, -1, 0, 0, 6013, 5000 },
        },
        // HIT_LOCATION_GROIN
        {
            { 3, 0, -1, 0, 0, 6000, 5000 },
            { 3, 0, -1, 0, 0, 6000, 5000 },
            { 3, 0, STAT_ENDURANCE, -1, DAM_KNOCKED_DOWN | DAM_LOSE_TURN, 6000, 6002 },
            { 3, 0, STAT_ENDURANCE, -4, DAM_KNOCKED_DOWN | DAM_LOSE_TURN, 6000, 6002 },
            { 3, DAM_KNOCKED_DOWN | DAM_LOSE_TURN, STAT_ENDURANCE, 0, 0, 6002, 6003 },
            { 3, DAM_KNOCKED_DOWN | DAM_LOSE_TURN, STAT_ENDURANCE, -4, 0, 6002, 6003 },
        },
        // HIT_LOCATION_UNCALLED
        {
            { 3, 0, -1, 0, 0, 6000, 5000 },
            { 3, DAM_BYPASS, -1, 0, 0, 6006, 5000 },
            { 4, 0, -1, 0, 0, 6007, 5000 },
            { 4, DAM_BYPASS, -1, 0, 0, 6008, 5000 },
            { 6, DAM_BYPASS, -1, 0, 0, 6009, 5000 },
            { 6, DAM_DEAD, -1, 0, 0, 6010, 5000 },
        },
    },
    // KILL_TYPE_DOG
    {
        // HIT_LOCATION_HEAD
        {
            { 4, 0, -1, 0, 0, 5016, 5000 },
            { 4, 0, STAT_ENDURANCE, 0, DAM_KNOCKED_DOWN, 5016, 6100 },
            { 4, 0, STAT_ENDURANCE, -3, DAM_KNOCKED_DOWN, 5016, 6100 },
            { 4, 0, STAT_ENDURANCE, -6, DAM_CRIP_ARM_LEFT | DAM_CRIP_ARM_RIGHT, 5016, 6101 },
            { 4, DAM_KNOCKED_DOWN, STAT_ENDURANCE, -3, DAM_KNOCKED_OUT, 6100, 6102 },
            { 4, DAM_DEAD, -1, 0, 0, 6103, 5000 },
        },
        // HIT_LOCATION_LEFT_ARM
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, 0, STAT_ENDURANCE, -1, DAM_CRIP_LEG_LEFT, 5001, 6104 },
            { 3, 0, STAT_ENDURANCE, -3, DAM_CRIP_LEG_LEFT, 5001, 6104 },
            { 3, 0, STAT_ENDURANCE, -5, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_LEFT, 5001, 6105 },
            { 3, DAM_CRIP_LEG_LEFT, STAT_AGILITY, -1, DAM_KNOCKED_DOWN, 6104, 6105 },
            { 3, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_LEFT, -1, 0, 0, 6105, 5000 },
        },
        // HIT_LOCATION_RIGHT_ARM
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, 0, STAT_ENDURANCE, -1, DAM_CRIP_LEG_RIGHT, 5001, 6104 },
            { 3, 0, STAT_ENDURANCE, -3, DAM_CRIP_LEG_RIGHT, 5001, 6104 },
            { 3, 0, STAT_ENDURANCE, -5, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_RIGHT, 5001, 6105 },
            { 3, DAM_CRIP_LEG_RIGHT, STAT_AGILITY, -1, DAM_KNOCKED_DOWN, 6104, 6105 },
            { 3, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_RIGHT, -1, 0, 0, 6105, 5000 },
        },
        // HIT_LOCATION_TORSO
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, 0, STAT_AGILITY, -1, DAM_KNOCKED_DOWN, 5001, 6100 },
            { 4, 0, -1, 0, 0, 5016, 5000 },
            { 4, 0, STAT_AGILITY, -3, DAM_KNOCKED_DOWN, 5016, 6100 },
            { 4, DAM_KNOCKED_DOWN, -1, 0, 0, 6100, 5000 },
            { 6, DAM_DEAD, -1, 0, 0, 6103, 5000 },
        },
        // HIT_LOCATION_RIGHT_LEG
        {
            { 3, 0, STAT_ENDURANCE, 1, DAM_CRIP_LEG_RIGHT, 5001, 6104 },
            { 3, 0, STAT_ENDURANCE, 0, DAM_CRIP_LEG_RIGHT, 5001, 6104 },
            { 3, 0, STAT_ENDURANCE, -2, DAM_CRIP_LEG_RIGHT, 5001, 6104 },
            { 3, 0, STAT_ENDURANCE, -4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_RIGHT, 5001, 6105 },
            { 3, DAM_CRIP_LEG_RIGHT, STAT_AGILITY, -1, DAM_KNOCKED_DOWN, 6104, 6105 },
            { 3, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_RIGHT, -1, 0, 0, 6105, 5000 },
        },
        // HIT_LOCATION_LEFT_LEG
        {
            { 3, 0, STAT_ENDURANCE, 1, DAM_CRIP_LEG_LEFT, 5001, 6104 },
            { 3, 0, STAT_ENDURANCE, 0, DAM_CRIP_LEG_LEFT, 5001, 6104 },
            { 3, 0, STAT_ENDURANCE, -2, DAM_CRIP_LEG_LEFT, 5001, 6104 },
            { 3, 0, STAT_ENDURANCE, -4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_LEFT, 5001, 6105 },
            { 3, DAM_CRIP_LEG_LEFT, STAT_AGILITY, -1, DAM_KNOCKED_DOWN, 6104, 6105 },
            { 3, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_LEFT, -1, 0, 0, 6105, 5000 },
        },
        // HIT_LOCATION_EYES
        {
            { 4, 0, -1, 0, 0, 5001, 5000 },
            { 4, DAM_BYPASS, -1, 0, 0, 5018, 5000 },
            { 6, DAM_BYPASS, -1, 0, 0, 5018, 5000 },
            { 6, DAM_BYPASS, STAT_ENDURANCE, 3, DAM_BLIND, 5018, 6106 },
            { 8, DAM_BYPASS, STAT_ENDURANCE, 0, DAM_BLIND, 5018, 6106 },
            { 8, DAM_DEAD, -1, 0, 0, 6107, 5000 },
        },
        // HIT_LOCATION_GROIN
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, 0, STAT_AGILITY, -2, DAM_KNOCKED_DOWN, 5001, 6100 },
            { 4, 0, -1, 0, 0, 5016, 5000 },
            { 4, 0, STAT_AGILITY, -5, DAM_KNOCKED_DOWN, 5016, 6100 },
            { 4, DAM_KNOCKED_DOWN, -1, 0, 0, 6100, 5000 },
            { 6, DAM_DEAD, -1, 0, 0, 6103, 5000 },
        },
        // HIT_LOCATION_UNCALLED
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, 0, STAT_AGILITY, -1, DAM_KNOCKED_DOWN, 5001, 6100 },
            { 4, 0, -1, 0, 0, 5016, 5000 },
            { 4, 0, STAT_AGILITY, -3, DAM_KNOCKED_DOWN, 5016, 6100 },
            { 4, DAM_KNOCKED_DOWN, -1, 0, 0, 6100, 5000 },
            { 6, DAM_DEAD, -1, 0, 0, 6103, 5000 },
        },
    },
    // KILL_TYPE_MANTIS
    {
        // HIT_LOCATION_HEAD
        {
            { 4, 0, -1, 0, 0, 5001, 5000 },
            { 4, 0, STAT_ENDURANCE, 0, DAM_KNOCKED_DOWN, 5001, 6200 },
            { 5, 0, STAT_ENDURANCE, -3, DAM_KNOCKED_DOWN, 5016, 6200 },
            { 5, DAM_KNOCKED_DOWN, STAT_ENDURANCE, -1, DAM_KNOCKED_OUT, 6200, 6201 },
            { 6, DAM_KNOCKED_DOWN, STAT_ENDURANCE, -3, DAM_KNOCKED_OUT, 6200, 6201 },
            { 6, DAM_DEAD, -1, 0, 0, 6202, 5000 },
        },
        // HIT_LOCATION_LEFT_ARM
        {
            { 3, 0, STAT_ENDURANCE, 0, DAM_CRIP_ARM_LEFT, 5001, 6203 },
            { 3, 0, STAT_ENDURANCE, 0, DAM_CRIP_ARM_LEFT, 5001, 6203 },
            { 3, 0, STAT_ENDURANCE, -2, DAM_CRIP_ARM_LEFT, 5001, 6203 },
            { 4, 0, STAT_ENDURANCE, -4, DAM_CRIP_ARM_LEFT, 5016, 6203 },
            { 4, 0, STAT_ENDURANCE, -4, DAM_CRIP_ARM_LEFT, 5016, 6203 },
            { 4, DAM_CRIP_ARM_LEFT | DAM_LOSE_TURN, -1, 0, 0, 6204, 5000 },
        },
        // HIT_LOCATION_RIGHT_ARM
        {
            { 3, 0, STAT_ENDURANCE, 0, DAM_CRIP_ARM_RIGHT, 5001, 6203 },
            { 3, 0, STAT_ENDURANCE, 0, DAM_CRIP_ARM_RIGHT, 5001, 6203 },
            { 3, 0, STAT_ENDURANCE, -2, DAM_CRIP_ARM_RIGHT, 5001, 6203 },
            { 4, 0, STAT_ENDURANCE, -4, DAM_CRIP_ARM_RIGHT, 5016, 6203 },
            { 4, 0, STAT_ENDURANCE, -4, DAM_CRIP_ARM_RIGHT, 5016, 6203 },
            { 4, DAM_CRIP_ARM_RIGHT | DAM_LOSE_TURN, -1, 0, 0, 6204, 5000 },
        },
        // HIT_LOCATION_TORSO
        {
            { 3, 0, -1, 0, 0, 1000, 5000 },
            { 3, 0, STAT_ENDURANCE, 0, DAM_BYPASS, 5001, 6205 },
            { 3, 0, STAT_ENDURANCE, -2, DAM_BYPASS, 5001, 6205 },
            { 4, 0, STAT_ENDURANCE, -2, DAM_BYPASS, 5016, 6205 },
            { 4, 0, STAT_ENDURANCE, -4, DAM_BYPASS, 5016, 6205 },
            { 6, DAM_DEAD, -1, 0, 0, 6206, 5000 },
        },
        // HIT_LOCATION_RIGHT_LEG
        {
            { 3, 0, STAT_AGILITY, 0, DAM_KNOCKED_DOWN, 5001, 6201 },
            { 3, 0, STAT_AGILITY, -2, DAM_KNOCKED_DOWN, 5001, 6201 },
            { 4, 0, STAT_AGILITY, -4, DAM_KNOCKED_DOWN, 5001, 6201 },
            { 3, DAM_KNOCKED_DOWN, STAT_ENDURANCE, 0, DAM_CRIP_LEG_RIGHT, 6201, 6203 },
            { 4, DAM_KNOCKED_DOWN, STAT_ENDURANCE, -3, DAM_CRIP_LEG_RIGHT, 6201, 6203 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_RIGHT, -1, 0, 0, 6207, 5000 },
        },
        // HIT_LOCATION_LEFT_LEG
        {
            { 3, 0, STAT_AGILITY, 0, DAM_KNOCKED_DOWN, 5001, 6201 },
            { 3, 0, STAT_AGILITY, -3, DAM_KNOCKED_DOWN, 5001, 6201 },
            { 3, DAM_KNOCKED_DOWN, STAT_ENDURANCE, -2, DAM_CRIP_LEG_LEFT, 6201, 6208 },
            { 3, DAM_KNOCKED_DOWN, STAT_ENDURANCE, -2, DAM_CRIP_LEG_LEFT, 6201, 6208 },
            { 3, DAM_KNOCKED_DOWN, STAT_ENDURANCE, -5, DAM_CRIP_LEG_LEFT, 6201, 6208 },
            { 3, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_LEFT, -1, 0, 0, 6208, 5000 },
        },
        // HIT_LOCATION_EYES
        {
            { 4, 0, -1, 0, 0, 5001, 5000 },
            { 4, DAM_BYPASS, STAT_ENDURANCE, 0, DAM_LOSE_TURN, 6205, 6209 },
            { 6, DAM_BYPASS, STAT_ENDURANCE, -3, DAM_LOSE_TURN, 6205, 6209 },
            { 6, DAM_BYPASS | DAM_LOSE_TURN, STAT_ENDURANCE, -3, DAM_BLIND, 6209, 6210 },
            { 8, DAM_KNOCKED_DOWN | DAM_BYPASS | DAM_LOSE_TURN, STAT_ENDURANCE, -3, DAM_BLIND, 6209, 6210 },
            { 8, DAM_DEAD, -1, 0, 0, 6202, 5000 },
        },
        // HIT_LOCATION_GROIN
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, DAM_BYPASS, -1, 0, 0, 6205, 5000 },
            { 4, 0, -1, 0, 0, 5016, 5000 },
            { 4, 0, -1, 0, 0, 5016, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 6209, 5000 },
        },
        // HIT_LOCATION_UNCALLED
        {
            { 3, 0, -1, 0, 0, 1000, 5000 },
            { 3, 0, STAT_ENDURANCE, 0, DAM_BYPASS, 5001, 6205 },
            { 3, 0, STAT_ENDURANCE, -2, DAM_BYPASS, 5001, 6205 },
            { 4, 0, STAT_ENDURANCE, -2, DAM_BYPASS, 5016, 6205 },
            { 4, 0, STAT_ENDURANCE, -4, DAM_BYPASS, 5016, 6205 },
            { 6, DAM_DEAD, -1, 0, 0, 6206, 5000 },
        },
    },
    // KILL_TYPE_DEATH_CLAW
    {
        // HIT_LOCATION_HEAD
        {
            { 4, 0, -1, 0, 0, 5016, 5000 },
            { 4, 0, STAT_ENDURANCE, 0, DAM_KNOCKED_DOWN, 5016, 5023 },
            { 5, 0, STAT_ENDURANCE, -3, DAM_KNOCKED_DOWN, 5016, 5023 },
            { 5, 0, STAT_ENDURANCE, -5, DAM_KNOCKED_DOWN, 5016, 5023 },
            { 6, 0, STAT_ENDURANCE, -4, DAM_KNOCKED_DOWN | DAM_LOSE_TURN, 5016, 5004 },
            { 6, 0, STAT_ENDURANCE, -5, DAM_KNOCKED_DOWN | DAM_LOSE_TURN, 5016, 5004 },
        },
        // HIT_LOCATION_LEFT_ARM
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, 0, STAT_ENDURANCE, 0, DAM_CRIP_ARM_LEFT, 5001, 5011 },
            { 3, 0, STAT_ENDURANCE, -2, DAM_CRIP_ARM_LEFT, 5001, 5011 },
            { 3, 0, STAT_ENDURANCE, -4, DAM_CRIP_ARM_LEFT, 5001, 5011 },
            { 3, 0, STAT_ENDURANCE, -6, DAM_CRIP_ARM_LEFT, 5001, 5011 },
            { 3, 0, STAT_ENDURANCE, -8, DAM_CRIP_ARM_LEFT, 5001, 5011 },
        },
        // HIT_LOCATION_RIGHT_ARM
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, 0, STAT_ENDURANCE, 0, DAM_CRIP_ARM_RIGHT, 5001, 5014 },
            { 3, 0, STAT_ENDURANCE, -2, DAM_CRIP_ARM_RIGHT, 5001, 5014 },
            { 3, 0, STAT_ENDURANCE, -4, DAM_CRIP_ARM_RIGHT, 5001, 5014 },
            { 3, 0, STAT_ENDURANCE, -6, DAM_CRIP_ARM_RIGHT, 5001, 5014 },
            { 3, 0, STAT_ENDURANCE, -8, DAM_CRIP_ARM_RIGHT, 5001, 5014 },
        },
        // HIT_LOCATION_TORSO
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, 0, STAT_ENDURANCE, -1, DAM_BYPASS, 5001, 6300 },
            { 4, 0, -1, 0, 0, 5016, 5000 },
            { 4, 0, STAT_ENDURANCE, -1, DAM_BYPASS, 5016, 6300 },
            { 5, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 5004, 5000 },
            { 5, DAM_KNOCKED_DOWN | DAM_BYPASS | DAM_LOSE_TURN, -1, 0, 0, 5005, 5000 },
        },
        // HIT_LOCATION_RIGHT_LEG
        {
            { 3, 0, STAT_AGILITY, 0, DAM_KNOCKED_DOWN, 5001, 5004 },
            { 3, 0, STAT_ENDURANCE, 0, DAM_CRIP_LEG_RIGHT, 5001, 5004 },
            { 3, DAM_KNOCKED_DOWN, STAT_ENDURANCE, -2, DAM_CRIP_LEG_RIGHT, 5001, 5004 },
            { 4, DAM_KNOCKED_DOWN, STAT_ENDURANCE, -4, DAM_CRIP_LEG_RIGHT, 5016, 5022 },
            { 4, DAM_KNOCKED_DOWN, STAT_ENDURANCE, -5, DAM_CRIP_LEG_RIGHT, 5023, 5024 },
            { 4, DAM_KNOCKED_DOWN, STAT_ENDURANCE, -6, DAM_CRIP_LEG_RIGHT, 5023, 5024 },
        },
        // HIT_LOCATION_LEFT_LEG
        {
            { 3, 0, STAT_AGILITY, 0, DAM_KNOCKED_DOWN, 5001, 5004 },
            { 3, 0, STAT_ENDURANCE, 0, DAM_CRIP_LEG_RIGHT, 5001, 5004 },
            { 3, DAM_KNOCKED_DOWN, STAT_ENDURANCE, -2, DAM_CRIP_LEG_RIGHT, 5001, 5004 },
            { 4, DAM_KNOCKED_DOWN, STAT_ENDURANCE, -4, DAM_CRIP_LEG_RIGHT, 5016, 5022 },
            { 4, DAM_KNOCKED_DOWN, STAT_ENDURANCE, -5, DAM_CRIP_LEG_RIGHT, 5023, 5024 },
            { 4, DAM_KNOCKED_DOWN, STAT_ENDURANCE, -6, DAM_CRIP_LEG_RIGHT, 5023, 5024 },
        },
        // HIT_LOCATION_EYES
        {
            { 4, 0, -1, 0, 0, 5001, 5000 },
            { 4, 0, STAT_ENDURANCE, -3, DAM_LOSE_TURN, 5001, 6301 },
            { 6, DAM_BYPASS, STAT_ENDURANCE, -6, DAM_LOSE_TURN, 6300, 6301 },
            { 6, DAM_BYPASS, STAT_ENDURANCE, -2, DAM_BLIND, 6301, 6302 },
            { 8, DAM_BLIND | DAM_BYPASS, -1, 0, 0, 6302, 5000 },
            { 8, DAM_BLIND | DAM_BYPASS, -1, 0, 0, 6302, 5000 },
        },
        // HIT_LOCATION_GROIN
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 4, 0, -1, 0, 0, 5001, 5000 },
            { 4, 0, -1, 0, 0, 5016, 5000 },
            { 5, 0, STAT_AGILITY, 0, DAM_KNOCKED_DOWN, 5016, 5004 },
            { 5, 0, STAT_AGILITY, -3, DAM_KNOCKED_DOWN, 5016, 5004 },
        },
        // HIT_LOCATION_UNCALLED
        {
            { 3, 0, -1, 0, 0, 5001, 5000 },
            { 3, 0, STAT_ENDURANCE, -1, DAM_BYPASS, 5001, 6300 },
            { 4, 0, -1, 0, 0, 5016, 5000 },
            { 4, 0, STAT_ENDURANCE, -1, DAM_BYPASS, 5016, 6300 },
            { 5, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 5004, 5000 },
            { 5, DAM_KNOCKED_DOWN | DAM_BYPASS | DAM_LOSE_TURN, -1, 0, 0, 5005, 5000 },
        },
    },
    // KILL_TYPE_PLANT
    {
        // HIT_LOCATION_HEAD
        {
            { 4, 0, -1, 0, 0, 6405, 5000 },
            { 4, DAM_BYPASS, -1, 0, 0, 6400, 5000 },
            { 5, 0, -1, 0, 0, 6401, 5000 },
            { 5, DAM_BYPASS, -1, 0, 0, 6402, 5000 },
            { 6, DAM_BYPASS, STAT_ENDURANCE, -3, DAM_LOSE_TURN, 6402, 6403 },
            { 6, DAM_BYPASS, STAT_ENDURANCE, -6, DAM_LOSE_TURN, 6402, 6403 },
        },
        // HIT_LOCATION_LEFT_ARM
        {
            { 3, 0, -1, 0, 0, 6405, 5000 },
            { 3, 0, -1, 0, 0, 6405, 5000 },
            { 3, 0, -1, 0, 0, 6405, 5000 },
            { 3, 0, -1, 0, 0, 6405, 5000 },
            { 3, 0, -1, 0, 0, 6405, 5000 },
            { 3, 0, -1, 0, 0, 6405, 5000 },
        },
        // HIT_LOCATION_RIGHT_ARM
        {
            { 3, 0, -1, 0, 0, 6405, 5000 },
            { 3, 0, -1, 0, 0, 6405, 5000 },
            { 3, 0, -1, 0, 0, 6405, 5000 },
            { 3, 0, -1, 0, 0, 6405, 5000 },
            { 3, 0, -1, 0, 0, 6405, 5000 },
            { 3, 0, -1, 0, 0, 6405, 5000 },
        },
        // HIT_LOCATION_TORSO
        {
            { 3, 0, -1, 0, 0, 6405, 5000 },
            { 3, 0, -1, 0, 0, 6405, 5000 },
            { 3, DAM_BYPASS, -1, 0, 0, 6400, 5000 },
            { 4, 0, -1, 0, 0, 6401, 5000 },
            { 4, 0, -1, 0, 0, 6401, 5000 },
            { 4, DAM_BYPASS, -1, 0, 0, 6402, 5000 },
        },
        // HIT_LOCATION_RIGHT_LEG
        {
            { 3, 0, -1, 0, 0, 6405, 5000 },
            { 3, 0, -1, 0, 0, 6405, 5000 },
            { 3, 0, -1, 0, 0, 6405, 5000 },
            { 3, 0, -1, 0, 0, 6405, 5000 },
            { 3, 0, -1, 0, 0, 6405, 5000 },
            { 3, 0, -1, 0, 0, 6405, 5000 },
        },
        // HIT_LOCATION_LEFT_LEG
        {
            { 3, 0, -1, 0, 0, 6405, 5000 },
            { 3, 0, -1, 0, 0, 6405, 5000 },
            { 3, 0, -1, 0, 0, 6405, 5000 },
            { 3, 0, -1, 0, 0, 6405, 5000 },
            { 3, 0, -1, 0, 0, 6405, 5000 },
            { 3, 0, -1, 0, 0, 6405, 5000 },
        },
        // HIT_LOCATION_EYES
        {
            { 4, 0, -1, 0, 0, 6405, 5000 },
            { 4, DAM_BYPASS, -1, 0, 0, 6400, 5000 },
            { 5, 0, -1, 0, 0, 6401, 5000 },
            { 5, DAM_BYPASS, -1, 0, 0, 6402, 5000 },
            { 6, DAM_BYPASS, STAT_ENDURANCE, -4, DAM_BLIND, 6402, 6406 },
            { 6, DAM_BLIND | DAM_BYPASS, -1, 0, 0, 6406, 6404 },
        },
        // HIT_LOCATION_GROIN
        {
            { 3, 0, -1, 0, 0, 6405, 5000 },
            { 3, 0, -1, 0, 0, 6405, 5000 },
            { 4, DAM_BYPASS, -1, 0, 0, 6402, 5000 },
            { 4, DAM_BYPASS, -1, 0, 0, 6402, 5000 },
            { 5, DAM_BYPASS, STAT_ENDURANCE, -3, DAM_LOSE_TURN, 6402, 6403 },
            { 5, DAM_BYPASS, STAT_ENDURANCE, -6, DAM_LOSE_TURN, 6402, 6403 },
        },
        // HIT_LOCATION_UNCALLED
        {
            { 3, 0, -1, 0, 0, 6405, 5000 },
            { 3, 0, -1, 0, 0, 6405, 5000 },
            { 3, DAM_BYPASS, -1, 0, 0, 6400, 5000 },
            { 4, 0, -1, 0, 0, 6401, 5000 },
            { 4, 0, -1, 0, 0, 6401, 5000 },
            { 4, DAM_BYPASS, -1, 0, 0, 6402, 5000 },
        },
    },
    // KILL_TYPE_GECKO
    {
        // HIT_LOCATION_HEAD
        {
            { 4, 0, -1, 0, 0, 6701, 5000 },
            { 4, DAM_BYPASS, STAT_ENDURANCE, 0, DAM_KNOCKED_OUT, 6700, 5003 },
            { 5, DAM_BYPASS, STAT_ENDURANCE, -3, DAM_KNOCKED_OUT, 6700, 5003 },
            { 5, DAM_KNOCKED_DOWN | DAM_BYPASS, STAT_ENDURANCE, -3, DAM_KNOCKED_OUT, 6700, 5003 },
            { 6, DAM_KNOCKED_OUT | DAM_BYPASS, STAT_LUCK, 0, DAM_BLIND, 6700, 5006 },
            { 6, DAM_DEAD, -1, 0, 0, 6700, 5000 },
        },
        // HIT_LOCATION_LEFT_ARM
        {
            { 3, 0, -1, 0, 0, 6702, 5000 },
            { 3, DAM_LOSE_TURN, -1, 0, 0, 6702, 5000 },
            { 4, 0, STAT_ENDURANCE, -3, DAM_CRIP_ARM_LEFT, 6702, 5011 },
            { 4, DAM_CRIP_ARM_LEFT | DAM_BYPASS, -1, 0, 0, 6702, 5000 },
            { 4, DAM_CRIP_ARM_LEFT | DAM_BYPASS, -1, 0, 0, 6702, 5000 },
            { 4, DAM_CRIP_ARM_LEFT | DAM_BYPASS, -1, 0, 0, 6702, 5000 },
        },
        // HIT_LOCATION_RIGHT_ARM
        {
            { 3, 0, -1, 0, 0, 6702, 5000 },
            { 3, DAM_LOSE_TURN, -1, 0, 0, 6702, 5000 },
            { 4, 0, STAT_ENDURANCE, -3, DAM_CRIP_ARM_RIGHT, 6702, 5000 },
            { 4, DAM_CRIP_ARM_RIGHT | DAM_BYPASS, -1, 0, 0, 6702, 5000 },
            { 4, DAM_CRIP_ARM_RIGHT | DAM_BYPASS, -1, 0, 0, 6702, 5000 },
            { 4, DAM_CRIP_ARM_RIGHT | DAM_BYPASS, -1, 0, 0, 6702, 5000 },
        },
        // HIT_LOCATION_TORSO
        {
            { 3, 0, -1, 0, 0, 6701, 5000 },
            { 3, DAM_BYPASS, -1, 0, 0, 6701, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 6704, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 6704, 5000 },
            { 6, DAM_KNOCKED_OUT | DAM_BYPASS, -1, 0, 0, 6704, 5000 },
            { 6, DAM_DEAD, -1, 0, 0, 6704, 5000 },
        },
        // HIT_LOCATION_RIGHT_LEG
        {
            { 3, DAM_KNOCKED_DOWN, -1, 0, 0, 6705, 5000 },
            { 3, DAM_KNOCKED_DOWN, STAT_ENDURANCE, 0, DAM_CRIP_LEG_RIGHT, 6705, 5024 },
            { 4, DAM_KNOCKED_DOWN, STAT_ENDURANCE, -3, DAM_CRIP_LEG_RIGHT, 6705, 5024 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_RIGHT | DAM_BYPASS, -1, 0, 0, 6705, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_RIGHT | DAM_BYPASS, STAT_ENDURANCE, 0, DAM_KNOCKED_OUT, 6705, 5026 },
            { 4, DAM_KNOCKED_OUT | DAM_CRIP_LEG_RIGHT | DAM_BYPASS, -1, 0, 0, 6705, 5000 },
        },
        // HIT_LOCATION_LEFT_LEG
        {
            { 3, DAM_KNOCKED_DOWN, -1, 0, 0, 6705, 5000 },
            { 3, DAM_KNOCKED_DOWN, STAT_ENDURANCE, 0, DAM_CRIP_LEG_LEFT, 6705, 5024 },
            { 4, DAM_KNOCKED_DOWN, STAT_ENDURANCE, -3, DAM_CRIP_LEG_LEFT, 6705, 5024 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_LEFT | DAM_BYPASS, -1, 0, 0, 6705, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_LEFT | DAM_BYPASS, STAT_ENDURANCE, 0, DAM_KNOCKED_OUT, 6705, 5026 },
            { 4, DAM_KNOCKED_OUT | DAM_CRIP_LEG_LEFT | DAM_BYPASS, -1, 0, 0, 6705, 5000 },
        },
        // HIT_LOCATION_EYES
        {
            { 4, 0, STAT_LUCK, 4, DAM_BLIND, 6700, 5028 },
            { 4, DAM_BYPASS, STAT_LUCK, 3, DAM_BLIND, 6700, 5028 },
            { 6, DAM_BYPASS, STAT_LUCK, 2, DAM_BLIND, 6700, 5028 },
            { 6, DAM_BLIND | DAM_BYPASS | DAM_LOSE_TURN, -1, 0, 0, 6700, 5000 },
            { 8, DAM_KNOCKED_OUT | DAM_BLIND | DAM_BYPASS, -1, 0, 0, 6700, 5000 },
            { 8, DAM_DEAD, -1, 0, 0, 6700, 5000 },
        },
        // HIT_LOCATION_GROIN
        {
            { 3, 0, -1, 0, 0, 6703, 5000 },
            { 3, DAM_BYPASS, STAT_ENDURANCE, -3, DAM_KNOCKED_DOWN, 6703, 5035 },
            { 3, DAM_KNOCKED_DOWN, STAT_ENDURANCE, -3, DAM_KNOCKED_OUT, 6703, 5036 },
            { 3, DAM_KNOCKED_OUT, -1, 0, 0, 6703, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, STAT_ENDURANCE, 0, DAM_KNOCKED_OUT, 6703, 5036 },
            { 4, DAM_KNOCKED_OUT | DAM_BYPASS, -1, 0, 0, 6703, 5000 },
        },
        // HIT_LOCATION_UNCALLED
        {
            { 3, 0, -1, 0, 0, 6700, 5000 },
            { 3, DAM_BYPASS, -1, 0, 0, 6700, 5000 },
            { 4, 0, -1, 0, 0, 6700, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 6700, 5000 },
            { 6, DAM_KNOCKED_OUT | DAM_BYPASS, -1, 0, 0, 6700, 5000 },
            { 6, DAM_DEAD, -1, 0, 0, 6700, 5000 },
        },
    },
    // KILL_TYPE_ALIEN
    {
        // HIT_LOCATION_HEAD
        {
            { 4, 0, -1, 0, 0, 6801, 5000 },
            { 4, DAM_BYPASS, STAT_ENDURANCE, 0, DAM_KNOCKED_OUT, 6800, 5003 },
            { 5, DAM_BYPASS, STAT_ENDURANCE, -3, DAM_KNOCKED_OUT, 6800, 5003 },
            { 5, DAM_KNOCKED_DOWN | DAM_BYPASS, STAT_ENDURANCE, -3, DAM_KNOCKED_OUT, 6803, 5003 },
            { 6, DAM_KNOCKED_OUT | DAM_BYPASS, STAT_LUCK, 0, DAM_BLIND, 6804, 5006 },
            { 6, DAM_DEAD, -1, 0, 0, 6804, 5000 },
        },
        // HIT_LOCATION_LEFT_ARM
        {
            { 3, 0, -1, 0, 0, 6806, 5000 },
            { 3, DAM_LOSE_TURN, -1, 0, 0, 6806, 5000 },
            { 4, 0, STAT_ENDURANCE, -3, DAM_CRIP_ARM_LEFT, 6806, 5011 },
            { 4, DAM_CRIP_ARM_LEFT | DAM_BYPASS, -1, 0, 0, 6806, 5000 },
            { 4, DAM_CRIP_ARM_LEFT | DAM_BYPASS, -1, 0, 0, 6806, 5000 },
            { 4, DAM_CRIP_ARM_LEFT | DAM_BYPASS, -1, 0, 0, 6806, 5000 },
        },
        // HIT_LOCATION_RIGHT_ARM
        {
            { 3, 0, -1, 0, 0, 6806, 5000 },
            { 3, DAM_LOSE_TURN, -1, 0, 0, 6806, 5000 },
            { 4, 0, STAT_ENDURANCE, -3, DAM_CRIP_ARM_RIGHT, 6806, 5000 },
            { 4, DAM_CRIP_ARM_RIGHT | DAM_BYPASS, -1, 0, 0, 6806, 5000 },
            { 4, DAM_CRIP_ARM_RIGHT | DAM_BYPASS, -1, 0, 0, 6806, 5000 },
            { 4, DAM_CRIP_ARM_RIGHT | DAM_BYPASS, -1, 0, 0, 6806, 5000 },
        },
        // HIT_LOCATION_TORSO
        {
            { 3, 0, -1, 0, 0, 6800, 5000 },
            { 3, DAM_BYPASS, -1, 0, 0, 6800, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 6800, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 6800, 5000 },
            { 6, DAM_KNOCKED_OUT | DAM_BYPASS, -1, 0, 0, 6800, 5000 },
            { 6, DAM_DEAD, -1, 0, 0, 6800, 5000 },
        },
        // HIT_LOCATION_RIGHT_LEG
        {
            { 3, DAM_KNOCKED_DOWN, -1, 0, 0, 6805, 5000 },
            { 3, DAM_KNOCKED_DOWN, STAT_ENDURANCE, 0, DAM_CRIP_LEG_RIGHT, 6805, 5024 },
            { 4, DAM_KNOCKED_DOWN, STAT_ENDURANCE, -3, DAM_CRIP_LEG_RIGHT, 6805, 5024 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_RIGHT | DAM_BYPASS, -1, 0, 0, 6805, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_RIGHT | DAM_BYPASS, STAT_ENDURANCE, 0, DAM_KNOCKED_OUT, 6805, 5026 },
            { 4, DAM_KNOCKED_OUT | DAM_CRIP_LEG_RIGHT | DAM_BYPASS, -1, 0, 0, 6805, 5000 },
        },
        // HIT_LOCATION_LEFT_LEG
        {
            { 3, DAM_KNOCKED_DOWN, -1, 0, 0, 6805, 5000 },
            { 3, DAM_KNOCKED_DOWN, STAT_ENDURANCE, 0, DAM_CRIP_LEG_LEFT, 6805, 5024 },
            { 4, DAM_KNOCKED_DOWN, STAT_ENDURANCE, -3, DAM_CRIP_LEG_LEFT, 6805, 5024 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_LEFT | DAM_BYPASS, -1, 0, 0, 6805, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_LEFT | DAM_BYPASS, STAT_ENDURANCE, 0, DAM_KNOCKED_OUT, 6805, 5026 },
            { 4, DAM_KNOCKED_OUT | DAM_CRIP_LEG_LEFT | DAM_BYPASS, -1, 0, 0, 6805, 5000 },
        },
        // HIT_LOCATION_EYES
        {
            { 4, 0, STAT_LUCK, 4, DAM_BLIND, 6803, 5028 },
            { 4, DAM_BYPASS, STAT_LUCK, 3, DAM_BLIND, 6803, 5028 },
            { 6, DAM_BYPASS, STAT_LUCK, 2, DAM_BLIND, 6803, 5028 },
            { 6, DAM_BLIND | DAM_BYPASS | DAM_LOSE_TURN, -1, 0, 0, 6803, 5000 },
            { 8, DAM_KNOCKED_OUT | DAM_BLIND | DAM_BYPASS, -1, 0, 0, 6803, 5000 },
            { 8, DAM_DEAD, -1, 0, 0, 6804, 5000 },
        },
        // HIT_LOCATION_GROIN
        {
            { 3, 0, -1, 0, 0, 6801, 5000 },
            { 3, DAM_BYPASS, STAT_ENDURANCE, -3, DAM_KNOCKED_DOWN, 6801, 5035 },
            { 3, DAM_KNOCKED_DOWN, STAT_ENDURANCE, -3, DAM_KNOCKED_OUT, 6801, 5036 },
            { 3, DAM_KNOCKED_OUT, -1, 0, 0, 6801, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, STAT_ENDURANCE, 0, DAM_KNOCKED_OUT, 6804, 5036 },
            { 4, DAM_KNOCKED_OUT | DAM_BYPASS, -1, 0, 0, 6804, 5000 },
        },
        // HIT_LOCATION_UNCALLED
        {
            { 3, 0, -1, 0, 0, 6800, 5000 },
            { 3, DAM_BYPASS, -1, 0, 0, 6800, 5000 },
            { 4, 0, -1, 0, 0, 6800, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 6800, 5000 },
            { 6, DAM_KNOCKED_OUT | DAM_BYPASS, -1, 0, 0, 6800, 5000 },
            { 6, DAM_DEAD, -1, 0, 0, 6800, 5000 },
        },
    },
    // KILL_TYPE_GIANT_ANT
    {
        // HIT_LOCATION_HEAD
        {
            { 4, 0, -1, 0, 0, 6901, 5000 },
            { 4, DAM_BYPASS, STAT_ENDURANCE, 0, DAM_KNOCKED_OUT, 6901, 5003 },
            { 5, DAM_BYPASS, STAT_ENDURANCE, -3, DAM_KNOCKED_OUT, 6902, 5003 },
            { 5, DAM_KNOCKED_DOWN | DAM_BYPASS, STAT_ENDURANCE, -3, DAM_KNOCKED_OUT, 6902, 5003 },
            { 6, DAM_KNOCKED_OUT | DAM_BYPASS, STAT_LUCK, 0, DAM_BLIND, 6902, 5006 },
            { 6, DAM_DEAD, -1, 0, 0, 6902, 5000 },
        },
        // HIT_LOCATION_LEFT_ARM
        {
            { 3, 0, -1, 0, 0, 6906, 5000 },
            { 3, DAM_LOSE_TURN, -1, 0, 0, 6906, 5000 },
            { 4, 0, STAT_ENDURANCE, -3, DAM_CRIP_ARM_LEFT, 6906, 5011 },
            { 4, DAM_CRIP_ARM_LEFT | DAM_BYPASS, -1, 0, 0, 6906, 5000 },
            { 4, DAM_CRIP_ARM_LEFT | DAM_BYPASS, -1, 0, 0, 6906, 5000 },
            { 4, DAM_CRIP_ARM_LEFT | DAM_BYPASS, -1, 0, 0, 6906, 5000 },
        },
        // HIT_LOCATION_RIGHT_ARM
        {
            { 3, 0, -1, 0, 0, 6906, 5000 },
            { 3, DAM_LOSE_TURN, -1, 0, 0, 6906, 5000 },
            { 4, 0, STAT_ENDURANCE, -3, DAM_CRIP_ARM_RIGHT, 6906, 5000 },
            { 4, DAM_CRIP_ARM_RIGHT | DAM_BYPASS, -1, 0, 0, 6906, 5000 },
            { 4, DAM_CRIP_ARM_RIGHT | DAM_BYPASS, -1, 0, 0, 6906, 5000 },
            { 4, DAM_CRIP_ARM_RIGHT | DAM_BYPASS, -1, 0, 0, 6906, 5000 },
        },
        // HIT_LOCATION_TORSO
        {
            { 3, 0, -1, 0, 0, 6900, 5000 },
            { 3, DAM_BYPASS, -1, 0, 0, 6900, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 6904, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 6904, 5000 },
            { 6, DAM_KNOCKED_OUT | DAM_BYPASS, -1, 0, 0, 6904, 5000 },
            { 6, DAM_DEAD, -1, 0, 0, 6904, 5000 },
        },
        // HIT_LOCATION_RIGHT_LEG
        {
            { 3, DAM_KNOCKED_DOWN, -1, 0, 0, 6905, 5000 },
            { 3, DAM_KNOCKED_DOWN, STAT_ENDURANCE, 0, DAM_CRIP_LEG_RIGHT, 6905, 5024 },
            { 4, DAM_KNOCKED_DOWN, STAT_ENDURANCE, -3, DAM_CRIP_LEG_RIGHT, 6905, 5024 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_RIGHT | DAM_BYPASS, -1, 0, 0, 6905, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_RIGHT | DAM_BYPASS, STAT_ENDURANCE, 0, DAM_KNOCKED_OUT, 6905, 5026 },
            { 4, DAM_KNOCKED_OUT | DAM_CRIP_LEG_RIGHT | DAM_BYPASS, -1, 0, 0, 6905, 5000 },
        },
        // HIT_LOCATION_LEFT_LEG
        {
            { 3, DAM_KNOCKED_DOWN, -1, 0, 0, 6905, 5000 },
            { 3, DAM_KNOCKED_DOWN, STAT_ENDURANCE, 0, DAM_CRIP_LEG_LEFT, 6905, 5024 },
            { 4, DAM_KNOCKED_DOWN, STAT_ENDURANCE, -3, DAM_CRIP_LEG_LEFT, 6905, 5024 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_LEFT | DAM_BYPASS, -1, 0, 0, 6905, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_LEFT | DAM_BYPASS, STAT_ENDURANCE, 0, DAM_KNOCKED_OUT, 6905, 5026 },
            { 4, DAM_KNOCKED_OUT | DAM_CRIP_LEG_LEFT | DAM_BYPASS, -1, 0, 0, 6905, 5000 },
        },
        // HIT_LOCATION_EYES
        {
            { 4, 0, STAT_LUCK, 4, DAM_BLIND, 6900, 5028 },
            { 4, DAM_BYPASS, STAT_LUCK, 3, DAM_BLIND, 6906, 5028 },
            { 6, DAM_BYPASS, STAT_LUCK, 2, DAM_BLIND, 6901, 5028 },
            { 6, DAM_BLIND | DAM_BYPASS | DAM_LOSE_TURN, -1, 0, 0, 6901, 5000 },
            { 8, DAM_KNOCKED_OUT | DAM_BLIND | DAM_BYPASS, -1, 0, 0, 6901, 5000 },
            { 8, DAM_DEAD, -1, 0, 0, 6901, 5000 },
        },
        // HIT_LOCATION_GROIN
        {
            { 3, 0, -1, 0, 0, 6900, 5000 },
            { 3, DAM_BYPASS, STAT_ENDURANCE, -3, DAM_KNOCKED_DOWN, 6900, 5035 },
            { 3, DAM_KNOCKED_DOWN, STAT_ENDURANCE, -3, DAM_KNOCKED_OUT, 6900, 5036 },
            { 3, DAM_KNOCKED_OUT, -1, 0, 0, 6903, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, STAT_ENDURANCE, 0, DAM_KNOCKED_OUT, 6903, 5036 },
            { 4, DAM_KNOCKED_OUT | DAM_BYPASS, -1, 0, 0, 6903, 5000 },
        },
        // HIT_LOCATION_UNCALLED
        {
            { 3, 0, -1, 0, 0, 6900, 5000 },
            { 3, DAM_BYPASS, -1, 0, 0, 6900, 5000 },
            { 4, 0, -1, 0, 0, 6904, 5000 },
            { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 6904, 5000 },
            { 6, DAM_KNOCKED_OUT | DAM_BYPASS, -1, 0, 0, 6904, 5000 },
            { 6, DAM_DEAD, -1, 0, 0, 6904, 5000 },
        },
    },
    // KILL_TYPE_BIG_BAD_BOSS
    {
        // HIT_LOCATION_HEAD
        {
            { 3, 0, -1, 0, 0, 7101, 7100 },
            { 3, 0, -1, 0, 0, 7102, 7103 },
            { 4, 0, -1, 0, 0, 7102, 7103 },
            { 4, DAM_LOSE_TURN, -1, 0, 0, 7104, 7103 },
            { 5, DAM_KNOCKED_DOWN, STAT_LUCK, 0, DAM_BLIND, 7105, 7106 },
            { 6, DAM_KNOCKED_DOWN, -1, 0, 0, 7105, 7100 },
        },
        // HIT_LOCATION_LEFT_ARM
        {
            { 3, 0, -1, 0, 0, 7106, 7100 },
            { 3, 0, -1, 0, 0, 7106, 7100 },
            { 3, DAM_LOSE_TURN, -1, 0, 0, 7106, 7011 },
            { 4, DAM_LOSE_TURN, -1, 0, 0, 7106, 7100 },
            { 4, DAM_CRIP_ARM_LEFT, -1, 0, 0, 7106, 7100 },
            { 4, DAM_CRIP_ARM_LEFT, -1, 0, 0, 7106, 7100 },
        },
        // HIT_LOCATION_RIGHT_ARM
        {
            { 3, 0, -1, 0, 0, 7106, 7100 },
            { 3, 0, -1, 0, 0, 7106, 7100 },
            { 3, DAM_LOSE_TURN, -1, 0, 0, 7106, 7100 },
            { 4, DAM_LOSE_TURN, -1, 0, 0, 7106, 7100 },
            { 4, DAM_CRIP_ARM_RIGHT, -1, 0, 0, 7106, 7100 },
            { 4, DAM_CRIP_ARM_RIGHT, -1, 0, 0, 7106, 7100 },
        },
        // HIT_LOCATION_TORSO
        {
            { 3, 0, -1, 0, 0, 7106, 7100 },
            { 3, 0, -1, 0, 0, 7106, 7100 },
            { 3, 0, -1, 0, 0, 7106, 7100 },
            { 4, 0, -1, 0, 0, 7106, 7100 },
            { 4, DAM_KNOCKED_DOWN, -1, 0, 0, 7106, 7100 },
            { 5, DAM_KNOCKED_DOWN, -1, 0, 0, 7106, 7100 },
        },
        // HIT_LOCATION_RIGHT_LEG
        {
            { 3, DAM_KNOCKED_DOWN, -1, 0, 0, 7106, 7100 },
            { 3, DAM_KNOCKED_DOWN, STAT_ENDURANCE, 0, DAM_CRIP_LEG_RIGHT, 7106, 7106 },
            { 3, DAM_KNOCKED_DOWN, STAT_ENDURANCE, -3, DAM_CRIP_LEG_RIGHT, 7060, 7106 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_RIGHT, -1, 0, 0, 7106, 7100 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_RIGHT, -1, 0, 0, 7106, 7106 },
            { 4, DAM_CRIP_LEG_RIGHT, -1, 0, 0, 7106, 7100 },
        },
        // HIT_LOCATION_LEFT_LEG
        {
            { 3, DAM_KNOCKED_DOWN, -1, 0, 0, 7106, 7100 },
            { 3, DAM_KNOCKED_DOWN, STAT_ENDURANCE, 0, DAM_CRIP_LEG_LEFT, 7106, 7024 },
            { 3, DAM_KNOCKED_DOWN, STAT_ENDURANCE, -3, DAM_CRIP_LEG_LEFT, 7106, 7024 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_LEFT, -1, 0, 0, 7106, 7100 },
            { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_LEFT, -1, 0, 0, 7106, 7106 },
            { 4, DAM_CRIP_LEG_LEFT, -1, 0, 0, 7106, 7100 },
        },
        // HIT_LOCATION_EYES
        {
            { 3, 0, -1, 0, 0, 7106, 7106 },
            { 3, 0, -1, 0, 0, 7106, 7106 },
            { 4, 0, STAT_LUCK, 2, DAM_BLIND, 7106, 7106 },
            { 4, DAM_LOSE_TURN, -1, 0, 0, 7106, 7100 },
            { 5, DAM_BLIND | DAM_LOSE_TURN, -1, 0, 0, 7106, 7100 },
            { 5, DAM_BLIND | DAM_LOSE_TURN, -1, 0, 0, 7106, 7100 },
        },
        // HIT_LOCATION_GROIN
        {
            { 3, 0, -1, 0, 0, 7106, 7100 },
            { 3, 0, STAT_ENDURANCE, -3, DAM_KNOCKED_DOWN, 7106, 7106 },
            { 3, DAM_KNOCKED_DOWN, -1, 0, 0, 7106, 7106 },
            { 3, DAM_KNOCKED_DOWN, -1, 0, 0, 7106, 7100 },
            { 4, 0, -1, 0, 0, 7106, 7106 },
            { 4, DAM_KNOCKED_DOWN, -1, 0, 0, 7106, 7100 },
        },
        // HIT_LOCATION_UNCALLED
        {
            { 3, 0, -1, 0, 0, 7106, 7100 },
            { 3, 0, -1, 0, 0, 7106, 7100 },
            { 4, 0, -1, 0, 0, 7106, 7100 },
            { 4, 0, -1, 0, 0, 7106, 7100 },
            { 5, DAM_KNOCKED_DOWN, -1, 0, 0, 7106, 7100 },
            { 5, DAM_KNOCKED_DOWN, -1, 0, 0, 7106, 7100 },
        },
    },
};

// Player's criticals effects.
//
// 0x5179B0
CriticalHitDescription gPlayerCriticalHitTable[HIT_LOCATION_COUNT][CRTICIAL_EFFECT_COUNT] = {
    {
        { 3, 0, -1, 0, 0, 6500, 5000 },
        { 3, DAM_BYPASS, STAT_ENDURANCE, 3, DAM_KNOCKED_DOWN, 6501, 6503 },
        { 3, DAM_BYPASS, STAT_ENDURANCE, 0, DAM_KNOCKED_DOWN, 6501, 6503 },
        { 3, DAM_KNOCKED_DOWN | DAM_BYPASS, STAT_ENDURANCE, 2, DAM_KNOCKED_OUT, 6503, 6502 },
        { 3, DAM_KNOCKED_OUT | DAM_BYPASS, STAT_LUCK, 2, DAM_BLIND, 6502, 6504 },
        { 6, DAM_BYPASS, STAT_ENDURANCE, -2, DAM_DEAD, 6501, 6505 },
    },
    {
        { 2, 0, -1, 0, 0, 6506, 5000 },
        { 2, DAM_LOSE_TURN, -1, 0, 0, 6507, 5000 },
        { 3, 0, STAT_ENDURANCE, 0, DAM_CRIP_ARM_LEFT, 6508, 6509 },
        { 3, DAM_BYPASS, -1, 0, 0, 6501, 5000 },
        { 3, DAM_CRIP_ARM_LEFT | DAM_BYPASS, -1, 0, 0, 6510, 5000 },
        { 3, DAM_CRIP_ARM_LEFT | DAM_BYPASS, -1, 0, 0, 6510, 5000 },
    },
    {
        { 2, 0, -1, 0, 0, 6506, 5000 },
        { 2, DAM_LOSE_TURN, -1, 0, 0, 6507, 5000 },
        { 3, 0, STAT_ENDURANCE, 0, DAM_CRIP_ARM_RIGHT, 6508, 6509 },
        { 3, DAM_BYPASS, -1, 0, 0, 6501, 5000 },
        { 3, DAM_CRIP_ARM_RIGHT | DAM_BYPASS, -1, 0, 0, 6511, 5000 },
        { 3, DAM_CRIP_ARM_RIGHT | DAM_BYPASS, -1, 0, 0, 6511, 5000 },
    },
    {
        { 3, 0, -1, 0, 0, 6512, 5000 },
        { 3, 0, -1, 0, 0, 6512, 5000 },
        { 3, DAM_BYPASS, -1, 0, 0, 6508, 5000 },
        { 3, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 6503, 5000 },
        { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 6503, 5000 },
        { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, STAT_LUCK, 2, DAM_DEAD, 6503, 6513 },
    },
    {
        { 3, 0, -1, 0, 0, 6512, 5000 },
        { 3, DAM_KNOCKED_DOWN, -1, 0, 0, 6514, 5000 },
        { 3, DAM_KNOCKED_DOWN, STAT_ENDURANCE, 0, DAM_CRIP_LEG_RIGHT, 6514, 6515 },
        { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_RIGHT | DAM_BYPASS, -1, 0, 0, 6516, 5000 },
        { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_RIGHT | DAM_BYPASS, -1, 0, 0, 6516, 5000 },
        { 4, DAM_KNOCKED_OUT | DAM_CRIP_LEG_RIGHT | DAM_BYPASS, -1, 0, 0, 6517, 5000 },
    },
    {
        { 3, 0, -1, 0, 0, 6512, 5000 },
        { 3, DAM_KNOCKED_DOWN, -1, 0, 0, 6514, 5000 },
        { 3, DAM_KNOCKED_DOWN, STAT_ENDURANCE, 0, DAM_CRIP_LEG_LEFT, 6514, 6515 },
        { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_LEFT | DAM_BYPASS, -1, 0, 0, 6516, 5000 },
        { 4, DAM_KNOCKED_DOWN | DAM_CRIP_LEG_LEFT | DAM_BYPASS, -1, 0, 0, 6516, 5000 },
        { 4, DAM_KNOCKED_OUT | DAM_CRIP_LEG_LEFT | DAM_BYPASS, -1, 0, 0, 6517, 5000 },
    },
    {
        { 3, 0, -1, 0, 0, 6518, 5000 },
        { 3, 0, STAT_LUCK, 3, DAM_BLIND, 6518, 6519 },
        { 3, DAM_BYPASS, STAT_LUCK, 3, DAM_BLIND, 6501, 6519 },
        { 4, DAM_BYPASS | DAM_LOSE_TURN, -1, 0, 0, 6520, 5000 },
        { 4, DAM_BLIND | DAM_BYPASS | DAM_LOSE_TURN, -1, 0, 0, 6521, 5000 },
        { 6, DAM_DEAD, -1, 0, 0, 6522, 5000 },
    },
    {
        { 3, 0, -1, 0, 0, 6523, 5000 },
        { 3, 0, STAT_ENDURANCE, 0, DAM_KNOCKED_DOWN, 6523, 6524 },
        { 3, DAM_KNOCKED_DOWN, -1, 0, 0, 6524, 5000 },
        { 3, DAM_KNOCKED_DOWN, STAT_ENDURANCE, 4, DAM_KNOCKED_OUT, 6524, 6525 },
        { 4, DAM_KNOCKED_DOWN, STAT_ENDURANCE, 2, DAM_KNOCKED_OUT, 6524, 6525 },
        { 4, DAM_KNOCKED_OUT, -1, 0, 0, 6526, 5000 },
    },
    {
        { 3, 0, -1, 0, 0, 6512, 5000 },
        { 3, 0, -1, 0, 0, 6512, 5000 },
        { 3, DAM_BYPASS, -1, 0, 0, 6508, 5000 },
        { 3, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 6503, 5000 },
        { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, -1, 0, 0, 6503, 5000 },
        { 4, DAM_KNOCKED_DOWN | DAM_BYPASS, STAT_LUCK, 2, DAM_DEAD, 6503, 6513 },
    },
};

// 0x517F98
int dword_517F98 = 0;

// 0x517F9C
bool dword_517F9C = false;

// Provides effects caused by failing weapons.
//
// 0x517FA0
const int dword_517FA0[WEAPON_CRITICAL_FAILURE_TYPE_COUNT][WEAPON_CRITICAL_FAILURE_EFFECT_COUNT] = {
    { 0, DAM_LOSE_TURN, DAM_LOSE_TURN, DAM_HURT_SELF | DAM_KNOCKED_DOWN, DAM_CRIP_RANDOM },
    { 0, DAM_LOSE_TURN, DAM_DROP, DAM_RANDOM_HIT, DAM_HIT_SELF },
    { 0, DAM_LOSE_AMMO, DAM_DROP, DAM_RANDOM_HIT, DAM_DESTROY },
    { DAM_LOSE_TURN, DAM_LOSE_TURN | DAM_LOSE_AMMO, DAM_DROP | DAM_LOSE_TURN, DAM_RANDOM_HIT, DAM_EXPLODE | DAM_LOSE_TURN },
    { DAM_DUD, DAM_DROP, DAM_DROP | DAM_HURT_SELF, DAM_RANDOM_HIT, DAM_EXPLODE },
    { DAM_LOSE_TURN, DAM_DUD, DAM_DESTROY, DAM_RANDOM_HIT, DAM_EXPLODE | DAM_LOSE_TURN | DAM_KNOCKED_DOWN },
    { 0, DAM_LOSE_TURN, DAM_RANDOM_HIT, DAM_DESTROY, DAM_EXPLODE | DAM_LOSE_TURN | DAM_ON_FIRE },
};

// 0x51802C
const int dword_51802C[4] = {
    122,
    188,
    251,
    316,
};

// 0x51803C
const int dword_51803C[4] = {
    HIT_LOCATION_HEAD,
    HIT_LOCATION_EYES,
    HIT_LOCATION_RIGHT_ARM,
    HIT_LOCATION_RIGHT_LEG,
};

// 0x51804C
const int dword_51804C[4] = {
    HIT_LOCATION_TORSO,
    HIT_LOCATION_GROIN,
    HIT_LOCATION_LEFT_ARM,
    HIT_LOCATION_LEFT_LEG,
};

// 0x56D2B0
Attack stru_56D2B0;

// combat.msg
//
// 0x56D368
MessageList gCombatMessageList;

// 0x56D370
Object* gCalledShotCritter;

// 0x56D374
int gCalledShotWindow;

// 0x56D378
int dword_56D378;

// 0x56D37C
int dword_56D37C;

// Probably last who_hit_me of obj_dude
//
// 0x56D380
Object* off_56D380;

// 0x56D384
int dword_56D384;

// 0x56D388
Object* off_56D388;

// target_highlight
//
// 0x56D38C
int dword_56D38C;

// 0x56D390
Object** off_56D390;

// 0x56D394
int dword_56D394;

// Experience received for killing critters during current combat.
//
// 0x56D398
int dword_56D398;

// bonus action points from BONUS_MOVE perk.
//
// 0x56D39C
int dword_56D39C;

// 0x56D3A0
Attack stru_56D3A0;

// 0x56D458
Attack stru_56D458;

// combat_init
// 0x420CC0
int combatInit()
{
    int max_action_points;
    char path[MAX_PATH];

    dword_51093C = 0;
    dword_510940 = 0;
    off_56D390 = 0;
    off_510948 = 0;
    dword_56D394 = 0;
    dword_56D384 = 0;
    dword_56D37C = 0;
    off_51094C = 0;
    dword_510950 = 0;
    gCombatState = COMBAT_STATE_0x02;

    max_action_points = critterGetStat(gDude, STAT_MAXIMUM_ACTION_POINTS);

    dword_56D39C = 0;
    off_56D380 = NULL;
    dword_517F98 = 0;

    gDude->data.critter.combat.ap = max_action_points;

    dword_517F9C = 0;

    if (!messageListInit(&gCombatMessageList)) {
        return -1;
    }

    sprintf(path, "%s%s", asc_5186C8, "combat.msg");

    if (!messageListLoad(&gCombatMessageList, path)) {
        return -1;
    }

    return 0;
}

// 0x420DA0
void combatReset()
{
    int max_action_points;

    dword_51093C = 0;
    dword_510940 = 0;
    off_56D390 = 0;
    off_510948 = 0;
    dword_56D394 = 0;
    dword_56D384 = 0;
    dword_56D37C = 0;
    off_51094C = 0;
    dword_510950 = 0;
    gCombatState = COMBAT_STATE_0x02;

    max_action_points = critterGetStat(gDude, STAT_MAXIMUM_ACTION_POINTS);

    dword_56D39C = 0;
    off_56D380 = NULL;

    gDude->data.critter.combat.ap = max_action_points;
}

// 0x420E14
void combatExit()
{
    messageListFree(&gCombatMessageList);
}

// 0x420E24
int sub_420E24(int a1, int cid, Object** critterList, int critterListLength)
{
    int index;

    for (index = a1; index < critterListLength; index++) {
        if (critterList[index]->cid == cid) {
            break;
        }
    }

    return index;
}

// 0x420E4C
int combatLoad(File* stream)
{
    int v14;
    STRUCT_510948* ptr;
    int a2;
    Object* obj;
    int v24;
    int i;
    int j;

    if (fileReadInt32(stream, &gCombatState) == -1) return -1;

    if (!isInCombat()) {
        obj = objectFindFirst();
        while (obj != NULL) {
            if (obj->pid >> 24 == OBJ_TYPE_CRITTER) {
                if (obj->data.critter.combat.whoHitMeCid == -1) {
                    obj->data.critter.combat.whoHitMe = NULL;
                }
            }
            obj = objectFindNext();
        }
        return 0;
    }

    if (fileReadInt32(stream, &dword_51093C) == -1) return -1;
    if (fileReadInt32(stream, &dword_56D39C) == -1) return -1;
    if (fileReadInt32(stream, &dword_56D398) == -1) return -1;
    if (fileReadInt32(stream, &dword_56D394) == -1) return -1;
    if (fileReadInt32(stream, &dword_56D384) == -1) return -1;
    if (fileReadInt32(stream, &dword_56D37C) == -1) return -1;

    if (objectListCreate(-1, gElevation, 1, &off_56D390) != dword_56D37C) {
        objectListFree(off_56D390);
        return -1;
    }

    if (fileReadInt32(stream, &v24) == -1) return -1;

    gDude->cid = v24;

    for (i = 0; i < dword_56D37C; i++) {
        if (off_56D390[i]->data.critter.combat.whoHitMeCid == -1) {
            off_56D390[i]->data.critter.combat.whoHitMe = NULL;
        } else {
            for (j = 0; j < dword_56D37C; j++) {
                if (off_56D390[i]->data.critter.combat.whoHitMeCid == off_56D390[j]->cid) {
                    break;
                }
            }

            if (j == dword_56D37C) {
                off_56D390[i]->data.critter.combat.whoHitMe = NULL;
            } else {
                off_56D390[i]->data.critter.combat.whoHitMe = off_56D390[j];
            }
        }
    }

    for (i = 0; i < dword_56D37C; i++) {
        if (fileReadInt32(stream, &v24) == -1) return -1;

        for (j = i; j < dword_56D37C; j++) {
            if (v24 == off_56D390[j]->cid) {
                break;
            }
        }

        if (j == dword_56D37C) {
            return -1;
        }

        obj = off_56D390[i];
        off_56D390[i] = off_56D390[j];
        off_56D390[j] = obj;
    }

    for (i = 0; i < dword_56D37C; i++) {
        off_56D390[i]->cid = i;
    }

    if (off_510948) {
        internal_free(off_510948);
    }

    off_510948 = internal_malloc(sizeof(*off_510948) * dword_56D37C);
    if (off_510948 == NULL) {
        return -1;
    }

    for (v14 = 0; v14 < dword_56D37C; v14++) {
        ptr = &(off_510948[v14]);

        if (fileReadInt32(stream, &a2) == -1) return -1;

        if (a2 == -1) {
            ptr->field_0 = 0;
        } else {
            ptr->field_0 = objectFindById(a2);
            if (ptr->field_0 == NULL) return -1;
        }

        if (fileReadInt32(stream, &a2) == -1) return -1;

        if (a2 == -1) {
            ptr->field_4 = 0;
        } else {
            ptr->field_4 = objectFindById(a2);
            if (ptr->field_4 == NULL) return -1;
        }

        if (fileReadInt32(stream, &a2) == -1) return -1;

        if (a2 == -1) {
            ptr->field_8 = 0;
        } else {
            ptr->field_8 = objectFindById(a2);
            if (ptr->field_8 == NULL) return -1;
        }

        if (fileReadInt32(stream, &(ptr->field_C)) == -1) return -1;
    }

    sub_421C8C(gDude);

    return 0;
}

// 0x421244
int combatSave(File* stream)
{
    if (fileWriteInt32(stream, gCombatState) == -1) return -1;

    if (!isInCombat()) return 0;

    if (fileWriteInt32(stream, dword_51093C) == -1) return -1;
    if (fileWriteInt32(stream, dword_56D39C) == -1) return -1;
    if (fileWriteInt32(stream, dword_56D398) == -1) return -1;
    if (fileWriteInt32(stream, dword_56D394) == -1) return -1;
    if (fileWriteInt32(stream, dword_56D384) == -1) return -1;
    if (fileWriteInt32(stream, dword_56D37C) == -1) return -1;
    if (fileWriteInt32(stream, gDude->cid) == -1) return -1;

    for (int index = 0; index < dword_56D37C; index++) {
        if (fileWriteInt32(stream, off_56D390[index]->cid) == -1) return -1;
    }

    if (off_510948 == NULL) {
        return -1;
    }

    for (int index = 0; index < dword_56D37C; index++) {
        STRUCT_510948* ptr = &(off_510948[index]);

        if (fileWriteInt32(stream, ptr->field_0 != NULL ? ptr->field_0->id : -1) == -1) return -1;
        if (fileWriteInt32(stream, ptr->field_4 != NULL ? ptr->field_4->id : -1) == -1) return -1;
        if (fileWriteInt32(stream, ptr->field_8 != NULL ? ptr->field_8->id : -1) == -1) return -1;
        if (fileWriteInt32(stream, ptr->field_C) == -1) return -1;
    }

    return 0;
}

// 0x4213E8
bool sub_4213E8(Object* a1, Object* a2, int hitMode, Object* a4, int* a5)
{
    return sub_4213FC(a1, a2, hitMode, a4, a5, NULL);
}

// 0x4213FC
bool sub_4213FC(Object* critter, Object* weapon, int hitMode, Object* a4, int* a5, Object* a6)
{
    if (a5 != NULL) {
        *a5 = 0;
    }

    if (critter->pid == PROTO_ID_0x10001E0) {
        return false;
    }

    int intelligence = critterGetStat(critter, STAT_INTELLIGENCE);
    int team = critter->data.critter.combat.team;
    int v41 = sub_47910C(weapon, hitMode);
    int maxDamage;
    weaponGetDamageMinMax(weapon, NULL, &maxDamage);
    int damageType = weaponGetDamageType(critter, weapon);

    if (v41 > 0) {
        if (intelligence < 5) {
            v41 -= 5 - intelligence;
            if (v41 < 0) {
                v41 = 0;
            }
        }

        if (a6 != NULL) {
            if (objectGetDistanceBetween(a4, a6) < v41) {
                debugPrint("Friendly was in the way!");
                return true;
            }
        }

        for (int index = 0; index < dword_56D37C; index++) {
            Object* candidate = off_56D390[index];
            if (candidate->data.critter.combat.team == team
                && candidate != critter
                && candidate != a4
                && !critterIsDead(candidate)) {
                int v14 = objectGetDistanceBetween(a4, candidate);
                if (v14 < v41 && candidate != candidate->data.critter.combat.whoHitMe) {
                    int damageThreshold = critterGetStat(candidate, STAT_DAMAGE_THRESHOLD + damageType);
                    int damageResistance = critterGetStat(candidate, STAT_DAMAGE_RESISTANCE + damageType);
                    if (damageResistance * (maxDamage - damageThreshold) / 100 > 0) {
                        return true;
                    }
                }
            }
        }

        int v17 = objectGetDistanceBetween(a4, critter);
        if (v17 <= v41) {
            if (a5 != NULL) {
                int v18 = objectGetDistanceBetween(a4, critter);
                *a5 = v41 - v18 + 1;
                return false;
            }

            return true;
        }

        return false;
    }

    int v19 = weaponGetAnimationForHitMode(weapon, hitMode);
    if (v19 != ANIM_FIRE_BURST && v19 != ANIM_FIRE_CONTINUOUS) {
        return false;
    }

    Attack attack;
    attackInit(&attack, critter, a4, hitMode, HIT_LOCATION_TORSO);

    int accuracy = attackDetermineToHit(critter, critter->tile, a4, HIT_LOCATION_TORSO, hitMode, 1);
    int v33;
    int a4a;
    sub_423488(&attack, accuracy, &v33, &a4a, v19);

    if (a6 != NULL) {
        for (int index = 0; index < attack.extrasLength; index++) {
            if (attack.extras[index] == a6) {
                debugPrint("Friendly was in the way!");
                return true;
            }
        }
    }

    for (int index = 0; index < attack.extrasLength; index++) {
        Object* candidate = attack.extras[index];
        if (candidate->data.critter.combat.team == team
            && candidate != critter
            && candidate != a4
            && !critterIsDead(candidate)
            && candidate != candidate->data.critter.combat.whoHitMe) {
            int damageThreshold = critterGetStat(candidate, STAT_DAMAGE_THRESHOLD + damageType);
            int damageResistance = critterGetStat(candidate, STAT_DAMAGE_RESISTANCE + damageType);
            if (damageResistance * (maxDamage - damageThreshold) / 100 > 0) {
                return true;
            }
        }
    }

    return false;
}

// 0x4217BC
bool sub_4217BC(Object* a1, Object* a2, Object* a3, Object* a4)
{
    return sub_4213FC(a1, a4, HIT_MODE_RIGHT_WEAPON_PRIMARY, a2, NULL, a3);
}

// 0x4217D4
Object* sub_4217D4()
{
    if (isInCombat()) {
        return off_56D388;
    } else {
        return NULL;
    }
}

// 0x4217E8
void sub_4217E8(Object* obj)
{
    obj->data.critter.combat.damageLastTurn = 0;
    obj->data.critter.combat.results = 0;
}

// 0x421850
int sub_421850(int a1, int a2)
{
    STRUCT_510948* v3;
    STRUCT_510948* v4;

    v3 = &off_510948[a1];
    v4 = &off_510948[a2];

    v4->field_0 = v3->field_0;
    v4->field_4 = v3->field_4;
    v4->field_8 = v3->field_8;
    v4->field_C = v3->field_C;

    return 0;
}

// 0x421880
Object* sub_421880(Object* obj)
{
    if (!isInCombat()) {
        return NULL;
    }

    if (obj == NULL) {
        return NULL;
    }

    if (obj->cid == -1) {
        return NULL;
    }

    return off_510948[obj->cid].field_0;
}

// 0x4218AC
int sub_4218AC(Object* a1, Object* a2)
{
    if (!isInCombat()) {
        return 0;
    }

    if (a1 == NULL) {
        return -1;
    }

    if (a1->cid == -1) {
        return -1;
    }

    if (a1 == a2) {
        return -1;
    }

    off_510948[a1->cid].field_0 = a2;

    return 0;
}

// 0x4218EC
Object* sub_4218EC(Object* obj)
{
    if (!isInCombat()) {
        return NULL;
    }

    if (obj == NULL) {
        return NULL;
    }

    if (obj->cid == -1) {
        return NULL;
    }

    return off_510948[obj->cid].field_4;
}

// 0x421918
int sub_421918(Object* a1, Object* a2)
{
    if (!isInCombat()) {
        return 0;
    }

    if (a1 == NULL) {
        return -1;
    }

    if (a1->cid == -1) {
        return -1;
    }

    if (a1 == a2) {
        return -1;
    }

    if (critterIsDead(a2)) {
        a2 = NULL;
    }

    off_510948[a1->cid].field_4 = a2;

    return 0;
}

// 0x42196C
Object* sub_42196C(Object* obj)
{
    int v1;

    if (!isInCombat()) {
        return NULL;
    }

    if (obj == NULL) {
        return NULL;
    }

    v1 = obj->cid;
    if (v1 == -1) {
        return NULL;
    }

    return off_510948[v1].field_8;
}

// 0x421998
int sub_421998(Object* obj, Object* a2)
{
    int v2;

    if (!isInCombat()) {
        return 0;
    }

    if (obj == NULL) {
        return -1;
    }

    v2 = obj->cid;
    if (v2 == -1) {
        return -1;
    }

    off_510948[v2].field_8 = NULL;

    return 0;
}

// 0x421A34
void sub_421A34(Object* a1)
{
    dword_51093C = 0;
    sub_4186CC();
    tickersRemove(sub_418168);
    dword_56D378 = gElevation;

    if (!isInCombat()) {
        dword_510940 = 0;
        dword_56D398 = 0;
        off_56D390 = NULL;
        dword_56D37C = objectListCreate(-1, dword_56D378, OBJ_TYPE_CRITTER, &off_56D390);
        dword_56D384 = dword_56D37C;
        dword_56D394 = 0;
        off_510948 = internal_malloc(sizeof(*off_510948) * dword_56D37C);
        if (off_510948 == NULL) {
            return;
        }

        for (int index = 0; index < dword_56D37C; index++) {
            STRUCT_510948* ptr = &(off_510948[index]);
            ptr->field_0 = NULL;
            ptr->field_4 = NULL;
            ptr->field_8 = NULL;
            ptr->field_C = 0;
        }

        Object* v1 = NULL;
        for (int index = 0; index < dword_56D37C; index++) {
            Object* critter = off_56D390[index];
            CritterCombatData* combatData = &(critter->data.critter.combat);
            combatData->maneuver &= 0x01;
            combatData->damageLastTurn = 0;
            combatData->whoHitMe = NULL;
            combatData->ap = 0;
            critter->cid = index;

            // NOTE: Not sure about this code, field_C is already reset.
            if (isInCombat() && critter != NULL && index != -1) {
                off_510948[index].field_C = 0;
            }

            scriptSetObjects(critter->sid, NULL, NULL);
            scriptSetFixedParam(critter->sid, 0);
            if (critter->pid == 0x1000098) {
                if (!critterIsDead(critter)) {
                    v1 = critter;
                }
            }
        }

        gCombatState |= COMBAT_STATE_0x01;

        tileWindowRefresh();
        gameUiDisable(0);
        gameMouseSetCursor(MOUSE_CURSOR_WAIT_WATCH);
        off_56D380 = NULL;
        sub_421C8C(a1);
        sub_429210(off_56D390, dword_56D37C);
        interfaceBarEndButtonsShow(true);
        sub_44B4CC();

        if (v1 != NULL && !sub_47DC60()) {
            int fid = buildFid((v1->fid & 0xF000000) >> 24,
                100,
                (v1->fid & 0xFF0000) >> 16,
                (v1->fid & 0xF000) >> 12,
                (v1->fid & 0x70000000) >> 28);

            reg_anim_clear(v1);
            reg_anim_begin(2);
            reg_anim_animate(v1, 6, -1);
            reg_anim_17(v1, fid, -1);
            reg_anim_end();

            while (animationIsBusy(v1)) {
                sub_4C8BDC();
            }
        }
    }
}

// 0x421C8C
void sub_421C8C(Object* a1)
{
    for (int index = 0; index < dword_56D37C; index++) {
        sub_421D50(off_56D390[index], 0);
    }

    attackInit(&stru_56D2B0, a1, NULL, 4, 3);

    off_56D388 = a1;

    sub_42AF78(dword_56D37C, off_56D390);

    dword_56D38C = 2;
    configGetInt(&gGameConfig, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_TARGET_HIGHLIGHT_KEY, &dword_56D38C);
}

// Something with outlining.
//
// 0x421D50
void sub_421D50(Object* critter, bool a2)
{
    if (critter->pid >> 24 != OBJ_TYPE_CRITTER) {
        return;
    }

    if (critter == gDude) {
        return;
    }

    if (critterIsDead(critter)) {
        return;
    }

    bool v5 = false;
    if (!sub_426CC4(gDude, gDude->tile, critter->tile, critter, 0)) {
        v5 = true;
    }

    if (v5) {
        int outlineType = critter->outline & OUTLINE_TYPE_MASK;
        if (outlineType != OUTLINE_TYPE_HOSTILE && outlineType != OUTLINE_TYPE_FRIENDLY) {
            int newOutlineType = gDude->data.critter.combat.team == critter->data.critter.combat.team
                ? OUTLINE_TYPE_FRIENDLY
                : OUTLINE_TYPE_HOSTILE;
            objectDisableOutline(critter, NULL);
            objectClearOutline(critter, NULL);
            objectSetOutline(critter, newOutlineType, NULL);
            if (a2) {
                objectEnableOutline(critter, NULL);
            } else {
                objectDisableOutline(critter, NULL);
            }
        } else {
            if (critter->outline != 0 && (critter->outline & OUTLINE_DISABLED) == 0) {
                if (!a2) {
                    objectDisableOutline(critter, NULL);
                }
            } else {
                if (a2) {
                    objectEnableOutline(critter, NULL);
                }
            }
        }
    } else {
        int v7 = objectGetDistanceBetween(gDude, critter);
        int v8 = critterGetStat(gDude, STAT_PERCEPTION) * 5;
        if ((critter->flags & 0x020000) != 0) {
            v8 /= 2;
        }

        if (v7 <= v8) {
            v5 = true;
        }

        int outlineType = critter->outline & OUTLINE_TYPE_MASK;
        if (outlineType != OUTLINE_TYPE_32) {
            objectDisableOutline(critter, NULL);
            objectClearOutline(critter, NULL);

            if (v5) {
                objectSetOutline(critter, OUTLINE_TYPE_32, NULL);

                if (a2) {
                    objectEnableOutline(critter, NULL);
                } else {
                    objectDisableOutline(critter, NULL);
                }
            }
        } else {
            if (critter->outline != 0 && (critter->outline & OUTLINE_DISABLED) == 0) {
                if (!a2) {
                    objectDisableOutline(critter, NULL);
                }
            } else {
                if (a2) {
                    objectEnableOutline(critter, NULL);
                }
            }
        }
    }
}

// Probably complete combat sequence.
//
// 0x421EFC
void sub_421EFC()
{
    if (dword_5186CC == 0) {
        for (int index = 0; index < dword_56D394; index++) {
            Object* critter = off_56D390[index];
            if (critter != gDude) {
                sub_42AECC(critter, 0);
            }
        }
    }

    tickersAdd(sub_418168);

    for (int index = 0; index < dword_56D384 + dword_56D394; index++) {
        Object* critter = off_56D390[index];
        critter->data.critter.combat.damageLastTurn = 0;
        critter->data.critter.combat.maneuver = 0;
    }

    for (int index = 0; index < dword_56D37C; index++) {
        Object* critter = off_56D390[index];
        critter->data.critter.combat.ap = 0;
        objectClearOutline(critter, NULL);
        critter->data.critter.combat.whoHitMe = NULL;

        scriptSetObjects(critter->sid, NULL, NULL);
        scriptSetFixedParam(critter->sid, 0);

        if (critter->pid == 0x1000098 && !critterIsDead(critter) && !sub_47DC60()) {
            int fid = buildFid((critter->fid & 0xF000000) >> 24,
                99,
                (critter->fid & 0xFF0000) >> 16,
                (critter->fid & 0xF000) >> 12,
                (critter->fid & 0x70000000) >> 28);
            reg_anim_clear(critter);
            reg_anim_begin(2);
            reg_anim_animate(critter, 6, -1);
            reg_anim_17(critter, fid, -1);
            reg_anim_end();

            while (animationIsBusy(critter)) {
                sub_4C8BDC();
            }
        }
    }

    tileWindowRefresh();

    int v13;
    int v12;
    sub_45F4B4(&v13, &v12);
    sub_45EFEC(true, v13, v12);

    gDude->data.critter.combat.ap = critterGetStat(gDude, STAT_MAXIMUM_HIT_POINTS);

    interfaceRenderActionPoints(0, 0);

    if (dword_5186CC == 0) {
        sub_4221B4(dword_56D398);
    }

    dword_56D398 = 0;

    gCombatState &= ~(COMBAT_STATE_0x01 | COMBAT_STATE_0x02);
    gCombatState |= COMBAT_STATE_0x02;

    if (dword_56D37C != 0) {
        objectListFree(off_56D390);

        if (off_510948 != NULL) {
            internal_free(off_510948);
        }
        off_510948 = NULL;
    }

    dword_56D37C = 0;

    sub_42AFBC();
    gameUiEnable();
    gameMouseSetMode(GAME_MOUSE_MODE_MOVE);
    interfaceRenderArmorClass(true);

    if (sub_42DD80(gDude) && !critterIsDead(gDude) && off_56D380 == NULL) {
        queueRemoveEventsByType(gDude, EVENT_TYPE_KNOCKOUT);
        knockoutEventProcess(gDude, NULL);
    }
}

// 0x422194
void sub_422194()
{
    sub_421EFC();
    gCombatState = 0;
    dword_517F98 = 1;
}

// Give exp for destroying critter.
//
// 0x4221B4
void sub_4221B4(int exp_points)
{
    MessageListItem v7;
    MessageListItem v9;
    int current_hp;
    int max_hp;
    char text[132];

    if (exp_points <= 0) {
        return;
    }

    if (critterIsDead(gDude)) {
        return;
    }

    pcAddExperience(exp_points);

    v7.num = 621; // %s you earn %d exp. points.
    if (!messageListGetItem(&gProtoMessageList, &v7)) {
        return;
    }

    v9.num = randomBetween(0, 3) + 622; // generate prefix for message

    current_hp = critterGetStat(gDude, STAT_CURRENT_HIT_POINTS);
    max_hp = critterGetStat(gDude, STAT_MAXIMUM_HIT_POINTS);
    if (current_hp == max_hp && randomBetween(0, 100) > 65) {
        v9.num = 626; // Best possible prefix: For destroying your enemies without taking a scratch,
    }

    if (!messageListGetItem(&gProtoMessageList, &v9)) {
        return;
    }

    sprintf(text, v7.text, v9.text, exp_points);
    displayMonitorAddMessage(text);
}

// 0x4222A8
void sub_4222A8()
{
    sub_42BCD4(gDude);

    for (int index = dword_56D394; index < dword_56D394 + dword_56D384; index++) {
        Object* obj = off_56D390[index];
        if (sub_42B3FC(obj)) {
            obj->data.critter.combat.maneuver = 0;

            Object** objectPtr1 = &(off_56D390[index]);
            Object** objectPtr2 = &(off_56D390[dword_56D394]);
            Object* t = *objectPtr1;
            *objectPtr1 = *objectPtr2;
            *objectPtr2 = t;

            dword_56D394 += 1;
            dword_56D384 -= 1;

            int actionPoints = 0;
            if (obj != gDude) {
                actionPoints = critterGetStat(obj, STAT_MAXIMUM_ACTION_POINTS);
            }

            if (off_51094C != NULL) {
                actionPoints += off_51094C->actionPointsBonus;
            }

            obj->data.critter.combat.ap = actionPoints;

            sub_42299C(obj, false);
        }
    }
}

// Compares critters by sequence.
//
// 0x4223C8
int sub_4223C8(const void* a1, const void* a2)
{
    Object* v1 = *(Object**)a1;
    Object* v2 = *(Object**)a2;

    int sequence1 = critterGetStat(v1, STAT_SEQUENCE);
    int sequence2 = critterGetStat(v2, STAT_SEQUENCE);
    if (sequence1 > sequence2) {
        return -1;
    } else if (sequence1 < sequence2) {
        return 1;
    }

    int luck1 = critterGetStat(v1, STAT_LUCK);
    int luck2 = critterGetStat(v2, STAT_LUCK);
    if (luck1 > luck2) {
        return -1;
    } else if (luck1 < luck2) {
        return 1;
    }

    return 0;
}

// 0x42243C
void sub_42243C(Object* a1, Object* a2)
{
    int next = 0;
    if (a1 != NULL) {
        for (int index = 0; index < dword_56D37C; index++) {
            Object* obj = off_56D390[index];
            if (obj == a1) {
                Object* temp = off_56D390[next];
                off_56D390[index] = temp;
                off_56D390[next] = obj;
                next += 1;
                break;
            }
        }
    }

    if (a2 != NULL) {
        for (int index = 0; index < dword_56D37C; index++) {
            Object* obj = off_56D390[index];
            if (obj == a2) {
                Object* temp = off_56D390[next];
                off_56D390[index] = temp;
                off_56D390[next] = obj;
                next += 1;
                break;
            }
        }
    }

    if (a1 != gDude && a2 != gDude) {
        for (int index = 0; index < dword_56D37C; index++) {
            Object* obj = off_56D390[index];
            if (obj == gDude) {
                Object* temp = off_56D390[next];
                off_56D390[index] = temp;
                off_56D390[next] = obj;
                next += 1;
                break;
            }
        }
    }

    dword_56D394 = next;
    dword_56D384 -= next;

    if (a1 != NULL) {
        sub_42E4C0(a1, a2);
    }

    if (a2 != NULL) {
        sub_42E4C0(a2, a1);
    }
}

// 0x422580
void sub_422580()
{
    sub_4222A8();

    int count = dword_56D394;

    for (int index = 0; index < count; index++) {
        Object* critter = off_56D390[index];
        if ((critter->data.critter.combat.results & DAM_DEAD) != 0) {
            off_56D390[index] = off_56D390[count - 1];
            off_56D390[count - 1] = critter;

            off_56D390[count - 1] = off_56D390[dword_56D384 + count - 1];
            off_56D390[dword_56D384 + count - 1] = critter;

            index -= 1;
            count -= 1;
        }
    }

    for (int index = 0; index < count; index++) {
        Object* critter = off_56D390[index];
        if (critter != gDude) {
            if ((critter->data.critter.combat.results & DAM_KNOCKED_OUT) != 0
                || critter->data.critter.combat.maneuver == CRITTER_MANEUVER_STOP_ATTACKING) {
                critter->data.critter.combat.maneuver &= ~CRITTER_MANEUVER_0x01;
                dword_56D384 += 1;

                off_56D390[index] = off_56D390[count - 1];
                off_56D390[count - 1] = critter;

                count -= 1;
                index -= 1;
            }
        }
    }

    if (count != 0) {
        dword_56D394 = count;
        qsort(off_56D390, count, sizeof(*off_56D390), sub_4223C8);
        count = dword_56D394;
    }

    dword_56D394 = count;

    gameTimeAddSeconds(5);
}

// 0x422694
void combatAttemptEnd()
{
    if (dword_56D378 == gDude->elevation) {
        MessageListItem messageListItem;
        int team = gDude->data.critter.combat.team;

        for (int index = 0; index < dword_56D394; index++) {
            Object* critter = off_56D390[index];
            if (critter != gDude) {
                int critterTeam = critter->data.critter.combat.team;
                Object* critterWhoHitMe = critter->data.critter.combat.whoHitMe;
                if (critterTeam != team || (critterWhoHitMe != NULL && critterWhoHitMe->data.critter.combat.team == team)) {
                    if (!sub_42B4A8(critter)) {
                        messageListItem.num = 103;
                        if (messageListGetItem(&gCombatMessageList, &messageListItem)) {
                            displayMonitorAddMessage(messageListItem.text);
                        }
                        return;
                    }
                }
            }
        }

        for (int index = dword_56D394; index < dword_56D394 + dword_56D384; index++) {
            Object* critter = off_56D390[index];
            if (critter != gDude) {
                int critterTeam = critter->data.critter.combat.team;
                Object* critterWhoHitMe = critter->data.critter.combat.whoHitMe;
                if (critterTeam == team || (critterWhoHitMe != NULL && critterWhoHitMe->data.critter.combat.team == critterTeam)) {
                    if (sub_42B3FC(critter)) {
                        messageListItem.num = 103;
                        if (messageListGetItem(&gCombatMessageList, &messageListItem)) {
                            displayMonitorAddMessage(messageListItem.text);
                        }
                        return;
                    }
                }
            }
        }
    }

    gCombatState |= COMBAT_STATE_0x08;
    sub_4292C0();
}

// 0x4227DC
void sub_4227DC()
{
    while (dword_51093C > 0) {
        sub_4C8BDC();
    }
}

// 0x4227F4
int sub_4227F4()
{
    while ((gCombatState & COMBAT_STATE_0x02) != 0) {
        if ((gCombatState & COMBAT_STATE_0x08) != 0) {
            break;
        }

        if ((gDude->data.critter.combat.results & (DAM_KNOCKED_OUT | DAM_DEAD | DAM_LOSE_TURN)) != 0) {
            break;
        }

        if (dword_5186CC != 0) {
            break;
        }

        if (dword_517F98 != 0) {
            break;
        }

        int keyCode = sub_4C8B78();
        if (sub_412CE4()) {
            while (dword_51093C > 0) {
                sub_4C8BDC();
            }
        }

        if (gDude->data.critter.combat.ap <= 0 && dword_56D39C <= 0) {
            break;
        }

        if (keyCode == KEY_SPACE) {
            break;
        }

        if (keyCode == KEY_RETURN) {
            combatAttemptEnd();
        } else {
            sub_4A43A0();
            gameHandleKey(keyCode, true);
        }
    }

    int v4 = dword_5186CC;
    if (dword_5186CC == 1) {
        dword_5186CC = 0;
    }

    if ((gCombatState & COMBAT_STATE_0x08) != 0) {
        gCombatState &= ~COMBAT_STATE_0x08;
        return -1;
    }

    if (dword_5186CC != 0 || v4 != 0 || dword_517F98 != 0) {
        return -1;
    }

    sub_4A43A0();

    return 0;
}

// 0x422914
void sub_422914()
{
    for (int index = 0; index < dword_56D394; index++) {
        Object* object = off_56D390[index];

        int actionPoints = critterGetStat(object, STAT_MAXIMUM_ACTION_POINTS);

        if (off_51094C) {
            actionPoints += off_51094C->actionPointsBonus;
        }

        object->data.critter.combat.ap = actionPoints;

        if (isInCombat()) {
            if (object->cid != -1) {
                off_510948[object->cid].field_C = 0;
            }
        }
    }
}

// 0x42299C
int sub_42299C(Object* a1, bool a2)
{
    off_56D388 = a1;

    attackInit(&stru_56D2B0, a1, NULL, HIT_MODE_PUNCH, HIT_LOCATION_TORSO);

    if ((a1->data.critter.combat.results & (DAM_KNOCKED_OUT | DAM_DEAD | DAM_LOSE_TURN)) != 0) {
        a1->data.critter.combat.results &= ~DAM_LOSE_TURN;
    } else {
        if (a1 == gDude) {
            keyboardReset();
            interfaceRenderArmorClass(true);
            dword_56D39C = 2 * perkGetRank(gDude, PERK_BONUS_MOVE);
            interfaceRenderActionPoints(gDude->data.critter.combat.ap, dword_56D39C);
        } else {
            soundContinueAll();
        }

        bool scriptOverrides = false;
        if (a1->sid != -1) {
            scriptSetObjects(a1->sid, NULL, NULL);
            scriptSetFixedParam(a1->sid, 4);
            scriptExecProc(a1->sid, SCRIPT_PROC_COMBAT);

            Script* scr;
            if (scriptGetScript(a1->sid, &scr) != -1) {
                scriptOverrides = scr->scriptOverrides;
            }

            if (dword_5186CC == 1) {
                return -1;
            }
        }

        if (!scriptOverrides) {
            if (!a2 && sub_42DD80(a1)) {
                sub_425FBC(a1);
            }

            if (a1 == gDude) {
                gameUiEnable();
                sub_44CBD0();

                if (off_51094C != NULL) {
                    sub_4267CC(off_51094C->defender);
                }

                if (!a2) {
                    gCombatState |= 0x02;
                }

                interfaceBarEndButtonsRenderGreenLights();

                for (int index = 0; index < dword_56D37C; index++) {
                    sub_421D50(off_56D390[index], false);
                }

                if (dword_56D38C != 0) {
                    sub_426AA8();
                }

                if (sub_4227F4() == -1) {
                    gameUiDisable(1);
                    gameMouseSetCursor(MOUSE_CURSOR_WAIT_WATCH);
                    a1->data.critter.combat.damageLastTurn = 0;
                    interfaceBarEndButtonsRenderRedLights();
                    sub_426BC0();
                    interfaceRenderActionPoints(-1, -1);
                    interfaceRenderArmorClass(true);
                    dword_56D39C = 0;
                    return -1;
                }
            } else {
                Rect rect;
                if (objectEnableOutline(a1, &rect) == 0) {
                    tileWindowRefreshRect(&rect, a1->elevation);
                }

                sub_42B130(a1, off_51094C != NULL ? off_51094C->defender : NULL);
            }
        }

        while (dword_51093C > 0) {
            sub_4C8BDC();
        }

        if (a1 == gDude) {
            gameUiDisable(1);
            gameMouseSetCursor(MOUSE_CURSOR_WAIT_WATCH);
            interfaceBarEndButtonsRenderRedLights();
            sub_426BC0();
            interfaceRenderActionPoints(-1, -1);
            off_56D388 = NULL;
            interfaceRenderArmorClass(true);
            off_56D388 = gDude;
        } else {
            Rect rect;
            if (objectDisableOutline(a1, &rect) == 0) {
                tileWindowRefreshRect(&rect, a1->elevation);
            }
        }
    }

    if ((gDude->data.critter.combat.results & DAM_DEAD) != 0) {
        return -1;
    }

    if (a1 != gDude || dword_56D378 == gDude->elevation) {
        dword_56D39C = 0;
        return 0;
    }

    return -1;
}

// 0x422C60
bool sub_422C60()
{
    if (dword_56D394 <= 1) {
        return true;
    }

    int index;
    for (index = 0; index < dword_56D394; index++) {
        if (off_56D390[index] == gDude) {
            break;
        }
    }

    if (index == dword_56D394) {
        return true;
    }

    int team = gDude->data.critter.combat.team;

    for (index = 0; index < dword_56D394; index++) {
        Object* critter = off_56D390[index];
        if (critter->data.critter.combat.team != team) {
            break;
        }

        Object* critterWhoHitMe = critter->data.critter.combat.whoHitMe;
        if (critterWhoHitMe != NULL && critterWhoHitMe->data.critter.combat.team == team) {
            break;
        }
    }

    if (index == dword_56D394) {
        return true;
    }

    return false;
}

// 0x422D2C
void sub_422D2C(STRUCT_664980* attack)
{
    if (attack == NULL
        || (attack->attacker == NULL || attack->attacker->elevation == gElevation)
        || (attack->defender == NULL || attack->defender->elevation == gElevation)) {
        int v3 = gCombatState & 0x01;

        sub_421A34(NULL);

        int v6;

        // TODO: Not sure.
        if (v3 != 0) {
            if (sub_42299C(gDude, true) == -1) {
                v6 = -1;
            } else {
                int index;
                for (index = 0; index < dword_56D394; index++) {
                    if (off_56D390[index] == gDude) {
                        break;
                    }
                }
                v6 = index + 1;
            }
            off_51094C = NULL;
        } else {
            Object* v3;
            Object* v9;
            if (attack != NULL) {
                v3 = attack->defender;
                v9 = attack->attacker;
            } else {
                v3 = NULL;
                v9 = NULL;
            }
            sub_42243C(v9, v3);
            off_51094C = attack;
            v6 = 0;
        }

        do {
            if (v6 == -1) {
                break;
            }

            sub_422914();

            for (; v6 < dword_56D394; v6++) {
                if (sub_42299C(off_56D390[v6], false) == -1) {
                    break;
                }

                if (off_56D380 != NULL) {
                    break;
                }

                off_51094C = NULL;
            }

            if (v6 < dword_56D394) {
                break;
            }

            sub_422580();
            v6 = 0;
            dword_510940 += 1;
        } while (!sub_422C60());

        if (dword_517F98) {
            gameUiEnable();
            gameMouseSetMode(GAME_MOUSE_MODE_MOVE);
        } else {
            sub_44B4D8();
            interfaceBarEndButtonsHide(true);
            sub_44B4CC();
            sub_421EFC();
            scriptsExecMapUpdateProc();
        }

        dword_517F98 = 0;

        if (dword_5186CC == 1) {
            dword_5186CC = 0;
        }
    }
}

// 0x422EC4
void attackInit(Attack* attack, Object* attacker, Object* defender, int hitMode, int hitLocation)
{
    attack->attacker = attacker;
    attack->hitMode = hitMode;
    attack->weapon = critterGetWeaponForHitMode(attacker, hitMode);
    attack->attackHitLocation = HIT_LOCATION_TORSO;
    attack->attackerDamage = 0;
    attack->attackerFlags = 0;
    attack->ammoQuantity = 0;
    attack->criticalMessageId = -1;
    attack->defender = defender;
    attack->tile = defender != NULL ? defender->tile : -1;
    attack->defenderHitLocation = hitLocation;
    attack->defenderDamage = 0;
    attack->defenderFlags = 0;
    attack->defenderKnockback = 0;
    attack->extrasLength = 0;
    attack->oops = defender;
}

// 0x422F3C
int sub_422F3C(Object* a1, Object* a2, int hitMode, int hitLocation)
{
    if (a1 != gDude && hitMode == HIT_MODE_PUNCH && randomBetween(1, 4) == 1) {
        int fid = buildFid(1, a1->fid & 0xFFF, ANIM_KICK_LEG, (a1->fid & 0xF000) >> 12, (a1->fid & 0x70000000) >> 28);
        if (artExists(fid)) {
            hitMode = HIT_MODE_KICK;
        }
    }

    attackInit(&stru_56D2B0, a1, a2, hitMode, hitLocation);
    debugPrint("computing attack...\n");

    if (attackCompute(&stru_56D2B0) == -1) {
        return -1;
    }

    if (off_51094C != NULL) {
        stru_56D2B0.defenderDamage += off_51094C->damageBonus;

        if (stru_56D2B0.defenderDamage < off_51094C->minDamage) {
            stru_56D2B0.defenderDamage = off_51094C->minDamage;
        }

        if (stru_56D2B0.defenderDamage > off_51094C->maxDamage) {
            stru_56D2B0.defenderDamage = off_51094C->maxDamage;
        }

        if (off_51094C->field_1C) {
            // FIXME: looks like a bug, two different fields are used to set
            // one field.
            stru_56D2B0.defenderFlags = off_51094C->field_20;
            stru_56D2B0.defenderFlags = off_51094C->field_24;
        }
    }

    bool aiming;
    if (stru_56D2B0.defenderHitLocation == HIT_LOCATION_TORSO || stru_56D2B0.defenderHitLocation == HIT_LOCATION_UNCALLED) {
        if (a1 == gDude) {
            interfaceGetCurrentHitMode(&hitMode, &aiming);
        } else {
            aiming = false;
        }
    } else {
        aiming = true;
    }

    int actionPoints = sub_478B24(a1, stru_56D2B0.hitMode, aiming);
    debugPrint("sequencing attack...\n");

    if (sub_411224(&stru_56D2B0) == -1) {
        return -1;
    }

    if (actionPoints > a1->data.critter.combat.ap) {
        a1->data.critter.combat.ap = 0;
    } else {
        a1->data.critter.combat.ap -= actionPoints;
    }

    if (a1 == gDude) {
        interfaceRenderActionPoints(a1->data.critter.combat.ap, dword_56D39C);
        sub_42E4C0(a1, a2);
    }

    dword_510950 = 1;
    dword_517F9C = 1;
    sub_421918(a1, a2);
    debugPrint("running attack...\n");

    return 0;
}

// Returns tile one step closer from [a1] to [a2]
//
// 0x423104
int sub_423104(const Object* a1, const Object* a2)
{
    int rotation = tileGetRotationTo(a1->tile, a2->tile);
    return tileGetTileInDirection(a1->tile, rotation, 1);
}

// 0x423128
bool sub_423128(Attack* attack)
{
    int range = sub_478A1C(attack->attacker, attack->hitMode);
    int to = sub_4B1B84(attack->attacker->tile, attack->defender->tile, range);

    int roll = ROLL_FAILURE;
    Object* critter = attack->attacker;
    if (critter != NULL) {
        int curr = attack->attacker->tile;
        while (curr != to) {
            sub_4163C8(attack->attacker, curr, to, NULL, &critter, 32, sub_48B930);
            if (critter != NULL) {
                if ((critter->flags & 0x80000000) == 0) {
                    if ((critter->fid & 0xF000000) >> 24 != OBJ_TYPE_CRITTER) {
                        roll = ROLL_SUCCESS;
                        break;
                    }

                    if (critter != attack->defender) {
                        int v6 = attackDetermineToHit(attack->attacker, attack->attacker->tile, critter, attack->defenderHitLocation, attack->hitMode, 1) / 3;
                        if (critterIsDead(critter)) {
                            v6 = 5;
                        }

                        if (randomBetween(1, 100) <= v6) {
                            roll = ROLL_SUCCESS;
                            break;
                        }
                    }

                    curr = critter->tile;
                }
            }

            if (critter == NULL) {
                break;
            }
        }
    }

    attack->defenderHitLocation = HIT_LOCATION_TORSO;

    if (roll < ROLL_SUCCESS || critter == NULL || (critter->flags & 0x80000000) == 0) {
        return false;
    }

    attack->defender = critter;
    attack->tile = critter->tile;
    attack->attackerFlags |= DAM_HIT;
    attack->defenderHitLocation = HIT_LOCATION_TORSO;
    attackComputeDamage(attack, 1, 2);
    return true;
}

// 0x423284
int sub_423284(Attack* attack, int a2, int a3, int anim)
{
    int v5 = a3;
    int v17 = 0;
    int v7 = attack->attacker->tile;

    Object* critter = attack->attacker;
    while (critter != NULL) {
        if (v5 <= 0 && anim != ANIM_FIRE_CONTINUOUS || v7 == a2 || attack->extrasLength >= 6) {
            break;
        }

        sub_4163C8(attack->attacker, v7, a2, NULL, &critter, 32, sub_48B930);

        if (critter != NULL) {
            if (((critter->fid & 0xF000000) >> 24) != OBJ_TYPE_CRITTER) {
                break;
            }

            int v8 = attackDetermineToHit(attack->attacker, attack->attacker->tile, critter, HIT_LOCATION_TORSO, attack->hitMode, 1);
            if (anim == ANIM_FIRE_CONTINUOUS) {
                v5 = 1;
            }

            int a2a = 0;
            while (randomBetween(1, 100) <= v8 && v5 > 0) {
                v5 -= 1;
                a2a += 1;
            }

            if (a2a != 0) {
                if (critter == attack->defender) {
                    v17 += a2a;
                } else {
                    int index;
                    for (index = 0; index < attack->extrasLength; index += 1) {
                        if (critter == attack->extras[index]) {
                            break;
                        }
                    }

                    attack->extrasHitLocation[index] = HIT_LOCATION_TORSO;
                    attack->extras[index] = critter;
                    attackInit(&stru_56D3A0, attack->attacker, critter, attack->hitMode, HIT_LOCATION_TORSO);
                    stru_56D3A0.attackerFlags |= DAM_HIT;
                    attackComputeDamage(&stru_56D3A0, a2a, 2);

                    if (index == attack->extrasLength) {
                        attack->extrasDamage[index] = stru_56D3A0.defenderDamage;
                        attack->extrasFlags[index] = stru_56D3A0.defenderFlags;
                        attack->extrasKnockback[index] = stru_56D3A0.defenderKnockback;
                    } else {
                        if (anim == ANIM_FIRE_BURST) {
                            attack->extrasDamage[index] += stru_56D3A0.defenderDamage;
                            attack->extrasFlags[index] |= stru_56D3A0.defenderFlags;
                            attack->extrasKnockback[index] += stru_56D3A0.defenderKnockback;
                        }
                    }
                }
            }

            v7 = critter->tile;
        }
    }

    if (anim == ANIM_FIRE_CONTINUOUS) {
        v17 = 0;
    }

    return v17;
}

// 0x423488
int sub_423488(Attack* attack, int accuracy, int* a3, int* a4, int anim)
{
    *a3 = 0;

    int ammoQuantity = ammoGetQuantity(attack->weapon);
    int burstRounds = weaponGetBurstRounds(attack->weapon);
    if (burstRounds < ammoQuantity) {
        ammoQuantity = burstRounds;
    }

    *a4 = ammoQuantity;

    int criticalChance = critterGetStat(attack->attacker, STAT_CRITICAL_CHANCE);
    int roll = randomRoll(accuracy, criticalChance, NULL);

    if (roll == ROLL_CRITICAL_FAILURE) {
        return roll;
    }

    if (roll == ROLL_CRITICAL_SUCCESS) {
        accuracy += 20;
    }

    int v31;
    int v14;
    int v33;
    int v30;
    if (anim == ANIM_FIRE_BURST) {
        v33 = ammoQuantity / 3;
        if (v33 == 0) {
            v33 = 1;
        }

        v31 = ammoQuantity / 3;
        v30 = ammoQuantity - v33 - v31;
        v14 = v33 / 2;
        if (v14 == 0) {
            v14 = 1;
            v33 -= 1;
        }
    } else {
        v31 = 1;
        v14 = 1;
        v33 = 1;
        v30 = 1;
    }

    for (int index = 0; index < v14; index += 1) {
        if (randomRoll(accuracy, 0, NULL) >= ROLL_SUCCESS) {
            *a3 += 1;
        }
    }

    if (*a3 == 0 && sub_423128(attack)) {
        *a3 = 1;
    }

    int range = sub_478A1C(attack->attacker, attack->hitMode);
    int v19 = sub_4B1B84(attack->attacker->tile, attack->defender->tile, range);

    *a3 += sub_423284(attack, v19, v33 - *a3, anim);

    int v20;
    if (objectGetDistanceBetween(attack->attacker, attack->defender) <= 3) {
        v20 = sub_4B1B84(attack->attacker->tile, attack->defender->tile, 3);
    } else {
        v20 = attack->defender->tile;
    }

    int rotation = tileGetRotationTo(v20, attack->attacker->tile);
    int v23 = tileGetTileInDirection(v20, (rotation + 1) % ROTATION_COUNT, 1);

    int v25 = sub_4B1B84(attack->attacker->tile, v23, range);

    *a3 += sub_423284(attack, v25, v31, anim);

    int v26 = tileGetTileInDirection(v20, (rotation + 5) % ROTATION_COUNT, 1);

    int v28 = sub_4B1B84(attack->attacker->tile, v26, range);
    *a3 += sub_423284(attack, v28, v30, anim);

    if (roll != ROLL_FAILURE || *a3 <= 0 && attack->extrasLength <= 0) {
        if (roll >= ROLL_SUCCESS && *a3 == 0 && attack->extrasLength == 0) {
            roll = ROLL_FAILURE;
        }
    } else {
        roll = ROLL_SUCCESS;
    }

    return roll;
}

// 0x423714
int attackComputeEnhancedKnockout(Attack* attack)
{
    if (weaponGetPerk(attack->weapon) == PERK_WEAPON_ENHANCED_KNOCKOUT) {
        int difficulty = critterGetStat(attack->attacker, STAT_STRENGTH) - 8;
        int chance = randomBetween(1, 100);
        if (chance <= difficulty) {
            Object* weapon = NULL;
            if (attack->defender != gDude) {
                weapon = critterGetWeaponForHitMode(attack->defender, HIT_MODE_RIGHT_WEAPON_PRIMARY);
            }

            if (!(sub_424088(attack->defender, weapon) & 1)) {
                attack->defenderFlags |= DAM_KNOCKED_OUT;
            }
        }
    }

    return 0;
}

// 0x42378C
int attackCompute(Attack* attack)
{
    int range = sub_478A1C(attack->attacker, attack->hitMode);
    int distance = objectGetDistanceBetween(attack->attacker, attack->defender);

    if (range < distance) {
        return -1;
    }

    int anim = critterGetAnimationForHitMode(attack->attacker, attack->hitMode);
    int accuracy = attackDetermineToHit(attack->attacker, attack->attacker->tile, attack->defender, attack->defenderHitLocation, attack->hitMode, 1);

    bool isGrenade = false;
    int damageType = weaponGetDamageType(attack->attacker, attack->weapon);
    if (anim == ANIM_THROW_ANIM && (damageType == DAMAGE_TYPE_EXPLOSION || damageType == DAMAGE_TYPE_PLASMA || damageType == DAMAGE_TYPE_EMP)) {
        isGrenade = true;
    }

    if (attack->defenderHitLocation == HIT_LOCATION_UNCALLED) {
        attack->defenderHitLocation = HIT_LOCATION_TORSO;
    }

    int attackType = weaponGetAttackTypeForHitMode(attack->weapon, attack->hitMode);
    int ammoQuantity = 1;
    int damageMultiplier = 2;
    int v26 = 1;

    int roll;

    if (anim == ANIM_FIRE_BURST || anim == ANIM_FIRE_CONTINUOUS) {
        roll = sub_423488(attack, accuracy, &ammoQuantity, &v26, anim);
    } else {
        int chance = critterGetStat(attack->attacker, STAT_CRITICAL_CHANCE);
        roll = randomRoll(accuracy, chance - dword_510954[attack->defenderHitLocation], NULL);
    }

    if (roll == ROLL_FAILURE) {
        if (traitIsSelected(TRAIT_JINXED) || perkHasRank(gDude, PERK_JINXED)) {
            if (randomBetween(0, 1) == 1) {
                roll = ROLL_CRITICAL_FAILURE;
            }
        }
    }

    if (roll == ROLL_SUCCESS) {
        if ((attackType == ATTACK_TYPE_MELEE || attackType == ATTACK_TYPE_UNARMED) && attack->attacker == gDude) {
            if (perkHasRank(attack->attacker, PERK_SLAYER)) {
                roll = ROLL_CRITICAL_SUCCESS;
            }

            if (perkHasRank(gDude, PERK_SILENT_DEATH)
                && !sub_412BC4(gDude, attack->defender)
                && dudeHasState(DUDE_STATE_SNEAKING)
                && gDude != attack->defender->data.critter.combat.whoHitMe) {
                damageMultiplier = 4;
            }

            if (((attack->hitMode == HIT_MODE_HAMMER_PUNCH || attack->hitMode == HIT_MODE_POWER_KICK) && randomBetween(1, 100) <= 5)
                || ((attack->hitMode == HIT_MODE_JAB || attack->hitMode == HIT_MODE_HOOK_KICK) && randomBetween(1, 100) <= 10)
                || (attack->hitMode == HIT_MODE_HAYMAKER && randomBetween(1, 100) <= 15)
                || (attack->hitMode == HIT_MODE_PALM_STRIKE && randomBetween(1, 100) <= 20)
                || (attack->hitMode == HIT_MODE_PIERCING_STRIKE && randomBetween(1, 100) <= 40)
                || (attack->hitMode == HIT_MODE_PIERCING_KICK && randomBetween(1, 100) <= 50)) {
                roll = ROLL_CRITICAL_SUCCESS;
            }
        }
    }

    if (attackType == ATTACK_TYPE_RANGED) {
        attack->ammoQuantity = v26;

        if (roll == ROLL_SUCCESS && attack->attacker == gDude) {
            if (perkGetRank(gDude, PERK_SNIPER) != 0) {
                int d10 = randomBetween(1, 10);
                int luck = critterGetStat(gDude, STAT_LUCK);
                if (d10 <= luck) {
                    roll = ROLL_CRITICAL_SUCCESS;
                }
            }
        }
    } else {
        if (ammoGetCapacity(attack->weapon) > 0) {
            attack->ammoQuantity = 1;
        }
    }

    if (sub_4790AC(attack->weapon, &(attack->ammoQuantity)) == -1) {
        return -1;
    }

    switch (roll) {
    case ROLL_CRITICAL_SUCCESS:
        damageMultiplier = attackComputeCriticalHit(attack);
        // FALLTHROUGH
    case ROLL_SUCCESS:
        attack->attackerFlags |= DAM_HIT;
        attackComputeEnhancedKnockout(attack);
        attackComputeDamage(attack, ammoQuantity, damageMultiplier);
        break;
    case ROLL_FAILURE:
        if (attackType == ATTACK_TYPE_RANGED || attackType == ATTACK_TYPE_THROW) {
            sub_423128(attack);
        }
        break;
    case ROLL_CRITICAL_FAILURE:
        attackComputeCriticalFailure(attack);
        break;
    }

    if (attackType == ATTACK_TYPE_RANGED || attackType == ATTACK_TYPE_THROW) {
        if ((attack->attackerFlags & (DAM_HIT | DAM_CRITICAL)) == 0) {
            int tile;
            if (isGrenade) {
                int throwDistance = randomBetween(1, distance / 2);
                if (throwDistance == 0) {
                    throwDistance = 1;
                }

                int rotation = randomBetween(0, 5);
                tile = tileGetTileInDirection(attack->defender->tile, rotation, throwDistance);
            } else {
                tile = sub_4B1B84(attack->attacker->tile, attack->defender->tile, range);
            }

            attack->tile = tile;

            Object* v25 = attack->defender;
            sub_4163C8(v25, attack->defender->tile, attack->tile, NULL, &v25, 32, sub_48B930);
            if (v25 != NULL && v25 != attack->defender) {
                attack->tile = v25->tile;
            } else {
                v25 = sub_48B848(NULL, attack->tile, attack->defender->elevation);
            }

            if (v25 != NULL && (v25->flags & 0x80000000) == 0) {
                attack->attackerFlags |= DAM_HIT;
                attack->defender = v25;
                attackComputeDamage(attack, 1, 2);
            }
        }
    }

    if ((damageType == DAMAGE_TYPE_EXPLOSION || isGrenade) && ((attack->attackerFlags & DAM_HIT) != 0 || (attack->attackerFlags & DAM_CRITICAL) == 0)) {
        sub_423C10(attack, 0, isGrenade, 0);
    } else {
        if ((attack->attackerFlags & DAM_EXPLODE) != 0) {
            sub_423C10(attack, 1, isGrenade, 0);
        }
    }

    attackComputeDeathFlags(attack);

    return 0;
}

// compute_explosion_on_extras
// 0x423C10
void sub_423C10(Attack* attack, int a2, int a3, int a4)
{
    Object* attacker;

    if (a2) {
        attacker = attack->attacker;
    } else {
        if ((attack->attackerFlags & DAM_HIT) != 0) {
            attacker = attack->defender;
        } else {
            attacker = NULL;
        }
    }

    int tile;
    if (attacker != NULL) {
        tile = attacker->tile;
    } else {
        tile = attack->tile;
    }

    if (tile == -1) {
        debugPrint("\nError: compute_explosion_on_extras: Called with bad target/tileNum");
        return;
    }

    // TODO: The math in this loop is rather complex and hard to understand.
    int v20;
    int v22 = 0;
    int rotation = 0;
    int v5 = -1;
    int v19 = tile;
    while (attack->extrasLength < 6) {
        if (v22 != 0 && (v5 == -1 || (v5 = tileGetTileInDirection(v5, rotation, 1)) != v19)) {
            v20++;
            if (v20 % v22 == 0) {
                rotation += 1;
                if (rotation == ROTATION_COUNT) {
                    rotation = ROTATION_NE;
                }
            }
        } else {
            v22++;
            if (a3 && sub_479180(attack->weapon) < v22) {
                v5 = -1;
            } else if (a3 || sub_479188(attack->weapon) >= v22) {
                v5 = tileGetTileInDirection(v19, ROTATION_NE, 1);
            } else {
                v5 = -1;
            }

            v19 = v5;
            rotation = ROTATION_SE;
            v20 = 0;
        }

        if (v5 == -1) {
            break;
        }

        Object* v11 = sub_48B848(attacker, v5, attack->attacker->elevation);
        if (v11 != NULL
            && (v11->fid & 0xF000000) >> 24 == OBJ_TYPE_CRITTER
            && (v11->data.critter.combat.results & DAM_DEAD) == 0
            && (v11->flags & 0x80000000)
            && !sub_426CC4(v11, v11->tile, tile, NULL, NULL)) {
            if (v11 == attack->attacker) {
                attack->attackerFlags &= ~DAM_HIT;
                attackComputeDamage(attack, 1, 2);
                attack->attackerFlags |= DAM_HIT;
                attack->attackerFlags |= DAM_BACKWASH;
            } else {
                int index;
                for (index = 0; index < attack->extrasLength; index++) {
                    if (attack->extras[index] == v11) {
                        break;
                    }
                }

                attack->extrasHitLocation[index] = HIT_LOCATION_TORSO;
                attack->extras[index] = v11;
                attackInit(&stru_56D458, attack->attacker, v11, attack->hitMode, HIT_LOCATION_TORSO);
                if (!a4) {
                    stru_56D458.attackerFlags |= DAM_HIT;
                    attackComputeDamage(&stru_56D458, 1, 2);
                }

                attack->extrasDamage[index] = stru_56D458.defenderDamage;
                attack->extrasFlags[index] = stru_56D458.defenderFlags;
                attack->extrasKnockback[index] = stru_56D458.defenderKnockback;
                attack->extrasLength += 1;
            }
        }
    }
}

// 0x423EB4
int attackComputeCriticalHit(Attack* attack)
{
    Object* defender = attack->defender;
    if (defender != NULL && sub_42E6AC(defender->pid, 1024)) {
        return 2;
    }

    if (defender != NULL && (defender->pid >> 24) != OBJ_TYPE_CRITTER) {
        return 2;
    }

    attack->attackerFlags |= DAM_CRITICAL;

    int chance = randomBetween(1, 100);

    chance += critterGetStat(attack->attacker, STAT_BETTER_CRITICALS);

    int effect;
    if (chance <= 20)
        effect = 0;
    else if (chance <= 45)
        effect = 1;
    else if (chance <= 70)
        effect = 2;
    else if (chance <= 90)
        effect = 3;
    else if (chance <= 100)
        effect = 4;
    else
        effect = 5;

    CriticalHitDescription* criticalHitDescription;
    if (defender == gDude) {
        criticalHitDescription = &(gPlayerCriticalHitTable[attack->defenderHitLocation][effect]);
    } else {
        int killType = critterGetKillType(defender);
        criticalHitDescription = &(gCriticalHitTables[killType][attack->defenderHitLocation][effect]);
    }

    attack->defenderFlags |= criticalHitDescription->flags;

    // NOTE: Original code is slightly different, it does not set message in
    // advance, instead using "else" statement.
    attack->criticalMessageId = criticalHitDescription->messageId;

    if (criticalHitDescription->massiveCriticalStat != -1) {
        if (statRoll(defender, criticalHitDescription->massiveCriticalStat, criticalHitDescription->massiveCriticalStatModifier, NULL) <= ROLL_FAILURE) {
            attack->defenderFlags |= criticalHitDescription->massiveCriticalFlags;
            attack->criticalMessageId = criticalHitDescription->massiveCriticalMessageId;
        }
    }

    if ((attack->defenderFlags & DAM_CRIP_RANDOM) != 0) {
        attack->defenderFlags &= ~DAM_CRIP_RANDOM;

        switch (randomBetween(0, 3)) {
        case 0:
            attack->defenderFlags |= DAM_CRIP_LEG_LEFT;
            break;
        case 1:
            attack->defenderFlags |= DAM_CRIP_LEG_RIGHT;
            break;
        case 2:
            attack->defenderFlags |= DAM_CRIP_ARM_LEFT;
            break;
        case 3:
            attack->defenderFlags |= DAM_CRIP_ARM_RIGHT;
            break;
        }
    }

    if (weaponGetPerk(attack->weapon) == PERK_WEAPON_ENHANCED_KNOCKOUT) {
        attack->defenderFlags |= DAM_KNOCKED_OUT;
    }

    Object* weapon = NULL;
    if (defender != gDude) {
        weapon = critterGetWeaponForHitMode(defender, HIT_MODE_RIGHT_WEAPON_PRIMARY);
    }

    int flags = sub_424088(defender, weapon);
    attack->defenderFlags &= ~flags;

    return criticalHitDescription->damageMultiplier;
}

// 0x424088
int sub_424088(Object* critter, Object* item)
{
    int flags = 0;

    if (critter != NULL && (critter->pid >> 24) == OBJ_TYPE_CRITTER && sub_42E6AC(critter->pid, 64)) {
        flags |= DAM_DROP;
    }

    if (item != NULL && weaponIsNatural(item)) {
        flags |= DAM_DROP;
    }

    return flags;
}

// 0x4240DC
int attackComputeCriticalFailure(Attack* attack)
{
    attack->attackerFlags |= DAM_HIT;

    if (attack->attacker != NULL && sub_42E6AC(attack->attacker->pid, 1024)) {
        return 0;
    }

    if (attack->attacker == gDude) {
        unsigned int gameTime = gameTimeGetTime();
        if (gameTime / GAME_TIME_TICKS_PER_DAY < 6) {
            return 0;
        }
    }

    int attackType = weaponGetAttackTypeForHitMode(attack->weapon, attack->hitMode);
    int criticalFailureTableIndex = weaponGetCriticalFailureType(attack->weapon);
    if (criticalFailureTableIndex == -1) {
        criticalFailureTableIndex = 0;
    }

    int chance = randomBetween(1, 100) - 5 * (critterGetStat(attack->attacker, STAT_LUCK) - 5);

    int effect;
    if (chance <= 20)
        effect = 0;
    else if (chance <= 50)
        effect = 1;
    else if (chance <= 75)
        effect = 2;
    else if (chance <= 95)
        effect = 3;
    else
        effect = 4;

    int flags = dword_517FA0[criticalFailureTableIndex][effect];
    if (flags == 0) {
        return 0;
    }

    attack->attackerFlags |= DAM_CRITICAL;
    attack->attackerFlags |= flags;

    int v17 = sub_424088(attack->attacker, attack->weapon);
    attack->attackerFlags &= ~v17;

    if ((attack->attackerFlags & DAM_HIT_SELF) != 0) {
        int ammoQuantity = attackType == ATTACK_TYPE_RANGED ? attack->ammoQuantity : 1;
        attackComputeDamage(attack, ammoQuantity, 2);
    } else if ((attack->attackerFlags & DAM_EXPLODE) != 0) {
        attackComputeDamage(attack, 1, 2);
    }

    if ((attack->attackerFlags & DAM_LOSE_TURN) != 0) {
        attack->attacker->data.critter.combat.ap = 0;
    }

    if ((attack->attackerFlags & DAM_LOSE_AMMO) != 0) {
        if (attackType == ATTACK_TYPE_RANGED) {
            attack->ammoQuantity = ammoGetQuantity(attack->weapon);
        } else {
            attack->attackerFlags &= ~DAM_LOSE_AMMO;
        }
    }

    if ((attack->attackerFlags & DAM_CRIP_RANDOM) != 0) {
        attack->attackerFlags &= ~DAM_CRIP_RANDOM;

        switch (randomBetween(0, 3)) {
        case 0:
            attack->attackerFlags |= DAM_CRIP_LEG_LEFT;
            break;
        case 1:
            attack->attackerFlags |= DAM_CRIP_LEG_RIGHT;
            break;
        case 2:
            attack->attackerFlags |= DAM_CRIP_ARM_LEFT;
            break;
        case 3:
            attack->attackerFlags |= DAM_CRIP_ARM_RIGHT;
            break;
        }
    }

    if ((attack->attackerFlags & DAM_RANDOM_HIT) != 0) {
        attack->defender = sub_42B868(attack);
        if (attack->defender != NULL) {
            attack->attackerFlags |= DAM_HIT;
            attack->defenderHitLocation = HIT_LOCATION_TORSO;
            attack->attackerFlags &= ~DAM_CRITICAL;

            int ammoQuantity = attackType == ATTACK_TYPE_RANGED ? attack->ammoQuantity : 1;
            attackComputeDamage(attack, ammoQuantity, 2);
        } else {
            attack->defender = attack->oops;
        }

        if (attack->defender != NULL) {
            attack->tile = attack->defender->tile;
        }
    }

    return 0;
}

// 0x42436C
int sub_42436C(Object* a1, Object* a2, int hitLocation, int hitMode)
{
    return attackDetermineToHit(a1, a1->tile, a2, hitLocation, hitMode, 1);
}

// 0x424380
int sub_424380(Object* a1, Object* a2, int hitLocation, int hitMode, unsigned char* a5)
{
    return attackDetermineToHit(a1, a1->tile, a2, hitLocation, hitMode, 0);
}

// 0x424394
int sub_424394(Object* a1, int tile, Object* a3, int hitLocation, int hitMode)
{
    return attackDetermineToHit(a1, tile, a3, hitLocation, hitMode, 1);
}

// determine_to_hit
// 0x4243A8
int attackDetermineToHit(Object* attacker, int tile, Object* defender, int hitLocation, int hitMode, int a6)
{
    Object* weapon = critterGetWeaponForHitMode(attacker, hitMode);

    bool targetIsCritter = defender != NULL
        ? ((defender->fid & 0xF000000) >> 24) == OBJ_TYPE_CRITTER
        : false;

    bool isUsingWeapon = false;

    int accuracy;
    if (weapon == NULL || hitMode == HIT_MODE_PUNCH || hitMode == HIT_MODE_KICK || (hitMode >= FIRST_ADVANCED_UNARMED_HIT_MODE && hitMode <= LAST_ADVANCED_UNARMED_HIT_MODE)) {
        accuracy = skillGetValue(attacker, SKILL_UNARMED);
    } else {
        isUsingWeapon = true;
        accuracy = sub_478370(attacker, hitMode);

        int modifier = 0;

        int attackType = weaponGetAttackTypeForHitMode(weapon, hitMode);
        if (attackType == ATTACK_TYPE_RANGED || attackType == ATTACK_TYPE_THROW) {
            int v29 = 0;
            int v25 = 0;

            int weaponPerk = weaponGetPerk(weapon);
            switch (weaponPerk) {
            case PERK_WEAPON_LONG_RANGE:
                v29 = 4;
                break;
            case PERK_WEAPON_SCOPE_RANGE:
                v29 = 5;
                v25 = 8;
                break;
            default:
                v29 = 2;
                break;
            }

            int perception = critterGetStat(attacker, STAT_PERCEPTION);

            if (defender != NULL) {
                modifier = objectGetDistanceBetweenTiles(attacker, tile, defender, defender->tile);
            } else {
                modifier = 0;
            }

            if (modifier >= v25) {
                int penalty = attacker == gDude
                    ? v29 * (perception - 2)
                    : v29 * perception;

                modifier -= penalty;
            } else {
                modifier += v25;
            }

            if (-2 * perception > modifier) {
                modifier = -2 * perception;
            }

            if (attacker == gDude) {
                modifier -= 2 * perkGetRank(gDude, PERK_SHARPSHOOTER);
            }

            if (modifier >= 0) {
                if ((attacker->data.critter.combat.results & DAM_BLIND) != 0) {
                    modifier *= -12;
                } else {
                    modifier *= -4;
                }
            } else {
                modifier *= -4;
            }

            if (a6 || modifier > 0) {
                accuracy += modifier;
            }

            modifier = 0;

            if (defender != NULL && a6) {
                sub_426CC4(attacker, tile, defender->tile, defender, &modifier);
            }

            accuracy -= 10 * modifier;
        }

        if (attacker == gDude && traitIsSelected(TRAIT_ONE_HANDER)) {
            if (weaponIsTwoHanded(weapon)) {
                accuracy -= 40;
            } else {
                accuracy += 20;
            }
        }

        int minStrength = weaponGetMinStrengthRequired(weapon);
        modifier = minStrength - critterGetStat(attacker, STAT_STRENGTH);
        if (attacker == gDude && perkGetRank(gDude, PERK_WEAPON_HANDLING) != 0) {
            modifier -= 3;
        }

        if (modifier > 0) {
            accuracy -= 20 * modifier;
        }

        if (weaponGetPerk(weapon) == PERK_WEAPON_ACCURATE) {
            accuracy += 20;
        }
    }

    if (targetIsCritter && defender != NULL) {
        int armorClass = critterGetStat(defender, STAT_ARMOR_CLASS);
        armorClass += weaponGetAmmoArmorClassModifier(weapon);
        if (armorClass < 0) {
            armorClass = 0;
        }

        accuracy -= armorClass;
    }

    if (isUsingWeapon) {
        accuracy += dword_510954[hitLocation];
    } else {
        accuracy += dword_510954[hitLocation] / 2;
    }

    if (defender != NULL && (defender->flags & 0x800) != 0) {
        accuracy += 15;
    }

    if (attacker == gDude) {
        int lightIntensity;
        if (defender != NULL) {
            lightIntensity = objectGetLightIntensity(defender);
            if (weaponGetPerk(weapon) == PERK_WEAPON_NIGHT_SIGHT) {
                lightIntensity = 65536;
            }
        } else {
            lightIntensity = 0;
        }

        if (lightIntensity <= 26214)
            accuracy -= 40;
        else if (lightIntensity <= 39321)
            accuracy -= 25;
        else if (lightIntensity <= 52428)
            accuracy -= 10;
    }

    if (off_51094C != NULL) {
        accuracy += off_51094C->accuracyBonus;
    }

    if ((attacker->data.critter.combat.results & DAM_BLIND) != 0) {
        accuracy -= 25;
    }

    if (targetIsCritter && defender != NULL && (defender->data.critter.combat.results & (DAM_KNOCKED_OUT | DAM_KNOCKED_DOWN)) != 0) {
        accuracy += 40;
    }

    if (attacker->data.critter.combat.team != gDude->data.critter.combat.team) {
        int combatDifficuly = 1;
        configGetInt(&gGameConfig, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_COMBAT_DIFFICULTY_KEY, &combatDifficuly);
        switch (combatDifficuly) {
        case 0:
            accuracy -= 20;
            break;
        case 2:
            accuracy += 20;
            break;
        }
    }

    if (accuracy > 95) {
        accuracy = 95;
    }

    if (accuracy < -100) {
        debugPrint("Whoa! Bad skill value in determine_to_hit!\n");
    }

    return accuracy;
}

// 0x4247B8
void attackComputeDamage(Attack* attack, int ammoQuantity, int a3)
{
    int* damagePtr;
    Object* critter;
    int* flagsPtr;
    int* knockbackDistancePtr;

    if ((attack->attackerFlags & DAM_HIT) != 0) {
        damagePtr = &(attack->defenderDamage);
        critter = attack->defender;
        flagsPtr = &(attack->defenderFlags);
        knockbackDistancePtr = &(attack->defenderKnockback);
    } else {
        damagePtr = &(attack->attackerDamage);
        critter = attack->attacker;
        flagsPtr = &(attack->attackerFlags);
        knockbackDistancePtr = NULL;
    }

    *damagePtr = 0;

    if ((critter->fid & 0xF000000) >> 24 != OBJ_TYPE_CRITTER) {
        return;
    }

    int damageType = weaponGetDamageType(attack->attacker, attack->weapon);
    int damageThreshold = critterGetStat(critter, STAT_DAMAGE_THRESHOLD + damageType);
    int damageResistance = critterGetStat(critter, STAT_DAMAGE_RESISTANCE + damageType);

    if ((*flagsPtr & DAM_BYPASS) != 0 && damageType != DAMAGE_TYPE_EMP) {
        damageThreshold = 20 * damageThreshold / 100;
        damageResistance = 20 * damageResistance / 100;
    } else {
        if (weaponGetPerk(attack->weapon) == PERK_WEAPON_PENETRATE
            || attack->hitMode == HIT_MODE_PALM_STRIKE
            || attack->hitMode == HIT_MODE_PIERCING_STRIKE
            || attack->hitMode == HIT_MODE_HOOK_KICK
            || attack->hitMode == HIT_MODE_PIERCING_KICK) {
            damageThreshold = 20 * damageThreshold / 100;
        }

        if (attack->attacker == gDude && traitIsSelected(TRAIT_FINESSE)) {
            damageResistance += 30;
        }
    }

    int damageBonus;
    if (attack->attacker == gDude && weaponGetAttackTypeForHitMode(attack->weapon, attack->hitMode) == ATTACK_TYPE_RANGED) {
        damageBonus = 2 * perkGetRank(gDude, PERK_BONUS_RANGED_DAMAGE);
    } else {
        damageBonus = 0;
    }

    int combatDifficultyDamageModifier = 100;
    if (attack->attacker->data.critter.combat.team != gDude->data.critter.combat.team) {
        int combatDifficulty = COMBAT_DIFFICULTY_NORMAL;
        configGetInt(&gGameConfig, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_COMBAT_DIFFICULTY_KEY, &combatDifficulty);

        switch (combatDifficulty) {
        case COMBAT_DIFFICULTY_EASY:
            combatDifficultyDamageModifier = 75;
            break;
        case COMBAT_DIFFICULTY_HARD:
            combatDifficultyDamageModifier = 125;
            break;
        }
    }

    damageResistance += weaponGetAmmoDamageResistanceModifier(attack->weapon);
    if (damageResistance > 100) {
        damageResistance = 100;
    } else if (damageResistance < 0) {
        damageResistance = 0;
    }

    int damageMultiplier = a3 * weaponGetAmmoDamageMultiplier(attack->weapon);
    int damageDivisor = weaponGetAmmoDamageDivisor(attack->weapon);

    for (int index = 0; index < ammoQuantity; index++) {
        int damage = weaponGetMeleeDamage(attack->attacker, attack->hitMode);

        damage += damageBonus;

        damage *= damageMultiplier;

        if (damageDivisor != 0) {
            damage /= damageDivisor;
        }

        // TODO: Why we're halving it?
        damage /= 2;

        damage *= combatDifficultyDamageModifier;
        damage /= 100;

        damage -= damageThreshold;

        if (damage > 0) {
            damage -= damage * damageResistance / 100;
        }

        if (damage > 0) {
            *damagePtr += damage;
        }
    }

    if (attack->attacker == gDude) {
        if (perkGetRank(attack->attacker, PERK_LIVING_ANATOMY) != 0) {
            int kt = critterGetKillType(attack->defender);
            if (kt != KILL_TYPE_ROBOT && kt != KILL_TYPE_ALIEN) {
                *damagePtr += 5;
            }
        }

        if (perkGetRank(attack->attacker, PERK_PYROMANIAC) != 0) {
            if (weaponGetDamageType(attack->attacker, attack->weapon) == DAMAGE_TYPE_FIRE) {
                *damagePtr += 5;
            }
        }
    }

    if (knockbackDistancePtr != NULL
        && (critter->flags & 0x0800) == 0
        && (damageType == DAMAGE_TYPE_EXPLOSION || attack->weapon == NULL || weaponGetAttackTypeForHitMode(attack->weapon, attack->hitMode) == ATTACK_TYPE_MELEE)
        && (critter->pid >> 24) == OBJ_TYPE_CRITTER
        && sub_42E6AC(critter->pid, 0x4000) == 0) {
        bool shouldKnockback = true;
        bool hasStonewall = false;
        if (critter == gDude) {
            if (perkGetRank(critter, PERK_STONEWALL) != 0) {
                int chance = randomBetween(0, 100);
                hasStonewall = true;
                if (chance < 50) {
                    shouldKnockback = false;
                }
            }
        }

        if (shouldKnockback) {
            int knockbackDistanceDivisor = weaponGetPerk(attack->weapon) == PERK_WEAPON_KNOCKBACK ? 5 : 10;

            *knockbackDistancePtr = *damagePtr / knockbackDistanceDivisor;

            if (hasStonewall) {
                *knockbackDistancePtr /= 2;
            }
        }
    }
}

// 0x424BAC
void attackComputeDeathFlags(Attack* attack)
{
    sub_424EE8(attack->attacker, attack->attackerDamage, &(attack->attackerFlags));
    sub_424EE8(attack->defender, attack->defenderDamage, &(attack->defenderFlags));

    for (int index = 0; index < attack->extrasLength; index++) {
        sub_424EE8(attack->extras[index], attack->extrasDamage[index], &(attack->extrasFlags[index]));
    }
}

// 0x424C04
void sub_424C04(Attack* attack, bool animated)
{
    Object* attacker = attack->attacker;
    bool attackerIsCritter = attacker != NULL && (attacker->fid & 0xF000000) >> 24 == OBJ_TYPE_CRITTER;
    bool v5 = attack->defender != attack->oops;

    if (attackerIsCritter && (attacker->data.critter.combat.results & DAM_DEAD) != 0) {
        sub_424F2C(attacker, attack->attackerFlags);
        // TODO: Not sure about "attack->defender == attack->oops".
        sub_425020(attacker, attack->attackerDamage, animated, attack->defender == attack->oops, attacker);
    }

    Object* v7 = attack->oops;
    if (v7 != NULL && v7 != attack->defender) {
        sub_42BC60(v7);
    }

    Object* defender = attack->defender;
    bool defenderIsCritter = defender != NULL && (defender->fid & 0xF000000) >> 24 == OBJ_TYPE_CRITTER;

    if (!defenderIsCritter && !v5) {
        bool v9 = objectIsPartyMember(attack->defender) && objectIsPartyMember(attack->attacker) ? false : true;
        if (v9) {
            if (defender != NULL) {
                if (defender->sid != -1) {
                    scriptSetFixedParam(defender->sid, attack->attackerDamage);
                    scriptSetObjects(defender->sid, attack->attacker, attack->weapon);
                    scriptExecProc(defender->sid, SCRIPT_PROC_DAMAGE);
                }
            }
        }
    }

    if (defenderIsCritter && (defender->data.critter.combat.results & DAM_DEAD) == 0) {
        sub_424F2C(defender, attack->defenderFlags);

        if (defenderIsCritter) {
            if (defenderIsCritter) {
                if ((defender->data.critter.combat.results & (DAM_DEAD | DAM_KNOCKED_OUT)) != 0) {
                    if (!v5 || defender != gDude) {
                        sub_42E4C0(defender, attack->attacker);
                    }
                } else if (defender == attack->oops || defender->data.critter.combat.team != attack->attacker->data.critter.combat.team) {
                    sub_42B9D4(defender, attack->attacker);
                }
            }
        }

        scriptSetObjects(defender->sid, attack->attacker, attack->weapon);
        sub_425020(defender, attack->defenderDamage, animated, attack->defender != attack->oops, attacker);

        if (defenderIsCritter) {
            sub_42BC60(defender);
        }

        if (attack->defenderDamage >= 0 && (attack->attackerFlags & DAM_HIT) != 0) {
            scriptSetObjects(attack->attacker->sid, NULL, attack->defender);
            scriptSetFixedParam(attack->attacker->sid, 2);
            scriptExecProc(attack->attacker->sid, SCRIPT_PROC_COMBAT);
        }
    }

    for (int index = 0; index < attack->extrasLength; index++) {
        Object* obj = attack->extras[index];
        if ((obj->fid & 0xF000000) >> 24 == OBJ_TYPE_CRITTER && (obj->data.critter.combat.results & DAM_DEAD) == 0) {
            sub_424F2C(obj, attack->extrasFlags[index]);

            if (defenderIsCritter) {
                if ((obj->data.critter.combat.results & (DAM_DEAD | DAM_KNOCKED_OUT)) != 0) {
                    sub_42E4C0(obj, attack->attacker);
                } else if (obj->data.critter.combat.team != attack->attacker->data.critter.combat.team) {
                    sub_42B9D4(obj, attack->attacker);
                }
            }

            scriptSetObjects(obj->sid, attack->attacker, attack->weapon);
            // TODO: Not sure about defender == oops.
            sub_425020(obj, attack->extrasDamage[index], animated, attack->defender == attack->oops, attack->attacker);
            sub_42BC60(obj);

            if (attack->extrasDamage[index] >= 0) {
                if ((attack->attackerFlags & DAM_HIT) != 0) {
                    scriptSetObjects(attack->attacker->sid, NULL, obj);
                    scriptSetFixedParam(attack->attacker->sid, 2);
                    scriptExecProc(attack->attacker->sid, SCRIPT_PROC_COMBAT);
                }
            }
        }
    }
}

// 0x424EE8
void sub_424EE8(Object* object, int damage, int* flags)
{
    if (object == NULL || !sub_42E6AC(object->pid, 0x0400)) {
        if (object == NULL || (object->pid >> 24) == OBJ_TYPE_CRITTER) {
            if (damage > 0) {
                if (critterGetHitPoints(object) - damage <= 0) {
                    *flags |= DAM_DEAD;
                }
            }
        }
    }
}

// 0x424F2C
void sub_424F2C(Object* a1, int a2)
{
    if (a1 == NULL) {
        return;
    }

    if (((a1->pid & 0xF000000) >> 24) != OBJ_TYPE_CRITTER) {
        return;
    }

    if (sub_42E6AC(a1->pid, 0x0400)) {
        return;
    }

    if ((a1->pid >> 24) != OBJ_TYPE_CRITTER) {
        return;
    }

    if ((a2 & DAM_DEAD) != 0) {
        queueRemoveEvents(a1);
    } else if ((a2 & DAM_KNOCKED_OUT) != 0) {
        int endurance = critterGetStat(a1, STAT_ENDURANCE);
        queueAddEvent(10 * (35 - 3 * endurance), a1, NULL, EVENT_TYPE_KNOCKOUT);
    }

    if (a1 == gDude && (a2 & (DAM_CRIP_ARM_LEFT | DAM_CRIP_ARM_RIGHT)) == 0) {
        a1->data.critter.combat.results |= (a2 & 0x80FF);

        int v5;
        int v4;
        sub_45F4B4(&v5, &v4);
        sub_45EFEC(true, v5, v4);
    } else {
        a1->data.critter.combat.results |= (a2 & 0x80FF);
    }
}

// 0x425020
void sub_425020(Object* a1, int damage, bool animated, int a4, Object* a5)
{
    if (a1 == NULL) {
        return;
    }

    if ((a1->fid & 0xF000000) >> 24 != OBJ_TYPE_CRITTER) {
        return;
    }

    if (sub_42E6AC(a1->pid, 1024)) {
        return;
    }

    if (damage <= 0) {
        return;
    }

    critterAdjustHitPoints(a1, -damage);

    if (a1 == gDude) {
        interfaceRenderHitPoints(animated);
    }

    a1->data.critter.combat.damageLastTurn += damage;

    if (!a4) {
        // TODO: Not sure about this one.
        if (!objectIsPartyMember(a1) || !objectIsPartyMember(a5)) {
            scriptSetFixedParam(a1->sid, damage);
            scriptExecProc(a1->sid, SCRIPT_PROC_DAMAGE);
        }
    }

    if ((a1->data.critter.combat.results & DAM_DEAD) != 0) {
        scriptSetObjects(a1->sid, a1->data.critter.combat.whoHitMe, NULL);
        scriptExecProc(a1->sid, SCRIPT_PROC_DESTROY);
        sub_477770(a1);

        if (a1 != gDude) {
            Object* whoHitMe = a1->data.critter.combat.whoHitMe;
            if (whoHitMe == gDude || whoHitMe != NULL && whoHitMe->data.critter.combat.team == gDude->data.critter.combat.team) {
                bool scriptOverrides = false;
                Script* scr;
                if (scriptGetScript(a1->sid, &scr) != -1) {
                    scriptOverrides = scr->scriptOverrides;
                }

                if (!scriptOverrides) {
                    dword_56D398 += critterGetExp(a1);
                    killsIncByType(critterGetKillType(a1));
                }
            }
        }

        if (a1->sid != -1) {
            scriptRemove(a1->sid);
            a1->sid = -1;
        }

        partyMemberRemove(a1);
    }
}

// Print attack description to monitor.
//
// 0x425170
void sub_425170(Attack* attack)
{
    MessageListItem messageListItem;

    if (attack->attacker == gDude) {
        Object* weapon = critterGetWeaponForHitMode(attack->attacker, attack->hitMode);
        int strengthRequired = weaponGetMinStrengthRequired(weapon);

        if (perkGetRank(attack->attacker, PERK_WEAPON_HANDLING) != 0) {
            strengthRequired -= 3;
        }

        if (weapon != NULL) {
            if (strengthRequired > critterGetStat(gDude, STAT_STRENGTH)) {
                // You are not strong enough to use this weapon properly.
                messageListItem.num = 107;
                if (messageListGetItem(&gCombatMessageList, &messageListItem)) {
                    displayMonitorAddMessage(messageListItem.text);
                }
            }
        }
    }

    Object* mainCritter;
    if ((attack->attackerFlags & DAM_HIT) != 0) {
        mainCritter = attack->defender;
    } else {
        mainCritter = attack->attacker;
    }

    char* mainCritterName = byte_500B50;

    char you[20];
    you[0] = '\0';
    if (critterGetStat(gDude, STAT_GENDER) == GENDER_MALE) {
        // You (male)
        messageListItem.num = 506;
    } else {
        // You (female)
        messageListItem.num = 556;
    }

    if (messageListGetItem(&gCombatMessageList, &messageListItem)) {
        strcpy(you, messageListItem.text);
    }

    int baseMessageId;
    if (mainCritter == gDude) {
        mainCritterName = you;
        if (critterGetStat(gDude, STAT_GENDER) == GENDER_MALE) {
            baseMessageId = 500;
        } else {
            baseMessageId = 550;
        }
    } else if (mainCritter != NULL) {
        mainCritterName = objectGetName(mainCritter);
        if (critterGetStat(mainCritter, STAT_GENDER) == GENDER_MALE) {
            baseMessageId = 600;
        } else {
            baseMessageId = 700;
        }
    }

    char text[280];
    if (attack->defender != NULL
        && attack->oops != NULL
        && attack->defender != attack->oops
        && (attack->attackerFlags & DAM_HIT) != 0) {
        if ((attack->defender->fid & 0xF000000) >> 24 == OBJ_TYPE_CRITTER) {
            if (attack->oops == gDude) {
                // 608 (male) - Oops! %s was hit instead of you!
                // 708 (female) - Oops! %s was hit instead of you!
                messageListItem.num = baseMessageId + 8;
                if (messageListGetItem(&gCombatMessageList, &messageListItem)) {
                    sprintf(text, messageListItem.text, mainCritterName);
                }
            } else {
                // 509 (male) - Oops! %s were hit instead of %s!
                // 559 (female) - Oops! %s were hit instead of %s!
                const char* name = objectGetName(attack->oops);
                messageListItem.num = baseMessageId + 9;
                if (messageListGetItem(&gCombatMessageList, &messageListItem)) {
                    sprintf(text, messageListItem.text, mainCritterName, name);
                }
            }
        } else {
            if (attack->attacker == gDude) {
                if (critterGetStat(attack->attacker, STAT_GENDER) == GENDER_MALE) {
                    // (male) %s missed
                    messageListItem.num = 515;
                } else {
                    // (female) %s missed
                    messageListItem.num = 565;
                }

                if (messageListGetItem(&gCombatMessageList, &messageListItem)) {
                    sprintf(text, messageListItem.text, you);
                }
            } else {
                const char* name = objectGetName(attack->attacker);
                if (critterGetStat(attack->attacker, STAT_GENDER) == GENDER_MALE) {
                    // (male) %s missed
                    messageListItem.num = 615;
                } else {
                    // (female) %s missed
                    messageListItem.num = 715;
                }

                if (messageListGetItem(&gCombatMessageList, &messageListItem)) {
                    sprintf(text, messageListItem.text, name);
                }
            }
        }

        strcat(text, ".");

        displayMonitorAddMessage(text);
    }

    if ((attack->attackerFlags & DAM_HIT) != 0) {
        Object* v21 = attack->defender;
        if (v21 != NULL && (v21->data.critter.combat.results & DAM_DEAD) == 0) {
            text[0] = '\0';

            if ((v21->fid & 0xF000000) >> 24 == OBJ_TYPE_CRITTER) {
                if (attack->defenderHitLocation == HIT_LOCATION_TORSO) {
                    if ((attack->attackerFlags & DAM_CRITICAL) != 0) {
                        switch (attack->defenderDamage) {
                        case 0:
                            // 528 - %s were critically hit for no damage
                            messageListItem.num = baseMessageId + 28;
                            break;
                        case 1:
                            // 524 - %s were critically hit for 1 hit point
                            messageListItem.num = baseMessageId + 24;
                            break;
                        default:
                            // 520 - %s were critically hit for %d hit points
                            messageListItem.num = baseMessageId + 20;
                            break;
                        }

                        if (messageListGetItem(&gCombatMessageList, &messageListItem)) {
                            if (attack->defenderDamage <= 1) {
                                sprintf(text, messageListItem.text, mainCritterName);
                            } else {
                                sprintf(text, messageListItem.text, mainCritterName, attack->defenderDamage);
                            }
                        }
                    } else {
                        combatCopyDamageAmountDescription(text, v21, attack->defenderDamage);
                    }
                } else {
                    const char* hitLocationName = hitLocationGetName(v21, attack->defenderHitLocation);
                    if (hitLocationName != NULL) {
                        if ((attack->attackerFlags & DAM_CRITICAL) != 0) {
                            switch (attack->defenderDamage) {
                            case 0:
                                // 525 - %s were critically hit in %s for no damage
                                messageListItem.num = baseMessageId + 25;
                                break;
                            case 1:
                                // 521 - %s were critically hit in %s for 1 damage
                                messageListItem.num = baseMessageId + 21;
                                break;
                            default:
                                // 511 - %s were critically hit in %s for %d hit points
                                messageListItem.num = baseMessageId + 11;
                                break;
                            }
                        } else {
                            switch (attack->defenderDamage) {
                            case 0:
                                // 526 - %s were hit in %s for no damage
                                messageListItem.num = baseMessageId + 26;
                                break;
                            case 1:
                                // 522 - %s were hit in %s for 1 damage
                                messageListItem.num = baseMessageId + 22;
                                break;
                            default:
                                // 512 - %s were hit in %s for %d hit points
                                messageListItem.num = baseMessageId + 12;
                                break;
                            }
                        }

                        if (messageListGetItem(&gCombatMessageList, &messageListItem)) {
                            if (attack->defenderDamage <= 1) {
                                sprintf(text, messageListItem.text, mainCritterName, hitLocationName);
                            } else {
                                sprintf(text, messageListItem.text, mainCritterName, hitLocationName, attack->defenderDamage);
                            }
                        }
                    }
                }

                int combatMessages = 1;
                configGetInt(&gGameConfig, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_COMBAT_MESSAGES_KEY, &combatMessages);

                if (combatMessages == 1 && (attack->attackerFlags & DAM_CRITICAL) != 0 && attack->criticalMessageId != -1) {
                    messageListItem.num = attack->criticalMessageId;
                    if (messageListGetItem(&gCombatMessageList, &messageListItem)) {
                        strcat(text, messageListItem.text);
                    }

                    if ((attack->defenderFlags & DAM_DEAD) != 0) {
                        strcat(text, ".");
                        displayMonitorAddMessage(text);

                        if (attack->defender == gDude) {
                            if (critterGetStat(attack->defender, STAT_GENDER) == GENDER_MALE) {
                                // were killed
                                messageListItem.num = 207;
                            } else {
                                // were killed
                                messageListItem.num = 257;
                            }
                        } else {
                            if (critterGetStat(attack->defender, STAT_GENDER) == GENDER_MALE) {
                                // was killed
                                messageListItem.num = 307;
                            } else {
                                // was killed
                                messageListItem.num = 407;
                            }
                        }

                        if (messageListGetItem(&gCombatMessageList, &messageListItem)) {
                            sprintf(text, "%s %s", mainCritterName, messageListItem.text);
                        }
                    }
                } else {
                    combatAddDamageFlagsDescription(text, attack->defenderFlags, attack->defender);
                }

                strcat(text, ".");

                displayMonitorAddMessage(text);
            }
        }
    }

    if (attack->attacker != NULL && (attack->attacker->data.critter.combat.results & DAM_DEAD) == 0) {
        if ((attack->attackerFlags & DAM_HIT) == 0) {
            if ((attack->attackerFlags & DAM_CRITICAL) != 0) {
                switch (attack->attackerDamage) {
                case 0:
                    // 514 - %s critically missed
                    messageListItem.num = baseMessageId + 14;
                    break;
                case 1:
                    // 533 - %s critically missed and took 1 hit point
                    messageListItem.num = baseMessageId + 33;
                    break;
                default:
                    // 534 - %s critically missed and took %d hit points
                    messageListItem.num = baseMessageId + 34;
                    break;
                }
            } else {
                // 515 - %s missed
                messageListItem.num = baseMessageId + 15;
            }

            if (messageListGetItem(&gCombatMessageList, &messageListItem)) {
                if (attack->attackerDamage <= 1) {
                    sprintf(text, messageListItem.text, mainCritterName);
                } else {
                    sprintf(text, messageListItem.text, mainCritterName, attack->attackerDamage);
                }
            }

            combatAddDamageFlagsDescription(text, attack->attackerFlags, attack->attacker);

            strcat(text, ".");

            displayMonitorAddMessage(text);
        }

        if ((attack->attackerFlags & DAM_HIT) != 0 || (attack->attackerFlags & DAM_CRITICAL) == 0) {
            if (attack->attackerDamage > 0) {
                combatCopyDamageAmountDescription(text, attack->attacker, attack->attackerDamage);
                combatAddDamageFlagsDescription(text, attack->attackerFlags, attack->attacker);
                strcat(text, ".");
                displayMonitorAddMessage(text);
            }
        }
    }

    for (int index = 0; index < attack->extrasLength; index++) {
        Object* critter = attack->extras[index];
        if ((critter->data.critter.combat.results & DAM_DEAD) == 0) {
            combatCopyDamageAmountDescription(text, critter, attack->extrasDamage[index]);
            combatAddDamageFlagsDescription(text, attack->extrasFlags[index], critter);
            strcat(text, ".");

            displayMonitorAddMessage(text);
        }
    }
}

// 0x425A9C
void combatCopyDamageAmountDescription(char* dest, Object* critter, int damage)
{
    MessageListItem messageListItem;
    char text[40];
    char* name;

    int messageId;
    if (critter == gDude) {
        text[0] = '\0';

        if (critterGetStat(gDude, STAT_GENDER) == GENDER_MALE) {
            messageId = 500;
        } else {
            messageId = 550;
        }

        // 506 - You
        messageListItem.num = messageId + 6;
        if (messageListGetItem(&gCombatMessageList, &messageListItem)) {
            strcpy(text, messageListItem.text);
        }

        name = text;
    } else {
        name = objectGetName(critter);

        if (critterGetStat(critter, STAT_GENDER) == GENDER_MALE) {
            messageId = 600;
        } else {
            messageId = 700;
        }
    }

    switch (damage) {
    case 0:
        // 627 - %s was hit for no damage
        messageId += 27;
        break;
    case 1:
        // 623 - %s was hit for 1 hit point
        messageId += 23;
        break;
    default:
        // 613 - %s was hit for %d hit points
        messageId += 13;
        break;
    }

    messageListItem.num = messageId;
    if (messageListGetItem(&gCombatMessageList, &messageListItem)) {
        if (damage <= 1) {
            sprintf(dest, messageListItem.text, name);
        } else {
            sprintf(dest, messageListItem.text, name, damage);
        }
    }
}

// 0x425BA4
void combatAddDamageFlagsDescription(char* dest, int flags, Object* critter)
{
    MessageListItem messageListItem;

    int num;
    if (critter == gDude) {
        if (critterGetStat(critter, STAT_GENDER) == GENDER_MALE) {
            num = 200;
        } else {
            num = 250;
        }
    } else {
        if (critterGetStat(critter, STAT_GENDER) == GENDER_MALE) {
            num = 300;
        } else {
            num = 400;
        }
    }

    if (flags == 0) {
        return;
    }

    if ((flags & DAM_DEAD) != 0) {
        // " and "
        messageListItem.num = 108;
        if (messageListGetItem(&gCombatMessageList, &messageListItem)) {
            strcat(dest, messageListItem.text);
        }

        // were killed
        messageListItem.num = num + 7;
        if (messageListGetItem(&gCombatMessageList, &messageListItem)) {
            strcat(dest, messageListItem.text);
        }

        return;
    }

    int bit = 1;
    int flagsListLength = 0;
    int flagsList[32];
    for (int index = 0; index < 32; index++) {
        if (bit != DAM_CRITICAL && bit != DAM_HIT && (bit & flags) != 0) {
            flagsList[flagsListLength++] = index;
        }
        bit <<= 1;
    }

    if (flagsListLength != 0) {
        for (int index = 0; index < flagsListLength - 1; index++) {
            strcat(dest, ", ");

            messageListItem.num = num + flagsList[index];
            if (messageListGetItem(&gCombatMessageList, &messageListItem)) {
                strcat(dest, messageListItem.text);
            }
        }

        // " and "
        messageListItem.num = 108;
        if (messageListGetItem(&gCombatMessageList, &messageListItem)) {
            strcat(dest, messageListItem.text);
        }

        messageListItem.num = flagsList[flagsListLength - 1];
        if (messageListGetItem(&gCombatMessageList, &messageListItem)) {
            strcat(dest, messageListItem.text);
        }
    }
}

// 0x425E3C
void sub_425E3C()
{
    if (++dword_51093C == 1 && gDude == stru_56D2B0.attacker) {
        gameUiDisable(1);
        gameMouseSetCursor(26);
        if (dword_56D38C == 2) {
            sub_426BC0();
        }
    }
}

// 0x425E80
void sub_425E80()
{
    dword_51093C -= 1;
    if (dword_51093C != 0) {
        return;
    }

    if (gDude == stru_56D2B0.attacker) {
        gameUiEnable();
    }

    if (dword_517F9C) {
        dword_517F9C = false;

        Object* weapon = critterGetWeaponForHitMode(stru_56D2B0.attacker, stru_56D2B0.hitMode);
        if (weapon != NULL) {
            if (ammoGetCapacity(weapon) > 0) {
                int ammoQuantity = ammoGetQuantity(weapon);
                ammoSetQuantity(weapon, ammoQuantity - stru_56D2B0.ammoQuantity);

                if (stru_56D2B0.attacker == gDude) {
                    sub_45F838();
                }
            }
        }

        if (dword_510950) {
            sub_425170(&stru_56D2B0);
            dword_510950 = false;
        }

        sub_424C04(&stru_56D2B0, true);

        Object* attacker = stru_56D2B0.attacker;
        if (attacker == gDude && dword_56D38C == 2) {
            sub_426AA8();
        }

        if (sub_4A6EFC()) {
            if ((gDude->data.critter.combat.results & DAM_KNOCKED_OUT) != 0) {
                if (attacker->data.critter.combat.team == gDude->data.critter.combat.team) {
                    off_56D380 = gDude->data.critter.combat.whoHitMe;
                } else {
                    off_56D380 = attacker;
                }
            }
        }

        attackInit(&stru_56D2B0, stru_56D2B0.attacker, NULL, HIT_MODE_PUNCH, HIT_LOCATION_TORSO);

        if ((attacker->data.critter.combat.results & (DAM_KNOCKED_OUT | DAM_KNOCKED_DOWN)) != 0) {
            if ((attacker->data.critter.combat.results & (DAM_KNOCKED_OUT | DAM_DEAD | DAM_LOSE_TURN)) == 0) {
                sub_425FBC(attacker);
            }
        }
    }
}

// 0x425FBC
void sub_425FBC(Object* a1)
{
    int v2;

    v2 = 3;
    if (a1 == gDude && perkGetRank(a1, PERK_QUICK_RECOVERY)) {
        v2 = 1;
    }

    if (v2 > a1->data.critter.combat.ap) {
        a1->data.critter.combat.ap = 0;
    } else {
        a1->data.critter.combat.ap -= v2;
    }

    if (a1 == gDude) {
        interfaceRenderActionPoints(gDude->data.critter.combat.ap, dword_56D39C);
    }

    sub_418574(a1);

    // NOTE: Uninline.
    sub_4227DC();
}

// Render two digits.
//
// 0x42603C
void sub_42603C(unsigned char* dest, int destPitch, int accuracy)
{
    CacheEntry* numbersFrmHandle;
    int numbersFrmFid = buildFid(6, 82, 0, 0, 0);
    unsigned char* numbersFrmData = artLockFrameData(numbersFrmFid, 0, 0, &numbersFrmHandle);
    if (numbersFrmData == NULL) {
        return;
    }

    if (accuracy >= 0) {
        blitBufferToBuffer(numbersFrmData + 9 * (accuracy % 10), 9, 17, 360, dest + 9, destPitch);
        blitBufferToBuffer(numbersFrmData + 9 * (accuracy / 10), 9, 17, 360, dest, destPitch);
    } else {
        blitBufferToBuffer(numbersFrmData + 108, 6, 17, 360, dest + 9, destPitch);
        blitBufferToBuffer(numbersFrmData + 108, 6, 17, 360, dest, destPitch);
    }

    artUnlock(numbersFrmHandle);
}

// 0x42612C
char* hitLocationGetName(Object* critter, int hitLocation)
{
    MessageListItem messageListItem;
    messageListItem.num = 1000 + 10 * sub_419998(critter->fid & 0xFFF) + hitLocation;
    if (messageListGetItem(&gCombatMessageList, &messageListItem)) {
        return messageListItem.text;
    }

    return NULL;
}

// 0x4261B4
void sub_4261B4(int a1, int a2)
{
    sub_4261CC(a2, byte_6A38D0[992]);
}

// 0x4261C0
void sub_4261C0(int a1, int a2)
{
    sub_4261CC(a2, byte_6A38D0[31744]);
}

// 0x4261CC
void sub_4261CC(int eventCode, int color)
{
    color |= 0x3000000;

    if (eventCode >= 4) {
        char* name = hitLocationGetName(gCalledShotCritter, dword_51804C[eventCode - 4]);
        int width = fontGetStringWidth(name);
        windowDrawText(gCalledShotWindow, name, 0, 431 - width, dword_51802C[eventCode - 4] - 86, color);
    } else {
        char* name = hitLocationGetName(gCalledShotCritter, dword_51803C[eventCode]);
        windowDrawText(gCalledShotWindow, name, 0, 74, dword_51802C[eventCode] - 86, color);
    }
}

// 0x426218
int calledShotSelectHitLocation(Object* critter, int* hitLocation, int hitMode)
{
    if (critter == NULL) {
        return 0;
    }

    if (critter->pid >> 24 != OBJ_TYPE_CRITTER) {
        return 0;
    }

    gCalledShotCritter = critter;
    gCalledShotWindow = windowCreate(CALLED_SHOW_WINDOW_X, CALLED_SHOW_WINDOW_Y, CALLED_SHOW_WINDOW_WIDTH, CALLED_SHOW_WINDOW_HEIGHT, byte_6A38D0[0], WINDOW_FLAG_0x10);
    if (gCalledShotWindow == -1) {
        return -1;
    }

    int fid;
    CacheEntry* handle;
    unsigned char* data;

    unsigned char* windowBuffer = windowGetBuffer(gCalledShotWindow);

    fid = buildFid(6, 118, 0, 0, 0);
    data = artLockFrameData(fid, 0, 0, &handle);
    if (data == NULL) {
        windowDestroy(gCalledShotWindow);
        return -1;
    }

    blitBufferToBuffer(data, CALLED_SHOW_WINDOW_WIDTH, CALLED_SHOW_WINDOW_HEIGHT, CALLED_SHOW_WINDOW_WIDTH, windowBuffer, CALLED_SHOW_WINDOW_WIDTH);
    artUnlock(handle);

    fid = buildFid(1, critter->fid & 0xFFF, ANIM_CALLED_SHOT_PIC, 0, 0);
    data = artLockFrameData(fid, 0, 0, &handle);
    if (data != NULL) {
        blitBufferToBuffer(data, 170, 225, 170, windowBuffer + CALLED_SHOW_WINDOW_WIDTH * 31 + 168, CALLED_SHOW_WINDOW_WIDTH);
        artUnlock(handle);
    }

    fid = buildFid(6, 8, 0, 0, 0);

    CacheEntry* upHandle;
    unsigned char* up = artLockFrameData(fid, 0, 0, &upHandle);
    if (up == NULL) {
        windowDestroy(gCalledShotWindow);
        return -1;
    }

    fid = buildFid(6, 9, 0, 0, 0);

    CacheEntry* downHandle;
    unsigned char* down = artLockFrameData(fid, 0, 0, &downHandle);
    if (down == NULL) {
        artUnlock(upHandle);
        windowDestroy(gCalledShotWindow);
        return -1;
    }

    // Cancel button
    int btn = buttonCreate(gCalledShotWindow, 210, 268, 15, 16, -1, -1, -1, KEY_ESCAPE, up, down, NULL, BUTTON_FLAG_TRANSPARENT);
    if (btn != -1) {
        buttonSetCallbacks(btn, sub_451970, sub_451978);
    }

    int oldFont = fontGetCurrent();
    fontSetCurrent(101);

    for (int index = 0; index < 4; index++) {
        int probability;
        int btn;

        probability = sub_42436C(gDude, critter, dword_51803C[index], hitMode);
        sub_42603C(windowBuffer + CALLED_SHOW_WINDOW_WIDTH * (dword_51802C[index] - 86) + 33, CALLED_SHOW_WINDOW_WIDTH, probability);

        btn = buttonCreate(gCalledShotWindow, 33, dword_51802C[index] - 90, 128, 20, index, index, -1, index, NULL, NULL, NULL, 0);
        buttonSetMouseCallbacks(btn, sub_4261C0, sub_4261B4, NULL, NULL);
        sub_4261CC(index, byte_6A38D0[992]);

        probability = sub_42436C(gDude, critter, dword_51804C[index], hitMode);
        sub_42603C(windowBuffer + CALLED_SHOW_WINDOW_WIDTH * (dword_51802C[index] - 86) + 453, CALLED_SHOW_WINDOW_WIDTH, probability);

        btn = buttonCreate(gCalledShotWindow, 341, dword_51802C[index] - 90, 128, 20, index + 4, index + 4, -1, index + 4, NULL, NULL, NULL, 0);
        buttonSetMouseCallbacks(btn, sub_4261C0, sub_4261B4, NULL, NULL);
        sub_4261CC(index + 4, byte_6A38D0[992]);
    }

    windowRefresh(gCalledShotWindow);

    bool gameUiWasDisabled = gameUiIsDisabled();
    if (gameUiWasDisabled) {
        gameUiEnable();
    }

    sub_44B48C(0);
    gameMouseSetCursor(MOUSE_CURSOR_ARROW);

    int eventCode;
    while (true) {
        eventCode = sub_4C8B78();

        if (eventCode == KEY_ESCAPE) {
            break;
        }

        if (eventCode >= 0 && eventCode < HIT_LOCATION_COUNT) {
            break;
        }

        if (dword_5186CC != 0) {
            break;
        }
    }

    sub_44B454();

    if (gameUiWasDisabled) {
        gameUiDisable(0);
    }

    fontSetCurrent(oldFont);

    artUnlock(downHandle);
    artUnlock(upHandle);
    windowDestroy(gCalledShotWindow);

    if (eventCode == VK_ESCAPE) {
        return -1;
    }

    *hitLocation = eventCode;

    soundPlayFile("icsxxxx1");

    return 0;
}

// check for possibility of performing attacking
// 0x426614
int sub_426614(Object* attacker, Object* defender, int hitMode, bool aiming)
{
    int range = 1;
    int tile = -1;
    if (defender != NULL) {
        tile = defender->tile;
        range = objectGetDistanceBetween(attacker, defender);
        if ((defender->data.critter.combat.results & DAM_DEAD) != 0) {
            return 4; // defender is dead
        }
    }

    Object* weapon = critterGetWeaponForHitMode(attacker, hitMode);
    if (weapon != NULL) {
        if ((defender->data.critter.combat.results & DAM_CRIP_ARM_LEFT) != 0
            && (defender->data.critter.combat.results & DAM_CRIP_ARM_RIGHT) != 0) {
            return 7; // both hands crippled
        }

        if ((defender->data.critter.combat.results & (DAM_CRIP_ARM_LEFT | DAM_CRIP_ARM_RIGHT)) != 0) {
            if (weaponIsTwoHanded(weapon)) {
                return 6; // crippled one arm for two-handed weapon
            }
        }
    }

    if (sub_478B24(attacker, hitMode, aiming) > attacker->data.critter.combat.ap) {
        return 3; // not enough action points
    }

    if (sub_478A1C(attacker, hitMode) < range) {
        return 2; // target out of range
    }

    int attackType = weaponGetAttackTypeForHitMode(weapon, hitMode);

    if (ammoGetCapacity(weapon) > 0) {
        if (ammoGetQuantity(weapon) == 0) {
            return 1; // out of ammo
        }
    }

    if (attackType == ATTACK_TYPE_RANGED
        || attackType == ATTACK_TYPE_THROW
        || sub_478A1C(attacker, hitMode) > 1) {
        if (sub_426CC4(attacker, attacker->tile, tile, defender, NULL)) {
            return 5; // Your aim is blocked
        }
    }

    return 0; // success
}

// 0x426744
bool sub_426744(Object* target, int* accuracy)
{
    int hitMode;
    bool aiming;
    if (interfaceGetCurrentHitMode(&hitMode, &aiming) == -1) {
        return false;
    }

    if (sub_426614(gDude, target, hitMode, aiming) != 0) {
        return false;
    }

    *accuracy = attackDetermineToHit(gDude, gDude->tile, target, HIT_LOCATION_UNCALLED, hitMode, 1);

    return true;
}

// 0x4267CC
void sub_4267CC(Object* a1)
{
    if (a1 == NULL) {
        return;
    }

    if ((gCombatState & 0x02) == 0) {
        return;
    }

    int hitMode;
    bool aiming;
    if (interfaceGetCurrentHitMode(&hitMode, &aiming) == -1) {
        return;
    }

    MessageListItem messageListItem;
    Object* item;
    char formattedText[80];
    const char* sfx;

    int rc = sub_426614(gDude, a1, hitMode, aiming);
    switch (rc) {
    case 1:
        item = critterGetWeaponForHitMode(gDude, hitMode);
        messageListItem.num = 101; // Out of ammo.
        if (messageListGetItem(&gCombatMessageList, &messageListItem)) {
            displayMonitorAddMessage(messageListItem.text);
        }

        sfx = sfxBuildWeaponName(WEAPON_SOUND_EFFECT_OUT_OF_AMMO, item, hitMode, NULL);
        soundPlayFile(sfx);
        return;
    case 2:
        messageListItem.num = 102; // Target out of range.
        if (messageListGetItem(&gCombatMessageList, &messageListItem)) {
            displayMonitorAddMessage(messageListItem.text);
        }
        return;
    case 3:
        item = critterGetWeaponForHitMode(gDude, hitMode);
        messageListItem.num = 100; // You need %d action points.
        if (messageListGetItem(&gCombatMessageList, &messageListItem)) {
            int actionPointsRequired = sub_478B24(gDude, hitMode, aiming);
            sprintf(formattedText, messageListItem.text, actionPointsRequired);
            displayMonitorAddMessage(formattedText);
        }
        return;
    case 4:
        return;
    case 5:
        messageListItem.num = 104; // Your aim is blocked.
        if (messageListGetItem(&gCombatMessageList, &messageListItem)) {
            displayMonitorAddMessage(messageListItem.text);
        }
        return;
    case 6:
        messageListItem.num = 106; // You cannot use two-handed weapons with a crippled arm.
        if (messageListGetItem(&gCombatMessageList, &messageListItem)) {
            displayMonitorAddMessage(messageListItem.text);
        }
        return;
    case 7:
        messageListItem.num = 105; // You cannot use weapons with both arms crippled.
        if (messageListGetItem(&gCombatMessageList, &messageListItem)) {
            displayMonitorAddMessage(messageListItem.text);
        }
        return;
    }

    if (!isInCombat()) {
        STRUCT_664980 stru;
        stru.attacker = gDude;
        stru.defender = a1;
        stru.actionPointsBonus = 0;
        stru.accuracyBonus = 0;
        stru.damageBonus = 0;
        stru.minDamage = 0;
        stru.maxDamage = INT_MAX;
        stru.field_1C = 0;
        sub_422D2C(&stru);
        return;
    }

    if (!aiming) {
        sub_422F3C(gDude, a1, hitMode, HIT_LOCATION_UNCALLED);
        return;
    }

    if (aiming != 1) {
        debugPrint("Bad called shot value %d\n", aiming);
    }

    int hitLocation;
    if (calledShotSelectHitLocation(a1, &hitLocation, hitMode) != -1) {
        sub_422F3C(gDude, a1, hitMode, hitLocation);
    }
}

// Highlights critters.
//
// 0x426AA8
void sub_426AA8()
{
    int targetHighlight = TARGET_HIGHLIGHT_TARGETING_ONLY;
    configGetInt(&gGameConfig, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_TARGET_HIGHLIGHT_KEY, &targetHighlight);
    if (targetHighlight == TARGET_HIGHLIGHT_OFF) {
        return;
    }

    if (gameMouseGetMode() != GAME_MOUSE_MODE_CROSSHAIR) {
        return;
    }

    if (isInCombat()) {
        for (int index = 0; index < dword_56D37C; index++) {
            sub_421D50(off_56D390[index], 1);
        }
    } else {
        Object** critterList;
        int critterListLength = objectListCreate(-1, gElevation, OBJ_TYPE_CRITTER, &critterList);
        for (int index = 0; index < critterListLength; index++) {
            Object* critter = critterList[index];
            if (critter != gDude && (critter->data.critter.combat.results & DAM_DEAD) == 0) {
                sub_421D50(critter, 1);
            }
        }

        if (critterListLength != 0) {
            objectListFree(critterList);
        }
    }

    for (int index = 0; index < dword_56D37C; index++) {
        sub_421D50(off_56D390[index], 1);
    }

    tileWindowRefresh();
}

// 0x426BC0
void sub_426BC0()
{
    int i;
    int v5;
    Object** v9;

    if (gCombatState & 1) {
        for (i = 0; i < dword_56D37C; i++) {
            objectDisableOutline(off_56D390[i], NULL);
        }
    } else {
        v5 = objectListCreate(-1, gElevation, 1, &v9);
        for (i = 0; i < v5; i++) {
            objectDisableOutline(v9[i], NULL);
            objectClearOutline(v9[i], NULL);
        }
        if (v5) {
            objectListFree(v9);
        }
    }

    tileWindowRefresh();
}

// 0x426C64
void sub_426C64()
{
    int targetHighlight = 2;
    configGetInt(&gGameConfig, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_TARGET_HIGHLIGHT_KEY, &targetHighlight);
    if (targetHighlight != dword_56D38C && isInCombat()) {
        if (targetHighlight != 0) {
            if (dword_56D38C == 0) {
                sub_426AA8();
            }
        } else {
            sub_426BC0();
        }
    }

    dword_56D38C = targetHighlight;
}

// Probably calculates line of sight or determines if object can see other object.
//
// 0x426CC4
bool sub_426CC4(Object* a1, int from, int to, Object* a4, int* a5)
{
    if (a5 != NULL) {
        *a5 = 0;
    }

    Object* v9 = a1;
    int current = from;
    while (v9 != NULL && current != to) {
        sub_4163C8(a1, current, to, 0, &v9, 32, sub_48B930);
        if (v9 != NULL) {
            if ((v9->fid & 0xF000000) >> 24 != OBJ_TYPE_CRITTER && v9 != a4) {
                return true;
            }

            if (a5 != NULL) {
                if (v9 != a4) {
                    if (a4 != NULL) {
                        if ((a4->data.critter.combat.results & DAM_DEAD) == 0) {
                            *a5 += 1;

                            if ((a4->flags & 0x0800) != 0) {
                                *a5 += 1;
                            }
                        }
                    }
                }
            }

            if ((v9->flags & 0x0800) != 0) {
                int rotation = tileGetRotationTo(current, to);
                current = tileGetTileInDirection(current, rotation, 1);
            } else {
                current = v9->tile;
            }
        }
    }

    return false;
}

// 0x426D94
int sub_426D94()
{
    if ((gDude->data.critter.combat.results & DAM_DEAD) != 0) {
        return -1;
    }

    if (off_56D380 == NULL) {
        return -1;
    }

    return off_56D380->data.critter.combat.team;
}

// 0x426DB8
int sub_426DB8(Object* a1, Object* a2)
{
    sub_4A6F70(a1, a1->tile, sub_479188(NULL), a1->elevation);
    return 0;
}

// 0x426DDC
void sub_426DDC(Object* obj)
{
    // TODO: Check entire function.
    if (!isInCombat()) {
        return;
    }

    if (dword_56D37C == 0) {
        return;
    }

    int i;
    for (i = 0; i < dword_56D37C; i++) {
        if (obj == off_56D390[i]) {
            break;
        }
    }

    if (i == dword_56D37C) {
        return;
    }

    while (i < (dword_56D37C - 1)) {
        off_56D390[i] = off_56D390[i + 1];
        sub_421850(i + 1, i);
        i++;
    }

    dword_56D37C--;

    off_56D390[dword_56D37C] = obj;

    if (i >= dword_56D394) {
        if (i < (dword_56D384 + dword_56D394)) {
            dword_56D384--;
        }
    } else {
        dword_56D394--;
    }

    obj->data.critter.combat.ap = 0;
    objectClearOutline(obj, NULL);

    obj->data.critter.combat.whoHitMe = NULL;
    sub_42BD28(obj);
}

// 0x426EC4
void sub_426EC4(Object* critter_obj, char* msg)
{
    if (critter_obj != gDude) {
        displayMonitorAddMessage(msg);
        scriptExecProc(critter_obj->sid, SCRIPT_PROC_DESTROY);
        critterKill(critter_obj, -1, 1);
    }
}
