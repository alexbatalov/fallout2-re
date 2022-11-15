#include "game/light.h"

#include <math.h>

#include "game/map_defs.h"
#include "game/object.h"
#include "game/perk.h"
#include "tile.h"

// 0x51923C
static int ambient_light = LIGHT_LEVEL_MAX;

// 0x59E994
static int tile_intensity[ELEVATION_COUNT][HEX_GRID_SIZE];

// 0x47A8F0
int light_init()
{
    light_reset_tiles();
    return 0;
}

// NOTE: From OS X.
void light_reset()
{
    light_reset_tiles();
}

// NOTE: From OS X.
void light_exit()
{
    light_reset_tiles();
}

// 0x47A8F8
int light_get_ambient()
{
    return ambient_light;
}

// 0x47A908
void light_set_ambient(int lightLevel, bool shouldUpdateScreen)
{
    int normalizedLightLevel;
    int oldLightLevel;

    normalizedLightLevel = lightLevel + perkGetRank(obj_dude, PERK_NIGHT_VISION) * LIGHT_LEVEL_NIGHT_VISION_BONUS;

    if (normalizedLightLevel < LIGHT_LEVEL_MIN) {
        normalizedLightLevel = LIGHT_LEVEL_MIN;
    }

    if (normalizedLightLevel > LIGHT_LEVEL_MAX) {
        normalizedLightLevel = LIGHT_LEVEL_MAX;
    }

    oldLightLevel = ambient_light;
    ambient_light = normalizedLightLevel;

    if (shouldUpdateScreen) {
        if (oldLightLevel != normalizedLightLevel) {
            tileWindowRefresh();
        }
    }
}

// NOTE: From OS X.
void light_increase_ambient(int value, bool shouldUpdateScreen)
{
    light_set_ambient(ambient_light + value, shouldUpdateScreen);
}

// 0x47A96C
void light_decrease_ambient(int value, bool shouldUpdateScreen)
{
    light_set_ambient(ambient_light - value, shouldUpdateScreen);
}

// 0x47A980
int light_get_tile(int elevation, int tile)
{
    int result;

    if (!elevationIsValid(elevation)) {
        return 0;
    }

    if (!hexGridTileIsValid(tile)) {
        return 0;
    }

    result = tile_intensity[elevation][tile];
    if (result >= LIGHT_LEVEL_MAX) {
        result = LIGHT_LEVEL_MAX;
    }

    return result;
}

// 0x47A9C4
int light_get_tile_true(int elevation, int tile)
{
    if (!elevationIsValid(elevation)) {
        return 0;
    }

    if (!hexGridTileIsValid(tile)) {
        return 0;
    }

    return tile_intensity[elevation][tile];
}

// 0x47A9EC
void light_set_tile(int elevation, int tile, int lightIntensity)
{
    if (!elevationIsValid(elevation)) {
        return;
    }

    if (!hexGridTileIsValid(tile)) {
        return;
    }

    tile_intensity[elevation][tile] = lightIntensity;
}

// 0x47AA10
void light_add_to_tile(int elevation, int tile, int lightIntensity)
{
    if (!elevationIsValid(elevation)) {
        return;
    }

    if (!hexGridTileIsValid(tile)) {
        return;
    }

    tile_intensity[elevation][tile] += lightIntensity;
}

// 0x47AA48
void light_subtract_from_tile(int elevation, int tile, int lightIntensity)
{
    if (!elevationIsValid(elevation)) {
        return;
    }

    if (!hexGridTileIsValid(tile)) {
        return;
    }

    tile_intensity[elevation][tile] -= lightIntensity;
}

// 0x47AA84
void light_reset_tiles()
{
    int elevation;
    int tile;

    for (elevation = 0; elevation < ELEVATION_COUNT; elevation++) {
        for (tile = 0; tile < HEX_GRID_SIZE; tile++) {
            tile_intensity[elevation][tile] = 655;
        }
    }
}
