#include "game/combat.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "game/actions.h"
#include "game/anim.h"
#include "game/art.h"
#include "color.h"
#include "game/combatai.h"
#include "core.h"
#include "game/critter.h"
#include "db.h"
#include "debug.h"
#include "game/display.h"
#include "draw.h"
#include "game/elevator.h"
#include "game/game.h"
#include "game/gconfig.h"
#include "game/gmouse.h"
#include "game/gsound.h"
#include "game/intface.h"
#include "game/item.h"
#include "game/loadsave.h"
#include "game/map.h"
#include "memory.h"
#include "game/object.h"
#include "game/perk.h"
#include "game/pipboy.h"
#include "game/proto.h"
#include "queue.h"
#include "random.h"
#include "scripts.h"
#include "skill.h"
#include "stat.h"
#include "text_font.h"
#include "tile.h"
#include "trait.h"
#include "window_manager.h"

#define CALLED_SHOT_WINDOW_X 68
#define CALLED_SHOT_WINDOW_Y 20
#define CALLED_SHOT_WINDOW_WIDTH 504
#define CALLED_SHOT_WINDOW_HEIGHT 309

static void combatInitAIInfoList();
static int combatCopyAIInfo(int srcIndex, int destIndex);
static void combat_begin(Object* a1);
static void combat_begin_extra(Object* a1);
static void combat_update_critters_in_los(int a1);
static void combat_over();
static void combat_add_noncoms();
static int compare_faster(const void* a1, const void* a2);
static void combat_sequence_init(Object* a1, Object* a2);
static void combat_sequence();
static int combat_input();
static void combat_set_move_all();
static int combat_turn(Object* a1, bool a2);
static bool combat_should_end();
static bool check_ranged_miss(Attack* attack);
static int shoot_along_path(Attack* attack, int a2, int a3, int anim);
static int compute_spray(Attack* attack, int accuracy, int* roundsHitMainTargetPtr, int* roundsSpentPtr, int anim);
static int correctAttackForPerks(Attack* attack);
static int compute_attack(Attack* attack);
static int attack_crit_success(Attack* a1);
static int attackFindInvalidFlags(Object* a1, Object* a2);
static int attack_crit_failure(Attack* attack);
static void do_random_cripple(int* flagsPtr);
static int determine_to_hit_func(Object* attacker, int tile, Object* defender, int hitLocation, int hitMode, int a6);
static void compute_damage(Attack* attack, int ammoQuantity, int bonusDamageMultiplier);
static void check_for_death(Object* a1, int a2, int* a3);
static void set_new_results(Object* a1, int a2);
static void damage_object(Object* a1, int damage, bool animated, int a4, Object* a5);
static void combat_display_hit(char* dest, Object* critter_obj, int damage);
static void combat_display_flags(char* a1, int flags, Object* a3);
static void combat_standup(Object* a1);
static void print_tohit(unsigned char* dest, int dest_pitch, int a3);
static char* combat_get_loc_name(Object* critter, int hitLocation);
static void draw_loc_off(int a1, int a2);
static void draw_loc_on(int a1, int a2);
static void draw_loc(int eventCode, int color);
static int get_called_shot_location(Object* critter, int* hitLocation, int hitMode);

// TODO: Remove.
//
// 0x500B50
static char _a_1[] = ".";

// 0x51093C
static int combat_turn_running = 0;

// 0x510940
int combatNumTurns = 0;

// 0x510944
unsigned int combat_state = COMBAT_STATE_0x02;

// 0x510948
static CombatAiInfo* aiInfoList = NULL;

// 0x51094C
STRUCT_664980* gcsd = NULL;

// 0x510950
bool combat_call_display = false;

// Accuracy modifiers for hit locations.
//
// 0x510954
static int hit_location_penalty[HIT_LOCATION_COUNT] = {
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
static CriticalHitDescription crit_succ_eff[KILL_TYPE_COUNT][HIT_LOCATION_COUNT][CRTICIAL_EFFECT_COUNT] = {
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
static CriticalHitDescription pc_crit_succ_eff[HIT_LOCATION_COUNT][CRTICIAL_EFFECT_COUNT] = {
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
static int combat_end_due_to_load = 0;

// 0x517F9C
static bool combat_cleanup_enabled = false;

// Provides effects caused by failing weapons.
//
// 0x517FA0
int cf_table[WEAPON_CRITICAL_FAILURE_TYPE_COUNT][WEAPON_CRITICAL_FAILURE_EFFECT_COUNT] = {
    { 0, DAM_LOSE_TURN, DAM_LOSE_TURN, DAM_HURT_SELF | DAM_KNOCKED_DOWN, DAM_CRIP_RANDOM },
    { 0, DAM_LOSE_TURN, DAM_DROP, DAM_RANDOM_HIT, DAM_HIT_SELF },
    { 0, DAM_LOSE_AMMO, DAM_DROP, DAM_RANDOM_HIT, DAM_DESTROY },
    { DAM_LOSE_TURN, DAM_LOSE_TURN | DAM_LOSE_AMMO, DAM_DROP | DAM_LOSE_TURN, DAM_RANDOM_HIT, DAM_EXPLODE | DAM_LOSE_TURN },
    { DAM_DUD, DAM_DROP, DAM_DROP | DAM_HURT_SELF, DAM_RANDOM_HIT, DAM_EXPLODE },
    { DAM_LOSE_TURN, DAM_DUD, DAM_DESTROY, DAM_RANDOM_HIT, DAM_EXPLODE | DAM_LOSE_TURN | DAM_KNOCKED_DOWN },
    { 0, DAM_LOSE_TURN, DAM_RANDOM_HIT, DAM_DESTROY, DAM_EXPLODE | DAM_LOSE_TURN | DAM_ON_FIRE },
};

// 0x51802C
static int call_ty[4] = {
    122,
    188,
    251,
    316,
};

// 0x51803C
static int hit_loc_left[4] = {
    HIT_LOCATION_HEAD,
    HIT_LOCATION_EYES,
    HIT_LOCATION_RIGHT_ARM,
    HIT_LOCATION_RIGHT_LEG,
};

// 0x51804C
static int hit_loc_right[4] = {
    HIT_LOCATION_TORSO,
    HIT_LOCATION_GROIN,
    HIT_LOCATION_LEFT_ARM,
    HIT_LOCATION_LEFT_LEG,
};

// 0x56D2B0
static Attack main_ctd;

// combat.msg
//
// 0x56D368
MessageList combat_message_file;

// 0x56D370
static Object* call_target;

// 0x56D374
static int call_win;

// 0x56D378
static int combat_elev;

// 0x56D37C
static int list_total;

// Probably last who_hit_me of obj_dude
//
// 0x56D380
static Object* combat_ending_guy;

// 0x56D384
static int list_noncom;

// 0x56D388
Object* combat_turn_obj;

// target_highlight
//
// 0x56D38C
static int combat_highlight;

// 0x56D390
static Object** combat_list;

// 0x56D394
static int list_com;

// Experience received for killing critters during current combat.
//
// 0x56D398
int combat_exps;

// bonus action points from BONUS_MOVE perk.
//
// 0x56D39C
int combat_free_move;

// combat_init
// 0x420CC0
int combat_init()
{
    int max_action_points;
    char path[MAX_PATH];

    combat_turn_running = 0;
    combatNumTurns = 0;
    combat_list = 0;
    aiInfoList = 0;
    list_com = 0;
    list_noncom = 0;
    list_total = 0;
    gcsd = 0;
    combat_call_display = 0;
    combat_state = COMBAT_STATE_0x02;

    max_action_points = critterGetStat(obj_dude, STAT_MAXIMUM_ACTION_POINTS);

    combat_free_move = 0;
    combat_ending_guy = NULL;
    combat_end_due_to_load = 0;

    obj_dude->data.critter.combat.ap = max_action_points;

    combat_cleanup_enabled = 0;

    if (!message_init(&combat_message_file)) {
        return -1;
    }

    sprintf(path, "%s%s", msg_path, "combat.msg");

    if (!message_load(&combat_message_file, path)) {
        return -1;
    }

    return 0;
}

// 0x420DA0
void combat_reset()
{
    int max_action_points;

    combat_turn_running = 0;
    combatNumTurns = 0;
    combat_list = 0;
    aiInfoList = 0;
    list_com = 0;
    list_noncom = 0;
    list_total = 0;
    gcsd = 0;
    combat_call_display = 0;
    combat_state = COMBAT_STATE_0x02;

    max_action_points = critterGetStat(obj_dude, STAT_MAXIMUM_ACTION_POINTS);

    combat_free_move = 0;
    combat_ending_guy = NULL;

    obj_dude->data.critter.combat.ap = max_action_points;
}

// 0x420E14
void combat_exit()
{
    message_exit(&combat_message_file);
}

// 0x420E24
int find_cid(int a1, int cid, Object** critterList, int critterListLength)
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
int combat_load(File* stream)
{
    int v14;
    int a2;
    Object* obj;
    int v24;
    int i;
    int j;

    if (fileReadUInt32(stream, &combat_state) == -1) return -1;

    if (!isInCombat()) {
        obj = obj_find_first();
        while (obj != NULL) {
            if (PID_TYPE(obj->pid) == OBJ_TYPE_CRITTER) {
                if (obj->data.critter.combat.whoHitMeCid == -1) {
                    obj->data.critter.combat.whoHitMe = NULL;
                }
            }
            obj = obj_find_next();
        }
        return 0;
    }

    if (fileReadInt32(stream, &combat_turn_running) == -1) return -1;
    if (fileReadInt32(stream, &combat_free_move) == -1) return -1;
    if (fileReadInt32(stream, &combat_exps) == -1) return -1;
    if (fileReadInt32(stream, &list_com) == -1) return -1;
    if (fileReadInt32(stream, &list_noncom) == -1) return -1;
    if (fileReadInt32(stream, &list_total) == -1) return -1;

    if (obj_create_list(-1, map_elevation, 1, &combat_list) != list_total) {
        obj_delete_list(combat_list);
        return -1;
    }

    if (fileReadInt32(stream, &v24) == -1) return -1;

    obj_dude->cid = v24;

    for (i = 0; i < list_total; i++) {
        if (combat_list[i]->data.critter.combat.whoHitMeCid == -1) {
            combat_list[i]->data.critter.combat.whoHitMe = NULL;
        } else {
            for (j = 0; j < list_total; j++) {
                if (combat_list[i]->data.critter.combat.whoHitMeCid == combat_list[j]->cid) {
                    break;
                }
            }

            if (j == list_total) {
                combat_list[i]->data.critter.combat.whoHitMe = NULL;
            } else {
                combat_list[i]->data.critter.combat.whoHitMe = combat_list[j];
            }
        }
    }

    for (i = 0; i < list_total; i++) {
        if (fileReadInt32(stream, &v24) == -1) return -1;

        for (j = i; j < list_total; j++) {
            if (v24 == combat_list[j]->cid) {
                break;
            }
        }

        if (j == list_total) {
            return -1;
        }

        obj = combat_list[i];
        combat_list[i] = combat_list[j];
        combat_list[j] = obj;
    }

    for (i = 0; i < list_total; i++) {
        combat_list[i]->cid = i;
    }

    if (aiInfoList) {
        internal_free(aiInfoList);
    }

    aiInfoList = (CombatAiInfo*)internal_malloc(sizeof(*aiInfoList) * list_total);
    if (aiInfoList == NULL) {
        return -1;
    }

    for (v14 = 0; v14 < list_total; v14++) {
        CombatAiInfo* aiInfo = &(aiInfoList[v14]);

        if (fileReadInt32(stream, &a2) == -1) return -1;

        if (a2 == -1) {
            aiInfo->friendlyDead = NULL;
        } else {
            aiInfo->friendlyDead = objFindObjPtrFromID(a2);
            if (aiInfo->friendlyDead == NULL) return -1;
        }

        if (fileReadInt32(stream, &a2) == -1) return -1;

        if (a2 == -1) {
            aiInfo->lastTarget = NULL;
        } else {
            aiInfo->lastTarget = objFindObjPtrFromID(a2);
            if (aiInfo->lastTarget == NULL) return -1;
        }

        if (fileReadInt32(stream, &a2) == -1) return -1;

        if (a2 == -1) {
            aiInfo->lastItem = NULL;
        } else {
            aiInfo->lastItem = objFindObjPtrFromID(a2);
            if (aiInfo->lastItem == NULL) return -1;
        }

        if (fileReadInt32(stream, &(aiInfo->lastMove)) == -1) return -1;
    }

    combat_begin_extra(obj_dude);

    return 0;
}

// 0x421244
int combat_save(File* stream)
{
    if (fileWriteInt32(stream, combat_state) == -1) return -1;

    if (!isInCombat()) return 0;

    if (fileWriteInt32(stream, combat_turn_running) == -1) return -1;
    if (fileWriteInt32(stream, combat_free_move) == -1) return -1;
    if (fileWriteInt32(stream, combat_exps) == -1) return -1;
    if (fileWriteInt32(stream, list_com) == -1) return -1;
    if (fileWriteInt32(stream, list_noncom) == -1) return -1;
    if (fileWriteInt32(stream, list_total) == -1) return -1;
    if (fileWriteInt32(stream, obj_dude->cid) == -1) return -1;

    for (int index = 0; index < list_total; index++) {
        if (fileWriteInt32(stream, combat_list[index]->cid) == -1) return -1;
    }

    if (aiInfoList == NULL) {
        return -1;
    }

    for (int index = 0; index < list_total; index++) {
        CombatAiInfo* aiInfo = &(aiInfoList[index]);

        if (fileWriteInt32(stream, aiInfo->friendlyDead != NULL ? aiInfo->friendlyDead->id : -1) == -1) return -1;
        if (fileWriteInt32(stream, aiInfo->lastTarget != NULL ? aiInfo->lastTarget->id : -1) == -1) return -1;
        if (fileWriteInt32(stream, aiInfo->lastItem != NULL ? aiInfo->lastItem->id : -1) == -1) return -1;
        if (fileWriteInt32(stream, aiInfo->lastMove) == -1) return -1;
    }

    return 0;
}

// 0x4213E8
bool combat_safety_invalidate_weapon(Object* attacker, Object* weapon, int hitMode, Object* defender, int* safeDistancePtr)
{
    return combat_safety_invalidate_weapon_func(attacker, weapon, hitMode, defender, safeDistancePtr, NULL);
}

// 0x4213FC
bool combat_safety_invalidate_weapon_func(Object* attacker, Object* weapon, int hitMode, Object* defender, int* safeDistancePtr, Object* attackerFriend)
{
    if (safeDistancePtr != NULL) {
        *safeDistancePtr = 0;
    }

    if (attacker->pid == PROTO_ID_0x10001E0) {
        return false;
    }

    int intelligence = critterGetStat(attacker, STAT_INTELLIGENCE);
    int team = attacker->data.critter.combat.team;
    int damageRadius = item_w_area_damage_radius(weapon, hitMode);
    int maxDamage;
    item_w_damage_min_max(weapon, NULL, &maxDamage);
    int damageType = item_w_damage_type(attacker, weapon);

    if (damageRadius > 0) {
        if (intelligence < 5) {
            damageRadius -= 5 - intelligence;
            if (damageRadius < 0) {
                damageRadius = 0;
            }
        }

        if (attackerFriend != NULL) {
            if (obj_dist(defender, attackerFriend) < damageRadius) {
                debugPrint("Friendly was in the way!");
                return true;
            }
        }

        for (int index = 0; index < list_total; index++) {
            Object* candidate = combat_list[index];
            if (candidate->data.critter.combat.team == team
                && candidate != attacker
                && candidate != defender
                && !critter_is_dead(candidate)) {
                int attackerDefenderDistance = obj_dist(defender, candidate);
                if (attackerDefenderDistance < damageRadius && candidate != candidate->data.critter.combat.whoHitMe) {
                    int damageThreshold = critterGetStat(candidate, STAT_DAMAGE_THRESHOLD + damageType);
                    int damageResistance = critterGetStat(candidate, STAT_DAMAGE_RESISTANCE + damageType);
                    if (damageResistance * (maxDamage - damageThreshold) / 100 > 0) {
                        return true;
                    }
                }
            }
        }

        if (obj_dist(defender, attacker) <= damageRadius) {
            if (safeDistancePtr != NULL) {
                *safeDistancePtr = damageRadius - obj_dist(defender, attacker) + 1;
                return false;
            }

            return true;
        }

        return false;
    }

    int anim = item_w_anim_weap(weapon, hitMode);
    if (anim != ANIM_FIRE_BURST && anim != ANIM_FIRE_CONTINUOUS) {
        return false;
    }

    Attack attack;
    combat_ctd_init(&attack, attacker, defender, hitMode, HIT_LOCATION_TORSO);

    int accuracy = determine_to_hit_func(attacker, attacker->tile, defender, HIT_LOCATION_TORSO, hitMode, 1);
    int roundsHitMainTarget;
    int roundsSpent;
    compute_spray(&attack, accuracy, &roundsHitMainTarget, &roundsSpent, anim);

    if (attackerFriend != NULL) {
        for (int index = 0; index < attack.extrasLength; index++) {
            if (attack.extras[index] == attackerFriend) {
                debugPrint("Friendly was in the way!");
                return true;
            }
        }
    }

    for (int index = 0; index < attack.extrasLength; index++) {
        Object* candidate = attack.extras[index];
        if (candidate->data.critter.combat.team == team
            && candidate != attacker
            && candidate != defender
            && !critter_is_dead(candidate)
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
bool combatTestIncidentalHit(Object* attacker, Object* defender, Object* attackerFriend, Object* weapon)
{
    return combat_safety_invalidate_weapon_func(attacker, weapon, HIT_MODE_RIGHT_WEAPON_PRIMARY, defender, NULL, attackerFriend);
}

// 0x4217D4
Object* combat_whose_turn()
{
    if (isInCombat()) {
        return combat_turn_obj;
    } else {
        return NULL;
    }
}

// 0x4217E8
void combat_data_init(Object* obj)
{
    obj->data.critter.combat.damageLastTurn = 0;
    obj->data.critter.combat.results = 0;
}

// NOTE: Inlined.
//
// 0x4217FC
static void combatInitAIInfoList()
{
    int index;

    for (index = 0; index < list_total; index++) {
        aiInfoList[index].friendlyDead = NULL;
        aiInfoList[index].lastTarget = NULL;
        aiInfoList[index].lastItem = NULL;
        aiInfoList[index].lastMove = 0;
    }
}

// 0x421850
static int combatCopyAIInfo(int srcIndex, int destIndex)
{
    CombatAiInfo* src = &aiInfoList[srcIndex];
    CombatAiInfo* dest = &aiInfoList[destIndex];

    dest->friendlyDead = src->friendlyDead;
    dest->lastTarget = src->lastTarget;
    dest->lastItem = src->lastItem;
    dest->lastMove = src->lastMove;

    return 0;
}

// 0x421880
Object* combatAIInfoGetFriendlyDead(Object* obj)
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

    return aiInfoList[obj->cid].friendlyDead;
}

// 0x4218AC
int combatAIInfoSetFriendlyDead(Object* a1, Object* a2)
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

    aiInfoList[a1->cid].friendlyDead = a2;

    return 0;
}

// 0x4218EC
Object* combatAIInfoGetLastTarget(Object* obj)
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

    return aiInfoList[obj->cid].lastTarget;
}

// 0x421918
int combatAIInfoSetLastTarget(Object* a1, Object* a2)
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

    if (critter_is_dead(a2)) {
        a2 = NULL;
    }

    aiInfoList[a1->cid].lastTarget = a2;

    return 0;
}

// 0x42196C
Object* combatAIInfoGetLastItem(Object* obj)
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

    return aiInfoList[v1].lastItem;
}

// 0x421998
int combatAIInfoSetLastItem(Object* obj, Object* a2)
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

    aiInfoList[v2].lastItem = NULL;

    return 0;
}

// NOTE: Unused.
//
// 0x4219CC
int combatAIInfoGetLastMove(Object* object)
{
    if (!isInCombat()) {
        return 0;
    }

    if (object == NULL) {
        return -1;
    }

    if (object->cid == -1) {
        return -1;
    }

    return aiInfoList[object->cid].lastMove;
}

// NOTE: Inlined.
//
// 0x421A00
int combatAIInfoSetLastMove(Object* object, int move)
{
    if (!isInCombat()) {
        return 0;
    }

    if (object == NULL) {
        return -1;
    }

    if (object->cid == -1) {
        return -1;
    }

    aiInfoList[object->cid].lastMove = move;

    return 0;
}

// 0x421A34
static void combat_begin(Object* a1)
{
    combat_turn_running = 0;
    anim_stop();
    tickersRemove(dude_fidget);
    combat_elev = map_elevation;

    if (!isInCombat()) {
        combatNumTurns = 0;
        combat_exps = 0;
        combat_list = NULL;
        list_total = obj_create_list(-1, combat_elev, OBJ_TYPE_CRITTER, &combat_list);
        list_noncom = list_total;
        list_com = 0;
        aiInfoList = (CombatAiInfo*)internal_malloc(sizeof(*aiInfoList) * list_total);
        if (aiInfoList == NULL) {
            return;
        }

        // NOTE: Uninline.
        combatInitAIInfoList();

        Object* v1 = NULL;
        for (int index = 0; index < list_total; index++) {
            Object* critter = combat_list[index];
            CritterCombatData* combatData = &(critter->data.critter.combat);
            combatData->maneuver &= CRITTER_MANEUVER_0x01;
            combatData->damageLastTurn = 0;
            combatData->whoHitMe = NULL;
            combatData->ap = 0;
            critter->cid = index;

            // NOTE: Uninline.
            combatAIInfoSetLastMove(critter, 0);

            scriptSetObjects(critter->sid, NULL, NULL);
            scriptSetFixedParam(critter->sid, 0);
            if (critter->pid == 0x1000098) {
                if (!critter_is_dead(critter)) {
                    v1 = critter;
                }
            }
        }

        combat_state |= COMBAT_STATE_0x01;

        tileWindowRefresh();
        game_ui_disable(0);
        gmouse_set_cursor(MOUSE_CURSOR_WAIT_WATCH);
        combat_ending_guy = NULL;
        combat_begin_extra(a1);
        caiTeamCombatInit(combat_list, list_total);
        intface_end_window_open(true);
        gmouse_enable_scrolling();

        if (v1 != NULL && !isLoadingGame()) {
            int fid = art_id(FID_TYPE(v1->fid),
                100,
                FID_ANIM_TYPE(v1->fid),
                (v1->fid & 0xF000) >> 12,
                (v1->fid & 0x70000000) >> 28);

            register_clear(v1);
            register_begin(ANIMATION_REQUEST_RESERVED);
            register_object_animate(v1, ANIM_UP_STAIRS_RIGHT, -1);
            register_object_change_fid(v1, fid, -1);
            register_end();

            while (anim_busy(v1)) {
                _process_bk();
            }
        }
    }
}

// 0x421C8C
static void combat_begin_extra(Object* a1)
{
    for (int index = 0; index < list_total; index++) {
        combat_update_critter_outline_for_los(combat_list[index], 0);
    }

    combat_ctd_init(&main_ctd, a1, NULL, 4, 3);

    combat_turn_obj = a1;

    combat_ai_begin(list_total, combat_list);

    combat_highlight = 2;
    config_get_value(&game_config, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_TARGET_HIGHLIGHT_KEY, &combat_highlight);
}

// NOTE: Inlined.
//
// 0x421D18
static void combat_update_critters_in_los(int a1)
{
    int index;

    for (index = 0; index < list_total; index++) {
        combat_update_critter_outline_for_los(combat_list[index], a1);
    }
}

// Something with outlining.
//
// 0x421D50
void combat_update_critter_outline_for_los(Object* critter, bool a2)
{
    if (PID_TYPE(critter->pid) != OBJ_TYPE_CRITTER) {
        return;
    }

    if (critter == obj_dude) {
        return;
    }

    if (critter_is_dead(critter)) {
        return;
    }

    bool v5 = false;
    if (!combat_is_shot_blocked(obj_dude, obj_dude->tile, critter->tile, critter, 0)) {
        v5 = true;
    }

    if (v5) {
        int outlineType = critter->outline & OUTLINE_TYPE_MASK;
        if (outlineType != OUTLINE_TYPE_HOSTILE && outlineType != OUTLINE_TYPE_FRIENDLY) {
            int newOutlineType = obj_dude->data.critter.combat.team == critter->data.critter.combat.team
                ? OUTLINE_TYPE_FRIENDLY
                : OUTLINE_TYPE_HOSTILE;
            obj_turn_off_outline(critter, NULL);
            obj_remove_outline(critter, NULL);
            obj_outline_object(critter, newOutlineType, NULL);
            if (a2) {
                obj_turn_on_outline(critter, NULL);
            } else {
                obj_turn_off_outline(critter, NULL);
            }
        } else {
            if (critter->outline != 0 && (critter->outline & OUTLINE_DISABLED) == 0) {
                if (!a2) {
                    obj_turn_off_outline(critter, NULL);
                }
            } else {
                if (a2) {
                    obj_turn_on_outline(critter, NULL);
                }
            }
        }
    } else {
        int v7 = obj_dist(obj_dude, critter);
        int v8 = critterGetStat(obj_dude, STAT_PERCEPTION) * 5;
        if ((critter->flags & OBJECT_TRANS_GLASS) != 0) {
            v8 /= 2;
        }

        if (v7 <= v8) {
            v5 = true;
        }

        int outlineType = critter->outline & OUTLINE_TYPE_MASK;
        if (outlineType != OUTLINE_TYPE_32) {
            obj_turn_off_outline(critter, NULL);
            obj_remove_outline(critter, NULL);

            if (v5) {
                obj_outline_object(critter, OUTLINE_TYPE_32, NULL);

                if (a2) {
                    obj_turn_on_outline(critter, NULL);
                } else {
                    obj_turn_off_outline(critter, NULL);
                }
            }
        } else {
            if (critter->outline != 0 && (critter->outline & OUTLINE_DISABLED) == 0) {
                if (!a2) {
                    obj_turn_off_outline(critter, NULL);
                }
            } else {
                if (a2) {
                    obj_turn_on_outline(critter, NULL);
                }
            }
        }
    }
}

// Probably complete combat sequence.
//
// 0x421EFC
static void combat_over()
{
    if (game_user_wants_to_quit == 0) {
        for (int index = 0; index < list_com; index++) {
            Object* critter = combat_list[index];
            if (critter != obj_dude) {
                cai_attempt_w_reload(critter, 0);
            }
        }
    }

    tickersAdd(dude_fidget);

    for (int index = 0; index < list_noncom + list_com; index++) {
        Object* critter = combat_list[index];
        critter->data.critter.combat.damageLastTurn = 0;
        critter->data.critter.combat.maneuver = CRITTER_MANEUVER_NONE;
    }

    for (int index = 0; index < list_total; index++) {
        Object* critter = combat_list[index];
        critter->data.critter.combat.ap = 0;
        obj_remove_outline(critter, NULL);
        critter->data.critter.combat.whoHitMe = NULL;

        scriptSetObjects(critter->sid, NULL, NULL);
        scriptSetFixedParam(critter->sid, 0);

        if (critter->pid == 0x1000098 && !critter_is_dead(critter) && !isLoadingGame()) {
            int fid = art_id(FID_TYPE(critter->fid),
                99,
                FID_ANIM_TYPE(critter->fid),
                (critter->fid & 0xF000) >> 12,
                (critter->fid & 0x70000000) >> 28);
            register_clear(critter);
            register_begin(ANIMATION_REQUEST_RESERVED);
            register_object_animate(critter, ANIM_UP_STAIRS_RIGHT, -1);
            register_object_change_fid(critter, fid, -1);
            register_end();

            while (anim_busy(critter)) {
                _process_bk();
            }
        }
    }

    tileWindowRefresh();

    int leftItemAction;
    int rightItemAction;
    intface_get_item_states(&leftItemAction, &rightItemAction);
    intface_update_items(true, leftItemAction, rightItemAction);

    obj_dude->data.critter.combat.ap = critterGetStat(obj_dude, STAT_MAXIMUM_HIT_POINTS);

    intface_update_move_points(0, 0);

    if (game_user_wants_to_quit == 0) {
        combat_give_exps(combat_exps);
    }

    combat_exps = 0;

    combat_state &= ~(COMBAT_STATE_0x01 | COMBAT_STATE_0x02);
    combat_state |= COMBAT_STATE_0x02;

    if (list_total != 0) {
        obj_delete_list(combat_list);

        if (aiInfoList != NULL) {
            internal_free(aiInfoList);
        }
        aiInfoList = NULL;
    }

    list_total = 0;

    combat_ai_over();
    game_ui_enable();
    gmouse_3d_set_mode(GAME_MOUSE_MODE_MOVE);
    intface_update_ac(true);

    if (critter_is_prone(obj_dude) && !critter_is_dead(obj_dude) && combat_ending_guy == NULL) {
        queueRemoveEventsByType(obj_dude, EVENT_TYPE_KNOCKOUT);
        critter_wake_up(obj_dude, NULL);
    }
}

// 0x422194
void combat_over_from_load()
{
    combat_over();
    combat_state = 0;
    combat_end_due_to_load = 1;
}

// Give exp for destroying critter.
//
// 0x4221B4
void combat_give_exps(int exp_points)
{
    MessageListItem v7;
    MessageListItem v9;
    int current_hp;
    int max_hp;
    char text[132];

    if (exp_points <= 0) {
        return;
    }

    if (critter_is_dead(obj_dude)) {
        return;
    }

    pcAddExperience(exp_points);

    v7.num = 621; // %s you earn %d exp. points.
    if (!message_search(&proto_main_msg_file, &v7)) {
        return;
    }

    v9.num = randomBetween(0, 3) + 622; // generate prefix for message

    current_hp = critterGetStat(obj_dude, STAT_CURRENT_HIT_POINTS);
    max_hp = critterGetStat(obj_dude, STAT_MAXIMUM_HIT_POINTS);
    if (current_hp == max_hp && randomBetween(0, 100) > 65) {
        v9.num = 626; // Best possible prefix: For destroying your enemies without taking a scratch,
    }

    if (!message_search(&proto_main_msg_file, &v9)) {
        return;
    }

    sprintf(text, v7.text, v9.text, exp_points);
    display_print(text);
}

// 0x4222A8
static void combat_add_noncoms()
{
    combatai_notify_friends(obj_dude);

    for (int index = list_com; index < list_com + list_noncom; index++) {
        Object* obj = combat_list[index];
        if (combatai_want_to_join(obj)) {
            obj->data.critter.combat.maneuver = CRITTER_MANEUVER_NONE;

            Object** objectPtr1 = &(combat_list[index]);
            Object** objectPtr2 = &(combat_list[list_com]);
            Object* t = *objectPtr1;
            *objectPtr1 = *objectPtr2;
            *objectPtr2 = t;

            list_com += 1;
            list_noncom -= 1;

            int actionPoints = 0;
            if (obj != obj_dude) {
                actionPoints = critterGetStat(obj, STAT_MAXIMUM_ACTION_POINTS);
            }

            if (gcsd != NULL) {
                actionPoints += gcsd->actionPointsBonus;
            }

            obj->data.critter.combat.ap = actionPoints;

            combat_turn(obj, false);
        }
    }
}

// NOTE: Unused.
//
// 0x422374
int combat_in_range(Object* critter)
{
    int perception;
    int index;

    perception = critterGetStat(critter, STAT_PERCEPTION);

    for (index = 0; index < list_com; index++) {
        if (obj_dist(combat_list[index], critter) <= perception) {
            return 1;
        }
    }

    return 0;
}

// Compares critters by sequence.
//
// 0x4223C8
static int compare_faster(const void* a1, const void* a2)
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
static void combat_sequence_init(Object* a1, Object* a2)
{
    int next = 0;
    if (a1 != NULL) {
        for (int index = 0; index < list_total; index++) {
            Object* obj = combat_list[index];
            if (obj == a1) {
                Object* temp = combat_list[next];
                combat_list[index] = temp;
                combat_list[next] = obj;
                next += 1;
                break;
            }
        }
    }

    if (a2 != NULL) {
        for (int index = 0; index < list_total; index++) {
            Object* obj = combat_list[index];
            if (obj == a2) {
                Object* temp = combat_list[next];
                combat_list[index] = temp;
                combat_list[next] = obj;
                next += 1;
                break;
            }
        }
    }

    if (a1 != obj_dude && a2 != obj_dude) {
        for (int index = 0; index < list_total; index++) {
            Object* obj = combat_list[index];
            if (obj == obj_dude) {
                Object* temp = combat_list[next];
                combat_list[index] = temp;
                combat_list[next] = obj;
                next += 1;
                break;
            }
        }
    }

    list_com = next;
    list_noncom -= next;

    if (a1 != NULL) {
        critter_set_who_hit_me(a1, a2);
    }

    if (a2 != NULL) {
        critter_set_who_hit_me(a2, a1);
    }
}

// 0x422580
static void combat_sequence()
{
    combat_add_noncoms();

    int count = list_com;

    for (int index = 0; index < count; index++) {
        Object* critter = combat_list[index];
        if ((critter->data.critter.combat.results & DAM_DEAD) != 0) {
            combat_list[index] = combat_list[count - 1];
            combat_list[count - 1] = critter;

            combat_list[count - 1] = combat_list[list_noncom + count - 1];
            combat_list[list_noncom + count - 1] = critter;

            index -= 1;
            count -= 1;
        }
    }

    for (int index = 0; index < count; index++) {
        Object* critter = combat_list[index];
        if (critter != obj_dude) {
            if ((critter->data.critter.combat.results & DAM_KNOCKED_OUT) != 0
                || critter->data.critter.combat.maneuver == CRITTER_MANEUVER_STOP_ATTACKING) {
                critter->data.critter.combat.maneuver &= ~CRITTER_MANEUVER_0x01;
                list_noncom += 1;

                combat_list[index] = combat_list[count - 1];
                combat_list[count - 1] = critter;

                count -= 1;
                index -= 1;
            }
        }
    }

    if (count != 0) {
        list_com = count;
        qsort(combat_list, count, sizeof(*combat_list), compare_faster);
        count = list_com;
    }

    list_com = count;

    gameTimeAddSeconds(5);
}

// 0x422694
void combat_end()
{
    if (combat_elev == obj_dude->elevation) {
        MessageListItem messageListItem;
        int dudeTeam = obj_dude->data.critter.combat.team;

        for (int index = 0; index < list_com; index++) {
            Object* critter = combat_list[index];
            if (critter != obj_dude) {
                int critterTeam = critter->data.critter.combat.team;
                Object* critterWhoHitMe = critter->data.critter.combat.whoHitMe;
                if (critterTeam != dudeTeam || (critterWhoHitMe != NULL && critterWhoHitMe->data.critter.combat.team == critterTeam)) {
                    if (!combatai_want_to_stop(critter)) {
                        messageListItem.num = 103;
                        if (message_search(&combat_message_file, &messageListItem)) {
                            display_print(messageListItem.text);
                        }
                        return;
                    }
                }
            }
        }

        for (int index = list_com; index < list_com + list_noncom; index++) {
            Object* critter = combat_list[index];
            if (critter != obj_dude) {
                int critterTeam = critter->data.critter.combat.team;
                Object* critterWhoHitMe = critter->data.critter.combat.whoHitMe;
                if (critterTeam != dudeTeam || (critterWhoHitMe != NULL && critterWhoHitMe->data.critter.combat.team == critterTeam)) {
                    if (combatai_want_to_join(critter)) {
                        messageListItem.num = 103;
                        if (message_search(&combat_message_file, &messageListItem)) {
                            display_print(messageListItem.text);
                        }
                        return;
                    }
                }
            }
        }
    }

    combat_state |= COMBAT_STATE_0x08;
    caiTeamCombatExit();
}

// 0x4227DC
void combat_turn_run()
{
    while (combat_turn_running > 0) {
        _process_bk();
    }
}

// 0x4227F4
static int combat_input()
{
    while ((combat_state & COMBAT_STATE_0x02) != 0) {
        if ((combat_state & COMBAT_STATE_0x08) != 0) {
            break;
        }

        if ((obj_dude->data.critter.combat.results & (DAM_KNOCKED_OUT | DAM_DEAD | DAM_LOSE_TURN)) != 0) {
            break;
        }

        if (game_user_wants_to_quit != 0) {
            break;
        }

        if (combat_end_due_to_load != 0) {
            break;
        }

        int keyCode = _get_input();
        if (action_explode_running()) {
            while (combat_turn_running > 0) {
                _process_bk();
            }
        }

        if (obj_dude->data.critter.combat.ap <= 0 && combat_free_move <= 0) {
            break;
        }

        if (keyCode == KEY_SPACE) {
            break;
        }

        if (keyCode == KEY_RETURN) {
            combat_end();
        } else {
            _scripts_check_state_in_combat();
            game_handle_input(keyCode, true);
        }
    }

    int v4 = game_user_wants_to_quit;
    if (game_user_wants_to_quit == 1) {
        game_user_wants_to_quit = 0;
    }

    if ((combat_state & COMBAT_STATE_0x08) != 0) {
        combat_state &= ~COMBAT_STATE_0x08;
        return -1;
    }

    if (game_user_wants_to_quit != 0 || v4 != 0 || combat_end_due_to_load != 0) {
        return -1;
    }

    _scripts_check_state_in_combat();

    return 0;
}

// NOTE: Unused.
//
// 0x42290C
void combat_end_turn()
{
    combat_state &= ~COMBAT_STATE_0x02;
}

// 0x422914
static void combat_set_move_all()
{
    for (int index = 0; index < list_com; index++) {
        Object* object = combat_list[index];

        int actionPoints = critterGetStat(object, STAT_MAXIMUM_ACTION_POINTS);

        if (gcsd) {
            actionPoints += gcsd->actionPointsBonus;
        }

        object->data.critter.combat.ap = actionPoints;

        // NOTE: Uninline.
        combatAIInfoSetLastMove(object, 0);
    }
}

// 0x42299C
static int combat_turn(Object* a1, bool a2)
{
    combat_turn_obj = a1;

    combat_ctd_init(&main_ctd, a1, NULL, HIT_MODE_PUNCH, HIT_LOCATION_TORSO);

    if ((a1->data.critter.combat.results & (DAM_KNOCKED_OUT | DAM_DEAD | DAM_LOSE_TURN)) != 0) {
        a1->data.critter.combat.results &= ~DAM_LOSE_TURN;
    } else {
        if (a1 == obj_dude) {
            kb_clear();
            intface_update_ac(true);
            combat_free_move = 2 * perk_level(obj_dude, PERK_BONUS_MOVE);
            intface_update_move_points(obj_dude->data.critter.combat.ap, combat_free_move);
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

            if (game_user_wants_to_quit == 1) {
                return -1;
            }
        }

        if (!scriptOverrides) {
            if (!a2 && critter_is_prone(a1)) {
                combat_standup(a1);
            }

            if (a1 == obj_dude) {
                game_ui_enable();
                gmouse_3d_refresh();

                if (gcsd != NULL) {
                    combat_attack_this(gcsd->defender);
                }

                if (!a2) {
                    combat_state |= 0x02;
                }

                intface_end_buttons_enable();

                // NOTE: Uninline.
                combat_update_critters_in_los(0);

                if (combat_highlight != 0) {
                    combat_outline_on();
                }

                if (combat_input() == -1) {
                    game_ui_disable(1);
                    gmouse_set_cursor(MOUSE_CURSOR_WAIT_WATCH);
                    a1->data.critter.combat.damageLastTurn = 0;
                    intface_end_buttons_disable();
                    combat_outline_off();
                    intface_update_move_points(-1, -1);
                    intface_update_ac(true);
                    combat_free_move = 0;
                    return -1;
                }
            } else {
                Rect rect;
                if (obj_turn_on_outline(a1, &rect) == 0) {
                    tileWindowRefreshRect(&rect, a1->elevation);
                }

                combat_ai(a1, gcsd != NULL ? gcsd->defender : NULL);
            }
        }

        while (combat_turn_running > 0) {
            _process_bk();
        }

        if (a1 == obj_dude) {
            game_ui_disable(1);
            gmouse_set_cursor(MOUSE_CURSOR_WAIT_WATCH);
            intface_end_buttons_disable();
            combat_outline_off();
            intface_update_move_points(-1, -1);
            combat_turn_obj = NULL;
            intface_update_ac(true);
            combat_turn_obj = obj_dude;
        } else {
            Rect rect;
            if (obj_turn_off_outline(a1, &rect) == 0) {
                tileWindowRefreshRect(&rect, a1->elevation);
            }
        }
    }

    if ((obj_dude->data.critter.combat.results & DAM_DEAD) != 0) {
        return -1;
    }

    if (a1 != obj_dude || combat_elev == obj_dude->elevation) {
        combat_free_move = 0;
        return 0;
    }

    return -1;
}

// 0x422C60
static bool combat_should_end()
{
    if (list_com <= 1) {
        return true;
    }

    int index;
    for (index = 0; index < list_com; index++) {
        if (combat_list[index] == obj_dude) {
            break;
        }
    }

    if (index == list_com) {
        return true;
    }

    int team = obj_dude->data.critter.combat.team;

    for (index = 0; index < list_com; index++) {
        Object* critter = combat_list[index];
        if (critter->data.critter.combat.team != team) {
            break;
        }

        Object* critterWhoHitMe = critter->data.critter.combat.whoHitMe;
        if (critterWhoHitMe != NULL && critterWhoHitMe->data.critter.combat.team == team) {
            break;
        }
    }

    if (index == list_com) {
        return true;
    }

    return false;
}

// 0x422D2C
void combat(STRUCT_664980* attack)
{
    if (attack == NULL
        || (attack->attacker == NULL || attack->attacker->elevation == map_elevation)
        || (attack->defender == NULL || attack->defender->elevation == map_elevation)) {
        int v3 = combat_state & 0x01;

        combat_begin(NULL);

        int v6;

        // TODO: Not sure.
        if (v3 != 0) {
            if (combat_turn(obj_dude, true) == -1) {
                v6 = -1;
            } else {
                int index;
                for (index = 0; index < list_com; index++) {
                    if (combat_list[index] == obj_dude) {
                        break;
                    }
                }
                v6 = index + 1;
            }
            gcsd = NULL;
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
            combat_sequence_init(v9, v3);
            gcsd = attack;
            v6 = 0;
        }

        do {
            if (v6 == -1) {
                break;
            }

            combat_set_move_all();

            for (; v6 < list_com; v6++) {
                if (combat_turn(combat_list[v6], false) == -1) {
                    break;
                }

                if (combat_ending_guy != NULL) {
                    break;
                }

                gcsd = NULL;
            }

            if (v6 < list_com) {
                break;
            }

            combat_sequence();
            v6 = 0;
            combatNumTurns += 1;
        } while (!combat_should_end());

        if (combat_end_due_to_load) {
            game_ui_enable();
            gmouse_3d_set_mode(GAME_MOUSE_MODE_MOVE);
        } else {
            gmouse_disable_scrolling();
            intface_end_window_close(true);
            gmouse_enable_scrolling();
            combat_over();
            scriptsExecMapUpdateProc();
        }

        combat_end_due_to_load = 0;

        if (game_user_wants_to_quit == 1) {
            game_user_wants_to_quit = 0;
        }
    }
}

// 0x422EC4
void combat_ctd_init(Attack* attack, Object* attacker, Object* defender, int hitMode, int hitLocation)
{
    attack->attacker = attacker;
    attack->hitMode = hitMode;
    attack->weapon = item_hit_with(attacker, hitMode);
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
int combat_attack(Object* a1, Object* a2, int hitMode, int hitLocation)
{
    if (a1 != obj_dude && hitMode == HIT_MODE_PUNCH && randomBetween(1, 4) == 1) {
        int fid = art_id(OBJ_TYPE_CRITTER, a1->fid & 0xFFF, ANIM_KICK_LEG, (a1->fid & 0xF000) >> 12, (a1->fid & 0x70000000) >> 28);
        if (art_exists(fid)) {
            hitMode = HIT_MODE_KICK;
        }
    }

    combat_ctd_init(&main_ctd, a1, a2, hitMode, hitLocation);
    debugPrint("computing attack...\n");

    if (compute_attack(&main_ctd) == -1) {
        return -1;
    }

    if (gcsd != NULL) {
        main_ctd.defenderDamage += gcsd->damageBonus;

        if (main_ctd.defenderDamage < gcsd->minDamage) {
            main_ctd.defenderDamage = gcsd->minDamage;
        }

        if (main_ctd.defenderDamage > gcsd->maxDamage) {
            main_ctd.defenderDamage = gcsd->maxDamage;
        }

        if (gcsd->field_1C) {
            // FIXME: looks like a bug, two different fields are used to set
            // one field.
            main_ctd.defenderFlags = gcsd->field_20;
            main_ctd.defenderFlags = gcsd->field_24;
        }
    }

    bool aiming;
    if (main_ctd.defenderHitLocation == HIT_LOCATION_TORSO || main_ctd.defenderHitLocation == HIT_LOCATION_UNCALLED) {
        if (a1 == obj_dude) {
            intface_get_attack(&hitMode, &aiming);
        } else {
            aiming = false;
        }
    } else {
        aiming = true;
    }

    int actionPoints = item_w_mp_cost(a1, main_ctd.hitMode, aiming);
    debugPrint("sequencing attack...\n");

    if (action_attack(&main_ctd) == -1) {
        return -1;
    }

    if (actionPoints > a1->data.critter.combat.ap) {
        a1->data.critter.combat.ap = 0;
    } else {
        a1->data.critter.combat.ap -= actionPoints;
    }

    if (a1 == obj_dude) {
        intface_update_move_points(a1->data.critter.combat.ap, combat_free_move);
        critter_set_who_hit_me(a1, a2);
    }

    combat_call_display = 1;
    combat_cleanup_enabled = 1;
    combatAIInfoSetLastTarget(a1, a2);
    debugPrint("running attack...\n");

    return 0;
}

// Returns tile one step closer from [a1] to [a2]
//
// 0x423104
int combat_bullet_start(const Object* a1, const Object* a2)
{
    int rotation = tileGetRotationTo(a1->tile, a2->tile);
    return tileGetTileInDirection(a1->tile, rotation, 1);
}

// 0x423128
static bool check_ranged_miss(Attack* attack)
{
    int range = item_w_range(attack->attacker, attack->hitMode);
    int to = _tile_num_beyond(attack->attacker->tile, attack->defender->tile, range);

    int roll = ROLL_FAILURE;
    Object* critter = attack->attacker;
    if (critter != NULL) {
        int curr = attack->attacker->tile;
        while (curr != to) {
            make_straight_path_func(attack->attacker, curr, to, NULL, &critter, 32, obj_shoot_blocking_at);
            if (critter != NULL) {
                if ((critter->flags & OBJECT_SHOOT_THRU) == 0) {
                    if (FID_TYPE(critter->fid) != OBJ_TYPE_CRITTER) {
                        roll = ROLL_SUCCESS;
                        break;
                    }

                    if (critter != attack->defender) {
                        int v6 = determine_to_hit_func(attack->attacker, attack->attacker->tile, critter, attack->defenderHitLocation, attack->hitMode, 1) / 3;
                        if (critter_is_dead(critter)) {
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

    if (roll < ROLL_SUCCESS || critter == NULL || (critter->flags & OBJECT_SHOOT_THRU) == 0) {
        return false;
    }

    attack->defender = critter;
    attack->tile = critter->tile;
    attack->attackerFlags |= DAM_HIT;
    attack->defenderHitLocation = HIT_LOCATION_TORSO;
    compute_damage(attack, 1, 2);
    return true;
}

// 0x423284
static int shoot_along_path(Attack* attack, int endTile, int rounds, int anim)
{
    // 0x56D3A0
    static Attack temp_ctd;

    int remainingRounds = rounds;
    int roundsHitMainTarget = 0;
    int currentTile = attack->attacker->tile;

    Object* critter = attack->attacker;
    while (critter != NULL) {
        if ((remainingRounds <= 0 && anim != ANIM_FIRE_CONTINUOUS) || currentTile == endTile || attack->extrasLength >= 6) {
            break;
        }

        make_straight_path_func(attack->attacker, currentTile, endTile, NULL, &critter, 32, obj_shoot_blocking_at);

        if (critter != NULL) {
            if (FID_TYPE(critter->fid) != OBJ_TYPE_CRITTER) {
                break;
            }

            int accuracy = determine_to_hit_func(attack->attacker, attack->attacker->tile, critter, HIT_LOCATION_TORSO, attack->hitMode, 1);
            if (anim == ANIM_FIRE_CONTINUOUS) {
                remainingRounds = 1;
            }

            int roundsHit = 0;
            while (randomBetween(1, 100) <= accuracy && remainingRounds > 0) {
                remainingRounds -= 1;
                roundsHit += 1;
            }

            if (roundsHit != 0) {
                if (critter == attack->defender) {
                    roundsHitMainTarget += roundsHit;
                } else {
                    int index;
                    for (index = 0; index < attack->extrasLength; index += 1) {
                        if (critter == attack->extras[index]) {
                            break;
                        }
                    }

                    attack->extrasHitLocation[index] = HIT_LOCATION_TORSO;
                    attack->extras[index] = critter;
                    combat_ctd_init(&temp_ctd, attack->attacker, critter, attack->hitMode, HIT_LOCATION_TORSO);
                    temp_ctd.attackerFlags |= DAM_HIT;
                    compute_damage(&temp_ctd, roundsHit, 2);

                    if (index == attack->extrasLength) {
                        attack->extrasDamage[index] = temp_ctd.defenderDamage;
                        attack->extrasFlags[index] = temp_ctd.defenderFlags;
                        attack->extrasKnockback[index] = temp_ctd.defenderKnockback;
                        attack->extrasLength++;
                    } else {
                        if (anim == ANIM_FIRE_BURST) {
                            attack->extrasDamage[index] += temp_ctd.defenderDamage;
                            attack->extrasFlags[index] |= temp_ctd.defenderFlags;
                            attack->extrasKnockback[index] += temp_ctd.defenderKnockback;
                        }
                    }
                }
            }

            currentTile = critter->tile;
        }
    }

    if (anim == ANIM_FIRE_CONTINUOUS) {
        roundsHitMainTarget = 0;
    }

    return roundsHitMainTarget;
}

// 0x423488
static int compute_spray(Attack* attack, int accuracy, int* roundsHitMainTargetPtr, int* roundsSpentPtr, int anim)
{
    *roundsHitMainTargetPtr = 0;

    int ammoQuantity = item_w_curr_ammo(attack->weapon);
    int burstRounds = item_w_rounds(attack->weapon);
    if (burstRounds < ammoQuantity) {
        ammoQuantity = burstRounds;
    }

    *roundsSpentPtr = ammoQuantity;

    int criticalChance = critterGetStat(attack->attacker, STAT_CRITICAL_CHANCE);
    int roll = randomRoll(accuracy, criticalChance, NULL);

    if (roll == ROLL_CRITICAL_FAILURE) {
        return roll;
    }

    if (roll == ROLL_CRITICAL_SUCCESS) {
        accuracy += 20;
    }

    int leftRounds;
    int mainTargetRounds;
    int centerRounds;
    int rightRounds;
    if (anim == ANIM_FIRE_BURST) {
        centerRounds = ammoQuantity / 3;
        if (centerRounds == 0) {
            centerRounds = 1;
        }

        leftRounds = ammoQuantity / 3;
        rightRounds = ammoQuantity - centerRounds - leftRounds;
        mainTargetRounds = centerRounds / 2;
        if (mainTargetRounds == 0) {
            mainTargetRounds = 1;
            centerRounds -= 1;
        }
    } else {
        leftRounds = 1;
        mainTargetRounds = 1;
        centerRounds = 1;
        rightRounds = 1;
    }

    for (int index = 0; index < mainTargetRounds; index += 1) {
        if (randomRoll(accuracy, 0, NULL) >= ROLL_SUCCESS) {
            *roundsHitMainTargetPtr += 1;
        }
    }

    if (*roundsHitMainTargetPtr == 0 && check_ranged_miss(attack)) {
        *roundsHitMainTargetPtr = 1;
    }

    int range = item_w_range(attack->attacker, attack->hitMode);
    int mainTargetEndTile = _tile_num_beyond(attack->attacker->tile, attack->defender->tile, range);
    *roundsHitMainTargetPtr += shoot_along_path(attack, mainTargetEndTile, centerRounds - *roundsHitMainTargetPtr, anim);

    int centerTile;
    if (obj_dist(attack->attacker, attack->defender) <= 3) {
        centerTile = _tile_num_beyond(attack->attacker->tile, attack->defender->tile, 3);
    } else {
        centerTile = attack->defender->tile;
    }

    int rotation = tileGetRotationTo(centerTile, attack->attacker->tile);

    int leftTile = tileGetTileInDirection(centerTile, (rotation + 1) % ROTATION_COUNT, 1);
    int leftEndTile = _tile_num_beyond(attack->attacker->tile, leftTile, range);
    *roundsHitMainTargetPtr += shoot_along_path(attack, leftEndTile, leftRounds, anim);

    int rightTile = tileGetTileInDirection(centerTile, (rotation + 5) % ROTATION_COUNT, 1);
    int rightEndTile = _tile_num_beyond(attack->attacker->tile, rightTile, range);
    *roundsHitMainTargetPtr += shoot_along_path(attack, rightEndTile, rightRounds, anim);

    if (roll != ROLL_FAILURE || (*roundsHitMainTargetPtr <= 0 && attack->extrasLength <= 0)) {
        if (roll >= ROLL_SUCCESS && *roundsHitMainTargetPtr == 0 && attack->extrasLength == 0) {
            roll = ROLL_FAILURE;
        }
    } else {
        roll = ROLL_SUCCESS;
    }

    return roll;
}

// 0x423714
static int correctAttackForPerks(Attack* attack)
{
    if (item_w_perk(attack->weapon) == PERK_WEAPON_ENHANCED_KNOCKOUT) {
        int difficulty = critterGetStat(attack->attacker, STAT_STRENGTH) - 8;
        int chance = randomBetween(1, 100);
        if (chance <= difficulty) {
            Object* weapon = NULL;
            if (attack->defender != obj_dude) {
                weapon = item_hit_with(attack->defender, HIT_MODE_RIGHT_WEAPON_PRIMARY);
            }

            if (!(attackFindInvalidFlags(attack->defender, weapon) & 1)) {
                attack->defenderFlags |= DAM_KNOCKED_OUT;
            }
        }
    }

    return 0;
}

// 0x42378C
static int compute_attack(Attack* attack)
{
    int range = item_w_range(attack->attacker, attack->hitMode);
    int distance = obj_dist(attack->attacker, attack->defender);

    if (range < distance) {
        return -1;
    }

    int anim = item_w_anim(attack->attacker, attack->hitMode);
    int accuracy = determine_to_hit_func(attack->attacker, attack->attacker->tile, attack->defender, attack->defenderHitLocation, attack->hitMode, 1);

    bool isGrenade = false;
    int damageType = item_w_damage_type(attack->attacker, attack->weapon);
    if (anim == ANIM_THROW_ANIM && (damageType == DAMAGE_TYPE_EXPLOSION || damageType == DAMAGE_TYPE_PLASMA || damageType == DAMAGE_TYPE_EMP)) {
        isGrenade = true;
    }

    if (attack->defenderHitLocation == HIT_LOCATION_UNCALLED) {
        attack->defenderHitLocation = HIT_LOCATION_TORSO;
    }

    int attackType = item_w_subtype(attack->weapon, attack->hitMode);
    int roundsHitMainTarget = 1;
    int damageMultiplier = 2;
    int roundsSpent = 1;

    int roll;

    if (anim == ANIM_FIRE_BURST || anim == ANIM_FIRE_CONTINUOUS) {
        roll = compute_spray(attack, accuracy, &roundsHitMainTarget, &roundsSpent, anim);
    } else {
        int chance = critterGetStat(attack->attacker, STAT_CRITICAL_CHANCE);
        roll = randomRoll(accuracy, chance - hit_location_penalty[attack->defenderHitLocation], NULL);
    }

    if (roll == ROLL_FAILURE) {
        if (traitIsSelected(TRAIT_JINXED) || perkHasRank(obj_dude, PERK_JINXED)) {
            if (randomBetween(0, 1) == 1) {
                roll = ROLL_CRITICAL_FAILURE;
            }
        }
    }

    if (roll == ROLL_SUCCESS) {
        if ((attackType == ATTACK_TYPE_MELEE || attackType == ATTACK_TYPE_UNARMED) && attack->attacker == obj_dude) {
            if (perkHasRank(attack->attacker, PERK_SLAYER)) {
                roll = ROLL_CRITICAL_SUCCESS;
            }

            if (perkHasRank(obj_dude, PERK_SILENT_DEATH)
                && !is_hit_from_front(obj_dude, attack->defender)
                && is_pc_flag(DUDE_STATE_SNEAKING)
                && obj_dude != attack->defender->data.critter.combat.whoHitMe) {
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
        attack->ammoQuantity = roundsSpent;

        if (roll == ROLL_SUCCESS && attack->attacker == obj_dude) {
            if (perk_level(obj_dude, PERK_SNIPER) != 0) {
                int d10 = randomBetween(1, 10);
                int luck = critterGetStat(obj_dude, STAT_LUCK);
                if (d10 <= luck) {
                    roll = ROLL_CRITICAL_SUCCESS;
                }
            }
        }
    } else {
        if (item_w_max_ammo(attack->weapon) > 0) {
            attack->ammoQuantity = 1;
        }
    }

    if (item_w_compute_ammo_cost(attack->weapon, &(attack->ammoQuantity)) == -1) {
        return -1;
    }

    switch (roll) {
    case ROLL_CRITICAL_SUCCESS:
        damageMultiplier = attack_crit_success(attack);
        // FALLTHROUGH
    case ROLL_SUCCESS:
        attack->attackerFlags |= DAM_HIT;
        correctAttackForPerks(attack);
        compute_damage(attack, roundsHitMainTarget, damageMultiplier);
        break;
    case ROLL_FAILURE:
        if (attackType == ATTACK_TYPE_RANGED || attackType == ATTACK_TYPE_THROW) {
            check_ranged_miss(attack);
        }
        break;
    case ROLL_CRITICAL_FAILURE:
        attack_crit_failure(attack);
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
                tile = _tile_num_beyond(attack->attacker->tile, attack->defender->tile, range);
            }

            attack->tile = tile;

            Object* v25 = attack->defender;
            make_straight_path_func(v25, attack->defender->tile, attack->tile, NULL, &v25, 32, obj_shoot_blocking_at);
            if (v25 != NULL && v25 != attack->defender) {
                attack->tile = v25->tile;
            } else {
                v25 = obj_blocking_at(NULL, attack->tile, attack->defender->elevation);
            }

            if (v25 != NULL && (v25->flags & OBJECT_SHOOT_THRU) == 0) {
                attack->attackerFlags |= DAM_HIT;
                attack->defender = v25;
                compute_damage(attack, 1, 2);
            }
        }
    }

    if ((damageType == DAMAGE_TYPE_EXPLOSION || isGrenade) && ((attack->attackerFlags & DAM_HIT) != 0 || (attack->attackerFlags & DAM_CRITICAL) == 0)) {
        compute_explosion_on_extras(attack, 0, isGrenade, 0);
    } else {
        if ((attack->attackerFlags & DAM_EXPLODE) != 0) {
            compute_explosion_on_extras(attack, 1, isGrenade, 0);
        }
    }

    death_checks(attack);

    return 0;
}

// compute_explosion_on_extras
// 0x423C10
void compute_explosion_on_extras(Attack* attack, int a2, bool isGrenade, int a4)
{
    // 0x56D458
    Attack temp_ctd;

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
            if (isGrenade && item_w_grenade_dmg_radius(attack->weapon) < v22) {
                v5 = -1;
            } else if (isGrenade || item_w_rocket_dmg_radius(attack->weapon) >= v22) {
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

        Object* obstacle = obj_blocking_at(attacker, v5, attack->attacker->elevation);
        if (obstacle != NULL
            && FID_TYPE(obstacle->fid) == OBJ_TYPE_CRITTER
            && (obstacle->data.critter.combat.results & DAM_DEAD) == 0
            && (obstacle->flags & OBJECT_SHOOT_THRU) == 0
            && !combat_is_shot_blocked(obstacle, obstacle->tile, tile, NULL, NULL)) {
            if (obstacle == attack->attacker) {
                attack->attackerFlags &= ~DAM_HIT;
                compute_damage(attack, 1, 2);
                attack->attackerFlags |= DAM_HIT;
                attack->attackerFlags |= DAM_BACKWASH;
            } else {
                int index;
                for (index = 0; index < attack->extrasLength; index++) {
                    if (attack->extras[index] == obstacle) {
                        break;
                    }
                }

                if (index == attack->extrasLength) {
                    attack->extrasHitLocation[index] = HIT_LOCATION_TORSO;
                    attack->extras[index] = obstacle;
                    combat_ctd_init(&temp_ctd, attack->attacker, obstacle, attack->hitMode, HIT_LOCATION_TORSO);
                    if (!a4) {
                        temp_ctd.attackerFlags |= DAM_HIT;
                        compute_damage(&temp_ctd, 1, 2);
                    }

                    attack->extrasDamage[index] = temp_ctd.defenderDamage;
                    attack->extrasFlags[index] = temp_ctd.defenderFlags;
                    attack->extrasKnockback[index] = temp_ctd.defenderKnockback;
                    attack->extrasLength += 1;
                }
            }
        }
    }
}

// 0x423EB4
static int attack_crit_success(Attack* attack)
{
    Object* defender = attack->defender;
    if (defender != NULL && critter_flag_check(defender->pid, CRITTER_INVULNERABLE)) {
        return 2;
    }

    if (defender != NULL && PID_TYPE(defender->pid) != OBJ_TYPE_CRITTER) {
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
    if (defender == obj_dude) {
        criticalHitDescription = &(pc_crit_succ_eff[attack->defenderHitLocation][effect]);
    } else {
        int killType = critterGetKillType(defender);
        criticalHitDescription = &(crit_succ_eff[killType][attack->defenderHitLocation][effect]);
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
        // NOTE: Uninline.
        do_random_cripple(&(attack->defenderFlags));
    }

    if (item_w_perk(attack->weapon) == PERK_WEAPON_ENHANCED_KNOCKOUT) {
        attack->defenderFlags |= DAM_KNOCKED_OUT;
    }

    Object* weapon = NULL;
    if (defender != obj_dude) {
        weapon = item_hit_with(defender, HIT_MODE_RIGHT_WEAPON_PRIMARY);
    }

    int flags = attackFindInvalidFlags(defender, weapon);
    attack->defenderFlags &= ~flags;

    return criticalHitDescription->damageMultiplier;
}

// 0x424088
static int attackFindInvalidFlags(Object* critter, Object* item)
{
    int flags = 0;

    if (critter != NULL && PID_TYPE(critter->pid) == OBJ_TYPE_CRITTER && critter_flag_check(critter->pid, CRITTER_NO_DROP)) {
        flags |= DAM_DROP;
    }

    if (item != NULL && item_is_hidden(item)) {
        flags |= DAM_DROP;
    }

    return flags;
}

// 0x4240DC
static int attack_crit_failure(Attack* attack)
{
    attack->attackerFlags |= DAM_HIT;

    if (attack->attacker != NULL && critter_flag_check(attack->attacker->pid, CRITTER_INVULNERABLE)) {
        return 0;
    }

    if (attack->attacker == obj_dude) {
        unsigned int gameTime = gameTimeGetTime();
        if (gameTime / GAME_TIME_TICKS_PER_DAY < 6) {
            return 0;
        }
    }

    int attackType = item_w_subtype(attack->weapon, attack->hitMode);
    int criticalFailureTableIndex = item_w_crit_fail(attack->weapon);
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

    int flags = cf_table[criticalFailureTableIndex][effect];
    if (flags == 0) {
        return 0;
    }

    attack->attackerFlags |= DAM_CRITICAL;
    attack->attackerFlags |= flags;

    int v17 = attackFindInvalidFlags(attack->attacker, attack->weapon);
    attack->attackerFlags &= ~v17;

    if ((attack->attackerFlags & DAM_HIT_SELF) != 0) {
        int ammoQuantity = attackType == ATTACK_TYPE_RANGED ? attack->ammoQuantity : 1;
        compute_damage(attack, ammoQuantity, 2);
    } else if ((attack->attackerFlags & DAM_EXPLODE) != 0) {
        compute_damage(attack, 1, 2);
    }

    if ((attack->attackerFlags & DAM_LOSE_TURN) != 0) {
        attack->attacker->data.critter.combat.ap = 0;
    }

    if ((attack->attackerFlags & DAM_LOSE_AMMO) != 0) {
        if (attackType == ATTACK_TYPE_RANGED) {
            attack->ammoQuantity = item_w_curr_ammo(attack->weapon);
        } else {
            attack->attackerFlags &= ~DAM_LOSE_AMMO;
        }
    }

    if ((attack->attackerFlags & DAM_CRIP_RANDOM) != 0) {
        // NOTE: Uninline.
        do_random_cripple(&(attack->attackerFlags));
    }

    if ((attack->attackerFlags & DAM_RANDOM_HIT) != 0) {
        attack->defender = combat_ai_random_target(attack);
        if (attack->defender != NULL) {
            attack->attackerFlags |= DAM_HIT;
            attack->defenderHitLocation = HIT_LOCATION_TORSO;
            attack->attackerFlags &= ~DAM_CRITICAL;

            int ammoQuantity = attackType == ATTACK_TYPE_RANGED ? attack->ammoQuantity : 1;
            compute_damage(attack, ammoQuantity, 2);
        } else {
            attack->defender = attack->oops;
        }

        if (attack->defender != NULL) {
            attack->tile = attack->defender->tile;
        }
    }

    return 0;
}

// 0x42432C
static void do_random_cripple(int* flagsPtr)
{
    *flagsPtr &= ~DAM_CRIP_RANDOM;

    switch (randomBetween(0, 3)) {
    case 0:
        *flagsPtr |= DAM_CRIP_LEG_LEFT;
        break;
    case 1:
        *flagsPtr |= DAM_CRIP_LEG_RIGHT;
        break;
    case 2:
        *flagsPtr |= DAM_CRIP_ARM_LEFT;
        break;
    case 3:
        *flagsPtr |= DAM_CRIP_ARM_RIGHT;
        break;
    }
}

// 0x42436C
int determine_to_hit(Object* a1, Object* a2, int hitLocation, int hitMode)
{
    return determine_to_hit_func(a1, a1->tile, a2, hitLocation, hitMode, 1);
}

// 0x424380
int determine_to_hit_no_range(Object* a1, Object* a2, int hitLocation, int hitMode, unsigned char* a5)
{
    return determine_to_hit_func(a1, a1->tile, a2, hitLocation, hitMode, 0);
}

// 0x424394
int determine_to_hit_from_tile(Object* a1, int tile, Object* a3, int hitLocation, int hitMode)
{
    return determine_to_hit_func(a1, tile, a3, hitLocation, hitMode, 1);
}

// determine_to_hit
// 0x4243A8
static int determine_to_hit_func(Object* attacker, int tile, Object* defender, int hitLocation, int hitMode, int a6)
{
    Object* weapon = item_hit_with(attacker, hitMode);

    bool targetIsCritter = defender != NULL
        ? FID_TYPE(defender->fid) == OBJ_TYPE_CRITTER
        : false;

    bool isRangedWeapon = false;

    int accuracy;
    if (weapon == NULL || hitMode == HIT_MODE_PUNCH || hitMode == HIT_MODE_KICK || (hitMode >= FIRST_ADVANCED_UNARMED_HIT_MODE && hitMode <= LAST_ADVANCED_UNARMED_HIT_MODE)) {
        accuracy = skillGetValue(attacker, SKILL_UNARMED);
    } else {
        accuracy = item_w_skill_level(attacker, hitMode);

        int modifier = 0;

        int attackType = item_w_subtype(weapon, hitMode);
        if (attackType == ATTACK_TYPE_RANGED || attackType == ATTACK_TYPE_THROW) {
            isRangedWeapon = true;

            int v29 = 0;
            int v25 = 0;

            int weaponPerk = item_w_perk(weapon);
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
                modifier = obj_dist_with_tile(attacker, tile, defender, defender->tile);
            } else {
                modifier = 0;
            }

            if (modifier >= v25) {
                int penalty = attacker == obj_dude
                    ? v29 * (perception - 2)
                    : v29 * perception;

                modifier -= penalty;
            } else {
                modifier += v25;
            }

            if (-2 * perception > modifier) {
                modifier = -2 * perception;
            }

            if (attacker == obj_dude) {
                modifier -= 2 * perk_level(obj_dude, PERK_SHARPSHOOTER);
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
                combat_is_shot_blocked(attacker, tile, defender->tile, defender, &modifier);
            }

            accuracy -= 10 * modifier;
        }

        if (attacker == obj_dude && traitIsSelected(TRAIT_ONE_HANDER)) {
            if (item_w_is_2handed(weapon)) {
                accuracy -= 40;
            } else {
                accuracy += 20;
            }
        }

        int minStrength = item_w_min_st(weapon);
        modifier = minStrength - critterGetStat(attacker, STAT_STRENGTH);
        if (attacker == obj_dude && perk_level(obj_dude, PERK_WEAPON_HANDLING) != 0) {
            modifier -= 3;
        }

        if (modifier > 0) {
            accuracy -= 20 * modifier;
        }

        if (item_w_perk(weapon) == PERK_WEAPON_ACCURATE) {
            accuracy += 20;
        }
    }

    if (targetIsCritter && defender != NULL) {
        int armorClass = critterGetStat(defender, STAT_ARMOR_CLASS);
        armorClass += item_w_ac_adjust(weapon);
        if (armorClass < 0) {
            armorClass = 0;
        }

        accuracy -= armorClass;
    }

    if (isRangedWeapon) {
        accuracy += hit_location_penalty[hitLocation];
    } else {
        accuracy += hit_location_penalty[hitLocation] / 2;
    }

    if (defender != NULL && (defender->flags & OBJECT_MULTIHEX) != 0) {
        accuracy += 15;
    }

    if (attacker == obj_dude) {
        int lightIntensity;
        if (defender != NULL) {
            lightIntensity = obj_get_visible_light(defender);
            if (item_w_perk(weapon) == PERK_WEAPON_NIGHT_SIGHT) {
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

    if (gcsd != NULL) {
        accuracy += gcsd->accuracyBonus;
    }

    if ((attacker->data.critter.combat.results & DAM_BLIND) != 0) {
        accuracy -= 25;
    }

    if (targetIsCritter && defender != NULL && (defender->data.critter.combat.results & (DAM_KNOCKED_OUT | DAM_KNOCKED_DOWN)) != 0) {
        accuracy += 40;
    }

    if (attacker->data.critter.combat.team != obj_dude->data.critter.combat.team) {
        int combatDifficuly = 1;
        config_get_value(&game_config, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_COMBAT_DIFFICULTY_KEY, &combatDifficuly);
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
static void compute_damage(Attack* attack, int ammoQuantity, int bonusDamageMultiplier)
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

    if (FID_TYPE(critter->fid) != OBJ_TYPE_CRITTER) {
        return;
    }

    int damageType = item_w_damage_type(attack->attacker, attack->weapon);
    int damageThreshold = critterGetStat(critter, STAT_DAMAGE_THRESHOLD + damageType);
    int damageResistance = critterGetStat(critter, STAT_DAMAGE_RESISTANCE + damageType);

    if ((*flagsPtr & DAM_BYPASS) != 0 && damageType != DAMAGE_TYPE_EMP) {
        damageThreshold = 20 * damageThreshold / 100;
        damageResistance = 20 * damageResistance / 100;
    } else {
        if (item_w_perk(attack->weapon) == PERK_WEAPON_PENETRATE
            || attack->hitMode == HIT_MODE_PALM_STRIKE
            || attack->hitMode == HIT_MODE_PIERCING_STRIKE
            || attack->hitMode == HIT_MODE_HOOK_KICK
            || attack->hitMode == HIT_MODE_PIERCING_KICK) {
            damageThreshold = 20 * damageThreshold / 100;
        }

        if (attack->attacker == obj_dude && traitIsSelected(TRAIT_FINESSE)) {
            damageResistance += 30;
        }
    }

    int damageBonus;
    if (attack->attacker == obj_dude && item_w_subtype(attack->weapon, attack->hitMode) == ATTACK_TYPE_RANGED) {
        damageBonus = 2 * perk_level(obj_dude, PERK_BONUS_RANGED_DAMAGE);
    } else {
        damageBonus = 0;
    }

    int combatDifficultyDamageModifier = 100;
    if (attack->attacker->data.critter.combat.team != obj_dude->data.critter.combat.team) {
        int combatDifficulty = COMBAT_DIFFICULTY_NORMAL;
        config_get_value(&game_config, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_COMBAT_DIFFICULTY_KEY, &combatDifficulty);

        switch (combatDifficulty) {
        case COMBAT_DIFFICULTY_EASY:
            combatDifficultyDamageModifier = 75;
            break;
        case COMBAT_DIFFICULTY_HARD:
            combatDifficultyDamageModifier = 125;
            break;
        }
    }

    damageResistance += item_w_dr_adjust(attack->weapon);
    if (damageResistance > 100) {
        damageResistance = 100;
    } else if (damageResistance < 0) {
        damageResistance = 0;
    }

    int damageMultiplier = bonusDamageMultiplier * item_w_dam_mult(attack->weapon);
    int damageDivisor = item_w_dam_div(attack->weapon);

    for (int index = 0; index < ammoQuantity; index++) {
        int damage = item_w_damage(attack->attacker, attack->hitMode);

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

    if (attack->attacker == obj_dude) {
        if (perk_level(attack->attacker, PERK_LIVING_ANATOMY) != 0) {
            int kt = critterGetKillType(attack->defender);
            if (kt != KILL_TYPE_ROBOT && kt != KILL_TYPE_ALIEN) {
                *damagePtr += 5;
            }
        }

        if (perk_level(attack->attacker, PERK_PYROMANIAC) != 0) {
            if (item_w_damage_type(attack->attacker, attack->weapon) == DAMAGE_TYPE_FIRE) {
                *damagePtr += 5;
            }
        }
    }

    if (knockbackDistancePtr != NULL
        && (critter->flags & OBJECT_MULTIHEX) == 0
        && (damageType == DAMAGE_TYPE_EXPLOSION || attack->weapon == NULL || item_w_subtype(attack->weapon, attack->hitMode) == ATTACK_TYPE_MELEE)
        && PID_TYPE(critter->pid) == OBJ_TYPE_CRITTER
        && critter_flag_check(critter->pid, CRITTER_NO_KNOCKBACK) == 0) {
        bool shouldKnockback = true;
        bool hasStonewall = false;
        if (critter == obj_dude) {
            if (perk_level(critter, PERK_STONEWALL) != 0) {
                int chance = randomBetween(0, 100);
                hasStonewall = true;
                if (chance < 50) {
                    shouldKnockback = false;
                }
            }
        }

        if (shouldKnockback) {
            int knockbackDistanceDivisor = item_w_perk(attack->weapon) == PERK_WEAPON_KNOCKBACK ? 5 : 10;

            *knockbackDistancePtr = *damagePtr / knockbackDistanceDivisor;

            if (hasStonewall) {
                *knockbackDistancePtr /= 2;
            }
        }
    }
}

// 0x424BAC
void death_checks(Attack* attack)
{
    check_for_death(attack->attacker, attack->attackerDamage, &(attack->attackerFlags));
    check_for_death(attack->defender, attack->defenderDamage, &(attack->defenderFlags));

    for (int index = 0; index < attack->extrasLength; index++) {
        check_for_death(attack->extras[index], attack->extrasDamage[index], &(attack->extrasFlags[index]));
    }
}

// 0x424C04
void apply_damage(Attack* attack, bool animated)
{
    Object* attacker = attack->attacker;
    bool attackerIsCritter = attacker != NULL && FID_TYPE(attacker->fid) == OBJ_TYPE_CRITTER;
    bool v5 = attack->defender != attack->oops;

    if (attackerIsCritter && (attacker->data.critter.combat.results & DAM_DEAD) != 0) {
        set_new_results(attacker, attack->attackerFlags);
        // TODO: Not sure about "attack->defender == attack->oops".
        damage_object(attacker, attack->attackerDamage, animated, attack->defender == attack->oops, attacker);
    }

    Object* v7 = attack->oops;
    if (v7 != NULL && v7 != attack->defender) {
        combatai_notify_onlookers(v7);
    }

    Object* defender = attack->defender;
    bool defenderIsCritter = defender != NULL && FID_TYPE(defender->fid) == OBJ_TYPE_CRITTER;

    if (!defenderIsCritter && !v5) {
        bool v9 = isPartyMember(attack->defender) && isPartyMember(attack->attacker) ? false : true;
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
        set_new_results(defender, attack->defenderFlags);

        if (defenderIsCritter) {
            if (defenderIsCritter) {
                if ((defender->data.critter.combat.results & (DAM_DEAD | DAM_KNOCKED_OUT)) != 0) {
                    if (!v5 || defender != obj_dude) {
                        critter_set_who_hit_me(defender, attack->attacker);
                    }
                } else if (defender == attack->oops || defender->data.critter.combat.team != attack->attacker->data.critter.combat.team) {
                    combatai_check_retaliation(defender, attack->attacker);
                }
            }
        }

        scriptSetObjects(defender->sid, attack->attacker, attack->weapon);
        damage_object(defender, attack->defenderDamage, animated, attack->defender != attack->oops, attacker);

        if (defenderIsCritter) {
            combatai_notify_onlookers(defender);
        }

        if (attack->defenderDamage >= 0 && (attack->attackerFlags & DAM_HIT) != 0) {
            scriptSetObjects(attack->attacker->sid, NULL, attack->defender);
            scriptSetFixedParam(attack->attacker->sid, 2);
            scriptExecProc(attack->attacker->sid, SCRIPT_PROC_COMBAT);
        }
    }

    for (int index = 0; index < attack->extrasLength; index++) {
        Object* obj = attack->extras[index];
        if (FID_TYPE(obj->fid) == OBJ_TYPE_CRITTER && (obj->data.critter.combat.results & DAM_DEAD) == 0) {
            set_new_results(obj, attack->extrasFlags[index]);

            if (defenderIsCritter) {
                if ((obj->data.critter.combat.results & (DAM_DEAD | DAM_KNOCKED_OUT)) != 0) {
                    critter_set_who_hit_me(obj, attack->attacker);
                } else if (obj->data.critter.combat.team != attack->attacker->data.critter.combat.team) {
                    combatai_check_retaliation(obj, attack->attacker);
                }
            }

            scriptSetObjects(obj->sid, attack->attacker, attack->weapon);
            // TODO: Not sure about defender == oops.
            damage_object(obj, attack->extrasDamage[index], animated, attack->defender == attack->oops, attack->attacker);
            combatai_notify_onlookers(obj);

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
static void check_for_death(Object* object, int damage, int* flags)
{
    if (object == NULL || !critter_flag_check(object->pid, CRITTER_INVULNERABLE)) {
        if (object == NULL || PID_TYPE(object->pid) == OBJ_TYPE_CRITTER) {
            if (damage > 0) {
                if (critter_get_hits(object) - damage <= 0) {
                    *flags |= DAM_DEAD;
                }
            }
        }
    }
}

// 0x424F2C
static void set_new_results(Object* critter, int flags)
{
    if (critter == NULL) {
        return;
    }

    if (FID_TYPE(critter->fid) != OBJ_TYPE_CRITTER) {
        return;
    }

    if (critter_flag_check(critter->pid, CRITTER_INVULNERABLE)) {
        return;
    }

    if (PID_TYPE(critter->pid) != OBJ_TYPE_CRITTER) {
        return;
    }

    if ((flags & DAM_DEAD) != 0) {
        queueRemoveEvents(critter);
    } else if ((flags & DAM_KNOCKED_OUT) != 0) {
        int endurance = critterGetStat(critter, STAT_ENDURANCE);
        queueAddEvent(10 * (35 - 3 * endurance), critter, NULL, EVENT_TYPE_KNOCKOUT);
    }

    if (critter == obj_dude && (flags & DAM_CRIP_ARM_ANY) != 0) {
        critter->data.critter.combat.results |= flags & (DAM_KNOCKED_OUT | DAM_KNOCKED_DOWN | DAM_CRIP | DAM_DEAD | DAM_LOSE_TURN);

        int leftItemAction;
        int rightItemAction;
        intface_get_item_states(&leftItemAction, &rightItemAction);
        intface_update_items(true, leftItemAction, rightItemAction);
    } else {
        critter->data.critter.combat.results |= flags & (DAM_KNOCKED_OUT | DAM_KNOCKED_DOWN | DAM_CRIP | DAM_DEAD | DAM_LOSE_TURN);
    }
}

// 0x425020
static void damage_object(Object* a1, int damage, bool animated, int a4, Object* a5)
{
    if (a1 == NULL) {
        return;
    }

    if (FID_TYPE(a1->fid) != OBJ_TYPE_CRITTER) {
        return;
    }

    if (critter_flag_check(a1->pid, CRITTER_INVULNERABLE)) {
        return;
    }

    if (damage <= 0) {
        return;
    }

    critter_adjust_hits(a1, -damage);

    if (a1 == obj_dude) {
        intface_update_hit_points(animated);
    }

    a1->data.critter.combat.damageLastTurn += damage;

    if (!a4) {
        // TODO: Not sure about this one.
        if (!isPartyMember(a1) || !isPartyMember(a5)) {
            scriptSetFixedParam(a1->sid, damage);
            scriptExecProc(a1->sid, SCRIPT_PROC_DAMAGE);
        }
    }

    if ((a1->data.critter.combat.results & DAM_DEAD) != 0) {
        scriptSetObjects(a1->sid, a1->data.critter.combat.whoHitMe, NULL);
        scriptExecProc(a1->sid, SCRIPT_PROC_DESTROY);
        item_destroy_all_hidden(a1);

        if (a1 != obj_dude) {
            Object* whoHitMe = a1->data.critter.combat.whoHitMe;
            if (whoHitMe == obj_dude || (whoHitMe != NULL && whoHitMe->data.critter.combat.team == obj_dude->data.critter.combat.team)) {
                bool scriptOverrides = false;
                Script* scr;
                if (scriptGetScript(a1->sid, &scr) != -1) {
                    scriptOverrides = scr->scriptOverrides;
                }

                if (!scriptOverrides) {
                    combat_exps += critter_kill_exps(a1);
                    critter_kill_count_inc(critterGetKillType(a1));
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
void combat_display(Attack* attack)
{
    MessageListItem messageListItem;

    if (attack->attacker == obj_dude) {
        Object* weapon = item_hit_with(attack->attacker, attack->hitMode);
        int strengthRequired = item_w_min_st(weapon);

        if (perk_level(attack->attacker, PERK_WEAPON_HANDLING) != 0) {
            strengthRequired -= 3;
        }

        if (weapon != NULL) {
            if (strengthRequired > critterGetStat(obj_dude, STAT_STRENGTH)) {
                // You are not strong enough to use this weapon properly.
                messageListItem.num = 107;
                if (message_search(&combat_message_file, &messageListItem)) {
                    display_print(messageListItem.text);
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

    char* mainCritterName = _a_1;

    char you[20];
    you[0] = '\0';
    if (critterGetStat(obj_dude, STAT_GENDER) == GENDER_MALE) {
        // You (male)
        messageListItem.num = 506;
    } else {
        // You (female)
        messageListItem.num = 556;
    }

    if (message_search(&combat_message_file, &messageListItem)) {
        strcpy(you, messageListItem.text);
    }

    int baseMessageId;
    if (mainCritter == obj_dude) {
        mainCritterName = you;
        if (critterGetStat(obj_dude, STAT_GENDER) == GENDER_MALE) {
            baseMessageId = 500;
        } else {
            baseMessageId = 550;
        }
    } else if (mainCritter != NULL) {
        mainCritterName = object_name(mainCritter);
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
        if (FID_TYPE(attack->defender->fid) == OBJ_TYPE_CRITTER) {
            if (attack->oops == obj_dude) {
                // 608 (male) - Oops! %s was hit instead of you!
                // 708 (female) - Oops! %s was hit instead of you!
                messageListItem.num = baseMessageId + 8;
                if (message_search(&combat_message_file, &messageListItem)) {
                    sprintf(text, messageListItem.text, mainCritterName);
                }
            } else {
                // 509 (male) - Oops! %s were hit instead of %s!
                // 559 (female) - Oops! %s were hit instead of %s!
                const char* name = object_name(attack->oops);
                messageListItem.num = baseMessageId + 9;
                if (message_search(&combat_message_file, &messageListItem)) {
                    sprintf(text, messageListItem.text, mainCritterName, name);
                }
            }
        } else {
            if (attack->attacker == obj_dude) {
                if (critterGetStat(attack->attacker, STAT_GENDER) == GENDER_MALE) {
                    // (male) %s missed
                    messageListItem.num = 515;
                } else {
                    // (female) %s missed
                    messageListItem.num = 565;
                }

                if (message_search(&combat_message_file, &messageListItem)) {
                    sprintf(text, messageListItem.text, you);
                }
            } else {
                const char* name = object_name(attack->attacker);
                if (critterGetStat(attack->attacker, STAT_GENDER) == GENDER_MALE) {
                    // (male) %s missed
                    messageListItem.num = 615;
                } else {
                    // (female) %s missed
                    messageListItem.num = 715;
                }

                if (message_search(&combat_message_file, &messageListItem)) {
                    sprintf(text, messageListItem.text, name);
                }
            }
        }

        strcat(text, ".");

        display_print(text);
    }

    if ((attack->attackerFlags & DAM_HIT) != 0) {
        Object* v21 = attack->defender;
        if (v21 != NULL && (v21->data.critter.combat.results & DAM_DEAD) == 0) {
            text[0] = '\0';

            if (FID_TYPE(v21->fid) == OBJ_TYPE_CRITTER) {
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

                        if (message_search(&combat_message_file, &messageListItem)) {
                            if (attack->defenderDamage <= 1) {
                                sprintf(text, messageListItem.text, mainCritterName);
                            } else {
                                sprintf(text, messageListItem.text, mainCritterName, attack->defenderDamage);
                            }
                        }
                    } else {
                        combat_display_hit(text, v21, attack->defenderDamage);
                    }
                } else {
                    const char* hitLocationName = combat_get_loc_name(v21, attack->defenderHitLocation);
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

                        if (message_search(&combat_message_file, &messageListItem)) {
                            if (attack->defenderDamage <= 1) {
                                sprintf(text, messageListItem.text, mainCritterName, hitLocationName);
                            } else {
                                sprintf(text, messageListItem.text, mainCritterName, hitLocationName, attack->defenderDamage);
                            }
                        }
                    }
                }

                int combatMessages = 1;
                config_get_value(&game_config, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_COMBAT_MESSAGES_KEY, &combatMessages);

                if (combatMessages == 1 && (attack->attackerFlags & DAM_CRITICAL) != 0 && attack->criticalMessageId != -1) {
                    messageListItem.num = attack->criticalMessageId;
                    if (message_search(&combat_message_file, &messageListItem)) {
                        strcat(text, messageListItem.text);
                    }

                    if ((attack->defenderFlags & DAM_DEAD) != 0) {
                        strcat(text, ".");
                        display_print(text);

                        if (attack->defender == obj_dude) {
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

                        if (message_search(&combat_message_file, &messageListItem)) {
                            sprintf(text, "%s %s", mainCritterName, messageListItem.text);
                        }
                    }
                } else {
                    combat_display_flags(text, attack->defenderFlags, attack->defender);
                }

                strcat(text, ".");

                display_print(text);
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

            if (message_search(&combat_message_file, &messageListItem)) {
                if (attack->attackerDamage <= 1) {
                    sprintf(text, messageListItem.text, mainCritterName);
                } else {
                    sprintf(text, messageListItem.text, mainCritterName, attack->attackerDamage);
                }
            }

            combat_display_flags(text, attack->attackerFlags, attack->attacker);

            strcat(text, ".");

            display_print(text);
        }

        if ((attack->attackerFlags & DAM_HIT) != 0 || (attack->attackerFlags & DAM_CRITICAL) == 0) {
            if (attack->attackerDamage > 0) {
                combat_display_hit(text, attack->attacker, attack->attackerDamage);
                combat_display_flags(text, attack->attackerFlags, attack->attacker);
                strcat(text, ".");
                display_print(text);
            }
        }
    }

    for (int index = 0; index < attack->extrasLength; index++) {
        Object* critter = attack->extras[index];
        if ((critter->data.critter.combat.results & DAM_DEAD) == 0) {
            combat_display_hit(text, critter, attack->extrasDamage[index]);
            combat_display_flags(text, attack->extrasFlags[index], critter);
            strcat(text, ".");

            display_print(text);
        }
    }
}

// 0x425A9C
static void combat_display_hit(char* dest, Object* critter, int damage)
{
    MessageListItem messageListItem;
    char text[40];
    char* name;

    int messageId;
    if (critter == obj_dude) {
        text[0] = '\0';

        if (critterGetStat(obj_dude, STAT_GENDER) == GENDER_MALE) {
            messageId = 500;
        } else {
            messageId = 550;
        }

        // 506 - You
        messageListItem.num = messageId + 6;
        if (message_search(&combat_message_file, &messageListItem)) {
            strcpy(text, messageListItem.text);
        }

        name = text;
    } else {
        name = object_name(critter);

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
    if (message_search(&combat_message_file, &messageListItem)) {
        if (damage <= 1) {
            sprintf(dest, messageListItem.text, name);
        } else {
            sprintf(dest, messageListItem.text, name, damage);
        }
    }
}

// 0x425BA4
static void combat_display_flags(char* dest, int flags, Object* critter)
{
    MessageListItem messageListItem;

    int num;
    if (critter == obj_dude) {
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
        if (message_search(&combat_message_file, &messageListItem)) {
            strcat(dest, messageListItem.text);
        }

        // were killed
        messageListItem.num = num + 7;
        if (message_search(&combat_message_file, &messageListItem)) {
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
            if (message_search(&combat_message_file, &messageListItem)) {
                strcat(dest, messageListItem.text);
            }
        }

        // " and "
        messageListItem.num = 108;
        if (message_search(&combat_message_file, &messageListItem)) {
            strcat(dest, messageListItem.text);
        }

        messageListItem.num = flagsList[flagsListLength - 1];
        if (message_search(&combat_message_file, &messageListItem)) {
            strcat(dest, messageListItem.text);
        }
    }
}

// 0x425E3C
void combat_anim_begin()
{
    if (++combat_turn_running == 1 && obj_dude == main_ctd.attacker) {
        game_ui_disable(1);
        gmouse_set_cursor(26);
        if (combat_highlight == 2) {
            combat_outline_off();
        }
    }
}

// 0x425E80
void combat_anim_finished()
{
    combat_turn_running -= 1;
    if (combat_turn_running != 0) {
        return;
    }

    if (obj_dude == main_ctd.attacker) {
        game_ui_enable();
    }

    if (combat_cleanup_enabled) {
        combat_cleanup_enabled = false;

        Object* weapon = item_hit_with(main_ctd.attacker, main_ctd.hitMode);
        if (weapon != NULL) {
            if (item_w_max_ammo(weapon) > 0) {
                int ammoQuantity = item_w_curr_ammo(weapon);
                item_w_set_curr_ammo(weapon, ammoQuantity - main_ctd.ammoQuantity);

                if (main_ctd.attacker == obj_dude) {
                    intface_update_ammo_lights();
                }
            }
        }

        if (combat_call_display) {
            combat_display(&main_ctd);
            combat_call_display = false;
        }

        apply_damage(&main_ctd, true);

        Object* attacker = main_ctd.attacker;
        if (attacker == obj_dude && combat_highlight == 2) {
            combat_outline_on();
        }

        if (_scr_end_combat()) {
            if ((obj_dude->data.critter.combat.results & DAM_KNOCKED_OUT) != 0) {
                if (attacker->data.critter.combat.team == obj_dude->data.critter.combat.team) {
                    combat_ending_guy = obj_dude->data.critter.combat.whoHitMe;
                } else {
                    combat_ending_guy = attacker;
                }
            }
        }

        combat_ctd_init(&main_ctd, main_ctd.attacker, NULL, HIT_MODE_PUNCH, HIT_LOCATION_TORSO);

        if ((attacker->data.critter.combat.results & (DAM_KNOCKED_OUT | DAM_KNOCKED_DOWN)) != 0) {
            if ((attacker->data.critter.combat.results & (DAM_KNOCKED_OUT | DAM_DEAD | DAM_LOSE_TURN)) == 0) {
                combat_standup(attacker);
            }
        }
    }
}

// 0x425FBC
static void combat_standup(Object* a1)
{
    int v2;

    v2 = 3;
    if (a1 == obj_dude && perk_level(a1, PERK_QUICK_RECOVERY)) {
        v2 = 1;
    }

    if (v2 > a1->data.critter.combat.ap) {
        a1->data.critter.combat.ap = 0;
    } else {
        a1->data.critter.combat.ap -= v2;
    }

    if (a1 == obj_dude) {
        intface_update_move_points(obj_dude->data.critter.combat.ap, combat_free_move);
    }

    dude_standup(a1);

    // NOTE: Uninline.
    combat_turn_run();
}

// Render two digits.
//
// 0x42603C
static void print_tohit(unsigned char* dest, int destPitch, int accuracy)
{
    CacheEntry* numbersFrmHandle;
    int numbersFrmFid = art_id(OBJ_TYPE_INTERFACE, 82, 0, 0, 0);
    unsigned char* numbersFrmData = art_ptr_lock_data(numbersFrmFid, 0, 0, &numbersFrmHandle);
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

    art_ptr_unlock(numbersFrmHandle);
}

// 0x42612C
static char* combat_get_loc_name(Object* critter, int hitLocation)
{
    MessageListItem messageListItem;
    messageListItem.num = 1000 + 10 * art_alias_num(critter->fid & 0xFFF) + hitLocation;
    if (message_search(&combat_message_file, &messageListItem)) {
        return messageListItem.text;
    }

    return NULL;
}

// 0x4261B4
static void draw_loc_off(int a1, int a2)
{
    draw_loc(a2, colorTable[992]);
}

// 0x4261C0
static void draw_loc_on(int a1, int a2)
{
    draw_loc(a2, colorTable[31744]);
}

// 0x4261CC
static void draw_loc(int eventCode, int color)
{
    color |= 0x3000000;

    if (eventCode >= 4) {
        char* name = combat_get_loc_name(call_target, hit_loc_right[eventCode - 4]);
        int width = fontGetStringWidth(name);
        windowDrawText(call_win, name, 0, 431 - width, call_ty[eventCode - 4] - 86, color);
    } else {
        char* name = combat_get_loc_name(call_target, hit_loc_left[eventCode]);
        windowDrawText(call_win, name, 0, 74, call_ty[eventCode] - 86, color);
    }
}

// 0x426218
static int get_called_shot_location(Object* critter, int* hitLocation, int hitMode)
{
    if (critter == NULL) {
        return 0;
    }

    if (PID_TYPE(critter->pid) != OBJ_TYPE_CRITTER) {
        return 0;
    }

    call_target = critter;

    int calledShotWindowX = CALLED_SHOT_WINDOW_X;
    int calledShotWindowY = CALLED_SHOT_WINDOW_Y;
    call_win = windowCreate(calledShotWindowX,
        calledShotWindowY,
        CALLED_SHOT_WINDOW_WIDTH,
        CALLED_SHOT_WINDOW_HEIGHT,
        colorTable[0],
        WINDOW_FLAG_0x10);
    if (call_win == -1) {
        return -1;
    }

    int fid;
    CacheEntry* handle;
    unsigned char* data;

    unsigned char* windowBuffer = windowGetBuffer(call_win);

    fid = art_id(OBJ_TYPE_INTERFACE, 118, 0, 0, 0);
    data = art_ptr_lock_data(fid, 0, 0, &handle);
    if (data == NULL) {
        windowDestroy(call_win);
        return -1;
    }

    blitBufferToBuffer(data, CALLED_SHOT_WINDOW_WIDTH, CALLED_SHOT_WINDOW_HEIGHT, CALLED_SHOT_WINDOW_WIDTH, windowBuffer, CALLED_SHOT_WINDOW_WIDTH);
    art_ptr_unlock(handle);

    fid = art_id(OBJ_TYPE_CRITTER, critter->fid & 0xFFF, ANIM_CALLED_SHOT_PIC, 0, 0);
    data = art_ptr_lock_data(fid, 0, 0, &handle);
    if (data != NULL) {
        blitBufferToBuffer(data, 170, 225, 170, windowBuffer + CALLED_SHOT_WINDOW_WIDTH * 31 + 168, CALLED_SHOT_WINDOW_WIDTH);
        art_ptr_unlock(handle);
    }

    fid = art_id(OBJ_TYPE_INTERFACE, 8, 0, 0, 0);

    CacheEntry* upHandle;
    unsigned char* up = art_ptr_lock_data(fid, 0, 0, &upHandle);
    if (up == NULL) {
        windowDestroy(call_win);
        return -1;
    }

    fid = art_id(OBJ_TYPE_INTERFACE, 9, 0, 0, 0);

    CacheEntry* downHandle;
    unsigned char* down = art_ptr_lock_data(fid, 0, 0, &downHandle);
    if (down == NULL) {
        art_ptr_unlock(upHandle);
        windowDestroy(call_win);
        return -1;
    }

    // Cancel button
    int btn = buttonCreate(call_win, 210, 268, 15, 16, -1, -1, -1, KEY_ESCAPE, up, down, NULL, BUTTON_FLAG_TRANSPARENT);
    if (btn != -1) {
        buttonSetCallbacks(btn, gsound_red_butt_press, gsound_red_butt_release);
    }

    int oldFont = fontGetCurrent();
    fontSetCurrent(101);

    for (int index = 0; index < 4; index++) {
        int probability;
        int btn;

        probability = determine_to_hit(obj_dude, critter, hit_loc_left[index], hitMode);
        print_tohit(windowBuffer + CALLED_SHOT_WINDOW_WIDTH * (call_ty[index] - 86) + 33, CALLED_SHOT_WINDOW_WIDTH, probability);

        btn = buttonCreate(call_win, 33, call_ty[index] - 90, 128, 20, index, index, -1, index, NULL, NULL, NULL, 0);
        buttonSetMouseCallbacks(btn, draw_loc_on, draw_loc_off, NULL, NULL);
        draw_loc(index, colorTable[992]);

        probability = determine_to_hit(obj_dude, critter, hit_loc_right[index], hitMode);
        print_tohit(windowBuffer + CALLED_SHOT_WINDOW_WIDTH * (call_ty[index] - 86) + 453, CALLED_SHOT_WINDOW_WIDTH, probability);

        btn = buttonCreate(call_win, 341, call_ty[index] - 90, 128, 20, index + 4, index + 4, -1, index + 4, NULL, NULL, NULL, 0);
        buttonSetMouseCallbacks(btn, draw_loc_on, draw_loc_off, NULL, NULL);
        draw_loc(index + 4, colorTable[992]);
    }

    win_draw(call_win);

    bool gameUiWasDisabled = game_ui_is_disabled();
    if (gameUiWasDisabled) {
        game_ui_enable();
    }

    gmouse_disable(0);
    gmouse_set_cursor(MOUSE_CURSOR_ARROW);

    int eventCode;
    while (true) {
        eventCode = _get_input();

        if (eventCode == KEY_ESCAPE) {
            break;
        }

        if (eventCode >= 0 && eventCode < HIT_LOCATION_COUNT) {
            break;
        }

        if (game_user_wants_to_quit != 0) {
            break;
        }
    }

    gmouse_enable();

    if (gameUiWasDisabled) {
        game_ui_disable(0);
    }

    fontSetCurrent(oldFont);

    art_ptr_unlock(downHandle);
    art_ptr_unlock(upHandle);
    windowDestroy(call_win);

    if (eventCode == KEY_ESCAPE) {
        return -1;
    }

    *hitLocation = eventCode < 4 ? hit_loc_left[eventCode] : hit_loc_right[eventCode - 4];

    gsound_play_sfx_file("icsxxxx1");

    return 0;
}

// check for possibility of performing attacking
// 0x426614
int combat_check_bad_shot(Object* attacker, Object* defender, int hitMode, bool aiming)
{
    int range = 1;
    int tile = -1;
    if (defender != NULL) {
        tile = defender->tile;
        range = obj_dist(attacker, defender);
        if ((defender->data.critter.combat.results & DAM_DEAD) != 0) {
            return COMBAT_BAD_SHOT_ALREADY_DEAD;
        }
    }

    Object* weapon = item_hit_with(attacker, hitMode);
    if (weapon != NULL) {
        if ((attacker->data.critter.combat.results & DAM_CRIP_ARM_LEFT) != 0
            && (attacker->data.critter.combat.results & DAM_CRIP_ARM_RIGHT) != 0) {
            return COMBAT_BAD_SHOT_BOTH_ARMS_CRIPPLED;
        }

        if ((attacker->data.critter.combat.results & DAM_CRIP_ARM_ANY) != 0) {
            if (item_w_is_2handed(weapon)) {
                return COMBAT_BAD_SHOT_ARM_CRIPPLED;
            }
        }
    }

    if (item_w_mp_cost(attacker, hitMode, aiming) > attacker->data.critter.combat.ap) {
        return COMBAT_BAD_SHOT_NOT_ENOUGH_AP;
    }

    if (item_w_range(attacker, hitMode) < range) {
        return COMBAT_BAD_SHOT_OUT_OF_RANGE;
    }

    int attackType = item_w_subtype(weapon, hitMode);

    if (item_w_max_ammo(weapon) > 0) {
        if (item_w_curr_ammo(weapon) == 0) {
            return COMBAT_BAD_SHOT_NO_AMMO;
        }
    }

    if (attackType == ATTACK_TYPE_RANGED
        || attackType == ATTACK_TYPE_THROW
        || item_w_range(attacker, hitMode) > 1) {
        if (combat_is_shot_blocked(attacker, attacker->tile, tile, defender, NULL)) {
            return COMBAT_BAD_SHOT_AIM_BLOCKED;
        }
    }

    return COMBAT_BAD_SHOT_OK;
}

// 0x426744
bool combat_to_hit(Object* target, int* accuracy)
{
    int hitMode;
    bool aiming;
    if (intface_get_attack(&hitMode, &aiming) == -1) {
        return false;
    }

    if (combat_check_bad_shot(obj_dude, target, hitMode, aiming) != COMBAT_BAD_SHOT_OK) {
        return false;
    }

    *accuracy = determine_to_hit_func(obj_dude, obj_dude->tile, target, HIT_LOCATION_UNCALLED, hitMode, 1);

    return true;
}

// 0x4267CC
void combat_attack_this(Object* a1)
{
    if (a1 == NULL) {
        return;
    }

    if ((combat_state & 0x02) == 0) {
        return;
    }

    int hitMode;
    bool aiming;
    if (intface_get_attack(&hitMode, &aiming) == -1) {
        return;
    }

    MessageListItem messageListItem;
    Object* item;
    char formattedText[80];
    const char* sfx;

    int rc = combat_check_bad_shot(obj_dude, a1, hitMode, aiming);
    switch (rc) {
    case COMBAT_BAD_SHOT_NO_AMMO:
        item = item_hit_with(obj_dude, hitMode);
        messageListItem.num = 101; // Out of ammo.
        if (message_search(&combat_message_file, &messageListItem)) {
            display_print(messageListItem.text);
        }

        sfx = gsnd_build_weapon_sfx_name(WEAPON_SOUND_EFFECT_OUT_OF_AMMO, item, hitMode, NULL);
        gsound_play_sfx_file(sfx);
        return;
    case COMBAT_BAD_SHOT_OUT_OF_RANGE:
        messageListItem.num = 102; // Target out of range.
        if (message_search(&combat_message_file, &messageListItem)) {
            display_print(messageListItem.text);
        }
        return;
    case COMBAT_BAD_SHOT_NOT_ENOUGH_AP:
        item = item_hit_with(obj_dude, hitMode);
        messageListItem.num = 100; // You need %d action points.
        if (message_search(&combat_message_file, &messageListItem)) {
            int actionPointsRequired = item_w_mp_cost(obj_dude, hitMode, aiming);
            sprintf(formattedText, messageListItem.text, actionPointsRequired);
            display_print(formattedText);
        }
        return;
    case COMBAT_BAD_SHOT_ALREADY_DEAD:
        return;
    case COMBAT_BAD_SHOT_AIM_BLOCKED:
        messageListItem.num = 104; // Your aim is blocked.
        if (message_search(&combat_message_file, &messageListItem)) {
            display_print(messageListItem.text);
        }
        return;
    case COMBAT_BAD_SHOT_ARM_CRIPPLED:
        messageListItem.num = 106; // You cannot use two-handed weapons with a crippled arm.
        if (message_search(&combat_message_file, &messageListItem)) {
            display_print(messageListItem.text);
        }
        return;
    case COMBAT_BAD_SHOT_BOTH_ARMS_CRIPPLED:
        messageListItem.num = 105; // You cannot use weapons with both arms crippled.
        if (message_search(&combat_message_file, &messageListItem)) {
            display_print(messageListItem.text);
        }
        return;
    }

    if (!isInCombat()) {
        STRUCT_664980 stru;
        stru.attacker = obj_dude;
        stru.defender = a1;
        stru.actionPointsBonus = 0;
        stru.accuracyBonus = 0;
        stru.damageBonus = 0;
        stru.minDamage = 0;
        stru.maxDamage = INT_MAX;
        stru.field_1C = 0;
        combat(&stru);
        return;
    }

    if (!aiming) {
        combat_attack(obj_dude, a1, hitMode, HIT_LOCATION_UNCALLED);
        return;
    }

    if (aiming != 1) {
        debugPrint("Bad called shot value %d\n", aiming);
    }

    int hitLocation;
    if (get_called_shot_location(a1, &hitLocation, hitMode) != -1) {
        combat_attack(obj_dude, a1, hitMode, hitLocation);
    }
}

// Highlights critters.
//
// 0x426AA8
void combat_outline_on()
{
    int targetHighlight = TARGET_HIGHLIGHT_TARGETING_ONLY;
    config_get_value(&game_config, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_TARGET_HIGHLIGHT_KEY, &targetHighlight);
    if (targetHighlight == TARGET_HIGHLIGHT_OFF) {
        return;
    }

    if (gmouse_3d_get_mode() != GAME_MOUSE_MODE_CROSSHAIR) {
        return;
    }

    if (isInCombat()) {
        for (int index = 0; index < list_total; index++) {
            combat_update_critter_outline_for_los(combat_list[index], 1);
        }
    } else {
        Object** critterList;
        int critterListLength = obj_create_list(-1, map_elevation, OBJ_TYPE_CRITTER, &critterList);
        for (int index = 0; index < critterListLength; index++) {
            Object* critter = critterList[index];
            if (critter != obj_dude && (critter->data.critter.combat.results & DAM_DEAD) == 0) {
                combat_update_critter_outline_for_los(critter, 1);
            }
        }

        if (critterListLength != 0) {
            obj_delete_list(critterList);
        }
    }

    // NOTE: Uninline.
    combat_update_critters_in_los(1);

    tileWindowRefresh();
}

// 0x426BC0
void combat_outline_off()
{
    int i;
    int v5;
    Object** v9;

    if (combat_state & 1) {
        for (i = 0; i < list_total; i++) {
            obj_turn_off_outline(combat_list[i], NULL);
        }
    } else {
        v5 = obj_create_list(-1, map_elevation, 1, &v9);
        for (i = 0; i < v5; i++) {
            obj_turn_off_outline(v9[i], NULL);
            obj_remove_outline(v9[i], NULL);
        }
        if (v5) {
            obj_delete_list(v9);
        }
    }

    tileWindowRefresh();
}

// 0x426C64
void combat_highlight_change()
{
    int targetHighlight = 2;
    config_get_value(&game_config, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_TARGET_HIGHLIGHT_KEY, &targetHighlight);
    if (targetHighlight != combat_highlight && isInCombat()) {
        if (targetHighlight != 0) {
            if (combat_highlight == 0) {
                combat_outline_on();
            }
        } else {
            combat_outline_off();
        }
    }

    combat_highlight = targetHighlight;
}

// Probably calculates line of sight or determines if object can see other object.
//
// 0x426CC4
bool combat_is_shot_blocked(Object* a1, int from, int to, Object* a4, int* a5)
{
    if (a5 != NULL) {
        *a5 = 0;
    }

    Object* obstacle = a1;
    int current = from;
    while (obstacle != NULL && current != to) {
        make_straight_path_func(a1, current, to, 0, &obstacle, 32, obj_shoot_blocking_at);
        if (obstacle != NULL) {
            if (FID_TYPE(obstacle->fid) != OBJ_TYPE_CRITTER && obstacle != a4) {
                return true;
            }

            if (a5 != NULL) {
                if (obstacle != a4) {
                    if (a4 != NULL) {
                        if ((a4->data.critter.combat.results & DAM_DEAD) == 0) {
                            *a5 += 1;

                            if ((a4->flags & OBJECT_MULTIHEX) != 0) {
                                *a5 += 1;
                            }
                        }
                    }
                }
            }

            if ((obstacle->flags & OBJECT_MULTIHEX) != 0) {
                int rotation = tileGetRotationTo(current, to);
                current = tileGetTileInDirection(current, rotation, 1);
            } else {
                current = obstacle->tile;
            }
        }
    }

    return false;
}

// 0x426D94
int combat_player_knocked_out_by()
{
    if ((obj_dude->data.critter.combat.results & DAM_DEAD) != 0) {
        return -1;
    }

    if (combat_ending_guy == NULL) {
        return -1;
    }

    return combat_ending_guy->data.critter.combat.team;
}

// 0x426DB8
int combat_explode_scenery(Object* a1, Object* a2)
{
    _scr_explode_scenery(a1, a1->tile, item_w_rocket_dmg_radius(NULL), a1->elevation);
    return 0;
}

// 0x426DDC
void combat_delete_critter(Object* obj)
{
    // TODO: Check entire function.
    if (!isInCombat()) {
        return;
    }

    if (list_total == 0) {
        return;
    }

    int i;
    for (i = 0; i < list_total; i++) {
        if (obj == combat_list[i]) {
            break;
        }
    }

    if (i == list_total) {
        return;
    }

    while (i < (list_total - 1)) {
        combat_list[i] = combat_list[i + 1];
        combatCopyAIInfo(i + 1, i);
        i++;
    }

    list_total--;

    combat_list[list_total] = obj;

    if (i >= list_com) {
        if (i < (list_noncom + list_com)) {
            list_noncom--;
        }
    } else {
        list_com--;
    }

    obj->data.critter.combat.ap = 0;
    obj_remove_outline(obj, NULL);

    obj->data.critter.combat.whoHitMe = NULL;
    combatai_delete_critter(obj);
}

// 0x426EC4
void combatKillCritterOutsideCombat(Object* critter_obj, char* msg)
{
    if (critter_obj != obj_dude) {
        display_print(msg);
        scriptExecProc(critter_obj->sid, SCRIPT_PROC_DESTROY);
        critter_kill(critter_obj, -1, 1);
    }
}
