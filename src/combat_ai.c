#include "combat_ai.h"

#include "actions.h"
#include "animation.h"
#include "combat.h"
#include "config.h"
#include "core.h"
#include "critter.h"
#include "debug.h"
#include "display_monitor.h"
#include "game.h"
#include "game_config.h"
#include "game_sound.h"
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

#include <intrin.h>
#include <stdio.h>

// 0x51805C
Object* off_51805C = NULL;

// 0x518060
int gAiPacketsLength = 0;

// 0x518064
AiPacket* gAiPackets = NULL;

// 0x518068
int dword_518068 = 0;

// 0x51806C
const char* gAreaAttackModeKeys[AREA_ATTACK_MODE_COUNT] = {
    "always",
    "sometimes",
    "be_sure",
    "be_careful",
    "be_absolutely_sure",
};

// 0x5180D0
const char* gAttackWhoKeys[ATTACK_WHO_COUNT] = {
    "whomever_attacking_me",
    "strongest",
    "weakest",
    "whomever",
    "closest",
};

// 0x51809C
const char* gBestWeaponKeys[BEST_WEAPON_COUNT] = {
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
const char* gChemUseKeys[CHEM_USE_COUNT] = {
    "clean",
    "stims_when_hurt_little",
    "stims_when_hurt_lots",
    "sometimes",
    "anytime",
    "always",
};

// 0x5180BC
const char* gDistanceModeKeys[DISTANCE_COUNT] = {
    "stay_close",
    "charge",
    "snipe",
    "on_your_own",
    "stay",
};

// 0x518080
const char* gRunAwayModeKeys[RUN_AWAY_MODE_COUNT] = {
    "none",
    "coward",
    "finger_hurts",
    "bleeding",
    "not_feeling_good",
    "tourniquet",
    "never",
};

// 0x5180FC
const char* gDispositionKeys[DISPOSITION_COUNT] = {
    "none",
    "custom",
    "coward",
    "defensive",
    "aggressive",
    "berserk",
};

// 0x518114
const char* gHurtTooMuchKeys[HURT_COUNT] = {
    "blind",
    "crippled",
    "crippled_legs",
    "crippled_arms",
};

// hurt_too_much
//
// 0x518124
const int dword_518124[5] = {
    DAM_BLIND,
    DAM_CRIP_LEG_LEFT | DAM_CRIP_LEG_RIGHT | DAM_CRIP_ARM_LEFT | DAM_CRIP_ARM_RIGHT,
    DAM_CRIP_LEG_LEFT | DAM_CRIP_LEG_RIGHT,
    DAM_CRIP_ARM_LEFT | DAM_CRIP_ARM_RIGHT,
    0,
};

// Hit points in percent to choose run away mode.
//
// 0x518138
const int dword_518138[6] = {
    0,
    25,
    40,
    60,
    75,
    100,
};

// 0x518150
Object* off_518150 = NULL;

// 0x518154
Object* off_518154 = NULL;

// 0x518158
const int dword_518158[BEST_WEAPON_COUNT + 1][5] = {
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

// 0x51820C
const int dword_51820C[DISTANCE_COUNT] = {
    5,
    7,
    7,
    7,
    50000,
};

// 0x518220
int gLanguageFilter = -1;

// ai.msg
//
// 0x56D510
MessageList gCombatAiMessageList;

// 0x56D518
char byte_56D518[260];

// 0x56D61C
int dword_56D61C;

// 0x56D620
Object** off_56D620;

// 0x56D624
char byte_56D624[268];

// parse hurt_too_much
void sub_426F00(char* str, int* valuePtr)
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
            if (strcmp(str, gHurtTooMuchKeys[i]) == 0) {
                *valuePtr |= dword_518124[i];
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
int sub_426FA4(const char* str, const char** list, int count, int* valuePtr)
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
void aiPacketInit(AiPacket* ai)
{
    ai->name = NULL;

    ai->area_attack_mode = -1;
    ai->run_away_mode = -1;
    ai->best_weapon = -1;
    ai->distance = -1;
    ai->attack_who = -1;
    ai->chem_use = -1;

    __stosd((unsigned long*)ai->chem_primary_desire, -1, AI_PACKET_CHEM_PRIMARY_DESIRE_COUNT);

    ai->disposition = -1;
}

// ai_init
// 0x42703C
int aiInit()
{
    int index;

    if (aiMessageListInit() == -1) {
        return -1;
    }

    gAiPacketsLength = 0;

    Config config;
    if (!configInit(&config)) {
        return -1;
    }

    if (!configRead(&config, "data\\ai.txt", true)) {
        return -1;
    }

    gAiPackets = internal_malloc(sizeof(*gAiPackets) * config.entriesLength);
    if (gAiPackets == NULL) {
        goto err;
    }

    for (index = 0; index < config.entriesLength; index++) {
        aiPacketInit(&(gAiPackets[index]));
    }

    for (index = 0; index < config.entriesLength; index++) {
        DictionaryEntry* sectionEntry = &(config.entries[index]);
        AiPacket* ai = &(gAiPackets[index]);
        char* stringValue;

        ai->name = internal_strdup(sectionEntry->key);

        if (!configGetInt(&config, sectionEntry->key, "packet_num", &(ai->packet_num))) goto err;
        if (!configGetInt(&config, sectionEntry->key, "max_dist", &(ai->max_dist))) goto err;
        if (!configGetInt(&config, sectionEntry->key, "min_to_hit", &(ai->min_to_hit))) goto err;
        if (!configGetInt(&config, sectionEntry->key, "min_hp", &(ai->min_hp))) goto err;
        if (!configGetInt(&config, sectionEntry->key, "aggression", &(ai->aggression))) goto err;

        if (configGetString(&config, sectionEntry->key, "hurt_too_much", &stringValue)) {
            sub_426F00(stringValue, &(ai->hurt_too_much));
        }

        if (!configGetInt(&config, sectionEntry->key, "secondary_freq", &(ai->secondary_freq))) goto err;
        if (!configGetInt(&config, sectionEntry->key, "called_freq", &(ai->called_freq))) goto err;
        if (!configGetInt(&config, sectionEntry->key, "font", &(ai->font))) goto err;
        if (!configGetInt(&config, sectionEntry->key, "color", &(ai->color))) goto err;
        if (!configGetInt(&config, sectionEntry->key, "outline_color", &(ai->outline_color))) goto err;
        if (!configGetInt(&config, sectionEntry->key, "chance", &(ai->chance))) goto err;
        if (!configGetInt(&config, sectionEntry->key, "run_start", &(ai->run.start))) goto err;
        if (!configGetInt(&config, sectionEntry->key, "run_end", &(ai->run.end))) goto err;
        if (!configGetInt(&config, sectionEntry->key, "move_start", &(ai->move.start))) goto err;
        if (!configGetInt(&config, sectionEntry->key, "move_end", &(ai->move.end))) goto err;
        if (!configGetInt(&config, sectionEntry->key, "attack_start", &(ai->attack.start))) goto err;
        if (!configGetInt(&config, sectionEntry->key, "attack_end", &(ai->attack.end))) goto err;
        if (!configGetInt(&config, sectionEntry->key, "miss_start", &(ai->miss.start))) goto err;
        if (!configGetInt(&config, sectionEntry->key, "miss_end", &(ai->miss.end))) goto err;
        if (!configGetInt(&config, sectionEntry->key, "hit_head_start", &(ai->hit[HIT_LOCATION_HEAD].start))) goto err;
        if (!configGetInt(&config, sectionEntry->key, "hit_head_end", &(ai->hit[HIT_LOCATION_HEAD].end))) goto err;
        if (!configGetInt(&config, sectionEntry->key, "hit_left_arm_start", &(ai->hit[HIT_LOCATION_LEFT_ARM].start))) goto err;
        if (!configGetInt(&config, sectionEntry->key, "hit_left_arm_end", &(ai->hit[HIT_LOCATION_LEFT_ARM].end))) goto err;
        if (!configGetInt(&config, sectionEntry->key, "hit_right_arm_start", &(ai->hit[HIT_LOCATION_RIGHT_ARM].start))) goto err;
        if (!configGetInt(&config, sectionEntry->key, "hit_right_arm_end", &(ai->hit[HIT_LOCATION_RIGHT_ARM].end))) goto err;
        if (!configGetInt(&config, sectionEntry->key, "hit_torso_start", &(ai->hit[HIT_LOCATION_TORSO].start))) goto err;
        if (!configGetInt(&config, sectionEntry->key, "hit_torso_end", &(ai->hit[HIT_LOCATION_TORSO].end))) goto err;
        if (!configGetInt(&config, sectionEntry->key, "hit_right_leg_start", &(ai->hit[HIT_LOCATION_RIGHT_LEG].start))) goto err;
        if (!configGetInt(&config, sectionEntry->key, "hit_right_leg_end", &(ai->hit[HIT_LOCATION_RIGHT_LEG].end))) goto err;
        if (!configGetInt(&config, sectionEntry->key, "hit_left_leg_start", &(ai->hit[HIT_LOCATION_LEFT_LEG].start))) goto err;
        if (!configGetInt(&config, sectionEntry->key, "hit_left_leg_end", &(ai->hit[HIT_LOCATION_LEFT_LEG].end))) goto err;
        if (!configGetInt(&config, sectionEntry->key, "hit_eyes_start", &(ai->hit[HIT_LOCATION_EYES].start))) goto err;
        if (!configGetInt(&config, sectionEntry->key, "hit_eyes_end", &(ai->hit[HIT_LOCATION_EYES].end))) goto err;
        if (!configGetInt(&config, sectionEntry->key, "hit_groin_start", &(ai->hit[HIT_LOCATION_GROIN].start))) goto err;
        if (!configGetInt(&config, sectionEntry->key, "hit_groin_end", &(ai->hit[HIT_LOCATION_GROIN].end))) goto err;

        ai->hit[HIT_LOCATION_GROIN].end++;

        if (configGetString(&config, sectionEntry->key, "area_attack_mode", &stringValue)) {
            sub_426FA4(stringValue, gAreaAttackModeKeys, AREA_ATTACK_MODE_COUNT, &(ai->area_attack_mode));
        } else {
            ai->run_away_mode = -1;
        }

        if (configGetString(&config, sectionEntry->key, "run_away_mode", &stringValue)) {
            sub_426FA4(stringValue, gRunAwayModeKeys, RUN_AWAY_MODE_COUNT, &(ai->run_away_mode));

            if (ai->run_away_mode >= 0) {
                ai->run_away_mode--;
            }
        }

        if (configGetString(&config, sectionEntry->key, "best_weapon", &stringValue)) {
            sub_426FA4(stringValue, gBestWeaponKeys, BEST_WEAPON_COUNT, &(ai->best_weapon));
        }

        if (configGetString(&config, sectionEntry->key, "distance", &stringValue)) {
            sub_426FA4(stringValue, gDistanceModeKeys, DISTANCE_COUNT, &(ai->distance));
        }

        if (configGetString(&config, sectionEntry->key, "attack_who", &stringValue)) {
            sub_426FA4(stringValue, gAttackWhoKeys, ATTACK_WHO_COUNT, &(ai->attack_who));
        }

        if (configGetString(&config, sectionEntry->key, "chem_use", &stringValue)) {
            sub_426FA4(stringValue, gChemUseKeys, CHEM_USE_COUNT, &(ai->chem_use));
        }

        configGetIntList(&config, sectionEntry->key, "chem_primary_desire", ai->chem_primary_desire, AI_PACKET_CHEM_PRIMARY_DESIRE_COUNT);

        if (configGetString(&config, sectionEntry->key, "disposition", &stringValue)) {
            sub_426FA4(stringValue, gDispositionKeys, DISPOSITION_COUNT, &(ai->disposition));
            ai->disposition--;
        }

        if (configGetString(&config, sectionEntry->key, "body_type", &stringValue)) {
            ai->body_type = internal_strdup(stringValue);
        } else {
            ai->body_type = NULL;
        }

        if (configGetString(&config, sectionEntry->key, "general_type", &stringValue)) {
            ai->general_type = internal_strdup(stringValue);
        } else {
            ai->general_type = NULL;
        }
    }

    if (index < config.entriesLength) {
        goto err;
    }

    gAiPacketsLength = config.entriesLength;

    configFree(&config);

    dword_518068 = 1;

    return 0;

err:

    if (gAiPackets != NULL) {
        for (index = 0; index < config.entriesLength; index++) {
            AiPacket* ai = &(gAiPackets[index]);
            if (ai->name != NULL) {
                internal_free(ai->name);
            }

            // FIXME: leaking ai->body_type and ai->general_type, does not matter
            // because it halts further processing
        }
        internal_free(gAiPackets);
    }

    debugPrint("Error processing ai.txt");

    configFree(&config);

    return -1;
}

// 0x4279F8
void aiReset()
{
}

// 0x4279FC
int aiExit()
{
    for (int index = 0; index < gAiPacketsLength; index++) {
        AiPacket* ai = &(gAiPackets[index]);

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

    internal_free(gAiPackets);
    gAiPacketsLength = 0;

    dword_518068 = 0;

    // NOTE: Uninline.
    if (aiMessageListFree() != 0) {
        return -1;
    }

    return 0;
}

// 0x427AD8
int aiLoad(File* stream)
{
    for (int index = 0; index < gPartyMemberDescriptionsLength; index++) {
        int pid = gPartyMemberPids[index];
        if (pid != -1 && (pid >> 24) == OBJ_TYPE_CRITTER) {
            Proto* proto;
            if (protoGetProto(pid, &proto) == -1) {
                return -1;
            }

            AiPacket* ai = aiGetPacketByNum(proto->critter.aiPacket);
            if (ai->disposition == 0) {
                aiPacketRead(stream, ai);
            }
        }
    }

    return 0;
}

// 0x427B50
int aiSave(File* stream)
{
    for (int index = 0; index < gPartyMemberDescriptionsLength; index++) {
        int pid = gPartyMemberPids[index];
        if (pid != -1 && (pid >> 24) == OBJ_TYPE_CRITTER) {
            Proto* proto;
            if (protoGetProto(pid, &proto) == -1) {
                return -1;
            }

            AiPacket* ai = aiGetPacketByNum(proto->critter.aiPacket);
            if (ai->disposition == 0) {
                aiPacketWrite(stream, ai);
            }
        }
    }

    return 0;
}

// 0x427BC8
int aiPacketRead(File* stream, AiPacket* ai)
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
int aiPacketWrite(File* stream, AiPacket* ai)
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

// Get ai from object
//
// 0x4280B4
AiPacket* aiGetPacket(Object* obj)
{
    // NOTE: Uninline.
    AiPacket* ai = aiGetPacketByNum(obj->data.critter.combat.aiPacket);
    return ai;
}

// get ai packet by num
//
// 0x42811C
AiPacket* aiGetPacketByNum(int aiPacketId)
{
    for (int index = 0; index < gAiPacketsLength; index++) {
        AiPacket* ai = &(gAiPackets[index]);
        if (aiPacketId == ai->packet_num) {
            return ai;
        }
    }

    debugPrint("Missing AI Packet\n");

    return gAiPackets;
}

// 0x428184
int aiGetAreaAttackMode(Object* obj)
{
    AiPacket* ai = aiGetPacket(obj);
    return ai->area_attack_mode;
}

// 0x428190
int aiGetRunAwayMode(Object* obj)
{
    AiPacket* ai;
    int v3;
    int v5;
    int v6;
    int i;

    ai = aiGetPacket(obj);
    v3 = -1;

    if (ai->run_away_mode != -1) {
        return ai->run_away_mode;
    }

    v5 = 100 * ai->min_hp;
    v6 = v5 / critterGetStat(obj, STAT_MAXIMUM_HIT_POINTS);

    for (i = 0; i < 6; i++) {
        if (v6 >= dword_518138[i]) {
            v3 = i;
        }
    }

    return v3;
}

// 0x4281FC
int aiGetBestWeapon(Object* obj)
{
    AiPacket* ai = aiGetPacket(obj);
    return ai->best_weapon;
}

// 0x428208
int aiGetDistance(Object* obj)
{
    AiPacket* ai = aiGetPacket(obj);
    return ai->distance;
}

// 0x428214
int aiGetAttackWho(Object* obj)
{
    AiPacket* ai = aiGetPacket(obj);
    return ai->attack_who;
}

// 0x428220
int aiGetChemUse(Object* obj)
{
    AiPacket* ai = aiGetPacket(obj);
    return ai->chem_use;
}

// 0x42822C
int aiSetAreaAttackMode(Object* critter, int areaAttackMode)
{
    if (areaAttackMode >= AREA_ATTACK_MODE_COUNT) {
        return -1;
    }

    AiPacket* ai = aiGetPacket(critter);
    ai->area_attack_mode = areaAttackMode;
    return 0;
}

// 0x428248
int aiSetRunAwayMode(Object* obj, int runAwayMode)
{
    if (runAwayMode >= 6) {
        return -1;
    }

    AiPacket* ai = aiGetPacket(obj);
    ai->run_away_mode = runAwayMode;

    int maximumHp = critterGetStat(obj, STAT_MAXIMUM_HIT_POINTS);
    ai->min_hp = maximumHp - maximumHp * dword_518138[runAwayMode] / 100;

    int currentHp = critterGetStat(obj, STAT_CURRENT_HIT_POINTS);
    const char* name = critterGetName(obj);

    debugPrint("\n%s minHp = %d; curHp = %d", name, ai->min_hp, currentHp);

    return 0;
}

// 0x4282D0
int aiSetBestWeapon(Object* critter, int bestWeapon)
{
    if (bestWeapon >= BEST_WEAPON_COUNT) {
        return -1;
    }

    AiPacket* ai = aiGetPacket(critter);
    ai->best_weapon = bestWeapon;
    return 0;
}

// 0x4282EC
int aiSetDistance(Object* critter, int distance)
{
    if (distance >= DISTANCE_COUNT) {
        return -1;
    }

    AiPacket* ai = aiGetPacket(critter);
    ai->distance = distance;
    return 0;
}

// 0x428308
int aiSetAttackWho(Object* critter, int attackWho)
{
    if (attackWho >= ATTACK_WHO_COUNT) {
        return -1;
    }

    AiPacket* ai = aiGetPacket(critter);
    ai->attack_who = attackWho;
    return 0;
}

// 0x428324
int aiSetChemUse(Object* critter, int chemUse)
{
    if (chemUse >= CHEM_USE_COUNT) {
        return -1;
    }

    AiPacket* ai = aiGetPacket(critter);
    ai->chem_use = chemUse;
    return 0;
}

// 0x428340
int aiGetDisposition(Object* obj)
{
    if (obj == NULL) {
        return 0;
    }

    AiPacket* ai = aiGetPacket(obj);
    return ai->disposition;
}

// 0x428354
int aiSetDisposition(Object* obj, int disposition)
{
    if (obj == NULL) {
        return -1;
    }

    if (disposition == -1 || disposition >= 5) {
        return -1;
    }

    AiPacket* ai = aiGetPacket(obj);
    obj->data.critter.combat.aiPacket = ai->packet_num - (disposition - ai->disposition);

    return 0;
}

// 0x428398
int sub_428398(Object* critter, Object* item, int num)
{
    reg_anim_begin(2);

    reg_anim_animate(critter, ANIM_MAGIC_HANDS_MIDDLE, 0);

    if (reg_anim_end() == 0) {
        if (isInCombat()) {
            sub_4227DC();
        }
    }

    if (num != -1) {
        MessageListItem messageListItem;
        messageListItem.num = num;
        if (messageListGetItem(&gMiscMessageList, &messageListItem)) {
            const char* critterName = objectGetName(critter);

            char text[200];
            if (item != NULL) {
                const char* itemName = objectGetName(item);
                sprintf(text, "%s %s %s.", critterName, messageListItem.text, itemName);
            } else {
                sprintf(text, "%s %s.", critterName, messageListItem.text);
            }

            displayMonitorAddMessage(text);
        }
    }

    return 0;
}

// ai using drugs
// 0x428480
int sub_428480(Object* critter)
{
    if (critterGetBodyType(critter) != BODY_TYPE_BIPED) {
        return 0;
    }

    int v25 = 0;
    int v28 = 0;
    int v29 = 0;
    Object* v3 = sub_42196C(critter);
    if (v3 == NULL) {
        AiPacket* ai = aiGetPacket(critter);
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
            if ((dword_510940 % 3) == 0) {
                v26 = 25;
            }
            v2 = 50;
            break;
        case 5:
            if ((dword_510940 % 3) == 0) {
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

            Object* drug = sub_472698(critter, ITEM_TYPE_DRUG, &token);
            if (drug == NULL) {
                v25 = true;
                break;
            }

            int drugPid = drug->pid;
            if ((drugPid == PROTO_ID_STIMPACK || drugPid == PROTO_ID_SUPER_STIMPACK || drugPid == PROTO_ID_HEALING_POWDER)
                && itemRemove(critter, drug, 1) == 0) {
                if (sub_479F60(critter, drug) == -1) {
                    itemAdd(critter, drug, 1);
                } else {
                    sub_428398(critter, drug, 5000);
                    sub_489EC4(drug, critter->tile, critter->elevation, NULL);
                    sub_49B9A0(drug);
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
                Object* drug = sub_472698(critter, ITEM_TYPE_DRUG, &token);
                if (drug == NULL) {
                    v25 = 1;
                    break;
                }

                int drugPid = drug->pid;
                int index;
                for (index = 0; index < AI_PACKET_CHEM_PRIMARY_DESIRE_COUNT; index++) {
                    if (ai->chem_primary_desire[index] == drugPid) {
                        break;
                    }
                }

                if (index < AI_PACKET_CHEM_PRIMARY_DESIRE_COUNT) {
                    if (drugPid != PROTO_ID_STIMPACK && drugPid != PROTO_ID_SUPER_STIMPACK && drugPid != 273
                        && itemRemove(critter, drug, 1) == 0) {
                        if (sub_479F60(critter, drug) == -1) {
                            itemAdd(critter, drug, 1);
                        } else {
                            sub_428398(critter, drug, 5000);
                            sub_489EC4(drug, critter->tile, critter->elevation, NULL);
                            sub_49B9A0(drug);
                            v28 = 1;
                            v29 += 1;
                        }

                        if (critter->data.critter.combat.ap < 2) {
                            critter->data.critter.combat.ap = 0;
                        } else {
                            critter->data.critter.combat.ap -= 2;
                        }

                        if (ai->chem_use == CHEM_USE_SOMETIMES || ai->chem_use == CHEM_USE_ANYTIME && v29 >= 2) {
                            break;
                        }
                    }
                }
            }
        }
    }

    if (v3 != NULL || !v28 && v25 == 1) {
        do {
            if (v3 == NULL) {
                v3 = sub_429C18(critter, ITEM_TYPE_DRUG);
            }

            if (v3 != NULL) {
                v3 = sub_429D60(critter, v3);
            } else {
                Object* v22 = sub_429C18(critter, ITEM_TYPE_MISC);
                if (v22 != NULL) {
                    v3 = sub_429D60(critter, v22);
                }
            }

            if (v3 != NULL && itemRemove(critter, v3, 1) == 0) {
                if (sub_479F60(critter, v3) == -1) {
                    itemAdd(critter, v3, 1);
                } else {
                    sub_428398(critter, v3, 5000);
                    sub_489EC4(v3, critter->tile, critter->elevation, NULL);
                    sub_49B9A0(v3);
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
void sub_428868(Object* a1, Object* a2)
{
    if (a2 == NULL) {
        a2 = gDude;
    }

    CritterCombatData* combatData = &(a1->data.critter.combat);

    AiPacket* ai = aiGetPacket(a1);
    int distance = objectGetDistanceBetween(a1, a2);
    if (distance < ai->max_dist) {
        combatData->maneuver |= CRITTER_MANUEVER_FLEEING;

        int rotation = tileGetRotationTo(a2->tile, a1->tile);

        int destination;
        int actionPoints = combatData->ap;
        for (; actionPoints > 0; actionPoints -= 1) {
            destination = tileGetTileInDirection(a1->tile, rotation, actionPoints);
            if (sub_415EE8(a1, a1->tile, destination, NULL, 1) > 0) {
                break;
            }

            destination = tileGetTileInDirection(a1->tile, (rotation + 1) % ROTATION_COUNT, actionPoints);
            if (sub_415EE8(a1, a1->tile, destination, NULL, 1) > 0) {
                break;
            }

            destination = tileGetTileInDirection(a1->tile, (rotation + 5) % ROTATION_COUNT, actionPoints);
            if (sub_415EE8(a1, a1->tile, destination, NULL, 1) > 0) {
                break;
            }
        }

        if (actionPoints > 0) {
            reg_anim_begin(2);
            sub_42B634(a1, NULL, AI_MESSAGE_TYPE_RUN, 0);
            reg_anim_obj_run_to_tile(a1, destination, a1->elevation, combatData->ap, 0);
            if (reg_anim_end() == 0) {
                sub_4227DC();
            }
        }
    } else {
        combatData->maneuver |= CRITTER_MANEUVER_STOP_ATTACKING;
    }
}

// 0x42899C
int sub_42899C(Object* a1, Object* a2, int a3)
{
    if (aiGetPacket(a1)->distance == DISTANCE_STAY) {
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
            if (sub_415EE8(a1, a1->tile, destination, NULL, 1) > 0) {
                break;
            }

            destination = tileGetTileInDirection(a1->tile, (rotation + 1) % ROTATION_COUNT, actionPointsLeft);
            if (sub_415EE8(a1, a1->tile, destination, NULL, 1) > 0) {
                break;
            }

            destination = tileGetTileInDirection(a1->tile, (rotation + 5) % ROTATION_COUNT, actionPointsLeft);
            if (sub_415EE8(a1, a1->tile, destination, NULL, 1) > 0) {
                break;
            }
        }

        if (actionPoints > 0) {
            reg_anim_begin(2);
            reg_anim_obj_move_to_tile(a1, destination, a1->elevation, actionPoints, 0);
            if (reg_anim_end() == 0) {
                sub_4227DC();
            }
        }
    }

    return 0;
}

// 0x428AC4
bool sub_428AC4(Object* a1, int a2, int a3)
{
    Object* v1 = sub_428C3C(a1, a1, 1);
    if (v1 == NULL) {
        return false;
    }

    int distance = objectGetDistanceBetween(a1, v1);
    if (distance > a2) {
        return false;
    }

    if (a3 > distance) {
        int v2 = objectGetDistanceBetween(a1, v1) - a3;
        sub_429FC8(a1, v1, v2, 0);
    }

    return true;
}

// Compare objects by distance to origin.
//
// 0x428B1C
int sub_428B1C(const void* a1, const void* a2)
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

    int distance1 = objectGetDistanceBetween(v1, off_51805C);
    int distance2 = objectGetDistanceBetween(v2, off_51805C);

    if (distance1 < distance2) {
        return -1;
    } else if (distance1 > distance2) {
        return 1;
    } else {
        return 0;
    }
}

// qsort compare function - melee then ranged.
//
// 0x428B8C
int sub_428B8C(const void* p1, const void* p2)
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

    int v3 = sub_42B90C(a1);
    int v5 = sub_42B90C(a2);

    if (v3 < v5) {
        return -1;
    }

    if (v3 > v5) {
        return 1;
    }

    return 0;
}

// qsort compare unction - ranged then melee
//
// 0x428BE4
int sub_428BE4(const void* p1, const void* p2)
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

    int v3 = sub_42B90C(a1);
    int v5 = sub_42B90C(a2);

    if (v3 < v5) {
        return 1;
    }

    if (v3 > v5) {
        return -1;
    }

    return 0;
}

// 0x428C3C
Object* sub_428C3C(Object* a1, Object* a2, int a3)
{
    int i;
    Object* obj;

    if (a2 == NULL) {
        return NULL;
    }

    if (dword_56D61C == 0) {
        return NULL;
    }

    off_51805C = a1;
    qsort(off_56D620, dword_56D61C, sizeof(*off_56D620), sub_428B1C);

    for (i = 0; i < dword_56D61C; i++) {
        obj = off_56D620[i];
        if (a1 != obj && !(obj->data.critter.combat.results & 0x80) && ((a3 & 0x02) && a2->data.critter.combat.team != obj->data.critter.combat.team || (a3 & 0x01) && a2->data.critter.combat.team == obj->data.critter.combat.team)) {
            return obj;
        }
    }

    return NULL;
}

// 0x428CF4
Object* sub_428CF4(Object* a1, Object* a2, int a3)
{
    if (a2 == NULL) {
        return NULL;
    }

    if (dword_56D61C == 0) {
        return NULL;
    }

    int team = a2->data.critter.combat.team;

    off_51805C = a1;
    qsort(off_56D620, dword_56D61C, sizeof(*off_56D620), sub_428B1C);

    for (int index = 0; index < dword_56D61C; index++) {
        Object* obj = off_56D620[index];
        if (obj != a1
            && (obj->data.critter.combat.results & DAM_DEAD) == 0
            && ((a3 & 0x02) != 0 && team != obj->data.critter.combat.team
                || (a3 & 0x01) != 0 && team == obj->data.critter.combat.team)) {
            if (obj->data.critter.combat.whoHitMe != NULL) {
                return obj;
            }
        }
    }

    return NULL;
}

// 0x428DB0
int sub_428DB0(Object* a1, Object** a2, Object** a3, Object** a4)
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

    if (dword_56D61C == 0) {
        return 0;
    }

    off_51805C = a1;
    qsort(off_56D620, dword_56D61C, sizeof(*off_56D620), sub_428B1C);

    int team = a1->data.critter.combat.team;

    for (int index = 0; index < 3 && index < dword_56D61C; index++) {
        Object* candidate = off_56D620[index];
        if (candidate != a1) {
            if (a2 != NULL && *a2 == NULL) {
                if ((candidate->data.critter.combat.results & DAM_DEAD) == 0
                    && candidate->data.critter.combat.whoHitMe == a1) {
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
Object* sub_428F4C(Object* a1)
{
    if (a1 == NULL) {
        return NULL;
    }

    bool v2 = false;
    int attackWho;

    if (objectIsPartyMember(a1)) {
        int disposition = a1 != NULL ? aiGetPacket(a1)->disposition : 0;

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

        if (v2 && aiGetPacket(a1)->distance == 1) {
            v2 = false;
        }

        attackWho = aiGetPacket(a1)->attack_who;
        switch (attackWho) {
        case ATTACK_WHO_WHOMEVER_ATTACKING_ME: {
            Object* candidate = sub_4218EC(gDude);
            if (candidate == NULL || a1->data.critter.combat.team == candidate->data.critter.combat.team) {
                break;
            }

            if (pathfinderFindPath(a1, a1->tile, gDude->data.critter.combat.whoHitMe->tile, NULL, 0, sub_48B848) == 0
                && sub_426614(a1, candidate, HIT_MODE_RIGHT_WEAPON_PRIMARY, false) != 0) {
                debugPrint("\nai_danger_source: %s couldn't attack at target!  Picking alternate!", critterGetName(a1));
                break;
            }

            if (v2 && critterIsFleeing(a1)) {
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

    Object* v14[4];
    Object* whoHitMe = a1->data.critter.combat.whoHitMe;
    if (whoHitMe == NULL || a1 == whoHitMe) {
        v14[0] = NULL;
    } else {
        if ((whoHitMe->data.critter.combat.results & DAM_DEAD) == 0) {
            if (attackWho == ATTACK_WHO_WHOMEVER || attackWho == -1) {
                return whoHitMe;
            }
        } else {
            if (whoHitMe->data.critter.combat.team != a1->data.critter.combat.team) {
                v14[0] = sub_428C3C(a1, whoHitMe, 1);
            } else {
                v14[0] = NULL;
            }
        }
    }

    sub_428DB0(a1, &(v14[1]), &(v14[2]), &(v14[3]));

    if (v2) {
        for (int index = 0; index < 4; index++) {
            if (v14[index] != NULL && critterIsFleeing(v14[index])) {
                v14[index] = NULL;
            }
        }
    }

    int (*compareProc)(const void*, const void*);
    switch (attackWho) {
    case ATTACK_WHO_STRONGEST:
        compareProc = sub_428B8C;
        break;
    case ATTACK_WHO_WEAKEST:
        compareProc = sub_428BE4;
        break;
    default:
        compareProc = sub_428B1C;
        off_51805C = a1;
        break;
    }

    qsort(v14, 4, sizeof(*v14), compareProc);

    for (int index = 0; index < 4; index++) {
        Object* candidate = v14[index];
        if (candidate != NULL && objectCanHearObject(a1, candidate)) {
            if (pathfinderFindPath(a1, a1->tile, candidate->tile, NULL, 0, sub_48B848) != 0
                || sub_426614(a1, candidate, HIT_MODE_RIGHT_WEAPON_PRIMARY, false) == 0) {
                return candidate;
            }
            debugPrint("\nai_danger_source: I couldn't get at my target!  Picking alternate!");
        }
    }

    return NULL;
}

// 0x4291C4
int sub_4291C4(Object* a1, Object* a2)
{
    Object* obj;

    obj = objectFindFirstAtElevation(a1->elevation);
    while (obj != NULL) {
        if ((obj->pid >> 24) == OBJ_TYPE_CRITTER && obj != gDude) {
            obj->data.critter.combat.maneuver |= 0x01;
        }
        obj = objectFindNextAtElevation();
    }

    off_518150 = a1;
    off_518154 = a2;

    return 0;
}

// 0x_429210
int sub_429210(Object** a1, int a2)
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

    if (off_518150 == NULL) {
        return 0;
    }

    v9 = off_518150->data.critter.combat.team;
    v10 = off_518154->data.critter.combat.team;

    for (i = 0; i < a2; i++) {
        if (a1[i]->data.critter.combat.team == v9) {
            v8 = off_518154;
        } else if (a1[i]->data.critter.combat.team == v10) {
            v8 = off_518150;
        } else {
            continue;
        }

        a1[i]->data.critter.combat.whoHitMe = sub_428C3C(a1[i], v8, 1);
    }

    off_518150 = NULL;
    off_518154 = NULL;

    return 0;
}

// 0x4292C0
void sub_4292C0()
{
    off_518154 = 0;
    off_518150 = 0;
}

// 0x4292D4
int sub_4292D4(Object* critter_obj, Object* weapon_obj, Object** out_ammo_obj)
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
        ammo_obj = sub_472698(critter_obj, 4, &v9);
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
            if (sub_478A1C(critter_obj, 2) < 3) {
                sub_472A54(critter_obj, 1);
            }
        } else {
            sub_472A54(critter_obj, 1);
        }
    }

    return 0;
}

// 0x42938C
bool sub_42938C(AiPacket* ai, int attackType)
{
    int bestWeapon = ai->best_weapon + 1;

    for (int index = 0; index < 5; index++) {
        if (attackType == dword_518158[bestWeapon][index]) {
            return true;
        }
    }

    return false;
}

// 0x4293BC
Object* sub_4293BC(Object* attacker, Object* weapon1, Object* weapon2, Object* defender)
{
    if (attacker == NULL) {
        return NULL;
    }

    AiPacket* ai = aiGetPacket(attacker);
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
    attackInit(&attack, attacker, defender, HIT_MODE_RIGHT_WEAPON_PRIMARY, HIT_LOCATION_TORSO);

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
        if (sub_47910C(weapon1, HIT_MODE_RIGHT_WEAPON_PRIMARY) > 0 && defender != NULL) {
            attack.weapon = weapon1;
            sub_423C10(&attack, 0, sub_4790E8(weapon1), 1);
            avgDamage1 *= attack.extrasLength + 1;
        }

        // TODO: Probably an error, why it takes [weapon2], should likely use
        // [weapon1].
        if (weaponGetPerk(weapon2) != -1) {
            avgDamage1 *= 5;
        }

        if (defender != NULL) {
            if (sub_4213E8(attacker, weapon1, HIT_MODE_RIGHT_WEAPON_PRIMARY, defender, NULL)) {
                v24 = 1;
            }
        }

        if (weaponIsNatural(weapon1)) {
            return weapon1;
        }
    } else {
        distance = objectGetDistanceBetween(attacker, defender);
        if (sub_478A1C(attacker, HIT_MODE_PUNCH) >= distance) {
            attackType1 = ATTACK_TYPE_UNARMED;
        }
    }

    if (!v24) {
        for (int index = 0; index < ATTACK_TYPE_COUNT; index++) {
            if (dword_518158[ai->best_weapon + 1][index] == attackType1) {
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
        if (sub_47910C(weapon2, HIT_MODE_RIGHT_WEAPON_PRIMARY) > 0 && defender != NULL) {
            attack.weapon = weapon2;
            sub_423C10(&attack, 0, sub_4790E8(weapon2), 1);
            avgDamage2 *= attack.extrasLength + 1;
        }

        if (weaponGetPerk(weapon2) != -1) {
            avgDamage2 *= 5;
        }

        if (defender != NULL) {
            if (sub_4213E8(attacker, weapon2, HIT_MODE_RIGHT_WEAPON_PRIMARY, defender, NULL)) {
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

        if (sub_478A1C(attacker, HIT_MODE_PUNCH) >= distance) {
            attackType2 = ATTACK_TYPE_UNARMED;
        }
    }

    if (!v23) {
        for (int index = 0; index < ATTACK_TYPE_COUNT; index++) {
            if (dword_518158[ai->best_weapon + 1][index] == attackType2) {
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
bool sub_4298EC(Object* critter, Object* weapon, int hitMode)
{
    int damageFlags = critter->data.critter.combat.results;
    if ((damageFlags & DAM_CRIP_ARM_LEFT) != 0 && (damageFlags & DAM_CRIP_ARM_RIGHT) != 0) {
        return false;
    }

    if ((damageFlags & (DAM_CRIP_ARM_LEFT | DAM_CRIP_ARM_RIGHT)) != 0 && weaponIsTwoHanded(weapon)) {
        return false;
    }

    int rotation = critter->rotation + 1;
    int animationCode = weaponGetAnimationCode(weapon);
    int v9 = weaponGetAnimationForHitMode(weapon, hitMode);
    int fid = buildFid(1, critter->fid & 0xFFF, v9, animationCode, rotation);
    if (!artExists(fid)) {
        return false;
    }

    int skill = weaponGetSkillForHitMode(weapon, hitMode);
    AiPacket* ai = aiGetPacket(critter);
    if (skillGetValue(critter, skill) < ai->min_to_hit) {
        return false;
    }

    int attackType = weaponGetAttackTypeForHitMode(weapon, HIT_MODE_RIGHT_WEAPON_PRIMARY);
    return sub_42938C(ai, attackType) != 0;
}

// 0x4299A0
Object* sub_4299A0(Object* critter, int a2, Object* a3)
{
    int bodyType = critterGetBodyType(critter);
    if (bodyType != BODY_TYPE_BIPED
        && bodyType != BODY_TYPE_ROBOTIC
        && critter->pid != PROTO_ID_0x1000098) {
        return NULL;
    }

    int token = -1;
    Object* bestWeapon = NULL;
    Object* rightHandWeapon = critterGetItem2(critter);
    while (true) {
        Object* weapon = sub_472698(critter, ITEM_TYPE_WEAPON, &token);
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

        if (!sub_4298EC(critter, weapon, HIT_MODE_RIGHT_WEAPON_PRIMARY)) {
            continue;
        }

        if (weaponGetAttackTypeForHitMode(weapon, HIT_MODE_RIGHT_WEAPON_PRIMARY) == ATTACK_TYPE_RANGED) {
            if (ammoGetQuantity(weapon) == 0) {
                if (!sub_4292D4(critter, weapon, NULL)) {
                    continue;
                }
            }
        }

        bestWeapon = sub_4293BC(critter, bestWeapon, weapon, a3);
    }

    return bestWeapon;
}

// Finds new best armor (other than what's already equipped) based on the armor score.
//
// 0x429A6C
Object* sub_429A6C(Object* critter)
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
        Object* candidate = sub_472698(critter, ITEM_TYPE_ARMOR, &v15);
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
bool aiCanUseItem(Object* critter, Object* item)
{
    if (critter == NULL) {
        return false;
    }

    if (item == NULL) {
        return false;
    }

    AiPacket* ai = aiGetPacketByNum(critter->data.critter.combat.aiPacket);
    if (ai == NULL) {
        return false;
    }

    for (int index = 0; index < AI_PACKET_CHEM_PRIMARY_DESIRE_COUNT; index++) {
        if (item->pid == ai->chem_primary_desire[index]) {
            return true;
        }
    }

    if (critterGetBodyType(critter) != BODY_TYPE_BIPED) {
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
Object* sub_429C18(Object* critter, int itemType)
{
    if (critterGetBodyType(critter) != BODY_TYPE_BIPED) {
        return NULL;
    }

    Object** objects;
    int count = objectListCreate(-1, gElevation, OBJ_TYPE_ITEM, &objects);
    if (count == 0) {
        return NULL;
    }

    off_51805C = critter;
    qsort(objects, count, sizeof(*objects), sub_428B1C);

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
                if (sub_4298EC(critter, item, HIT_MODE_RIGHT_WEAPON_PRIMARY)) {
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
                if (aiCanUseItem(critter, item)) {
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
Object* sub_429D60(Object* a1, Object* a2)
{
    if (actionPickUp(a1, a2) != 0) {
        return NULL;
    }

    sub_4227DC();

    Object* v3 = sub_4726EC(a1, a2->id);

    // TODO: Not sure about this one.
    if (v3 != NULL || a2->owner != NULL) {
        a2 = NULL;
    }

    sub_421998(v3, a2);

    return v3;
}

// 0x429DB4
int sub_429DB4(Object* a1, Object* a2, Object* a3)
{
    if (a2 == NULL) {
        return HIT_MODE_PUNCH;
    }

    if (itemGetType(a2) != ITEM_TYPE_WEAPON) {
        return HIT_MODE_PUNCH;
    }

    int attackType = weaponGetAttackTypeForHitMode(a2, HIT_MODE_RIGHT_WEAPON_SECONDARY);
    int intelligence = critterGetStat(a1, STAT_INTELLIGENCE);
    if (attackType == ATTACK_TYPE_NONE || !sub_4298EC(a1, a2, HIT_MODE_RIGHT_WEAPON_SECONDARY)) {
        return HIT_MODE_RIGHT_WEAPON_PRIMARY;
    }

    bool useSecondaryMode = false;

    AiPacket* ai = aiGetPacket(a1);
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
            if (sub_42436C(a1, a3, HIT_LOCATION_TORSO, HIT_MODE_RIGHT_WEAPON_SECONDARY) >= 85
                && !sub_4213E8(a1, a2, 3, a3, 0)) {
                useSecondaryMode = true;
            }
            break;
        case AREA_ATTACK_MODE_BE_CAREFUL:
            if (sub_42436C(a1, a3, HIT_LOCATION_TORSO, HIT_MODE_RIGHT_WEAPON_SECONDARY) >= 50
                && !sub_4213E8(a1, a2, 3, a3, 0)) {
                useSecondaryMode = true;
            }
            break;
        case AREA_ATTACK_MODE_BE_ABSOLUTELY_SURE:
            if (sub_42436C(a1, a3, HIT_LOCATION_TORSO, HIT_MODE_RIGHT_WEAPON_SECONDARY) >= 95
                && !sub_4213E8(a1, a2, 3, a3, 0)) {
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
        if (!sub_42938C(ai, attackType)) {
            useSecondaryMode = false;
        }
    }

    if (useSecondaryMode) {
        if (attackType != ATTACK_TYPE_THROW
            || sub_4299A0(a1, 0, a3) != NULL
            || statRoll(a1, STAT_INTELLIGENCE, 0, NULL) <= 1) {
            return HIT_MODE_RIGHT_WEAPON_SECONDARY;
        }
    }

    return HIT_MODE_RIGHT_WEAPON_PRIMARY;
}

// 0x429FC8
int sub_429FC8(Object* a1, Object* a2, int actionPoints, int a4)
{
    if (actionPoints <= 0) {
        return -1;
    }

    int distance = aiGetPacket(a1)->distance;
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

    reg_anim_begin(2);

    if (a4) {
        sub_42B634(a1, NULL, AI_MESSAGE_TYPE_MOVE, 0);
    }

    Object* v18 = a2;

    bool v19;
    if ((a2->flags & 0x800) != 0) {
        v19 = true;
        a2->flags |= 0x01;
    } else {
        v19 = false;
    }

    if (pathfinderFindPath(a1, a1->tile, a2->tile, NULL, 0, sub_48B848) == 0) {
        off_519794 = NULL;
        if (pathfinderFindPath(a1, a1->tile, a2->tile, NULL, 0, sub_48BA20) == 0
            && off_519794 != NULL
            && (off_519794->pid >> 24) == OBJ_TYPE_CRITTER) {
            if (v19) {
                a2->flags &= ~0x01;
            }

            a2 = off_519794;
            if ((a2->flags & 0x800) != 0) {
                v19 = true;
                a2->flags |= 0x01;
            } else {
                v19 = false;
            }
        }
    }

    if (v19) {
        a2->flags &= ~0x01;
    }

    int tile = a2->tile;
    if (a2 == v18) {
        sub_42A1D4(a1, a2, &tile);
    }

    if (actionPoints >= critterGetStat(a1, STAT_MAXIMUM_ACTION_POINTS) / 2 && artCritterFidShouldRun(a1->fid)) {
        if ((a2->flags & 0x800) != 0) {
            reg_anim_obj_run_to_obj(a1, a2, actionPoints, 0);
        } else {
            reg_anim_obj_run_to_tile(a1, tile, a1->elevation, actionPoints, 0);
        }
    } else {
        if ((a2->flags & 0x800) != 0) {
            reg_anim_obj_move_to_obj(a1, a2, actionPoints, 0);
        } else {
            reg_anim_obj_move_to_tile(a1, tile, a1->elevation, actionPoints, 0);
        }
    }

    if (reg_anim_end() != 0) {
        return -1;
    }

    sub_4227DC();

    return 0;
}

// 0x42A1D4
int sub_42A1D4(Object* a1, Object* a2, int* a3)
{
    if (a1 == NULL) {
        return -1;
    }

    if (a2 == NULL) {
        return -1;
    }

    if (a3 == NULL) {
        return -1;
    }

    if (*a3 == -1) {
        return -1;
    }

    if (dword_56D61C == 0) {
        return -1;
    }

    int tiles[32];

    STRUCT_832 v1;
    v1.field_0 = a1;
    v1.field_4 = a2;
    v1.field_32C = a1->data.critter.combat.team;
    v1.field_330 = sub_42B90C(a1);
    v1.field_328 = 0;
    v1.field_338 = tiles;
    v1.field_334 = *a3 != a1->tile;
    v1.field_33C = 0;
    v1.field_340 = critterGetStat(a1, STAT_INTELLIGENCE);

    for (int index = 0; index < 32; index++) {
        tiles[index] = -1;
    }

    for (int index = 0; index < dword_56D61C; index++) {
        Object* obj = off_56D620[index];
        if ((obj->data.critter.combat.results & DAM_DEAD) == 0
            && obj->data.critter.combat.team == v1.field_32C
            && sub_4218EC(obj) == v1.field_4
            && obj != v1.field_0) {
            int v10 = sub_42B90C(obj);
            if (v10 >= v1.field_330) {
                v1.field_8[v1.field_328] = obj;
                v1.field_198[v1.field_328] = v10;
                v1.field_328 += 1;
            }
        }
    }

    off_51805C = a1;

    qsort(v1.field_8, v1.field_328, sizeof(*v1.field_8), sub_428B1C);

    if (sub_42A410(&v1, *a3) == 0) {
        int minDistance = 99999;
        int minDistanceIndex = -1;

        for (int index = 0; index < 32; index++) {
            int tile = tiles[index];
            if (tile == -1) {
                break;
            }

            if (sub_48B848(NULL, tile, a1->elevation) == 0) {
                int distance = tileDistanceBetween(*a3, tile);
                if (distance < minDistance) {
                    minDistance = distance;
                    minDistanceIndex = index;
                }
            }
        }

        if (minDistanceIndex != -1) {
            *a3 = tiles[minDistanceIndex];
        }
    }

    return 0;
}

// 0x42A410
int sub_42A410(STRUCT_832* a1, int tile)
{
    if (a1->field_340 <= 0) {
        return 0;
    }

    int distance = 1;

    for (int index = 0; index < a1->field_328; index++) {
        Object* obj = a1->field_8[index];
        if (sub_42A518(obj, a1->field_4, a1->field_0, tile, &distance)) {
            debugPrint("In the way!");

            a1->field_338[a1->field_33C] = tileGetTileInDirection(tile, (obj->rotation + 1) % ROTATION_COUNT, distance);
            a1->field_338[a1->field_33C + 1] = tileGetTileInDirection(tile, (obj->rotation + 5) % ROTATION_COUNT, distance);

            a1->field_340 -= 2;
            a1->field_33C += 2;
            break;
        }
    }

    return 0;
}

// 0x42A518
bool sub_42A518(Object* a1, Object* a2, Object* a3, int tile, int* distance)
{
    int hitMode = HIT_MODE_RIGHT_WEAPON_PRIMARY;
    bool aiming = false;
    if (a1 == gDude) {
        interfaceGetCurrentHitMode(&hitMode, &aiming);
    }

    Object* v8 = critterGetWeaponForHitMode(a1, hitMode);
    if (v8 == NULL) {
        return false;
    }

    if (sub_478A1C(a1, hitMode) < 1) {
        return false;
    }

    Object* object = NULL;
    sub_4163C8(a1, a1->tile, a2->tile, NULL, &object, 32, sub_48B930);
    if (object != a3) {
        if (!sub_4217BC(a1, a2, a3, v8)) {
            return false;
        }
    }

    return true;
}

// 0x42A5B8
int sub_42A5B8(Object* a1, int* hitMode, Object** weapon, Object* a4)
{
    *weapon = NULL;
    *hitMode = HIT_MODE_PUNCH;

    Object* bestWeapon = sub_4299A0(a1, 1, a4);
    if (bestWeapon != NULL) {
        *weapon = bestWeapon;
        *hitMode = sub_429DB4(a1, bestWeapon, a4);
    } else {
        Object* v8 = sub_429C18(a1, ITEM_TYPE_WEAPON);
        if (v8 == NULL) {
            if (sub_478B24(a1, *hitMode, 0) <= a1->data.critter.combat.ap) {
                return 0;
            }

            return -1;
        }

        Object* v9 = sub_429D60(a1, v8);
        if (v9 != NULL) {
            *weapon = v9;
            *hitMode = sub_429DB4(a1, v9, a4);
        }
    }

    if (*weapon != NULL) {
        sub_472758(a1, *weapon, 1);
        sub_4227DC();
        if (sub_478B24(a1, *hitMode, 0) <= a1->data.critter.combat.ap) {
            return 0;
        }
    }

    return -1;
}

// 0x42A670
int sub_42A670(Object* a1, Object* a2, int a3)
{
    AiPacket* ai;
    int v5;
    int v6;
    int v7;
    int combat_difficulty;

    v5 = 3;

    if (sub_478B24(a1, a3, 1) <= a1->data.critter.combat.ap) {
        if (sub_478E5C(a1, a3)) {
            ai = aiGetPacket(a1);
            if (randomBetween(1, ai->called_freq) == 1) {
                combat_difficulty = 1;
                configGetInt(&gGameConfig, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_COMBAT_DIFFICULTY_KEY, &combat_difficulty);
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
                    v7 = sub_42436C(a1, a2, a3, v5);
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
int sub_42A748(Object* a1, Object* a2, int a3)
{
    int v6;

    if (a1->data.critter.combat.maneuver & CRITTER_MANUEVER_FLEEING) {
        return -1;
    }

    reg_anim_begin(2);
    reg_anim_set_rotation_to_tile(a1, a2->tile);
    reg_anim_end();
    sub_4227DC();

    v6 = sub_42A670(a1, a2, a3);
    if (sub_422F3C(a1, a2, a3, v6)) {
        return -1;
    }

    sub_4227DC();

    return 0;
}

// 0x42A7D8
int sub_42A7D8(Object* a1, Object* a2)
{
    sub_42E4C0(a1, a2);

    CritterCombatData* combatData = &(a1->data.critter.combat);
    int v38 = 1;

    Object* weapon = critterGetItem2(a1);
    if (weapon != NULL && itemGetType(weapon) != ITEM_TYPE_WEAPON) {
        weapon = NULL;
    }

    int hitMode = sub_429DB4(a1, weapon, a2);
    int minToHit = aiGetPacket(a1)->min_to_hit;

    int actionPoints = a1->data.critter.combat.ap;
    int v31 = 0;
    int v42 = 0;
    if (weapon == NULL) {
        if (critterGetBodyType(a2) != BODY_TYPE_BIPED
            || ((a2->fid & 0xF000) >> 12 != 0)
            || !artExists(buildFid(1, a1->fid & 0xFFF, ANIM_THROW_PUNCH, 0, a1->rotation + 1))
            || sub_4213E8(a1, weapon, HIT_MODE_RIGHT_WEAPON_PRIMARY, a2, &v31)) {
            sub_42A5B8(a1, &hitMode, &weapon, a2);
        }
    }

    unsigned char v30[800];

    Object* ammo = NULL;
    for (int attempt = 0; attempt < 10; attempt++) {
        if ((combatData->results & (DAM_KNOCKED_OUT | DAM_DEAD | DAM_LOSE_TURN)) != 0) {
            break;
        }

        int reason = sub_426614(a1, a2, hitMode, false);
        if (reason == 1) {
            // out of ammo
            if (sub_4292D4(a1, weapon, &ammo)) {
                int v9 = sub_478918(weapon, ammo);
                if (v9 == 0 && ammo != NULL) {
                    sub_49B9A0(ammo);
                }

                if (v9 != -1) {
                    int volume = sub_451534(a1);
                    const char* sfx = sfxBuildWeaponName(WEAPON_SOUND_EFFECT_READY, weapon, hitMode, NULL);
                    sub_45108C(sfx, volume);
                    sub_428398(a1, weapon, 5002);

                    int actionPoints = a1->data.critter.combat.ap;
                    if (actionPoints >= 2) {
                        a1->data.critter.combat.ap = actionPoints - 2;
                    } else {
                        a1->data.critter.combat.ap = 0;
                    }
                }
            } else {
                ammo = sub_429C18(a1, ITEM_TYPE_AMMO);
                if (ammo != NULL) {
                    ammo = sub_429D60(a1, ammo);
                    if (ammo != NULL) {
                        int v15 = sub_478918(weapon, ammo);
                        if (v15 == 0) {
                            sub_49B9A0(ammo);
                        }

                        if (v15 != -1) {
                            int volume = sub_451534(a1);
                            const char* sfx = sfxBuildWeaponName(WEAPON_SOUND_EFFECT_READY, weapon, hitMode, NULL);
                            sub_45108C(sfx, volume);
                            sub_428398(a1, weapon, 5002);

                            int actionPoints = a1->data.critter.combat.ap;
                            if (actionPoints >= 2) {
                                a1->data.critter.combat.ap = actionPoints - 2;
                            } else {
                                a1->data.critter.combat.ap = 0;
                            }
                        }
                    }
                } else {
                    int volume = sub_451534(a1);
                    const char* sfx = sfxBuildWeaponName(WEAPON_SOUND_EFFECT_OUT_OF_AMMO, weapon, hitMode, NULL);
                    sub_45108C(sfx, volume);
                    sub_428398(a1, weapon, 5001);

                    if (sub_472A54(a1, 1) == 0) {
                        sub_4227DC();
                    }

                    sub_42A5B8(a1, &hitMode, &weapon, a2);
                }
            }
        } else if (reason == 3 || reason == 6 || reason == 7) {
            // 3 - not enough action points
            // 6 - crippled one arm for two-handed weapon
            // 7 - both hands crippled
            if (sub_42A5B8(a1, &hitMode, &weapon, a2) == -1) {
                return -1;
            }
        } else if (reason == 2) {
            // target out of range
            int accuracy = sub_424380(a1, a2, HIT_LOCATION_UNCALLED, hitMode, v30);
            if (accuracy < minToHit) {
                const char* name = critterGetName(a1);
                debugPrint("%s: FLEEING: Can't possibly Hit Target!", name);
                sub_428868(a1, a2);
                return 0;
            }

            if (weapon != NULL) {
                if (sub_429FC8(a1, a2, actionPoints, v38) == -1) {
                    return -1;
                }
                v38 = 0;
            } else {
                if (sub_42A5B8(a1, &hitMode, &weapon, a2) == -1 || weapon == NULL) {
                    if (sub_429FC8(a1, a2, a1->data.critter.combat.ap, v38) == -1) {
                        return -1;
                    }
                }
                v38 = 0;
            }
        } else if (reason == 5) {
            // aim is blocked
            if (sub_429FC8(a1, a2, a1->data.critter.combat.ap, v38) == -1) {
                return -1;
            }
            v38 = 0;
        } else if (reason == 0) {
            int accuracy = sub_42436C(a1, a2, HIT_LOCATION_UNCALLED, hitMode);
            if (v31) {
                if (sub_42899C(a1, a2, v31) == -1) {
                    return -1;
                }
            }

            if (accuracy < minToHit) {
                int v22 = sub_424380(a1, a2, HIT_LOCATION_UNCALLED, hitMode, v30);
                if (v22 < minToHit) {
                    const char* name = critterGetName(a1);
                    debugPrint("%s: FLEEING: Can't possibly Hit Target!", name);
                    sub_428868(a1, a2);
                    return 0;
                }

                if (actionPoints > 0) {
                    int v24 = pathfinderFindPath(a1, a1->tile, a2->tile, v30, 0, sub_48B848);
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

                            int v27 = sub_424394(a1, tile, a2, HIT_LOCATION_UNCALLED, hitMode);
                            if (v27 >= minToHit) {
                                break;
                            }
                        }

                        if (index == actionPoints) {
                            v42 = actionPoints;
                        }
                    }
                }

                if (sub_429FC8(a1, a2, v42, v38) == -1) {
                    const char* name = critterGetName(a1);
                    debugPrint("%s: FLEEING: Can't possibly get closer to Target!", name);
                    sub_428868(a1, a2);
                    return 0;
                }

                v38 = 0;
                if (sub_42A748(a1, a2, hitMode) == -1 || sub_478B24(a1, hitMode, 0) > a1->data.critter.combat.ap) {
                    return -1;
                }
            } else {
                if (sub_42A748(a1, a2, hitMode) == -1 || sub_478B24(a1, hitMode, 0) > a1->data.critter.combat.ap) {
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
int sub_42AE90(Object* critter, Object* item)
{
    if (item != NULL && critterGetStat(critter, STAT_INTELLIGENCE) >= 3 && item->pid == PROTO_ID_FLARE && lightGetLightLevel() < 55705) {
        sub_49BF38(critter, item);
    }
    return 0;
}

// 0x42AECC
void sub_42AECC(Object* critter_obj, int a2)
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
    if (v5 < ammoGetCapacity(weapon_obj) && sub_4292D4(critter_obj, weapon_obj, &ammo_obj)) {
        v9 = sub_478918(weapon_obj, ammo_obj);
        if (v9 == 0) {
            sub_49B9A0(ammo_obj);
        }

        if (v9 != -1 && objectIsPartyMember(critter_obj)) {
            v10 = sub_451534(critter_obj);
            sfx = sfxBuildWeaponName(WEAPON_SOUND_EFFECT_READY, weapon_obj, HIT_MODE_RIGHT_WEAPON_PRIMARY, NULL);
            sub_45108C(sfx, v10);
            if (a2) {
                sub_428398(critter_obj, weapon_obj, 5002);
            }
        }
    }
}

// 0x42AF78
void sub_42AF78(int a1, void* a2)
{
    dword_56D61C = a1;

    if (a1 != 0) {
        off_56D620 = internal_malloc(sizeof(Object*) * a1);
        if (off_56D620) {
            memcpy(off_56D620, a2, sizeof(Object*) * a1);
        } else {
            dword_56D61C = 0;
        }
    }
}

// 0x42AFBC
void sub_42AFBC()
{
    if (dword_56D61C) {
        internal_free(off_56D620);
    }

    dword_56D61C = 0;
}

// 0x42AFDC
int sub_42AFDC(Object* a1, Object* a2)
{
    if (a1->data.critter.combat.ap <= 0) {
        return -1;
    }

    int distance = aiGetPacket(a1)->distance;

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
                sub_429FC8(a1, gDude, distance - 5, 0);
            }
        }
        break;
    case DISTANCE_CHARGE:
        if (a2 != NULL) {
            sub_429FC8(a1, a2, a1->data.critter.combat.ap, 1);
        }
        break;
    case DISTANCE_SNIPE:
        if (a2 != NULL) {
            if (objectGetDistanceBetween(a1, a2) < 10) {
                // NOTE: some odd code omitted
                sub_42899C(a1, a2, 10);
            }
        }
        break;
    }

    int tile = a1->tile;
    if (sub_42A1D4(a1, a2, &tile) == 0 && tile != a1->tile) {
        reg_anim_begin(2);
        reg_anim_obj_move_to_tile(a1, tile, a1->elevation, a1->data.critter.combat.ap, 0);
        if (reg_anim_end() != 0) {
            return -1;
        }
        sub_4227DC();
    }

    return 0;
}

// 0x42B100
int sub_42B100(AiPacket* ai)
{
    if (ai == NULL) {
        return 0;
    }

    int run_away_mode = ai->run_away_mode;
    if (run_away_mode >= 0 && run_away_mode < RUN_AWAY_MODE_COUNT) {
        return dword_518138[run_away_mode];
    } else if (run_away_mode == -1) {
        return ai->min_hp;
    }

    return 0;
}

// 0x42B130
void sub_42B130(Object* a1, Object* a2)
{
    AiPacket* ai = aiGetPacket(a1);
    int hpRatio = sub_42B100(ai);
    if (ai->run_away_mode != -1) {
        int v7 = critterGetStat(a1, STAT_MAXIMUM_HIT_POINTS) * hpRatio / 100;
        int minimumHitPoints = critterGetStat(a1, STAT_MAXIMUM_HIT_POINTS) - v7;
        int currentHitPoints = critterGetStat(a1, STAT_CURRENT_HIT_POINTS);
        const char* name = critterGetName(a1);
        debugPrint("\n%s minHp = %d; curHp = %d", name, minimumHitPoints, currentHitPoints);
    }

    CritterCombatData* combatData = &(a1->data.critter.combat);
    if ((combatData->maneuver & CRITTER_MANUEVER_FLEEING) != 0
        || (combatData->results & ai->hurt_too_much) != 0
        || critterGetStat(a1, STAT_CURRENT_HIT_POINTS) < ai->min_hp) {
        const char* name = critterGetName(a1);
        debugPrint("%s: FLEEING: I'm Hurt!", name);
        sub_428868(a1, a2);
        return;
    }

    if (sub_428480(a1)) {
        const char* name = critterGetName(a1);
        debugPrint("%s: FLEEING: I need DRUGS!", name);
        sub_428868(a1, a2);
    } else {
        if (a2 == NULL) {
            a2 = sub_428F4C(a1);
        }

        sub_42AFDC(a1, a2);

        if (a2 != NULL) {
            sub_42A7D8(a1, a2);
        }
    }

    if (a2 != NULL
        && (a1->data.critter.combat.results & DAM_DEAD) == 0
        && a1->data.critter.combat.ap != 0
        && objectGetDistanceBetween(a1, a2) > ai->max_dist) {
        Object* v13 = sub_421880(a1);
        if (v13 != NULL) {
            sub_42899C(a1, v13, 10);
            sub_4218AC(a1, NULL);
        } else {
            int perception = critterGetStat(a1, STAT_PERCEPTION);
            if (!sub_428AC4(a1, perception * 2, 5)) {
                combatData->maneuver |= CRITTER_MANEUVER_STOP_ATTACKING;
            }
        }
    }

    if (a2 == NULL && !objectIsPartyMember(a1)) {
        Object* whoHitMe = combatData->whoHitMe;
        if (whoHitMe != NULL) {
            if ((whoHitMe->data.critter.combat.results & DAM_DEAD) == 0 && combatData->damageLastTurn > 0) {
                Object* v16 = sub_421880(a1);
                if (v16 != NULL) {
                    sub_42899C(a1, v16, 10);
                    sub_4218AC(a1, NULL);
                } else {
                    const char* name = critterGetName(a1);
                    debugPrint("%s: FLEEING: Somebody is shooting at me that I can't see!");
                    sub_428868(a1, NULL);
                }
            }
        }
    }

    Object* v18 = sub_421880(a1);
    if (v18 != NULL) {
        sub_42899C(a1, v18, 10);
        if (objectGetDistanceBetween(a1, v18) >= 10) {
            sub_4218AC(a1, NULL);
        }
    }

    Object* v20;
    int v21 = 5; // 0x42B156
    if (a1->data.critter.combat.team != 0) {
        v20 = sub_428CF4(a1, a1, 1);
    } else {
        v20 = gDude;
        if (objectIsPartyMember(a1)) {
            // NOTE: Uninline
            int distance = aiGetDistance(a1);
            if (distance != -1) {
                v21 = dword_51820C[distance];
            }
        }
    }

    if (a2 == NULL && v20 != NULL && objectGetDistanceBetween(a1, v20) > v21) {
        int v23 = objectGetDistanceBetween(a1, v20);
        sub_429FC8(a1, v20, v23 - v21, 0);
    } else {
        if (a1->data.critter.combat.ap > 0) {
            debugPrint("\n>>>NOTE: %s had extra AP's to use!<<<", critterGetName(a1));
            sub_42AFDC(a1, a2);
        }
    }
}

// 0x42B3FC
bool sub_42B3FC(Object* a1)
{
    sub_4C8BDC();

    if ((a1->flags & 0x01) != 0) {
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

    if (sub_428F4C(a1) == NULL) {
        return false;
    }

    return true;
}

// 0x42B4A8
bool sub_42B4A8(Object* a1)
{
    sub_4C8BDC();

    if ((a1->data.critter.combat.maneuver & CRITTER_MANEUVER_STOP_ATTACKING) != 0) {
        return true;
    }

    if ((a1->data.critter.combat.results & (DAM_KNOCKED_OUT | DAM_DEAD)) != 0) {
        return true;
    }

    if ((a1->data.critter.combat.maneuver & CRITTER_MANUEVER_FLEEING) != 0) {
        return true;
    }

    Object* v4 = sub_428F4C(a1);
    return v4 == NULL || !objectCanHearObject(a1, v4);
}

// 0x42B504
int critterSetTeam(Object* obj, int team)
{
    if ((obj->pid >> 24) != OBJ_TYPE_CRITTER) {
        return 0;
    }

    obj->data.critter.combat.team = team;

    if (obj->data.critter.combat.whoHitMeCid == -1) {
        sub_42E4C0(obj, NULL);
        debugPrint("\nError: CombatData found with invalid who_hit_me!");
        return -1;
    }

    Object* whoHitMe = obj->data.critter.combat.whoHitMe;
    if (whoHitMe != NULL) {
        if (whoHitMe->data.critter.combat.team == team) {
            sub_42E4C0(obj, NULL);
        }
    }

    sub_421918(obj, NULL);

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
int critterSetAiPacket(Object* object, int aiPacket)
{
    if ((object->pid >> 24) != OBJ_TYPE_CRITTER) {
        return -1;
    }

    object->data.critter.combat.aiPacket = aiPacket;

    if (sub_494F64(object)) {
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
int sub_42B634(Object* a1, Attack* attack, int type, int delay)
{
    if ((a1->pid >> 24) != OBJ_TYPE_CRITTER) {
        return -1;
    }

    bool combatTaunts = true;
    configGetBool(&gGameConfig, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_COMBAT_TAUNTS_KEY, &combatTaunts);
    if (!combatTaunts) {
        return -1;
    }

    if (a1 == gDude) {
        return -1;
    }

    if (a1->data.critter.combat.results & 0x81) {
        return -1;
    }

    AiPacket* ai = aiGetPacket(a1);

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
        string = byte_56D624;
        break;
    case AI_MESSAGE_TYPE_MOVE:
        start = ai->move.start;
        end = ai->move.end;
        string = byte_56D624;
        break;
    case AI_MESSAGE_TYPE_ATTACK:
        start = ai->attack.start;
        end = ai->attack.end;
        string = byte_56D624;
        break;
    case AI_MESSAGE_TYPE_MISS:
        start = ai->miss.start;
        end = ai->miss.end;
        string = byte_56D518;
        break;
    case AI_MESSAGE_TYPE_HIT:
        start = ai->hit[attack->defenderHitLocation].start;
        end = ai->hit[attack->defenderHitLocation].end;
        string = byte_56D518;
        break;
    default:
        return -1;
    }

    if (end < start) {
        return -1;
    }

    MessageListItem messageListItem;
    messageListItem.num = randomBetween(start, end);
    if (!messageListGetItem(&gCombatAiMessageList, &messageListItem)) {
        debugPrint("\nERROR: combatai_msg: Couldn't find message # %d for %s", messageListItem.num, critterGetName(a1));
        return -1;
    }

    debugPrint("%s said message %d\n", objectGetName(a1), messageListItem.num);
    strncpy(string, messageListItem.text, 259);

    // TODO: Get rid of casts.
    return reg_anim_11_0(a1, (Object*)type, (AnimationProc*)sub_42B80C, delay);
}

// 0x42B80C
int sub_42B80C(Object* critter, int type)
{
    if (textObjectsGetCount() > 0) {
        return 0;
    }

    char* string;
    switch (type) {
    case AI_MESSAGE_TYPE_HIT:
    case AI_MESSAGE_TYPE_MISS:
        string = byte_56D518;
        break;
    default:
        string = byte_56D624;
        break;
    }

    AiPacket* ai = aiGetPacket(critter);

    Rect rect;
    if (textObjectAdd(critter, string, ai->font, ai->color, ai->outline_color, &rect) == 0) {
        tileWindowRefreshRect(&rect, critter->elevation);
    }

    return 0;
}

// Returns random critter for attacking as a result of critical weapon failure.
//
// 0x42B868
Object* sub_42B868(Attack* attack)
{
    // Looks like this function does nothing because it's result is not used. I
    // suppose it was planned to use range as a condition below, but it was
    // later moved into 0x426614, but remained here.
    sub_478A1C(attack->attacker, attack->hitMode);

    Object* critter = NULL;

    if (dword_56D61C != 0) {
        // Randomize starting critter.
        int start = randomBetween(0, dword_56D61C - 1);
        int index = start;
        while (true) {
            Object* obj = off_56D620[index];
            if (obj != attack->attacker
                && obj != attack->defender
                && sub_412BEC(attack->attacker, obj)
                && sub_426614(attack->attacker, obj, attack->hitMode, false)) {
                critter = obj;
                break;
            }

            index += 1;
            if (index == dword_56D61C) {
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
int sub_42B90C(Object* obj)
{
    int melee_damage;
    Object* item;
    int weapon_damage_min;
    int weapon_damage_max;

    if (obj == NULL) {
        return 0;
    }

    if ((obj->fid & 0xF000000) >> 24 != OBJ_TYPE_CRITTER) {
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
int sub_42B9D4(Object* a1, Object* a2)
{
    Object* whoHitMe = a1->data.critter.combat.whoHitMe;
    if (whoHitMe != NULL) {
        int v3 = sub_42B90C(a2);
        int result = sub_42B90C(whoHitMe);
        if (v3 <= result) {
            return result;
        }
    }
    return sub_42E4C0(a1, a2);
}

// 0x42BA04
bool objectCanHearObject(Object* a1, Object* a2)
{
    if (a2 == NULL) {
        return false;
    }

    int distance = objectGetDistanceBetween(a2, a1);
    int perception = critterGetStat(a1, STAT_PERCEPTION);
    int sneak = skillGetValue(a2, SKILL_SNEAK);
    if (sub_412BEC(a1, a2)) {
        int v8 = perception * 5;
        if ((a2->flags & 0x20000) != 0) {
            v8 /= 2;
        }

        if (a2 == gDude) {
            if (dudeIsSneaking()) {
                v8 /= 4;
                if (sneak > 120) {
                    v8 -= 1;
                }
            } else if (dudeHasState(0)) {
                v8 = v8 * 2 / 3;
            }
        }

        if (distance <= v8) {
            return true;
        }
    }

    int v12;
    if (isInCombat()) {
        v12 = perception * 2;
    } else {
        v12 = perception;
    }

    if (a2 == gDude) {
        if (dudeIsSneaking()) {
            v12 /= 4;
            if (sneak > 120) {
                v12 -= 1;
            }
        } else if (dudeHasState(0)) {
            v12 = v12 * 2 / 3;
        }
    }

    if (distance <= v12) {
        return true;
    }

    return false;
}

// Load combatai.msg and apply language filter.
//
// 0x42BB34
int aiMessageListInit()
{
    if (!messageListInit(&gCombatAiMessageList)) {
        return -1;
    }

    char path[MAX_PATH];
    sprintf(path, "%s%s", asc_5186C8, "combatai.msg");

    if (!messageListLoad(&gCombatAiMessageList, path)) {
        return -1;
    }

    bool languageFilter;
    configGetBool(&gGameConfig, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_LANGUAGE_FILTER_KEY, &languageFilter);

    if (languageFilter) {
        messageListFilterBadwords(&gCombatAiMessageList);
    }

    return 0;
}

// NOTE: Inlined.
//
// 0x42BBD8
int aiMessageListFree()
{
    if (!messageListFree(&gCombatAiMessageList)) {
        return -1;
    }

    return 0;
}

// 0x42BBF0
void aiMessageListReloadIfNeeded()
{
    bool languageFilter = false;
    configGetBool(&gGameConfig, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_LANGUAGE_FILTER_KEY, &languageFilter);

    if (languageFilter != gLanguageFilter) {
        gLanguageFilter = languageFilter;

        if (languageFilter) {
            messageListFilterBadwords(&gCombatAiMessageList);
        } else {
            // NOTE: Uninline.
            aiMessageListFree();

            aiMessageListInit();
        }
    }
}

// 0x42BC60
void sub_42BC60(Object* a1)
{
    for (int index = 0; index < dword_56D61C; index++) {
        Object* obj = off_56D620[index];
        if ((obj->data.critter.combat.maneuver & CRITTER_MANEUVER_0x01) == 0) {
            if (objectCanHearObject(obj, a1)) {
                obj->data.critter.combat.maneuver |= CRITTER_MANEUVER_0x01;
                if ((a1->data.critter.combat.results & DAM_DEAD) != 0) {
                    if (!objectCanHearObject(obj, obj->data.critter.combat.whoHitMe)) {
                        debugPrint("\nSomebody Died and I don't know why!  Run!!!");
                        sub_4218AC(obj, a1);
                    }
                }
            }
        }
    }
}

// 0x42BCD4
void sub_42BCD4(Object* a1)
{
    int team = a1->data.critter.combat.team;

    for (int index = 0; index < dword_56D61C; index++) {
        Object* obj = off_56D620[index];
        if ((obj->data.critter.combat.maneuver & CRITTER_MANEUVER_0x01) == 0 && team == obj->data.critter.combat.team) {
            if (objectCanHearObject(obj, a1)) {
                obj->data.critter.combat.maneuver |= CRITTER_MANEUVER_0x01;
            }
        }
    }
}

// 0x42BD28
void sub_42BD28(Object* obj)
{
    // TODO: Check entire function.
    for (int i = 0; i < dword_56D61C; i++) {
        if (obj == off_56D620[i]) {
            dword_56D61C--;
            off_56D620[i] = off_56D620[dword_56D61C];
            off_56D620[dword_56D61C] = obj;
            break;
        }
    }
}
