#ifndef SCRIPTS_H
#define SCRIPTS_H

#include <stdbool.h>

#include "game/combat_defs.h"
#include "db.h"
#include "int/intrpret.h"
#include "game/message.h"
#include "game/object_types.h"

#define SCRIPT_FLAG_0x01 0x01
#define SCRIPT_FLAG_0x02 0x02
#define SCRIPT_FLAG_0x04 0x04
#define SCRIPT_FLAG_0x08 0x08
#define SCRIPT_FLAG_0x10 0x10

// 60 * 60 * 10
#define GAME_TIME_TICKS_PER_HOUR 36000

// 24 * 60 * 60 * 10
#define GAME_TIME_TICKS_PER_DAY 864000

// 365 * 24 * 60 * 60 * 10
#define GAME_TIME_TICKS_PER_YEAR 315360000

typedef enum ScriptRequests {
    SCRIPT_REQUEST_COMBAT = 0x01,
    SCRIPT_REQUEST_0x02 = 0x02,
    SCRIPT_REQUEST_WORLD_MAP = 0x04,
    SCRIPT_REQUEST_ELEVATOR = 0x08,
    SCRIPT_REQUEST_EXPLOSION = 0x10,
    SCRIPT_REQUEST_DIALOG = 0x20,
    SCRIPT_REQUEST_0x40 = 0x40,
    SCRIPT_REQUEST_ENDGAME = 0x80,
    SCRIPT_REQUEST_LOOTING = 0x100,
    SCRIPT_REQUEST_STEALING = 0x200,
    SCRIPT_REQUEST_0x0400 = 0x400,
} ScriptRequests;

typedef enum ScriptType {
    SCRIPT_TYPE_SYSTEM, // s_system
    SCRIPT_TYPE_SPATIAL, // s_spatial
    SCRIPT_TYPE_TIMED, // s_time
    SCRIPT_TYPE_ITEM, // s_item
    SCRIPT_TYPE_CRITTER, // s_critter
    SCRIPT_TYPE_COUNT,
} ScriptType;

typedef enum ScriptProc {
    SCRIPT_PROC_NO_PROC = 0,
    SCRIPT_PROC_START = 1,
    SCRIPT_PROC_SPATIAL = 2,
    SCRIPT_PROC_DESCRIPTION = 3,
    SCRIPT_PROC_PICKUP = 4,
    SCRIPT_PROC_DROP = 5,
    SCRIPT_PROC_USE = 6,
    SCRIPT_PROC_USE_OBJ_ON = 7,
    SCRIPT_PROC_USE_SKILL_ON = 8,
    SCRIPT_PROC_9 = 9, // use_ad_on_proc
    SCRIPT_PROC_10 = 10, // use_disad_on_proc
    SCRIPT_PROC_TALK = 11,
    SCRIPT_PROC_CRITTER = 12,
    SCRIPT_PROC_COMBAT = 13,
    SCRIPT_PROC_DAMAGE = 14,
    SCRIPT_PROC_MAP_ENTER = 15,
    SCRIPT_PROC_MAP_EXIT = 16,
    SCRIPT_PROC_CREATE = 17,
    SCRIPT_PROC_DESTROY = 18,
    SCRIPT_PROC_19 = 19, // barter_init_proc
    SCRIPT_PROC_20 = 20, // barter_proc
    SCRIPT_PROC_LOOK_AT = 21,
    SCRIPT_PROC_TIMED = 22,
    SCRIPT_PROC_MAP_UPDATE = 23,
    SCRIPT_PROC_PUSH = 24,
    SCRIPT_PROC_IS_DROPPING = 25,
    SCRIPT_PROC_COMBAT_IS_STARTING = 26,
    SCRIPT_PROC_COMBAT_IS_OVER = 27,
    SCRIPT_PROC_COUNT,
} ScriptProc;

static_assert(SCRIPT_PROC_COUNT == 28, "wrong count");

typedef struct ScriptsListEntry {
    char name[16];
    int local_vars_num;
} ScriptsListEntry;

typedef struct Script {
    // scr_id
    int sid;

    // scr_next
    int field_4;

    union {
        struct {
            // scr_udata.sp.built_tile
            int built_tile;
            // scr_udata.sp.radius
            int radius;
        } sp;
        struct {
            // scr_udata.tm.time
            int time;
        } tm;
    };

    // scr_flags
    int flags;

    // scr_script_idx
    int field_14;

    Program* program;

    // scr_oid
    int field_1C;

    // scr_local_var_offset
    int localVarsOffset;

    // scr_num_local_vars
    int localVarsCount;

    // return value?
    int field_28;

    // Currently executed action.
    //
    // See [op_script_action].
    int action;
    int fixedParam;
    Object* owner;

    // source_obj
    Object* source;

    // target_obj
    Object* target;
    int actionBeingUsed;
    int scriptOverrides;
    int field_48;
    int howMuch;
    int field_50;
    int procs[SCRIPT_PROC_COUNT];
    int field_C4;
    int field_C8;
    int field_CC;
    int field_D0;
    int field_D4;
    int field_D8;
    int field_DC;
} Script;

static_assert(sizeof(Script) == 0xE0, "wrong size");

extern int num_script_indexes;
extern Object* scrQueueTestObj;
extern int scrQueueTestValue;

extern MessageList script_dialog_msgs[1450];
extern MessageList script_message_file;

int game_time();
void game_time_date(int* monthPtr, int* dayPtr, int* yearPtr);
int game_time_hour();
char* game_time_hour_str();
void inc_game_time(int a1);
void inc_game_time_in_seconds(int a1);
void gameTimeSetTime(int time);
int gtime_q_add();
int gtime_q_process(Object* obj, void* data);
int scriptsCheckGameEvents(int* moviePtr, int window);
int scr_map_q_process(Object* obj, void* data);
int new_obj_id();
int scr_find_sid_from_program(Program* program);
Object* scr_find_obj_from_program(Program* program);
int scr_set_objs(int sid, Object* source, Object* target);
void scr_set_ext_param(int a1, int a2);
int scr_set_action_num(int sid, int a2);
Program* loadProgram(const char* name);
void scrSetQueueTestVals(Object* a1, int a2);
int scrQueueRemoveFixed(Object* obj, void* data);
int script_q_add(int sid, int delay, int param);
int script_q_save(File* stream, void* data);
int script_q_load(File* stream, void** dataPtr);
int script_q_process(Object* obj, void* data);
int scripts_clear_state();
int scripts_clear_combat_requests(Script* script);
int scripts_check_state();
int scripts_check_state_in_combat();
int scripts_request_combat(STRUCT_664980* a1);
void scripts_request_combat_locked(STRUCT_664980* ptr);
void scripts_request_worldmap();
int scripts_request_elevator(Object* a1, int a2);
int scripts_request_explosion(int tile, int elevation, int minDamage, int maxDamage);
void scripts_request_dialog(Object* a1);
void scripts_request_endgame_slideshow();
int scripts_request_loot_container(Object* a1, Object* a2);
int scripts_request_steal_container(Object* a1, Object* a2);
void script_make_path(char* path);
int exec_script_proc(int sid, int proc);
bool scriptHasProc(int sid, int proc);
int scr_find_str_run_info(int a1, int* a2, int sid);
int scr_set_dude_script();
int scr_clear_dude_script();
int scr_init();
int scr_reset();
int scr_game_init();
int scr_game_reset();
int scr_exit();
int scr_message_free();
int scr_game_exit();
int scr_enable();
int scr_disable();
void scr_enable_critters();
void scr_disable_critters();
int scr_game_save(File* stream);
int scr_game_load(File* stream);
int scr_game_load2(File* stream);
int scr_save(File* stream);
int scr_load(File* stream);
int scr_ptr(int sid, Script** script);
int scr_new(int* sidPtr, int scriptType);
int scr_remove_local_vars(Script* script);
int scr_remove(int index);
int scr_remove_all();
int scr_remove_all_force();
Script* scr_find_first_at(int elevation);
Script* scr_find_next_at();
void scr_spatials_enable();
void scr_spatials_disable();
bool scr_chk_spatials_in(Object* obj, int tile, int elevation);
int scr_load_all_scripts();
void scr_exec_map_enter_scripts();
void scr_exec_map_update_scripts();
void scr_exec_map_exit_scripts();
int scr_get_dialog_msg_file(int a1, MessageList** out_message_list);
char* scr_get_msg_str(int messageListId, int messageId);
char* scr_get_msg_str_speech(int messageListId, int messageId, int a3);
int scr_get_local_var(int a1, int a2, int* a3);
int scr_set_local_var(int a1, int a2, int a3);
bool scr_end_combat();
int scr_explode_scenery(Object* a1, int tile, int radius, int elevation);

#endif /* SCRIPTS_H */
