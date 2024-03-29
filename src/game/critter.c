#include "game/critter.h"

#include <stdio.h>
#include <string.h>

#include "game/anim.h"
#include "game/editor.h"
#include "game/combat.h"
#include "plib/gnw/debug.h"
#include "game/display.h"
#include "game/endgame.h"
#include "game/game.h"
#include "plib/gnw/rect.h"
#include "game/intface.h"
#include "game/item.h"
#include "game/map.h"
#include "plib/gnw/memory.h"
#include "game/message.h"
#include "game/object.h"
#include "game/party.h"
#include "game/proto.h"
#include "game/queue.h"
#include "game/roll.h"
#include "game/reaction.h"
#include "game/scripts.h"
#include "game/skill.h"
#include "game/stat.h"
#include "game/tile.h"
#include "game/trait.h"
#include "game/worldmap.h"

static int get_rad_damage_level(Object* obj, void* data);
static int clear_rad_damage(Object* obj, void* data);
static void process_rads(Object* obj, int radiationLevel, bool direction);
static int critter_kill_count_clear();
static int critterClearObjDrugs(Object* obj, void* data);

// TODO: Remove.
// 0x50141C
char _aCorpse[] = "corpse";

// TODO: Remove.
// 0x501494
char byte_501494[] = "";

// List of stats affected by radiation.
//
// The values of this list specify stats that can be affected by radiation.
// The amount of penalty to every stat (identified by index) is stored
// separately in [rad_bonus] per radiation level.
//
// The order of stats is important - primary stats must be at the top. See
// [RADIATION_EFFECT_PRIMARY_STAT_COUNT] for more info.
//
// 0x518358
int rad_stat[RADIATION_EFFECT_COUNT] = {
    STAT_STRENGTH,
    STAT_PERCEPTION,
    STAT_ENDURANCE,
    STAT_CHARISMA,
    STAT_INTELLIGENCE,
    STAT_AGILITY,
    STAT_CURRENT_HIT_POINTS,
    STAT_HEALING_RATE,
};

// Denotes how many primary stats at the top of [rad_stat] array.
// These stats are used to determine if critter is alive after applying
// radiation effects.
#define RADIATION_EFFECT_PRIMARY_STAT_COUNT 6

// List of stat modifiers caused by radiation at different radiation levels.
//
// 0x518378
int rad_bonus[RADIATION_LEVEL_COUNT][RADIATION_EFFECT_COUNT] = {
    // clang-format off
    {   0,   0,   0,   0,   0,   0,   0,   0 },
    {  -1,   0,   0,   0,   0,   0,   0,   0 },
    {  -1,   0,   0,   0,   0,  -1,   0,  -3 },
    {  -2,   0,  -1,   0,   0,  -2,  -5,  -5 },
    {  -4,  -3,  -3,  -3,  -1,  -5, -15, -10 },
    {  -6,  -5,  -5,  -5,  -3,  -6, -20, -10 },
    // clang-format on
};

// 0x518438
static Object* critterClearObj = NULL;

// scrname.msg
//
// 0x56D754
static MessageList critter_scrmsg_file;

// 0x56D75C
static char pc_name[DUDE_NAME_MAX_LENGTH];

// 0x56D77C
static int sneak_working;

// 0x56D780
static int pc_kill_counts[KILL_TYPE_COUNT];

// Something with radiation.
//
// 0x56D7CC
static int old_rad_level;

// scrname_init
// 0x42CF50
int critter_init()
{
    critter_pc_reset_name();

    // NOTE: Uninline.
    critter_kill_count_clear();

    if (!message_init(&critter_scrmsg_file)) {
        debug_printf("\nError: Initing critter name message file!");
        return -1;
    }

    char path[MAX_PATH];
    sprintf(path, "%sscrname.msg", msg_path);

    if (!message_load(&critter_scrmsg_file, path)) {
        debug_printf("\nError: Loading critter name message file!");
        return -1;
    }

    return 0;
}

// 0x42CFE4
void critter_reset()
{
    critter_pc_reset_name();

    // NOTE: Uninline;
    critter_kill_count_clear();
}

// 0x42D004
void critter_exit()
{
    message_exit(&critter_scrmsg_file);
}

// 0x42D01C
int critter_load(File* stream)
{
    if (db_freadInt(stream, &sneak_working) == -1) {
        return -1;
    }

    Proto* proto;
    proto_ptr(obj_dude->pid, &proto);

    return critter_read_data(stream, &(proto->critter.data));
}

// 0x42D058
int critter_save(File* stream)
{
    if (db_fwriteInt(stream, sneak_working) == -1) {
        return -1;
    }

    Proto* proto;
    proto_ptr(obj_dude->pid, &proto);

    return critter_write_data(stream, &(proto->critter.data));
}

// 0x42D094
void critter_copy(CritterProtoData* dest, CritterProtoData* src)
{
    memcpy(dest, src, sizeof(CritterProtoData));
}

// 0x42D0A8
char* critter_name(Object* obj)
{
    // TODO: Rename.
    // 0x51833C
    static char* _name_critter = _aCorpse;

    if (obj == obj_dude) {
        return pc_name;
    }

    if (obj->field_80 == -1) {
        if (obj->sid != -1) {
            Script* script;
            if (scr_ptr(obj->sid, &script) != -1) {
                obj->field_80 = script->field_14;
            }
        }
    }

    char* name = NULL;
    if (obj->field_80 != -1) {
        MessageListItem messageListItem;
        messageListItem.num = 101 + obj->field_80;
        if (message_search(&critter_scrmsg_file, &messageListItem)) {
            name = messageListItem.text;
        }
    }

    if (name == NULL || *name == '\0') {
        name = proto_name(obj->pid);
    }

    _name_critter = name;

    return name;
}

// 0x42D138
int critter_pc_set_name(const char* name)
{
    if (strlen(name) <= DUDE_NAME_MAX_LENGTH) {
        strncpy(pc_name, name, DUDE_NAME_MAX_LENGTH);
        return 0;
    }

    return -1;
}

// 0x42D170
void critter_pc_reset_name()
{
    strncpy(pc_name, "None", DUDE_NAME_MAX_LENGTH);
}

// 0x42D18C
int critter_get_hits(Object* critter)
{
    return PID_TYPE(critter->pid) == OBJ_TYPE_CRITTER ? critter->data.critter.hp : 0;
}

// 0x42D1A4
int critter_adjust_hits(Object* critter, int hp)
{
    if (PID_TYPE(critter->pid) != OBJ_TYPE_CRITTER) {
        return 0;
    }

    int maximumHp = critterGetStat(critter, STAT_MAXIMUM_HIT_POINTS);
    int newHp = critter->data.critter.hp + hp;

    critter->data.critter.hp = newHp;
    if (maximumHp >= newHp) {
        if (newHp <= 0 && (critter->data.critter.combat.results & DAM_DEAD) == 0) {
            critter_kill(critter, -1, true);
        }
    } else {
        critter->data.critter.hp = maximumHp;
    }

    return 0;
}

// 0x42D1F8
int critter_get_poison(Object* critter)
{
    return PID_TYPE(critter->pid) == OBJ_TYPE_CRITTER ? critter->data.critter.poison : 0;
}

// Adjust critter's current poison by specified amount.
//
// For unknown reason this function only works on dude.
//
// The [amount] can either be positive (adds poison) or negative (removes
// poison).
//
// 0x42D210
int critter_adjust_poison(Object* critter, int amount)
{
    MessageListItem messageListItem;

    if (critter != obj_dude) {
        return -1;
    }

    if (amount > 0) {
        // Take poison resistance into account.
        amount -= amount * critterGetStat(critter, STAT_POISON_RESISTANCE) / 100;
    } else {
        if (obj_dude->data.critter.poison <= 0) {
            // Critter is not poisoned and we're want to decrease it even
            // further, which makes no sense.
            return 0;
        }
    }

    int newPoison = critter->data.critter.poison + amount;
    if (newPoison > 0) {
        critter->data.critter.poison = newPoison;

        queue_clear_type(EVENT_TYPE_POISON, NULL);
        queue_add(10 * (505 - 5 * newPoison), obj_dude, NULL, EVENT_TYPE_POISON);

        // You have been poisoned!
        messageListItem.num = 3000;
        if (amount < 0) {
            // You feel a little better.
            messageListItem.num = 3002;
        }
    } else {
        critter->data.critter.poison = 0;

        // You feel better.
        messageListItem.num = 3003;
    }

    if (message_search(&misc_message_file, &messageListItem)) {
        display_print(messageListItem.text);
    }

    if (critter == obj_dude) {
        refresh_box_bar_win();
    }

    return 0;
}

// 0x42D318
int critter_check_poison(Object* obj, void* data)
{
    if (obj != obj_dude) {
        return 0;
    }

    critter_adjust_poison(obj, -2);
    critter_adjust_hits(obj, -1);

    intface_update_hit_points(false);

    MessageListItem messageListItem;
    // You take damage from poison.
    messageListItem.num = 3001;
    if (message_search(&misc_message_file, &messageListItem)) {
        display_print(messageListItem.text);
    }

    // NOTE: Uninline.
    int hitPoints = critter_get_hits(obj);
    if (hitPoints > 5) {
        return 0;
    }

    return 1;
}

// 0x42D38C
int critter_get_rads(Object* obj)
{
    return PID_TYPE(obj->pid) == OBJ_TYPE_CRITTER ? obj->data.critter.radiation : 0;
}

// 0x42D3A4
int critter_adjust_rads(Object* obj, int amount)
{
    MessageListItem messageListItem;

    if (obj != obj_dude) {
        return -1;
    }

    Proto* proto;
    proto_ptr(obj_dude->pid, &proto);

    if (amount > 0) {
        amount -= critterGetStat(obj, STAT_RADIATION_RESISTANCE) * amount / 100;
    }

    if (amount > 0) {
        proto->critter.data.flags |= CRITTER_BARTER;
    }

    if (amount > 0) {
        Object* geigerCounter = NULL;

        Object* item1 = inven_left_hand(obj_dude);
        if (item1 != NULL) {
            if (item1->pid == PROTO_ID_GEIGER_COUNTER_I || item1->pid == PROTO_ID_GEIGER_COUNTER_II) {
                geigerCounter = item1;
            }
        }

        Object* item2 = inven_right_hand(obj_dude);
        if (item2 != NULL) {
            if (item2->pid == PROTO_ID_GEIGER_COUNTER_I || item2->pid == PROTO_ID_GEIGER_COUNTER_II) {
                geigerCounter = item2;
            }
        }

        if (geigerCounter != NULL) {
            if (item_m_on(geigerCounter)) {
                if (amount > 5) {
                    // The geiger counter is clicking wildly.
                    messageListItem.num = 1009;
                } else {
                    // The geiger counter is clicking.
                    messageListItem.num = 1008;
                }

                if (message_search(&misc_message_file, &messageListItem)) {
                    display_print(messageListItem.text);
                }
            }
        }
    }

    if (amount >= 10) {
        // You have received a large dose of radiation.
        messageListItem.num = 1007;

        if (message_search(&misc_message_file, &messageListItem)) {
            display_print(messageListItem.text);
        }
    }

    obj->data.critter.radiation += amount;
    if (obj->data.critter.radiation < 0) {
        obj->data.critter.radiation = 0;
    }

    if (obj == obj_dude) {
        refresh_box_bar_win();
    }

    return 0;
}

// 0x42D4F4
int critter_check_rads(Object* obj)
{
    // Modifiers to endurance for performing radiation damage check.
    //
    // 0x518340
    static int bonus[RADIATION_LEVEL_COUNT] = {
        2,
        0,
        -2,
        -4,
        -6,
        -8,
    };

    if (obj != obj_dude) {
        return 0;
    }

    Proto* proto;
    proto_ptr(obj->pid, &proto);
    if ((proto->critter.data.flags & CRITTER_BARTER) == 0) {
        return 0;
    }

    old_rad_level = 0;

    queue_clear_type(EVENT_TYPE_RADIATION, get_rad_damage_level);

    // NOTE: Uninline
    int radiation = critter_get_rads(obj);

    int radiationLevel;
    if (radiation > 999)
        radiationLevel = RADIATION_LEVEL_FATAL;
    else if (radiation > 599)
        radiationLevel = RADIATION_LEVEL_DEADLY;
    else if (radiation > 399)
        radiationLevel = RADIATION_LEVEL_CRITICAL;
    else if (radiation > 199)
        radiationLevel = RADIATION_LEVEL_ADVANCED;
    else if (radiation > 99)
        radiationLevel = RADIATION_LEVEL_MINOR;
    else
        radiationLevel = RADIATION_LEVEL_NONE;

    if (stat_result(obj, STAT_ENDURANCE, bonus[radiationLevel], NULL) <= ROLL_FAILURE) {
        radiationLevel++;
    }

    if (radiationLevel > old_rad_level) {
        // Create timer event for applying radiation damage.
        RadiationEvent* radiationEvent = (RadiationEvent*)mem_malloc(sizeof(*radiationEvent));
        if (radiationEvent == NULL) {
            return 0;
        }

        radiationEvent->radiationLevel = radiationLevel;
        radiationEvent->isHealing = 0;
        queue_add(GAME_TIME_TICKS_PER_HOUR * roll_random(4, 18), obj, radiationEvent, EVENT_TYPE_RADIATION);
    }

    proto->critter.data.flags &= ~(CRITTER_BARTER);

    return 0;
}

// 0x42D618
static int get_rad_damage_level(Object* obj, void* data)
{
    RadiationEvent* radiationEvent = (RadiationEvent*)data;

    old_rad_level = radiationEvent->radiationLevel;

    return 0;
}

// 0x42D624
static int clear_rad_damage(Object* obj, void* data)
{
    RadiationEvent* radiationEvent = (RadiationEvent*)data;

    if (radiationEvent->isHealing) {
        process_rads(obj, radiationEvent->radiationLevel, true);
    }

    return 1;
}

// Applies radiation.
//
// 0x42D63C
static void process_rads(Object* obj, int radiationLevel, bool isHealing)
{
    MessageListItem messageListItem;

    if (radiationLevel == RADIATION_LEVEL_NONE) {
        return;
    }

    int radiationLevelIndex = radiationLevel - 1;
    int modifier = isHealing ? -1 : 1;

    if (obj == obj_dude) {
        // Radiation level message, higher is worse.
        messageListItem.num = 1000 + radiationLevelIndex;
        if (message_search(&misc_message_file, &messageListItem)) {
            display_print(messageListItem.text);
        }
    }

    for (int effect = 0; effect < RADIATION_EFFECT_COUNT; effect++) {
        int value = stat_get_bonus(obj, rad_stat[effect]);
        value += modifier * rad_bonus[radiationLevelIndex][effect];
        stat_set_bonus(obj, rad_stat[effect], value);
    }

    if ((obj->data.critter.combat.results & DAM_DEAD) == 0) {
        // Loop thru effects affecting primary stats. If any of the primary stat
        // dropped below minimal value, kill it.
        for (int effect = 0; effect < RADIATION_EFFECT_PRIMARY_STAT_COUNT; effect++) {
            int base = stat_get_base(obj, rad_stat[effect]);
            int bonus = stat_get_bonus(obj, rad_stat[effect]);
            if (base + bonus < PRIMARY_STAT_MIN) {
                critter_kill(obj, -1, 1);
                break;
            }
        }
    }

    if ((obj->data.critter.combat.results & DAM_DEAD) != 0) {
        if (obj == obj_dude) {
            // You have died from radiation sickness.
            messageListItem.num = 1006;
            if (message_search(&misc_message_file, &messageListItem)) {
                display_print(messageListItem.text);
            }
        }
    }
}

// 0x42D740
int critter_process_rads(Object* obj, void* data)
{
    RadiationEvent* radiationEvent = (RadiationEvent*)data;
    if (!radiationEvent->isHealing) {
        // Schedule healing stats event in 7 days.
        RadiationEvent* newRadiationEvent = (RadiationEvent*)mem_malloc(sizeof(*newRadiationEvent));
        if (newRadiationEvent != NULL) {
            queue_clear_type(EVENT_TYPE_RADIATION, clear_rad_damage);
            newRadiationEvent->radiationLevel = radiationEvent->radiationLevel;
            newRadiationEvent->isHealing = 1;
            queue_add(GAME_TIME_TICKS_PER_DAY * 7, obj, newRadiationEvent, EVENT_TYPE_RADIATION);
        }
    }

    process_rads(obj, radiationEvent->radiationLevel, radiationEvent->isHealing);

    return 1;
}

// 0x42D7A0
int critter_load_rads(File* stream, void** dataPtr)
{
    RadiationEvent* radiationEvent = (RadiationEvent*)mem_malloc(sizeof(*radiationEvent));
    if (radiationEvent == NULL) {
        return -1;
    }

    if (db_freadInt(stream, &(radiationEvent->radiationLevel)) == -1) goto err;
    if (db_freadInt(stream, &(radiationEvent->isHealing)) == -1) goto err;

    *dataPtr = radiationEvent;
    return 0;

err:

    mem_free(radiationEvent);
    return -1;
}

// 0x42D7FC
int critter_save_rads(File* stream, void* data)
{
    RadiationEvent* radiationEvent = (RadiationEvent*)data;

    if (db_fwriteInt(stream, radiationEvent->radiationLevel) == -1) return -1;
    if (db_fwriteInt(stream, radiationEvent->isHealing) == -1) return -1;

    return 0;
}

// 0x42D82C
int critter_get_base_damage_type(Object* obj)
{
    if (PID_TYPE(obj->pid) != OBJ_TYPE_CRITTER) {
        return 0;
    }

    Proto* proto;
    if (proto_ptr(obj->pid, &proto) == -1) {
        return 0;
    }

    return proto->critter.data.damageType;
}

// NOTE: Inlined.
//
// 0x42D860
static int critter_kill_count_clear()
{
    memset(pc_kill_counts, 0, sizeof(pc_kill_counts));
    return 0;
}

// 0x42D878
int critter_kill_count_inc(int killType)
{
    if (killType != -1 && killType < KILL_TYPE_COUNT) {
        pc_kill_counts[killType]++;
        return 0;
    }

    return -1;
}

// 0x42D8A8
int critter_kill_count(int killType)
{
    if (killType != -1 && killType < KILL_TYPE_COUNT) {
        return pc_kill_counts[killType];
    }

    return 0;
}

// 0x42D8C0
int critter_kill_count_load(File* stream)
{
    if (db_freadIntCount(stream, pc_kill_counts, KILL_TYPE_COUNT) == -1) {
        db_fclose(stream);
        return -1;
    }

    return 0;
}

// 0x42D8F0
int critter_kill_count_save(File* stream)
{
    if (db_fwriteIntCount(stream, pc_kill_counts, KILL_TYPE_COUNT) == -1) {
        db_fclose(stream);
        return -1;
    }

    return 0;
}

// 0x42D920
int critterGetKillType(Object* obj)
{
    if (obj == obj_dude) {
        int gender = critterGetStat(obj, STAT_GENDER);
        if (gender == GENDER_FEMALE) {
            return KILL_TYPE_WOMAN;
        }
        return KILL_TYPE_MAN;
    }

    if (PID_TYPE(obj->pid) != OBJ_TYPE_CRITTER) {
        return -1;
    }

    Proto* proto;
    proto_ptr(obj->pid, &proto);

    return proto->critter.data.killType;
}

// 0x42D974
char* critter_kill_name(int killType)
{
    if (killType != -1 && killType < KILL_TYPE_COUNT) {
        if (killType >= 0 && killType < KILL_TYPE_COUNT) {
            MessageListItem messageListItem;
            return getmsg(&proto_main_msg_file, &messageListItem, 1450 + killType);
        } else {
            return NULL;
        }
    } else {
        return byte_501494;
    }
}

// 0x42D9B4
char* critter_kill_info(int killType)
{
    if (killType != -1 && killType < KILL_TYPE_COUNT) {
        if (killType >= 0 && killType < KILL_TYPE_COUNT) {
            MessageListItem messageListItem;
            return getmsg(&proto_main_msg_file, &messageListItem, 1469 + killType);
        } else {
            return NULL;
        }
    } else {
        return byte_501494;
    }
}

// 0x42D9F4
int critter_heal_hours(Object* critter, int a2)
{
    if (PID_TYPE(critter->pid) != OBJ_TYPE_CRITTER) {
        return -1;
    }

    if (critter->data.critter.hp < critterGetStat(critter, STAT_MAXIMUM_HIT_POINTS)) {
        critter_adjust_hits(critter, 14 * (a2 / 3));
    }

    critter->data.critter.combat.maneuver &= ~CRITTER_MANUEVER_FLEEING;

    return 0;
}

// 0x42DA54
static int critterClearObjDrugs(Object* obj, void* data)
{
    return obj == critterClearObj;
}

// 0x42DA64
void critter_kill(Object* critter, int anim, bool a3)
{
    if (PID_TYPE(critter->pid) != OBJ_TYPE_CRITTER) {
        return;
    }

    int elevation = critter->elevation;

    partyMemberRemove(critter);

    // NOTE: Original code uses goto to jump out from nested conditions below.
    bool shouldChangeFid = false;
    int fid;
    if (critter_is_prone(critter)) {
        int current = FID_ANIM_TYPE(critter->fid);
        if (current == ANIM_FALL_BACK || current == ANIM_FALL_FRONT) {
            bool back = false;
            if (current == ANIM_FALL_BACK) {
                back = true;
            } else {
                fid = art_id(OBJ_TYPE_CRITTER, critter->fid & 0xFFF, ANIM_FALL_FRONT_SF, (critter->fid & 0xF000) >> 12, critter->rotation + 1);
                if (!art_exists(fid)) {
                    back = true;
                }
            }

            if (back) {
                fid = art_id(OBJ_TYPE_CRITTER, critter->fid & 0xFFF, ANIM_FALL_BACK_SF, (critter->fid & 0xF000) >> 12, critter->rotation + 1);
            }

            shouldChangeFid = true;
        }
    } else {
        if (anim < 0) {
            anim = LAST_SF_DEATH_ANIM;
        }

        if (anim > LAST_SF_DEATH_ANIM) {
            debug_printf("\nError: Critter Kill: death_frame out of range!");
            anim = LAST_SF_DEATH_ANIM;
        }

        fid = art_id(OBJ_TYPE_CRITTER, critter->fid & 0xFFF, anim, (critter->fid & 0xF000) >> 12, critter->rotation + 1);
        obj_fix_violence_settings(&fid);
        if (!art_exists(fid)) {
            debug_printf("\nError: Critter Kill: Can't match fid!");

            fid = art_id(OBJ_TYPE_CRITTER, critter->fid & 0xFFF, ANIM_FALL_BACK_BLOOD_SF, (critter->fid & 0xF000) >> 12, critter->rotation + 1);
            obj_fix_violence_settings(&fid);
        }

        shouldChangeFid = true;
    }

    Rect updatedRect;
    Rect tempRect;

    if (shouldChangeFid) {
        obj_set_frame(critter, 0, &updatedRect);

        obj_change_fid(critter, fid, &tempRect);
        rect_min_bound(&updatedRect, &tempRect, &updatedRect);
    }

    if (!critter_flag_check(critter->pid, CRITTER_FLAT)) {
        critter->flags |= OBJECT_NO_BLOCK;
        obj_toggle_flat(critter, &tempRect);
    }

    // NOTE: using uninitialized updatedRect/tempRect if fid was not set.

    rect_min_bound(&updatedRect, &tempRect, &updatedRect);

    obj_turn_off_light(critter, &tempRect);
    rect_min_bound(&updatedRect, &tempRect, &updatedRect);

    critter->data.critter.hp = 0;
    critter->data.critter.combat.results |= DAM_DEAD;

    if (critter->sid != -1) {
        scr_remove(critter->sid);
        critter->sid = -1;
    }

    critterClearObj = critter;
    queue_clear_type(EVENT_TYPE_DRUG, critterClearObjDrugs);

    item_destroy_all_hidden(critter);

    if (a3) {
        tile_refresh_rect(&updatedRect, elevation);
    }

    if (critter == obj_dude) {
        endgameSetupDeathEnding(ENDGAME_DEATH_ENDING_REASON_DEATH);
        game_user_wants_to_quit = 2;
    }
}

// Returns experience for killing [critter].
//
// 0x42DCB8
int critter_kill_exps(Object* critter)
{
    Proto* proto;
    proto_ptr(critter->pid, &proto);
    return proto->critter.data.experience;
}

// 0x42DCDC
bool critter_is_active(Object* critter)
{
    if (critter == NULL) {
        return false;
    }

    if (PID_TYPE(critter->pid) != OBJ_TYPE_CRITTER) {
        return false;
    }

    if ((critter->data.critter.combat.results & (DAM_KNOCKED_OUT | DAM_DEAD)) != 0) {
        return false;
    }

    if ((critter->data.critter.combat.results & (DAM_KNOCKED_OUT | DAM_DEAD | DAM_LOSE_TURN)) != 0) {
        return false;
    }

    return (critter->data.critter.combat.results & DAM_DEAD) == 0;
}

// 0x42DD18
bool critter_is_dead(Object* critter)
{
    if (critter == NULL) {
        return false;
    }

    if (PID_TYPE(critter->pid) != OBJ_TYPE_CRITTER) {
        return false;
    }

    if (critterGetStat(critter, STAT_CURRENT_HIT_POINTS) <= 0) {
        return true;
    }

    if ((critter->data.critter.combat.results & DAM_DEAD) != 0) {
        return true;
    }

    return false;
}

// 0x42DD58
bool critter_is_crippled(Object* critter)
{
    if (critter == NULL) {
        return false;
    }

    if (PID_TYPE(critter->pid) != OBJ_TYPE_CRITTER) {
        return false;
    }

    return (critter->data.critter.combat.results & DAM_CRIP) != 0;
}

// 0x42DD80
bool critter_is_prone(Object* critter)
{
    if (critter == NULL) {
        return false;
    }

    if (PID_TYPE(critter->pid) != OBJ_TYPE_CRITTER) {
        return false;
    }

    int anim = FID_ANIM_TYPE(critter->fid);

    return (critter->data.critter.combat.results & (DAM_KNOCKED_OUT | DAM_KNOCKED_DOWN)) != 0
        || (anim >= FIRST_KNOCKDOWN_AND_DEATH_ANIM && anim <= LAST_KNOCKDOWN_AND_DEATH_ANIM)
        || (anim >= FIRST_SF_DEATH_ANIM && anim <= LAST_SF_DEATH_ANIM);
}

// critter_body_type
// 0x42DDC4
int critter_body_type(Object* critter)
{
    if (critter == NULL) {
        debug_printf("\nError: critter_body_type: pobj was NULL!");
        return 0;
    }

    if (PID_TYPE(critter->pid) != OBJ_TYPE_CRITTER) {
        return 0;
    }

    Proto* proto;
    proto_ptr(critter->pid, &proto);
    return proto->critter.data.bodyType;
}

// NOTE: Unused.
//
// 0x42DE10
int critter_load_data(CritterProtoData* critterData, const char* path)
{
    File* stream;

    stream = db_fopen(path, "rb");
    if (stream == NULL) {
        return -1;
    }

    if (critter_read_data(stream, critterData) == -1) {
        db_fclose(stream);
        return -1;
    }

    db_fclose(stream);
    return 0;
}

// 0x42DE58
int pc_load_data(const char* path)
{
    File* stream = db_fopen(path, "rb");
    if (stream == NULL) {
        return -1;
    }

    Proto* proto;
    proto_ptr(obj_dude->pid, &proto);

    if (critter_read_data(stream, &(proto->critter.data)) == -1) {
        db_fclose(stream);
        return -1;
    }

    db_fread(pc_name, DUDE_NAME_MAX_LENGTH, 1, stream);

    if (skill_load(stream) == -1) {
        db_fclose(stream);
        return -1;
    }

    if (trait_load(stream) == -1) {
        db_fclose(stream);
        return -1;
    }

    if (db_freadInt(stream, &character_points) == -1) {
        db_fclose(stream);
        return -1;
    }

    proto->critter.data.baseStats[STAT_DAMAGE_RESISTANCE_EMP] = 100;
    proto->critter.data.bodyType = 0;
    proto->critter.data.experience = 0;
    proto->critter.data.killType = 0;

    db_fclose(stream);
    return 0;
}

// 0x42DF70
int critter_read_data(File* stream, CritterProtoData* critterData)
{
    if (db_freadInt(stream, &(critterData->flags)) == -1) return -1;
    if (db_freadIntCount(stream, critterData->baseStats, SAVEABLE_STAT_COUNT) == -1) return -1;
    if (db_freadIntCount(stream, critterData->bonusStats, SAVEABLE_STAT_COUNT) == -1) return -1;
    if (db_freadIntCount(stream, critterData->skills, SKILL_COUNT) == -1) return -1;
    if (db_freadInt(stream, &(critterData->bodyType)) == -1) return -1;
    if (db_freadInt(stream, &(critterData->experience)) == -1) return -1;
    if (db_freadInt(stream, &(critterData->killType)) == -1) return -1;

    // NOTE: For unknown reason damage type is not present in two protos: Sentry
    // Bot and Weak Brahmin. These two protos are 412 bytes, not 416.
    //
    // Given that only Floating Eye Bot, Floater, and Nasty Floater have
    // natural damage type other than normal, I think addition of natural
    // damage type as a feature was a last minute design decision. Most protos
    // were updated, but not all. Another suggestion is that some team member
    // used outdated toolset to build those two protos (mapper or whatever
    // they used to create protos in the first place).
    //
    // Regardless of the reason, damage type is considered optional by original
    // code as seen at 0x42E01B.
    if (db_freadInt(stream, &(critterData->damageType)) == -1) {
        critterData->damageType = DAMAGE_TYPE_NORMAL;
    }

    return 0;
}

// NOTE: Unused.
//
// 0x42E044
int critter_save_data(CritterProtoData* critterData, const char* path)
{
    File* stream;

    stream = db_fopen(path, "wb");
    if (stream == NULL) {
        return -1;
    }

    if (critter_write_data(stream, critterData) == -1) {
        db_fclose(stream);
        return -1;
    }

    db_fclose(stream);
    return 0;
}

// 0x42E08C
int pc_save_data(const char* path)
{
    File* stream = db_fopen(path, "wb");
    if (stream == NULL) {
        return -1;
    }

    Proto* proto;
    proto_ptr(obj_dude->pid, &proto);

    if (critter_write_data(stream, &(proto->critter.data)) == -1) {
        db_fclose(stream);
        return -1;
    }

    db_fwrite(pc_name, DUDE_NAME_MAX_LENGTH, 1, stream);

    if (skill_save(stream) == -1) {
        db_fclose(stream);
        return -1;
    }

    if (trait_save(stream) == -1) {
        db_fclose(stream);
        return -1;
    }

    if (db_fwriteInt(stream, character_points) == -1) {
        db_fclose(stream);
        return -1;
    }

    db_fclose(stream);
    return 0;
}

// 0x42E174
int critter_write_data(File* stream, CritterProtoData* critterData)
{
    if (db_fwriteInt(stream, critterData->flags) == -1) return -1;
    if (db_fwriteIntCount(stream, critterData->baseStats, SAVEABLE_STAT_COUNT) == -1) return -1;
    if (db_fwriteIntCount(stream, critterData->bonusStats, SAVEABLE_STAT_COUNT) == -1) return -1;
    if (db_fwriteIntCount(stream, critterData->skills, SKILL_COUNT) == -1) return -1;
    if (db_fwriteInt(stream, critterData->bodyType) == -1) return -1;
    if (db_fwriteInt(stream, critterData->experience) == -1) return -1;
    if (db_fwriteInt(stream, critterData->killType) == -1) return -1;
    if (db_fwriteInt(stream, critterData->damageType) == -1) return -1;

    return 0;
}

// 0x42E220
void pc_flag_off(int state)
{
    Proto* proto;
    proto_ptr(obj_dude->pid, &proto);

    proto->critter.data.flags &= ~(1 << state);

    if (state == DUDE_STATE_SNEAKING) {
        queue_remove_this(obj_dude, EVENT_TYPE_SNEAK);
    }

    refresh_box_bar_win();
}

// 0x42E26C
void pc_flag_on(int state)
{
    Proto* proto;
    proto_ptr(obj_dude->pid, &proto);

    proto->critter.data.flags |= (1 << state);

    if (state == DUDE_STATE_SNEAKING) {
        critter_sneak_check(NULL, NULL);
    }

    refresh_box_bar_win();
}

// 0x42E2B0
void pc_flag_toggle(int state)
{
    // NOTE: Uninline.
    if (is_pc_flag(state)) {
        pc_flag_off(state);
    } else {
        pc_flag_on(state);
    }
}

// 0x42E2F8
bool is_pc_flag(int state)
{
    Proto* proto;
    proto_ptr(obj_dude->pid, &proto);
    return (proto->critter.data.flags & (1 << state)) != 0;
}

// 0x42E32C
int critter_sneak_check(Object* obj, void* data)
{
    int time;

    int sneak = skill_level(obj_dude, SKILL_SNEAK);
    if (skill_result(obj_dude, SKILL_SNEAK, 0, NULL) < ROLL_SUCCESS) {
        time = 600;
        sneak_working = false;

        if (sneak > 250)
            time = 100;
        else if (sneak > 200)
            time = 120;
        else if (sneak > 170)
            time = 150;
        else if (sneak > 135)
            time = 200;
        else if (sneak > 100)
            time = 300;
        else if (sneak > 80)
            time = 400;
    } else {
        time = 600;
        sneak_working = true;
    }

    queue_add(time, obj_dude, NULL, EVENT_TYPE_SNEAK);

    return 0;
}

// 0x42E3E4
int critter_sneak_clear(Object* obj, void* data)
{
    pc_flag_off(DUDE_STATE_SNEAKING);
    return 1;
}

// Returns true if dude is really sneaking.
//
// 0x42E3F4
bool is_pc_sneak_working()
{
    // NOTE: Uninline.
    if (is_pc_flag(DUDE_STATE_SNEAKING)) {
        return sneak_working;
    }

    return false;
}

// 0x42E424
int critter_wake_up(Object* obj, void* data)
{
    if ((obj->data.critter.combat.results & DAM_DEAD) != 0) {
        return 0;
    }

    obj->data.critter.combat.results &= ~(DAM_KNOCKED_OUT | DAM_KNOCKED_DOWN);
    obj->data.critter.combat.results |= DAM_KNOCKED_DOWN;

    if (isInCombat()) {
        obj->data.critter.combat.maneuver |= CRITTER_MANEUVER_0x01;
    } else {
        dude_standup(obj);
    }

    return 0;
}

// 0x42E460
int critter_wake_clear(Object* obj, void* data)
{
    if (PID_TYPE(obj->pid) != OBJ_TYPE_CRITTER) {
        return 0;
    }

    if ((obj->data.critter.combat.results & DAM_DEAD) != 0) {
        return 0;
    }

    obj->data.critter.combat.results &= ~(DAM_KNOCKED_OUT | DAM_KNOCKED_DOWN);

    int fid = art_id(FID_TYPE(obj->fid), obj->fid & 0xFFF, ANIM_STAND, (obj->fid & 0xF000) >> 12, obj->rotation + 1);
    obj_change_fid(obj, fid, 0);

    return 0;
}

// 0x42E4C0
int critter_set_who_hit_me(Object* a1, Object* a2)
{
    if (a1 == NULL) {
        return -1;
    }

    if (a2 != NULL && FID_TYPE(a2->fid) != OBJ_TYPE_CRITTER) {
        return -1;
    }

    if (PID_TYPE(a1->pid) == OBJ_TYPE_CRITTER) {
        if (a2 == NULL || a1->data.critter.combat.team != a2->data.critter.combat.team || (stat_result(a1, STAT_INTELLIGENCE, -1, NULL) < 2 && (!isPartyMember(a1) || !isPartyMember(a2)))) {
            a1->data.critter.combat.whoHitMe = a2;
            if (a2 == obj_dude) {
                reaction_set(a1, -3);
            }
        }
    }

    return 0;
}

// 0x42E564
bool critter_can_obj_dude_rest()
{
    bool v1 = false;
    if (!wmMapCanRestHere(map_elevation)) {
        v1 = true;
    }

    bool result = true;

    Object** critterList;
    int critterListLength = obj_create_list(-1, map_elevation, OBJ_TYPE_CRITTER, &critterList);

    // TODO: Check conditions in this loop.
    for (int index = 0; index < critterListLength; index++) {
        Object* critter = critterList[index];
        if ((critter->data.critter.combat.results & DAM_DEAD) != 0) {
            continue;
        }

        if (critter == obj_dude) {
            continue;
        }

        if (critter->data.critter.combat.whoHitMe != obj_dude) {
            if (!v1 || critter->data.critter.combat.team == obj_dude->data.critter.combat.team) {
                continue;
            }
        }

        result = false;
        break;
    }

    if (critterListLength != 0) {
        obj_delete_list(critterList);
    }

    return result;
}

// 0x42E62C
int critter_compute_ap_from_distance(Object* critter, int actionPoints)
{
    if (PID_TYPE(critter->pid) != OBJ_TYPE_CRITTER) {
        return 0;
    }

    int flags = critter->data.critter.combat.results;
    if ((flags & DAM_CRIP_LEG_LEFT) != 0 && (flags & DAM_CRIP_LEG_RIGHT) != 0) {
        return 8 * actionPoints;
    } else if ((flags & DAM_CRIP_LEG_ANY) != 0) {
        return 4 * actionPoints;
    } else {
        return actionPoints;
    }
}

// 0x42E66C
bool critterIsOverloaded(Object* critter)
{
    int maxWeight = critterGetStat(critter, STAT_CARRY_WEIGHT);
    int currentWeight = item_total_weight(critter);
    return maxWeight < currentWeight;
}

// 0x42E690
bool critter_is_fleeing(Object* critter)
{
    return critter != NULL
        ? (critter->data.critter.combat.maneuver & CRITTER_MANUEVER_FLEEING) != 0
        : false;
}

// Checks proto critter flag.
//
// 0x42E6AC
bool critter_flag_check(int pid, int flag)
{
    if (pid == -1) {
        return false;
    }

    if (PID_TYPE(pid) != OBJ_TYPE_CRITTER) {
        return false;
    }

    Proto* proto;
    proto_ptr(pid, &proto);
    return (proto->critter.data.flags & flag) != 0;
}

// NOTE: Unused.
//
// 0x42E6F0
void critter_flag_set(int pid, int flag)
{
    Proto* proto;

    if (pid == -1) {
        return;
    }

    if (PID_TYPE(pid) != OBJ_TYPE_CRITTER) {
        return;
    }

    proto_ptr(pid, &proto);

    proto->critter.data.flags |= flag;
}

// NOTE: Unused.
//
// 0x42E71C
void critter_flag_unset(int pid, int flag)
{
    Proto* proto;

    if (pid == -1) {
        return;
    }

    if (PID_TYPE(pid) != OBJ_TYPE_CRITTER) {
        return;
    }

    proto_ptr(pid, &proto);

    proto->critter.data.flags &= ~flag;
}

// NOTE: Unused.
//
// 0x42E74C
void critter_flag_toggle(int pid, int flag)
{
    Proto* proto;

    if (pid == -1) {
        return;
    }

    if (PID_TYPE(pid) != OBJ_TYPE_CRITTER) {
        return;
    }

    proto_ptr(pid, &proto);

    proto->critter.data.flags ^= flag;
}
