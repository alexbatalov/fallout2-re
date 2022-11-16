#ifndef FALLOUT_GAME_MAP_H_
#define FALLOUT_GAME_MAP_H_

#include <stdbool.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "game/combat_defs.h"
#include "db.h"
#include "plib/gnw/rect.h"
#include "game/map_defs.h"
#include "game/message.h"

// TODO: Probably not needed -> replace with array?
typedef struct TileData {
    int field_0[SQUARE_GRID_SIZE];
} TileData;

typedef struct MapHeader {
    // map_ver
    int version;

    // map_name
    char name[16];

    // map_ent_tile
    int enteringTile;

    // map_ent_elev
    int enteringElevation;

    // map_ent_rot
    int enteringRotation;

    // map_num_loc_vars
    int localVariablesCount;

    // 0map_script_idx
    int scriptIndex;

    // map_flags
    int flags;

    // map_darkness
    int darkness;

    // map_num_glob_vars
    int globalVariablesCount;

    // map_number
    int field_34;

    // Time in game ticks when PC last visited this map.
    int lastVisitTime;
    int field_3C[44];
} MapHeader;

typedef struct MapTransition {
    int map;
    int elevation;
    int tile;
    int rotation;
} MapTransition;

typedef void IsoWindowRefreshProc(Rect* rect);

extern char byte_50B058[];
extern char _aErrorF2[];
extern int map_script_id;
extern int* map_local_vars;
extern int* map_global_vars;
extern int num_map_local_vars;
extern int num_map_global_vars;
extern int map_elevation;

extern TileData square_data[ELEVATION_COUNT];
extern MessageList map_msg_file;
extern MapHeader map_data;
extern TileData* square[ELEVATION_COUNT];
extern int display_win;

int iso_init();
void iso_reset();
void iso_exit();
void map_init();
void map_reset();
void map_exit();
void map_enable_bk_processes();
bool map_disable_bk_processes();
bool map_bk_processes_are_disabled();
int map_set_elevation(int elevation);
bool map_is_elevation_empty(int elevation);
int map_set_global_var(int var, int value);
int map_get_global_var(int var);
int map_set_local_var(int var, int value);
int map_get_local_var(int var);
int map_malloc_local_var(int a1);
void map_set_entrance_hex(int a1, int a2, int a3);
void map_set_name(const char* name);
void map_get_name(char* name);
char* map_get_elev_idx(int map_num, int elev);
bool is_map_idx_same(int map_num1, int map_num2);
int get_map_idx_same(int map_num1, int map_num2);
char* map_get_short_name(int map_num);
char* map_get_description();
char* map_get_description_idx(int map_index);
int map_get_index_number();
int map_scroll(int dx, int dy);
char* map_file_path(char* name);
int mapSetEntranceInfo(int a1, int a2, int a3);
void map_new_map();
int map_load(char* fileName);
int map_load_idx(int map_index);
int map_load_file(File* stream);
int map_load_in_game(char* fileName);
static int map_age_dead_critters();
int map_target_load_area();
int map_leave_map(MapTransition* transition);
int map_check_state();
void map_fix_critter_combat_data();
int map_save();
int map_save_file(File* stream);
int map_save_in_game(bool a1);
void map_setup_paths();

#endif /* FALLOUT_GAME_MAP_H_ */
