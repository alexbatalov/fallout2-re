#include "game/scripts.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "game/actions.h"
#include "game/automap.h"
#include "game/combat.h"
#include "core.h"
#include "game/critter.h"
#include "debug.h"
#include "int/dialog.h"
#include "game/elevator.h"
#include "game/endgame.h"
#include "int/export.h"
#include "game/game.h"
#include "game/gdialog.h"
#include "game/gmouse.h"
#include "game/gmovie.h"
#include "memory.h"
#include "game/object.h"
#include "game/proto.h"
#include "game/protinst.h"
#include "game/queue.h"
#include "game/tile.h"
#include "window_manager.h"
#include "window_manager_private.h"
#include "worldmap.h"

#define SCRIPT_LIST_EXTENT_SIZE 16

typedef struct ScriptListExtent {
    Script scripts[SCRIPT_LIST_EXTENT_SIZE];
    // Number of scripts in the extent
    int length;
    struct ScriptListExtent* next;
} ScriptListExtent;

static_assert(sizeof(ScriptListExtent) == 0xE08, "wrong size");

typedef struct ScriptList {
    ScriptListExtent* head;
    ScriptListExtent* tail;
    // Number of extents in the script list.
    int length;
    int nextScriptId;
} ScriptList;

static_assert(sizeof(ScriptList) == 0x10, "wrong size");

typedef struct ScriptState {
    unsigned int requests;
    STRUCT_664980 combatState1;
    STRUCT_664980 combatState2;
    int elevatorType;
    int elevatorLevel;
    int explosionTile;
    int explosionElevation;
    int explosionMinDamage;
    int explosionMaxDamage;
    Object* dialogTarget;
    Object* lootingBy;
    Object* lootingFrom;
    Object* stealingBy;
    Object* stealingFrom;
} ScriptState;

static void doBkProcesses();
static void script_chk_critters();
static void script_chk_timed_events();
static int scr_build_lookup_table(Script* scr);
static int scrInitListInfo();
static int scrExitListInfo();
static int scr_index_to_name(int scriptIndex, char* name);
static int scr_header_load();
static int scr_write_ScriptSubNode(Script* scr, File* stream);
static int scr_write_ScriptNode(ScriptListExtent* a1, File* stream);
static int scr_read_ScriptSubNode(Script* scr, File* stream);
static int scr_read_ScriptNode(ScriptListExtent* a1, File* stream);
static int scr_new_id(int scriptType);
static void scrExecMapProcScripts(int a1);

// Number of lines in scripts.lst
//
// 0x51C6AC
int num_script_indexes = 0;

// 0x51C6B0
static int scr_find_first_idx = 0;

// 0x51C6B4
static ScriptListExtent* scr_find_first_ptr = NULL;

// 0x51C6B8
static int scr_find_first_elev = 0;

// 0x51C6BC
static bool scrSpatialsEnabled = true;

// 0x51C6C0
static ScriptList scriptlists[SCRIPT_TYPE_COUNT];

// 0x51C710
static char script_path_base[] = "scripts\\";

// 0x51C714
static bool script_engine_running = false;

// 0x51C718
static int script_engine_run_critters = 0;

// 0x51C71C
static int script_engine_game_mode = 0;

// Game time in ticks (1/10 second).
//
// 0x51C720
static int fallout_game_time = 302400;

// 0x51C724
static int days_in_month[12] = {
    31, // Jan
    28, // Feb
    31, // Mar
    30, // Apr
    31, // May
    30, // Jun
    31, // Jul
    31, // Aug
    30, // Sep
    31, // Oct
    30, // Nov
    31, // Dec
};

// 0x51C758
static const char* procTableStrs[SCRIPT_PROC_COUNT] = {
    "no_p_proc",
    "start",
    "spatial_p_proc",
    "description_p_proc",
    "pickup_p_proc",
    "drop_p_proc",
    "use_p_proc",
    "use_obj_on_p_proc",
    "use_skill_on_p_proc",
    "none_x_bad",
    "none_x_bad",
    "talk_p_proc",
    "critter_p_proc",
    "combat_p_proc",
    "damage_p_proc",
    "map_enter_p_proc",
    "map_exit_p_proc",
    "create_p_proc",
    "destroy_p_proc",
    "none_x_bad",
    "none_x_bad",
    "look_at_p_proc",
    "timed_event_p_proc",
    "map_update_p_proc",
    "push_p_proc",
    "is_dropping_p_proc",
    "combat_is_starting_p_proc",
    "combat_is_over_p_proc",
};

// scripts.lst
//
// 0x51C7C8
static ScriptsListEntry* scriptListInfo = NULL;

// 0x51C7CC
static int maxScriptNum = 0;

// 0x51C7E8
Object* scrQueueTestObj = NULL;

// 0x51C7EC
int scrQueueTestValue = 0;

// 0x664954
static ScriptState scriptState;

// 0x6649D4
MessageList script_dialog_msgs[1450];

// scr.msg
//
// 0x667724
MessageList script_message_file;

// TODO: Make unsigned.
//
// Returns game time in ticks (1/10 second).
//
// 0x4A3330
int game_time()
{
    return fallout_game_time;
}

// 0x4A3338
void game_time_date(int* monthPtr, int* dayPtr, int* yearPtr)
{
    int year = (fallout_game_time / GAME_TIME_TICKS_PER_DAY + 24) / 365 + 2241;
    int month = 6;
    int day = (fallout_game_time / GAME_TIME_TICKS_PER_DAY + 24) % 365;

    while (1) {
        int daysInMonth = days_in_month[month];
        if (day < daysInMonth) {
            break;
        }

        month++;
        day -= daysInMonth;

        if (month == 12) {
            year++;
            month = 0;
        }
    }

    if (dayPtr != NULL) {
        *dayPtr = day + 1;
    }

    if (monthPtr != NULL) {
        *monthPtr = month + 1;
    }

    if (yearPtr != NULL) {
        *yearPtr = year;
    }
}

// Returns game hour/minute in military format (hhmm).
//
// Examples:
// - 8:00 A.M. -> 800
// - 3:00 P.M. -> 1500
// - 11:59 P.M. -> 2359
//
// game_time_hour
// 0x4A33C8
int game_time_hour()
{
    return 100 * ((fallout_game_time / 600) / 60 % 24) + (fallout_game_time / 600) % 60;
}

// Returns time string (h:mm)
//
// 0x4A3420
char* game_time_hour_str()
{
    // 0x66772C
    static char hour_str[7];

    sprintf(hour_str, "%d:%02d", (fallout_game_time / 600) / 60 % 24, (fallout_game_time / 600) % 60);
    return hour_str;
}

// TODO: Make unsigned.
//
// 0x4A347C
void gameTimeSetTime(int time)
{
    if (time == 0) {
        time = 1;
    }

    fallout_game_time = time;
}

// 0x4A34CC
void inc_game_time(int ticks)
{
    fallout_game_time += ticks;

    int v1 = 0;

    unsigned int year = fallout_game_time / GAME_TIME_TICKS_PER_YEAR;
    if (year >= 13) {
        endgameSetupDeathEnding(ENDGAME_DEATH_ENDING_REASON_TIMEOUT);
        game_user_wants_to_quit = 2;
    }

    // FIXME: This condition will never be true.
    if (v1) {
        gtime_q_process(NULL, NULL);
    }
}

// 0x4A3518
void inc_game_time_in_seconds(int seconds)
{
    // NOTE: Uninline.
    inc_game_time(seconds * 10);
}

// 0x4A3570
int gtime_q_add()
{
    int v1 = 10 * (60 * (60 - (fallout_game_time / 600) % 60 - 1) + 3600 * (24 - (fallout_game_time / 600) / 60 % 24 - 1) + 60);
    if (queue_add(v1, NULL, NULL, EVENT_TYPE_GAME_TIME) == -1) {
        return -1;
    }

    if (map_data.name[0] != '\0') {
        if (queue_add(600, NULL, NULL, EVENT_TYPE_MAP_UPDATE_EVENT) == -1) {
            return -1;
        }
    }

    return 0;
}

// 0x4A3620
int gtime_q_process(Object* obj, void* data)
{
    int movie_index;
    int v4;

    movie_index = -1;

    debugPrint("\nQUEUE PROCESS: Midnight!");

    if (gmovieIsPlaying()) {
        return 0;
    }

    obj_unjam_all_locks();

    if (!gdialogActive()) {
        scriptsCheckGameEvents(&movie_index, -1);
    }

    v4 = critter_check_rads(obj_dude);

    queue_clear_type(4, 0);

    gtime_q_add();

    if (movie_index != -1) {
        v4 = 1;
    }

    return v4;
}

// 0x4A3690
int scriptsCheckGameEvents(int* moviePtr, int window)
{
    int movie = -1;
    int movieFlags = GAME_MOVIE_FADE_IN | GAME_MOVIE_FADE_OUT | GAME_MOVIE_PAUSE_MUSIC;
    bool endgame = false;
    bool adjustRep = false;

    int day = fallout_game_time / GAME_TIME_TICKS_PER_DAY;

    if (game_get_global_var(GVAR_ENEMY_ARROYO)) {
        movie = MOVIE_AFAILED;
        movieFlags = GAME_MOVIE_FADE_IN | GAME_MOVIE_STOP_MUSIC;
        endgame = true;
    } else {
        if (day >= 360 || game_get_global_var(GVAR_FALLOUT_2) >= 3) {
            movie = MOVIE_ARTIMER4;
            if (!gmovie_has_been_played(MOVIE_ARTIMER4)) {
                adjustRep = true;
                wmAreaSetVisibleState(CITY_ARROYO, 0, 1);
                wmAreaSetVisibleState(CITY_DESTROYED_ARROYO, 1, 1);
                wmAreaMarkVisitedState(CITY_DESTROYED_ARROYO, 2);
            }
        } else if (day >= 270 && game_get_global_var(GVAR_FALLOUT_2) != 3) {
            adjustRep = true;
            movie = MOVIE_ARTIMER3;
        } else if (day >= 180 && game_get_global_var(GVAR_FALLOUT_2) != 3) {
            adjustRep = true;
            movie = MOVIE_ARTIMER2;
        } else if (day >= 90 && game_get_global_var(GVAR_FALLOUT_2) != 3) {
            adjustRep = true;
            movie = MOVIE_ARTIMER1;
        }
    }

    if (movie != -1) {
        if (gmovie_has_been_played(movie)) {
            movie = -1;
        } else {
            if (window != -1) {
                win_hide(window);
            }

            gmovie_play(movie, movieFlags);

            if (window != -1) {
                win_show(window);
            }

            if (adjustRep) {
                int rep = game_get_global_var(GVAR_TOWN_REP_ARROYO);
                game_set_global_var(GVAR_TOWN_REP_ARROYO, rep - 15);
            }
        }
    }

    if (endgame) {
        game_user_wants_to_quit = 2;
    } else {
        tileWindowRefresh();
    }

    if (moviePtr != NULL) {
        *moviePtr = movie;
    }

    return 0;
}

// 0x4A382C
int scr_map_q_process(Object* obj, void* data)
{
    scrExecMapProcScripts(SCRIPT_PROC_MAP_UPDATE);

    queue_clear_type(EVENT_TYPE_MAP_UPDATE_EVENT, NULL);

    if (map_data.name[0] == '\0') {
        return 0;
    }

    if (queue_add(600, NULL, NULL, EVENT_TYPE_MAP_UPDATE_EVENT) != -1) {
        return 0;
    }

    return -1;
}

// 0x4A386C
int new_obj_id()
{
    // 0x51C7D4
    static int cur_id = 4;

    Object* ptr;

    do {
        cur_id++;
        ptr = obj_find_first();

        while (ptr) {
            if (cur_id == ptr->id) {
                break;
            }

            ptr = obj_find_next();
        }
    } while (ptr);

    if (cur_id >= 18000) {
        debugPrint("\n    ERROR: new_obj_id() !!!! Picked PLAYER ID!!!!");
    }

    cur_id++;

    return cur_id;
}

// 0x4A390C
int scr_find_sid_from_program(Program* program)
{
    for (int type = 0; type < SCRIPT_TYPE_COUNT; type++) {
        ScriptListExtent* extent = scriptlists[type].head;
        while (extent != NULL) {
            for (int index = 0; index < extent->length; index++) {
                Script* script = &(extent->scripts[index]);
                if (script->program == program) {
                    return script->sid;
                }
            }
            extent = extent->next;
        }
    }

    return -1;
}

// 0x4A39AC
Object* scr_find_obj_from_program(Program* program)
{
    int sid = scr_find_sid_from_program(program);

    Script* script;
    if (scr_ptr(sid, &script) == -1) {
        return NULL;
    }

    if (script->owner != NULL) {
        return script->owner;
    }

    if (SID_TYPE(sid) != SCRIPT_TYPE_SPATIAL) {
        return NULL;
    }

    Object* object;
    int fid = art_id(OBJ_TYPE_INTERFACE, 3, 0, 0, 0);
    obj_new(&object, fid, -1);
    obj_turn_off(object, NULL);
    obj_toggle_flat(object, NULL);
    object->sid = sid;

    // NOTE: Redundant, we've already obtained script earlier. Probably
    // inlining.
    Script* v1;
    if (scr_ptr(sid, &v1) == -1) {
        // FIXME: this is clearly an error, but I guess it's never reached since
        // we've already obtained script for given sid earlier.
        return (Object*)-1;
    }

    object->id = new_obj_id();
    v1->field_1C = object->id;
    v1->owner = object;

    for (int elevation = 0; elevation < ELEVATION_COUNT; elevation++) {
        Script* spatialScript = scr_find_first_at(elevation);
        while (spatialScript != NULL) {
            if (spatialScript == script) {
                obj_move_to_tile(object, builtTileGetTile(script->sp.built_tile), elevation, NULL);
                return object;
            }
            spatialScript = scr_find_next_at();
        }
    }

    return object;
}

// 0x4A3B0C
int scr_set_objs(int sid, Object* source, Object* target)
{
    Script* script;
    if (scr_ptr(sid, &script) == -1) {
        return -1;
    }

    script->source = source;
    script->target = target;

    return 0;
}

// 0x4A3B34
void scr_set_ext_param(int sid, int value)
{
    Script* script;
    if (scr_ptr(sid, &script) != -1) {
        script->fixedParam = value;
    }
}

// 0x4A3B54
int scr_set_action_num(int sid, int value)
{
    Script* scr;

    if (scr_ptr(sid, &scr) == -1) {
        return -1;
    }

    scr->actionBeingUsed = value;

    return 0;
}

// 0x4A3B74
Program* loadProgram(const char* name)
{
    char path[MAX_PATH];

    strcpy(path, cd_path_base);
    strcat(path, script_path_base);
    strcat(path, name);
    strcat(path, ".int");

    return allocateProgram(path);
}

// 0x4A3C2C
static void doBkProcesses()
{
    // 0x66774C
    static bool set;

    // 0x667748
    static int lasttime;

    if (!set) {
        lasttime = _get_bk_time();
        set = 1;
    }

    int v0 = _get_bk_time();
    if (script_engine_running) {
        lasttime = v0;

        // NOTE: There is a loop at 0x4A3C64, consisting of one iteration, going
        // downwards from 1.
        for (int index = 0; index < 1; index++) {
            updatePrograms();
        }
    }

    _updateWindows();

    if (script_engine_running && script_engine_run_critters) {
        if (!gdialogActive()) {
            script_chk_critters();
            script_chk_timed_events();
        }
    }
}

// 0x4A3CA0
static void script_chk_critters()
{
    // 0x51C7DC
    static int count = 0;

    if (!gdialogActive() && !isInCombat()) {
        ScriptList* scriptList;
        ScriptListExtent* scriptListExtent;

        int scriptsCount = 0;

        scriptList = &(scriptlists[SCRIPT_TYPE_CRITTER]);
        scriptListExtent = scriptList->head;
        while (scriptListExtent != NULL) {
            scriptsCount += scriptListExtent->length;
            scriptListExtent = scriptListExtent->next;
        }

        count += 1;
        if (count >= scriptsCount) {
            count = 0;
        }

        if (count < scriptsCount) {
            int proc = isInCombat() ? SCRIPT_PROC_COMBAT : SCRIPT_PROC_CRITTER;
            int extentIndex = count / SCRIPT_LIST_EXTENT_SIZE;
            int scriptIndex = count % SCRIPT_LIST_EXTENT_SIZE;

            scriptList = &(scriptlists[SCRIPT_TYPE_CRITTER]);
            scriptListExtent = scriptList->head;
            while (scriptListExtent != NULL && extentIndex != 0) {
                extentIndex -= 1;
                scriptListExtent = scriptListExtent->next;
            }

            if (scriptListExtent != NULL) {
                Script* script = &(scriptListExtent->scripts[scriptIndex]);
                exec_script_proc(script->sid, proc);
            }
        }
    }
}

// TODO: Check.
//
// 0x4A3D84
static void script_chk_timed_events()
{
    // 0x51C7E0
    static int last_time = 0;

    // 0x51C7E4
    static int last_light_time = 0;

    int v0 = _get_bk_time();

    int v1 = false;
    if (!isInCombat()) {
        v1 = true;
    }

    if (game_state() != GAME_STATE_4) {
        if (getTicksBetween(v0, last_light_time) >= 30000) {
            last_light_time = v0;
            scrExecMapProcScripts(SCRIPT_PROC_MAP_UPDATE);
        }
    } else {
        v1 = false;
    }

    if (getTicksBetween(v0, last_time) >= 100) {
        last_time = v0;
        if (!isInCombat()) {
            fallout_game_time += 1;
        }
        v1 = true;
    }

    if (v1) {
        while (!queue_is_empty()) {
            int time = game_time();
            int v2 = queue_next_time();
            if (time < v2) {
                break;
            }

            queue_process();
        }
    }
}

// 0x4A3E30
void scrSetQueueTestVals(Object* a1, int a2)
{
    scrQueueTestObj = a1;
    scrQueueTestValue = a2;
}

// 0x4A3E3C
int scrQueueRemoveFixed(Object* obj, void* data)
{
    ScriptEvent* scriptEvent = (ScriptEvent*)data;
    return obj == scrQueueTestObj && scriptEvent->fixedParam == scrQueueTestValue;
}

// 0x4A3E60
int script_q_add(int sid, int delay, int param)
{
    ScriptEvent* scriptEvent = (ScriptEvent*)internal_malloc(sizeof(*scriptEvent));
    if (scriptEvent == NULL) {
        return -1;
    }

    scriptEvent->sid = sid;
    scriptEvent->fixedParam = param;

    Script* script;
    if (scr_ptr(sid, &script) == -1) {
        internal_free(scriptEvent);
        return -1;
    }

    if (queue_add(delay, script->owner, scriptEvent, EVENT_TYPE_SCRIPT) == -1) {
        internal_free(scriptEvent);
        return -1;
    }

    return 0;
}

// 0x4A3EDC
int script_q_save(File* stream, void* data)
{
    ScriptEvent* scriptEvent = (ScriptEvent*)data;

    if (fileWriteInt32(stream, scriptEvent->sid) == -1) return -1;
    if (fileWriteInt32(stream, scriptEvent->fixedParam) == -1) return -1;

    return 0;
}

// 0x4A3F04
int script_q_load(File* stream, void** dataPtr)
{
    ScriptEvent* scriptEvent = (ScriptEvent*)internal_malloc(sizeof(*scriptEvent));
    if (scriptEvent == NULL) {
        return -1;
    }

    if (fileReadInt32(stream, &(scriptEvent->sid)) == -1) goto err;
    if (fileReadInt32(stream, &(scriptEvent->fixedParam)) == -1) goto err;

    *dataPtr = scriptEvent;

    return 0;

err:

    // there is a memory leak in original code, free is not called
    internal_free(scriptEvent);

    return -1;
}

// 0x4A3F4C
int script_q_process(Object* obj, void* data)
{
    ScriptEvent* scriptEvent = (ScriptEvent*)data;

    Script* script;
    if (scr_ptr(scriptEvent->sid, &script) == -1) {
        return 0;
    }

    script->fixedParam = scriptEvent->fixedParam;

    exec_script_proc(scriptEvent->sid, SCRIPT_PROC_TIMED);

    return 0;
}

// 0x4A3F80
int scripts_clear_state()
{
    scriptState.requests = 0;
    return 0;
}

// NOTE: Inlined.
//
// 0x4A3F90
int scripts_clear_combat_requests(Script* script)
{
    if ((scriptState.requests & SCRIPT_REQUEST_COMBAT) != 0 && scriptState.combatState1.attacker == script->owner) {
        scriptState.requests &= ~(SCRIPT_REQUEST_LOCKED | SCRIPT_REQUEST_COMBAT);
    }
    return 0;
}

// 0x4A3FB4
int scripts_check_state()
{
    if (scriptState.requests == 0) {
        return 0;
    }

    if ((scriptState.requests & SCRIPT_REQUEST_COMBAT) != 0) {
        if (!action_explode_running()) {
            // entering combat
            scriptState.requests &= ~(SCRIPT_REQUEST_LOCKED | SCRIPT_REQUEST_COMBAT);
            memcpy(&scriptState.combatState2, &scriptState.combatState1, sizeof(scriptState.combatState2));

            if ((scriptState.requests & SCRIPT_REQUEST_NO_INITIAL_COMBAT_STATE) != 0) {
                scriptState.requests &= ~SCRIPT_REQUEST_NO_INITIAL_COMBAT_STATE;
                combat(NULL);
            } else {
                combat(&scriptState.combatState2);
                memset(&scriptState.combatState2, 0, sizeof(scriptState.combatState2));
            }
        }
    }

    if ((scriptState.requests & SCRIPT_REQUEST_TOWN_MAP) != 0) {
        scriptState.requests &= ~SCRIPT_REQUEST_TOWN_MAP;
        wmTownMap();
    }

    if ((scriptState.requests & SCRIPT_REQUEST_WORLD_MAP) != 0) {
        scriptState.requests &= ~SCRIPT_REQUEST_WORLD_MAP;
        wmWorldMap();
    }

    if ((scriptState.requests & SCRIPT_REQUEST_ELEVATOR) != 0) {
        int map = map_data.field_34;
        int elevation = scriptState.elevatorLevel;
        int tile = -1;

        scriptState.requests &= ~SCRIPT_REQUEST_ELEVATOR;

        if (elevator_select(scriptState.elevatorType, &map, &elevation, &tile) != -1) {
            automap_pip_save();

            if (map == map_data.field_34) {
                if (elevation == map_elevation) {
                    register_clear(obj_dude);
                    obj_set_rotation(obj_dude, ROTATION_SE, 0);
                    obj_attempt_placement(obj_dude, tile, elevation, 0);
                } else {
                    Object* elevatorDoors = obj_find_first_at(obj_dude->elevation);
                    while (elevatorDoors != NULL) {
                        int pid = elevatorDoors->pid;
                        if (PID_TYPE(pid) == OBJ_TYPE_SCENERY
                            && (pid == PROTO_ID_0x2000099 || pid == PROTO_ID_0x20001A5 || pid == PROTO_ID_0x20001D6)
                            && tileDistanceBetween(elevatorDoors->tile, obj_dude->tile) <= 4) {
                            break;
                        }
                        elevatorDoors = obj_find_next_at();
                    }

                    register_clear(obj_dude);
                    obj_set_rotation(obj_dude, ROTATION_SE, 0);
                    obj_attempt_placement(obj_dude, tile, elevation, 0);

                    if (elevatorDoors != NULL) {
                        obj_set_frame(elevatorDoors, 0, NULL);
                        obj_move_to_tile(elevatorDoors, elevatorDoors->tile, elevatorDoors->elevation, NULL);
                        elevatorDoors->flags &= ~OBJECT_OPEN_DOOR;
                        elevatorDoors->data.scenery.door.openFlags &= ~0x01;
                        obj_rebuild_all_light();
                    } else {
                        debugPrint("\nWarning: Elevator: Couldn't find old elevator doors!");
                    }
                }
            } else {
                Object* elevatorDoors = obj_find_first_at(obj_dude->elevation);
                while (elevatorDoors != NULL) {
                    int pid = elevatorDoors->pid;
                    if (PID_TYPE(pid) == OBJ_TYPE_SCENERY
                        && (pid == PROTO_ID_0x2000099 || pid == PROTO_ID_0x20001A5 || pid == PROTO_ID_0x20001D6)
                        && tileDistanceBetween(elevatorDoors->tile, obj_dude->tile) <= 4) {
                        break;
                    }
                    elevatorDoors = obj_find_next_at();
                }

                if (elevatorDoors != NULL) {
                    obj_set_frame(elevatorDoors, 0, NULL);
                    obj_move_to_tile(elevatorDoors, elevatorDoors->tile, elevatorDoors->elevation, NULL);
                    elevatorDoors->flags &= ~OBJECT_OPEN_DOOR;
                    elevatorDoors->data.scenery.door.openFlags &= ~0x01;
                    obj_rebuild_all_light();
                } else {
                    debugPrint("\nWarning: Elevator: Couldn't find old elevator doors!");
                }

                MapTransition transition;
                memset(&transition, 0, sizeof(transition));

                transition.map = map;
                transition.elevation = elevation;
                transition.tile = tile;
                transition.rotation = ROTATION_SE;

                map_leave_map(&transition);
            }
        }
    }

    if ((scriptState.requests & SCRIPT_REQUEST_EXPLOSION) != 0) {
        scriptState.requests &= ~SCRIPT_REQUEST_EXPLOSION;
        action_explode(scriptState.explosionTile, scriptState.explosionElevation, scriptState.explosionMinDamage, scriptState.explosionMaxDamage, NULL, 1);
    }

    if ((scriptState.requests & SCRIPT_REQUEST_DIALOG) != 0) {
        scriptState.requests &= ~SCRIPT_REQUEST_DIALOG;
        gdialogEnter(scriptState.dialogTarget, 0);
    }

    if ((scriptState.requests & SCRIPT_REQUEST_ENDGAME) != 0) {
        scriptState.requests &= ~SCRIPT_REQUEST_ENDGAME;
        endgame_slideshow();
        endgame_movie();
    }

    if ((scriptState.requests & SCRIPT_REQUEST_LOOTING) != 0) {
        scriptState.requests &= ~SCRIPT_REQUEST_LOOTING;
        loot_container(scriptState.lootingBy, scriptState.lootingFrom);
    }

    if ((scriptState.requests & SCRIPT_REQUEST_STEALING) != 0) {
        scriptState.requests &= ~SCRIPT_REQUEST_STEALING;
        inven_steal_container(scriptState.stealingBy, scriptState.stealingFrom);
    }

    return 0;
}

// 0x4A43A0
int scripts_check_state_in_combat()
{
    if ((scriptState.requests & SCRIPT_REQUEST_ELEVATOR) != 0) {
        int map = map_data.field_34;
        int elevation = scriptState.elevatorLevel;
        int tile = -1;

        if (elevator_select(scriptState.elevatorType, &map, &elevation, &tile) != -1) {
            automap_pip_save();

            if (map == map_data.field_34) {
                if (elevation == map_elevation) {
                    register_clear(obj_dude);
                    obj_set_rotation(obj_dude, ROTATION_SE, 0);
                    obj_attempt_placement(obj_dude, tile, elevation, 0);
                } else {
                    Object* elevatorDoors = obj_find_first_at(obj_dude->elevation);
                    while (elevatorDoors != NULL) {
                        int pid = elevatorDoors->pid;
                        if (PID_TYPE(pid) == OBJ_TYPE_SCENERY
                            && (pid == PROTO_ID_0x2000099 || pid == PROTO_ID_0x20001A5 || pid == PROTO_ID_0x20001D6)
                            && tileDistanceBetween(elevatorDoors->tile, obj_dude->tile) <= 4) {
                            break;
                        }
                        elevatorDoors = obj_find_next_at();
                    }

                    register_clear(obj_dude);
                    obj_set_rotation(obj_dude, ROTATION_SE, 0);
                    obj_attempt_placement(obj_dude, tile, elevation, 0);

                    if (elevatorDoors != NULL) {
                        obj_set_frame(elevatorDoors, 0, NULL);
                        obj_move_to_tile(elevatorDoors, elevatorDoors->tile, elevatorDoors->elevation, NULL);
                        elevatorDoors->flags &= ~OBJECT_OPEN_DOOR;
                        elevatorDoors->data.scenery.door.openFlags &= ~0x01;
                        obj_rebuild_all_light();
                    } else {
                        debugPrint("\nWarning: Elevator: Couldn't find old elevator doors!");
                    }
                }
            } else {
                MapTransition transition;
                memset(&transition, 0, sizeof(transition));

                transition.map = map;
                transition.elevation = elevation;
                transition.tile = tile;
                transition.rotation = ROTATION_SE;

                map_leave_map(&transition);
            }
        }
    }

    if ((scriptState.requests & SCRIPT_REQUEST_LOOTING) != 0) {
        loot_container(scriptState.lootingBy, scriptState.lootingFrom);
    }

    // NOTE: Uninline.
    scripts_clear_state();

    return 0;
}

// 0x4A457C
int scripts_request_combat(STRUCT_664980* a1)
{
    if ((scriptState.requests & SCRIPT_REQUEST_LOCKED) != 0) {
        return -1;
    }

    if (a1) {
        static_assert(sizeof(scriptState.combatState1) == sizeof(*a1), "wrong size");
        memcpy(&scriptState.combatState1, a1, sizeof(scriptState.combatState1));
    } else {
        scriptState.requests |= SCRIPT_REQUEST_NO_INITIAL_COMBAT_STATE;
    }

    scriptState.requests |= SCRIPT_REQUEST_COMBAT;

    return 0;
}

// 0x4A45D4
void scripts_request_combat_locked(STRUCT_664980* a1)
{
    if (a1 != NULL) {
        memcpy(&scriptState.combatState1, a1, sizeof(scriptState.combatState1));
    } else {
        scriptState.requests |= SCRIPT_REQUEST_NO_INITIAL_COMBAT_STATE;
    }

    scriptState.requests |= (SCRIPT_REQUEST_LOCKED | SCRIPT_REQUEST_COMBAT);
}

// 0x4A4644
void scripts_request_worldmap()
{
    if (isInCombat()) {
        game_user_wants_to_quit = 1;
    }

    scriptState.requests |= SCRIPT_REQUEST_WORLD_MAP;
}

// scripts_request_elevator
// 0x4A466C
int scripts_request_elevator(Object* a1, int a2)
{
    int elevatorType = a2;
    int elevatorLevel = map_elevation;

    int tile = a1->tile;
    if (tile == -1) {
        debugPrint("\nError: scripts_request_elevator! Bad tile num");
        return -1;
    }

    // In the following code we are looking for an elevator. 5 tiles in each direction
    tile = tile - (HEX_GRID_WIDTH * 5) - 5; // left upper corner

    Object* obj;
    for (int y = -5; y < 5; y++) {
        for (int x = -5; x < 5; x++) {
            obj = obj_find_first_at(a1->elevation);
            while (obj != NULL) {
                if (tile == obj->tile && obj->pid == PROTO_ID_0x200050D) {
                    break;
                }

                obj = obj_find_next_at();
            }

            if (obj != NULL) {
                break;
            }

            tile += 1;
        }

        if (obj != NULL) {
            break;
        }

        tile += HEX_GRID_WIDTH - 10;
    }

    if (obj != NULL) {
        elevatorType = obj->data.scenery.elevator.type;
        elevatorLevel = obj->data.scenery.elevator.level;
    }

    if (elevatorType == -1) {
        return -1;
    }

    scriptState.requests |= SCRIPT_REQUEST_ELEVATOR;
    scriptState.elevatorType = elevatorType;
    scriptState.elevatorLevel = elevatorLevel;

    return 0;
}

// 0x4A4730
int scripts_request_explosion(int tile, int elevation, int minDamage, int maxDamage)
{
    scriptState.requests |= SCRIPT_REQUEST_EXPLOSION;
    scriptState.explosionTile = tile;
    scriptState.explosionElevation = elevation;
    scriptState.explosionMinDamage = minDamage;
    scriptState.explosionMaxDamage = maxDamage;
    return 0;
}

// 0x4A4754
void scripts_request_dialog(Object* obj)
{
    scriptState.dialogTarget = obj;
    scriptState.requests |= SCRIPT_REQUEST_DIALOG;
}

// 0x4A4770
void scripts_request_endgame_slideshow()
{
    scriptState.requests |= SCRIPT_REQUEST_ENDGAME;
}

// 0x4A477C
int scripts_request_loot_container(Object* a1, Object* a2)
{
    scriptState.lootingBy = a1;
    scriptState.lootingFrom = a2;
    scriptState.requests |= SCRIPT_REQUEST_LOOTING;
    return 0;
}

// 0x4A479C
int scripts_request_steal_container(Object* a1, Object* a2)
{
    scriptState.stealingBy = a1;
    scriptState.stealingFrom = a2;
    scriptState.requests |= SCRIPT_REQUEST_STEALING;
    return 0;
}

// NOTE: Inlined.
void script_make_path(char* path)
{
    strcpy(path, cd_path_base);
    strcat(path, script_path_base);
}

// exec_script_proc
// 0x4A4810
int exec_script_proc(int sid, int proc)
{
    if (!script_engine_running) {
        return -1;
    }

    Script* script;
    if (scr_ptr(sid, &script) == -1) {
        return -1;
    }

    script->scriptOverrides = 0;

    bool programLoaded = false;
    if ((script->flags & SCRIPT_FLAG_0x01) == 0) {
        clock();

        char name[16];
        if (scr_index_to_name(script->field_14 & 0xFFFFFF, name) == -1) {
            return -1;
        }

        char* pch = strchr(name, '.');
        if (pch != NULL) {
            *pch = '\0';
        }

        script->program = loadProgram(name);
        if (script->program == NULL) {
            debugPrint("\nError: exec_script_proc: script load failed!");
            return -1;
        }

        programLoaded = true;
        script->flags |= SCRIPT_FLAG_0x01;
    }

    Program* program = script->program;
    if (program == NULL) {
        return -1;
    }

    if ((program->flags & 0x0124) != 0) {
        return 0;
    }

    int v9 = script->procs[proc];
    if (v9 == 0) {
        v9 = 1;
    }

    if (v9 == -1) {
        return -1;
    }

    if (script->target == NULL) {
        script->target = script->owner;
    }

    script->flags |= SCRIPT_FLAG_0x04;

    if (programLoaded) {
        scr_build_lookup_table(script);

        v9 = script->procs[proc];
        if (v9 == 0) {
            v9 = 1;
        }

        script->action = 0;
        // NOTE: Uninline.
        runProgram(program);
        interpret(program, -1);
    }

    script->action = proc;

    executeProcedure(program, v9);

    script->source = NULL;

    return 0;
}

// Locate built-in procs for given script.
//
// 0x4A49D0
static int scr_build_lookup_table(Script* script)
{
    for (int proc = 0; proc < SCRIPT_PROC_COUNT; proc++) {
        int index = interpretFindProcedure(script->program, procTableStrs[proc]);
        if (index == -1) {
            index = SCRIPT_PROC_NO_PROC;
        }
        script->procs[proc] = index;
    }

    return 0;
}

// 0x4A4A08
bool scriptHasProc(int sid, int proc)
{
    Script* scr;

    if (scr_ptr(sid, &scr) == -1) {
        return 0;
    }

    return scr->procs[proc] != SCRIPT_PROC_NO_PROC;
}

// 0x4A4D50
static int scrInitListInfo()
{
    char path[MAX_PATH];
    script_make_path(path);
    strcat(path, "scripts.lst");

    File* stream = fileOpen(path, "rt");
    if (stream == NULL) {
        return -1;
    }

    char string[260];
    while (fileReadString(string, 260, stream)) {
        maxScriptNum++;

        ScriptsListEntry* entries = (ScriptsListEntry*)internal_realloc(scriptListInfo, sizeof(*entries) * maxScriptNum);
        if (entries == NULL) {
            return -1;
        }

        scriptListInfo = entries;

        ScriptsListEntry* entry = &(entries[maxScriptNum - 1]);
        entry->local_vars_num = 0;

        char* substr = strstr(string, ".int");
        if (substr != NULL) {
            int length = substr - string;
            if (length > 13) {
                return -1;
            }

            strncpy(entry->name, string, 13);
            entry->name[length] = '\0';
        }

        if (strstr(string, "#") != NULL) {
            substr = strstr(string, "local_vars=");
            if (substr != NULL) {
                entry->local_vars_num = atoi(substr + 11);
            }
        }
    }

    fileClose(stream);

    return 0;
}

// NOTE: Inlined.
//
// 0x4A4EFC
static int scrExitListInfo()
{
    if (scriptListInfo != NULL) {
        internal_free(scriptListInfo);
        scriptListInfo = NULL;
    }

    maxScriptNum = 0;

    return 0;
}

// 0x4A4F28
int scr_find_str_run_info(int scriptIndex, int* a2, int sid)
{
    Script* script;
    if (scr_ptr(sid, &script) == -1) {
        return -1;
    }

    script->localVarsCount = scriptListInfo[scriptIndex].local_vars_num;

    return 0;
}

// 0x4A4F68
static int scr_index_to_name(int scriptIndex, char* name)
{
    sprintf(name, "%s.int", scriptListInfo[scriptIndex].name);
    return 0;
}

// scr_set_dude_script
// 0x4A4F90
int scr_set_dude_script()
{
    if (scr_clear_dude_script() == -1) {
        return -1;
    }

    if (obj_dude == NULL) {
        debugPrint("Error in scr_set_dude_script: obj_dude uninitialized!");
        return -1;
    }

    Proto* proto;
    if (proto_ptr(0x1000000, &proto) == -1) {
        debugPrint("Error in scr_set_dude_script: can't find obj_dude proto!");
        return -1;
    }

    proto->critter.sid = 0x4000000;

    obj_new_sid(obj_dude, &(obj_dude->sid));

    Script* script;
    if (scr_ptr(obj_dude->sid, &script) == -1) {
        debugPrint("Error in scr_set_dude_script: can't find obj_dude script!");
        return -1;
    }

    script->flags |= (SCRIPT_FLAG_0x08 | SCRIPT_FLAG_0x10);

    return 0;
}

// scr_clear_dude_script
// 0x4A5044
int scr_clear_dude_script()
{
    if (obj_dude == NULL) {
        debugPrint("\nError in scr_clear_dude_script: obj_dude uninitialized!");
        return -1;
    }

    if (obj_dude->sid != -1) {
        Script* script;
        if (scr_ptr(obj_dude->sid, &script) != -1) {
            script->flags &= ~(SCRIPT_FLAG_0x08 | SCRIPT_FLAG_0x10);
        }

        scr_remove(obj_dude->sid);

        obj_dude->sid = -1;
    }

    return 0;
}

// scr_init
// 0x4A50A8
int scr_init()
{
    if (!message_init(&script_message_file)) {
        return -1;
    }

    for (int index = 0; index < 1450; index++) {
        if (!message_init(&(script_dialog_msgs[index]))) {
            return -1;
        }
    }

    scr_remove_all();
    interpretOutputFunc(_win_debug);
    initInterpreter();
    scr_header_load();

    // NOTE: Uninline.
    scripts_clear_state();

    partyMemberClear();

    if (scrInitListInfo() == -1) {
        return -1;
    }

    return 0;
}

// 0x4A5120
int scr_reset()
{
    scr_remove_all();

    // NOTE: Uninline.
    scripts_clear_state();

    partyMemberClear();

    return 0;
}

// 0x4A5138
int scr_game_init()
{
    int i;
    char path[MAX_PATH];

    if (!message_init(&script_message_file)) {
        debugPrint("\nError initing script message file!");
        return -1;
    }

    for (i = 0; i < 1450; i++) {
        if (!message_init(&(script_dialog_msgs[i]))) {
            debugPrint("\nERROR IN SCRIPT_DIALOG_MSGS!");
            return -1;
        }
    }

    sprintf(path, "%s%s", msg_path, "script.msg");
    if (!message_load(&script_message_file, path)) {
        debugPrint("\nError loading script message file!");
        return -1;
    }

    script_engine_running = true;
    script_engine_game_mode = 1;
    fallout_game_time = 1;
    gameTimeSetTime(302400);
    tickersAdd(doBkProcesses);

    if (scr_set_dude_script() == -1) {
        return -1;
    }

    scrSpatialsEnabled = true;

    // NOTE: Uninline.
    scripts_clear_state();

    return 0;
}

// 0x4A5240
int scr_game_reset()
{
    debugPrint("\nScripts: [Game Reset]");
    scr_game_exit();
    scr_game_init();
    partyMemberClear();
    scr_remove_all_force();
    return scr_set_dude_script();
}

// 0x4A5274
int scr_exit()
{
    script_engine_running = false;
    script_engine_run_critters = 0;
    if (!message_exit(&script_message_file)) {
        debugPrint("\nError exiting script message file!");
        return -1;
    }

    scr_remove_all();
    scr_remove_all_force();
    interpretClose();
    clearPrograms();

    // NOTE: Uninline.
    scripts_clear_state();

    // NOTE: Uninline.
    scrExitListInfo();

    return 0;
}

// scr_message_free
// 0x4A52F4
int scr_message_free()
{
    for (int index = 0; index < 1450; index++) {
        MessageList* messageList = &(script_dialog_msgs[index]);
        if (messageList->entries_num != 0) {
            if (!message_exit(messageList)) {
                debugPrint("\nERROR in scr_message_free!");
                return -1;
            }

            if (!message_init(messageList)) {
                debugPrint("\nERROR in scr_message_free!");
                return -1;
            }
        }
    }

    return 0;
}

// 0x4A535C
int scr_game_exit()
{
    script_engine_game_mode = 0;
    script_engine_running = false;
    script_engine_run_critters = 0;
    scr_message_free();
    scr_remove_all();
    clearPrograms();
    tickersRemove(doBkProcesses);
    message_exit(&script_message_file);
    if (scr_clear_dude_script() == -1) {
        return -1;
    }

    // NOTE: Uninline.
    scripts_clear_state();

    return 0;
}

// scr_enable
// 0x4A53A8
int scr_enable()
{
    if (!script_engine_game_mode) {
        return -1;
    }

    script_engine_run_critters = 1;
    script_engine_running = true;
    return 0;
}

// scr_disable
// 0x4A53D0
int scr_disable()
{
    script_engine_running = false;
    return 0;
}

// 0x4A53E0
void scr_enable_critters()
{
    script_engine_run_critters = 1;
}

// 0x4A53F0
void scr_disable_critters()
{
    script_engine_run_critters = 0;
}

// 0x4A5400
int scr_game_save(File* stream)
{
    return fileWriteInt32List(stream, game_global_vars, num_game_global_vars);
}

// 0x4A5424
int scr_game_load(File* stream)
{
    return fileReadInt32List(stream, game_global_vars, num_game_global_vars);
}

// NOTE: For unknown reason save game files contains two identical sets of game
// global variables (saved with [scr_game_save]). The first set is
// read with [scr_game_load], the second set is simply thrown away
// using this function.
//
// 0x4A5448
int scr_game_load2(File* stream)
{
    int* vars = (int*)internal_malloc(sizeof(*vars) * num_game_global_vars);
    if (vars == NULL) {
        return -1;
    }

    if (fileReadInt32List(stream, vars, num_game_global_vars) == -1) {
        // FIXME: Leaks vars.
        return -1;
    }

    internal_free(vars);

    return 0;
}

// 0x4A5490
static int scr_header_load()
{
    num_script_indexes = 0;

    char path[MAX_PATH];
    script_make_path(path);
    strcat(path, "scripts.lst");

    File* stream = fileOpen(path, "rt");
    if (stream == NULL) {
        return -1;
    }

    while (1) {
        int ch = fileReadChar(stream);
        if (ch == -1) {
            break;
        }

        if (ch == '\n') {
            num_script_indexes++;
        }
    }

    num_script_indexes++;

    fileClose(stream);

    for (int scriptType = 0; scriptType < SCRIPT_TYPE_COUNT; scriptType++) {
        ScriptList* scriptList = &(scriptlists[scriptType]);
        scriptList->head = NULL;
        scriptList->tail = NULL;
        scriptList->length = 0;
        scriptList->nextScriptId = 0;
    }

    return 0;
}

// 0x4A5590
static int scr_write_ScriptSubNode(Script* scr, File* stream)
{
    if (fileWriteInt32(stream, scr->sid) == -1) return -1;
    if (fileWriteInt32(stream, scr->field_4) == -1) return -1;

    switch (SID_TYPE(scr->sid)) {
    case SCRIPT_TYPE_SPATIAL:
        if (fileWriteInt32(stream, scr->sp.built_tile) == -1) return -1;
        if (fileWriteInt32(stream, scr->sp.radius) == -1) return -1;
        break;
    case SCRIPT_TYPE_TIMED:
        if (fileWriteInt32(stream, scr->tm.time) == -1) return -1;
        break;
    }

    if (fileWriteInt32(stream, scr->flags) == -1) return -1;
    if (fileWriteInt32(stream, scr->field_14) == -1) return -1;
    if (fileWriteInt32(stream, (int)scr->program) == -1) return -1; // FIXME: writing pointer to file
    if (fileWriteInt32(stream, scr->field_1C) == -1) return -1;
    if (fileWriteInt32(stream, scr->localVarsOffset) == -1) return -1;
    if (fileWriteInt32(stream, scr->localVarsCount) == -1) return -1;
    if (fileWriteInt32(stream, scr->field_28) == -1) return -1;
    if (fileWriteInt32(stream, scr->action) == -1) return -1;
    if (fileWriteInt32(stream, scr->fixedParam) == -1) return -1;
    if (fileWriteInt32(stream, scr->actionBeingUsed) == -1) return -1;
    if (fileWriteInt32(stream, scr->scriptOverrides) == -1) return -1;
    if (fileWriteInt32(stream, scr->field_48) == -1) return -1;
    if (fileWriteInt32(stream, scr->howMuch) == -1) return -1;
    if (fileWriteInt32(stream, scr->field_50) == -1) return -1;

    return 0;
}

// 0x4A5704
static int scr_write_ScriptNode(ScriptListExtent* a1, File* stream)
{
    for (int index = 0; index < SCRIPT_LIST_EXTENT_SIZE; index++) {
        Script* script = &(a1->scripts[index]);
        if (scr_write_ScriptSubNode(script, stream) != 0) {
            return -1;
        }
    }

    if (fileWriteInt32(stream, a1->length) != 0) {
        return -1;
    }

    if (fileWriteInt32(stream, (int)a1->next) != 0) {
        // FIXME: writing pointer to file
        return -1;
    }

    return 0;
}

// 0x4A5768
int scr_save(File* stream)
{
    for (int scriptType = 0; scriptType < SCRIPT_TYPE_COUNT; scriptType++) {
        ScriptList* scriptList = &(scriptlists[scriptType]);

        int scriptCount = scriptList->length * SCRIPT_LIST_EXTENT_SIZE;
        if (scriptList->tail != NULL) {
            scriptCount += scriptList->tail->length - SCRIPT_LIST_EXTENT_SIZE;
        }

        ScriptListExtent* scriptExtent = scriptList->head;
        ScriptListExtent* lastScriptExtent = NULL;
        while (scriptExtent != NULL) {
            for (int index = 0; index < scriptExtent->length; index++) {
                Script* script = &(scriptExtent->scripts[index]);

                lastScriptExtent = scriptList->tail;
                if ((script->flags & SCRIPT_FLAG_0x08) != 0) {
                    scriptCount--;

                    int backwardsIndex = lastScriptExtent->length - 1;
                    if (lastScriptExtent == scriptExtent && backwardsIndex <= index) {
                        break;
                    }

                    while (lastScriptExtent != scriptExtent || backwardsIndex > index) {
                        Script* backwardsScript = &(lastScriptExtent->scripts[backwardsIndex]);
                        if ((backwardsScript->flags & SCRIPT_FLAG_0x08) == 0) {
                            break;
                        }

                        backwardsIndex--;

                        if (backwardsIndex < 0) {
                            ScriptListExtent* previousScriptExtent = scriptList->head;
                            while (previousScriptExtent->next != lastScriptExtent) {
                                previousScriptExtent = previousScriptExtent->next;
                            }

                            lastScriptExtent = previousScriptExtent;
                            backwardsIndex = lastScriptExtent->length - 1;
                        }
                    }

                    if (lastScriptExtent != scriptExtent || backwardsIndex > index) {
                        Script temp;
                        memcpy(&temp, script, sizeof(Script));
                        memcpy(script, &(lastScriptExtent->scripts[backwardsIndex]), sizeof(Script));
                        memcpy(&(lastScriptExtent->scripts[backwardsIndex]), &temp, sizeof(Script));

                        scriptCount++;
                    }
                }
            }
            scriptExtent = scriptExtent->next;
        }

        if (fileWriteInt32(stream, scriptCount) == -1) {
            return -1;
        }

        if (scriptCount > 0) {
            ScriptListExtent* scriptExtent = scriptList->head;
            while (scriptExtent != lastScriptExtent) {
                if (scr_write_ScriptNode(scriptExtent, stream) == -1) {
                    return -1;
                }
                scriptExtent = scriptExtent->next;
            }

            if (lastScriptExtent != NULL) {
                int index;
                for (index = 0; index < lastScriptExtent->length; index++) {
                    Script* script = &(lastScriptExtent->scripts[index]);
                    if ((script->flags & SCRIPT_FLAG_0x08) != 0) {
                        break;
                    }
                }

                if (index > 0) {
                    int length = lastScriptExtent->length;
                    lastScriptExtent->length = index;
                    if (scr_write_ScriptNode(lastScriptExtent, stream) == -1) {
                        return -1;
                    }
                    lastScriptExtent->length = length;
                }
            }
        }
    }

    return 0;
}

// 0x4A5A1C
static int scr_read_ScriptSubNode(Script* scr, File* stream)
{
    int prg;

    if (fileReadInt32(stream, &(scr->sid)) == -1) return -1;
    if (fileReadInt32(stream, &(scr->field_4)) == -1) return -1;

    switch (SID_TYPE(scr->sid)) {
    case SCRIPT_TYPE_SPATIAL:
        if (fileReadInt32(stream, &(scr->sp.built_tile)) == -1) return -1;
        if (fileReadInt32(stream, &(scr->sp.radius)) == -1) return -1;
        break;
    case SCRIPT_TYPE_TIMED:
        if (fileReadInt32(stream, &(scr->tm.time)) == -1) return -1;
        break;
    }

    if (fileReadInt32(stream, &(scr->flags)) == -1) return -1;
    if (fileReadInt32(stream, &(scr->field_14)) == -1) return -1;
    if (fileReadInt32(stream, &(prg)) == -1) return -1;
    if (fileReadInt32(stream, &(scr->field_1C)) == -1) return -1;
    if (fileReadInt32(stream, &(scr->localVarsOffset)) == -1) return -1;
    if (fileReadInt32(stream, &(scr->localVarsCount)) == -1) return -1;
    if (fileReadInt32(stream, &(scr->field_28)) == -1) return -1;
    if (fileReadInt32(stream, &(scr->action)) == -1) return -1;
    if (fileReadInt32(stream, &(scr->fixedParam)) == -1) return -1;
    if (fileReadInt32(stream, &(scr->actionBeingUsed)) == -1) return -1;
    if (fileReadInt32(stream, &(scr->scriptOverrides)) == -1) return -1;
    if (fileReadInt32(stream, &(scr->field_48)) == -1) return -1;
    if (fileReadInt32(stream, &(scr->howMuch)) == -1) return -1;
    if (fileReadInt32(stream, &(scr->field_50)) == -1) return -1;

    scr->program = NULL;
    scr->owner = NULL;
    scr->source = NULL;
    scr->target = NULL;

    for (int index = 0; index < SCRIPT_PROC_COUNT; index++) {
        scr->procs[index] = 0;
    }

    if (!(map_data.flags & 1)) {
        scr->localVarsCount = 0;
    }

    return 0;
}

// 0x4A5BE8
static int scr_read_ScriptNode(ScriptListExtent* scriptExtent, File* stream)
{
    for (int index = 0; index < SCRIPT_LIST_EXTENT_SIZE; index++) {
        Script* scr = &(scriptExtent->scripts[index]);
        if (scr_read_ScriptSubNode(scr, stream) != 0) {
            return -1;
        }
    }

    if (fileReadInt32(stream, &(scriptExtent->length)) != 0) {
        return -1;
    }

    int next;
    if (fileReadInt32(stream, &(next)) != 0) {
        return -1;
    }

    return 0;
}

// 0x4A5C50
int scr_load(File* stream)
{
    for (int index = 0; index < SCRIPT_TYPE_COUNT; index++) {
        ScriptList* scriptList = &(scriptlists[index]);

        int scriptsCount = 0;
        if (fileReadInt32(stream, &scriptsCount) == -1) {
            return -1;
        }

        if (scriptsCount != 0) {
            scriptList->length = scriptsCount / 16;

            if (scriptsCount % 16 != 0) {
                scriptList->length++;
            }

            ScriptListExtent* extent = (ScriptListExtent*)internal_malloc(sizeof(*extent));
            scriptList->head = extent;
            scriptList->tail = extent;
            if (extent == NULL) {
                return -1;
            }

            if (scr_read_ScriptNode(extent, stream) != 0) {
                return -1;
            }

            for (int scriptIndex = 0; scriptIndex < extent->length; scriptIndex++) {
                Script* script = &(extent->scripts[scriptIndex]);
                script->owner = NULL;
                script->source = NULL;
                script->target = NULL;
                script->program = NULL;
                script->flags &= ~SCRIPT_FLAG_0x01;
            }

            extent->next = NULL;

            ScriptListExtent* prevExtent = extent;
            for (int extentIndex = 1; extentIndex < scriptList->length; extentIndex++) {
                ScriptListExtent* extent = (ScriptListExtent*)internal_malloc(sizeof(*extent));
                if (extent == NULL) {
                    return -1;
                }

                if (scr_read_ScriptNode(extent, stream) != 0) {
                    return -1;
                }

                for (int scriptIndex = 0; scriptIndex < extent->length; scriptIndex++) {
                    Script* script = &(extent->scripts[scriptIndex]);
                    script->owner = NULL;
                    script->source = NULL;
                    script->target = NULL;
                    script->program = NULL;
                    script->flags &= ~SCRIPT_FLAG_0x01;
                }

                prevExtent->next = extent;

                extent->next = NULL;
                prevExtent = extent;
            }

            scriptList->tail = prevExtent;
        } else {
            scriptList->head = NULL;
            scriptList->tail = NULL;
            scriptList->length = 0;
        }
    }

    return 0;
}

// scr_ptr
// 0x4A5E34
int scr_ptr(int sid, Script** scriptPtr)
{
    *scriptPtr = NULL;

    if (sid == -1) {
        return -1;
    }

    if (sid == 0xCCCCCCCC) {
        debugPrint("\nERROR: scr_ptr called with UN-SET id #!!!!");
        return -1;
    }

    ScriptList* scriptList = &(scriptlists[SID_TYPE(sid)]);
    ScriptListExtent* scriptListExtent = scriptList->head;

    while (scriptListExtent != NULL) {
        for (int index = 0; index < scriptListExtent->length; index++) {
            Script* script = &(scriptListExtent->scripts[index]);
            if (script->sid == sid) {
                *scriptPtr = script;
                return 0;
            }
        }
        scriptListExtent = scriptListExtent->next;
    }

    return -1;
}

// 0x4A5ED8
static int scr_new_id(int scriptType)
{
    int scriptId = scriptlists[scriptType].nextScriptId++;
    int v1 = scriptType << 24;

    while (scriptId < 32000) {
        Script* script;
        if (scr_ptr(v1 | scriptId, &script) == -1) {
            break;
        }
        scriptId++;
    }

    return scriptId;
}

// 0x4A5F28
int scr_new(int* sidPtr, int scriptType)
{
    ScriptList* scriptList = &(scriptlists[scriptType]);
    ScriptListExtent* scriptListExtent = scriptList->tail;
    if (scriptList->head != NULL) {
        // There is at least one extent available, which means tail is also set.
        if (scriptListExtent->length == SCRIPT_LIST_EXTENT_SIZE) {
            ScriptListExtent* newExtent = scriptListExtent->next = (ScriptListExtent*)internal_malloc(sizeof(*newExtent));
            if (newExtent == NULL) {
                return -1;
            }

            newExtent->length = 0;
            newExtent->next = NULL;

            scriptList->tail = newExtent;
            scriptList->length++;

            scriptListExtent = newExtent;
        }
    } else {
        // Script head
        scriptListExtent = (ScriptListExtent*)internal_malloc(sizeof(ScriptListExtent));
        if (scriptListExtent == NULL) {
            return -1;
        }

        scriptListExtent->length = 0;
        scriptListExtent->next = NULL;

        scriptList->head = scriptListExtent;
        scriptList->tail = scriptListExtent;
        scriptList->length = 1;
    }

    int sid = scr_new_id(scriptType) | (scriptType << 24);

    *sidPtr = sid;

    Script* scr = &(scriptListExtent->scripts[scriptListExtent->length]);
    scr->sid = sid;
    scr->sp.built_tile = -1;
    scr->sp.radius = -1;
    scr->flags = 0;
    scr->field_14 = -1;
    scr->program = 0;
    scr->localVarsOffset = -1;
    scr->localVarsCount = 0;
    scr->field_28 = 0;
    scr->action = 0;
    scr->fixedParam = 0;
    scr->owner = 0;
    scr->source = 0;
    scr->target = 0;
    scr->actionBeingUsed = -1;
    scr->scriptOverrides = 0;
    scr->field_48 = 0;
    scr->howMuch = 0;
    scr->field_50 = 0;

    for (int index = 0; index < SCRIPT_PROC_COUNT; index++) {
        scr->procs[index] = SCRIPT_PROC_NO_PROC;
    }

    scriptListExtent->length++;

    return 0;
}

// scr_remove_local_vars
// 0x4A60D4
int scr_remove_local_vars(Script* script)
{
    if (script == NULL) {
        return -1;
    }

    if (script->localVarsCount != 0) {
        int oldMapLocalVarsCount = num_map_local_vars;
        if (oldMapLocalVarsCount > 0 && script->localVarsOffset >= 0) {
            num_map_local_vars -= script->localVarsCount;

            if (oldMapLocalVarsCount - script->localVarsCount != script->localVarsOffset && script->localVarsOffset != -1) {
                memmove(map_local_vars + script->localVarsOffset,
                    map_local_vars + (script->localVarsOffset + script->localVarsCount),
                    sizeof(*map_local_vars) * (oldMapLocalVarsCount - script->localVarsCount - script->localVarsOffset));

                map_local_vars = (int*)internal_realloc(map_local_vars, sizeof(*map_local_vars) * num_map_local_vars);
                if (map_local_vars == NULL) {
                    debugPrint("\nError in mem_realloc in scr_remove_local_vars!\n");
                }

                for (int index = 0; index < SCRIPT_TYPE_COUNT; index++) {
                    ScriptList* scriptList = &(scriptlists[index]);
                    ScriptListExtent* extent = scriptList->head;
                    while (extent != NULL) {
                        for (int index = 0; index < extent->length; index++) {
                            Script* other = &(extent->scripts[index]);
                            if (other->localVarsOffset > script->localVarsOffset) {
                                other->localVarsOffset -= script->localVarsCount;
                            }
                        }
                        extent = extent->next;
                    }
                }
            }
        }
    }

    return 0;
}

// scr_remove
// 0x4A61D4
int scr_remove(int sid)
{
    if (sid == -1) {
        return -1;
    }

    ScriptList* scriptList = &(scriptlists[SID_TYPE(sid)]);

    ScriptListExtent* scriptListExtent = scriptList->head;
    int index;
    while (scriptListExtent != NULL) {
        for (index = 0; index < scriptListExtent->length; index++) {
            Script* script = &(scriptListExtent->scripts[index]);
            if (script->sid == sid) {
                break;
            }
        }

        if (index < scriptListExtent->length) {
            break;
        }

        scriptListExtent = scriptListExtent->next;
    }

    if (scriptListExtent == NULL) {
        return -1;
    }

    Script* script = &(scriptListExtent->scripts[index]);
    if ((script->flags & SCRIPT_FLAG_0x02) != 0) {
        if (script->program != NULL) {
            script->program = NULL;
        }
    }

    if ((script->flags & SCRIPT_FLAG_0x10) == 0) {
        // NOTE: Uninline.
        scripts_clear_combat_requests(script);

        if (scr_remove_local_vars(script) == -1) {
            debugPrint("\nERROR Removing local vars on scr_remove!!\n");
        }

        if (queue_remove_this(script->owner, EVENT_TYPE_SCRIPT) == -1) {
            debugPrint("\nERROR Removing Timed Events on scr_remove!!\n");
        }

        if (scriptListExtent == scriptList->tail && index + 1 == scriptListExtent->length) {
            // Removing last script in tail extent
            scriptListExtent->length -= 1;

            if (scriptListExtent->length == 0) {
                scriptList->length--;
                internal_free(scriptListExtent);

                if (scriptList->length != 0) {
                    ScriptListExtent* v13 = scriptList->head;
                    while (scriptList->tail != v13->next) {
                        v13 = v13->next;
                    }
                    v13->next = NULL;
                    scriptList->tail = v13;
                } else {
                    scriptList->head = NULL;
                    scriptList->tail = NULL;
                }
            }
        } else {
            // Relocate last script from tail extent into this script's slot.
            memcpy(&(scriptListExtent->scripts[index]), &(scriptList->tail->scripts[scriptList->tail->length - 1]), sizeof(Script));

            // Decrement number of scripts in tail extent.
            scriptList->tail->length -= 1;

            // Check to see if this extent became empty.
            if (scriptList->tail->length == 0) {
                scriptList->length -= 1;

                // Find previous extent that is about to become a new tail for
                // this script list.
                ScriptListExtent* prev = scriptList->head;
                while (prev->next != scriptList->tail) {
                    prev = prev->next;
                }
                prev->next = NULL;

                internal_free(scriptList->tail);
                scriptList->tail = prev;
            }
        }
    }

    return 0;
}

// 0x4A63E0
int scr_remove_all()
{
    queue_clear_type(EVENT_TYPE_SCRIPT, NULL);
    scr_message_free();

    for (int scrType = 0; scrType < SCRIPT_TYPE_COUNT; scrType++) {
        ScriptList* scriptList = &(scriptlists[scrType]);

        // TODO: Super odd way to remove scripts. The problem is that [scrRemove]
        // does relocate scripts between extents, so current extent may become
        // empty. In addition there is a 0x10 flag on the script that is not
        // removed. Find a way to refactor this.
        ScriptListExtent* scriptListExtent = scriptList->head;
        while (scriptListExtent != NULL) {
            ScriptListExtent* next = NULL;
            for (int scriptIndex = 0; scriptIndex < scriptListExtent->length;) {
                Script* script = &(scriptListExtent->scripts[scriptIndex]);

                if ((script->flags & SCRIPT_FLAG_0x10) != 0) {
                    scriptIndex++;
                } else {
                    if (scriptIndex != 0 || scriptListExtent->length != 1) {
                        scr_remove(script->sid);
                    } else {
                        next = scriptListExtent->next;
                        scr_remove(script->sid);
                    }
                }
            }

            scriptListExtent = next;
        }
    }

    scr_find_first_idx = 0;
    scr_find_first_ptr = NULL;
    scr_find_first_elev = 0;
    map_script_id = -1;

    clearPrograms();
    exportClearAllVariables();

    return 0;
}

// 0x4A64A8
int scr_remove_all_force()
{
    queue_clear_type(EVENT_TYPE_SCRIPT, NULL);
    scr_message_free();

    for (int type = 0; type < SCRIPT_TYPE_COUNT; type++) {
        ScriptList* scriptList = &(scriptlists[type]);
        ScriptListExtent* extent = scriptList->head;
        while (extent != NULL) {
            ScriptListExtent* next = extent->next;
            internal_free(extent);
            extent = next;
        }

        scriptList->head = NULL;
        scriptList->tail = NULL;
        scriptList->length = 0;
    }

    scr_find_first_idx = 0;
    scr_find_first_ptr = 0;
    scr_find_first_elev = 0;
    map_script_id = -1;
    clearPrograms();
    exportClearAllVariables();

    return 0;
}

// 0x4A6524
Script* scr_find_first_at(int elevation)
{
    scr_find_first_elev = elevation;
    scr_find_first_idx = 0;
    scr_find_first_ptr = scriptlists[SCRIPT_TYPE_SPATIAL].head;

    if (scr_find_first_ptr == NULL) {
        return NULL;
    }

    Script* script = &(scr_find_first_ptr->scripts[0]);
    if ((script->flags & SCRIPT_FLAG_0x02) != 0 || builtTileGetElevation(script->sp.built_tile) != elevation) {
        script = scr_find_next_at();
    }

    return script;
}

// 0x4A6564
Script* scr_find_next_at()
{
    ScriptListExtent* scriptListExtent = scr_find_first_ptr;
    int scriptIndex = scr_find_first_idx;

    if (scriptListExtent == NULL) {
        return NULL;
    }

    for (;;) {
        scriptIndex++;

        if (scriptIndex == SCRIPT_LIST_EXTENT_SIZE) {
            scriptListExtent = scriptListExtent->next;
            scriptIndex = 0;
        } else if (scriptIndex >= scriptListExtent->length) {
            scriptListExtent = NULL;
        }

        if (scriptListExtent == NULL) {
            break;
        }

        Script* script = &(scriptListExtent->scripts[scriptIndex]);
        if ((script->flags & SCRIPT_FLAG_0x02) == 0 && builtTileGetElevation(script->sp.built_tile) == scr_find_first_elev) {
            break;
        }
    }

    Script* script;
    if (scriptListExtent != NULL) {
        script = &(scriptListExtent->scripts[scriptIndex]);
    } else {
        script = NULL;
    }

    scr_find_first_idx = scriptIndex;
    scr_find_first_ptr = scriptListExtent;

    return script;
}

// 0x4A65F0
void scr_spatials_enable()
{
    scrSpatialsEnabled = true;
}

// 0x4A6600
void scr_spatials_disable()
{
    scrSpatialsEnabled = false;
}

// 0x4A6610
bool scr_chk_spatials_in(Object* object, int tile, int elevation)
{
    if (object == obj_mouse) {
        return false;
    }

    if (object == obj_mouse_flat) {
        return false;
    }

    if ((object->flags & OBJECT_HIDDEN) != 0 || (object->flags & OBJECT_FLAT) != 0) {
        return false;
    }

    if (tile < 10) {
        return false;
    }

    if (!scrSpatialsEnabled) {
        return false;
    }

    scrSpatialsEnabled = false;

    int builtTile = builtTileCreate(tile, elevation);

    for (Script* script = scr_find_first_at(elevation); script != NULL; script = scr_find_next_at()) {
        if (builtTile == script->sp.built_tile) {
            // NOTE: Uninline.
            scr_set_objs(script->sid, object, NULL);
        } else {
            if (script->sp.radius == 0) {
                continue;
            }

            int distance = tileDistanceBetween(builtTileGetTile(script->sp.built_tile), tile);
            if (distance > script->sp.radius) {
                continue;
            }

            // NOTE: Uninline.
            scr_set_objs(script->sid, object, NULL);
        }

        exec_script_proc(script->sid, SCRIPT_PROC_SPATIAL);
    }

    scrSpatialsEnabled = true;

    return true;
}

// 0x4A677C
int scr_load_all_scripts()
{
    for (int scriptListIndex = 0; scriptListIndex < SCRIPT_TYPE_COUNT; scriptListIndex++) {
        ScriptList* scriptList = &(scriptlists[scriptListIndex]);
        ScriptListExtent* extent = scriptList->head;
        while (extent != NULL) {
            for (int scriptIndex = 0; scriptIndex < extent->length; scriptIndex++) {
                Script* script = &(extent->scripts[scriptIndex]);
                exec_script_proc(script->sid, SCRIPT_PROC_START);
            }
            extent = extent->next;
        }
    }

    return 0;
}

// 0x4A67DC
void scr_exec_map_enter_scripts()
{
    scrExecMapProcScripts(SCRIPT_PROC_MAP_ENTER);
}

// 0x4A67E4
void scr_exec_map_update_scripts()
{
    scrExecMapProcScripts(SCRIPT_PROC_MAP_UPDATE);
}

// 0x4A67EC
static void scrExecMapProcScripts(int proc)
{
    scrSpatialsEnabled = false;

    int fixedParam = 0;
    if (proc == SCRIPT_PROC_MAP_ENTER) {
        fixedParam = (map_data.flags & 1) == 0;
    } else {
        exec_script_proc(map_script_id, proc);
    }

    int sidListCapacity = 0;
    for (int scriptType = 0; scriptType < SCRIPT_TYPE_COUNT; scriptType++) {
        ScriptList* scriptList = &(scriptlists[scriptType]);
        ScriptListExtent* scriptListExtent = scriptList->head;
        while (scriptListExtent != NULL) {
            sidListCapacity += scriptListExtent->length;
            scriptListExtent = scriptListExtent->next;
        }
    }

    if (sidListCapacity == 0) {
        return;
    }

    int* sidList = (int*)internal_malloc(sizeof(*sidList) * sidListCapacity);
    if (sidList == NULL) {
        debugPrint("\nError: scr_exec_map_update_scripts: Out of memory for sidList!");
        return;
    }

    int sidListLength = 0;
    for (int scriptType = 0; scriptType < SCRIPT_TYPE_COUNT; scriptType++) {
        ScriptList* scriptList = &(scriptlists[scriptType]);
        ScriptListExtent* scriptListExtent = scriptList->head;
        while (scriptListExtent != NULL) {
            for (int scriptIndex = 0; scriptIndex < scriptListExtent->length; scriptIndex++) {
                Script* script = &(scriptListExtent->scripts[scriptIndex]);
                if (script->sid != map_script_id && script->procs[proc] > 0) {
                    sidList[sidListLength++] = script->sid;
                }
            }
            scriptListExtent = scriptListExtent->next;
        }
    }

    if (proc == SCRIPT_PROC_MAP_ENTER) {
        for (int index = 0; index < sidListLength; index++) {
            Script* script;
            if (scr_ptr(sidList[index], &script) != -1) {
                script->fixedParam = fixedParam;
            }

            exec_script_proc(sidList[index], proc);
        }
    } else {
        for (int index = 0; index < sidListLength; index++) {
            exec_script_proc(sidList[index], proc);
        }
    }

    internal_free(sidList);

    scrSpatialsEnabled = true;
}

// 0x4A69A0
void scr_exec_map_exit_scripts()
{
    scrExecMapProcScripts(SCRIPT_PROC_MAP_EXIT);
}

// 0x4A6B64
int scr_get_dialog_msg_file(int a1, MessageList** messageListPtr)
{
    if (a1 == -1) {
        return -1;
    }

    int messageListIndex = a1 - 1;
    MessageList* messageList = &(script_dialog_msgs[messageListIndex]);
    if (messageList->entries_num == 0) {
        char scriptName[20];
        scriptName[0] = '\0';
        scr_index_to_name(messageListIndex & 0xFFFFFF, scriptName);

        char* pch = strrchr(scriptName, '.');
        if (pch != NULL) {
            *pch = '\0';
        }

        char path[MAX_PATH];
        sprintf(path, "dialog\\%s.msg", scriptName);

        if (!message_load(messageList, path)) {
            debugPrint("\nError loading script dialog message file!");
            return -1;
        }

        if (!message_filter(messageList)) {
            debugPrint("\nError filtering script dialog message file!");
            return -1;
        }
    }

    *messageListPtr = messageList;

    return 0;
}

// 0x4A6C50
char* scr_get_msg_str(int messageListId, int messageId)
{
    return scr_get_msg_str_speech(messageListId, messageId, 0);
}

// 0x4A6C5C
char* scr_get_msg_str_speech(int messageListId, int messageId, int a3)
{
    // 0x51C7F0
    static char err_str[] = "Error";

    // 0x51C7F4
    static char blank_str[] = "";

    if (messageListId == 0 && messageId == 0) {
        return blank_str;
    }

    if (messageListId == -1 && messageId == -1) {
        return blank_str;
    }

    if (messageListId == -2 && messageId == -2) {
        MessageListItem messageListItem;
        return getmsg(&proto_main_msg_file, &messageListItem, 650);
    }

    MessageList* messageList;
    if (scr_get_dialog_msg_file(messageListId, &messageList) == -1) {
        debugPrint("\nERROR: message_str: can't find message file: List: %d!", messageListId);
        return NULL;
    }

    if (FID_TYPE(dialogue_head) != OBJ_TYPE_HEAD) {
        a3 = 0;
    }

    MessageListItem messageListItem;
    messageListItem.num = messageId;
    if (!message_search(messageList, &messageListItem)) {
        debugPrint("\nError: can't find message: List: %d, Num: %d!", messageListId, messageId);
        return err_str;
    }

    if (a3) {
        if (gdialogActive()) {
            if (messageListItem.audio != NULL && messageListItem.audio[0] != '\0') {
                if (messageListItem.flags & 0x01) {
                    gdialogSetupSpeech(NULL);
                } else {
                    gdialogSetupSpeech(messageListItem.audio);
                }
            } else {
                debugPrint("Missing speech name: %d\n", messageListItem.num);
            }
        }
    }

    return messageListItem.text;
}

// 0x4A6D64
int scr_get_local_var(int sid, int variable, int* value)
{
    // 0x667750
    static char tempStr[20];

    if (SID_TYPE(sid) == SCRIPT_TYPE_SYSTEM) {
        debugPrint("\nError! System scripts/Map scripts not allowed local_vars! ");

        tempStr[0] = '\0';
        scr_index_to_name(sid & 0xFFFFFF, tempStr);

        debugPrint(":%s\n", tempStr);

        *value = -1;
        return -1;
    }

    Script* script;
    if (scr_ptr(sid, &script) == -1) {
        *value = -1;
        return -1;
    }

    if (script->localVarsCount == 0) {
        // NOTE: Uninline.
        scr_find_str_run_info(script->field_14, &(script->field_50), sid);
    }

    if (script->localVarsCount > 0) {
        if (script->localVarsOffset == -1) {
            script->localVarsOffset = map_malloc_local_var(script->localVarsCount);
        }

        *value = map_get_local_var(script->localVarsOffset + variable);
    }

    return 0;
}

// 0x4A6E58
int scr_set_local_var(int sid, int variable, int value)
{
    Script* script;
    if (scr_ptr(sid, &script) == -1) {
        return -1;
    }

    if (script->localVarsCount == 0) {
        // NOTE: Uninline.
        scr_find_str_run_info(script->field_14, &(script->field_50), sid);
    }

    if (script->localVarsCount <= 0) {
        return -1;
    }

    if (script->localVarsOffset == -1) {
        script->localVarsOffset = map_malloc_local_var(script->localVarsCount);
    }

    map_set_local_var(script->localVarsOffset + variable, value);

    return 0;
}

// Performs combat script and returns true if default action has been overriden
// by script.
//
// 0x4A6EFC
bool scr_end_combat()
{
    if (map_script_id == 0 || map_script_id == -1) {
        return false;
    }

    int team = combat_player_knocked_out_by();
    if (team == -1) {
        return false;
    }

    Script* before;
    if (scr_ptr(map_script_id, &before) != -1) {
        before->fixedParam = team;
    }

    exec_script_proc(map_script_id, SCRIPT_PROC_COMBAT);

    bool success = false;

    Script* after;
    if (scr_ptr(map_script_id, &after) != -1) {
        if (after->scriptOverrides != 0) {
            success = true;
        }
    }

    return success;
}

// 0x4A6F70
int scr_explode_scenery(Object* a1, int tile, int radius, int elevation)
{
    int scriptExtentsCount = scriptlists[SCRIPT_TYPE_SPATIAL].length + scriptlists[SCRIPT_TYPE_ITEM].length;
    if (scriptExtentsCount == 0) {
        return 0;
    }

    int* scriptIds = (int*)internal_malloc(sizeof(*scriptIds) * scriptExtentsCount * SCRIPT_LIST_EXTENT_SIZE);
    if (scriptIds == NULL) {
        return -1;
    }

    ScriptListExtent* extent;
    int scriptsCount = 0;

    scrSpatialsEnabled = false;

    extent = scriptlists[SCRIPT_TYPE_ITEM].head;
    while (extent != NULL) {
        for (int index = 0; index < extent->length; index++) {
            Script* script = &(extent->scripts[index]);
            if (script->procs[SCRIPT_PROC_DAMAGE] <= 0 && script->program == NULL) {
                exec_script_proc(script->sid, SCRIPT_PROC_START);
            }

            if (script->procs[SCRIPT_PROC_DAMAGE] > 0) {
                Object* self = script->owner;
                if (self != NULL) {
                    if (self->elevation == elevation && tileDistanceBetween(self->tile, tile) <= radius) {
                        scriptIds[scriptsCount] = script->sid;
                        scriptsCount += 1;
                    }
                }
            }
        }
        extent = extent->next;
    }

    extent = scriptlists[SCRIPT_TYPE_SPATIAL].head;
    while (extent != NULL) {
        for (int index = 0; index < extent->length; index++) {
            Script* script = &(extent->scripts[index]);
            if (script->procs[SCRIPT_PROC_DAMAGE] <= 0 && script->program == NULL) {
                exec_script_proc(script->sid, SCRIPT_PROC_START);
            }

            if (script->procs[SCRIPT_PROC_DAMAGE] > 0
                && builtTileGetElevation(script->sp.built_tile) == elevation
                && tileDistanceBetween(builtTileGetTile(script->sp.built_tile), tile) <= radius) {
                scriptIds[scriptsCount] = script->sid;
                scriptsCount += 1;
            }
        }
        extent = extent->next;
    }

    for (int index = 0; index < scriptsCount; index++) {
        Script* script;
        int sid = scriptIds[index];

        if (scr_ptr(sid, &script) != -1) {
            script->fixedParam = 20;
        }

        // TODO: Obtaining script twice, probably some inlining.
        if (scr_ptr(sid, &script) != -1) {
            script->source = NULL;
            script->target = a1;
        }

        exec_script_proc(sid, SCRIPT_PROC_DAMAGE);
    }

    // TODO: Redundant, we already know `scriptIds` is not NULL.
    if (scriptIds != NULL) {
        internal_free(scriptIds);
    }

    scrSpatialsEnabled = true;

    return 0;
}
