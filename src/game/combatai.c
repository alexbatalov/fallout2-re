#include "game/combatai.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "game/actions.h"
#include "game/anim.h"
#include "game/combat.h"
#include "game/config.h"
#include "core.h"
#include "game/critter.h"
#include "debug.h"
#include "game/display.h"
#include "game/game.h"
#include "game/gconfig.h"
#include "game/gsound.h"
#include "interface.h"
#include "item.h"
#include "light.h"
#include "map.h"
#include "memory.h"
#include "object.h"
#include "proto.h"
#include "proto_instance.h"
#include "random.h"
#include "scripts.h"
#include "skill.h"
#include "stat.h"
#include "text_object.h"
#include "tile.h"

static void parse_hurt_str(char* str, int* out_value);
static int cai_match_str_to_list(const char* str, const char** list, int count, int* out_value);
static void cai_init_cap(AiPacket* ai);
static int cai_cap_load(File* stream, AiPacket* ai);
static int cai_cap_save(File* stream, AiPacket* ai);
static AiPacket* ai_cap(Object* obj);
static AiPacket* ai_cap_from_packet(int aiPacketNum);
static int ai_magic_hands(Object* a1, Object* a2, int num);
static int ai_check_drugs(Object* critter);
static void ai_run_away(Object* a1, Object* a2);
static int ai_move_away(Object* a1, Object* a2, int a3);
static bool ai_find_friend(Object* a1, int a2, int a3);
static int compare_nearer(const void* a1, const void* a2);
static void ai_sort_list_distance(Object** critterList, int length, Object* origin);
static void ai_sort_list_strength(Object** critterList, int length);
static void ai_sort_list_weakness(Object** critterList, int length);
static Object* ai_find_nearest_team(Object* a1, Object* a2, int a3);
static Object* ai_find_nearest_team_in_combat(Object* a1, Object* a2, int a3);
static int ai_find_attackers(Object* a1, Object** a2, Object** a3, Object** a4);
static int ai_have_ammo(Object* critter_obj, Object* weapon_obj, Object** out_ammo_obj);
static bool caiHasWeapPrefType(AiPacket* ai, int attackType);
static Object* ai_best_weapon(Object* a1, Object* a2, Object* a3, Object* a4);
static bool ai_can_use_weapon(Object* critter, Object* weapon, int hitMode);
static bool ai_can_use_drug(Object* obj, Object* a2);
static Object* ai_search_environ(Object* critter, int itemType);
static Object* ai_retrieve_object(Object* a1, Object* a2);
static int ai_pick_hit_mode(Object* a1, Object* a2, Object* a3);
static int ai_move_steps_closer(Object* a1, Object* a2, int actionPoints, int a4);
static int ai_move_closer(Object* a1, Object* a2, int a3);
static int cai_retargetTileFromFriendlyFire(Object* source, Object* target, int* tilePtr);
static int cai_retargetTileFromFriendlyFireSubFunc(AiRetargetData* aiRetargetData, int tile);
static bool cai_attackWouldIntersect(Object* attacker, Object* defender, Object* attackerFriend, int tile, int* distance);
static int ai_switch_weapons(Object* a1, int* hitMode, Object** weapon, Object* a4);
static int ai_called_shot(Object* a1, Object* a2, int a3);
static int ai_attack(Object* a1, Object* a2, int a3);
static int ai_try_attack(Object* a1, Object* a2);
static int cai_perform_distance_prefs(Object* a1, Object* a2);
static int cai_get_min_hp(AiPacket* ai);
static int ai_print_msg(Object* critter, int type);
static int combatai_rating(Object* obj);
static int combatai_load_messages();
static int combatai_unload_messages();

// 0x51805C
static Object* combat_obj = NULL;

// 0x518060
static int num_caps = 0;

// 0x518064
static AiPacket* cap = NULL;

// 0x518068
static bool combatai_is_initialized = false;

// 0x51806C
const char* area_attack_mode_strs[AREA_ATTACK_MODE_COUNT] = {
    "always",
    "sometimes",
    "be_sure",
    "be_careful",
    "be_absolutely_sure",
};

// 0x5180D0
const char* attack_who_mode_strs[ATTACK_WHO_COUNT] = {
    "whomever_attacking_me",
    "strongest",
    "weakest",
    "whomever",
    "closest",
};

// 0x51809C
const char* weapon_pref_strs[BEST_WEAPON_COUNT] = {
    "no_pref",
    "melee",
    "melee_over_ranged",
    "ranged_over_melee",
    "ranged",
    "unarmed",
    "unarmed_over_thrown",
    "random",
};

// 0x5180E4
const char* chem_use_mode_strs[CHEM_USE_COUNT] = {
    "clean",
    "stims_when_hurt_little",
    "stims_when_hurt_lots",
    "sometimes",
    "anytime",
    "always",
};

// 0x5180BC
const char* distance_pref_strs[DISTANCE_COUNT] = {
    "stay_close",
    "charge",
    "snipe",
    "on_your_own",
    "stay",
};

// 0x518080
const char* run_away_mode_strs[RUN_AWAY_MODE_COUNT] = {
    "none",
    "coward",
    "finger_hurts",
    "bleeding",
    "not_feeling_good",
    "tourniquet",
    "never",
};

// 0x5180FC
const char* disposition_strs[DISPOSITION_COUNT] = {
    "none",
    "custom",
    "coward",
    "defensive",
    "aggressive",
    "berserk",
};

// 0x518114
static const char* matchHurtStrs[HURT_COUNT] = {
    "blind",
    "crippled",
    "crippled_legs",
    "crippled_arms",
};

// hurt_too_much
//
// 0x518124
static int rmatchHurtVals[5] = {
    DAM_BLIND,
    DAM_CRIP_LEG_LEFT | DAM_CRIP_LEG_RIGHT | DAM_CRIP_ARM_LEFT | DAM_CRIP_ARM_RIGHT,
    DAM_CRIP_LEG_LEFT | DAM_CRIP_LEG_RIGHT,
    DAM_CRIP_ARM_LEFT | DAM_CRIP_ARM_RIGHT,
    0,
};

// Hit points in percent to choose run away mode.
//
// 0x518138
static int runModeValues[6] = {
    0,
    25,
    40,
    60,
    75,
    100,
};

// 0x518150
static Object* attackerTeamObj = NULL;

// 0x518154
static Object* targetTeamObj = NULL;

// 0x518158
static int weapPrefOrderings[BEST_WEAPON_COUNT + 1][5] = {
    { ATTACK_TYPE_RANGED, ATTACK_TYPE_THROW, ATTACK_TYPE_MELEE, ATTACK_TYPE_UNARMED, 0 },
    { ATTACK_TYPE_RANGED, ATTACK_TYPE_THROW, ATTACK_TYPE_MELEE, ATTACK_TYPE_UNARMED, 0 }, // BEST_WEAPON_NO_PREF
    { ATTACK_TYPE_MELEE, 0, 0, 0, 0 }, // BEST_WEAPON_MELEE
    { ATTACK_TYPE_MELEE, ATTACK_TYPE_RANGED, 0, 0, 0 }, // BEST_WEAPON_MELEE_OVER_RANGED
    { ATTACK_TYPE_RANGED, ATTACK_TYPE_MELEE, 0, 0, 0 }, // BEST_WEAPON_RANGED_OVER_MELEE
    { ATTACK_TYPE_RANGED, 0, 0, 0, 0 }, // BEST_WEAPON_RANGED
    { ATTACK_TYPE_UNARMED, 0, 0, 0, 0 }, // BEST_WEAPON_UNARMED
    { ATTACK_TYPE_UNARMED, ATTACK_TYPE_THROW, 0, 0, 0 }, // BEST_WEAPON_UNARMED_OVER_THROW
    { 0, 0, 0, 0, 0 }, // BEST_WEAPON_RANDOM
};

// ai.msg
//
// 0x56D510
static MessageList ai_message_file;

// 0x56D518
static char target_str[260];

// 0x56D61C
static int curr_crit_num;

// 0x56D620
static Object** curr_crit_list;

// 0x56D624
static char attack_str[268];

// parse hurt_too_much
static void parse_hurt_str(char* str, int* valuePtr)
{
    int v5, v10;
    char tmp;
    int i;

    *valuePtr = 0;

    str = strlwr(str);
    while (*str) {
        v5 = strspn(str, " ");
        str += v5;

        v10 = strcspn(str, ",");
        tmp = str[v10];
        str[v10] = '\0';

        for (i = 0; i < 4; i++) {
            if (strcmp(str, matchHurtStrs[i]) == 0) {
                *valuePtr |= rmatchHurtVals[i];
                break;
            }
        }

        if (i == 4) {
            debugPrint("Unrecognized flag: %s\n", str);
        }

        str[v10] = tmp;

        if (tmp == '\0') {
            break;
        }

        str += v10 + 1;
    }
}

// parse behaviour entry
static int cai_match_str_to_list(const char* str, const char** list, int count, int* valuePtr)
{
    *valuePtr = -1;
    for (int index = 0; index < count; index++) {
        if (stricmp(str, list[index]) == 0) {
            *valuePtr = index;
        }
    }

    return 0;
}

// 0x426FE0
static void cai_init_cap(AiPacket* ai)
{
    ai->name = NULL;

    ai->area_attack_mode = -1;
    ai->run_away_mode = -1;
    ai->best_weapon = -1;
    ai->distance = -1;
    ai->attack_who = -1;
    ai->chem_use = -1;

    for (int index = 0; index < AI_PACKET_CHEM_PRIMARY_DESIRE_COUNT; index++) {
        ai->chem_primary_desire[index] = -1;
    }

    ai->disposition = -1;
}

// ai_init
// 0x42703C
int combat_ai_init()
{
    int index;

    if (combatai_load_messages() == -1) {
        return -1;
    }

    num_caps = 0;

    Config config;
    if (!config_init(&config)) {
        return -1;
    }

    if (!config_load(&config, "data\\ai.txt", true)) {
        return -1;
    }

    cap = (AiPacket*)internal_malloc(sizeof(*cap) * config.entriesLength);
    if (cap == NULL) {
        goto err;
    }

    for (index = 0; index < config.entriesLength; index++) {
        cai_init_cap(&(cap[index]));
    }

    for (index = 0; index < config.entriesLength; index++) {
        DictionaryEntry* sectionEntry = &(config.entries[index]);
        AiPacket* ai = &(cap[index]);
        char* stringValue;

        ai->name = internal_strdup(sectionEntry->key);

        if (!config_get_value(&config, sectionEntry->key, "packet_num", &(ai->packet_num))) goto err;
        if (!config_get_value(&config, sectionEntry->key, "max_dist", &(ai->max_dist))) goto err;
        if (!config_get_value(&config, sectionEntry->key, "min_to_hit", &(ai->min_to_hit))) goto err;
        if (!config_get_value(&config, sectionEntry->key, "min_hp", &(ai->min_hp))) goto err;
        if (!config_get_value(&config, sectionEntry->key, "aggression", &(ai->aggression))) goto err;

        if (config_get_string(&config, sectionEntry->key, "hurt_too_much", &stringValue)) {
            parse_hurt_str(stringValue, &(ai->hurt_too_much));
        }

        if (!config_get_value(&config, sectionEntry->key, "secondary_freq", &(ai->secondary_freq))) goto err;
        if (!config_get_value(&config, sectionEntry->key, "called_freq", &(ai->called_freq))) goto err;
        if (!config_get_value(&config, sectionEntry->key, "font", &(ai->font))) goto err;
        if (!config_get_value(&config, sectionEntry->key, "color", &(ai->color))) goto err;
        if (!config_get_value(&config, sectionEntry->key, "outline_color", &(ai->outline_color))) goto err;
        if (!config_get_value(&config, sectionEntry->key, "chance", &(ai->chance))) goto err;
        if (!config_get_value(&config, sectionEntry->key, "run_start", &(ai->run.start))) goto err;
        if (!config_get_value(&config, sectionEntry->key, "run_end", &(ai->run.end))) goto err;
        if (!config_get_value(&config, sectionEntry->key, "move_start", &(ai->move.start))) goto err;
        if (!config_get_value(&config, sectionEntry->key, "move_end", &(ai->move.end))) goto err;
        if (!config_get_value(&config, sectionEntry->key, "attack_start", &(ai->attack.start))) goto err;
        if (!config_get_value(&config, sectionEntry->key, "attack_end", &(ai->attack.end))) goto err;
        if (!config_get_value(&config, sectionEntry->key, "miss_start", &(ai->miss.start))) goto err;
        if (!config_get_value(&config, sectionEntry->key, "miss_end", &(ai->miss.end))) goto err;
        if (!config_get_value(&config, sectionEntry->key, "hit_head_start", &(ai->hit[HIT_LOCATION_HEAD].start))) goto err;
        if (!config_get_value(&config, sectionEntry->key, "hit_head_end", &(ai->hit[HIT_LOCATION_HEAD].end))) goto err;
        if (!config_get_value(&config, sectionEntry->key, "hit_left_arm_start", &(ai->hit[HIT_LOCATION_LEFT_ARM].start))) goto err;
        if (!config_get_value(&config, sectionEntry->key, "hit_left_arm_end", &(ai->hit[HIT_LOCATION_LEFT_ARM].end))) goto err;
        if (!config_get_value(&config, sectionEntry->key, "hit_right_arm_start", &(ai->hit[HIT_LOCATION_RIGHT_ARM].start))) goto err;
        if (!config_get_value(&config, sectionEntry->key, "hit_right_arm_end", &(ai->hit[HIT_LOCATION_RIGHT_ARM].end))) goto err;
        if (!config_get_value(&config, sectionEntry->key, "hit_torso_start", &(ai->hit[HIT_LOCATION_TORSO].start))) goto err;
        if (!config_get_value(&config, sectionEntry->key, "hit_torso_end", &(ai->hit[HIT_LOCATION_TORSO].end))) goto err;
        if (!config_get_value(&config, sectionEntry->key, "hit_right_leg_start", &(ai->hit[HIT_LOCATION_RIGHT_LEG].start))) goto err;
        if (!config_get_value(&config, sectionEntry->key, "hit_right_leg_end", &(ai->hit[HIT_LOCATION_RIGHT_LEG].end))) goto err;
        if (!config_get_value(&config, sectionEntry->key, "hit_left_leg_start", &(ai->hit[HIT_LOCATION_LEFT_LEG].start))) goto err;
        if (!config_get_value(&config, sectionEntry->key, "hit_left_leg_end", &(ai->hit[HIT_LOCATION_LEFT_LEG].end))) goto err;
        if (!config_get_value(&config, sectionEntry->key, "hit_eyes_start", &(ai->hit[HIT_LOCATION_EYES].start))) goto err;
        if (!config_get_value(&config, sectionEntry->key, "hit_eyes_end", &(ai->hit[HIT_LOCATION_EYES].end))) goto err;
        if (!config_get_value(&config, sectionEntry->key, "hit_groin_start", &(ai->hit[HIT_LOCATION_GROIN].start))) goto err;
        if (!config_get_value(&config, sectionEntry->key, "hit_groin_end", &(ai->hit[HIT_LOCATION_GROIN].end))) goto err;

        ai->hit[HIT_LOCATION_GROIN].end++;

        if (config_get_string(&config, sectionEntry->key, "area_attack_mode", &stringValue)) {
            cai_match_str_to_list(stringValue, area_attack_mode_strs, AREA_ATTACK_MODE_COUNT, &(ai->area_attack_mode));
        } else {
            ai->run_away_mode = -1;
        }

        if (config_get_string(&config, sectionEntry->key, "run_away_mode", &stringValue)) {
            cai_match_str_to_list(stringValue, run_away_mode_strs, RUN_AWAY_MODE_COUNT, &(ai->run_away_mode));

            if (ai->run_away_mode >= 0) {
                ai->run_away_mode--;
            }
        }

        if (config_get_string(&config, sectionEntry->key, "best_weapon", &stringValue)) {
            cai_match_str_to_list(stringValue, weapon_pref_strs, BEST_WEAPON_COUNT, &(ai->best_weapon));
        }

        if (config_get_string(&config, sectionEntry->key, "distance", &stringValue)) {
            cai_match_str_to_list(stringValue, distance_pref_strs, DISTANCE_COUNT, &(ai->distance));
        }

        if (config_get_string(&config, sectionEntry->key, "attack_who", &stringValue)) {
            cai_match_str_to_list(stringValue, attack_who_mode_strs, ATTACK_WHO_COUNT, &(ai->attack_who));
        }

        if (config_get_string(&config, sectionEntry->key, "chem_use", &stringValue)) {
            cai_match_str_to_list(stringValue, chem_use_mode_strs, CHEM_USE_COUNT, &(ai->chem_use));
        }

        config_get_values(&config, sectionEntry->key, "chem_primary_desire", ai->chem_primary_desire, AI_PACKET_CHEM_PRIMARY_DESIRE_COUNT);

        if (config_get_string(&config, sectionEntry->key, "disposition", &stringValue)) {
            cai_match_str_to_list(stringValue, disposition_strs, DISPOSITION_COUNT, &(ai->disposition));
            ai->disposition--;
        }

        if (config_get_string(&config, sectionEntry->key, "body_type", &stringValue)) {
            ai->body_type = internal_strdup(stringValue);
        } else {
            ai->body_type = NULL;
        }

        if (config_get_string(&config, sectionEntry->key, "general_type", &stringValue)) {
            ai->general_type = internal_strdup(stringValue);
        } else {
            ai->general_type = NULL;
        }
    }

    if (index < config.entriesLength) {
        goto err;
    }

    num_caps = config.entriesLength;

    config_exit(&config);

    combatai_is_initialized = true;

    return 0;

err:

    if (cap != NULL) {
        for (index = 0; index < config.entriesLength; index++) {
            AiPacket* ai = &(cap[index]);
            if (ai->name != NULL) {
                internal_free(ai->name);
            }

            // FIXME: leaking ai->body_type and ai->general_type, does not matter
            // because it halts further processing
        }
        internal_free(cap);
    }

    debugPrint("Error processing ai.txt");

    config_exit(&config);

    return -1;
}

// 0x4279F8
void combat_ai_reset()
{
}

// 0x4279FC
int combat_ai_exit()
{
    for (int index = 0; index < num_caps; index++) {
        AiPacket* ai = &(cap[index]);

        if (ai->name != NULL) {
            internal_free(ai->name);
            ai->name = NULL;
        }

        if (ai->general_type != NULL) {
            internal_free(ai->general_type);
            ai->general_type = NULL;
        }

        if (ai->body_type != NULL) {
            internal_free(ai->body_type);
            ai->body_type = NULL;
        }
    }

    internal_free(cap);
    num_caps = 0;

    combatai_is_initialized = false;

    // NOTE: Uninline.
    if (combatai_unload_messages() != 0) {
        return -1;
    }

    return 0;
}

// 0x427AD8
int combat_ai_load(File* stream)
{
    for (int index = 0; index < gPartyMemberDescriptionsLength; index++) {
        int pid = gPartyMemberPids[index];
        if (pid != -1 && PID_TYPE(pid) == OBJ_TYPE_CRITTER) {
            Proto* proto;
            if (protoGetProto(pid, &proto) == -1) {
                return -1;
            }

            AiPacket* ai = ai_cap_from_packet(proto->critter.aiPacket);
            if (ai->disposition == 0) {
                cai_cap_load(stream, ai);
            }
        }
    }

    return 0;
}

// 0x427B50
int combat_ai_save(File* stream)
{
    for (int index = 0; index < gPartyMemberDescriptionsLength; index++) {
        int pid = gPartyMemberPids[index];
        if (pid != -1 && PID_TYPE(pid) == OBJ_TYPE_CRITTER) {
            Proto* proto;
            if (protoGetProto(pid, &proto) == -1) {
                return -1;
            }

            AiPacket* ai = ai_cap_from_packet(proto->critter.aiPacket);
            if (ai->disposition == 0) {
                cai_cap_save(stream, ai);
            }
        }
    }

    return 0;
}

// 0x427BC8
static int cai_cap_load(File* stream, AiPacket* ai)
{
    if (fileReadInt32(stream, &(ai->packet_num)) == -1) return -1;
    if (fileReadInt32(stream, &(ai->max_dist)) == -1) return -1;
    if (fileReadInt32(stream, &(ai->min_to_hit)) == -1) return -1;
    if (fileReadInt32(stream, &(ai->min_hp)) == -1) return -1;
    if (fileReadInt32(stream, &(ai->aggression)) == -1) return -1;
    if (fileReadInt32(stream, &(ai->hurt_too_much)) == -1) return -1;
    if (fileReadInt32(stream, &(ai->secondary_freq)) == -1) return -1;
    if (fileReadInt32(stream, &(ai->called_freq)) == -1) return -1;
    if (fileReadInt32(stream, &(ai->font)) == -1) return -1;
    if (fileReadInt32(stream, &(ai->color)) == -1) return -1;
    if (fileReadInt32(stream, &(ai->outline_color)) == -1) return -1;
    if (fileReadInt32(stream, &(ai->chance)) == -1) return -1;
    if (fileReadInt32(stream, &(ai->run.start)) == -1) return -1;
    if (fileReadInt32(stream, &(ai->run.end)) == -1) return -1;
    if (fileReadInt32(stream, &(ai->move.start)) == -1) return -1;
    if (fileReadInt32(stream, &(ai->move.end)) == -1) return -1;
    if (fileReadInt32(stream, &(ai->attack.start)) == -1) return -1;
    if (fileReadInt32(stream, &(ai->attack.end)) == -1) return -1;
    if (fileReadInt32(stream, &(ai->miss.start)) == -1) return -1;
    if (fileReadInt32(stream, &(ai->miss.end)) == -1) return -1;

    for (int index = 0; index < HIT_LOCATION_SPECIFIC_COUNT; index++) {
        AiMessageRange* range = &(ai->hit[index]);
        if (fileReadInt32(stream, &(range->start)) == -1) return -1;
        if (fileReadInt32(stream, &(range->end)) == -1) return -1;
    }

    if (fileReadInt32(stream, &(ai->area_attack_mode)) == -1) return -1;
    if (fileReadInt32(stream, &(ai->best_weapon)) == -1) return -1;
    if (fileReadInt32(stream, &(ai->distance)) == -1) return -1;
    if (fileReadInt32(stream, &(ai->attack_who)) == -1) return -1;
    if (fileReadInt32(stream, &(ai->chem_use)) == -1) return -1;
    if (fileReadInt32(stream, &(ai->run_away_mode)) == -1) return -1;

    for (int index = 0; index < AI_PACKET_CHEM_PRIMARY_DESIRE_COUNT; index++) {
        if (fileReadInt32(stream, &(ai->chem_primary_desire[index])) == -1) return -1;
    }

    return 0;
}

// 0x427E1C
static int cai_cap_save(File* stream, AiPacket* ai)
{
    if (fileWriteInt32(stream, ai->packet_num) == -1) return -1;
    if (fileWriteInt32(stream, ai->max_dist) == -1) return -1;
    if (fileWriteInt32(stream, ai->min_to_hit) == -1) return -1;
    if (fileWriteInt32(stream, ai->min_hp) == -1) return -1;
    if (fileWriteInt32(stream, ai->aggression) == -1) return -1;
    if (fileWriteInt32(stream, ai->hurt_too_much) == -1) return -1;
    if (fileWriteInt32(stream, ai->secondary_freq) == -1) return -1;
    if (fileWriteInt32(stream, ai->called_freq) == -1) return -1;
    if (fileWriteInt32(stream, ai->font) == -1) return -1;
    if (fileWriteInt32(stream, ai->color) == -1) return -1;
    if (fileWriteInt32(stream, ai->outline_color) == -1) return -1;
    if (fileWriteInt32(stream, ai->chance) == -1) return -1;
    if (fileWriteInt32(stream, ai->run.start) == -1) return -1;
    if (fileWriteInt32(stream, ai->run.end) == -1) return -1;
    if (fileWriteInt32(stream, ai->move.start) == -1) return -1;
    if (fileWriteInt32(stream, ai->move.end) == -1) return -1;
    if (fileWriteInt32(stream, ai->attack.start) == -1) return -1;
    if (fileWriteInt32(stream, ai->attack.end) == -1) return -1;
    if (fileWriteInt32(stream, ai->miss.start) == -1) return -1;
    if (fileWriteInt32(stream, ai->miss.end) == -1) return -1;

    for (int index = 0; index < HIT_LOCATION_SPECIFIC_COUNT; index++) {
        AiMessageRange* range = &(ai->hit[index]);
        if (fileWriteInt32(stream, range->start) == -1) return -1;
        if (fileWriteInt32(stream, range->end) == -1) return -1;
    }

    if (fileWriteInt32(stream, ai->area_attack_mode) == -1) return -1;
    if (fileWriteInt32(stream, ai->best_weapon) == -1) return -1;
    if (fileWriteInt32(stream, ai->distance) == -1) return -1;
    if (fileWriteInt32(stream, ai->attack_who) == -1) return -1;
    if (fileWriteInt32(stream, ai->chem_use) == -1) return -1;
    if (fileWriteInt32(stream, ai->run_away_mode) == -1) return -1;

    for (int index = 0; index < AI_PACKET_CHEM_PRIMARY_DESIRE_COUNT; index++) {
        // TODO: Check, probably writes chem_primary_desire[0] three times,
        // might be a bug in original source code.
        if (fileWriteInt32(stream, ai->chem_primary_desire[index]) == -1) return -1;
    }

    return 0;
}

// NOTE: Unused.
//
// 0x428058
int combat_ai_num()
{
    return num_caps;
}

// 0x428060
char* combat_ai_name(int packetNum)
{
    int index;

    if (packetNum < 0 || packetNum >= num_caps) {
        return NULL;
    }

    for (index = 0; index < num_caps; index++) {
        if (cap[index].packet_num == packetNum) {
            return cap[index].name;
        }
    }

    return NULL;
}

// Get ai from object
//
// 0x4280B4
static AiPacket* ai_cap(Object* obj)
{
    // NOTE: Uninline.
    AiPacket* ai = ai_cap_from_packet(obj->data.critter.combat.aiPacket);
    return ai;
}

// get ai packet by num
//
// 0x42811C
static AiPacket* ai_cap_from_packet(int aiPacketId)
{
    for (int index = 0; index < num_caps; index++) {
        AiPacket* ai = &(cap[index]);
        if (aiPacketId == ai->packet_num) {
            return ai;
        }
    }

    debugPrint("Missing AI Packet\n");

    return cap;
}

// 0x428184
int ai_get_burst_value(Object* obj)
{
    AiPacket* ai = ai_cap(obj);
    return ai->area_attack_mode;
}

// 0x428190
int ai_get_run_away_value(Object* obj)
{
    AiPacket* ai;
    int v3;
    int v5;
    int v6;
    int i;

    ai = ai_cap(obj);
    v3 = -1;

    if (ai->run_away_mode != -1) {
        return ai->run_away_mode;
    }

    v5 = 100 * ai->min_hp;
    v6 = v5 / critterGetStat(obj, STAT_MAXIMUM_HIT_POINTS);

    for (i = 0; i < 6; i++) {
        if (v6 >= runModeValues[i]) {
            v3 = i;
        }
    }

    return v3;
}

// 0x4281FC
int ai_get_weapon_pref_value(Object* obj)
{
    AiPacket* ai = ai_cap(obj);
    return ai->best_weapon;
}

// 0x428208
int ai_get_distance_pref_value(Object* obj)
{
    AiPacket* ai = ai_cap(obj);
    return ai->distance;
}

// 0x428214
int ai_get_attack_who_value(Object* obj)
{
    AiPacket* ai = ai_cap(obj);
    return ai->attack_who;
}

// 0x428220
int ai_get_chem_use_value(Object* obj)
{
    AiPacket* ai = ai_cap(obj);
    return ai->chem_use;
}

// 0x42822C
int ai_set_burst_value(Object* critter, int areaAttackMode)
{
    if (areaAttackMode >= AREA_ATTACK_MODE_COUNT) {
        return -1;
    }

    AiPacket* ai = ai_cap(critter);
    ai->area_attack_mode = areaAttackMode;
    return 0;
}

// 0x428248
int ai_set_run_away_value(Object* obj, int runAwayMode)
{
    if (runAwayMode >= 6) {
        return -1;
    }

    AiPacket* ai = ai_cap(obj);
    ai->run_away_mode = runAwayMode;

    int maximumHp = critterGetStat(obj, STAT_MAXIMUM_HIT_POINTS);
    ai->min_hp = maximumHp - maximumHp * runModeValues[runAwayMode] / 100;

    int currentHp = critterGetStat(obj, STAT_CURRENT_HIT_POINTS);
    const char* name = critter_name(obj);

    debugPrint("\n%s minHp = %d; curHp = %d", name, ai->min_hp, currentHp);

    return 0;
}

// 0x4282D0
int ai_set_weapon_pref_value(Object* critter, int bestWeapon)
{
    if (bestWeapon >= BEST_WEAPON_COUNT) {
        return -1;
    }

    AiPacket* ai = ai_cap(critter);
    ai->best_weapon = bestWeapon;
    return 0;
}

// 0x4282EC
int ai_set_distance_pref_value(Object* critter, int distance)
{
    if (distance >= DISTANCE_COUNT) {
        return -1;
    }

    AiPacket* ai = ai_cap(critter);
    ai->distance = distance;
    return 0;
}

// 0x428308
int ai_set_attack_who_value(Object* critter, int attackWho)
{
    if (attackWho >= ATTACK_WHO_COUNT) {
        return -1;
    }

    AiPacket* ai = ai_cap(critter);
    ai->attack_who = attackWho;
    return 0;
}

// 0x428324
int ai_set_chem_use_value(Object* critter, int chemUse)
{
    if (chemUse >= CHEM_USE_COUNT) {
        return -1;
    }

    AiPacket* ai = ai_cap(critter);
    ai->chem_use = chemUse;
    return 0;
}

// 0x428340
int ai_get_disposition(Object* obj)
{
    if (obj == NULL) {
        return 0;
    }

    AiPacket* ai = ai_cap(obj);
    return ai->disposition;
}

// 0x428354
int ai_set_disposition(Object* obj, int disposition)
{
    if (obj == NULL) {
        return -1;
    }

    if (disposition == -1 || disposition >= 5) {
        return -1;
    }

    AiPacket* ai = ai_cap(obj);
    obj->data.critter.combat.aiPacket = ai->packet_num - (disposition - ai->disposition);

    return 0;
}

// 0x428398
static int ai_magic_hands(Object* critter, Object* item, int num)
{
    register_begin(ANIMATION_REQUEST_RESERVED);

    register_object_animate(critter, ANIM_MAGIC_HANDS_MIDDLE, 0);

    if (register_end() == 0) {
        if (isInCombat()) {
            combat_turn_run();
        }
    }

    if (num != -1) {
        MessageListItem messageListItem;
        messageListItem.num = num;
        if (messageListGetItem(&misc_message_file, &messageListItem)) {
            const char* critterName = objectGetName(critter);

            char text[200];
            if (item != NULL) {
                const char* itemName = objectGetName(item);
                sprintf(text, "%s %s %s.", critterName, messageListItem.text, itemName);
            } else {
                sprintf(text, "%s %s.", critterName, messageListItem.text);
            }

            display_print(text);
        }
    }

    return 0;
}

// ai using drugs
// 0x428480
static int ai_check_drugs(Object* critter)
{
    if (critter_body_type(critter) != BODY_TYPE_BIPED) {
        return 0;
    }

    int v25 = 0;
    int v28 = 0;
    int v29 = 0;
    Object* v3 = combatAIInfoGetLastItem(critter);
    if (v3 == NULL) {
        AiPacket* ai = ai_cap(critter);
        if (ai == NULL) {
            return 0;
        }

        int v2 = 50;
        int v26 = 0;
        switch (ai->chem_use + 1) {
        case 1:
            return 0;
        case 2:
            v2 = 60;
            break;
        case 3:
            v2 = 30;
            break;
        case 4:
            if ((combatNumTurns % 3) == 0) {
                v26 = 25;
            }
            v2 = 50;
            break;
        case 5:
            if ((combatNumTurns % 3) == 0) {
                v26 = 75;
            }
            v2 = 50;
            break;
        case 6:
            v26 = 100;
            break;
        }

        int v27 = critterGetStat(critter, STAT_MAXIMUM_HIT_POINTS) * v2 / 100;
        int token = -1;
        while (true) {
            if (critterGetStat(critter, STAT_CURRENT_HIT_POINTS) >= v27 || critter->data.critter.combat.ap < 2) {
                break;
            }

            Object* drug = _inven_find_type(critter, ITEM_TYPE_DRUG, &token);
            if (drug == NULL) {
                v25 = true;
                break;
            }

            int drugPid = drug->pid;
            if ((drugPid == PROTO_ID_STIMPACK || drugPid == PROTO_ID_SUPER_STIMPACK || drugPid == PROTO_ID_HEALING_POWDER)
                && itemRemove(critter, drug, 1) == 0) {
                if (_item_d_take_drug(critter, drug) == -1) {
                    itemAdd(critter, drug, 1);
                } else {
                    ai_magic_hands(critter, drug, 5000);
                    _obj_connect(drug, critter->tile, critter->elevation, NULL);
                    _obj_destroy(drug);
                    v28 = 1;
                }

                if (critter->data.critter.combat.ap < 2) {
                    critter->data.critter.combat.ap = 0;
                } else {
                    critter->data.critter.combat.ap -= 2;
                }

                token = -1;
            }
        }

        if (!v28 && v26 > 0 && randomBetween(0, 100) < v26) {
            while (critter->data.critter.combat.ap >= 2) {
                Object* drug = _inven_find_type(critter, ITEM_TYPE_DRUG, &token);
                if (drug == NULL) {
                    v25 = 1;
                    break;
                }

                int drugPid = drug->pid;
                int index;
                for (index = 0; index < AI_PACKET_CHEM_PRIMARY_DESIRE_COUNT; index++) {
                    // TODO: Find out why it checks for inequality at 0x4286B1.
                    if (ai->chem_primary_desire[index] != drugPid) {
                        break;
                    }
                }

                if (index < AI_PACKET_CHEM_PRIMARY_DESIRE_COUNT) {
                    if (drugPid != PROTO_ID_STIMPACK && drugPid != PROTO_ID_SUPER_STIMPACK && drugPid != 273
                        && itemRemove(critter, drug, 1) == 0) {
                        if (_item_d_take_drug(critter, drug) == -1) {
                            itemAdd(critter, drug, 1);
                        } else {
                            ai_magic_hands(critter, drug, 5000);
                            _obj_connect(drug, critter->tile, critter->elevation, NULL);
                            _obj_destroy(drug);
                            v28 = 1;
                            v29 += 1;
                        }

                        if (critter->data.critter.combat.ap < 2) {
                            critter->data.critter.combat.ap = 0;
                        } else {
                            critter->data.critter.combat.ap -= 2;
                        }

                        if (ai->chem_use == CHEM_USE_SOMETIMES || (ai->chem_use == CHEM_USE_ANYTIME && v29 >= 2)) {
                            break;
                        }
                    }
                }
            }
        }
    }

    if (v3 != NULL || (!v28 && v25 == 1)) {
        do {
            if (v3 == NULL) {
                v3 = ai_search_environ(critter, ITEM_TYPE_DRUG);
            }

            if (v3 != NULL) {
                v3 = ai_retrieve_object(critter, v3);
            } else {
                Object* v22 = ai_search_environ(critter, ITEM_TYPE_MISC);
                if (v22 != NULL) {
                    v3 = ai_retrieve_object(critter, v22);
                }
            }

            if (v3 != NULL && itemRemove(critter, v3, 1) == 0) {
                if (_item_d_take_drug(critter, v3) == -1) {
                    itemAdd(critter, v3, 1);
                } else {
                    ai_magic_hands(critter, v3, 5000);
                    _obj_connect(v3, critter->tile, critter->elevation, NULL);
                    _obj_destroy(v3);
                    v3 = NULL;
                }

                if (critter->data.critter.combat.ap < 2) {
                    critter->data.critter.combat.ap = 0;
                } else {
                    critter->data.critter.combat.ap -= 2;
                }
            }

        } while (v3 != NULL && critter->data.critter.combat.ap >= 2);
    }

    return 0;
}

// 0x428868
static void ai_run_away(Object* a1, Object* a2)
{
    if (a2 == NULL) {
        a2 = gDude;
    }

    CritterCombatData* combatData = &(a1->data.critter.combat);

    AiPacket* ai = ai_cap(a1);
    int distance = objectGetDistanceBetween(a1, a2);
    if (distance < ai->max_dist) {
        combatData->maneuver |= CRITTER_MANUEVER_FLEEING;

        int rotation = tileGetRotationTo(a2->tile, a1->tile);

        int destination;
        int actionPoints = combatData->ap;
        for (; actionPoints > 0; actionPoints -= 1) {
            destination = tileGetTileInDirection(a1->tile, rotation, actionPoints);
            if (make_path(a1, a1->tile, destination, NULL, 1) > 0) {
                break;
            }

            destination = tileGetTileInDirection(a1->tile, (rotation + 1) % ROTATION_COUNT, actionPoints);
            if (make_path(a1, a1->tile, destination, NULL, 1) > 0) {
                break;
            }

            destination = tileGetTileInDirection(a1->tile, (rotation + 5) % ROTATION_COUNT, actionPoints);
            if (make_path(a1, a1->tile, destination, NULL, 1) > 0) {
                break;
            }
        }

        if (actionPoints > 0) {
            register_begin(ANIMATION_REQUEST_RESERVED);
            combatai_msg(a1, NULL, AI_MESSAGE_TYPE_RUN, 0);
            register_object_run_to_tile(a1, destination, a1->elevation, combatData->ap, 0);
            if (register_end() == 0) {
                combat_turn_run();
            }
        }
    } else {
        combatData->maneuver |= CRITTER_MANEUVER_STOP_ATTACKING;
    }
}

// 0x42899C
static int ai_move_away(Object* a1, Object* a2, int a3)
{
    if (ai_cap(a1)->distance == DISTANCE_STAY) {
        return -1;
    }

    if (objectGetDistanceBetween(a1, a2) <= a3) {
        int actionPoints = a1->data.critter.combat.ap;
        if (a3 < actionPoints) {
            actionPoints = a3;
        }

        int rotation = tileGetRotationTo(a2->tile, a1->tile);

        int destination;
        int actionPointsLeft = actionPoints;
        for (; actionPointsLeft > 0; actionPointsLeft -= 1) {
            destination = tileGetTileInDirection(a1->tile, rotation, actionPointsLeft);
            if (make_path(a1, a1->tile, destination, NULL, 1) > 0) {
                break;
            }

            destination = tileGetTileInDirection(a1->tile, (rotation + 1) % ROTATION_COUNT, actionPointsLeft);
            if (make_path(a1, a1->tile, destination, NULL, 1) > 0) {
                break;
            }

            destination = tileGetTileInDirection(a1->tile, (rotation + 5) % ROTATION_COUNT, actionPointsLeft);
            if (make_path(a1, a1->tile, destination, NULL, 1) > 0) {
                break;
            }
        }

        if (actionPoints > 0) {
            register_begin(ANIMATION_REQUEST_RESERVED);
            register_object_move_to_tile(a1, destination, a1->elevation, actionPoints, 0);
            if (register_end() == 0) {
                combat_turn_run();
            }
        }
    }

    return 0;
}

// 0x428AC4
static bool ai_find_friend(Object* a1, int a2, int a3)
{
    Object* v1 = ai_find_nearest_team(a1, a1, 1);
    if (v1 == NULL) {
        return false;
    }

    int distance = objectGetDistanceBetween(a1, v1);
    if (distance > a2) {
        return false;
    }

    if (a3 > distance) {
        int v2 = objectGetDistanceBetween(a1, v1) - a3;
        ai_move_steps_closer(a1, v1, v2, 0);
    }

    return true;
}

// Compare objects by distance to origin.
//
// 0x428B1C
static int compare_nearer(const void* a1, const void* a2)
{
    Object* v1 = *(Object**)a1;
    Object* v2 = *(Object**)a2;

    if (v1 == NULL) {
        if (v2 == NULL) {
            return 0;
        }
        return 1;
    } else {
        if (v2 == NULL) {
            return -1;
        }
    }

    int distance1 = objectGetDistanceBetween(v1, combat_obj);
    int distance2 = objectGetDistanceBetween(v2, combat_obj);

    if (distance1 < distance2) {
        return -1;
    } else if (distance1 > distance2) {
        return 1;
    } else {
        return 0;
    }
}

// NOTE: Inlined.
//
// 0x428B74
static void ai_sort_list_distance(Object** critterList, int length, Object* origin)
{
    combat_obj = origin;
    qsort(critterList, length, sizeof(*critterList), compare_nearer);
}

// qsort compare function - melee then ranged.
//
// 0x428B8C
int compare_strength(const void* p1, const void* p2)
{
    Object* a1 = *(Object**)p1;
    Object* a2 = *(Object**)p2;

    if (a1 == NULL) {
        if (a2 == NULL) {
            return 0;
        }

        return 1;
    }

    if (a2 == NULL) {
        return -1;
    }

    int v3 = combatai_rating(a1);
    int v5 = combatai_rating(a2);

    if (v3 < v5) {
        return -1;
    }

    if (v3 > v5) {
        return 1;
    }

    return 0;
}

// NOTE: Inlined.
//
// 0x428BD0
static void ai_sort_list_strength(Object** critterList, int length)
{
    qsort(critterList, length, sizeof(*critterList), compare_strength);
}

// qsort compare unction - ranged then melee
//
// 0x428BE4
int compare_weakness(const void* p1, const void* p2)
{
    Object* a1 = *(Object**)p1;
    Object* a2 = *(Object**)p2;

    if (a1 == NULL) {
        if (a2 == NULL) {
            return 0;
        }

        return 1;
    }

    if (a2 == NULL) {
        return -1;
    }

    int v3 = combatai_rating(a1);
    int v5 = combatai_rating(a2);

    if (v3 < v5) {
        return 1;
    }

    if (v3 > v5) {
        return -1;
    }

    return 0;
}

// NOTE: Inlined.
//
// 0x428C28
static void ai_sort_list_weakness(Object** critterList, int length)
{
    qsort(critterList, length, sizeof(*critterList), compare_weakness);
}

// 0x428C3C
static Object* ai_find_nearest_team(Object* a1, Object* a2, int a3)
{
    int i;
    Object* obj;

    if (a2 == NULL) {
        return NULL;
    }

    if (curr_crit_num == 0) {
        return NULL;
    }

    // NOTE: Uninline.
    ai_sort_list_distance(curr_crit_list, curr_crit_num, a1);

    for (i = 0; i < curr_crit_num; i++) {
        obj = curr_crit_list[i];
        if (a1 != obj && !(obj->data.critter.combat.results & 0x80) && (((a3 & 0x02) && a2->data.critter.combat.team != obj->data.critter.combat.team) || ((a3 & 0x01) && a2->data.critter.combat.team == obj->data.critter.combat.team))) {
            return obj;
        }
    }

    return NULL;
}

// 0x428CF4
static Object* ai_find_nearest_team_in_combat(Object* a1, Object* a2, int a3)
{
    if (a2 == NULL) {
        return NULL;
    }

    if (curr_crit_num == 0) {
        return NULL;
    }

    int team = a2->data.critter.combat.team;

    // NOTE: Uninline.
    ai_sort_list_distance(curr_crit_list, curr_crit_num, a1);

    for (int index = 0; index < curr_crit_num; index++) {
        Object* obj = curr_crit_list[index];
        if (obj != a1
            && (obj->data.critter.combat.results & DAM_DEAD) == 0
            && (((a3 & 0x02) != 0 && team != obj->data.critter.combat.team)
                || ((a3 & 0x01) != 0 && team == obj->data.critter.combat.team))) {
            if (obj->data.critter.combat.whoHitMe != NULL) {
                return obj;
            }
        }
    }

    return NULL;
}

// 0x428DB0
static int ai_find_attackers(Object* a1, Object** a2, Object** a3, Object** a4)
{
    if (a2 != NULL) {
        *a2 = NULL;
    }

    if (a3 != NULL) {
        *a3 = NULL;
    }

    if (*a4 != NULL) {
        *a4 = NULL;
    }

    if (curr_crit_num == 0) {
        return 0;
    }

    // NOTE: Uninline.
    ai_sort_list_distance(curr_crit_list, curr_crit_num, a1);

    int foundTargetCount = 0;
    int team = a1->data.critter.combat.team;

    for (int index = 0; foundTargetCount < 3 && index < curr_crit_num; index++) {
        Object* candidate = curr_crit_list[index];
        if (candidate != a1) {
            if (a2 != NULL && *a2 == NULL) {
                if ((candidate->data.critter.combat.results & DAM_DEAD) == 0
                    && candidate->data.critter.combat.whoHitMe == a1) {
                    foundTargetCount++;
                    *a2 = candidate;
                }
            }

            if (a3 != NULL && *a3 == NULL) {
                if (team == candidate->data.critter.combat.team) {
                    Object* whoHitCandidate = candidate->data.critter.combat.whoHitMe;
                    if (whoHitCandidate != NULL
                        && whoHitCandidate != a1
                        && team != whoHitCandidate->data.critter.combat.team
                        && (whoHitCandidate->data.critter.combat.results & DAM_DEAD) == 0) {
                        foundTargetCount++;
                        *a3 = whoHitCandidate;
                    }
                }
            }

            if (a4 != NULL && *a4 == NULL) {
                if (candidate->data.critter.combat.team != team
                    && (candidate->data.critter.combat.results & DAM_DEAD) == 0) {
                    Object* whoHitCandidate = candidate->data.critter.combat.whoHitMe;
                    if (whoHitCandidate != NULL
                        && whoHitCandidate->data.critter.combat.team == team) {
                        foundTargetCount++;
                        *a4 = candidate;
                    }
                }
            }
        }
    }

    return 0;
}

// ai_danger_source
// 0x428F4C
Object* ai_danger_source(Object* a1)
{
    if (a1 == NULL) {
        return NULL;
    }

    bool v2 = false;
    int attackWho;

    Object* targets[4];
    targets[0] = NULL;

    if (objectIsPartyMember(a1)) {
        int disposition = a1 != NULL ? ai_cap(a1)->disposition : 0;

        switch (disposition + 1) {
        case 1:
        case 2:
        case 3:
        case 4:
            v2 = true;
            break;
        case 0:
        case 5:
            v2 = false;
            break;
        }

        if (v2 && ai_cap(a1)->distance == 1) {
            v2 = false;
        }

        attackWho = ai_cap(a1)->attack_who;
        switch (attackWho) {
        case ATTACK_WHO_WHOMEVER_ATTACKING_ME: {
            Object* candidate = combatAIInfoGetLastTarget(gDude);
            if (candidate == NULL || a1->data.critter.combat.team == candidate->data.critter.combat.team) {
                break;
            }

            if (make_path_func(a1, a1->tile, gDude->data.critter.combat.whoHitMe->tile, NULL, 0, _obj_blocking_at) == 0
                && combat_check_bad_shot(a1, candidate, HIT_MODE_RIGHT_WEAPON_PRIMARY, false) != COMBAT_BAD_SHOT_OK) {
                debugPrint("\nai_danger_source: %s couldn't attack at target!  Picking alternate!", critter_name(a1));
                break;
            }

            if (v2 && critter_is_fleeing(a1)) {
                break;
            }

            return candidate;
        }
        case ATTACK_WHO_STRONGEST:
        case ATTACK_WHO_WEAKEST:
        case ATTACK_WHO_CLOSEST:
            a1->data.critter.combat.whoHitMe = NULL;
            break;
        default:
            break;
        }
    } else {
        attackWho = -1;
    }

    Object* whoHitMe = a1->data.critter.combat.whoHitMe;
    if (whoHitMe == NULL || a1 == whoHitMe) {
        targets[0] = NULL;
    } else {
        if ((whoHitMe->data.critter.combat.results & DAM_DEAD) == 0) {
            if (attackWho == ATTACK_WHO_WHOMEVER || attackWho == -1) {
                return whoHitMe;
            }
        } else {
            if (whoHitMe->data.critter.combat.team != a1->data.critter.combat.team) {
                targets[0] = ai_find_nearest_team(a1, whoHitMe, 1);
            } else {
                targets[0] = NULL;
            }
        }
    }

    ai_find_attackers(a1, &(targets[1]), &(targets[2]), &(targets[3]));

    if (v2) {
        for (int index = 0; index < 4; index++) {
            if (targets[index] != NULL && critter_is_fleeing(targets[index])) {
                targets[index] = NULL;
            }
        }
    }

    switch (attackWho) {
    case ATTACK_WHO_STRONGEST:
        // NOTE: Uninline.
        ai_sort_list_strength(targets, 4);
        break;
    case ATTACK_WHO_WEAKEST:
        // NOTE: Uninline.
        ai_sort_list_weakness(targets, 4);
        break;
    default:
        // NOTE: Uninline.
        ai_sort_list_distance(targets, 4, a1);
        break;
    }

    for (int index = 0; index < 4; index++) {
        Object* candidate = targets[index];
        if (candidate != NULL && is_within_perception(a1, candidate)) {
            if (make_path_func(a1, a1->tile, candidate->tile, NULL, 0, _obj_blocking_at) != 0
                || combat_check_bad_shot(a1, candidate, HIT_MODE_RIGHT_WEAPON_PRIMARY, false) == COMBAT_BAD_SHOT_OK) {
                return candidate;
            }
            debugPrint("\nai_danger_source: I couldn't get at my target!  Picking alternate!");
        }
    }

    return NULL;
}

// 0x4291C4
int caiSetupTeamCombat(Object* a1, Object* a2)
{
    Object* obj;

    obj = objectFindFirstAtElevation(a1->elevation);
    while (obj != NULL) {
        if (PID_TYPE(obj->pid) == OBJ_TYPE_CRITTER && obj != gDude) {
            obj->data.critter.combat.maneuver |= CRITTER_MANEUVER_0x01;
        }
        obj = objectFindNextAtElevation();
    }

    attackerTeamObj = a1;
    targetTeamObj = a2;

    return 0;
}

// 0x_429210
int caiTeamCombatInit(Object** a1, int a2)
{
    int v9;
    int v10;
    int i;
    Object* v8;

    if (a1 == NULL) {
        return -1;
    }

    if (a2 == 0) {
        return 0;
    }

    if (attackerTeamObj == NULL) {
        return 0;
    }

    v9 = attackerTeamObj->data.critter.combat.team;
    v10 = targetTeamObj->data.critter.combat.team;

    for (i = 0; i < a2; i++) {
        if (a1[i]->data.critter.combat.team == v9) {
            v8 = targetTeamObj;
        } else if (a1[i]->data.critter.combat.team == v10) {
            v8 = attackerTeamObj;
        } else {
            continue;
        }

        a1[i]->data.critter.combat.whoHitMe = ai_find_nearest_team(a1[i], v8, 1);
    }

    attackerTeamObj = NULL;
    targetTeamObj = NULL;

    return 0;
}

// 0x4292C0
void caiTeamCombatExit()
{
    targetTeamObj = 0;
    attackerTeamObj = 0;
}

// 0x4292D4
static int ai_have_ammo(Object* critter_obj, Object* weapon_obj, Object** out_ammo_obj)
{
    int v9;
    Object* ammo_obj;

    if (out_ammo_obj) {
        *out_ammo_obj = NULL;
    }

    if (weapon_obj->pid == PROTO_ID_SOLAR_SCORCHER) {
        return lightGetLightLevel() > 62259;
    }

    v9 = -1;

    while (1) {
        ammo_obj = _inven_find_type(critter_obj, 4, &v9);
        if (ammo_obj == NULL) {
            break;
        }

        if (weaponCanBeReloadedWith(weapon_obj, ammo_obj)) {
            if (out_ammo_obj) {
                *out_ammo_obj = ammo_obj;
            }
            return 1;
        }

        if (weaponGetAnimationCode(weapon_obj)) {
            if (_item_w_range(critter_obj, 2) < 3) {
                _inven_unwield(critter_obj, 1);
            }
        } else {
            _inven_unwield(critter_obj, 1);
        }
    }

    return 0;
}

// 0x42938C
static bool caiHasWeapPrefType(AiPacket* ai, int attackType)
{
    int bestWeapon = ai->best_weapon + 1;

    for (int index = 0; index < 5; index++) {
        if (attackType == weapPrefOrderings[bestWeapon][index]) {
            return true;
        }
    }

    return false;
}

// 0x4293BC
static Object* ai_best_weapon(Object* attacker, Object* weapon1, Object* weapon2, Object* defender)
{
    if (attacker == NULL) {
        return NULL;
    }

    AiPacket* ai = ai_cap(attacker);
    if (ai->best_weapon == BEST_WEAPON_RANDOM) {
        return randomBetween(1, 100) <= 50 ? weapon1 : weapon2;
    }
    int minDamage;
    int maxDamage;

    int v24 = 0;
    int v25 = 999;
    int v26 = 999;
    int avgDamage1 = 0;

    Attack attack;
    combat_ctd_init(&attack, attacker, defender, HIT_MODE_RIGHT_WEAPON_PRIMARY, HIT_LOCATION_TORSO);

    int attackType1;
    int distance;
    int attackType2;
    int avgDamage2 = 0;

    int v23 = 0;

    // NOTE: weaponClass1 and weaponClass2 both use ESI but they are not
    // initialized. I'm not sure if this is right, but at least it doesn't
    // crash.
    attackType1 = -1;
    attackType2 = -1;

    if (weapon1 != NULL) {
        attackType1 = weaponGetAttackTypeForHitMode(weapon1, HIT_MODE_RIGHT_WEAPON_PRIMARY);
        if (weaponGetDamageMinMax(weapon1, &minDamage, &maxDamage) == -1) {
            return NULL;
        }

        avgDamage1 = (maxDamage - minDamage) / 2;
        if (_item_w_area_damage_radius(weapon1, HIT_MODE_RIGHT_WEAPON_PRIMARY) > 0 && defender != NULL) {
            attack.weapon = weapon1;
            compute_explosion_on_extras(&attack, 0, weaponIsGrenade(weapon1), 1);
            avgDamage1 *= attack.extrasLength + 1;
        }

        // TODO: Probably an error, why it takes [weapon2], should likely use
        // [weapon1].
        if (weaponGetPerk(weapon2) != -1) {
            avgDamage1 *= 5;
        }

        if (defender != NULL) {
            if (combat_safety_invalidate_weapon(attacker, weapon1, HIT_MODE_RIGHT_WEAPON_PRIMARY, defender, NULL)) {
                v24 = 1;
            }
        }

        if (weaponIsNatural(weapon1)) {
            return weapon1;
        }
    } else {
        distance = objectGetDistanceBetween(attacker, defender);
        if (_item_w_range(attacker, HIT_MODE_PUNCH) >= distance) {
            attackType1 = ATTACK_TYPE_UNARMED;
        }
    }

    if (!v24) {
        for (int index = 0; index < ATTACK_TYPE_COUNT; index++) {
            if (weapPrefOrderings[ai->best_weapon + 1][index] == attackType1) {
                v26 = index;
                break;
            }
        }
    }

    if (weapon2 != NULL) {
        attackType2 = weaponGetAttackTypeForHitMode(weapon2, HIT_MODE_RIGHT_WEAPON_PRIMARY);
        if (weaponGetDamageMinMax(weapon2, &minDamage, &maxDamage) == -1) {
            return NULL;
        }

        avgDamage2 = (maxDamage - minDamage) / 2;
        if (_item_w_area_damage_radius(weapon2, HIT_MODE_RIGHT_WEAPON_PRIMARY) > 0 && defender != NULL) {
            attack.weapon = weapon2;
            compute_explosion_on_extras(&attack, 0, weaponIsGrenade(weapon2), 1);
            avgDamage2 *= attack.extrasLength + 1;
        }

        if (weaponGetPerk(weapon2) != -1) {
            avgDamage2 *= 5;
        }

        if (defender != NULL) {
            if (combat_safety_invalidate_weapon(attacker, weapon2, HIT_MODE_RIGHT_WEAPON_PRIMARY, defender, NULL)) {
                v23 = 1;
            }
        }

        if (weaponIsNatural(weapon2)) {
            return weapon2;
        }
    } else {
        if (distance == 0) {
            distance = objectGetDistanceBetween(attacker, weapon1);
        }

        if (_item_w_range(attacker, HIT_MODE_PUNCH) >= distance) {
            attackType2 = ATTACK_TYPE_UNARMED;
        }
    }

    if (!v23) {
        for (int index = 0; index < ATTACK_TYPE_COUNT; index++) {
            if (weapPrefOrderings[ai->best_weapon + 1][index] == attackType2) {
                v25 = index;
                break;
            }
        }
    }

    if (v26 == v25) {
        if (v26 == 999) {
            return NULL;
        }

        if (abs(avgDamage2 - avgDamage1) <= 5) {
            return itemGetCost(weapon2) > itemGetCost(weapon1) ? weapon2 : weapon1;
        }

        return avgDamage2 > avgDamage1 ? weapon2 : weapon1;
    }

    if (weapon1 != NULL && weapon1->pid == PROTO_ID_FLARE && weapon2 != NULL) {
        return weapon2;
    }

    if (weapon2 != NULL && weapon2->pid == PROTO_ID_FLARE && weapon1 != NULL) {
        return weapon1;
    }

    if ((ai->best_weapon == -1 || ai->best_weapon >= BEST_WEAPON_UNARMED_OVER_THROW)
        && abs(avgDamage2 - avgDamage1) > 5) {
        return avgDamage2 > avgDamage1 ? weapon2 : weapon1;
    }

    return v26 > v25 ? weapon2 : weapon1;
}

// 0x4298EC
static bool ai_can_use_weapon(Object* critter, Object* weapon, int hitMode)
{
    int damageFlags = critter->data.critter.combat.results;
    if ((damageFlags & DAM_CRIP_ARM_LEFT) != 0 && (damageFlags & DAM_CRIP_ARM_RIGHT) != 0) {
        return false;
    }

    if ((damageFlags & DAM_CRIP_ARM_ANY) != 0 && weaponIsTwoHanded(weapon)) {
        return false;
    }

    int rotation = critter->rotation + 1;
    int animationCode = weaponGetAnimationCode(weapon);
    int v9 = weaponGetAnimationForHitMode(weapon, hitMode);
    int fid = art_id(OBJ_TYPE_CRITTER, critter->fid & 0xFFF, v9, animationCode, rotation);
    if (!art_exists(fid)) {
        return false;
    }

    int skill = weaponGetSkillForHitMode(weapon, hitMode);
    AiPacket* ai = ai_cap(critter);
    if (skillGetValue(critter, skill) < ai->min_to_hit) {
        return false;
    }

    int attackType = weaponGetAttackTypeForHitMode(weapon, HIT_MODE_RIGHT_WEAPON_PRIMARY);
    return caiHasWeapPrefType(ai, attackType) != 0;
}

// 0x4299A0
Object* ai_search_inven_weap(Object* critter, int a2, Object* a3)
{
    int bodyType = critter_body_type(critter);
    if (bodyType != BODY_TYPE_BIPED
        && bodyType != BODY_TYPE_ROBOTIC
        && critter->pid != PROTO_ID_0x1000098) {
        return NULL;
    }

    int token = -1;
    Object* bestWeapon = NULL;
    Object* rightHandWeapon = critterGetItem2(critter);
    while (true) {
        Object* weapon = _inven_find_type(critter, ITEM_TYPE_WEAPON, &token);
        if (weapon == NULL) {
            break;
        }

        if (weapon == rightHandWeapon) {
            continue;
        }

        if (a2) {
            if (weaponGetActionPointCost1(weapon) > critter->data.critter.combat.ap) {
                continue;
            }
        }

        if (!ai_can_use_weapon(critter, weapon, HIT_MODE_RIGHT_WEAPON_PRIMARY)) {
            continue;
        }

        if (weaponGetAttackTypeForHitMode(weapon, HIT_MODE_RIGHT_WEAPON_PRIMARY) == ATTACK_TYPE_RANGED) {
            if (ammoGetQuantity(weapon) == 0) {
                if (!ai_have_ammo(critter, weapon, NULL)) {
                    continue;
                }
            }
        }

        bestWeapon = ai_best_weapon(critter, bestWeapon, weapon, a3);
    }

    return bestWeapon;
}

// Finds new best armor (other than what's already equipped) based on the armor score.
//
// 0x429A6C
Object* ai_search_inven_armor(Object* critter)
{
    if (!objectIsPartyMember(critter)) {
        return NULL;
    }

    // Calculate armor score - it's a unitless combination of armor class and bonuses across
    // all damage types.
    int armorScore = 0;
    Object* armor = critterGetArmor(critter);
    if (armor != NULL) {
        armorScore = armorGetArmorClass(armor);

        for (int damageType = 0; damageType < DAMAGE_TYPE_COUNT; damageType++) {
            armorScore += armorGetDamageResistance(armor, damageType);
            armorScore += armorGetDamageThreshold(armor, damageType);
        }
    } else {
        armorScore = 0;
    }

    Object* bestArmor = NULL;

    int v15 = -1;
    while (true) {
        Object* candidate = _inven_find_type(critter, ITEM_TYPE_ARMOR, &v15);
        if (candidate == NULL) {
            break;
        }

        if (armor != candidate) {
            int candidateScore = armorGetArmorClass(candidate);
            for (int damageType = 0; damageType < DAMAGE_TYPE_COUNT; damageType++) {
                candidateScore += armorGetDamageResistance(candidate, damageType);
                candidateScore += armorGetDamageThreshold(candidate, damageType);
            }

            if (candidateScore > armorScore) {
                armorScore = candidateScore;
                bestArmor = candidate;
            }
        }
    }

    return bestArmor;
}

// Returns true if critter can use given item.
//
// That means the item is one of it's primary desires,
// or it's a humanoid being with intelligence at least 3,
// and the iteam is a something healing.
//
// 0x429B44
static bool ai_can_use_drug(Object* critter, Object* item)
{
    if (critter == NULL) {
        return false;
    }

    if (item == NULL) {
        return false;
    }

    AiPacket* ai = ai_cap_from_packet(critter->data.critter.combat.aiPacket);
    if (ai == NULL) {
        return false;
    }

    for (int index = 0; index < AI_PACKET_CHEM_PRIMARY_DESIRE_COUNT; index++) {
        if (item->pid == ai->chem_primary_desire[index]) {
            return true;
        }
    }

    if (critter_body_type(critter) != BODY_TYPE_BIPED) {
        return false;
    }

    int killType = critterGetKillType(critter);
    if (killType != KILL_TYPE_MAN
        && killType != KILL_TYPE_WOMAN
        && killType != KILL_TYPE_SUPER_MUTANT
        && killType != KILL_TYPE_GHOUL
        && killType != KILL_TYPE_CHILD) {
        return false;
    }

    if (critterGetStat(critter, STAT_INTELLIGENCE) < 3) {
        return false;
    }

    int itemPid = item->pid;
    if (itemPid != PROTO_ID_STIMPACK
        && itemPid != PROTO_ID_SUPER_STIMPACK
        && itemPid != PROTO_ID_HEALING_POWDER) {
        return false;
    }

    return true;
}

// Find best item type to use?
//
// 0x429C18
static Object* ai_search_environ(Object* critter, int itemType)
{
    if (critter_body_type(critter) != BODY_TYPE_BIPED) {
        return NULL;
    }

    Object** objects;
    int count = objectListCreate(-1, gElevation, OBJ_TYPE_ITEM, &objects);
    if (count == 0) {
        return NULL;
    }

    // NOTE: Uninline.
    ai_sort_list_distance(objects, count, critter);

    int perception = critterGetStat(critter, STAT_PERCEPTION) + 5;
    Object* item2 = critterGetItem2(critter);

    Object* foundItem = NULL;

    for (int index = 0; index < count; index++) {
        Object* item = objects[index];
        int distance = objectGetDistanceBetween(critter, item);
        if (distance > perception) {
            break;
        }

        if (itemGetType(item) == itemType) {
            switch (itemType) {
            case ITEM_TYPE_WEAPON:
                if (ai_can_use_weapon(critter, item, HIT_MODE_RIGHT_WEAPON_PRIMARY)) {
                    foundItem = item;
                }
                break;
            case ITEM_TYPE_AMMO:
                if (weaponCanBeReloadedWith(item2, item)) {
                    foundItem = item;
                }
                break;
            case ITEM_TYPE_DRUG:
            case ITEM_TYPE_MISC:
                if (ai_can_use_drug(critter, item)) {
                    foundItem = item;
                }
                break;
            }

            if (foundItem != NULL) {
                break;
            }
        }
    }

    objectListFree(objects);

    return foundItem;
}

// 0x429D60
static Object* ai_retrieve_object(Object* a1, Object* a2)
{
    if (action_get_an_object(a1, a2) != 0) {
        return NULL;
    }

    combat_turn_run();

    Object* v3 = _inven_find_id(a1, a2->id);

    // TODO: Not sure about this one.
    if (v3 != NULL || a2->owner != NULL) {
        a2 = NULL;
    }

    combatAIInfoSetLastItem(v3, a2);

    return v3;
}

// 0x429DB4
static int ai_pick_hit_mode(Object* a1, Object* a2, Object* a3)
{
    if (a2 == NULL) {
        return HIT_MODE_PUNCH;
    }

    if (itemGetType(a2) != ITEM_TYPE_WEAPON) {
        return HIT_MODE_PUNCH;
    }

    int attackType = weaponGetAttackTypeForHitMode(a2, HIT_MODE_RIGHT_WEAPON_SECONDARY);
    int intelligence = critterGetStat(a1, STAT_INTELLIGENCE);
    if (attackType == ATTACK_TYPE_NONE || !ai_can_use_weapon(a1, a2, HIT_MODE_RIGHT_WEAPON_SECONDARY)) {
        return HIT_MODE_RIGHT_WEAPON_PRIMARY;
    }

    bool useSecondaryMode = false;

    AiPacket* ai = ai_cap(a1);
    if (ai == NULL) {
        return HIT_MODE_PUNCH;
    }

    if (ai->area_attack_mode != -1) {
        switch (ai->area_attack_mode) {
        case AREA_ATTACK_MODE_ALWAYS:
            useSecondaryMode = true;
            break;
        case AREA_ATTACK_MODE_SOMETIMES:
            if (randomBetween(1, ai->secondary_freq) == 1) {
                useSecondaryMode = true;
            }
            break;
        case AREA_ATTACK_MODE_BE_SURE:
            if (determine_to_hit(a1, a3, HIT_LOCATION_TORSO, HIT_MODE_RIGHT_WEAPON_SECONDARY) >= 85
                && !combat_safety_invalidate_weapon(a1, a2, 3, a3, 0)) {
                useSecondaryMode = true;
            }
            break;
        case AREA_ATTACK_MODE_BE_CAREFUL:
            if (determine_to_hit(a1, a3, HIT_LOCATION_TORSO, HIT_MODE_RIGHT_WEAPON_SECONDARY) >= 50
                && !combat_safety_invalidate_weapon(a1, a2, 3, a3, 0)) {
                useSecondaryMode = true;
            }
            break;
        case AREA_ATTACK_MODE_BE_ABSOLUTELY_SURE:
            if (determine_to_hit(a1, a3, HIT_LOCATION_TORSO, HIT_MODE_RIGHT_WEAPON_SECONDARY) >= 95
                && !combat_safety_invalidate_weapon(a1, a2, 3, a3, 0)) {
                useSecondaryMode = true;
            }
            break;
        }
    } else {
        if (intelligence < 6 || objectGetDistanceBetween(a1, a3) < 10) {
            if (randomBetween(1, ai->secondary_freq) == 1) {
                useSecondaryMode = true;
            }
        }
    }

    if (useSecondaryMode) {
        if (!caiHasWeapPrefType(ai, attackType)) {
            useSecondaryMode = false;
        }
    }

    if (useSecondaryMode) {
        if (attackType != ATTACK_TYPE_THROW
            || ai_search_inven_weap(a1, 0, a3) != NULL
            || statRoll(a1, STAT_INTELLIGENCE, 0, NULL) <= 1) {
            return HIT_MODE_RIGHT_WEAPON_SECONDARY;
        }
    }

    return HIT_MODE_RIGHT_WEAPON_PRIMARY;
}

// 0x429FC8
static int ai_move_steps_closer(Object* a1, Object* a2, int actionPoints, int a4)
{
    if (actionPoints <= 0) {
        return -1;
    }

    int distance = ai_cap(a1)->distance;
    if (distance == DISTANCE_STAY) {
        return -1;
    }

    if (distance == DISTANCE_STAY_CLOSE) {
        if (a2 != gDude) {
            int v10 = objectGetDistanceBetween(a1, gDude);
            if (v10 > 5 && objectGetDistanceBetween(a2, gDude) > 5 && v10 + actionPoints > 5) {
                return -1;
            }
        }
    }

    if (objectGetDistanceBetween(a1, a2) <= 1) {
        return -1;
    }

    register_begin(ANIMATION_REQUEST_RESERVED);

    if (a4) {
        combatai_msg(a1, NULL, AI_MESSAGE_TYPE_MOVE, 0);
    }

    Object* v18 = a2;

    bool shouldUnhide;
    if ((a2->flags & 0x800) != 0) {
        shouldUnhide = true;
        a2->flags |= OBJECT_HIDDEN;
    } else {
        shouldUnhide = false;
    }

    if (make_path_func(a1, a1->tile, a2->tile, NULL, 0, _obj_blocking_at) == 0) {
        _moveBlockObj = NULL;
        if (make_path_func(a1, a1->tile, a2->tile, NULL, 0, _obj_ai_blocking_at) == 0
            && _moveBlockObj != NULL
            && PID_TYPE(_moveBlockObj->pid) == OBJ_TYPE_CRITTER) {
            if (shouldUnhide) {
                a2->flags &= ~OBJECT_HIDDEN;
            }

            a2 = _moveBlockObj;
            if ((a2->flags & 0x800) != 0) {
                shouldUnhide = true;
                a2->flags |= OBJECT_HIDDEN;
            } else {
                shouldUnhide = false;
            }
        }
    }

    if (shouldUnhide) {
        a2->flags &= ~OBJECT_HIDDEN;
    }

    int tile = a2->tile;
    if (a2 == v18) {
        cai_retargetTileFromFriendlyFire(a1, a2, &tile);
    }

    if (actionPoints >= critterGetStat(a1, STAT_MAXIMUM_ACTION_POINTS) / 2 && artCritterFidShouldRun(a1->fid)) {
        if ((a2->flags & OBJECT_MULTIHEX) != 0) {
            register_object_run_to_object(a1, a2, actionPoints, 0);
        } else {
            register_object_run_to_tile(a1, tile, a1->elevation, actionPoints, 0);
        }
    } else {
        if ((a2->flags & OBJECT_MULTIHEX) != 0) {
            register_object_move_to_object(a1, a2, actionPoints, 0);
        } else {
            register_object_move_to_tile(a1, tile, a1->elevation, actionPoints, 0);
        }
    }

    if (register_end() != 0) {
        return -1;
    }

    combat_turn_run();

    return 0;
}

// NOTE: Inlined.
//
// 0x42A1C0
static int ai_move_closer(Object* a1, Object* a2, int a3)
{
    return ai_move_steps_closer(a1, a2, a1->data.critter.combat.ap, a3);
}

// 0x42A1D4
static int cai_retargetTileFromFriendlyFire(Object* source, Object* target, int* tilePtr)
{
    if (source == NULL) {
        return -1;
    }

    if (target == NULL) {
        return -1;
    }

    if (tilePtr == NULL) {
        return -1;
    }

    if (*tilePtr == -1) {
        return -1;
    }

    if (curr_crit_num == 0) {
        return -1;
    }

    int tiles[32];

    AiRetargetData aiRetargetData;
    aiRetargetData.source = source;
    aiRetargetData.target = target;
    aiRetargetData.sourceTeam = source->data.critter.combat.team;
    aiRetargetData.sourceRating = combatai_rating(source);
    aiRetargetData.critterCount = 0;
    aiRetargetData.tiles = tiles;
    aiRetargetData.notSameTile = *tilePtr != source->tile;
    aiRetargetData.currentTileIndex = 0;
    aiRetargetData.sourceIntelligence = critterGetStat(source, STAT_INTELLIGENCE);

    for (int index = 0; index < 32; index++) {
        tiles[index] = -1;
    }

    for (int index = 0; index < curr_crit_num; index++) {
        Object* obj = curr_crit_list[index];
        if ((obj->data.critter.combat.results & DAM_DEAD) == 0
            && obj->data.critter.combat.team == aiRetargetData.sourceTeam
            && combatAIInfoGetLastTarget(obj) == aiRetargetData.target
            && obj != aiRetargetData.source) {
            int rating = combatai_rating(obj);
            if (rating >= aiRetargetData.sourceRating) {
                aiRetargetData.critterList[aiRetargetData.critterCount] = obj;
                aiRetargetData.ratingList[aiRetargetData.critterCount] = rating;
                aiRetargetData.critterCount += 1;
            }
        }
    }

    // NOTE: Uninline.
    ai_sort_list_distance(aiRetargetData.critterList, aiRetargetData.critterCount, source);

    if (cai_retargetTileFromFriendlyFireSubFunc(&aiRetargetData, *tilePtr) == 0) {
        int minDistance = 99999;
        int minDistanceIndex = -1;

        for (int index = 0; index < 32; index++) {
            int tile = tiles[index];
            if (tile == -1) {
                break;
            }

            if (_obj_blocking_at(NULL, tile, source->elevation) == 0) {
                int distance = tileDistanceBetween(*tilePtr, tile);
                if (distance < minDistance) {
                    minDistance = distance;
                    minDistanceIndex = index;
                }
            }
        }

        if (minDistanceIndex != -1) {
            *tilePtr = tiles[minDistanceIndex];
        }
    }

    return 0;
}

// 0x42A410
static int cai_retargetTileFromFriendlyFireSubFunc(AiRetargetData* aiRetargetData, int tile)
{
    if (aiRetargetData->sourceIntelligence <= 0) {
        return 0;
    }

    int distance = 1;

    for (int index = 0; index < aiRetargetData->critterCount; index++) {
        Object* critter = aiRetargetData->critterList[index];
        if (cai_attackWouldIntersect(critter, aiRetargetData->target, aiRetargetData->source, tile, &distance)) {
            debugPrint("In the way!");

            aiRetargetData->tiles[aiRetargetData->currentTileIndex] = tileGetTileInDirection(tile, (critter->rotation + 1) % ROTATION_COUNT, distance);
            aiRetargetData->tiles[aiRetargetData->currentTileIndex + 1] = tileGetTileInDirection(tile, (critter->rotation + 5) % ROTATION_COUNT, distance);

            aiRetargetData->sourceIntelligence -= 2;
            aiRetargetData->currentTileIndex += 2;
            break;
        }
    }

    return 0;
}

// 0x42A518
static bool cai_attackWouldIntersect(Object* attacker, Object* defender, Object* attackerFriend, int tile, int* distance)
{
    int hitMode = HIT_MODE_RIGHT_WEAPON_PRIMARY;
    bool aiming = false;
    if (attacker == gDude) {
        interfaceGetCurrentHitMode(&hitMode, &aiming);
    }

    Object* weapon = critterGetWeaponForHitMode(attacker, hitMode);
    if (weapon == NULL) {
        return false;
    }

    if (_item_w_range(attacker, hitMode) < 1) {
        return false;
    }

    Object* object = NULL;
    make_straight_path_func(attacker, attacker->tile, defender->tile, NULL, &object, 32, _obj_shoot_blocking_at);
    if (object != attackerFriend) {
        if (!combatTestIncidentalHit(attacker, defender, attackerFriend, weapon)) {
            return false;
        }
    }

    return true;
}

// 0x42A5B8
static int ai_switch_weapons(Object* a1, int* hitMode, Object** weapon, Object* a4)
{
    *weapon = NULL;
    *hitMode = HIT_MODE_PUNCH;

    Object* bestWeapon = ai_search_inven_weap(a1, 1, a4);
    if (bestWeapon != NULL) {
        *weapon = bestWeapon;
        *hitMode = ai_pick_hit_mode(a1, bestWeapon, a4);
    } else {
        Object* v8 = ai_search_environ(a1, ITEM_TYPE_WEAPON);
        if (v8 == NULL) {
            if (_item_w_mp_cost(a1, *hitMode, 0) <= a1->data.critter.combat.ap) {
                return 0;
            }

            return -1;
        }

        Object* v9 = ai_retrieve_object(a1, v8);
        if (v9 != NULL) {
            *weapon = v9;
            *hitMode = ai_pick_hit_mode(a1, v9, a4);
        }
    }

    if (*weapon != NULL) {
        _inven_wield(a1, *weapon, 1);
        combat_turn_run();
        if (_item_w_mp_cost(a1, *hitMode, 0) <= a1->data.critter.combat.ap) {
            return 0;
        }
    }

    return -1;
}

// 0x42A670
static int ai_called_shot(Object* a1, Object* a2, int a3)
{
    AiPacket* ai;
    int v5;
    int v6;
    int v7;
    int combat_difficulty;

    v5 = 3;

    if (_item_w_mp_cost(a1, a3, 1) <= a1->data.critter.combat.ap) {
        if (_item_w_called_shot(a1, a3)) {
            ai = ai_cap(a1);
            if (randomBetween(1, ai->called_freq) == 1) {
                combat_difficulty = 1;
                config_get_value(&game_config, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_COMBAT_DIFFICULTY_KEY, &combat_difficulty);
                if (combat_difficulty) {
                    if (combat_difficulty == 2) {
                        v6 = 3;
                    } else {
                        v6 = 5;
                    }
                } else {
                    v6 = 7;
                }

                if (critterGetStat(a1, STAT_INTELLIGENCE) >= v6) {
                    v5 = randomBetween(0, 8);
                    v7 = determine_to_hit(a1, a2, a3, v5);
                    if (v7 < ai->min_to_hit) {
                        v5 = 3;
                    }
                }
            }
        }
    }

    return v5;
}

// 0x42A748
static int ai_attack(Object* a1, Object* a2, int a3)
{
    int v6;

    if (a1->data.critter.combat.maneuver & CRITTER_MANUEVER_FLEEING) {
        return -1;
    }

    register_begin(ANIMATION_REQUEST_RESERVED);
    register_object_turn_towards(a1, a2->tile);
    register_end();
    combat_turn_run();

    v6 = ai_called_shot(a1, a2, a3);
    if (combat_attack(a1, a2, a3, v6)) {
        return -1;
    }

    combat_turn_run();

    return 0;
}

// 0x42A7D8
static int ai_try_attack(Object* a1, Object* a2)
{
    critter_set_who_hit_me(a1, a2);

    CritterCombatData* combatData = &(a1->data.critter.combat);
    int v38 = 1;

    Object* weapon = critterGetItem2(a1);
    if (weapon != NULL && itemGetType(weapon) != ITEM_TYPE_WEAPON) {
        weapon = NULL;
    }

    int hitMode = ai_pick_hit_mode(a1, weapon, a2);
    int minToHit = ai_cap(a1)->min_to_hit;

    int actionPoints = a1->data.critter.combat.ap;
    int v31 = 0;
    int v42 = 0;
    if (weapon != NULL
        || (critter_body_type(a2) == BODY_TYPE_BIPED
            && ((a2->fid & 0xF000) >> 12 == 0)
            && art_exists(art_id(OBJ_TYPE_CRITTER, a1->fid & 0xFFF, ANIM_THROW_PUNCH, 0, a1->rotation + 1)))) {
        if (combat_safety_invalidate_weapon(a1, weapon, HIT_MODE_RIGHT_WEAPON_PRIMARY, a2, &v31)) {
            ai_switch_weapons(a1, &hitMode, &weapon, a2);
        }
    } else {
        ai_switch_weapons(a1, &hitMode, &weapon, a2);
    }

    unsigned char v30[800];

    Object* ammo = NULL;
    for (int attempt = 0; attempt < 10; attempt++) {
        if ((combatData->results & (DAM_KNOCKED_OUT | DAM_DEAD | DAM_LOSE_TURN)) != 0) {
            break;
        }

        int reason = combat_check_bad_shot(a1, a2, hitMode, false);
        if (reason == COMBAT_BAD_SHOT_NO_AMMO) {
            // out of ammo
            if (ai_have_ammo(a1, weapon, &ammo)) {
                int v9 = _item_w_reload(weapon, ammo);
                if (v9 == 0 && ammo != NULL) {
                    _obj_destroy(ammo);
                }

                if (v9 != -1) {
                    int volume = gsound_compute_relative_volume(a1);
                    const char* sfx = gsnd_build_weapon_sfx_name(WEAPON_SOUND_EFFECT_READY, weapon, hitMode, NULL);
                    gsound_play_sfx_file_volume(sfx, volume);
                    ai_magic_hands(a1, weapon, 5002);

                    int actionPoints = a1->data.critter.combat.ap;
                    if (actionPoints >= 2) {
                        a1->data.critter.combat.ap = actionPoints - 2;
                    } else {
                        a1->data.critter.combat.ap = 0;
                    }
                }
            } else {
                ammo = ai_search_environ(a1, ITEM_TYPE_AMMO);
                if (ammo != NULL) {
                    ammo = ai_retrieve_object(a1, ammo);
                    if (ammo != NULL) {
                        int v15 = _item_w_reload(weapon, ammo);
                        if (v15 == 0) {
                            _obj_destroy(ammo);
                        }

                        if (v15 != -1) {
                            int volume = gsound_compute_relative_volume(a1);
                            const char* sfx = gsnd_build_weapon_sfx_name(WEAPON_SOUND_EFFECT_READY, weapon, hitMode, NULL);
                            gsound_play_sfx_file_volume(sfx, volume);
                            ai_magic_hands(a1, weapon, 5002);

                            int actionPoints = a1->data.critter.combat.ap;
                            if (actionPoints >= 2) {
                                a1->data.critter.combat.ap = actionPoints - 2;
                            } else {
                                a1->data.critter.combat.ap = 0;
                            }
                        }
                    }
                } else {
                    int volume = gsound_compute_relative_volume(a1);
                    const char* sfx = gsnd_build_weapon_sfx_name(WEAPON_SOUND_EFFECT_OUT_OF_AMMO, weapon, hitMode, NULL);
                    gsound_play_sfx_file_volume(sfx, volume);
                    ai_magic_hands(a1, weapon, 5001);

                    if (_inven_unwield(a1, 1) == 0) {
                        combat_turn_run();
                    }

                    ai_switch_weapons(a1, &hitMode, &weapon, a2);
                }
            }
        } else if (reason == COMBAT_BAD_SHOT_NOT_ENOUGH_AP || reason == COMBAT_BAD_SHOT_ARM_CRIPPLED || reason == COMBAT_BAD_SHOT_BOTH_ARMS_CRIPPLED) {
            // 3 - not enough action points
            // 6 - crippled one arm for two-handed weapon
            // 7 - both hands crippled
            if (ai_switch_weapons(a1, &hitMode, &weapon, a2) == -1) {
                return -1;
            }
        } else if (reason == COMBAT_BAD_SHOT_OUT_OF_RANGE) {
            // target out of range
            int accuracy = determine_to_hit_no_range(a1, a2, HIT_LOCATION_UNCALLED, hitMode, v30);
            if (accuracy < minToHit) {
                const char* name = critter_name(a1);
                debugPrint("%s: FLEEING: Can't possibly Hit Target!", name);
                ai_run_away(a1, a2);
                return 0;
            }

            if (weapon != NULL) {
                if (ai_move_steps_closer(a1, a2, actionPoints, v38) == -1) {
                    return -1;
                }
                v38 = 0;
            } else {
                if (ai_switch_weapons(a1, &hitMode, &weapon, a2) == -1 || weapon == NULL) {
                    // NOTE: Uninline.
                    if (ai_move_closer(a1, a2, v38) == -1) {
                        return -1;
                    }
                }
                v38 = 0;
            }
        } else if (reason == COMBAT_BAD_SHOT_AIM_BLOCKED) {
            // aim is blocked
            if (ai_move_steps_closer(a1, a2, a1->data.critter.combat.ap, v38) == -1) {
                return -1;
            }
            v38 = 0;
        } else if (reason == COMBAT_BAD_SHOT_OK) {
            int accuracy = determine_to_hit(a1, a2, HIT_LOCATION_UNCALLED, hitMode);
            if (v31) {
                if (ai_move_away(a1, a2, v31) == -1) {
                    return -1;
                }
            }

            if (accuracy < minToHit) {
                int v22 = determine_to_hit_no_range(a1, a2, HIT_LOCATION_UNCALLED, hitMode, v30);
                if (v22 < minToHit) {
                    const char* name = critter_name(a1);
                    debugPrint("%s: FLEEING: Can't possibly Hit Target!", name);
                    ai_run_away(a1, a2);
                    return 0;
                }

                if (actionPoints > 0) {
                    int v24 = make_path_func(a1, a1->tile, a2->tile, v30, 0, _obj_blocking_at);
                    if (v24 == 0) {
                        v42 = actionPoints;
                    } else {
                        if (v24 < actionPoints) {
                            actionPoints = v24;
                        }

                        int tile = a1->tile;
                        int index;
                        for (index = 0; index < actionPoints; index++) {
                            tile = tileGetTileInDirection(tile, v30[index], 1);

                            v42++;

                            int v27 = determine_to_hit_from_tile(a1, tile, a2, HIT_LOCATION_UNCALLED, hitMode);
                            if (v27 >= minToHit) {
                                break;
                            }
                        }

                        if (index == actionPoints) {
                            v42 = actionPoints;
                        }
                    }
                }

                if (ai_move_steps_closer(a1, a2, v42, v38) == -1) {
                    const char* name = critter_name(a1);
                    debugPrint("%s: FLEEING: Can't possibly get closer to Target!", name);
                    ai_run_away(a1, a2);
                    return 0;
                }

                v38 = 0;
                if (ai_attack(a1, a2, hitMode) == -1 || _item_w_mp_cost(a1, hitMode, 0) > a1->data.critter.combat.ap) {
                    return -1;
                }
            } else {
                if (ai_attack(a1, a2, hitMode) == -1 || _item_w_mp_cost(a1, hitMode, 0) > a1->data.critter.combat.ap) {
                    return -1;
                }
            }
        }
    }

    return -1;
}

// Something with using flare
//
// 0x42AE90
int cAIPrepWeaponItem(Object* critter, Object* item)
{
    if (item != NULL && critterGetStat(critter, STAT_INTELLIGENCE) >= 3 && item->pid == PROTO_ID_FLARE && lightGetLightLevel() < 55705) {
        _protinst_use_item(critter, item);
    }
    return 0;
}

// 0x42AECC
void cai_attempt_w_reload(Object* critter_obj, int a2)
{
    Object* weapon_obj;
    Object* ammo_obj;
    int v5;
    int v9;
    const char* sfx;
    int v10;

    weapon_obj = critterGetItem2(critter_obj);
    if (weapon_obj == NULL) {
        return;
    }

    v5 = ammoGetQuantity(weapon_obj);
    if (v5 < ammoGetCapacity(weapon_obj) && ai_have_ammo(critter_obj, weapon_obj, &ammo_obj)) {
        v9 = _item_w_reload(weapon_obj, ammo_obj);
        if (v9 == 0) {
            _obj_destroy(ammo_obj);
        }

        if (v9 != -1 && objectIsPartyMember(critter_obj)) {
            v10 = gsound_compute_relative_volume(critter_obj);
            sfx = gsnd_build_weapon_sfx_name(WEAPON_SOUND_EFFECT_READY, weapon_obj, HIT_MODE_RIGHT_WEAPON_PRIMARY, NULL);
            gsound_play_sfx_file_volume(sfx, v10);
            if (a2) {
                ai_magic_hands(critter_obj, weapon_obj, 5002);
            }
        }
    }
}

// 0x42AF78
void combat_ai_begin(int a1, void* a2)
{
    curr_crit_num = a1;

    if (a1 != 0) {
        curr_crit_list = (Object**)internal_malloc(sizeof(Object*) * a1);
        if (curr_crit_list) {
            memcpy(curr_crit_list, a2, sizeof(Object*) * a1);
        } else {
            curr_crit_num = 0;
        }
    }
}

// 0x42AFBC
void combat_ai_over()
{
    if (curr_crit_num) {
        internal_free(curr_crit_list);
    }

    curr_crit_num = 0;
}

// 0x42AFDC
static int cai_perform_distance_prefs(Object* a1, Object* a2)
{
    if (a1->data.critter.combat.ap <= 0) {
        return -1;
    }

    int distance = ai_cap(a1)->distance;

    if (a2 != NULL) {
        if ((a2->data.critter.combat.ap & DAM_DEAD) != 0) {
            a2 = NULL;
        }
    }

    switch (distance) {
    case DISTANCE_STAY_CLOSE:
        if (a1->data.critter.combat.whoHitMe != gDude) {
            int distance = objectGetDistanceBetween(a1, gDude);
            if (distance > 5) {
                ai_move_steps_closer(a1, gDude, distance - 5, 0);
            }
        }
        break;
    case DISTANCE_CHARGE:
        if (a2 != NULL) {
            // NOTE: Uninline.
            ai_move_closer(a1, a2, 1);
        }
        break;
    case DISTANCE_SNIPE:
        if (a2 != NULL) {
            if (objectGetDistanceBetween(a1, a2) < 10) {
                // NOTE: some odd code omitted
                ai_move_away(a1, a2, 10);
            }
        }
        break;
    }

    int tile = a1->tile;
    if (cai_retargetTileFromFriendlyFire(a1, a2, &tile) == 0 && tile != a1->tile) {
        register_begin(ANIMATION_REQUEST_RESERVED);
        register_object_move_to_tile(a1, tile, a1->elevation, a1->data.critter.combat.ap, 0);
        if (register_end() != 0) {
            return -1;
        }
        combat_turn_run();
    }

    return 0;
}

// 0x42B100
static int cai_get_min_hp(AiPacket* ai)
{
    if (ai == NULL) {
        return 0;
    }

    int run_away_mode = ai->run_away_mode;
    if (run_away_mode >= 0 && run_away_mode < RUN_AWAY_MODE_COUNT) {
        return runModeValues[run_away_mode];
    } else if (run_away_mode == -1) {
        return ai->min_hp;
    }

    return 0;
}

// 0x42B130
void combat_ai(Object* a1, Object* a2)
{
    // 0x51820C
    static int aiPartyMemberDistances[DISTANCE_COUNT] = {
        5,
        7,
        7,
        7,
        50000,
    };

    AiPacket* ai = ai_cap(a1);
    int hpRatio = cai_get_min_hp(ai);
    if (ai->run_away_mode != -1) {
        int v7 = critterGetStat(a1, STAT_MAXIMUM_HIT_POINTS) * hpRatio / 100;
        int minimumHitPoints = critterGetStat(a1, STAT_MAXIMUM_HIT_POINTS) - v7;
        int currentHitPoints = critterGetStat(a1, STAT_CURRENT_HIT_POINTS);
        const char* name = critter_name(a1);
        debugPrint("\n%s minHp = %d; curHp = %d", name, minimumHitPoints, currentHitPoints);
    }

    CritterCombatData* combatData = &(a1->data.critter.combat);
    if ((combatData->maneuver & CRITTER_MANUEVER_FLEEING) != 0
        || (combatData->results & ai->hurt_too_much) != 0
        || critterGetStat(a1, STAT_CURRENT_HIT_POINTS) < ai->min_hp) {
        const char* name = critter_name(a1);
        debugPrint("%s: FLEEING: I'm Hurt!", name);
        ai_run_away(a1, a2);
        return;
    }

    if (ai_check_drugs(a1)) {
        const char* name = critter_name(a1);
        debugPrint("%s: FLEEING: I need DRUGS!", name);
        ai_run_away(a1, a2);
    } else {
        if (a2 == NULL) {
            a2 = ai_danger_source(a1);
        }

        cai_perform_distance_prefs(a1, a2);

        if (a2 != NULL) {
            ai_try_attack(a1, a2);
        }
    }

    if (a2 != NULL
        && (a1->data.critter.combat.results & DAM_DEAD) == 0
        && a1->data.critter.combat.ap != 0
        && objectGetDistanceBetween(a1, a2) > ai->max_dist) {
        Object* v13 = combatAIInfoGetFriendlyDead(a1);
        if (v13 != NULL) {
            ai_move_away(a1, v13, 10);
            combatAIInfoSetFriendlyDead(a1, NULL);
        } else {
            int perception = critterGetStat(a1, STAT_PERCEPTION);
            if (!ai_find_friend(a1, perception * 2, 5)) {
                combatData->maneuver |= CRITTER_MANEUVER_STOP_ATTACKING;
            }
        }
    }

    if (a2 == NULL && !objectIsPartyMember(a1)) {
        Object* whoHitMe = combatData->whoHitMe;
        if (whoHitMe != NULL) {
            if ((whoHitMe->data.critter.combat.results & DAM_DEAD) == 0 && combatData->damageLastTurn > 0) {
                Object* v16 = combatAIInfoGetFriendlyDead(a1);
                if (v16 != NULL) {
                    ai_move_away(a1, v16, 10);
                    combatAIInfoSetFriendlyDead(a1, NULL);
                } else {
                    const char* name = critter_name(a1);
                    debugPrint("%s: FLEEING: Somebody is shooting at me that I can't see!");
                    ai_run_away(a1, NULL);
                }
            }
        }
    }

    Object* v18 = combatAIInfoGetFriendlyDead(a1);
    if (v18 != NULL) {
        ai_move_away(a1, v18, 10);
        if (objectGetDistanceBetween(a1, v18) >= 10) {
            combatAIInfoSetFriendlyDead(a1, NULL);
        }
    }

    Object* v20;
    int v21 = 5; // 0x42B156
    if (a1->data.critter.combat.team != 0) {
        v20 = ai_find_nearest_team_in_combat(a1, a1, 1);
    } else {
        v20 = gDude;
        if (objectIsPartyMember(a1)) {
            // NOTE: Uninline
            int distance = ai_get_distance_pref_value(a1);
            if (distance != -1) {
                v21 = aiPartyMemberDistances[distance];
            }
        }
    }

    if (a2 == NULL && v20 != NULL && objectGetDistanceBetween(a1, v20) > v21) {
        int v23 = objectGetDistanceBetween(a1, v20);
        ai_move_steps_closer(a1, v20, v23 - v21, 0);
    } else {
        if (a1->data.critter.combat.ap > 0) {
            debugPrint("\n>>>NOTE: %s had extra AP's to use!<<<", critter_name(a1));
            cai_perform_distance_prefs(a1, a2);
        }
    }
}

// 0x42B3FC
bool combatai_want_to_join(Object* a1)
{
    _process_bk();

    if ((a1->flags & OBJECT_HIDDEN) != 0) {
        return false;
    }

    if (a1->elevation != gDude->elevation) {
        return false;
    }

    if ((a1->data.critter.combat.results & (DAM_DEAD | DAM_KNOCKED_OUT)) != 0) {
        return false;
    }

    if (a1->data.critter.combat.damageLastTurn > 0) {
        return true;
    }

    if (a1->sid != -1) {
        scriptSetObjects(a1->sid, NULL, NULL);
        scriptSetFixedParam(a1->sid, 5);
        scriptExecProc(a1->sid, SCRIPT_PROC_COMBAT);
    }

    if ((a1->data.critter.combat.maneuver & CRITTER_MANEUVER_0x01) != 0) {
        return true;
    }

    if ((a1->data.critter.combat.maneuver & CRITTER_MANEUVER_STOP_ATTACKING) == 0) {
        return false;
    }

    if ((a1->data.critter.combat.maneuver & CRITTER_MANUEVER_FLEEING) == 0) {
        return false;
    }

    if (ai_danger_source(a1) == NULL) {
        return false;
    }

    return true;
}

// 0x42B4A8
bool combatai_want_to_stop(Object* a1)
{
    _process_bk();

    if ((a1->data.critter.combat.maneuver & CRITTER_MANEUVER_STOP_ATTACKING) != 0) {
        return true;
    }

    if ((a1->data.critter.combat.results & (DAM_KNOCKED_OUT | DAM_DEAD)) != 0) {
        return true;
    }

    if ((a1->data.critter.combat.maneuver & CRITTER_MANUEVER_FLEEING) != 0) {
        return true;
    }

    Object* v4 = ai_danger_source(a1);
    return v4 == NULL || !is_within_perception(a1, v4);
}

// 0x42B504
int combatai_switch_team(Object* obj, int team)
{
    if (PID_TYPE(obj->pid) != OBJ_TYPE_CRITTER) {
        return 0;
    }

    obj->data.critter.combat.team = team;

    if (obj->data.critter.combat.whoHitMeCid == -1) {
        critter_set_who_hit_me(obj, NULL);
        debugPrint("\nError: CombatData found with invalid who_hit_me!");
        return -1;
    }

    Object* whoHitMe = obj->data.critter.combat.whoHitMe;
    if (whoHitMe != NULL) {
        if (whoHitMe->data.critter.combat.team == team) {
            critter_set_who_hit_me(obj, NULL);
        }
    }

    combatAIInfoSetLastTarget(obj, NULL);

    if (isInCombat()) {
        bool outlineWasEnabled = obj->outline != 0 && (obj->outline & OUTLINE_DISABLED) == 0;

        objectClearOutline(obj, NULL);

        int outlineType;
        if (obj->data.critter.combat.team == gDude->data.critter.combat.team) {
            outlineType = OUTLINE_TYPE_2;
        } else {
            outlineType = OUTLINE_TYPE_HOSTILE;
        }

        objectSetOutline(obj, outlineType, NULL);

        if (outlineWasEnabled) {
            Rect rect;
            objectEnableOutline(obj, &rect);
            tileWindowRefreshRect(&rect, obj->elevation);
        }
    }

    return 0;
}

// 0x42B5D4
int combat_ai_set_ai_packet(Object* object, int aiPacket)
{
    if (PID_TYPE(object->pid) != OBJ_TYPE_CRITTER) {
        return -1;
    }

    object->data.critter.combat.aiPacket = aiPacket;

    if (_isPotentialPartyMember(object)) {
        Proto* proto;
        if (protoGetProto(object->pid, &proto) == -1) {
            return -1;
        }

        proto->critter.aiPacket = aiPacket;
    }

    return 0;
}

// combatai_msg
// 0x42B634
int combatai_msg(Object* a1, Attack* attack, int type, int delay)
{
    if (PID_TYPE(a1->pid) != OBJ_TYPE_CRITTER) {
        return -1;
    }

    bool combatTaunts = true;
    configGetBool(&game_config, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_COMBAT_TAUNTS_KEY, &combatTaunts);
    if (!combatTaunts) {
        return -1;
    }

    if (a1 == gDude) {
        return -1;
    }

    if (a1->data.critter.combat.results & 0x81) {
        return -1;
    }

    AiPacket* ai = ai_cap(a1);

    debugPrint("%s is using %s packet with a %d%% chance to taunt\n", objectGetName(a1), ai->name, ai->chance);

    if (randomBetween(1, 100) > ai->chance) {
        return -1;
    }

    int start;
    int end;
    char* string;

    switch (type) {
    case AI_MESSAGE_TYPE_RUN:
        start = ai->run.start;
        end = ai->run.end;
        string = attack_str;
        break;
    case AI_MESSAGE_TYPE_MOVE:
        start = ai->move.start;
        end = ai->move.end;
        string = attack_str;
        break;
    case AI_MESSAGE_TYPE_ATTACK:
        start = ai->attack.start;
        end = ai->attack.end;
        string = attack_str;
        break;
    case AI_MESSAGE_TYPE_MISS:
        start = ai->miss.start;
        end = ai->miss.end;
        string = target_str;
        break;
    case AI_MESSAGE_TYPE_HIT:
        start = ai->hit[attack->defenderHitLocation].start;
        end = ai->hit[attack->defenderHitLocation].end;
        string = target_str;
        break;
    default:
        return -1;
    }

    if (end < start) {
        return -1;
    }

    MessageListItem messageListItem;
    messageListItem.num = randomBetween(start, end);
    if (!messageListGetItem(&ai_message_file, &messageListItem)) {
        debugPrint("\nERROR: combatai_msg: Couldn't find message # %d for %s", messageListItem.num, critter_name(a1));
        return -1;
    }

    debugPrint("%s said message %d\n", objectGetName(a1), messageListItem.num);
    strncpy(string, messageListItem.text, 259);

    // TODO: Get rid of casts.
    return register_object_call(a1, (void*)type, (AnimationCallback*)ai_print_msg, delay);
}

// 0x42B80C
static int ai_print_msg(Object* critter, int type)
{
    if (textObjectsGetCount() > 0) {
        return 0;
    }

    char* string;
    switch (type) {
    case AI_MESSAGE_TYPE_HIT:
    case AI_MESSAGE_TYPE_MISS:
        string = target_str;
        break;
    default:
        string = attack_str;
        break;
    }

    AiPacket* ai = ai_cap(critter);

    Rect rect;
    if (textObjectAdd(critter, string, ai->font, ai->color, ai->outline_color, &rect) == 0) {
        tileWindowRefreshRect(&rect, critter->elevation);
    }

    return 0;
}

// Returns random critter for attacking as a result of critical weapon failure.
//
// 0x42B868
Object* combat_ai_random_target(Attack* attack)
{
    // Looks like this function does nothing because it's result is not used. I
    // suppose it was planned to use range as a condition below, but it was
    // later moved into 0x426614, but remained here.
    _item_w_range(attack->attacker, attack->hitMode);

    Object* critter = NULL;

    if (curr_crit_num != 0) {
        // Randomize starting critter.
        int start = randomBetween(0, curr_crit_num - 1);
        int index = start;
        while (true) {
            Object* obj = curr_crit_list[index];
            if (obj != attack->attacker
                && obj != attack->defender
                && can_see(attack->attacker, obj)
                && combat_check_bad_shot(attack->attacker, obj, attack->hitMode, false) == COMBAT_BAD_SHOT_OK) {
                critter = obj;
                break;
            }

            index += 1;
            if (index == curr_crit_num) {
                index = 0;
            }

            if (index == start) {
                break;
            }
        }
    }

    return critter;
}

// 0x42B90C
static int combatai_rating(Object* obj)
{
    int melee_damage;
    Object* item;
    int weapon_damage_min;
    int weapon_damage_max;

    if (obj == NULL) {
        return 0;
    }

    if (FID_TYPE(obj->fid) != OBJ_TYPE_CRITTER) {
        return 0;
    }

    if ((obj->data.critter.combat.results & (DAM_DEAD | DAM_KNOCKED_OUT)) != 0) {
        return 0;
    }

    melee_damage = critterGetStat(obj, STAT_MELEE_DAMAGE);

    item = critterGetItem2(obj);
    if (item != NULL && itemGetType(item) == ITEM_TYPE_WEAPON && weaponGetDamageMinMax(item, &weapon_damage_min, &weapon_damage_max) != -1 && melee_damage < weapon_damage_max) {
        melee_damage = weapon_damage_max;
    }

    item = critterGetItem1(obj);
    if (item != NULL && itemGetType(item) == ITEM_TYPE_WEAPON && weaponGetDamageMinMax(item, &weapon_damage_min, &weapon_damage_max) != -1 && melee_damage < weapon_damage_max) {
        melee_damage = weapon_damage_max;
    }

    return melee_damage + critterGetStat(obj, STAT_ARMOR_CLASS);
}

// 0x42B9D4
int combatai_check_retaliation(Object* a1, Object* a2)
{
    Object* whoHitMe = a1->data.critter.combat.whoHitMe;
    if (whoHitMe != NULL) {
        int v3 = combatai_rating(a2);
        int result = combatai_rating(whoHitMe);
        if (v3 <= result) {
            return result;
        }
    }
    return critter_set_who_hit_me(a1, a2);
}

// 0x42BA04
bool is_within_perception(Object* a1, Object* a2)
{
    if (a2 == NULL) {
        return false;
    }

    int distance = objectGetDistanceBetween(a2, a1);
    int perception = critterGetStat(a1, STAT_PERCEPTION);
    int sneak = skillGetValue(a2, SKILL_SNEAK);
    if (can_see(a1, a2)) {
        int maxDistance = perception * 5;
        if ((a2->flags & OBJECT_TRANS_GLASS) != 0) {
            maxDistance /= 2;
        }

        if (a2 == gDude) {
            if (is_pc_sneak_working()) {
                maxDistance /= 4;
                if (sneak > 120) {
                    maxDistance -= 1;
                }
            } else if (is_pc_flag(DUDE_STATE_SNEAKING)) {
                maxDistance = maxDistance * 2 / 3;
            }
        }

        if (distance <= maxDistance) {
            return true;
        }
    }

    int maxDistance;
    if (isInCombat()) {
        maxDistance = perception * 2;
    } else {
        maxDistance = perception;
    }

    if (a2 == gDude) {
        if (is_pc_sneak_working()) {
            maxDistance /= 4;
            if (sneak > 120) {
                maxDistance -= 1;
            }
        } else if (is_pc_flag(DUDE_STATE_SNEAKING)) {
            maxDistance = maxDistance * 2 / 3;
        }
    }

    if (distance <= maxDistance) {
        return true;
    }

    return false;
}

// Load combatai.msg and apply language filter.
//
// 0x42BB34
static int combatai_load_messages()
{
    if (!messageListInit(&ai_message_file)) {
        return -1;
    }

    char path[MAX_PATH];
    sprintf(path, "%s%s", msg_path, "combatai.msg");

    if (!messageListLoad(&ai_message_file, path)) {
        return -1;
    }

    bool languageFilter;
    configGetBool(&game_config, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_LANGUAGE_FILTER_KEY, &languageFilter);

    if (languageFilter) {
        messageListFilterBadwords(&ai_message_file);
    }

    return 0;
}

// NOTE: Inlined.
//
// 0x42BBD8
static int combatai_unload_messages()
{
    if (!messageListFree(&ai_message_file)) {
        return -1;
    }

    return 0;
}

// 0x42BBF0
void combatai_refresh_messages()
{
    // 0x518220
    static int old_state = -1;

    int languageFilter = 0;
    config_get_value(&game_config, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_LANGUAGE_FILTER_KEY, &languageFilter);

    if (languageFilter != old_state) {
        old_state = languageFilter;

        if (languageFilter == 1) {
            messageListFilterBadwords(&ai_message_file);
        } else {
            // NOTE: Uninline.
            combatai_unload_messages();

            combatai_load_messages();
        }
    }
}

// 0x42BC60
void combatai_notify_onlookers(Object* a1)
{
    for (int index = 0; index < curr_crit_num; index++) {
        Object* obj = curr_crit_list[index];
        if ((obj->data.critter.combat.maneuver & CRITTER_MANEUVER_0x01) == 0) {
            if (is_within_perception(obj, a1)) {
                obj->data.critter.combat.maneuver |= CRITTER_MANEUVER_0x01;
                if ((a1->data.critter.combat.results & DAM_DEAD) != 0) {
                    if (!is_within_perception(obj, obj->data.critter.combat.whoHitMe)) {
                        debugPrint("\nSomebody Died and I don't know why!  Run!!!");
                        combatAIInfoSetFriendlyDead(obj, a1);
                    }
                }
            }
        }
    }
}

// 0x42BCD4
void combatai_notify_friends(Object* a1)
{
    int team = a1->data.critter.combat.team;

    for (int index = 0; index < curr_crit_num; index++) {
        Object* obj = curr_crit_list[index];
        if ((obj->data.critter.combat.maneuver & CRITTER_MANEUVER_0x01) == 0 && team == obj->data.critter.combat.team) {
            if (is_within_perception(obj, a1)) {
                obj->data.critter.combat.maneuver |= CRITTER_MANEUVER_0x01;
            }
        }
    }
}

// 0x42BD28
void combatai_delete_critter(Object* obj)
{
    // TODO: Check entire function.
    for (int i = 0; i < curr_crit_num; i++) {
        if (obj == curr_crit_list[i]) {
            curr_crit_num--;
            curr_crit_list[i] = curr_crit_list[curr_crit_num];
            curr_crit_list[curr_crit_num] = obj;
            break;
        }
    }
}
