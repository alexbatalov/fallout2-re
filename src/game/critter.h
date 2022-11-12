#ifndef FALLOUT_GAME_CRITTER_H_
#define FALLOUT_GAME_CRITTER_H_

#include <stdbool.h>

#include "db.h"
#include "obj_types.h"
#include "proto_types.h"

// Maximum length of dude's name length.
#define DUDE_NAME_MAX_LENGTH 32

// The number of effects caused by radiation.
//
// A radiation effect is an identifier and does not have it's own name. It's
// stat is specified in [rad_stat], and it's amount is specified
// in [rad_bonus] for every [RadiationLevel].
#define RADIATION_EFFECT_COUNT 8

// Radiation levels.
//
// The names of levels are taken from Fallout 3, comments from Fallout 2.
typedef enum RadiationLevel {
    // Very nauseous.
    RADIATION_LEVEL_NONE,

    // Slightly fatigued.
    RADIATION_LEVEL_MINOR,

    // Vomiting does not stop.
    RADIATION_LEVEL_ADVANCED,

    // Hair is falling out.
    RADIATION_LEVEL_CRITICAL,

    // Skin is falling off.
    RADIATION_LEVEL_DEADLY,

    // Intense agony.
    RADIATION_LEVEL_FATAL,

    // The number of radiation levels.
    RADIATION_LEVEL_COUNT,
} RadiationLevel;

typedef enum DudeState {
    DUDE_STATE_SNEAKING = 0,
    DUDE_STATE_LEVEL_UP_AVAILABLE = 3,
    DUDE_STATE_ADDICTED = 4,
} DudeState;

extern int rad_stat[RADIATION_EFFECT_COUNT];
extern int rad_bonus[RADIATION_LEVEL_COUNT][RADIATION_EFFECT_COUNT];

int critter_init();
void critter_reset();
void critter_exit();
int critter_load(File* stream);
int critter_save(File* stream);
char* critter_name(Object* obj);
void critter_copy(CritterProtoData* dest, CritterProtoData* src);
int critter_pc_set_name(const char* name);
void critter_pc_reset_name();
int critter_get_hits(Object* critter);
int critter_adjust_hits(Object* critter, int hp);
int critter_get_poison(Object* critter);
int critter_adjust_poison(Object* obj, int amount);
int critter_check_poison(Object* obj, void* data);
int critter_get_rads(Object* critter);
int critter_adjust_rads(Object* obj, int amount);
int critter_check_rads(Object* critter);
int critter_process_rads(Object* obj, void* data);
int critter_load_rads(File* stream, void** dataPtr);
int critter_save_rads(File* stream, void* data);
int critter_get_base_damage_type(Object* critter);
int critter_kill_count_inc(int killType);
int critter_kill_count(int killType);
int critter_kill_count_load(File* stream);
int critter_kill_count_save(File* stream);
int critterGetKillType(Object* critter);
char* critter_kill_name(int killType);
char* critter_kill_info(int killType);
int critter_heal_hours(Object* obj, int a2);
void critter_kill(Object* critter, int anim, bool a3);
int critter_kill_exps(Object* critter);
bool critter_is_active(Object* critter);
bool critter_is_dead(Object* critter);
bool critter_is_crippled(Object* critter);
bool critter_is_prone(Object* critter);
int critter_body_type(Object* critter);
int critter_load_data(CritterProtoData* critterData, const char* path);
int pc_load_data(const char* path);
int critter_read_data(File* stream, CritterProtoData* critterData);
int critter_save_data(CritterProtoData* critterData, const char* path);
int pc_save_data(const char* path);
int critter_write_data(File* stream, CritterProtoData* critterData);
void pc_flag_off(int state);
void pc_flag_on(int state);
void pc_flag_toggle(int state);
bool is_pc_flag(int state);
int critter_sneak_check(Object* obj, void* data);
int critter_sneak_clear(Object* obj, void* data);
bool is_pc_sneak_working();
int critter_wake_up(Object* obj, void* data);
int critter_wake_clear(Object* obj, void* data);
int critter_set_who_hit_me(Object* a1, Object* a2);
bool critter_can_obj_dude_rest();
int critter_compute_ap_from_distance(Object* critter, int a2);
bool critterIsOverloaded(Object* critter);
bool critter_is_fleeing(Object* a1);
bool critter_flag_check(int pid, int flag);
void critter_flag_set(int pid, int flag);
void critter_flag_unset(int pid, int flag);
void critter_flag_toggle(int pid, int flag);

#endif /* FALLOUT_GAME_CRITTER_H_ */
