#include "game/stat.h"

#include <stdio.h>

#include "game/combat.h"
#include "plib/gnw/input.h"
#include "game/critter.h"
#include "game/display.h"
#include "game/game.h"
#include "game/gsound.h"
#include "game/intface.h"
#include "game/item.h"
#include "game/message.h"
#include "plib/gnw/memory.h"
#include "game/object.h"
#include "game/perk.h"
#include "game/proto.h"
#include "game/roll.h"
#include "game/scripts.h"
#include "game/skill.h"
#include "game/tile.h"
#include "game/trait.h"

// Provides metadata about stats.
typedef struct StatDescription {
    char* name;
    char* description;
    int frmId;
    int minimumValue;
    int maximumValue;
    int defaultValue;
} StatDescription;

// 0x51D53C
static StatDescription stat_data[STAT_COUNT] = {
    { NULL, NULL, 0, PRIMARY_STAT_MIN, PRIMARY_STAT_MAX, 5 },
    { NULL, NULL, 1, PRIMARY_STAT_MIN, PRIMARY_STAT_MAX, 5 },
    { NULL, NULL, 2, PRIMARY_STAT_MIN, PRIMARY_STAT_MAX, 5 },
    { NULL, NULL, 3, PRIMARY_STAT_MIN, PRIMARY_STAT_MAX, 5 },
    { NULL, NULL, 4, PRIMARY_STAT_MIN, PRIMARY_STAT_MAX, 5 },
    { NULL, NULL, 5, PRIMARY_STAT_MIN, PRIMARY_STAT_MAX, 5 },
    { NULL, NULL, 6, PRIMARY_STAT_MIN, PRIMARY_STAT_MAX, 5 },
    { NULL, NULL, 10, 0, 999, 0 },
    { NULL, NULL, 75, 1, 99, 0 },
    { NULL, NULL, 18, 0, 999, 0 },
    { NULL, NULL, 31, 0, INT_MAX, 0 },
    { NULL, NULL, 32, 0, 500, 0 },
    { NULL, NULL, 20, 0, 999, 0 },
    { NULL, NULL, 24, 0, 60, 0 },
    { NULL, NULL, 25, 0, 30, 0 },
    { NULL, NULL, 26, 0, 100, 0 },
    { NULL, NULL, 94, -60, 100, 0 },
    { NULL, NULL, 0, 0, 100, 0 },
    { NULL, NULL, 0, 0, 100, 0 },
    { NULL, NULL, 0, 0, 100, 0 },
    { NULL, NULL, 0, 0, 100, 0 },
    { NULL, NULL, 0, 0, 100, 0 },
    { NULL, NULL, 0, 0, 100, 0 },
    { NULL, NULL, 0, 0, 100, 0 },
    { NULL, NULL, 22, 0, 90, 0 },
    { NULL, NULL, 0, 0, 90, 0 },
    { NULL, NULL, 0, 0, 90, 0 },
    { NULL, NULL, 0, 0, 90, 0 },
    { NULL, NULL, 0, 0, 90, 0 },
    { NULL, NULL, 0, 0, 100, 0 },
    { NULL, NULL, 0, 0, 90, 0 },
    { NULL, NULL, 83, 0, 95, 0 },
    { NULL, NULL, 23, 0, 95, 0 },
    { NULL, NULL, 0, 16, 101, 25 },
    { NULL, NULL, 0, 0, 1, 0 },
    { NULL, NULL, 10, 0, 2000, 0 },
    { NULL, NULL, 11, 0, 2000, 0 },
    { NULL, NULL, 12, 0, 2000, 0 },
};

// 0x51D8CC
static StatDescription pc_stat_data[PC_STAT_COUNT] = {
    { NULL, NULL, 0, 0, INT_MAX, 0 },
    { NULL, NULL, 0, 1, PC_LEVEL_MAX, 1 },
    { NULL, NULL, 0, 0, INT_MAX, 0 },
    { NULL, NULL, 0, -20, 20, 0 },
    { NULL, NULL, 0, 0, INT_MAX, 0 },
};

// 0x66817C
static MessageList stat_message_file;

// 0x668184
static char* level_description[PRIMARY_STAT_RANGE];

// 0x6681AC
static int curr_pc_stat[PC_STAT_COUNT];

// 0x4AED70
int stat_init()
{
    MessageListItem messageListItem;

    // NOTE: Uninline.
    stat_pc_set_defaults();

    if (!message_init(&stat_message_file)) {
        return -1;
    }

    char path[MAX_PATH];
    sprintf(path, "%s%s", msg_path, "stat.msg");

    if (!message_load(&stat_message_file, path)) {
        return -1;
    }

    for (int stat = 0; stat < STAT_COUNT; stat++) {
        stat_data[stat].name = getmsg(&stat_message_file, &messageListItem, 100 + stat);
        stat_data[stat].description = getmsg(&stat_message_file, &messageListItem, 200 + stat);
    }

    for (int pcStat = 0; pcStat < PC_STAT_COUNT; pcStat++) {
        pc_stat_data[pcStat].name = getmsg(&stat_message_file, &messageListItem, 400 + pcStat);
        pc_stat_data[pcStat].description = getmsg(&stat_message_file, &messageListItem, 500 + pcStat);
    }

    for (int index = 0; index < PRIMARY_STAT_RANGE; index++) {
        level_description[index] = getmsg(&stat_message_file, &messageListItem, 301 + index);
    }

    return 0;
}

// 0x4AEEC0
int stat_reset()
{
    // NOTE: Uninline.
    stat_pc_set_defaults();

    return 0;
}

// 0x4AEEE4
int stat_exit()
{
    message_exit(&stat_message_file);

    return 0;
}

// 0x4AEEF4
int stat_load(File* stream)
{
    for (int index = 0; index < PC_STAT_COUNT; index++) {
        if (fileReadInt32(stream, &(curr_pc_stat[index])) == -1) {
            return -1;
        }
    }

    return 0;
}

// 0x4AEF20
int stat_save(File* stream)
{
    for (int index = 0; index < PC_STAT_COUNT; index++) {
        if (db_fwriteInt(stream, curr_pc_stat[index]) == -1) {
            return -1;
        }
    }

    return 0;
}

// 0x4AEF48
int critterGetStat(Object* critter, int stat)
{
    int value;
    if (stat >= 0 && stat < SAVEABLE_STAT_COUNT) {
        value = stat_get_base(critter, stat);
        value += stat_get_bonus(critter, stat);

        switch (stat) {
        case STAT_PERCEPTION:
            if ((critter->data.critter.combat.results & DAM_BLIND) != 0) {
                value -= 5;
            }
            break;
        case STAT_MAXIMUM_ACTION_POINTS:
            if (1) {
                int remainingCarryWeight = critterGetStat(critter, STAT_CARRY_WEIGHT) - item_total_weight(critter);
                if (remainingCarryWeight < 0) {
                    value -= -remainingCarryWeight / 40 + 1;
                }
            }
            break;
        case STAT_ARMOR_CLASS:
            if (isInCombat()) {
                if (combat_whose_turn() != critter) {
                    int actionPointsMultiplier = 1;
                    int hthEvadeBonus = 0;

                    if (critter == obj_dude) {
                        if (perkHasRank(obj_dude, PERK_HTH_EVADE)) {
                            bool hasWeapon = false;

                            Object* item2 = inven_right_hand(obj_dude);
                            if (item2 != NULL) {
                                if (item_get_type(item2) == ITEM_TYPE_WEAPON) {
                                    if (item_w_anim_code(item2) != WEAPON_ANIMATION_NONE) {
                                        hasWeapon = true;
                                    }
                                }
                            }

                            if (!hasWeapon) {
                                Object* item1 = inven_left_hand(obj_dude);
                                if (item1 != NULL) {
                                    if (item_get_type(item1) == ITEM_TYPE_WEAPON) {
                                        if (item_w_anim_code(item1) != WEAPON_ANIMATION_NONE) {
                                            hasWeapon = true;
                                        }
                                    }
                                }
                            }

                            if (!hasWeapon) {
                                actionPointsMultiplier = 2;
                                hthEvadeBonus = skill_level(obj_dude, SKILL_UNARMED) / 12;
                            }
                        }
                    }
                    value += hthEvadeBonus;
                    value += critter->data.critter.combat.ap * actionPointsMultiplier;
                }
            }
            break;
        case STAT_AGE:
            value += game_time() / GAME_TIME_TICKS_PER_YEAR;
            break;
        }

        if (critter == obj_dude) {
            switch (stat) {
            case STAT_STRENGTH:
                if (perk_level(critter, PERK_GAIN_STRENGTH)) {
                    value++;
                }

                if (perk_level(critter, PERK_ADRENALINE_RUSH)) {
                    if (critterGetStat(critter, STAT_CURRENT_HIT_POINTS) < (critterGetStat(critter, STAT_MAXIMUM_HIT_POINTS) / 2)) {
                        value++;
                    }
                }
                break;
            case STAT_PERCEPTION:
                if (perk_level(critter, PERK_GAIN_PERCEPTION)) {
                    value++;
                }
                break;
            case STAT_ENDURANCE:
                if (perk_level(critter, PERK_GAIN_ENDURANCE)) {
                    value++;
                }
                break;
            case STAT_CHARISMA:
                if (1) {
                    if (perk_level(critter, PERK_GAIN_CHARISMA)) {
                        value++;
                    }

                    bool hasMirrorShades = false;

                    Object* item2 = inven_right_hand(critter);
                    if (item2 != NULL && item2->pid == PROTO_ID_MIRRORED_SHADES) {
                        hasMirrorShades = true;
                    }

                    Object* item1 = inven_left_hand(critter);
                    if (item1 != NULL && item1->pid == PROTO_ID_MIRRORED_SHADES) {
                        hasMirrorShades = true;
                    }

                    if (hasMirrorShades) {
                        value++;
                    }
                }
                break;
            case STAT_INTELLIGENCE:
                if (perk_level(critter, PERK_GAIN_INTELLIGENCE)) {
                    value++;
                }
                break;
            case STAT_AGILITY:
                if (perk_level(critter, PERK_GAIN_AGILITY)) {
                    value++;
                }
                break;
            case STAT_LUCK:
                if (perk_level(critter, PERK_GAIN_LUCK)) {
                    value++;
                }
                break;
            case STAT_MAXIMUM_HIT_POINTS:
                if (perk_level(critter, PERK_ALCOHOL_RAISED_HIT_POINTS)) {
                    value += 2;
                }

                if (perk_level(critter, PERK_ALCOHOL_RAISED_HIT_POINTS_II)) {
                    value += 4;
                }

                if (perk_level(critter, PERK_ALCOHOL_LOWERED_HIT_POINTS)) {
                    value -= 2;
                }

                if (perk_level(critter, PERK_ALCOHOL_LOWERED_HIT_POINTS_II)) {
                    value -= 4;
                }

                if (perk_level(critter, PERK_AUTODOC_RAISED_HIT_POINTS)) {
                    value += 2;
                }

                if (perk_level(critter, PERK_AUTODOC_RAISED_HIT_POINTS_II)) {
                    value += 4;
                }

                if (perk_level(critter, PERK_AUTODOC_LOWERED_HIT_POINTS)) {
                    value -= 2;
                }

                if (perk_level(critter, PERK_AUTODOC_LOWERED_HIT_POINTS_II)) {
                    value -= 4;
                }
                break;
            case STAT_DAMAGE_RESISTANCE:
            case STAT_DAMAGE_RESISTANCE_EXPLOSION:
                if (perk_level(critter, PERK_DERMAL_IMPACT_ARMOR)) {
                    value += 5;
                } else if (perk_level(critter, PERK_DERMAL_IMPACT_ASSAULT_ENHANCEMENT)) {
                    value += 10;
                }
                break;
            case STAT_DAMAGE_RESISTANCE_LASER:
            case STAT_DAMAGE_RESISTANCE_FIRE:
            case STAT_DAMAGE_RESISTANCE_PLASMA:
                if (perk_level(critter, PERK_PHOENIX_ARMOR_IMPLANTS)) {
                    value += 5;
                } else if (perk_level(critter, PERK_PHOENIX_ASSAULT_ENHANCEMENT)) {
                    value += 10;
                }
                break;
            case STAT_RADIATION_RESISTANCE:
            case STAT_POISON_RESISTANCE:
                if (perk_level(critter, PERK_VAULT_CITY_INOCULATIONS)) {
                    value += 10;
                }
                break;
            }
        }

        value = min(max(value, stat_data[stat].minimumValue), stat_data[stat].maximumValue);
    } else {
        switch (stat) {
        case STAT_CURRENT_HIT_POINTS:
            value = critter_get_hits(critter);
            break;
        case STAT_CURRENT_POISON_LEVEL:
            value = critter_get_poison(critter);
            break;
        case STAT_CURRENT_RADIATION_LEVEL:
            value = critter_get_rads(critter);
            break;
        default:
            value = 0;
            break;
        }
    }

    return value;
}

// Returns base stat value (accounting for traits if critter is dude).
//
// 0x4AF3E0
int stat_get_base(Object* critter, int stat)
{
    int value = stat_get_base_direct(critter, stat);

    if (critter == obj_dude) {
        value += trait_adjust_stat(stat);
    }

    return value;
}

// 0x4AF408
int stat_get_base_direct(Object* critter, int stat)
{
    Proto* proto;

    if (stat >= 0 && stat < SAVEABLE_STAT_COUNT) {
        proto_ptr(critter->pid, &proto);
        return proto->critter.data.baseStats[stat];
    } else {
        switch (stat) {
        case STAT_CURRENT_HIT_POINTS:
            return critter_get_hits(critter);
        case STAT_CURRENT_POISON_LEVEL:
            return critter_get_poison(critter);
        case STAT_CURRENT_RADIATION_LEVEL:
            return critter_get_rads(critter);
        }
    }

    return 0;
}

// 0x4AF474
int stat_get_bonus(Object* critter, int stat)
{
    if (stat >= 0 && stat < SAVEABLE_STAT_COUNT) {
        Proto* proto;
        proto_ptr(critter->pid, &proto);
        return proto->critter.data.bonusStats[stat];
    }

    return 0;
}

// 0x4AF4BC
int stat_set_base(Object* critter, int stat, int value)
{
    Proto* proto;

    if (!statIsValid(stat)) {
        return -5;
    }

    if (stat >= 0 && stat < SAVEABLE_STAT_COUNT) {
        if (stat > STAT_LUCK && stat <= STAT_POISON_RESISTANCE) {
            // Cannot change base value of derived stats.
            return -1;
        }

        if (critter == obj_dude) {
            value -= trait_adjust_stat(stat);
        }

        if (value < stat_data[stat].minimumValue) {
            return -2;
        }

        if (value > stat_data[stat].maximumValue) {
            return -3;
        }

        proto_ptr(critter->pid, &proto);
        proto->critter.data.baseStats[stat] = value;

        if (stat >= STAT_STRENGTH && stat <= STAT_LUCK) {
            stat_recalc_derived(critter);
        }

        return 0;
    }

    switch (stat) {
    case STAT_CURRENT_HIT_POINTS:
        return critter_adjust_hits(critter, value - critter_get_hits(critter));
    case STAT_CURRENT_POISON_LEVEL:
        return critter_adjust_poison(critter, value - critter_get_poison(critter));
    case STAT_CURRENT_RADIATION_LEVEL:
        return critter_adjust_rads(critter, value - critter_get_rads(critter));
    }

    // Should be unreachable
    return 0;
}

// 0x4AF5D4
int inc_stat(Object* critter, int stat)
{
    int value = stat_get_base_direct(critter, stat);

    if (critter == obj_dude) {
        value += trait_adjust_stat(stat);
    }

    return stat_set_base(critter, stat, value + 1);
}

// 0x4AF608
int dec_stat(Object* critter, int stat)
{
    int value = stat_get_base_direct(critter, stat);

    if (critter == obj_dude) {
        value += trait_adjust_stat(stat);
    }

    return stat_set_base(critter, stat, value - 1);
}

// 0x4AF63C
int stat_set_bonus(Object* critter, int stat, int value)
{
    if (!statIsValid(stat)) {
        return -5;
    }

    if (stat >= 0 && stat < SAVEABLE_STAT_COUNT) {
        Proto* proto;
        proto_ptr(critter->pid, &proto);
        proto->critter.data.bonusStats[stat] = value;

        if (stat >= STAT_STRENGTH && stat <= STAT_LUCK) {
            stat_recalc_derived(critter);
        }

        return 0;
    } else {
        switch (stat) {
        case STAT_CURRENT_HIT_POINTS:
            return critter_adjust_hits(critter, value);
        case STAT_CURRENT_POISON_LEVEL:
            return critter_adjust_poison(critter, value);
        case STAT_CURRENT_RADIATION_LEVEL:
            return critter_adjust_rads(critter, value);
        }
    }

    // Should be unreachable
    return -1;
}

// 0x4AF6CC
void stat_set_defaults(CritterProtoData* data)
{
    for (int stat = 0; stat < SAVEABLE_STAT_COUNT; stat++) {
        data->baseStats[stat] = stat_data[stat].defaultValue;
        data->bonusStats[stat] = 0;
    }
}

// 0x4AF6FC
void stat_recalc_derived(Object* critter)
{
    int strength = critterGetStat(critter, STAT_STRENGTH);
    int perception = critterGetStat(critter, STAT_PERCEPTION);
    int endurance = critterGetStat(critter, STAT_ENDURANCE);
    int intelligence = critterGetStat(critter, STAT_INTELLIGENCE);
    int agility = critterGetStat(critter, STAT_AGILITY);
    int luck = critterGetStat(critter, STAT_LUCK);

    Proto* proto;
    proto_ptr(critter->pid, &proto);
    CritterProtoData* data = &(proto->critter.data);

    data->baseStats[STAT_MAXIMUM_HIT_POINTS] = stat_get_base(critter, STAT_STRENGTH) + stat_get_base(critter, STAT_ENDURANCE) * 2 + 15;
    data->baseStats[STAT_MAXIMUM_ACTION_POINTS] = agility / 2 + 5;
    data->baseStats[STAT_ARMOR_CLASS] = agility;
    data->baseStats[STAT_MELEE_DAMAGE] = max(strength - 5, 1);
    data->baseStats[STAT_CARRY_WEIGHT] = 25 * strength + 25;
    data->baseStats[STAT_SEQUENCE] = 2 * perception;
    data->baseStats[STAT_HEALING_RATE] = max(endurance / 3, 1);
    data->baseStats[STAT_CRITICAL_CHANCE] = luck;
    data->baseStats[STAT_BETTER_CRITICALS] = 0;
    data->baseStats[STAT_RADIATION_RESISTANCE] = 2 * endurance;
    data->baseStats[STAT_POISON_RESISTANCE] = 5 * endurance;
}

// 0x4AF854
char* stat_name(int stat)
{
    return statIsValid(stat) ? stat_data[stat].name : NULL;
}

// 0x4AF898
char* stat_description(int stat)
{
    return statIsValid(stat) ? stat_data[stat].description : NULL;
}

// 0x4AF8DC
char* stat_level_description(int value)
{
    if (value < PRIMARY_STAT_MIN) {
        value = PRIMARY_STAT_MIN;
    } else if (value > PRIMARY_STAT_MAX) {
        value = PRIMARY_STAT_MAX;
    }

    return level_description[value - PRIMARY_STAT_MIN];
}

// 0x4AF8FC
int stat_pc_get(int pcStat)
{
    return pcStatIsValid(pcStat) ? curr_pc_stat[pcStat] : 0;
}

// 0x4AF910
int stat_pc_set(int pcStat, int value)
{
    int result;

    if (!pcStatIsValid(pcStat)) {
        return -5;
    }

    if (value < pc_stat_data[pcStat].minimumValue) {
        return -2;
    }

    if (value > pc_stat_data[pcStat].maximumValue) {
        return -3;
    }

    if (pcStat != PC_STAT_EXPERIENCE || value >= curr_pc_stat[PC_STAT_EXPERIENCE]) {
        curr_pc_stat[pcStat] = value;
        if (pcStat == PC_STAT_EXPERIENCE) {
            result = statPCAddExperienceCheckPMs(0, true);
        } else {
            result = 0;
        }
    } else {
        result = statPcResetExperience(value);
    }

    return result;
}

// Reset stats.
//
// 0x4AF980
void stat_pc_set_defaults()
{
    for (int pcStat = 0; pcStat < PC_STAT_COUNT; pcStat++) {
        curr_pc_stat[pcStat] = pc_stat_data[pcStat].defaultValue;
    }
}

// Returns experience to reach next level.
//
// 0x4AF9A0
int stat_pc_min_exp()
{
    return statPcMinExpForLevel(curr_pc_stat[PC_STAT_LEVEL] + 1);
}

// Returns exp to reach given level.
//
// 0x4AF9A8
int statPcMinExpForLevel(int level)
{
    if (level >= PC_LEVEL_MAX) {
        return -1;
    }

    int v1 = level / 2;
    if ((level & 1) != 0) {
        return 1000 * v1 * level;
    } else {
        return 1000 * v1 * (level - 1);
    }
}

// 0x4AF9F4
char* stat_pc_name(int pcStat)
{
    return pcStat >= 0 && pcStat < PC_STAT_COUNT ? pc_stat_data[pcStat].name : NULL;
}

// 0x4AFA14
char* stat_pc_description(int pcStat)
{
    return pcStat >= 0 && pcStat < PC_STAT_COUNT ? pc_stat_data[pcStat].description : NULL;
}

// 0x4AFA34
int stat_picture(int stat)
{
    return statIsValid(stat) ? stat_data[stat].frmId : 0;
}

// Roll D10 against specified stat.
//
// This function is intended to be used with one of SPECIAL stats (which are
// capped at 10, hence d10), not with artitrary stat, but does not enforce it.
//
// An optional [modifier] can be supplied as a bonus (or penalty) to the stat's
// value.
//
// Upon return [howMuch] will be set to difference between stat's value
// (accounting for given [modifier]) and d10 roll, which can be positive (or
// zero) when roll succeeds, or negative when roll fails. Set [howMuch] to
// `NULL` if you're not interested in this value.
//
// 0x4AFA78
int stat_result(Object* critter, int stat, int modifier, int* howMuch)
{
    int value = critterGetStat(critter, stat) + modifier;
    int chance = roll_random(PRIMARY_STAT_MIN, PRIMARY_STAT_MAX);

    if (howMuch != NULL) {
        *howMuch = value - chance;
    }

    if (chance <= value) {
        return ROLL_SUCCESS;
    }

    return ROLL_FAILURE;
}

// 0x4AFAA8
int stat_pc_add_experience(int xp)
{
    return statPCAddExperienceCheckPMs(xp, true);
}

// 0x4AFAB8
int statPCAddExperienceCheckPMs(int xp, bool a2)
{
    int newXp = curr_pc_stat[PC_STAT_EXPERIENCE];
    newXp += xp;
    newXp += perk_level(obj_dude, PERK_SWIFT_LEARNER) * 5 * xp / 100;

    if (newXp < pc_stat_data[PC_STAT_EXPERIENCE].minimumValue) {
        newXp = pc_stat_data[PC_STAT_EXPERIENCE].minimumValue;
    }

    if (newXp > pc_stat_data[PC_STAT_EXPERIENCE].maximumValue) {
        newXp = pc_stat_data[PC_STAT_EXPERIENCE].maximumValue;
    }

    curr_pc_stat[PC_STAT_EXPERIENCE] = newXp;

    while (curr_pc_stat[PC_STAT_LEVEL] < PC_LEVEL_MAX) {
        if (newXp < stat_pc_min_exp()) {
            break;
        }

        if (stat_pc_set(PC_STAT_LEVEL, curr_pc_stat[PC_STAT_LEVEL] + 1) == 0) {
            int maxHpBefore = critterGetStat(obj_dude, STAT_MAXIMUM_HIT_POINTS);

            // You have gone up a level.
            MessageListItem messageListItem;
            messageListItem.num = 600;
            if (message_search(&stat_message_file, &messageListItem)) {
                display_print(messageListItem.text);
            }

            pc_flag_on(DUDE_STATE_LEVEL_UP_AVAILABLE);

            gsound_play_sfx_file("levelup");

            // NOTE: Uninline.
            int endurance = stat_get_base(obj_dude, STAT_ENDURANCE);

            int hpPerLevel = endurance / 2 + 2;
            hpPerLevel += perk_level(obj_dude, PERK_LIFEGIVER) * 4;

            int bonusHp = stat_get_bonus(obj_dude, STAT_MAXIMUM_HIT_POINTS);
            stat_set_bonus(obj_dude, STAT_MAXIMUM_HIT_POINTS, bonusHp + hpPerLevel);

            int maxHpAfter = critterGetStat(obj_dude, STAT_MAXIMUM_HIT_POINTS);
            critter_adjust_hits(obj_dude, maxHpAfter - maxHpBefore);

            intface_update_hit_points(false);

            if (a2) {
                partyMemberIncLevels();
            }
        }
    }

    return 0;
}

// 0x4AFC38
int statPcResetExperience(int xp)
{
    int oldLevel = curr_pc_stat[PC_STAT_LEVEL];
    curr_pc_stat[PC_STAT_EXPERIENCE] = xp;

    int level = 1;
    do {
        level += 1;
    } while (xp >= statPcMinExpForLevel(level) && level < PC_LEVEL_MAX);

    int newLevel = level - 1;

    stat_pc_set(PC_STAT_LEVEL, newLevel);
    pc_flag_off(DUDE_STATE_LEVEL_UP_AVAILABLE);

    // NOTE: Uninline.
    int endurance = stat_get_base(obj_dude, STAT_ENDURANCE);

    int hpPerLevel = endurance / 2 + 2;
    hpPerLevel += perk_level(obj_dude, PERK_LIFEGIVER) * 4;

    int deltaHp = (oldLevel - newLevel) * hpPerLevel;
    critter_adjust_hits(obj_dude, -deltaHp);

    int bonusHp = stat_get_bonus(obj_dude, STAT_MAXIMUM_HIT_POINTS);

    stat_set_bonus(obj_dude, STAT_MAXIMUM_HIT_POINTS, bonusHp - deltaHp);

    intface_update_hit_points(false);

    return 0;
}
