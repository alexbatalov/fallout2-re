#ifndef FALLOUT_GAME_AUTOMAP_H_
#define FALLOUT_GAME_AUTOMAP_H_

#include <stdbool.h>

#include "db.h"
#include "game/map_defs.h"

#define AUTOMAP_DB "AUTOMAP.DB"
#define AUTOMAP_TMP "AUTOMAP.TMP"

// The number of map entries that is stored in automap.db.
//
// NOTE: I don't know why this value is not equal to the number of maps.
#define AUTOMAP_MAP_COUNT 160

typedef struct AutomapHeader {
    unsigned char version;

    // The size of entire automap database (including header itself).
    int dataSize;

    // Offsets from the beginning of the automap database file into
    // entries data.
    //
    // These offsets are specified for every map/elevation combination. A value
    // of 0 specifies that there is no data for appropriate map/elevation
    // combination.
    int offsets[AUTOMAP_MAP_COUNT][ELEVATION_COUNT];
} AutomapHeader;

typedef struct AutomapEntry {
    int dataSize;
    unsigned char isCompressed;
} AutomapEntry;

int automap_init();
int automap_reset();
void automap_exit();
int automap_load(File* stream);
int automap_save(File* stream);
int automapDisplayMap(int map);
void automap(bool isInGame, bool isUsingScanner);
int draw_top_down_map_pipboy(int win, int map, int elevation);
int automap_pip_save();
int YesWriteIndex(int mapIndex, int elevation);
int ReadAMList(AutomapHeader** automapHeaderPtr);

#endif /* FALLOUT_GAME_AUTOMAP_H_ */
