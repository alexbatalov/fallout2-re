#include "int/support/intextra.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "game/actions.h"
#include "game/anim.h"
#include "plib/color/color.h"
#include "game/combat.h"
#include "game/combatai.h"
#include "plib/gnw/input.h"
#include "game/critter.h"
#include "plib/gnw/debug.h"
#include "int/dialog.h"
#include "game/display.h"
#include "game/endgame.h"
#include "game/game.h"
#include "game/gconfig.h"
#include "game/gdialog.h"
#include "game/gmovie.h"
#include "game/gsound.h"
#include "plib/gnw/rect.h"
#include "game/intface.h"
#include "game/item.h"
#include "game/light.h"
#include "game/loadsave.h"
#include "game/map.h"
#include "game/object.h"
#include "game/palette.h"
#include "game/perk.h"
#include "game/proto.h"
#include "game/protinst.h"
#include "game/queue.h"
#include "game/roll.h"
#include "game/reaction.h"
#include "game/scripts.h"
#include "game/skill.h"
#include "game/stat.h"
#include "game/textobj.h"
#include "game/tile.h"
#include "game/trait.h"
#include "plib/gnw/vcr.h"
#include "game/worldmap.h"

typedef enum Metarule {
    METARULE_SIGNAL_END_GAME = 13,
    METARULE_FIRST_RUN = 14,
    METARULE_ELEVATOR = 15,
    METARULE_PARTY_COUNT = 16,
    METARULE_AREA_KNOWN = 17,
    METARULE_WHO_ON_DRUGS = 18,
    METARULE_MAP_KNOWN = 19,
    METARULE_IS_LOADGAME = 22,
    METARULE_CAR_CURRENT_TOWN = 30,
    METARULE_GIVE_CAR_TO_PARTY = 31,
    METARULE_GIVE_CAR_GAS = 32,
    METARULE_SKILL_CHECK_TAG = 40,
    METARULE_DROP_ALL_INVEN = 42,
    METARULE_INVEN_UNWIELD_WHO = 43,
    METARULE_GET_WORLDMAP_XPOS = 44,
    METARULE_GET_WORLDMAP_YPOS = 45,
    METARULE_CURRENT_TOWN = 46,
    METARULE_LANGUAGE_FILTER = 47,
    METARULE_VIOLENCE_FILTER = 48,
    METARULE_WEAPON_DAMAGE_TYPE = 49,
    METARULE_CRITTER_BARTERS = 50,
    METARULE_CRITTER_KILL_TYPE = 51,
    METARULE_SET_CAR_CARRY_AMOUNT = 52,
    METARULE_GET_CAR_CARRY_AMOUNT = 53,
} Metarule;

typedef enum Metarule3 {
    METARULE3_CLR_FIXED_TIMED_EVENTS = 100,
    METARULE3_MARK_SUBTILE = 101,
    METARULE3_SET_WM_MUSIC = 102,
    METARULE3_GET_KILL_COUNT = 103,
    METARULE3_MARK_MAP_ENTRANCE = 104,
    METARULE3_WM_SUBTILE_STATE = 105,
    METARULE3_TILE_GET_NEXT_CRITTER = 106,
    METARULE3_ART_SET_BASE_FID_NUM = 107,
    METARULE3_TILE_SET_CENTER = 108,
    // chem use preference
    METARULE3_109 = 109,
    // probably true if car is out of fuel
    METARULE3_110 = 110,
    // probably returns city index
    METARULE3_111 = 111,
} Metarule3;

typedef enum CritterTrait {
    CRITTER_TRAIT_PERK = 0,
    CRITTER_TRAIT_OBJECT = 1,
    CRITTER_TRAIT_TRAIT = 2,
} CritterTrait;

typedef enum CritterTraitObject {
    CRITTER_TRAIT_OBJECT_AI_PACKET = 5,
    CRITTER_TRAIT_OBJECT_TEAM = 6,
    CRITTER_TRAIT_OBJECT_ROTATION = 10,
    CRITTER_TRAIT_OBJECT_IS_INVISIBLE = 666,
    CRITTER_TRAIT_OBJECT_GET_INVENTORY_WEIGHT = 669,
} CritterTraitObject;

// See [op_critter_state].
typedef enum CritterState {
    CRITTER_STATE_NORMAL = 0x00,
    CRITTER_STATE_DEAD = 0x01,
    CRITTER_STATE_PRONE = 0x02,
} CritterState;

enum {
    INVEN_TYPE_WORN = 0,
    INVEN_TYPE_RIGHT_HAND = 1,
    INVEN_TYPE_LEFT_HAND = 2,
    INVEN_TYPE_INV_COUNT = -2,
};

typedef enum FloatingMessageType {
    FLOATING_MESSAGE_TYPE_WARNING = -2,
    FLOATING_MESSAGE_TYPE_COLOR_SEQUENCE = -1,
    FLOATING_MESSAGE_TYPE_NORMAL = 0,
    FLOATING_MESSAGE_TYPE_BLACK,
    FLOATING_MESSAGE_TYPE_RED,
    FLOATING_MESSAGE_TYPE_GREEN,
    FLOATING_MESSAGE_TYPE_BLUE,
    FLOATING_MESSAGE_TYPE_PURPLE,
    FLOATING_MESSAGE_TYPE_NEAR_WHITE,
    FLOATING_MESSAGE_TYPE_LIGHT_RED,
    FLOATING_MESSAGE_TYPE_YELLOW,
    FLOATING_MESSAGE_TYPE_WHITE,
    FLOATING_MESSAGE_TYPE_GREY,
    FLOATING_MESSAGE_TYPE_DARK_GREY,
    FLOATING_MESSAGE_TYPE_LIGHT_GREY,
    FLOATING_MESSAGE_TYPE_COUNT,
} FloatingMessageType;

typedef enum OpRegAnimFunc {
    OP_REG_ANIM_FUNC_BEGIN = 1,
    OP_REG_ANIM_FUNC_CLEAR = 2,
    OP_REG_ANIM_FUNC_END = 3,
} OpRegAnimFunc;

static void int_debug(const char* format, ...);
static int scripts_tile_is_visible(int tile);
static int correctFidForRemovedItem(Object* a1, Object* a2, int a3);
static void op_give_exp_points(Program* program);
static void op_scr_return(Program* program);
static void op_play_sfx(Program* program);
static void op_set_map_start(Program* program);
static void op_override_map_start(Program* program);
static void op_has_skill(Program* program);
static void op_using_skill(Program* program);
static void op_roll_vs_skill(Program* program);
static void op_skill_contest(Program* program);
static void op_do_check(Program* program);
static void op_is_success(Program* program);
static void op_is_critical(Program* program);
static void op_how_much(Program* program);
static void op_mark_area_known(Program* program);
static void op_reaction_influence(Program* program);
static void op_random(Program* program);
static void op_roll_dice(Program* program);
static void op_move_to(Program* program);
static void op_create_object_sid(Program* program);
static void op_destroy_object(Program* program);
static void op_display_msg(Program* program);
static void op_script_overrides(Program* program);
static void op_obj_is_carrying_obj_pid(Program* program);
static void op_tile_contains_obj_pid(Program* program);
static void op_self_obj(Program* program);
static void op_source_obj(Program* program);
static void op_target_obj(Program* program);
static void op_dude_obj(Program* program);
static void op_obj_being_used_with(Program* program);
static void op_local_var(Program* program);
static void op_set_local_var(Program* program);
static void op_map_var(Program* program);
static void op_set_map_var(Program* program);
static void op_global_var(Program* program);
static void op_set_global_var(Program* program);
static void op_script_action(Program* program);
static void op_obj_type(Program* program);
static void op_obj_item_subtype(Program* program);
static void op_get_critter_stat(Program* program);
static void op_set_critter_stat(Program* program);
static void op_animate_stand_obj(Program* program);
static void op_animate_stand_reverse_obj(Program* program);
static void op_animate_move_obj_to_tile(Program* program);
static void op_tile_in_tile_rect(Program* program);
static void op_make_daytime(Program* program);
static void op_tile_distance(Program* program);
static void op_tile_distance_objs(Program* program);
static void op_tile_num(Program* program);
static void op_tile_num_in_direction(Program* program);
static void op_pickup_obj(Program* program);
static void op_drop_obj(Program* program);
static void op_add_obj_to_inven(Program* program);
static void op_rm_obj_from_inven(Program* program);
static void op_wield_obj_critter(Program* program);
static void op_use_obj(Program* program);
static void op_obj_can_see_obj(Program* program);
static void op_attack(Program* program);
static void op_start_gdialog(Program* program);
static void op_end_dialogue(Program* program);
static void op_dialogue_reaction(Program* program);
static void op_metarule3(Program* program);
static void op_set_map_music(Program* program);
static void op_set_obj_visibility(Program* program);
static void op_load_map(Program* program);
static void op_wm_area_set_pos(Program* program);
static void op_set_exit_grids(Program* program);
static void op_anim_busy(Program* program);
static void op_critter_heal(Program* program);
static void op_set_light_level(Program* program);
static void op_game_time(Program* program);
static void op_game_time_in_seconds(Program* program);
static void op_elevation(Program* program);
static void op_kill_critter(Program* program);
static void op_kill_critter_type(Program* program);
static void op_critter_damage(Program* program);
static void op_add_timer_event(Program* program);
static void op_rm_timer_event(Program* program);
static void op_game_ticks(Program* program);
static void op_has_trait(Program* program);
static void op_obj_can_hear_obj(Program* program);
static void op_game_time_hour(Program* program);
static void op_fixed_param(Program* program);
static void op_tile_is_visible(Program* program);
static void op_dialogue_system_enter(Program* program);
static void op_action_being_used(Program* program);
static void op_critter_state(Program* program);
static void op_game_time_advance(Program* program);
static void op_radiation_inc(Program* program);
static void op_radiation_dec(Program* program);
static void op_critter_attempt_placement(Program* program);
static void op_obj_pid(Program* program);
static void op_cur_map_index(Program* program);
static void op_critter_add_trait(Program* program);
static void op_critter_rm_trait(Program* program);
static void op_proto_data(Program* program);
static void op_message_str(Program* program);
static void op_critter_inven_obj(Program* program);
static void op_obj_set_light_level(Program* program);
static void op_world_map(Program* program);
static void op_inven_cmds(Program* program);
static void op_float_msg(Program* program);
static void op_metarule(Program* program);
static void op_anim(Program* program);
static void op_obj_carrying_pid_obj(Program* program);
static void op_reg_anim_func(Program* program);
static void op_reg_anim_animate(Program* program);
static void op_reg_anim_animate_reverse(Program* program);
static void op_reg_anim_obj_move_to_obj(Program* program);
static void op_reg_anim_obj_run_to_obj(Program* program);
static void op_reg_anim_obj_move_to_tile(Program* program);
static void op_reg_anim_obj_run_to_tile(Program* program);
static void op_play_gmovie(Program* program);
static void op_add_mult_objs_to_inven(Program* program);
static void op_rm_mult_objs_from_inven(Program* program);
static void op_get_month(Program* program);
static void op_get_day(Program* program);
static void op_explosion(Program* program);
static void op_days_since_visited(Program* program);
static void op_gsay_start(Program* program);
static void op_gsay_end(Program* program);
static void op_gsay_reply(Program* program);
static void op_gsay_option(Program* program);
static void op_gsay_message(Program* program);
static void op_giq_option(Program* program);
static void op_poison(Program* program);
static void op_get_poison(Program* program);
static void op_party_add(Program* program);
static void op_party_remove(Program* program);
static void op_reg_anim_animate_forever(Program* program);
static void op_critter_injure(Program* program);
static void op_combat_is_initialized(Program* program);
static void op_gdialog_barter(Program* program);
static void op_difficulty_level(Program* program);
static void op_running_burning_guy(Program* program);
static void op_inven_unwield(Program* program);
static void op_obj_is_locked(Program* program);
static void op_obj_lock(Program* program);
static void op_obj_unlock(Program* program);
static void op_obj_is_open(Program* program);
static void op_obj_open(Program* program);
static void op_obj_close(Program* program);
static void op_game_ui_disable(Program* program);
static void op_game_ui_enable(Program* program);
static void op_game_ui_is_disabled(Program* program);
static void op_gfade_out(Program* program);
static void op_gfade_in(Program* program);
static void op_item_caps_total(Program* program);
static void op_item_caps_adjust(Program* program);
static void op_anim_action_frame(Program* program);
static void op_reg_anim_play_sfx(Program* program);
static void op_critter_mod_skill(Program* program);
static void op_sfx_build_char_name(Program* program);
static void op_sfx_build_ambient_name(Program* program);
static void op_sfx_build_interface_name(Program* program);
static void op_sfx_build_item_name(Program* program);
static void op_sfx_build_weapon_name(Program* program);
static void op_sfx_build_scenery_name(Program* program);
static void op_sfx_build_open_name(Program* program);
static void op_attack_setup(Program* program);
static void op_destroy_mult_objs(Program* program);
static void op_use_obj_on_obj(Program* program);
static void op_endgame_slideshow(Program* program);
static void op_move_obj_inven_to_obj(Program* program);
static void op_endgame_movie(Program* program);
static void op_obj_art_fid(Program* program);
static void op_art_anim(Program* program);
static void op_party_member_obj(Program* program);
static void op_rotation_to_tile(Program* program);
static void op_jam_lock(Program* program);
static void op_gdialog_set_barter_mod(Program* program);
static void op_combat_difficulty(Program* program);
static void op_obj_on_screen(Program* program);
static void op_critter_is_fleeing(Program* program);
static void op_critter_set_flee_state(Program* program);
static void op_terminate_combat(Program* program);
static void op_debug_msg(Program* program);
static void op_critter_stop_attacking(Program* program);
static void op_tile_contains_pid_obj(Program* program);
static void op_obj_name(Program* program);
static void op_get_pc_stat(Program* program);

// TODO: Remove.
// 0x504B0C
char _aCritter[] = "<Critter>";

// NOTE: This value is a little bit odd. It's used to handle 2 operations:
// [op_start_gdialog] and [op_dialogue_reaction]. It's not used outside those
// functions.
//
// When used inside [op_start_gdialog] this value stores [Fidget] constant
// (1 - Good, 4 - Neutral, 7 - Bad).
//
// When used inside [op_dialogue_reaction] this value contains specified
// reaction (-1 - Good, 0 - Neutral, 1 - Bad).
//
// 0x5970D0
static int dialogue_mood;

// 0x453FD0
void dbg_error(Program* program, const char* name, int error)
{
    // 0x518EC0
    static const char* dbg_error_strs[SCRIPT_ERROR_COUNT] = {
        "unimped",
        "obj is NULL",
        "can't match program to sid",
        "follows",
    };

    char string[260];

    sprintf(string, "Script Error: %s: op_%s: %s", program->name, name, dbg_error_strs[error]);

    debug_printf(string);
}

// 0x45400C
static void int_debug(const char* format, ...)
{
    char string[260];

    va_list argptr;
    va_start(argptr, format);
    vsprintf(string, format, argptr);
    va_end(argptr);

    debug_printf(string);
}

// 0x45404C
static int scripts_tile_is_visible(int tile)
{
    if (abs(tile_center_tile - tile) % 200 < 5) {
        return 1;
    }

    if (abs(tile_center_tile - tile) / 200 < 5) {
        return 1;
    }

    return 0;
}

// 0x45409C
static int correctFidForRemovedItem(Object* a1, Object* a2, int flags)
{
    if (a1 == obj_dude) {
        bool animated = !game_ui_is_disabled();
        intface_update_items(animated, INTERFACE_ITEM_ACTION_DEFAULT, INTERFACE_ITEM_ACTION_DEFAULT);
    }

    int fid = a1->fid;
    int v8 = (fid & 0xF000) >> 12;
    int newFid = -1;

    if ((flags & 0x03000000) != 0) {
        if (a1 == obj_dude) {
            if (intface_is_item_right_hand()) {
                if ((flags & 0x02000000) != 0) {
                    v8 = 0;
                }
            } else {
                if ((flags & 0x01000000) != 0) {
                    v8 = 0;
                }
            }
        } else {
            if ((flags & 0x02000000) != 0) {
                v8 = 0;
            }
        }

        if (v8 == 0) {
            newFid = art_id(FID_TYPE(fid), fid & 0xFFF, FID_ANIM_TYPE(fid), 0, (fid & 0x70000000) >> 28);
        }
    } else {
        if (a1 == obj_dude) {
            newFid = art_id(FID_TYPE(fid), art_vault_guy_num, FID_ANIM_TYPE(fid), v8, (fid & 0x70000000) >> 28);
        }

        adjust_ac(a1, a2, NULL);
    }

    if (newFid != -1) {
        Rect rect;
        obj_change_fid(a1, newFid, &rect);
        tile_refresh_rect(&rect, map_elevation);
    }

    return 0;
}

// 0x4541C8
static void op_give_exp_points(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to give_exp_points", program->name);
    }

    if (stat_pc_add_experience(data) != 0) {
        int_debug("\nScript Error: %s: op_give_exp_points: stat_pc_set failed");
    }
}

// 0x454238
static void op_scr_return(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to scr_return", program->name);
    }

    int sid = scr_find_sid_from_program(program);

    Script* script;
    if (scr_ptr(sid, &script) != -1) {
        script->field_28 = data;
    }
}

// 0x4542AC
static void op_play_sfx(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        interpretError("script error: %s: invalid arg to play_sfx", program->name);
    }

    char* name = interpretGetString(program, opcode, data);
    gsound_play_sfx_file(name);
}

// 0x454314
static void op_set_map_start(Program* program)
{
    opcode_t opcode[4];
    int data[4];

    for (int arg = 0; arg < 4; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to set_map_start", program->name, arg);
        }
    }

    int x = data[3];
    int y = data[2];
    int elevation = data[1];
    int rotation = data[0];

    if (map_set_elevation(elevation) != 0) {
        int_debug("\nScript Error: %s: op_set_map_start: map_set_elevation failed", program->name);
        return;
    }

    int tile = 200 * y + x;
    if (tile_set_center(tile, TILE_SET_CENTER_REFRESH_WINDOW | TILE_SET_CENTER_FLAG_IGNORE_SCROLL_RESTRICTIONS) != 0) {
        int_debug("\nScript Error: %s: op_set_map_start: tile_set_center failed", program->name);
        return;
    }

    map_set_entrance_hex(tile, elevation, rotation);
}

// 0x4543F4
static void op_override_map_start(Program* program)
{
    program->flags |= PROGRAM_FLAG_0x20;

    opcode_t opcode[4];
    int data[4];

    for (int arg = 0; arg < 4; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to override_map_start", program->name, arg);
        }
    }

    int x = data[3];
    int y = data[2];
    int elevation = data[1];
    int rotation = data[0];

    char text[60];
    sprintf(text, "OVERRIDE_MAP_START: x: %d, y: %d", x, y);
    debug_printf(text);

    int tile = 200 * y + x;
    int previousTile = tile_center_tile;
    if (tile != -1) {
        if (obj_set_rotation(obj_dude, rotation, NULL) != 0) {
            int_debug("\nError: %s: obj_set_rotation failed in override_map_start!", program->name);
        }

        if (obj_move_to_tile(obj_dude, tile, elevation, NULL) != 0) {
            int_debug("\nError: %s: obj_move_to_tile failed in override_map_start!", program->name);

            if (obj_move_to_tile(obj_dude, previousTile, elevation, NULL) != 0) {
                int_debug("\nError: %s: obj_move_to_tile RECOVERY Also failed!");
                exit(1);
            }
        }

        tile_set_center(tile, TILE_SET_CENTER_REFRESH_WINDOW);
        tile_refresh_display();
    }

    program->flags &= ~PROGRAM_FLAG_0x20;
}

// 0x454568
static void op_has_skill(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to has_skill", program->name, arg);
        }
    }

    Object* object = (Object*)data[1];
    int skill = data[0];

    int result = 0;
    if (object != NULL) {
        if (PID_TYPE(object->pid) == OBJ_TYPE_CRITTER) {
            result = skill_level(object, skill);
        }
    } else {
        dbg_error(program, "has_skill", SCRIPT_ERROR_OBJECT_IS_NULL);
    }

    interpretPushLong(program, result);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x454634
static void op_using_skill(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to using_skill", program->name, arg);
        }
    }

    Object* object = (Object*)data[1];
    int skill = data[0];

    // NOTE: In the original source code this value is left uninitialized, that
    // explains why garbage is returned when using something else than dude and
    // SKILL_SNEAK as arguments.
    int result = 0;

    if (skill == SKILL_SNEAK && object == obj_dude) {
        result = is_pc_flag(DUDE_STATE_SNEAKING);
    }

    interpretPushLong(program, result);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x4546E8
static void op_roll_vs_skill(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to roll_vs_skill", program->name, arg);
        }
    }

    Object* object = (Object*)data[2];
    int skill = data[1];
    int modifier = data[0];

    int roll = ROLL_CRITICAL_FAILURE;
    if (object != NULL) {
        if (PID_TYPE(object->pid) == OBJ_TYPE_CRITTER) {
            int sid = scr_find_sid_from_program(program);

            Script* script;
            if (scr_ptr(sid, &script) != -1) {
                roll = skill_result(object, skill, modifier, &(script->howMuch));
            }
        }
    } else {
        dbg_error(program, "roll_vs_skill", SCRIPT_ERROR_OBJECT_IS_NULL);
    }

    interpretPushLong(program, roll);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x4547D4
static void op_skill_contest(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to skill_contest", program->name, arg);
        }
    }

    dbg_error(program, "skill_contest", SCRIPT_ERROR_NOT_IMPLEMENTED);
    interpretPushLong(program, 0);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x454890
static void op_do_check(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to do_check", program->name, arg);
        }
    }

    Object* object = (Object*)data[2];
    int stat = data[1];
    int mod = data[0];

    int roll = 0;
    if (object != NULL) {
        int sid = scr_find_sid_from_program(program);

        Script* script;
        if (scr_ptr(sid, &script) != -1) {
            switch (stat) {
            case STAT_STRENGTH:
            case STAT_PERCEPTION:
            case STAT_ENDURANCE:
            case STAT_CHARISMA:
            case STAT_INTELLIGENCE:
            case STAT_AGILITY:
            case STAT_LUCK:
                roll = stat_result(object, stat, mod, &(script->howMuch));
                break;
            default:
                int_debug("\nScript Error: %s: op_do_check: Stat out of range", program->name);
                break;
            }
        }
    } else {
        dbg_error(program, "do_check", SCRIPT_ERROR_OBJECT_IS_NULL);
    }

    interpretPushLong(program, roll);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// success
// 0x4549A8
static void op_is_success(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to success", program->name);
    }

    int result = -1;

    switch (data) {
    case ROLL_CRITICAL_FAILURE:
    case ROLL_FAILURE:
        result = 0;
        break;
    case ROLL_SUCCESS:
    case ROLL_CRITICAL_SUCCESS:
        result = 1;
        break;
    }

    interpretPushLong(program, result);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// critical
// 0x454A44
static void op_is_critical(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to critical", program->name);
    }

    int result = -1;

    switch (data) {
    case ROLL_CRITICAL_FAILURE:
    case ROLL_CRITICAL_SUCCESS:
        result = 1;
        break;
    case ROLL_FAILURE:
    case ROLL_SUCCESS:
        result = 0;
        break;
    }

    interpretPushLong(program, result);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x454AD0
static void op_how_much(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to how_much", program->name);
    }

    int result = 0;

    int sid = scr_find_sid_from_program(program);

    Script* script;
    if (scr_ptr(sid, &script) != -1) {
        result = script->howMuch;
    } else {
        dbg_error(program, "how_much", SCRIPT_ERROR_CANT_MATCH_PROGRAM_TO_SID);
    }

    interpretPushLong(program, result);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x454B6C
static void op_mark_area_known(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to mark_area_known", program->name, arg);
        }
    }

    // TODO: Provide meaningful names.
    if (data[2] == 0) {
        if (data[0] == CITY_STATE_INVISIBLE) {
            wmAreaSetVisibleState(data[1], 0, 1);
        } else {
            wmAreaSetVisibleState(data[1], 1, 1);
            wmAreaMarkVisitedState(data[1], data[0]);
        }
    } else if (data[2] == 1) {
        wmMapMarkVisited(data[1]);
    }
}

// 0x454C34
static void op_reaction_influence(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to reaction_influence", program->name, arg);
        }
    }

    int result = reaction_influence();
    interpretPushLong(program, result);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x454CD4
static void op_random(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to random", program->name, arg);
        }
    }

    int result;
    if (vcr_status() == VCR_STATE_TURNED_OFF) {
        result = roll_random(data[1], data[0]);
    } else {
        result = (data[0] - data[1]) / 2;
    }

    interpretPushLong(program, result);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x454D88
static void op_roll_dice(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to roll_dice", program->name, arg);
        }
    }

    dbg_error(program, "roll_dice", SCRIPT_ERROR_NOT_IMPLEMENTED);

    interpretPushLong(program, 0);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x454E28
static void op_move_to(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to move_to", program->name, arg);
        }
    }

    Object* object = (Object*)data[2];
    int tile = data[1];
    int elevation = data[0];

    int newTile;

    if (object != NULL) {
        if (object == obj_dude) {
            bool tileLimitingEnabled = tile_get_scroll_limiting();
            bool tileBlockingEnabled = tile_get_scroll_blocking();

            if (tileLimitingEnabled) {
                tile_disable_scroll_limiting();
            }

            if (tileBlockingEnabled) {
                tile_disable_scroll_blocking();
            }

            Rect rect;
            newTile = obj_move_to_tile(object, tile, elevation, &rect);
            if (newTile != -1) {
                tile_set_center(object->tile, TILE_SET_CENTER_REFRESH_WINDOW);
            }

            if (tileLimitingEnabled) {
                tile_enable_scroll_limiting();
            }

            if (tileBlockingEnabled) {
                tile_enable_scroll_blocking();
            }
        } else {
            Rect before;
            obj_bound(object, &before);

            if (object->elevation != elevation && PID_TYPE(object->pid) == OBJ_TYPE_CRITTER) {
                combat_delete_critter(object);
            }

            Rect after;
            newTile = obj_move_to_tile(object, tile, elevation, &after);
            if (newTile != -1) {
                rect_min_bound(&before, &after, &before);
                tile_refresh_rect(&before, map_elevation);
            }
        }
    } else {
        dbg_error(program, "move_to", SCRIPT_ERROR_OBJECT_IS_NULL);
        newTile = -1;
    }

    interpretPushLong(program, newTile);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x454FA8
static void op_create_object_sid(Program* program)
{
    opcode_t opcode[4];
    int data[4];

    for (int arg = 0; arg < 4; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to create_object", program->name, arg);
        }
    }

    int pid = data[3];
    int tile = data[2];
    int elevation = data[1];
    int sid = data[0];

    Object* object = NULL;

    if (isLoadingGame() != 0) {
        debug_printf("\nError: attempt to Create critter in load/save-game: %s!", program->name);
        goto out;
    }

    if (pid == 0) {
        debug_printf("\nError: attempt to Create critter With PID of 0: %s!", program->name);
        goto out;
    }

    Proto* proto;
    if (proto_ptr(pid, &proto) != -1) {
        if (obj_new(&object, proto->fid, pid) != -1) {
            if (tile == -1) {
                tile = 0;
            }

            Rect rect;
            if (obj_move_to_tile(object, tile, elevation, &rect) != -1) {
                tile_refresh_rect(&rect, object->elevation);
            }
        }
    }

    if (sid != -1) {
        int scriptType = 0;
        switch (PID_TYPE(object->pid)) {
        case OBJ_TYPE_CRITTER:
            scriptType = SCRIPT_TYPE_CRITTER;
            break;
        case OBJ_TYPE_ITEM:
        case OBJ_TYPE_SCENERY:
            scriptType = SCRIPT_TYPE_ITEM;
            break;
        }

        if (object->sid != -1) {
            scr_remove(object->sid);
            object->sid = -1;
        }

        if (scr_new(&(object->sid), scriptType) == -1) {
            goto out;
        }

        Script* script;
        if (scr_ptr(object->sid, &script) == -1) {
            goto out;
        }

        script->field_14 = sid - 1;

        if (scriptType == SCRIPT_TYPE_SPATIAL) {
            script->sp.built_tile = builtTileCreate(object->tile, object->elevation);
            script->sp.radius = 3;
        }

        object->id = new_obj_id();
        script->field_1C = object->id;
        script->owner = object;
        scr_find_str_run_info(sid - 1, &(script->field_50), object->sid);
    };

out:

    interpretPushLong(program, (int)object);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x4551E4
static void op_destroy_object(Program* program)
{
    program->flags |= PROGRAM_FLAG_0x20;

    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to destroy_object", program->name);
    }

    Object* object = (Object*)data;

    if (object == NULL) {
        dbg_error(program, "destroy_object", SCRIPT_ERROR_OBJECT_IS_NULL);
        program->flags &= ~PROGRAM_FLAG_0x20;
        return;
    }

    if (PID_TYPE(object->pid) == OBJ_TYPE_CRITTER) {
        if (isLoadingGame()) {
            debug_printf("\nError: attempt to destroy critter in load/save-game: %s!", program->name);
            program->flags &= ~PROGRAM_FLAG_0x20;
            return;
        }
    }

    bool isSelf = object == scr_find_obj_from_program(program);

    if (PID_TYPE(object->pid) == OBJ_TYPE_CRITTER) {
        combat_delete_critter(object);
    }

    Object* owner = obj_top_environment(object);
    if (owner != NULL) {
        int quantity = item_count(owner, object);
        item_remove_mult(owner, object, quantity);

        if (owner == obj_dude) {
            bool animated = !game_ui_is_disabled();
            intface_update_items(animated, INTERFACE_ITEM_ACTION_DEFAULT, INTERFACE_ITEM_ACTION_DEFAULT);
        }

        obj_connect(object, 1, 0, NULL);

        if (isSelf) {
            object->sid = -1;
            object->flags |= (OBJECT_HIDDEN | OBJECT_TEMPORARY);
        } else {
            register_clear(object);
            obj_erase_object(object, NULL);
        }
    } else {
        register_clear(object);

        Rect rect;
        obj_erase_object(object, &rect);
        tile_refresh_rect(&rect, map_elevation);
    }

    program->flags &= ~PROGRAM_FLAG_0x20;

    if (isSelf) {
        program->flags |= PROGRAM_FLAG_0x0100;
    }
}

// 0x455388
static void op_display_msg(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        interpretError("script error: %s: invalid arg to display_msg", program->name);
    }

    char* string = interpretGetString(program, opcode, data);
    display_print(string);

    bool showScriptMessages = false;
    configGetBool(&game_config, GAME_CONFIG_DEBUG_KEY, GAME_CONFIG_SHOW_SCRIPT_MESSAGES_KEY, &showScriptMessages);

    if (showScriptMessages) {
        debug_printf("\n");
        debug_printf(string);
    }
}

// 0x455430
static void op_script_overrides(Program* program)
{
    int sid = scr_find_sid_from_program(program);

    Script* script;
    if (scr_ptr(sid, &script) != -1) {
        script->scriptOverrides = 1;
    } else {
        dbg_error(program, "script_overrides", SCRIPT_ERROR_CANT_MATCH_PROGRAM_TO_SID);
    }
}

// 0x455470
static void op_obj_is_carrying_obj_pid(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to obj_is_carrying_obj", program->name, arg);
        }
    }

    Object* obj = (Object*)data[1];
    int pid = data[0];

    int result = 0;
    if (obj != NULL) {
        result = inven_pid_quantity_carried(obj, pid);
    } else {
        dbg_error(program, "obj_is_carrying_obj_pid", SCRIPT_ERROR_OBJECT_IS_NULL);
    }

    interpretPushLong(program, result);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x455534
static void op_tile_contains_obj_pid(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to tile_contains_obj_pid", program->name, arg);
        }
    }

    int tile = data[2];
    int elevation = data[1];
    int pid = data[0];

    int result = 0;

    Object* object = obj_find_first_at_tile(elevation, tile);
    while (object) {
        if (object->pid == pid) {
            result = 1;
            break;
        }
        object = obj_find_next_at_tile();
    }

    interpretPushLong(program, result);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x455600
static void op_self_obj(Program* program)
{
    Object* object = scr_find_obj_from_program(program);
    interpretPushLong(program, (int)object);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x455624
static void op_source_obj(Program* program)
{
    Object* object = NULL;

    int sid = scr_find_sid_from_program(program);

    Script* script;
    if (scr_ptr(sid, &script) != -1) {
        object = script->source;
    } else {
        dbg_error(program, "source_obj", SCRIPT_ERROR_CANT_MATCH_PROGRAM_TO_SID);
    }

    interpretPushLong(program, (int)object);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x455678
static void op_target_obj(Program* program)
{
    Object* object = NULL;

    int sid = scr_find_sid_from_program(program);

    Script* script;
    if (scr_ptr(sid, &script) != -1) {
        object = script->target;
    } else {
        dbg_error(program, "target_obj", SCRIPT_ERROR_CANT_MATCH_PROGRAM_TO_SID);
    }

    interpretPushLong(program, (int)object);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x4556CC
static void op_dude_obj(Program* program)
{
    interpretPushLong(program, (int)obj_dude);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// NOTE: The implementation is the same as in [op_target_obj].
//
// 0x4556EC
static void op_obj_being_used_with(Program* program)
{
    Object* object = NULL;

    int sid = scr_find_sid_from_program(program);

    Script* script;
    if (scr_ptr(sid, &script) != -1) {
        object = script->target;
    } else {
        dbg_error(program, "obj_being_used_with", SCRIPT_ERROR_CANT_MATCH_PROGRAM_TO_SID);
    }

    interpretPushLong(program, (int)object);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x455740
static void op_local_var(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        // FIXME: The error message is wrong.
        interpretError("script error: %s: invalid arg to op_global_var", program->name);
    }

    int value = -1;

    int sid = scr_find_sid_from_program(program);
    scr_get_local_var(sid, data, &value);

    interpretPushLong(program, value);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x4557C8
static void op_set_local_var(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to set_local_var", program->name, arg);
        }
    }

    int variable = data[1];
    int value = data[0];

    int sid = scr_find_sid_from_program(program);
    scr_set_local_var(sid, variable, value);
}

// 0x455858
static void op_map_var(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to op_map_var", program->name);
    }

    int value = map_get_global_var(data);

    interpretPushLong(program, value);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x4558C8
static void op_set_map_var(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to set_map_var", program->name, arg);
        }
    }

    int variable = data[1];
    int value = data[0];

    map_set_global_var(variable, value);
}

// 0x455950
static void op_global_var(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to op_global_var", program->name);
    }

    int value = -1;
    if (num_game_global_vars != 0) {
        value = game_get_global_var(data);
    } else {
        int_debug("\nScript Error: %s: op_global_var: no global vars found!", program->name);
    }

    interpretPushLong(program, value);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x4559EC
static void op_set_global_var(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to set_global_var", program->name, arg);
        }
    }

    int variable = data[1];
    int value = data[0];

    if (num_game_global_vars != 0) {
        game_set_global_var(variable, value);
    } else {
        int_debug("\nScript Error: %s: op_set_global_var: no global vars found!", program->name);
    }
}

// 0x455A90
static void op_script_action(Program* program)
{
    int action = 0;

    int sid = scr_find_sid_from_program(program);

    Script* script;
    if (scr_ptr(sid, &script) != -1) {
        action = script->action;
    } else {
        dbg_error(program, "script_action", SCRIPT_ERROR_CANT_MATCH_PROGRAM_TO_SID);
    }

    interpretPushLong(program, action);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x455AE4
static void op_obj_type(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to op_obj_type", program->name);
    }

    Object* object = (Object*)data;

    int objectType = -1;
    if (object != NULL) {
        objectType = FID_TYPE(object->fid);
    }

    interpretPushLong(program, objectType);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x455B6C
static void op_obj_item_subtype(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to op_item_subtype", program->name);
    }

    Object* obj = (Object*)data;

    int itemType = -1;
    if (obj != NULL) {
        if (PID_TYPE(obj->pid) == OBJ_TYPE_ITEM) {
            Proto* proto;
            if (proto_ptr(obj->pid, &proto) != -1) {
                itemType = item_get_type(obj);
            }
        }
    }

    interpretPushLong(program, itemType);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x455C10
static void op_get_critter_stat(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to get_critter_stat", program->name, arg);
        }
    }

    Object* object = (Object*)data[1];
    int stat = data[0];

    int value = -1;
    if (object != NULL) {
        value = critterGetStat(object, stat);
    } else {
        dbg_error(program, "get_critter_stat", SCRIPT_ERROR_OBJECT_IS_NULL);
    }

    interpretPushLong(program, value);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// NOTE: Despite it's name it does not actually "set" stat, but "adjust". So
// it's last argument is amount of adjustment, not it's final value.
//
// 0x455CCC
static void op_set_critter_stat(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to set_critter_stat", program->name, arg);
        }
    }

    Object* object = (Object*)data[2];
    int stat = data[1];
    int value = data[0];

    int result = 0;
    if (object != NULL) {
        if (object == obj_dude) {
            int currentValue = stat_get_base(object, stat);
            stat_set_base(object, stat, currentValue + value);
        } else {
            dbg_error(program, "set_critter_stat", SCRIPT_ERROR_FOLLOWS);
            debug_printf(" Can't modify anyone except obj_dude!");
            result = -1;
        }
    } else {
        dbg_error(program, "set_critter_stat", SCRIPT_ERROR_OBJECT_IS_NULL);
        result = -1;
    }

    interpretPushLong(program, result);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x455DC8
static void op_animate_stand_obj(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to animate_stand_obj", program->name);
    }

    Object* object = (Object*)data;
    if (object == NULL) {
        int sid = scr_find_sid_from_program(program);

        Script* script;
        if (scr_ptr(sid, &script) == -1) {
            dbg_error(program, "animate_stand_obj", SCRIPT_ERROR_CANT_MATCH_PROGRAM_TO_SID);
            return;
        }

        object = scr_find_obj_from_program(program);
    }

    if (!isInCombat()) {
        register_begin(ANIMATION_REQUEST_UNRESERVED);
        register_object_animate(object, ANIM_STAND, 0);
        register_end();
    }
}

// 0x455E7C
static void op_animate_stand_reverse_obj(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        // FIXME: typo in message, should be animate_stand_reverse_obj.
        interpretError("script error: %s: invalid arg to animate_stand_obj", program->name);
    }

    Object* object = (Object*)data;
    if (object == NULL) {
        int sid = scr_find_sid_from_program(program);

        Script* script;
        if (scr_ptr(sid, &script) == -1) {
            dbg_error(program, "animate_stand_reverse_obj", SCRIPT_ERROR_CANT_MATCH_PROGRAM_TO_SID);
            return;
        }

        object = scr_find_obj_from_program(program);
    }

    if (!isInCombat()) {
        register_begin(ANIMATION_REQUEST_UNRESERVED);
        register_object_animate_reverse(object, ANIM_STAND, 0);
        register_end();
    }
}

// 0x455F30
static void op_animate_move_obj_to_tile(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to animate_move_obj_to_tile", program->name, arg);
        }
    }

    Object* object = (Object*)data[2];
    int tile = data[1];
    int flags = data[0];

    if (object == NULL) {
        dbg_error(program, "animate_move_obj_to_tile", SCRIPT_ERROR_OBJECT_IS_NULL);
        return;
    }

    if (tile <= -1) {
        return;
    }

    int sid = scr_find_sid_from_program(program);

    Script* script;
    if (scr_ptr(sid, &script) == -1) {
        dbg_error(program, "animate_move_obj_to_tile", SCRIPT_ERROR_CANT_MATCH_PROGRAM_TO_SID);
        return;
    }

    if (!critter_is_active(object)) {
        return;
    }

    if (isInCombat()) {
        return;
    }

    if ((flags & 0x10) != 0) {
        register_clear(object);
        flags &= ~0x10;
    }

    register_begin(ANIMATION_REQUEST_UNRESERVED);

    if (flags == 0) {
        register_object_move_to_tile(object, tile, object->elevation, -1, 0);
    } else {
        register_object_run_to_tile(object, tile, object->elevation, -1, 0);
    }

    register_end();
}

// 0x45607C
static void op_tile_in_tile_rect(Program* program)
{
    opcode_t opcode[5];
    int data[5];
    Point points[5];

    for (int arg = 0; arg < 5; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to tile_in_tile_rect", program->name, arg);
        }

        points[arg].x = data[arg] % 200;
        points[arg].y = data[arg] / 200;
    }

    int x = points[0].x;
    int y = points[0].y;

    int minX = points[1].x;
    int maxX = points[4].x;

    int minY = points[4].y;
    int maxY = points[1].y;

    int result = 0;
    if (x >= minX && x <= maxX && y >= minY && y <= maxY) {
        result = 1;
    }

    interpretPushLong(program, result);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x456170
static void op_make_daytime(Program* program)
{
}

// 0x456174
static void op_tile_distance(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to tile_distance", program->name, arg);
        }
    }

    int tile1 = data[1];
    int tile2 = data[0];

    int distance;

    if (tile1 != -1 && tile2 != -1) {
        distance = tile_dist(tile1, tile2);
    } else {
        distance = 9999;
    }

    interpretPushLong(program, distance);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x456228
static void op_tile_distance_objs(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to tile_distance_objs", program->name, arg);
        }
    }

    Object* object1 = (Object*)data[1];
    Object* object2 = (Object*)data[0];

    int distance = 9999;
    if (object1 != NULL && object2 != NULL) {
        if ((unsigned int)data[1] >= HEX_GRID_SIZE && (unsigned int)data[0] >= HEX_GRID_SIZE) {
            if (object1->elevation == object2->elevation) {
                if (object1->tile != -1 && object2->tile != -1) {
                    distance = tile_dist(object1->tile, object2->tile);
                }
            }
        } else {
            dbg_error(program, "tile_distance_objs", SCRIPT_ERROR_FOLLOWS);
            debug_printf(" Passed a tile # instead of an object!!!BADBADBAD!");
        }
    }

    interpretPushLong(program, distance);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x456324
static void op_tile_num(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to tile_num", program->name);
    }

    Object* obj = (Object*)data;

    int tile = -1;
    if (obj != NULL) {
        tile = obj->tile;
    } else {
        dbg_error(program, "tile_num", SCRIPT_ERROR_OBJECT_IS_NULL);
    }

    interpretPushLong(program, tile);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x4563B4
static void op_tile_num_in_direction(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to tile_num_in_direction", program->name, arg);
        }
    }

    int origin = data[2];
    int rotation = data[1];
    int distance = data[0];

    int tile = -1;

    if (origin != -1) {
        if (rotation < ROTATION_COUNT) {
            if (distance != 0) {
                tile = tile_num_in_direction(origin, rotation, distance);
                if (tile < -1) {
                    debug_printf("\nError: %s: op_tile_num_in_direction got #: %d", program->name, tile);
                    tile = -1;
                }
            }
        } else {
            dbg_error(program, "tile_num_in_direction", SCRIPT_ERROR_FOLLOWS);
            debug_printf(" rotation out of Range!");
        }
    } else {
        dbg_error(program, "tile_num_in_direction", SCRIPT_ERROR_FOLLOWS);
        debug_printf(" tileNum is -1!");
    }

    interpretPushLong(program, tile);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x4564D4
static void op_pickup_obj(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to pickup_obj", program->name);
    }

    Object* object = (Object*)data;

    if (object == NULL) {
        return;
    }

    int sid = scr_find_sid_from_program(program);

    Script* script;
    if (scr_ptr(sid, &script) == 1) {
        dbg_error(program, "pickup_obj", SCRIPT_ERROR_CANT_MATCH_PROGRAM_TO_SID);
        return;
    }

    if (script->target == NULL) {
        dbg_error(program, "pickup_obj", SCRIPT_ERROR_OBJECT_IS_NULL);
        return;
    }

    action_get_an_object(script->target, object);
}

// 0x456580
static void op_drop_obj(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to drop_obj", program->name);
    }

    Object* object = (Object*)data;

    if (object == NULL) {
        return;
    }

    int sid = scr_find_sid_from_program(program);

    Script* script;
    if (scr_ptr(sid, &script) == -1) {
        // FIXME: Should be SCRIPT_ERROR_CANT_MATCH_PROGRAM_TO_SID.
        dbg_error(program, "drop_obj", SCRIPT_ERROR_OBJECT_IS_NULL);
        return;
    }

    if (script->target == NULL) {
        // FIXME: Should be SCRIPT_ERROR_OBJECT_IS_NULL.
        dbg_error(program, "drop_obj", SCRIPT_ERROR_CANT_MATCH_PROGRAM_TO_SID);
        return;
    }

    obj_drop(script->target, object);
}

// 0x45662C
static void op_add_obj_to_inven(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to add_obj_to_inven", program->name, arg);
        }
    }

    Object* owner = (Object*)data[1];
    Object* item = (Object*)data[0];

    if (owner == NULL || item == NULL) {
        return;
    }

    if (item->owner == NULL) {
        if (item_add_force(owner, item, 1) == 0) {
            Rect rect;
            obj_disconnect(item, &rect);
            tile_refresh_rect(&rect, item->elevation);
        }
    } else {
        dbg_error(program, "add_obj_to_inven", SCRIPT_ERROR_FOLLOWS);
        debug_printf(" Item was already attached to something else!");
    }
}

// 0x456708
static void op_rm_obj_from_inven(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to rm_obj_from_inven", program->name, arg);
        }
    }

    Object* owner = (Object*)data[1];
    Object* item = (Object*)data[0];

    if (owner == NULL || item == NULL) {
        return;
    }

    bool updateFlags = false;
    int flags = 0;

    if ((item->flags & OBJECT_EQUIPPED) != 0) {
        if ((item->flags & OBJECT_IN_LEFT_HAND) != 0) {
            flags |= OBJECT_IN_LEFT_HAND;
        }

        if ((item->flags & OBJECT_IN_RIGHT_HAND) != 0) {
            flags |= OBJECT_IN_RIGHT_HAND;
        }

        if ((item->flags & OBJECT_WORN) != 0) {
            flags |= OBJECT_WORN;
        }

        updateFlags = true;
    }

    if (item_remove_mult(owner, item, 1) == 0) {
        Rect rect;
        obj_connect(item, 1, 0, &rect);
        tile_refresh_rect(&rect, item->elevation);

        if (updateFlags) {
            correctFidForRemovedItem(owner, item, flags);
        }
    }
}

// 0x45681C
static void op_wield_obj_critter(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to wield_obj_critter", program->name, arg);
        }
    }

    Object* critter = (Object*)data[1];
    Object* item = (Object*)data[0];

    if (critter == NULL) {
        dbg_error(program, "wield_obj_critter", SCRIPT_ERROR_OBJECT_IS_NULL);
        return;
    }

    if (item == NULL) {
        dbg_error(program, "wield_obj_critter", SCRIPT_ERROR_OBJECT_IS_NULL);
        return;
    }

    if (PID_TYPE(critter->pid) != OBJ_TYPE_CRITTER) {
        dbg_error(program, "wield_obj_critter", SCRIPT_ERROR_FOLLOWS);
        debug_printf(" Only works for critters!  ERROR ERROR ERROR!");
        return;
    }

    int hand = HAND_RIGHT;

    bool shouldAdjustArmorClass = false;
    Object* oldArmor = NULL;
    Object* newArmor = NULL;
    if (critter == obj_dude) {
        if (intface_is_item_right_hand() == HAND_LEFT) {
            hand = HAND_LEFT;
        }

        if (item_get_type(item) == ITEM_TYPE_ARMOR) {
            oldArmor = inven_worn(obj_dude);
        }

        shouldAdjustArmorClass = true;
        newArmor = item;
    }

    if (inven_wield(critter, item, hand) == -1) {
        dbg_error(program, "wield_obj_critter", SCRIPT_ERROR_FOLLOWS);
        debug_printf(" inven_wield failed!  ERROR ERROR ERROR!");
        return;
    }

    if (critter == obj_dude) {
        if (shouldAdjustArmorClass) {
            adjust_ac(critter, oldArmor, newArmor);
        }

        bool animated = !game_ui_is_disabled();
        intface_update_items(animated, INTERFACE_ITEM_ACTION_DEFAULT, INTERFACE_ITEM_ACTION_DEFAULT);
    }
}

// 0x4569D0
static void op_use_obj(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to use_obj", program->name);
    }

    Object* object = (Object*)data;

    if (object == NULL) {
        dbg_error(program, "use_obj", SCRIPT_ERROR_OBJECT_IS_NULL);
        return;
    }

    int sid = scr_find_sid_from_program(program);

    Script* script;
    if (scr_ptr(sid, &script) == -1) {
        // FIXME: Should be SCRIPT_ERROR_CANT_MATCH_PROGRAM_TO_SID.
        dbg_error(program, "use_obj", SCRIPT_ERROR_OBJECT_IS_NULL);
        return;
    }

    if (script->target == NULL) {
        dbg_error(program, "use_obj", SCRIPT_ERROR_OBJECT_IS_NULL);
        return;
    }

    Object* self = scr_find_obj_from_program(program);
    if (PID_TYPE(self->pid) == OBJ_TYPE_CRITTER) {
        action_use_an_object(script->target, object);
    } else {
        obj_use(self, object);
    }
}

// 0x456AC4
static void op_obj_can_see_obj(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to obj_can_see_obj", program->name, arg);
        }
    }

    Object* object1 = (Object*)data[1];
    Object* object2 = (Object*)data[0];

    int result = 0;

    if (object1 != NULL && object2 != NULL) {
        if (object2->tile != -1) {
            // NOTE: Looks like dead code, I guess these checks were incorporated
            // into higher level functions, but this code left intact.
            if (object2 == obj_dude) {
                is_pc_flag(0);
            }

            critterGetStat(object1, STAT_PERCEPTION);

            if (is_within_perception(object1, object2)) {
                Object* a5;
                make_straight_path(object1, object1->tile, object2->tile, NULL, &a5, 16);
                if (a5 == object2) {
                    result = 1;
                }
            }
        }
    } else {
        dbg_error(program, "obj_can_see_obj", SCRIPT_ERROR_OBJECT_IS_NULL);
    }

    interpretPushLong(program, result);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x456C00
static void op_attack(Program* program)
{
    opcode_t opcode[8];
    int data[8];

    for (int arg = 0; arg < 8; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to attack", program->name, arg);
        }
    }

    Object* target = (Object*)data[7];
    if (target == NULL) {
        dbg_error(program, "attack", SCRIPT_ERROR_OBJECT_IS_NULL);
        return;
    }

    program->flags |= PROGRAM_FLAG_0x20;

    Object* self = scr_find_obj_from_program(program);
    if (self == NULL) {
        program->flags &= ~PROGRAM_FLAG_0x20;
        return;
    }

    if (!critter_is_active(self) || (self->flags & OBJECT_HIDDEN) != 0) {
        debug_printf("\n   But is already Inactive (Dead/Stunned/Invisible)");
        program->flags &= ~PROGRAM_FLAG_0x20;
        return;
    }

    if (!critter_is_active(target) || (target->flags & OBJECT_HIDDEN) != 0) {
        debug_printf("\n   But target is already dead or invisible");
        program->flags &= ~PROGRAM_FLAG_0x20;
        return;
    }

    if ((target->data.critter.combat.maneuver & CRITTER_MANUEVER_FLEEING) != 0) {
        debug_printf("\n   But target is AFRAID");
        program->flags &= ~PROGRAM_FLAG_0x20;
        return;
    }

    if (gdialogActive()) {
        // TODO: Might be an error, program flag is not removed.
        return;
    }

    if (isInCombat()) {
        CritterCombatData* combatData = &(self->data.critter.combat);
        if ((combatData->maneuver & CRITTER_MANEUVER_0x01) == 0) {
            combatData->maneuver |= CRITTER_MANEUVER_0x01;
            combatData->whoHitMe = target;
        }
    } else {
        STRUCT_664980 attack;
        attack.attacker = self;
        attack.defender = target;
        attack.actionPointsBonus = 0;
        attack.accuracyBonus = data[4];
        attack.damageBonus = 0;
        attack.minDamage = data[3];
        attack.maxDamage = data[2];

        // TODO: Something is probably broken here, why it wants
        // flags to be the same? Maybe because both of them
        // are applied to defender because of the bug in 0x422F3C?
        if (data[1] == data[0]) {
            attack.field_1C = 1;
            attack.field_24 = data[0];
            attack.field_20 = data[1];
        } else {
            attack.field_1C = 0;
        }

        scripts_request_combat(&attack);
    }

    program->flags &= ~PROGRAM_FLAG_0x20;
}

// 0x456DF0
static void op_start_gdialog(Program* program)
{
    opcode_t opcode[5];
    int data[5];

    for (int arg = 0; arg < 5; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to start_gdialog", program->name, arg);
        }
    }

    Object* obj = (Object*)data[3];
    int reactionLevel = data[2];
    int headId = data[1];
    int backgroundId = data[0];

    if (isInCombat()) {
        return;
    }

    if (obj == NULL) {
        dbg_error(program, "start_gdialog", SCRIPT_ERROR_OBJECT_IS_NULL);
        return;
    }

    dialogue_head = -1;
    if (PID_TYPE(obj->pid) == OBJ_TYPE_CRITTER) {
        Proto* proto;
        if (proto_ptr(obj->pid, &proto) == -1) {
            return;
        }
    }

    if (headId != -1) {
        dialogue_head = art_id(OBJ_TYPE_HEAD, headId, 0, 0, 0);
    }

    gdialogSetBackground(backgroundId);
    dialogue_mood = reactionLevel;

    if (dialogue_head != -1) {
        int npcReactionValue = reaction_get(dialog_target);
        int npcReactionType = reaction_lookup_internal(npcReactionValue);
        switch (npcReactionType) {
        case NPC_REACTION_BAD:
            dialogue_mood = FIDGET_BAD;
            break;
        case NPC_REACTION_NEUTRAL:
            dialogue_mood = FIDGET_NEUTRAL;
            break;
        case NPC_REACTION_GOOD:
            dialogue_mood = FIDGET_GOOD;
            break;
        }
    }

    dialogue_scr_id = scr_find_sid_from_program(program);
    dialog_target = scr_find_obj_from_program(program);
    gdialogInitFromScript(dialogue_head, dialogue_mood);
}

// 0x456F80
static void op_end_dialogue(Program* program)
{
    if (gdialogExitFromScript() != -1) {
        dialog_target = NULL;
        dialogue_scr_id = -1;
    }
}

// 0x456FA4
static void op_dialogue_reaction(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int value = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, value);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to dialogue_reaction", program->name);
    }

    dialogue_mood = value;
    talk_to_critter_reacts(value);
}

// 0x457110
static void op_metarule3(Program* program)
{
    opcode_t opcode[4];
    int data[4];

    for (int arg = 0; arg < 4; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to metarule3", program->name, arg);
        }
    }

    int rule = data[3];
    int result = 0;

    switch (rule) {
    case METARULE3_CLR_FIXED_TIMED_EVENTS:
        if (1) {
            scrSetQueueTestVals((Object*)data[2], data[1]);
            queue_clear_type(EVENT_TYPE_SCRIPT, scrQueueRemoveFixed);
        }
        break;
    case METARULE3_MARK_SUBTILE:
        result = wmSubTileMarkRadiusVisited(data[2], data[1], data[0]);
        break;
    case METARULE3_GET_KILL_COUNT:
        result = critter_kill_count(data[2]);
        break;
    case METARULE3_MARK_MAP_ENTRANCE:
        result = wmMapMarkMapEntranceState(data[2], data[1], data[0]);
        break;
    case METARULE3_WM_SUBTILE_STATE:
        if (1) {
            int state;
            if (wmSubTileGetVisitedState(data[2], data[1], &state) == 0) {
                result = state;
            }
        }
        break;
    case METARULE3_TILE_GET_NEXT_CRITTER:
        if (1) {
            int tile = data[2];
            int elevation = data[1];
            Object* previousCritter = (Object*)data[0];

            bool critterFound = previousCritter == NULL;

            Object* object = obj_find_first_at_tile(elevation, tile);
            while (object != NULL) {
                if (PID_TYPE(object->pid) == OBJ_TYPE_CRITTER) {
                    if (critterFound) {
                        result = (int)object;
                        break;
                    }
                }

                if (object == previousCritter) {
                    critterFound = true;
                }

                object = obj_find_next_at_tile();
            }
        }
        break;
    case METARULE3_ART_SET_BASE_FID_NUM:
        if (1) {
            Object* obj = (Object*)data[2];
            int frmId = data[1];

            int fid = art_id(FID_TYPE(obj->fid),
                frmId,
                FID_ANIM_TYPE(obj->fid),
                (obj->fid & 0xF000) >> 12,
                (obj->fid & 0x70000000) >> 28);

            Rect updatedRect;
            obj_change_fid(obj, fid, &updatedRect);
            tile_refresh_rect(&updatedRect, map_elevation);
        }
        break;
    case METARULE3_TILE_SET_CENTER:
        result = tile_set_center(data[2], TILE_SET_CENTER_REFRESH_WINDOW);
        break;
    case METARULE3_109:
        result = ai_get_chem_use_value((Object*)data[2]);
        break;
    case METARULE3_110:
        result = wmCarIsOutOfGas() ? 1 : 0;
        break;
    case METARULE3_111:
        result = map_target_load_area();
        break;
    }

    interpretPushLong(program, result);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x45734C
static void op_set_map_music(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        // FIXME: argument is wrong, should be 1.
        interpretError("script error: %s: invalid arg %d to set_map_music", program->name, 2);
    }

    int mapIndex = data[1];

    char* string = NULL;
    if ((opcode[0] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        string = interpretGetString(program, opcode[0], data[0]);
    } else {
        // FIXME: argument is wrong, should be 0.
        interpretError("script error: %s: invalid arg %d to set_map_music", program->name, 2);
    }

    debug_printf("\nset_map_music: %d, %s", mapIndex, string);
    wmSetMapMusic(mapIndex, string);
}

// NOTE: Function name is a bit misleading. Last parameter is a boolean value
// where 1 or true makes object invisible, and value 0 (false) makes it visible
// again. So a better name for this function is opSetObjectInvisible.
//
//
// 0x45741C
static void op_set_obj_visibility(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to set_obj_visibility", program->name, arg);
        }
    }

    Object* obj = (Object*)data[1];
    int invisible = data[0];

    if (obj == NULL) {
        dbg_error(program, "set_obj_visibility", SCRIPT_ERROR_OBJECT_IS_NULL);
        return;
    }

    if (isLoadingGame()) {
        debug_printf("Error: attempt to set_obj_visibility in load/save-game: %s!", program->name);
        return;
    }

    if (invisible != 0) {
        if ((obj->flags & OBJECT_HIDDEN) == 0) {
            if (isInCombat()) {
                obj_turn_off_outline(obj, NULL);
                obj_remove_outline(obj, NULL);
            }

            Rect rect;
            if (obj_turn_off(obj, &rect) != -1) {
                if (PID_TYPE(obj->pid) == OBJ_TYPE_CRITTER) {
                    obj->flags |= OBJECT_NO_BLOCK;
                }

                tile_refresh_rect(&rect, obj->elevation);
            }
        }
    } else {
        if ((obj->flags & OBJECT_HIDDEN) != 0) {
            if (PID_TYPE(obj->pid) == OBJ_TYPE_CRITTER) {
                obj->flags &= ~OBJECT_NO_BLOCK;
            }

            Rect rect;
            if (obj_turn_on(obj, &rect) != -1) {
                tile_refresh_rect(&rect, obj->elevation);
            }
        }
    }
}

// 0x45755C
static void op_load_map(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    opcode[0] = interpretPopShort(program);
    data[0] = interpretPopLong(program);

    if (opcode[0] == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode[0], data[0]);
    }

    if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg 0 to load_map", program->name);
    }

    opcode[1] = interpretPopShort(program);
    data[1] = interpretPopLong(program);

    if (opcode[1] == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode[1], data[1]);
    }

    int param = data[0];
    int mapIndexOrName = data[1];

    char* mapName = NULL;

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        if ((opcode[1] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
            mapName = interpretGetString(program, opcode[1], mapIndexOrName);
        } else {
            interpretError("script error: %s: invalid arg 1 to load_map", program->name);
        }
    }

    int mapIndex = -1;

    if (mapName != NULL) {
        game_global_vars[GVAR_LOAD_MAP_INDEX] = param;
        mapIndex = wmMapMatchNameToIdx(mapName);
    } else {
        if (mapIndexOrName >= 0) {
            game_global_vars[GVAR_LOAD_MAP_INDEX] = param;
            mapIndex = mapIndexOrName;
        }
    }

    if (mapIndex != -1) {
        MapTransition transition;
        transition.map = mapIndex;
        transition.elevation = -1;
        transition.tile = -1;
        transition.rotation = -1;
        map_leave_map(&transition);
    }
}

// 0x457680
static void op_wm_area_set_pos(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to wm_area_set_pos", program->name, arg);
        }
    }

    int city = data[2];
    int x = data[1];
    int y = data[0];

    if (wmAreaSetWorldPos(city, x, y) == -1) {
        dbg_error(program, "wm_area_set_pos", SCRIPT_ERROR_FOLLOWS);
        debug_printf("Invalid Parameter!");
    }
}

// 0x457730
static void op_set_exit_grids(Program* program)
{
    opcode_t opcode[5];
    int data[5];

    for (int arg = 0; arg < 5; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to set_exit_grids", program->name, arg);
        }
    }

    int elevation = data[4];
    int destinationMap = data[3];
    int destinationElevation = data[2];
    int destinationTile = data[1];
    int destinationRotation = data[0];

    Object* object = obj_find_first_at(elevation);
    while (object != NULL) {
        if (object->pid >= PROTO_ID_0x5000010 && object->pid <= PROTO_ID_0x5000017) {
            object->data.misc.map = destinationMap;
            object->data.misc.tile = destinationTile;
            object->data.misc.elevation = destinationElevation;
        }
        object = obj_find_next_at();
    }
}

// 0x4577EC
static void op_anim_busy(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to anim_busy", program->name);
    }

    Object* object = (Object*)data;

    int rc = 0;
    if (object != NULL) {
        rc = anim_busy(object);
    } else {
        dbg_error(program, "anim_busy", SCRIPT_ERROR_OBJECT_IS_NULL);
    }

    interpretPushLong(program, rc);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x457880
static void op_critter_heal(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to critter_heal", program->name, arg);
        }
    }

    Object* critter = (Object*)data[1];
    int amount = data[0];

    int rc = critter_adjust_hits(critter, amount);

    if (critter == obj_dude) {
        intface_update_hit_points(true);
    }

    interpretPushLong(program, rc);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x457934
static void op_set_light_level(Program* program)
{
    // Maps light level to light intensity.
    //
    // Middle value is mapped one-to-one which corresponds to 50% light level
    // (cavern lighting). Light levels above (51-100%) and below (0-49) is
    // calculated as percentage from two adjacent light values.
    //
    // 0x453F90
    static const int dword_453F90[3] = {
        0x4000,
        0xA000,
        0x10000,
    };

    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to set_light_level", program->name);
    }

    int lightLevel = data;

    if (data == 50) {
        light_set_ambient(dword_453F90[1], true);
        return;
    }

    int lightIntensity;
    if (data > 50) {
        lightIntensity = dword_453F90[1] + data * (dword_453F90[2] - dword_453F90[1]) / 100;
    } else {
        lightIntensity = dword_453F90[0] + data * (dword_453F90[1] - dword_453F90[0]) / 100;
    }

    light_set_ambient(lightIntensity, true);
}

// 0x4579F4
static void op_game_time(Program* program)
{
    int time = game_time();
    interpretPushLong(program, time);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x457A18
static void op_game_time_in_seconds(Program* program)
{
    int time = game_time();
    interpretPushLong(program, time / 10);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x457A44
static void op_elevation(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to elevation", program->name);
    }

    Object* object = (Object*)data;

    int elevation = 0;
    if (object != NULL) {
        elevation = object->elevation;
    } else {
        dbg_error(program, "elevation", SCRIPT_ERROR_OBJECT_IS_NULL);
    }

    interpretPushLong(program, elevation);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x457AD4
static void op_kill_critter(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to kill_critter", program->name, arg);
        }
    }

    Object* object = (Object*)data[1];
    int deathFrame = data[0];

    if (object == NULL) {
        dbg_error(program, "kill_critter", SCRIPT_ERROR_OBJECT_IS_NULL);
        return;
    }

    if (isLoadingGame()) {
        debug_printf("\nError: attempt to destroy critter in load/save-game: %s!", program->name);
    }

    program->flags |= PROGRAM_FLAG_0x20;

    Object* self = scr_find_obj_from_program(program);
    bool isSelf = self == object;

    register_clear(object);
    combat_delete_critter(object);
    critter_kill(object, deathFrame, 1);

    program->flags &= ~PROGRAM_FLAG_0x20;

    if (isSelf) {
        program->flags |= PROGRAM_FLAG_0x0100;
    }
}

// [forceBack] is to force fall back animation, otherwise it's fall front if it's present
int correctDeath(Object* critter, int anim, bool forceBack)
{
    if (anim >= ANIM_BIG_HOLE_SF && anim <= ANIM_FALL_FRONT_BLOOD_SF) {
        int violenceLevel = VIOLENCE_LEVEL_MAXIMUM_BLOOD;
        config_get_value(&game_config, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_VIOLENCE_LEVEL_KEY, &violenceLevel);

        bool useStandardDeath = false;
        if (violenceLevel < VIOLENCE_LEVEL_MAXIMUM_BLOOD) {
            useStandardDeath = true;
        } else {
            int fid = art_id(OBJ_TYPE_CRITTER, critter->fid & 0xFFF, anim, (critter->fid & 0xF000) >> 12, critter->rotation + 1);
            if (!art_exists(fid)) {
                useStandardDeath = true;
            }
        }

        if (useStandardDeath) {
            if (forceBack) {
                anim = ANIM_FALL_BACK;
            } else {
                int fid = art_id(OBJ_TYPE_CRITTER, critter->fid & 0xFFF, ANIM_FALL_FRONT, (critter->fid & 0xF000) >> 12, critter->rotation + 1);
                if (art_exists(fid)) {
                    anim = ANIM_FALL_FRONT;
                } else {
                    anim = ANIM_FALL_BACK;
                }
            }
        }
    }

    return anim;
}

// 0x457CB4
static void op_kill_critter_type(Program* program)
{
    // 0x518ED0
    static int ftList[11] = {
        ANIM_FALL_BACK_BLOOD_SF,
        ANIM_BIG_HOLE_SF,
        ANIM_CHARRED_BODY_SF,
        ANIM_CHUNKS_OF_FLESH_SF,
        ANIM_FALL_FRONT_BLOOD_SF,
        ANIM_FALL_BACK_BLOOD_SF,
        ANIM_DANCING_AUTOFIRE_SF,
        ANIM_SLICED_IN_HALF_SF,
        ANIM_EXPLODED_TO_NOTHING_SF,
        ANIM_FALL_BACK_BLOOD_SF,
        ANIM_FALL_FRONT_BLOOD_SF,
    };

    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to kill_critter", program->name, arg);
        }
    }

    int pid = data[1];
    int deathFrame = data[0];

    if (isLoadingGame()) {
        debug_printf("\nError: attempt to destroy critter in load/save-game: %s!", program->name);
        return;
    }

    program->flags |= PROGRAM_FLAG_0x20;

    Object* previousObj = NULL;
    int count = 0;
    int v3 = 0;

    Object* obj = obj_find_first();
    while (obj != NULL) {
        if (FID_ANIM_TYPE(obj->fid) >= ANIM_FALL_BACK_SF) {
            obj = obj_find_next();
            continue;
        }

        if ((obj->flags & OBJECT_HIDDEN) == 0 && obj->pid == pid && !critter_is_dead(obj)) {
            if (obj == previousObj || count > 200) {
                dbg_error(program, "kill_critter_type", SCRIPT_ERROR_FOLLOWS);
                debug_printf(" Infinite loop destroying critters!");
                program->flags &= ~PROGRAM_FLAG_0x20;
                return;
            }

            register_clear(obj);

            if (deathFrame != 0) {
                combat_delete_critter(obj);
                if (deathFrame == 1) {
                    int anim = correctDeath(obj, ftList[v3], 1);
                    critter_kill(obj, anim, 1);
                    v3 += 1;
                    if (v3 >= 11) {
                        v3 = 0;
                    }
                } else {
                    critter_kill(obj, ANIM_FALL_BACK_SF, 1);
                }
            } else {
                register_clear(obj);

                Rect rect;
                obj_erase_object(obj, &rect);
                tile_refresh_rect(&rect, map_elevation);
            }

            previousObj = obj;
            count += 1;

            obj_find_first();

            map_data.lastVisitTime = game_time();
        }

        obj = obj_find_next();
    }

    program->flags &= ~PROGRAM_FLAG_0x20;
}

// critter_dmg
// 0x457EB4
static void op_critter_damage(Program* program)
{
    program->flags |= PROGRAM_FLAG_0x20;

    opcode_t opcode[3];
    int data[3];

    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to critter_damage", program->name, arg);
        }
    }

    Object* object = (Object*)data[2];
    int amount = data[1];
    int damageTypeWithFlags = data[0];

    if (object == NULL) {
        dbg_error(program, "critter_damage", SCRIPT_ERROR_OBJECT_IS_NULL);
        return;
    }

    if (PID_TYPE(object->pid) != OBJ_TYPE_CRITTER) {
        dbg_error(program, "critter_damage", SCRIPT_ERROR_FOLLOWS);
        debug_printf(" Can't call on non-critters!");
        return;
    }

    Object* self = scr_find_obj_from_program(program);
    if (object->data.critter.combat.whoHitMeCid == -1) {
        object->data.critter.combat.whoHitMe = NULL;
    }

    bool animate = (damageTypeWithFlags & 0x200) == 0;
    bool bypassArmor = (damageTypeWithFlags & 0x100) != 0;
    int damageType = damageTypeWithFlags & ~(0x100 | 0x200);
    action_dmg(object->tile, object->elevation, amount, amount, damageType, animate, bypassArmor);

    program->flags &= ~PROGRAM_FLAG_0x20;

    if (self == object) {
        program->flags |= PROGRAM_FLAG_0x0100;
    }
}

// 0x457FF0
static void op_add_timer_event(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to add_timer_event", program->name, arg);
        }
    }

    Object* object = (Object*)data[2];
    int delay = data[1];
    int param = data[0];

    if (object == NULL) {
        int_debug("\nScript Error: %s: op_add_timer_event: pobj is NULL!", program->name);
        return;
    }

    script_q_add(object->sid, delay, param);
}

// 0x458094
static void op_rm_timer_event(Program* program)
{
    int elevation;

    elevation = 0;

    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to rm_timer_event", program->name);
    }

    Object* object = (Object*)data;

    if (object == NULL) {
        // FIXME: Should be op_rm_timer_event.
        int_debug("\nScript Error: %s: op_add_timer_event: pobj is NULL!");
        return;
    }

    queue_remove(object);
}

// Converts seconds into game ticks.
//
// 0x458108
static void op_game_ticks(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to game_ticks", program->name);
    }

    int ticks = data;

    if (ticks < 0) {
        ticks = 0;
    }

    interpretPushLong(program, ticks * 10);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// NOTE: The name of this function is misleading. It has (almost) nothing to do
// with player's "Traits" as a feature. Instead it's used to query many
// information of the critters using passed parameters. It's like "metarule" but
// for critters.
//
// 0x458180
// has_trait
static void op_has_trait(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to has_trait", program->name, arg);
        }
    }

    int type = data[2];
    Object* object = (Object*)data[1];
    int param = data[0];

    int result = 0;

    if (object != NULL) {
        switch (type) {
        case CRITTER_TRAIT_PERK:
            if (param < PERK_COUNT) {
                result = perk_level(object, param);
            } else {
                int_debug("\nScript Error: %s: op_has_trait: Perk out of range", program->name);
            }
            break;
        case CRITTER_TRAIT_OBJECT:
            switch (param) {
            case CRITTER_TRAIT_OBJECT_AI_PACKET:
                if (PID_TYPE(object->pid) == OBJ_TYPE_CRITTER) {
                    result = object->data.critter.combat.aiPacket;
                }
                break;
            case CRITTER_TRAIT_OBJECT_TEAM:
                if (PID_TYPE(object->pid) == OBJ_TYPE_CRITTER) {
                    result = object->data.critter.combat.team;
                }
                break;
            case CRITTER_TRAIT_OBJECT_ROTATION:
                result = object->rotation;
                break;
            case CRITTER_TRAIT_OBJECT_IS_INVISIBLE:
                result = (object->flags & OBJECT_HIDDEN) == 0;
                break;
            case CRITTER_TRAIT_OBJECT_GET_INVENTORY_WEIGHT:
                result = item_total_weight(object);
                break;
            }
            break;
        case CRITTER_TRAIT_TRAIT:
            if (param < TRAIT_COUNT) {
                result = trait_level(param);
            } else {
                int_debug("\nScript Error: %s: op_has_trait: Trait out of range", program->name);
            }
            break;
        default:
            int_debug("\nScript Error: %s: op_has_trait: Trait out of range", program->name);
            break;
        }
    } else {
        dbg_error(program, "has_trait", SCRIPT_ERROR_OBJECT_IS_NULL);
    }

    interpretPushLong(program, result);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x45835C
static void op_obj_can_hear_obj(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d, to obj_can_hear_obj", program->name, arg);
        }
    }

    Object* object1 = (Object*)data[1];
    Object* object2 = (Object*)data[0];

    bool canHear = false;

    // FIXME: This is clearly an error. If any of the object is NULL
    // dereferencing will crash the game.
    if (object2 == NULL || object1 == NULL) {
        if (object2->elevation == object1->elevation) {
            if (object2->tile != -1 && object1->tile != -1) {
                if (is_within_perception(object2, object1)) {
                    canHear = true;
                }
            }
        }
    }

    interpretPushLong(program, canHear);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x458438
static void op_game_time_hour(Program* program)
{
    int value = game_time_hour();
    interpretPushLong(program, value);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x45845C
static void op_fixed_param(Program* program)
{
    int fixedParam = 0;

    int sid = scr_find_sid_from_program(program);

    Script* script;
    if (scr_ptr(sid, &script) != -1) {
        fixedParam = script->fixedParam;
    } else {
        dbg_error(program, "fixed_param", SCRIPT_ERROR_CANT_MATCH_PROGRAM_TO_SID);
    }

    interpretPushLong(program, fixedParam);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x4584B0
static void op_tile_is_visible(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to tile_is_visible", program->name);
    }

    int isVisible = 0;
    if (scripts_tile_is_visible(data)) {
        isVisible = 1;
    }

    interpretPushLong(program, isVisible);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x458534
static void op_dialogue_system_enter(Program* program)
{
    int sid = scr_find_sid_from_program(program);

    Script* script;
    if (scr_ptr(sid, &script) == -1) {
        return;
    }

    Object* self = scr_find_obj_from_program(program);
    if (PID_TYPE(self->pid) == OBJ_TYPE_CRITTER) {
        if (!critter_is_active(self)) {
            return;
        }
    }

    if (isInCombat()) {
        return;
    }

    if (game_state_request(GAME_STATE_4) == -1) {
        return;
    }

    dialog_target = scr_find_obj_from_program(program);
}

// 0x458594
static void op_action_being_used(Program* program)
{
    int action = -1;

    int sid = scr_find_sid_from_program(program);

    Script* script;
    if (scr_ptr(sid, &script) != -1) {
        action = script->actionBeingUsed;
    } else {
        dbg_error(program, "action_being_used", SCRIPT_ERROR_CANT_MATCH_PROGRAM_TO_SID);
    }

    interpretPushLong(program, action);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x4585E8
static void op_critter_state(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to critter_state", program->name);
    }

    Object* critter = (Object*)data;

    int state = CRITTER_STATE_DEAD;
    if (critter != NULL && PID_TYPE(critter->pid) == OBJ_TYPE_CRITTER) {
        if (critter_is_active(critter)) {
            state = CRITTER_STATE_NORMAL;

            int anim = FID_ANIM_TYPE(critter->fid);
            if (anim >= ANIM_FALL_BACK_SF && anim <= ANIM_FALL_FRONT_SF) {
                state = CRITTER_STATE_PRONE;
            }

            state |= (critter->data.critter.combat.results & DAM_CRIP);
        } else {
            if (!critter_is_dead(critter)) {
                state = CRITTER_STATE_PRONE;
            }
        }
    } else {
        dbg_error(program, "critter_state", SCRIPT_ERROR_OBJECT_IS_NULL);
    }

    interpretPushLong(program, state);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x4586C8
static void op_game_time_advance(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to game_time_advance", program->name);
    }

    int days = data / GAME_TIME_TICKS_PER_DAY;
    int remainder = data % GAME_TIME_TICKS_PER_DAY;

    for (int day = 0; day < days; day++) {
        inc_game_time(GAME_TIME_TICKS_PER_DAY);
        queue_process();
    }

    inc_game_time(remainder);
    queue_process();
}

// 0x458760
static void op_radiation_inc(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to radiation_inc", program->name, arg);
        }
    }

    Object* object = (Object*)data[1];
    int amount = data[0];

    if (object == NULL) {
        dbg_error(program, "radiation_inc", SCRIPT_ERROR_OBJECT_IS_NULL);
        return;
    }

    critter_adjust_rads(object, amount);
}

// 0x458800
static void op_radiation_dec(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to radiation_dec", program->name, arg);
        }
    }

    Object* object = (Object*)data[1];
    int amount = data[0];

    if (object == NULL) {
        dbg_error(program, "radiation_dec", SCRIPT_ERROR_OBJECT_IS_NULL);
        return;
    }

    int radiation = critter_get_rads(object);
    int adjustment = radiation >= 0 ? -amount : 0;

    critter_adjust_rads(object, adjustment);
}

// 0x4588B4
static void op_critter_attempt_placement(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to critter_attempt_placement", program->name, arg);
        }
    }

    Object* critter = (Object*)data[2];
    int tile = data[1];
    int elevation = data[0];

    if (critter == NULL) {
        dbg_error(program, "critter_attempt_placement", SCRIPT_ERROR_OBJECT_IS_NULL);
        return;
    }

    if (elevation != critter->elevation && PID_TYPE(critter->pid) == OBJ_TYPE_CRITTER) {
        combat_delete_critter(critter);
    }

    obj_move_to_tile(critter, 0, elevation, NULL);

    int rc = obj_attempt_placement(critter, tile, elevation, 1);
    interpretPushLong(program, rc);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x4589A0
static void op_obj_pid(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to obj_pid", program->name);
    }

    Object* obj = (Object*)data;

    int pid = -1;
    if (obj) {
        pid = obj->pid;
    } else {
        dbg_error(program, "obj_pid", SCRIPT_ERROR_OBJECT_IS_NULL);
    }

    interpretPushLong(program, pid);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x458A30
static void op_cur_map_index(Program* program)
{
    int mapIndex = map_get_index_number();
    interpretPushLong(program, mapIndex);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x458A54
static void op_critter_add_trait(Program* program)
{
    opcode_t opcode[4];
    int data[4];

    for (int arg = 0; arg < 4; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to critter_add_trait", program->name, arg);
        }
    }

    Object* object = (Object*)data[3];
    int kind = data[2];
    int param = data[1];
    int value = data[0];

    if (object != NULL) {
        if (PID_TYPE(object->pid) == OBJ_TYPE_CRITTER) {
            switch (kind) {
            case CRITTER_TRAIT_PERK:
                if (1) {
                    char* critterName = critter_name(object);
                    char* perkName = perk_name(param);
                    debug_printf("\nintextra::critter_add_trait: Adding Perk %s to %s", perkName, critterName);

                    if (value > 0) {
                        if (perk_add_force(object, param) != 0) {
                            int_debug("\nScript Error: %s: op_critter_add_trait: perk_add_force failed", program->name);
                            debug_printf("Perk: %d", param);
                        }
                    } else {
                        if (perk_sub(object, param) != 0) {
                            // FIXME: typo in debug message, should be perk_sub
                            int_debug("\nScript Error: %s: op_critter_add_trait: per_sub failed", program->name);
                            debug_printf("Perk: %d", param);
                        }
                    }

                    if (object == obj_dude) {
                        intface_update_hit_points(true);
                    }
                }
                break;
            case CRITTER_TRAIT_OBJECT:
                switch (param) {
                case CRITTER_TRAIT_OBJECT_AI_PACKET:
                    combat_ai_set_ai_packet(object, value);
                    break;
                case CRITTER_TRAIT_OBJECT_TEAM:
                    if (isPartyMember(object)) {
                        break;
                    }

                    if (object->data.critter.combat.team == value) {
                        break;
                    }

                    if (isLoadingGame()) {
                        break;
                    }

                    combatai_switch_team(object, value);
                    break;
                }
                break;
            default:
                int_debug("\nScript Error: %s: op_critter_add_trait: Trait out of range", program->name);
                break;
            }
        }
    } else {
        dbg_error(program, "critter_add_trait", SCRIPT_ERROR_OBJECT_IS_NULL);
    }

    interpretPushLong(program, -1);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x458C2C
static void op_critter_rm_trait(Program* program)
{
    opcode_t opcode[4];
    int data[4];

    for (int arg = 0; arg < 4; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to critter_rm_trait", program->name, arg);
        }
    }

    Object* object = (Object*)data[3];
    int kind = data[2];
    int param = data[1];
    int value = data[0];

    if (object == NULL) {
        dbg_error(program, "critter_rm_trait", SCRIPT_ERROR_OBJECT_IS_NULL);
        // FIXME: Ruins stack.
        return;
    }

    if (PID_TYPE(object->pid) == OBJ_TYPE_CRITTER) {
        switch (kind) {
        case CRITTER_TRAIT_PERK:
            while (perk_level(object, param) > 0) {
                if (perk_sub(object, param) != 0) {
                    int_debug("\nScript Error: op_critter_rm_trait: perk_sub failed");
                }
            }
            break;
        default:
            int_debug("\nScript Error: %s: op_critter_rm_trait: Trait out of range", program->name);
            break;
        }
    }

    interpretPushLong(program, -1);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x458D38
static void op_proto_data(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to proto_data", program->name, arg);
        }
    }

    int pid = data[1];
    int member = data[0];

    ProtoDataMemberValue value;
    value.integerValue = 0;
    int valueType = proto_data_member(pid, member, &value);
    switch (valueType) {
    case PROTO_DATA_MEMBER_TYPE_INT:
        interpretPushLong(program, value.integerValue);
        interpretPushShort(program, VALUE_TYPE_INT);
        break;
    case PROTO_DATA_MEMBER_TYPE_STRING:
        interpretPushLong(program, interpretAddString(program, value.stringValue));
        interpretPushShort(program, VALUE_TYPE_DYNAMIC_STRING);
        break;
    default:
        interpretPushLong(program, 0);
        interpretPushShort(program, VALUE_TYPE_INT);
        break;
    }
}

// 0x458E10
static void op_message_str(Program* program)
{
    // 0x518EFC
    static char errStr[] = "Error";

    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to message_str", program->name, arg);
        }
    }

    int messageListIndex = data[1];
    int messageIndex = data[0];

    char* string;
    if (messageIndex >= 1) {
        string = scr_get_msg_str_speech(messageListIndex, messageIndex, 1);
        if (string == NULL) {
            debug_printf("\nError: No message file EXISTS!: index %d, line %d", messageListIndex, messageIndex);
            string = errStr;
        }
    } else {
        string = errStr;
    }

    interpretPushLong(program, interpretAddString(program, string));
    interpretPushShort(program, VALUE_TYPE_DYNAMIC_STRING);
}

// 0x458F00
static void op_critter_inven_obj(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to critter_inven_obj", program->name, arg);
        }
    }

    Object* critter = (Object*)data[1];
    int type = data[0];

    int result = 0;

    if (PID_TYPE(critter->pid) == OBJ_TYPE_CRITTER) {
        switch (type) {
        case INVEN_TYPE_WORN:
            result = (int)inven_worn(critter);
            break;
        case INVEN_TYPE_RIGHT_HAND:
            if (critter == obj_dude) {
                if (intface_is_item_right_hand() != HAND_LEFT) {
                    result = (int)inven_right_hand(critter);
                }
            } else {
                result = (int)inven_right_hand(critter);
            }
            break;
        case INVEN_TYPE_LEFT_HAND:
            if (critter == obj_dude) {
                if (intface_is_item_right_hand() == HAND_LEFT) {
                    result = (int)inven_left_hand(critter);
                }
            } else {
                result = (int)inven_left_hand(critter);
            }
            break;
        case INVEN_TYPE_INV_COUNT:
            result = critter->data.inventory.length;
            break;
        default:
            int_debug("script error: %s: Error in critter_inven_obj -- wrong type!", program->name);
            break;
        }
    } else {
        dbg_error(program, "critter_inven_obj", SCRIPT_ERROR_FOLLOWS);
        debug_printf("  Not a critter!");
    }

    interpretPushLong(program, result);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x459088
static void op_obj_set_light_level(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to obj_set_light_level", program->name, arg);
        }
    }

    Object* object = (Object*)data[2];
    int lightIntensity = data[1];
    int lightDistance = data[0];

    if (object == NULL) {
        dbg_error(program, "obj_set_light_level", SCRIPT_ERROR_OBJECT_IS_NULL);
        return;
    }

    Rect rect;
    if (lightIntensity != 0) {
        if (obj_set_light(object, lightDistance, (lightIntensity * 65636) / 100, &rect) == -1) {
            return;
        }
    } else {
        if (obj_set_light(object, lightDistance, 0, &rect) == -1) {
            return;
        }
    }
    tile_refresh_rect(&rect, object->elevation);
}

// 0x459170
static void op_world_map(Program* program)
{
    scripts_request_worldmap();
}

// 0x459178
static void op_inven_cmds(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to inven_cmds", program->name, arg);
        }
    }

    Object* obj = (Object*)data[2];
    int cmd = data[1];
    int index = data[0];

    Object* item = NULL;

    if (obj != NULL) {
        switch (cmd) {
        case 13:
            item = inven_index_ptr(obj, index);
            break;
        }
    } else {
        // FIXME: Should be inven_cmds.
        dbg_error(program, "anim", SCRIPT_ERROR_OBJECT_IS_NULL);
    }

    interpretPushLong(program, (int)item);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x459280
static void op_float_msg(Program* program)
{
    // 0x518F00
    static int last_color = 1;

    opcode_t opcode[3];
    int data[3];

    char* string = NULL;
    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if (arg == 1) {
            if ((opcode[arg] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
                string = interpretGetString(program, opcode[arg], data[arg]);
            }
        } else {
            if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
                interpretError("script error: %s: invalid arg %d to float_msg", program->name, arg);
            }
        }
    }

    Object* obj = (Object*)data[2];
    int floatingMessageType = data[0];

    int color = colorTable[32747];
    int a5 = colorTable[0];
    int font = 101;

    if (obj == NULL) {
        dbg_error(program, "float_msg", SCRIPT_ERROR_OBJECT_IS_NULL);
        return;
    }

    if (string == NULL || *string == '\0') {
        text_object_remove(obj);
        tile_refresh_display();
        return;
    }

    if (obj->elevation != map_elevation) {
        return;
    }

    if (floatingMessageType == FLOATING_MESSAGE_TYPE_COLOR_SEQUENCE) {
        floatingMessageType = last_color + 1;
        if (floatingMessageType >= FLOATING_MESSAGE_TYPE_COUNT) {
            floatingMessageType = FLOATING_MESSAGE_TYPE_BLACK;
        }
        last_color = floatingMessageType;
    }

    switch (floatingMessageType) {
    case FLOATING_MESSAGE_TYPE_WARNING:
        color = colorTable[31744];
        a5 = colorTable[0];
        font = 103;
        tile_set_center(obj_dude->tile, TILE_SET_CENTER_REFRESH_WINDOW);
        break;
    case FLOATING_MESSAGE_TYPE_NORMAL:
    case FLOATING_MESSAGE_TYPE_YELLOW:
        color = colorTable[32747];
        break;
    case FLOATING_MESSAGE_TYPE_BLACK:
    case FLOATING_MESSAGE_TYPE_PURPLE:
    case FLOATING_MESSAGE_TYPE_GREY:
        color = colorTable[10570];
        break;
    case FLOATING_MESSAGE_TYPE_RED:
        color = colorTable[31744];
        break;
    case FLOATING_MESSAGE_TYPE_GREEN:
        color = colorTable[992];
        break;
    case FLOATING_MESSAGE_TYPE_BLUE:
        color = colorTable[31];
        break;
    case FLOATING_MESSAGE_TYPE_NEAR_WHITE:
        color = colorTable[21140];
        break;
    case FLOATING_MESSAGE_TYPE_LIGHT_RED:
        color = colorTable[32074];
        break;
    case FLOATING_MESSAGE_TYPE_WHITE:
        color = colorTable[32767];
        break;
    case FLOATING_MESSAGE_TYPE_DARK_GREY:
        color = colorTable[8456];
        break;
    case FLOATING_MESSAGE_TYPE_LIGHT_GREY:
        color = colorTable[15855];
        break;
    }

    Rect rect;
    if (text_object_create(obj, string, font, color, a5, &rect) != -1) {
        tile_refresh_rect(&rect, obj->elevation);
    }
}

// 0x4594A0
static void op_metarule(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to metarule", program->name, arg);
        }
    }

    int rule = data[1];
    int param = data[0];

    int result = 0;

    switch (rule) {
    case METARULE_SIGNAL_END_GAME:
        result = 0;
        game_user_wants_to_quit = 2;
        break;
    case METARULE_FIRST_RUN:
        result = (map_data.flags & MAP_SAVED) == 0;
        break;
    case METARULE_ELEVATOR:
        scripts_request_elevator(scr_find_obj_from_program(program), param);
        result = 0;
        break;
    case METARULE_PARTY_COUNT:
        result = getPartyMemberCount();
        break;
    case METARULE_AREA_KNOWN:
        result = wmAreaVisitedState(param);
        break;
    case METARULE_WHO_ON_DRUGS:
        result = queue_find((Object*)param, EVENT_TYPE_DRUG);
        break;
    case METARULE_MAP_KNOWN:
        result = wmMapIsKnown(param);
        break;
    case METARULE_IS_LOADGAME:
        result = isLoadingGame();
        break;
    case METARULE_CAR_CURRENT_TOWN:
        result = wmCarCurrentArea();
        break;
    case METARULE_GIVE_CAR_TO_PARTY:
        result = wmCarGiveToParty();
        break;
    case METARULE_GIVE_CAR_GAS:
        result = wmCarFillGas(param);
        break;
    case METARULE_SKILL_CHECK_TAG:
        result = skill_is_tagged(param);
        break;
    case METARULE_DROP_ALL_INVEN:
        if (1) {
            Object* object = (Object*)param;
            result = item_drop_all(object, object->tile);
            if (obj_dude == object) {
                intface_update_items(false, INTERFACE_ITEM_ACTION_DEFAULT, INTERFACE_ITEM_ACTION_DEFAULT);
                intface_update_ac(false);
            }
        }
        break;
    case METARULE_INVEN_UNWIELD_WHO:
        if (1) {
            Object* object = (Object*)param;

            int hand = HAND_RIGHT;
            if (object == obj_dude) {
                if (intface_is_item_right_hand() == HAND_LEFT) {
                    hand = HAND_LEFT;
                }
            }

            result = invenUnwieldFunc(object, hand, 0);

            if (object == obj_dude) {
                bool animated = !game_ui_is_disabled();
                intface_update_items(animated, INTERFACE_ITEM_ACTION_DEFAULT, INTERFACE_ITEM_ACTION_DEFAULT);
            } else {
                Object* item = inven_left_hand(object);
                if (item_get_type(item) == ITEM_TYPE_WEAPON) {
                    item->flags &= ~OBJECT_IN_LEFT_HAND;
                }
            }
        }
        break;
    case METARULE_GET_WORLDMAP_XPOS:
        wmGetPartyWorldPos(&result, NULL);
        break;
    case METARULE_GET_WORLDMAP_YPOS:
        wmGetPartyWorldPos(NULL, &result);
        break;
    case METARULE_CURRENT_TOWN:
        if (wmGetPartyCurArea(&result) == -1) {
            debug_printf("\nIntextra: Error: metarule: current_town");
        }
        break;
    case METARULE_LANGUAGE_FILTER:
        config_get_value(&game_config, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_LANGUAGE_FILTER_KEY, &result);
        break;
    case METARULE_VIOLENCE_FILTER:
        config_get_value(&game_config, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_VIOLENCE_LEVEL_KEY, &result);
        break;
    case METARULE_WEAPON_DAMAGE_TYPE:
        if (1) {
            Object* object = (Object*)param;
            if (PID_TYPE(object->pid) == OBJ_TYPE_ITEM) {
                if (item_get_type(object) == ITEM_TYPE_WEAPON) {
                    result = item_w_damage_type(NULL, object);
                    break;
                }
            } else {
                if (art_id(OBJ_TYPE_MISC, 10, 0, 0, 0) == object->fid) {
                    result = DAMAGE_TYPE_EXPLOSION;
                    break;
                }
            }

            dbg_error(program, "metarule:w_damage_type", SCRIPT_ERROR_FOLLOWS);
            debug_printf("Not a weapon!");
        }
        break;
    case METARULE_CRITTER_BARTERS:
        if (1) {
            Object* object = (Object*)param;
            if (PID_TYPE(object->pid) == OBJ_TYPE_CRITTER) {
                Proto* proto;
                proto_ptr(object->pid, &proto);
                if ((proto->critter.data.flags & CRITTER_BARTER) != 0) {
                    result = 1;
                }
            }
        }
        break;
    case METARULE_CRITTER_KILL_TYPE:
        result = critterGetKillType((Object*)param);
        break;
    case METARULE_SET_CAR_CARRY_AMOUNT:
        if (1) {
            Proto* proto;
            if (proto_ptr(PROTO_ID_CAR_TRUNK, &proto) != -1) {
                proto->item.data.container.maxSize = param;
                result = 1;
            }
        }
        break;
    case METARULE_GET_CAR_CARRY_AMOUNT:
        if (1) {
            Proto* proto;
            if (proto_ptr(PROTO_ID_CAR_TRUNK, &proto) != -1) {
                result = proto->item.data.container.maxSize;
            }
        }
        break;
    }

    interpretPushLong(program, result);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x4598BC
static void op_anim(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to anim", program->name, arg);
        }
    }

    Object* obj = (Object*)data[2];
    int anim = data[1];
    int frame = data[0];

    if (obj == NULL) {
        dbg_error(program, "anim", SCRIPT_ERROR_OBJECT_IS_NULL);
        return;
    }

    if (anim < ANIM_COUNT) {
        CritterCombatData* combatData = NULL;
        if (PID_TYPE(obj->pid) == OBJ_TYPE_CRITTER) {
            combatData = &(obj->data.critter.combat);
        }

        anim = correctDeath(obj, anim, true);

        register_begin(ANIMATION_REQUEST_UNRESERVED);

        // TODO: Not sure about the purpose, why it handles knock down flag?
        if (frame == 0) {
            register_object_animate(obj, anim, 0);
            if (anim >= ANIM_FALL_BACK && anim <= ANIM_FALL_FRONT_BLOOD) {
                int fid = art_id(OBJ_TYPE_CRITTER, obj->fid & 0xFFF, anim + 28, (obj->fid & 0xF000) >> 12, (obj->fid & 0x70000000) >> 28);
                register_object_change_fid(obj, fid, -1);
            }

            if (combatData != NULL) {
                combatData->results &= DAM_KNOCKED_DOWN;
            }
        } else {
            int fid = art_id(FID_TYPE(obj->fid), obj->fid & 0xFFF, anim, (obj->fid & 0xF000) >> 12, (obj->fid & 0x70000000) >> 24);
            register_object_animate_reverse(obj, anim, 0);

            if (anim == ANIM_PRONE_TO_STANDING) {
                fid = art_id(FID_TYPE(obj->fid), obj->fid & 0xFFF, ANIM_FALL_FRONT_SF, (obj->fid & 0xF000) >> 12, (obj->fid & 0x70000000) >> 24);
            } else if (anim == ANIM_BACK_TO_STANDING) {
                fid = art_id(FID_TYPE(obj->fid), obj->fid & 0xFFF, ANIM_FALL_BACK_SF, (obj->fid & 0xF000) >> 12, (obj->fid & 0x70000000) >> 24);
            }

            if (combatData != NULL) {
                combatData->results |= DAM_KNOCKED_DOWN;
            }

            register_object_change_fid(obj, fid, -1);
        }

        register_end();
    } else if (anim == 1000) {
        if (frame < ROTATION_COUNT) {
            Rect rect;
            obj_set_rotation(obj, frame, &rect);
            tile_refresh_rect(&rect, map_elevation);
        }
    } else if (anim == 1010) {
        Rect rect;
        obj_set_frame(obj, frame, &rect);
        tile_refresh_rect(&rect, map_elevation);
    } else {
        int_debug("\nScript Error: %s: op_anim: anim out of range", program->name);
    }
}

// 0x459B5C
static void op_obj_carrying_pid_obj(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to obj_carrying_pid_obj", program->name, arg);
        }
    }

    Object* object = (Object*)data[1];
    int pid = data[0];

    Object* result = NULL;
    if (object != NULL) {
        result = inven_pid_is_carried(object, pid);
    } else {
        dbg_error(program, "obj_carrying_pid_obj", SCRIPT_ERROR_OBJECT_IS_NULL);
    }

    interpretPushLong(program, (int)result);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x459C20
static void op_reg_anim_func(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to reg_anim_func", program->name, arg);
        }
    }

    int cmd = data[1];
    int param = data[0];

    if (!isInCombat()) {
        switch (cmd) {
        case OP_REG_ANIM_FUNC_BEGIN:
            register_begin(param);
            break;
        case OP_REG_ANIM_FUNC_CLEAR:
            register_clear((Object*)param);
            break;
        case OP_REG_ANIM_FUNC_END:
            register_end();
            break;
        }
    }
}

// 0x459CD4
static void op_reg_anim_animate(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to reg_anim_animate", program->name, arg);
        }
    }

    Object* object = (Object*)data[2];
    int anim = data[1];
    int delay = data[0];

    if (!isInCombat()) {
        int violenceLevel = VIOLENCE_LEVEL_NONE;
        if (anim != 20 || object == NULL || object->pid != 0x100002F || (config_get_value(&game_config, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_VIOLENCE_LEVEL_KEY, &violenceLevel) && violenceLevel >= 2)) {
            if (object != NULL) {
                register_object_animate(object, anim, delay);
            } else {
                dbg_error(program, "reg_anim_animate", SCRIPT_ERROR_OBJECT_IS_NULL);
            }
        }
    }
}

// 0x459DC4
static void op_reg_anim_animate_reverse(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to reg_anim_animate_reverse", program->name, arg);
        }
    }

    Object* object = (Object*)data[2];
    int anim = data[1];
    int delay = data[0];

    if (!isInCombat()) {
        if (object != NULL) {
            register_object_animate_reverse(object, anim, delay);
        } else {
            dbg_error(program, "reg_anim_animate_reverse", SCRIPT_ERROR_OBJECT_IS_NULL);
        }
    }
}

// 0x459E74
static void op_reg_anim_obj_move_to_obj(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to reg_anim_obj_move_to_obj", program->name, arg);
        }
    }

    Object* object = (Object*)data[2];
    Object* dest = (Object*)data[1];
    int delay = data[0];

    if (!isInCombat()) {
        if (object != NULL) {
            register_object_move_to_object(object, dest, -1, delay);
        } else {
            dbg_error(program, "reg_anim_obj_move_to_obj", SCRIPT_ERROR_OBJECT_IS_NULL);
        }
    }
}

// 0x459F28
static void op_reg_anim_obj_run_to_obj(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to reg_anim_obj_run_to_obj", program->name, arg);
        }
    }

    Object* object = (Object*)data[2];
    Object* dest = (Object*)data[1];
    int delay = data[0];

    if (!isInCombat()) {
        if (object != NULL) {
            register_object_run_to_object(object, dest, -1, delay);
        } else {
            dbg_error(program, "reg_anim_obj_run_to_obj", SCRIPT_ERROR_OBJECT_IS_NULL);
        }
    }
}

// 0x459FDC
static void op_reg_anim_obj_move_to_tile(Program* prg)
{
    opcode_t opcode[3];
    int data[3];

    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(prg);
        data[arg] = interpretPopLong(prg);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(prg, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to reg_anim_obj_move_to_tile", prg->name, arg);
        }
    }

    Object* object = (Object*)data[2];
    int tile = data[1];
    int delay = data[0];

    if (!isInCombat()) {
        if (object != NULL) {
            register_object_move_to_tile(object, tile, object->elevation, -1, delay);
        } else {
            dbg_error(prg, "reg_anim_obj_move_to_tile", SCRIPT_ERROR_OBJECT_IS_NULL);
        }
    }
}

// 0x45A094
static void op_reg_anim_obj_run_to_tile(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to reg_anim_obj_run_to_tile", program->name, arg);
        }
    }

    Object* object = (Object*)data[2];
    int tile = data[1];
    int delay = data[0];

    if (!isInCombat()) {
        if (object != NULL) {
            register_object_run_to_tile(object, tile, object->elevation, -1, delay);
        } else {
            dbg_error(program, "reg_anim_obj_run_to_tile", SCRIPT_ERROR_OBJECT_IS_NULL);
        }
    }
}

// 0x45A14C
static void op_play_gmovie(Program* program)
{
    // 0x453F9C
    static const unsigned short word_453F9C[MOVIE_COUNT] = {
        GAME_MOVIE_FADE_IN | GAME_MOVIE_FADE_OUT | GAME_MOVIE_PAUSE_MUSIC,
        GAME_MOVIE_FADE_IN | GAME_MOVIE_FADE_OUT | GAME_MOVIE_PAUSE_MUSIC,
        GAME_MOVIE_FADE_IN | GAME_MOVIE_FADE_OUT | GAME_MOVIE_PAUSE_MUSIC,
        GAME_MOVIE_FADE_IN | GAME_MOVIE_FADE_OUT | GAME_MOVIE_PAUSE_MUSIC,
        GAME_MOVIE_FADE_IN | GAME_MOVIE_FADE_OUT | GAME_MOVIE_PAUSE_MUSIC,
        GAME_MOVIE_FADE_IN | GAME_MOVIE_FADE_OUT | GAME_MOVIE_PAUSE_MUSIC,
        GAME_MOVIE_FADE_IN | GAME_MOVIE_FADE_OUT | GAME_MOVIE_PAUSE_MUSIC,
        GAME_MOVIE_FADE_IN | GAME_MOVIE_FADE_OUT | GAME_MOVIE_PAUSE_MUSIC,
        GAME_MOVIE_FADE_IN | GAME_MOVIE_FADE_OUT | GAME_MOVIE_PAUSE_MUSIC,
        GAME_MOVIE_FADE_IN | GAME_MOVIE_FADE_OUT | GAME_MOVIE_PAUSE_MUSIC,
        GAME_MOVIE_FADE_IN | GAME_MOVIE_FADE_OUT | GAME_MOVIE_PAUSE_MUSIC,
        GAME_MOVIE_FADE_IN | GAME_MOVIE_PAUSE_MUSIC,
        GAME_MOVIE_FADE_IN | GAME_MOVIE_FADE_OUT | GAME_MOVIE_PAUSE_MUSIC,
        GAME_MOVIE_FADE_IN | GAME_MOVIE_FADE_OUT | GAME_MOVIE_PAUSE_MUSIC,
        GAME_MOVIE_FADE_IN | GAME_MOVIE_FADE_OUT | GAME_MOVIE_PAUSE_MUSIC,
        GAME_MOVIE_FADE_IN | GAME_MOVIE_FADE_OUT | GAME_MOVIE_PAUSE_MUSIC,
        GAME_MOVIE_FADE_IN | GAME_MOVIE_FADE_OUT | GAME_MOVIE_PAUSE_MUSIC,
    };

    program->flags |= PROGRAM_FLAG_0x20;

    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to play_gmovie", program->name);
    }

    gdialogDisableBK();

    if (gmovie_play(data, word_453F9C[data]) == -1) {
        debug_printf("\nError playing movie %d!", data);
    }

    gdialogEnableBK();

    program->flags &= ~PROGRAM_FLAG_0x20;
}

// 0x45A200
static void op_add_mult_objs_to_inven(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to add_mult_objs_to_inven", program->name, arg);
        }
    }

    Object* object = (Object*)data[2];
    Object* item = (Object*)data[1];
    int quantity = data[0];

    if (object == NULL || item == NULL) {
        return;
    }

    if (quantity < 0) {
        quantity = 1;
    } else if (quantity > 99999) {
        quantity = 500;
    }

    if (item_add_force(object, item, quantity) == 0) {
        Rect rect;
        obj_disconnect(item, &rect);
        tile_refresh_rect(&rect, item->elevation);
    }
}

// 0x45A2D4
static void op_rm_mult_objs_from_inven(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to rm_mult_objs_from_inven", program->name, arg);
        }
    }

    Object* owner = (Object*)data[2];
    Object* item = (Object*)data[1];
    int quantityToRemove = data[0];

    if (owner == NULL || item == NULL) {
        // FIXME: Ruined stack.
        return;
    }

    bool itemWasEquipped = (item->flags & OBJECT_EQUIPPED) != 0;

    int quantity = item_count(owner, item);
    if (quantity > quantityToRemove) {
        quantity = quantityToRemove;
    }

    if (quantity != 0) {
        if (item_remove_mult(owner, item, quantity) == 0) {
            Rect updatedRect;
            obj_connect(item, 1, 0, &updatedRect);
            if (itemWasEquipped) {
                if (owner == obj_dude) {
                    bool animated = !game_ui_is_disabled();
                    intface_update_items(animated, INTERFACE_ITEM_ACTION_DEFAULT, INTERFACE_ITEM_ACTION_DEFAULT);
                }
            }
        }
    }

    interpretPushLong(program, quantity);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x45A40C
static void op_get_month(Program* program)
{
    int month;
    game_time_date(&month, NULL, NULL);

    interpretPushLong(program, month);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x45A43C
static void op_get_day(Program* program)
{
    int day;
    game_time_date(NULL, &day, NULL);

    interpretPushLong(program, day);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x45A46C
static void op_explosion(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to explosion", program->name, arg);
        }
    }

    int tile = data[2];
    int elevation = data[1];
    int maxDamage = data[0];

    if (tile == -1) {
        debug_printf("\nError: explosion: bad tile_num!");
        return;
    }

    int minDamage = 1;
    if (maxDamage == 0) {
        minDamage = 0;
    }

    scripts_request_explosion(tile, elevation, minDamage, maxDamage);
}

// 0x45A528
static void op_days_since_visited(Program* program)
{
    int days;

    if (map_data.lastVisitTime != 0) {
        days = (game_time() - map_data.lastVisitTime) / GAME_TIME_TICKS_PER_DAY;
    } else {
        days = -1;
    }

    interpretPushLong(program, days);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x45A56C
static void op_gsay_start(Program* program)
{
    program->flags |= PROGRAM_FLAG_0x20;

    if (gdialogStart() != 0) {
        program->flags &= ~PROGRAM_FLAG_0x20;
        interpretError("Error starting dialog.");
    }

    program->flags &= ~PROGRAM_FLAG_0x20;
}

// 0x45A5B0
static void op_gsay_end(Program* program)
{
    program->flags |= PROGRAM_FLAG_0x20;
    gdialogGo();
    program->flags &= ~PROGRAM_FLAG_0x20;
}

// 0x45A5D4
static void op_gsay_reply(Program* program)
{
    program->flags |= PROGRAM_FLAG_0x20;

    opcode_t opcode[2];
    int data[2];

    char* string = NULL;
    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            if (arg == 0) {
                if ((opcode[arg] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
                    string = interpretGetString(program, opcode[arg], data[arg]);
                } else {
                    interpretError("script error: %s: invalid arg %d to gsay_reply", program->name, arg);
                }
            } else {
                interpretError("script error: %s: invalid arg %d to gsay_reply", program->name, arg);
            }
        }
    }

    int messageListId = data[1];
    int messageId = data[0];

    if (string != NULL) {
        gdialogReplyStr(program, messageListId, string);
    } else {
        gdialogReply(program, messageListId, messageId);
    }

    program->flags &= ~PROGRAM_FLAG_0x20;
}

// 0x45A6C4
static void op_gsay_option(Program* program)
{
    program->flags |= PROGRAM_FLAG_0x20;

    opcode_t opcode[4];
    int data[4];

    // TODO: Original code is slightly different, does not use loop for first
    // two args, but uses loop for two last args.
    char* string = NULL;
    for (int arg = 0; arg < 4; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            if (arg == 2) {
                if ((opcode[arg] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
                    string = interpretGetString(program, opcode[arg], data[arg]);
                } else {
                    interpretError("script error: %s: invalid arg %d to gsay_option", program->name, arg);
                }
            } else {
                interpretError("script error: %s: invalid arg %d to gsay_option", program->name, arg);
            }
        }
    }

    int messageListId = data[3];
    int messageId = data[2];
    int proc = data[1];
    int reaction = data[0];

    // TODO: Not sure about this, needs testing.
    if ((opcode[1] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        char* procName = interpretGetString(program, opcode[1], data[1]);
        if (string != NULL) {
            gdialogOptionStr(data[3], string, procName, reaction);
        } else {
            gdialogOption(data[3], data[2], procName, reaction);
        }
        program->flags &= ~PROGRAM_FLAG_0x20;
        return;
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid arg 3 to sayOption");
        program->flags &= ~PROGRAM_FLAG_0x20;
        return;
    }

    if (string != NULL) {
        gdialogOptionProcStr(data[3], string, proc, reaction);
        program->flags &= ~PROGRAM_FLAG_0x20;
    } else {
        gdialogOptionProc(data[3], data[2], proc, reaction);
        program->flags &= ~PROGRAM_FLAG_0x20;
    }
}

// 0x45A8AC
static void op_gsay_message(Program* program)
{
    program->flags |= PROGRAM_FLAG_0x20;

    opcode_t opcode[3];
    int data[3];

    char* string = NULL;

    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            if (arg == 1) {
                if ((opcode[arg] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
                    string = interpretGetString(program, opcode[arg], data[arg]);
                } else {
                    interpretError("script error: %s: invalid arg %d to gsay_message", program->name, arg);
                }
            } else {
                interpretError("script error: %s: invalid arg %d to gsay_message", program->name, arg);
            }
        }
    }

    int messageListId = data[2];
    int messageId = data[1];
    int reaction = data[0];

    if (string != NULL) {
        gdialogReplyStr(program, messageListId, string);
    } else {
        gdialogReply(program, messageListId, messageId);
    }

    gdialogOption(-2, -2, NULL, 50);
    gdialogSayMessage();

    program->flags &= ~PROGRAM_FLAG_0x20;
}

// 0x45A9B4
static void op_giq_option(Program* program)
{
    program->flags |= PROGRAM_FLAG_0x20;

    opcode_t opcode[5];
    int data[5];

    char* string = NULL;

    for (int arg = 0; arg < 5; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            if (arg == 2) {
                if ((opcode[arg] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
                    string = interpretGetString(program, opcode[arg], data[arg]);
                } else {
                    interpretError("script error: %s: invalid arg %d to giq_option", program->name, arg);
                }
            } else {
                interpretError("script error: %s: invalid arg %d to giq_option", program->name, arg);
            }
        }
    }

    int iq = data[4];
    int messageListId = data[3];
    int messageId = data[2];
    int proc = data[1];
    int reaction = data[0];

    int intelligence = critterGetStat(obj_dude, STAT_INTELLIGENCE);
    intelligence += perk_level(obj_dude, PERK_SMOOTH_TALKER);

    if (iq < 0) {
        if (-intelligence < iq) {
            program->flags &= ~PROGRAM_FLAG_0x20;
            return;
        }
    } else {
        if (intelligence < iq) {
            program->flags &= ~PROGRAM_FLAG_0x20;
            return;
        }
    }

    if ((opcode[1] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        char* procName = interpretGetString(program, opcode[1], data[1]);
        if (string != NULL) {
            gdialogOptionStr(messageListId, string, procName, reaction);
        } else {
            gdialogOption(messageListId, messageId, procName, reaction);
        }
        program->flags &= ~PROGRAM_FLAG_0x20;
        return;
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid arg 4 to sayOption");
        program->flags &= ~PROGRAM_FLAG_0x20;
        return;
    }

    if (string != NULL) {
        gdialogOptionProcStr(messageListId, string, proc, reaction);
        program->flags &= ~PROGRAM_FLAG_0x20;
    } else {
        gdialogOptionProc(messageListId, messageId, proc, reaction);
        program->flags &= ~PROGRAM_FLAG_0x20;
    }
}

// 0x45AB90
static void op_poison(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to poison", program->name, arg);
        }
    }

    Object* obj = (Object*)data[1];
    int amount = data[0];

    if (obj == NULL) {
        dbg_error(program, "poison", SCRIPT_ERROR_OBJECT_IS_NULL);
        return;
    }

    if (critter_adjust_poison(obj, amount) != 0) {
        debug_printf("\nScript Error: poison: adjust failed!");
    }
}

// 0x45AC44
static void op_get_poison(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to get_poison", program->name);
    }

    Object* obj = (Object*)data;

    int poison = 0;
    if (obj != NULL) {
        if (PID_TYPE(obj->pid) == OBJ_TYPE_CRITTER) {
            poison = critter_get_poison(obj);
        } else {
            debug_printf("\nScript Error: get_poison: who is not a critter!");
        }
    } else {
        dbg_error(program, "get_poison", SCRIPT_ERROR_OBJECT_IS_NULL);
    }

    interpretPushLong(program, poison);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x45ACF4
static void op_party_add(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to party_add", program->name);
    }

    Object* object = (Object*)data;
    if (object == NULL) {
        dbg_error(program, "party_add", SCRIPT_ERROR_OBJECT_IS_NULL);
        return;
    }

    partyMemberAdd(object);
}

// 0x45AD68
static void op_party_remove(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to party_remove", program->name);
    }

    Object* object = (Object*)data;
    if (object == NULL) {
        dbg_error(program, "party_remove", SCRIPT_ERROR_OBJECT_IS_NULL);
        return;
    }

    partyMemberRemove(object);
}

// 0x45ADDC
static void op_reg_anim_animate_forever(Program* prg)
{
    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(prg);
        data[arg] = interpretPopLong(prg);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(prg, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to reg_anim_animate_forever", prg->name, arg);
        }
    }

    Object* obj = (Object*)data[1];
    int anim = data[0];

    if (!isInCombat()) {
        if (obj != NULL) {
            register_object_animate_forever(obj, anim, -1);
        } else {
            dbg_error(prg, "reg_anim_animate_forever", SCRIPT_ERROR_OBJECT_IS_NULL);
        }
    }
}

// 0x45AE8C
static void op_critter_injure(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to critter_injure", program->name, arg);
        }
    }

    Object* critter = (Object*)data[1];
    int flags = data[0];

    if (critter == NULL) {
        dbg_error(program, "critter_injure", SCRIPT_ERROR_OBJECT_IS_NULL);
        return;
    }

    bool reverse = (flags & DAM_PERFORM_REVERSE) != 0;

    flags &= DAM_CRIP;

    if (reverse) {
        critter->data.critter.combat.results &= ~flags;
    } else {
        critter->data.critter.combat.results |= flags;
    }

    if (critter == obj_dude) {
        if ((flags & DAM_CRIP_ARM_ANY) != 0) {
            int leftItemAction;
            int rightItemAction;
            intface_get_item_states(&leftItemAction, &rightItemAction);
            intface_update_items(true, leftItemAction, rightItemAction);
        }
    }
}

// 0x45AF7C
static void op_combat_is_initialized(Program* program)
{
    interpretPushLong(program, isInCombat());
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x45AFA0
static void op_gdialog_barter(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to gdialog_barter", program->name);
    }

    if (gdActivateBarter(data) == -1) {
        debug_printf("\nScript Error: gdialog_barter: failed");
    }
}

// 0x45B010
static void op_difficulty_level(Program* program)
{
    int gameDifficulty;
    if (!config_get_value(&game_config, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_GAME_DIFFICULTY_KEY, &gameDifficulty)) {
        gameDifficulty = GAME_DIFFICULTY_NORMAL;
    }

    interpretPushLong(program, gameDifficulty);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x45B05C
static void op_running_burning_guy(Program* program)
{
    int runningBurningGuy;
    if (!config_get_value(&game_config, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_RUNNING_BURNING_GUY_KEY, &runningBurningGuy)) {
        runningBurningGuy = 1;
    }

    interpretPushLong(program, runningBurningGuy);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x45B0A8
static void op_inven_unwield(Program* program)
{
    Object* obj;
    int v1;

    obj = scr_find_obj_from_program(program);
    v1 = 1;

    if (obj == obj_dude && !intface_is_item_right_hand()) {
        v1 = 0;
    }

    inven_unwield(obj, v1);
}

// 0x45B0D8
static void op_obj_is_locked(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to obj_is_locked", program->name);
    }

    Object* object = (Object*)data;

    bool locked = false;
    if (object != NULL) {
        locked = obj_is_locked(object);
    } else {
        dbg_error(program, "obj_is_locked", SCRIPT_ERROR_OBJECT_IS_NULL);
    }

    interpretPushLong(program, locked);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x45B16C
static void op_obj_lock(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to obj_lock", program->name);
    }

    Object* object = (Object*)data;

    if (object != NULL) {
        obj_lock(object);
    } else {
        dbg_error(program, "obj_lock", SCRIPT_ERROR_OBJECT_IS_NULL);
    }
}

// 0x45B1E0
static void op_obj_unlock(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to obj_unlock", program->name);
    }

    Object* object = (Object*)data;

    if (object != NULL) {
        obj_unlock(object);
    } else {
        dbg_error(program, "obj_unlock", SCRIPT_ERROR_OBJECT_IS_NULL);
    }
}

// 0x45B254
static void op_obj_is_open(Program* s)
{
    opcode_t opcode = interpretPopShort(s);
    int data = interpretPopLong(s);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(s, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to obj_is_open", s->name);
    }

    Object* object = (Object*)data;

    bool isOpen = false;
    if (object != NULL) {
        isOpen = obj_is_open(object);
    } else {
        dbg_error(s, "obj_is_open", SCRIPT_ERROR_OBJECT_IS_NULL);
    }

    interpretPushLong(s, isOpen);
    interpretPushShort(s, VALUE_TYPE_INT);
}

// 0x45B2E8
static void op_obj_open(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to obj_open", program->name);
    }

    Object* object = (Object*)data;

    if (object != NULL) {
        obj_open(object);
    } else {
        dbg_error(program, "obj_open", SCRIPT_ERROR_OBJECT_IS_NULL);
    }
}

// 0x45B35C
static void op_obj_close(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to obj_close", program->name);
    }

    Object* object = (Object*)data;

    if (object != NULL) {
        obj_close(object);
    } else {
        dbg_error(program, "obj_close", SCRIPT_ERROR_OBJECT_IS_NULL);
    }
}

// 0x45B3D0
static void op_game_ui_disable(Program* program)
{
    game_ui_disable(0);
}

// 0x45B3D8
static void op_game_ui_enable(Program* program)
{
    game_ui_enable();
}

// 0x45B3E0
static void op_game_ui_is_disabled(Program* program)
{
    interpretPushLong(program, game_ui_is_disabled());
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x45B404
static void op_gfade_out(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to gfade_out", program->name);
    }

    if (data != 0) {
        palette_fade_to(black_palette);
    } else {
        dbg_error(program, "gfade_out", SCRIPT_ERROR_OBJECT_IS_NULL);
    }
}

// 0x45B47C
static void op_gfade_in(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to gfade_in", program->name);
    }

    if (data != 0) {
        palette_fade_to(cmap);
    } else {
        dbg_error(program, "gfade_in", SCRIPT_ERROR_OBJECT_IS_NULL);
    }
}

// 0x45B4F4
static void op_item_caps_total(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to item_caps_total", program->name);
    }

    Object* object = (Object*)data;

    int amount = 0;
    if (object != NULL) {
        amount = item_caps_total(object);
    } else {
        dbg_error(program, "item_caps_total", SCRIPT_ERROR_OBJECT_IS_NULL);
    }

    interpretPushLong(program, amount);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x45B588
static void op_item_caps_adjust(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to item_caps_adjust", program->name, arg);
        }
    }

    Object* object = (Object*)data[1];
    int amount = data[0];

    int rc = -1;

    if (object != NULL) {
        rc = item_caps_adjust(object, amount);
    } else {
        dbg_error(program, "item_caps_adjust", SCRIPT_ERROR_OBJECT_IS_NULL);
    }

    interpretPushLong(program, rc);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x45B64C
static void op_anim_action_frame(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to anim_action_frame", program->name, arg);
        }
    }

    Object* object = (Object*)data[1];
    int anim = data[0];

    int actionFrame = 0;

    if (object != NULL) {
        int fid = art_id(FID_TYPE(object->fid), object->fid & 0xFFF, anim, 0, object->rotation);
        CacheEntry* frmHandle;
        Art* frm = art_ptr_lock(fid, &frmHandle);
        if (frm != NULL) {
            actionFrame = art_frame_action_frame(frm);
            art_ptr_unlock(frmHandle);
        }
    } else {
        dbg_error(program, "anim_action_frame", SCRIPT_ERROR_OBJECT_IS_NULL);
    }

    interpretPushLong(program, actionFrame);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x45B740
static void op_reg_anim_play_sfx(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if (arg == 1) {
            if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
                interpretError("script error: %s: invalid arg %d to reg_anim_play_sfx", program->name, arg);
            }
        } else {
            if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
                interpretError("script error: %s: invalid arg %d to reg_anim_play_sfx", program->name, arg);
            }
        }
    }

    Object* obj = (Object*)data[2];
    int name = data[1];
    int delay = data[0];

    char* soundEffectName = interpretGetString(program, opcode[1], name);
    if (soundEffectName == NULL) {
        dbg_error(program, "reg_anim_play_sfx", SCRIPT_ERROR_FOLLOWS);
        debug_printf(" Can't match string!");
    }

    if (obj != NULL) {
        register_object_play_sfx(obj, soundEffectName, delay);
    } else {
        dbg_error(program, "reg_anim_play_sfx", SCRIPT_ERROR_OBJECT_IS_NULL);
    }
}

// 0x45B840
static void op_critter_mod_skill(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to critter_mod_skill", program->name, arg);
        }
    }

    Object* critter = (Object*)data[2];
    int skill = data[1];
    int points = data[0];

    if (critter != NULL && points != 0) {
        if (PID_TYPE(critter->pid) == OBJ_TYPE_CRITTER) {
            if (critter == obj_dude) {
                int normalizedPoints = abs(points);
                if (skill_is_tagged(skill)) {
                    // Halve number of skill points. Increment/decrement skill
                    // points routines handle that.
                    normalizedPoints /= 2;
                }

                if (points > 0) {
                    // Increment skill points one by one.
                    for (int it = 0; it < normalizedPoints; it++) {
                        skill_inc_point_force(obj_dude, skill);
                    }
                } else {
                    // Decrement skill points one by one.
                    for (int it = 0; it < normalizedPoints; it++) {
                        skill_dec_point_force(obj_dude, skill);
                    }
                }

                // TODO: Checking for critter is dude twice probably means this
                // is inlined function.
                if (critter == obj_dude) {
                    int leftItemAction;
                    int rightItemAction;
                    intface_get_item_states(&leftItemAction, &rightItemAction);
                    intface_update_items(false, leftItemAction, rightItemAction);
                }
            } else {
                dbg_error(program, "critter_mod_skill", SCRIPT_ERROR_FOLLOWS);
                debug_printf(" Can't modify anyone except obj_dude!");
            }
        }
    } else {
        dbg_error(program, "critter_mod_skill", SCRIPT_ERROR_OBJECT_IS_NULL);
    }

    interpretPushLong(program, 0);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x45B9C4
static void op_sfx_build_char_name(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to sfx_build_char_name", program->name, arg);
        }
    }

    Object* obj = (Object*)data[2];
    int anim = data[1];
    int extra = data[0];

    int stringOffset = 0;

    if (obj != NULL) {
        char soundEffectName[16];
        strcpy(soundEffectName, gsnd_build_character_sfx_name(obj, anim, extra));
        stringOffset = interpretAddString(program, soundEffectName);
    } else {
        dbg_error(program, "sfx_build_char_name", SCRIPT_ERROR_OBJECT_IS_NULL);
    }

    interpretPushLong(program, stringOffset);
    interpretPushShort(program, VALUE_TYPE_DYNAMIC_STRING);
}

// 0x45BAA8
static void op_sfx_build_ambient_name(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to sfx_build_ambient_name", program->name);
    }

    char* baseName = interpretGetString(program, opcode, data);

    char soundEffectName[16];
    strcpy(soundEffectName, gsnd_build_ambient_sfx_name(baseName));

    int stringOffset = interpretAddString(program, soundEffectName);

    interpretPushLong(program, stringOffset);
    interpretPushShort(program, VALUE_TYPE_DYNAMIC_STRING);
}

// 0x45BB54
static void op_sfx_build_interface_name(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to sfx_build_interface_name", program->name);
    }

    char* baseName = interpretGetString(program, opcode, data);

    char soundEffectName[16];
    strcpy(soundEffectName, gsnd_build_interface_sfx_name(baseName));

    int stringOffset = interpretAddString(program, soundEffectName);

    interpretPushLong(program, stringOffset);
    interpretPushShort(program, VALUE_TYPE_DYNAMIC_STRING);
}

// 0x45BC00
static void op_sfx_build_item_name(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to sfx_build_item_name", program->name);
    }

    const char* baseName = interpretGetString(program, opcode, data);

    char soundEffectName[16];
    strcpy(soundEffectName, gsnd_build_interface_sfx_name(baseName));

    int stringOffset = interpretAddString(program, soundEffectName);

    interpretPushLong(program, stringOffset);
    interpretPushShort(program, VALUE_TYPE_DYNAMIC_STRING);
}

// 0x45BCAC
static void op_sfx_build_weapon_name(Program* program)
{
    opcode_t opcode[4];
    int data[4];

    for (int arg = 0; arg < 4; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to sfx_build_weapon_name", program->name, arg);
        }
    }

    int weaponSfxType = data[3];
    Object* weapon = (Object*)data[2];
    int hitMode = data[1];
    Object* target = (Object*)data[0];

    char soundEffectName[16];
    strcpy(soundEffectName, gsnd_build_weapon_sfx_name(weaponSfxType, weapon, hitMode, target));

    int stringOffset = interpretAddString(program, soundEffectName);

    interpretPushLong(program, stringOffset);
    interpretPushShort(program, VALUE_TYPE_DYNAMIC_STRING);
}

// 0x45BD7C
static void op_sfx_build_scenery_name(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to sfx_build_scenery_name", program->name, arg);
        }
    }

    int action = data[1];
    int actionType = data[0];

    char* baseName = interpretGetString(program, opcode[2], data[2]);

    char soundEffectName[16];
    strcpy(soundEffectName, gsnd_build_scenery_sfx_name(actionType, action, baseName));

    int stringOffset = interpretAddString(program, soundEffectName);

    interpretPushLong(program, stringOffset);
    interpretPushShort(program, VALUE_TYPE_DYNAMIC_STRING);
}

// 0x45BE58
static void op_sfx_build_open_name(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to sfx_build_open_name", program->name, arg);
        }
    }

    Object* object = (Object*)data[1];
    int action = data[0];

    int stringOffset = 0;

    if (object != NULL) {
        char soundEffectName[16];
        strcpy(soundEffectName, gsnd_build_open_sfx_name(object, action));

        stringOffset = interpretAddString(program, soundEffectName);
    } else {
        dbg_error(program, "sfx_build_open_name", SCRIPT_ERROR_OBJECT_IS_NULL);
    }

    interpretPushLong(program, stringOffset);
    interpretPushShort(program, VALUE_TYPE_DYNAMIC_STRING);
}

// 0x45BF38
static void op_attack_setup(Program* program)
{
    opcode_t opcodes[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcodes[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcodes[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcodes[arg], data[arg]);
        }

        if ((opcodes[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to attack_setup", program->name, arg);
        }
    }

    Object* attacker = (Object*)data[1];
    Object* defender = (Object*)data[0];

    program->flags |= PROGRAM_FLAG_0x20;

    if (attacker != NULL) {
        if (!critter_is_active(attacker) || (attacker->flags & OBJECT_HIDDEN) != 0) {
            debug_printf("\n   But is already dead or invisible");
            program->flags &= ~PROGRAM_FLAG_0x20;
            return;
        }

        if (!critter_is_active(defender) || (defender->flags & OBJECT_HIDDEN) != 0) {
            debug_printf("\n   But target is already dead or invisible");
            program->flags &= ~PROGRAM_FLAG_0x20;
            return;
        }

        if ((defender->data.critter.combat.maneuver & CRITTER_MANUEVER_FLEEING) != 0) {
            debug_printf("\n   But target is AFRAID");
            program->flags &= ~PROGRAM_FLAG_0x20;
            return;
        }

        if (isInCombat()) {
            if ((attacker->data.critter.combat.maneuver & CRITTER_MANEUVER_0x01) == 0) {
                attacker->data.critter.combat.maneuver |= CRITTER_MANEUVER_0x01;
                attacker->data.critter.combat.whoHitMe = defender;
            }
        } else {
            STRUCT_664980 attack;
            attack.attacker = attacker;
            attack.defender = defender;
            attack.actionPointsBonus = 0;
            attack.accuracyBonus = 0;
            attack.damageBonus = 0;
            attack.minDamage = 0;
            attack.maxDamage = INT_MAX;

            // FIXME: Something bad here, when attacker and defender are
            // the same object, these objects are used as flags, which
            // are later used in 0x422F3C as flags of defender.
            if (data[1] == data[0]) {
                attack.field_1C = 1;
                attack.field_20 = data[1];
                attack.field_24 = data[0];
            } else {
                attack.field_1C = 0;
            }

            scripts_request_combat(&attack);
        }
    }

    program->flags &= ~PROGRAM_FLAG_0x20;
}

// 0x45C0E8
static void op_destroy_mult_objs(Program* program)
{
    program->flags |= PROGRAM_FLAG_0x20;

    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to destroy_mult_objs", program->name, arg);
        }
    }

    Object* object = (Object*)data[1];
    int quantity = data[0];

    Object* self = scr_find_obj_from_program(program);
    bool isSelf = self == object;

    int result = 0;

    if (PID_TYPE(object->pid) == OBJ_TYPE_CRITTER) {
        combat_delete_critter(object);
    }

    Object* owner = obj_top_environment(object);
    if (owner != NULL) {
        int quantityToDestroy = item_count(owner, object);
        if (quantityToDestroy > quantity) {
            quantityToDestroy = quantity;
        }

        item_remove_mult(owner, object, quantityToDestroy);

        if (owner == obj_dude) {
            bool animated = !game_ui_is_disabled();
            intface_update_items(animated, INTERFACE_ITEM_ACTION_DEFAULT, INTERFACE_ITEM_ACTION_DEFAULT);
        }

        obj_connect(object, 1, 0, NULL);

        if (isSelf) {
            object->sid = -1;
            object->flags |= (OBJECT_HIDDEN | OBJECT_TEMPORARY);
        } else {
            register_clear(object);
            obj_erase_object(object, NULL);
        }

        result = quantityToDestroy;
    } else {
        register_clear(object);

        Rect rect;
        obj_erase_object(object, &rect);
        tile_refresh_rect(&rect, map_elevation);
    }

    interpretPushLong(program, result);
    interpretPushShort(program, VALUE_TYPE_INT);

    program->flags &= ~PROGRAM_FLAG_0x20;

    if (isSelf) {
        program->flags |= PROGRAM_FLAG_0x0100;
    }
}

// 0x45C290
static void op_use_obj_on_obj(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to use_obj_on_obj", program->name, arg);
        }
    }

    Object* item = (Object*)data[1];
    Object* target = (Object*)data[0];

    if (item == NULL) {
        dbg_error(program, "use_obj_on_obj", SCRIPT_ERROR_OBJECT_IS_NULL);
        return;
    }

    if (target == NULL) {
        dbg_error(program, "use_obj_on_obj", SCRIPT_ERROR_OBJECT_IS_NULL);
        return;
    }

    Script* script;
    int sid = scr_find_sid_from_program(program);
    if (scr_ptr(sid, &script) == -1) {
        // FIXME: Should be SCRIPT_ERROR_CANT_MATCH_PROGRAM_TO_SID.
        dbg_error(program, "use_obj_on_obj", SCRIPT_ERROR_OBJECT_IS_NULL);
        return;
    }

    Object* self = scr_find_obj_from_program(program);
    if (PID_TYPE(self->pid) == OBJ_TYPE_CRITTER) {
        action_use_an_item_on_object(self, target, item);
    } else {
        obj_use_item_on(self, target, item);
    }
}

// 0x45C3B0
static void op_endgame_slideshow(Program* program)
{
    program->flags |= PROGRAM_FLAG_0x20;
    scripts_request_endgame_slideshow();
    program->flags &= ~PROGRAM_FLAG_0x20;
}

// 0x45C3D0
static void op_move_obj_inven_to_obj(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to move_obj_inven_to_obj", program->name, arg);
        }
    }

    Object* object1 = (Object*)data[1];
    Object* object2 = (Object*)data[0];

    if (object1 == NULL) {
        dbg_error(program, "move_obj_inven_to_obj", SCRIPT_ERROR_OBJECT_IS_NULL);
        return;
    }

    if (object2 == NULL) {
        dbg_error(program, "move_obj_inven_to_obj", SCRIPT_ERROR_OBJECT_IS_NULL);
        return;
    }

    Object* oldArmor = NULL;
    Object* item2 = NULL;
    if (object1 == obj_dude) {
        oldArmor = inven_worn(object1);
    } else {
        item2 = inven_right_hand(object1);
    }

    if (object1 != obj_dude && item2 != NULL) {
        int flags = 0;
        if ((item2->flags & 0x01000000) != 0) {
            flags |= 0x01000000;
        }

        if ((item2->flags & 0x02000000) != 0) {
            flags |= 0x02000000;
        }

        correctFidForRemovedItem(object1, item2, flags);
    }

    item_move_all(object1, object2);

    if (object1 == obj_dude) {
        if (oldArmor != NULL) {
            adjust_ac(obj_dude, oldArmor, NULL);
        }

        proto_dude_update_gender();

        bool animated = !game_ui_is_disabled();
        intface_update_items(animated, INTERFACE_ITEM_ACTION_DEFAULT, INTERFACE_ITEM_ACTION_DEFAULT);
    }
}

// 0x45C54C
static void op_endgame_movie(Program* program)
{
    program->flags |= PROGRAM_FLAG_0x20;
    endgame_movie();
    program->flags &= ~PROGRAM_FLAG_0x20;
}

// 0x45C56C
static void op_obj_art_fid(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to obj_art_fid", program->name);
    }

    Object* object = (Object*)data;

    int fid = 0;
    if (object != NULL) {
        fid = object->fid;
    } else {
        dbg_error(program, "obj_art_fid", SCRIPT_ERROR_OBJECT_IS_NULL);
    }

    interpretPushLong(program, fid);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x45C5F8
static void op_art_anim(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to art_anim", program->name);
    }

    interpretPushLong(program, FID_ANIM_TYPE(data));
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x45C66C
static void op_party_member_obj(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to party_member_obj", program->name);
    }

    Object* object = partyMemberFindObjFromPid(data);
    interpretPushLong(program, (int)object);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x45C6DC
static void op_rotation_to_tile(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to rotation_to_tile", program->name, arg);
        }
    }

    int tile1 = data[1];
    int tile2 = data[0];

    int rotation = tile_dir(tile1, tile2);
    interpretPushLong(program, rotation);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x45C778
static void op_jam_lock(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to jam_lock", program->name);
    }

    Object* object = (Object*)data;

    obj_jam_lock(object);
}

// 0x45C7D4
static void op_gdialog_set_barter_mod(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to gdialog_set_barter_mod", program->name);
    }

    gdialogSetBarterMod(data);
}

// 0x45C830
static void op_combat_difficulty(Program* program)
{
    int combatDifficulty;
    if (!config_get_value(&game_config, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_COMBAT_DIFFICULTY_KEY, &combatDifficulty)) {
        combatDifficulty = 0;
    }

    interpretPushLong(program, combatDifficulty);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x45C878
static void op_obj_on_screen(Program* program)
{
    // 0x453FC0
    static Rect rect = { 0, 0, 640, 480 };

    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to obj_on_screen", program->name);
    }

    Object* object = (Object*)data;

    int result = 0;

    if (object != NULL) {
        if (map_elevation == object->elevation) {
            Rect objectRect;
            obj_bound(object, &objectRect);

            if (rect_inside_bound(&objectRect, &rect, &objectRect) == 0) {
                result = 1;
            }
        }
    } else {
        dbg_error(program, "obj_on_screen", SCRIPT_ERROR_OBJECT_IS_NULL);
    }

    //debug_printf("ObjOnScreen: %d\n", result);
    interpretPushLong(program, result);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x45C93C
static void op_critter_is_fleeing(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to critter_is_fleeing", program->name);
    }

    Object* obj = (Object*)data;

    bool fleeing = false;
    if (obj != NULL) {
        fleeing = (obj->data.critter.combat.maneuver & CRITTER_MANUEVER_FLEEING) != 0;
    } else {
        dbg_error(program, "critter_is_fleeing", SCRIPT_ERROR_OBJECT_IS_NULL);
    }

    interpretPushLong(program, fleeing);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x45C9DC
static void op_critter_set_flee_state(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to critter_set_flee_state", program->name, arg);
        }
    }

    Object* object = (Object*)data[1];
    int fleeing = data[0];

    if (object != NULL) {
        if (fleeing != 0) {
            object->data.critter.combat.maneuver |= CRITTER_MANUEVER_FLEEING;
        } else {
            object->data.critter.combat.maneuver &= ~CRITTER_MANUEVER_FLEEING;
        }
    } else {
        dbg_error(program, "critter_set_flee_state", SCRIPT_ERROR_OBJECT_IS_NULL);
    }
}

// 0x45CA84
static void op_terminate_combat(Program* program)
{
    if (isInCombat()) {
        game_user_wants_to_quit = 1;
        Object* self = scr_find_obj_from_program(program);
        if (self != NULL) {
            if (PID_TYPE(self->pid) == OBJ_TYPE_CRITTER) {
                self->data.critter.combat.maneuver |= CRITTER_MANEUVER_STOP_ATTACKING;
                self->data.critter.combat.whoHitMe = NULL;
                combatAIInfoSetLastTarget(self, NULL);
            }
        }
    }
}

// 0x45CAC8
static void op_debug_msg(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        interpretError("script error: %s: invalid arg to debug_msg", program->name);
    }

    char* string = interpretGetString(program, opcode, data);

    if (string != NULL) {
        bool showScriptMessages = false;
        configGetBool(&game_config, GAME_CONFIG_DEBUG_KEY, GAME_CONFIG_SHOW_SCRIPT_MESSAGES_KEY, &showScriptMessages);
        if (showScriptMessages) {
            debug_printf("\n");
            debug_printf(string);
        }
    }
}

// 0x45CB70
static void op_critter_stop_attacking(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to critter_stop_attacking", program->name);
    }

    Object* obj = (Object*)data;

    if (obj != NULL) {
        obj->data.critter.combat.maneuver |= CRITTER_MANEUVER_STOP_ATTACKING;
        obj->data.critter.combat.whoHitMe = NULL;
        combatAIInfoSetLastTarget(obj, NULL);
    } else {
        dbg_error(program, "critter_stop_attacking", SCRIPT_ERROR_OBJECT_IS_NULL);
    }
}

// 0x45CBF8
static void op_tile_contains_pid_obj(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }

        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("script error: %s: invalid arg %d to tile_contains_pid_obj", program->name, arg);
        }
    }

    int tile = data[2];
    int elevation = data[1];
    int pid = data[0];
    Object* found = NULL;

    if (tile != -1) {
        Object* object = obj_find_first_at_tile(elevation, tile);
        while (object != NULL) {
            if (object->pid == pid) {
                found = object;
                break;
            }
            object = obj_find_next_at_tile();
        }
    }

    interpretPushLong(program, (int)found);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x45CCC8
static void op_obj_name(Program* program)
{
    // 0x518F04
    static char* strName = _aCritter;

    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to obj_name", program->name);
    }

    Object* obj = (Object*)data;
    if (obj != NULL) {
        strName = object_name(obj);
    } else {
        dbg_error(program, "obj_name", SCRIPT_ERROR_OBJECT_IS_NULL);
    }

    int stringOffset = interpretAddString(program, strName);

    interpretPushLong(program, stringOffset);
    interpretPushShort(program, VALUE_TYPE_DYNAMIC_STRING);
}

// 0x45CD64
static void op_get_pc_stat(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("script error: %s: invalid arg to get_pc_stat", program->name);
    }

    int value = stat_pc_get(data);
    interpretPushLong(program, value);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x45CDD4
void intExtraClose()
{
}

// 0x45CDD8
void initIntExtra()
{
    interpretAddFunc(0x80A1, op_give_exp_points);
    interpretAddFunc(0x80A2, op_scr_return);
    interpretAddFunc(0x80A3, op_play_sfx);
    interpretAddFunc(0x80A4, op_obj_name);
    interpretAddFunc(0x80A5, op_sfx_build_open_name);
    interpretAddFunc(0x80A6, op_get_pc_stat);
    interpretAddFunc(0x80A7, op_tile_contains_pid_obj);
    interpretAddFunc(0x80A8, op_set_map_start);
    interpretAddFunc(0x80A9, op_override_map_start);
    interpretAddFunc(0x80AA, op_has_skill);
    interpretAddFunc(0x80AB, op_using_skill);
    interpretAddFunc(0x80AC, op_roll_vs_skill);
    interpretAddFunc(0x80AD, op_skill_contest);
    interpretAddFunc(0x80AE, op_do_check);
    interpretAddFunc(0x80AF, op_is_success);
    interpretAddFunc(0x80B0, op_is_critical);
    interpretAddFunc(0x80B1, op_how_much);
    interpretAddFunc(0x80B2, op_mark_area_known);
    interpretAddFunc(0x80B3, op_reaction_influence);
    interpretAddFunc(0x80B4, op_random);
    interpretAddFunc(0x80B5, op_roll_dice);
    interpretAddFunc(0x80B6, op_move_to);
    interpretAddFunc(0x80B7, op_create_object_sid);
    interpretAddFunc(0x80B8, op_display_msg);
    interpretAddFunc(0x80B9, op_script_overrides);
    interpretAddFunc(0x80BA, op_obj_is_carrying_obj_pid);
    interpretAddFunc(0x80BB, op_tile_contains_obj_pid);
    interpretAddFunc(0x80BC, op_self_obj);
    interpretAddFunc(0x80BD, op_source_obj);
    interpretAddFunc(0x80BE, op_target_obj);
    interpretAddFunc(0x80BF, op_dude_obj);
    interpretAddFunc(0x80C0, op_obj_being_used_with);
    interpretAddFunc(0x80C1, op_local_var);
    interpretAddFunc(0x80C2, op_set_local_var);
    interpretAddFunc(0x80C3, op_map_var);
    interpretAddFunc(0x80C4, op_set_map_var);
    interpretAddFunc(0x80C5, op_global_var);
    interpretAddFunc(0x80C6, op_set_global_var);
    interpretAddFunc(0x80C7, op_script_action);
    interpretAddFunc(0x80C8, op_obj_type);
    interpretAddFunc(0x80C9, op_obj_item_subtype);
    interpretAddFunc(0x80CA, op_get_critter_stat);
    interpretAddFunc(0x80CB, op_set_critter_stat);
    interpretAddFunc(0x80CC, op_animate_stand_obj);
    interpretAddFunc(0x80CD, op_animate_stand_reverse_obj);
    interpretAddFunc(0x80CE, op_animate_move_obj_to_tile);
    interpretAddFunc(0x80CF, op_tile_in_tile_rect);
    interpretAddFunc(0x80D0, op_attack);
    interpretAddFunc(0x80D1, op_make_daytime);
    interpretAddFunc(0x80D2, op_tile_distance);
    interpretAddFunc(0x80D3, op_tile_distance_objs);
    interpretAddFunc(0x80D4, op_tile_num);
    interpretAddFunc(0x80D5, op_tile_num_in_direction);
    interpretAddFunc(0x80D6, op_pickup_obj);
    interpretAddFunc(0x80D7, op_drop_obj);
    interpretAddFunc(0x80D8, op_add_obj_to_inven);
    interpretAddFunc(0x80D9, op_rm_obj_from_inven);
    interpretAddFunc(0x80DA, op_wield_obj_critter);
    interpretAddFunc(0x80DB, op_use_obj);
    interpretAddFunc(0x80DC, op_obj_can_see_obj);
    interpretAddFunc(0x80DD, op_attack);
    interpretAddFunc(0x80DE, op_start_gdialog);
    interpretAddFunc(0x80DF, op_end_dialogue);
    interpretAddFunc(0x80E0, op_dialogue_reaction);
    interpretAddFunc(0x80E1, op_metarule3);
    interpretAddFunc(0x80E2, op_set_map_music);
    interpretAddFunc(0x80E3, op_set_obj_visibility);
    interpretAddFunc(0x80E4, op_load_map);
    interpretAddFunc(0x80E5, op_wm_area_set_pos);
    interpretAddFunc(0x80E6, op_set_exit_grids);
    interpretAddFunc(0x80E7, op_anim_busy);
    interpretAddFunc(0x80E8, op_critter_heal);
    interpretAddFunc(0x80E9, op_set_light_level);
    interpretAddFunc(0x80EA, op_game_time);
    interpretAddFunc(0x80EB, op_game_time_in_seconds);
    interpretAddFunc(0x80EC, op_elevation);
    interpretAddFunc(0x80ED, op_kill_critter);
    interpretAddFunc(0x80EE, op_kill_critter_type);
    interpretAddFunc(0x80EF, op_critter_damage);
    interpretAddFunc(0x80F0, op_add_timer_event);
    interpretAddFunc(0x80F1, op_rm_timer_event);
    interpretAddFunc(0x80F2, op_game_ticks);
    interpretAddFunc(0x80F3, op_has_trait);
    interpretAddFunc(0x80F4, op_destroy_object);
    interpretAddFunc(0x80F5, op_obj_can_hear_obj);
    interpretAddFunc(0x80F6, op_game_time_hour);
    interpretAddFunc(0x80F7, op_fixed_param);
    interpretAddFunc(0x80F8, op_tile_is_visible);
    interpretAddFunc(0x80F9, op_dialogue_system_enter);
    interpretAddFunc(0x80FA, op_action_being_used);
    interpretAddFunc(0x80FB, op_critter_state);
    interpretAddFunc(0x80FC, op_game_time_advance);
    interpretAddFunc(0x80FD, op_radiation_inc);
    interpretAddFunc(0x80FE, op_radiation_dec);
    interpretAddFunc(0x80FF, op_critter_attempt_placement);
    interpretAddFunc(0x8100, op_obj_pid);
    interpretAddFunc(0x8101, op_cur_map_index);
    interpretAddFunc(0x8102, op_critter_add_trait);
    interpretAddFunc(0x8103, op_critter_rm_trait);
    interpretAddFunc(0x8104, op_proto_data);
    interpretAddFunc(0x8105, op_message_str);
    interpretAddFunc(0x8106, op_critter_inven_obj);
    interpretAddFunc(0x8107, op_obj_set_light_level);
    interpretAddFunc(0x8108, op_world_map);
    interpretAddFunc(0x8109, op_inven_cmds);
    interpretAddFunc(0x810A, op_float_msg);
    interpretAddFunc(0x810B, op_metarule);
    interpretAddFunc(0x810C, op_anim);
    interpretAddFunc(0x810D, op_obj_carrying_pid_obj);
    interpretAddFunc(0x810E, op_reg_anim_func);
    interpretAddFunc(0x810F, op_reg_anim_animate);
    interpretAddFunc(0x8110, op_reg_anim_animate_reverse);
    interpretAddFunc(0x8111, op_reg_anim_obj_move_to_obj);
    interpretAddFunc(0x8112, op_reg_anim_obj_run_to_obj);
    interpretAddFunc(0x8113, op_reg_anim_obj_move_to_tile);
    interpretAddFunc(0x8114, op_reg_anim_obj_run_to_tile);
    interpretAddFunc(0x8115, op_play_gmovie);
    interpretAddFunc(0x8116, op_add_mult_objs_to_inven);
    interpretAddFunc(0x8117, op_rm_mult_objs_from_inven);
    interpretAddFunc(0x8118, op_get_month);
    interpretAddFunc(0x8119, op_get_day);
    interpretAddFunc(0x811A, op_explosion);
    interpretAddFunc(0x811B, op_days_since_visited);
    interpretAddFunc(0x811C, op_gsay_start);
    interpretAddFunc(0x811D, op_gsay_end);
    interpretAddFunc(0x811E, op_gsay_reply);
    interpretAddFunc(0x811F, op_gsay_option);
    interpretAddFunc(0x8120, op_gsay_message);
    interpretAddFunc(0x8121, op_giq_option);
    interpretAddFunc(0x8122, op_poison);
    interpretAddFunc(0x8123, op_get_poison);
    interpretAddFunc(0x8124, op_party_add);
    interpretAddFunc(0x8125, op_party_remove);
    interpretAddFunc(0x8126, op_reg_anim_animate_forever);
    interpretAddFunc(0x8127, op_critter_injure);
    interpretAddFunc(0x8128, op_combat_is_initialized);
    interpretAddFunc(0x8129, op_gdialog_barter);
    interpretAddFunc(0x812A, op_difficulty_level);
    interpretAddFunc(0x812B, op_running_burning_guy);
    interpretAddFunc(0x812C, op_inven_unwield);
    interpretAddFunc(0x812D, op_obj_is_locked);
    interpretAddFunc(0x812E, op_obj_lock);
    interpretAddFunc(0x812F, op_obj_unlock);
    interpretAddFunc(0x8131, op_obj_open);
    interpretAddFunc(0x8130, op_obj_is_open);
    interpretAddFunc(0x8132, op_obj_close);
    interpretAddFunc(0x8133, op_game_ui_disable);
    interpretAddFunc(0x8134, op_game_ui_enable);
    interpretAddFunc(0x8135, op_game_ui_is_disabled);
    interpretAddFunc(0x8136, op_gfade_out);
    interpretAddFunc(0x8137, op_gfade_in);
    interpretAddFunc(0x8138, op_item_caps_total);
    interpretAddFunc(0x8139, op_item_caps_adjust);
    interpretAddFunc(0x813A, op_anim_action_frame);
    interpretAddFunc(0x813B, op_reg_anim_play_sfx);
    interpretAddFunc(0x813C, op_critter_mod_skill);
    interpretAddFunc(0x813D, op_sfx_build_char_name);
    interpretAddFunc(0x813E, op_sfx_build_ambient_name);
    interpretAddFunc(0x813F, op_sfx_build_interface_name);
    interpretAddFunc(0x8140, op_sfx_build_item_name);
    interpretAddFunc(0x8141, op_sfx_build_weapon_name);
    interpretAddFunc(0x8142, op_sfx_build_scenery_name);
    interpretAddFunc(0x8143, op_attack_setup);
    interpretAddFunc(0x8144, op_destroy_mult_objs);
    interpretAddFunc(0x8145, op_use_obj_on_obj);
    interpretAddFunc(0x8146, op_endgame_slideshow);
    interpretAddFunc(0x8147, op_move_obj_inven_to_obj);
    interpretAddFunc(0x8148, op_endgame_movie);
    interpretAddFunc(0x8149, op_obj_art_fid);
    interpretAddFunc(0x814A, op_art_anim);
    interpretAddFunc(0x814B, op_party_member_obj);
    interpretAddFunc(0x814C, op_rotation_to_tile);
    interpretAddFunc(0x814D, op_jam_lock);
    interpretAddFunc(0x814E, op_gdialog_set_barter_mod);
    interpretAddFunc(0x814F, op_combat_difficulty);
    interpretAddFunc(0x8150, op_obj_on_screen);
    interpretAddFunc(0x8151, op_critter_is_fleeing);
    interpretAddFunc(0x8152, op_critter_set_flee_state);
    interpretAddFunc(0x8153, op_terminate_combat);
    interpretAddFunc(0x8154, op_debug_msg);
    interpretAddFunc(0x8155, op_critter_stop_attacking);
}

// NOTE: Uncollapsed 0x45D878.
//
// 0x45D878
void updateIntExtra()
{
}

// NOTE: Uncollapsed 0x45D878.
//
// 0x45D878
void intExtraRemoveProgramReferences(Program* program)
{
}
