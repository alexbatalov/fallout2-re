#include "worldmap.h"

#include "animation.h"
#include "color.h"
#include "combat.h"
#include "combat_ai.h"
#include "core.h"
#include "critter.h"
#include "cycle.h"
#include "dbox.h"
#include "debug.h"
#include "display_monitor.h"
#include "draw.h"
#include "game.h"
#include "game_config.h"
#include "game_mouse.h"
#include "game_movie.h"
#include "game_sound.h"
#include "interface.h"
#include "item.h"
#include "memory.h"
#include "object.h"
#include "party_member.h"
#include "perk.h"
#include "proto_instance.h"
#include "queue.h"
#include "random.h"
#include "scripts.h"
#include "skill.h"
#include "stat.h"
#include "string_parsers.h"
#include "text_font.h"
#include "tile.h"
#include "window_manager.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static_assert(sizeof(Terrain) == 128, "wrong size");
static_assert(sizeof(ENC_BASE_TYPE) == 3056, "wrong size");
static_assert(sizeof(EncounterTable) == 7460, "wrong size");

#define WM_TILE_WIDTH (350)
#define WM_TILE_HEIGHT (300)

#define WM_SUBTILE_SIZE (50)

#define WM_WINDOW_WIDTH (640)
#define WM_WINDOW_HEIGHT (480)

#define WM_VIEW_X (22)
#define WM_VIEW_Y (21)
#define WM_VIEW_WIDTH (450)
#define WM_VIEW_HEIGHT (443)

typedef struct WmGenData {
    bool mousePressed;
    bool didMeetFrankHorrigan;

    int currentAreaId;
    int worldPosX;
    int worldPosY;
    SubtileInfo* currentSubtile;

    int dword_672E18;

    bool isWalking;
    int walkDestinationX;
    int walkDestinationY;
    int walkDistance;
    int walkLineDelta;
    int walkLineDeltaMainAxisStep;
    int walkLineDeltaCrossAxisStep;
    int walkWorldPosMainAxisStepX;
    int walkWorldPosCrossAxisStepX;
    int walkWorldPosMainAxisStepY;
    int walkWorldPosCrossAxisStepY;

    int encounterIconIsVisible;
    int encounterMapId;
    int encounterTableId;
    int encounterEntryId;
    int encounterCursorId;

    int oldWorldPosX;
    int oldWorldPosY;

    bool isInCar;
    int currentCarAreaId;
    int carFuel;

    CacheEntry* carImageFrmHandle;
    Art* carImageFrm;
    int carImageFrmWidth;
    int carImageFrmHeight;
    int carImageCurrentFrameIndex;

    CacheEntry* hotspotNormalFrmHandle;
    unsigned char* hotspotNormalFrmData;
    CacheEntry* hotspotPressedFrmHandle;
    unsigned char* hotspotPressedFrmData;
    int hotspotFrmWidth;
    int hotspotFrmHeight;

    CacheEntry* destinationMarkerFrmHandle;
    unsigned char* destinationMarkerFrmData;
    int destinationMarkerFrmWidth;
    int destinationMarkerFrmHeight;

    CacheEntry* locationMarkerFrmHandle;
    unsigned char* locationMarkerFrmData;
    int locationMarkerFrmWidth;
    int locationMarkerFrmHeight;

    CacheEntry* encounterCursorFrmHandle[WORLD_MAP_ENCOUNTER_FRM_COUNT];
    unsigned char* encounterCursorFrmData[WORLD_MAP_ENCOUNTER_FRM_COUNT];
    int encounterCursorFrmWidths[WORLD_MAP_ENCOUNTER_FRM_COUNT];
    int encounterCursorFrmHeights[WORLD_MAP_ENCOUNTER_FRM_COUNT];

    int viewportMaxX;
    int viewportMaxY;

    CacheEntry* tabsBackgroundFrmHandle;
    int tabsBackgroundFrmWidth;
    int tabsBackgroundFrmHeight;
    int tabsOffsetY;
    unsigned char* tabsBackgroundFrmData;

    CacheEntry* tabsBorderFrmHandle;
    unsigned char* tabsBorderFrmData;

    CacheEntry* dialFrmHandle;
    int dialFrmWidth;
    int dialFrmHeight;
    int dialFrmCurrentFrameIndex;
    Art* dialFrm;

    CacheEntry* carImageOverlayFrmHandle;
    int carImageOverlayFrmWidth;
    int carImageOverlayFrmHeight;
    unsigned char* carImageOverlayFrmData;

    CacheEntry* globeOverlayFrmHandle;
    int globeOverlayFrmWidth;
    int globeOverlayFrmHeight;
    unsigned char* globeOverlayFrmData;

    int oldTabsOffsetY;
    int tabsScrollingDelta;

    CacheEntry* littleRedButtonNormalFrmHandle;
    CacheEntry* littleRedButtonPressedFrmHandle;
    unsigned char* littleRedButtonNormalFrmData;
    unsigned char* littleRedButtonPressedFrmData;

    CacheEntry* scrollUpButtonFrmHandle[WORLDMAP_ARROW_FRM_COUNT];
    int scrollUpButtonFrmWidth;
    int scrollUpButtonFrmHeight;
    unsigned char* scrollUpButtonFrmData[WORLDMAP_ARROW_FRM_COUNT];

    CacheEntry* scrollDownButtonFrmHandle[WORLDMAP_ARROW_FRM_COUNT];
    int scrollDownButtonFrmWidth;
    int scrollDownButtonFrmHeight;
    unsigned char* scrollDownButtonFrmData[WORLDMAP_ARROW_FRM_COUNT];

    CacheEntry* monthsFrmHandle;
    Art* monthsFrm;

    CacheEntry* numbersFrmHandle;
    Art* numbersFrm;

    int oldFont;
} WmGenData;

// 0x4BC860
const int _can_rest_here[ELEVATION_COUNT] = {
    MAP_CAN_REST_ELEVATION_0,
    MAP_CAN_REST_ELEVATION_1,
    MAP_CAN_REST_ELEVATION_2,
};

// 0x4BC86C
const int gDayPartEncounterFrequencyModifiers[DAY_PART_COUNT] = {
    40,
    30,
    0,
};

// 0x4BC878
const char* gWorldmapEncDefaultMsg[2] = {
    "You detect something up ahead.",
    "Do you wish to encounter it?",
};

// 0x4BC880
MessageListItem gWorldmapMessageListItem;

// 0x50EE44
char _aCricket[] = "cricket";

// 0x50EE4C
char _aCricket1[] = "cricket1";

// 0x51DD88
const char* _wmStateStrs[2] = {
    "off",
    "on"
};

// 0x51DD90
const char* _wmYesNoStrs[2] = {
    "no",
    "yes",
};

// 0x51DD98
const char* gEncounterFrequencyTypeKeys[ENCOUNTER_FREQUENCY_TYPE_COUNT] = {
    "none",
    "rare",
    "uncommon",
    "common",
    "frequent",
    "forced",
};

// 0x51DDB0
const char* _wmFillStrs[9] = {
    "no_fill",
    "fill_n",
    "fill_s",
    "fill_e",
    "fill_w",
    "fill_nw",
    "fill_ne",
    "fill_sw",
    "fill_se",
};

// 0x51DDD4
const char* _wmSceneryStrs[ENCOUNTER_SCENERY_TYPE_COUNT] = {
    "none",
    "light",
    "normal",
    "heavy",
};

// 0x51DDE4
Terrain* gTerrains = NULL;

// 0x51DDE8
int gTerrainsLength = 0;

// 0x51DDEC
TileInfo* gWorldmapTiles = NULL;

// 0x51DDF0
int gWorldmapTilesLength = 0;

// The width of worldmap grid in tiles.
//
// There is no separate variable for grid height, instead its calculated as
// [gWorldmapTilesLength] / [gWorldmapTilesGridWidth].
//
// num_horizontal_tiles
// 0x51DDF4
int gWorldmapGridWidth = 0;

// 0x51DDF8
CityInfo* gCities = NULL;

// 0x51DDFC
int gCitiesLength = 0;

// 0x51DE00
const char* gCitySizeKeys[CITY_SIZE_COUNT] = {
    "small",
    "medium",
    "large",
};

// 0x51DE0C
MapInfo* gMaps = NULL;

// 0x51DE10
int gMapsLength = 0;

// 0x51DE14
int gWorldmapWindow = -1;

// 0x51DE18
CacheEntry* gWorldmapBoxFrmHandle = INVALID_CACHE_ENTRY;

// 0x51DE1C
int gWorldmapBoxFrmWidth = 0;

// 0x51DE20
int gWorldmapBoxFrmHeight = 0;

// 0x51DE24
unsigned char* gWorldmapWindowBuffer = NULL;

// 0x51DE28
unsigned char* gWorldmapBoxFrmData = NULL;

// 0x51DE2C
int gWorldmapOffsetX = 0;

// 0x51DE30
int gWorldmapOffsetY = 0;

//
unsigned char* _circleBlendTable = NULL;

//
int _wmInterfaceWasInitialized = 0;

// encounter types
const char* _wmEncOpStrs[ENCOUNTER_SITUATION_COUNT] = {
    "nothing",
    "ambush",
    "fighting",
    "and",
};

// operators
const char* _wmConditionalOpStrs[ENCOUNTER_CONDITIONAL_OPERATOR_COUNT] = {
    "_",
    "==",
    "!=",
    "<",
    ">",
};

// 0x51DE6C
const char* gEncounterFormationTypeKeys[ENCOUNTER_FORMATION_TYPE_COUNT] = {
    "surrounding",
    "straight_line",
    "double_line",
    "wedge",
    "cone",
    "huddle",
};

// 0x51DE84
int gWorldmapEncounterFrmIds[WORLD_MAP_ENCOUNTER_FRM_COUNT] = {
    154,
    155,
    438,
    439,
};

// 0x51DE94
int* gQuickDestinations = NULL;

// 0x51DE98
int gQuickDestinationsLength = 0;

// 0x51DE9C
int _wmTownMapCurArea = -1;

// 0x51DEA0
unsigned int _wmLastRndTime = 0;

// 0x51DEA4
int _wmRndIndex = 0;

// 0x51DEA8
int _wmRndCallCount = 0;

// 0x51DEAC
int _terrainCounter = 1;

// 0x51DEB0
unsigned int _lastTime_2 = 0;

// 0x51DEB4
bool _couldScroll = true;

// 0x51DEB8
unsigned char* gWorldmapCityMapFrmData = NULL;

// 0x51DEBC
CacheEntry* gWorldmapCityMapFrmHandle = INVALID_CACHE_ENTRY;

// 0x51DEC0
int gWorldmapCityMapFrmWidth = 0;

// 0x51DEC4
int gWorldmapCityMapFrmHeight = 0;

// 0x51DEC8
char* _wmRemapSfxList[2] = {
    _aCricket,
    _aCricket1,
};

// 0x672DB8
int _wmRndTileDirs[2];

// 0x672DC0
int _wmRndCenterTiles[2];

// 0x672DC8
int _wmRndCenterRotations[2];

// 0x672DD0
int _wmRndRotOffsets[2];

// Buttons for city entrances.
//
// 0x672DD8
int _wmTownMapButtonId[ENTRANCE_LIST_CAPACITY];

// NOTE: There are no symbols in |mapper2.exe| for the range between |wmGenData|
// and |wmMsgFile| implying everything in between are fields of the large
// struct.
//
// 0x672E00
static WmGenData wmGenData;

// worldmap.msg
//
// 0x672FB0
MessageList gWorldmapMessageList;

// 0x672FB8
int _wmFreqValues[6];

// 0x672FD0
int _wmRndOriginalCenterTile;

// worldmap.txt
//
// 0x672FD4
Config* gWorldmapConfig;

// 0x672FD8
int _wmTownMapSubButtonIds[7];

// 0x672FF4
ENC_BASE_TYPE* _wmEncBaseTypeList;

// 0x672FF8
CitySizeDescription gCitySizeDescriptions[CITY_SIZE_COUNT];

// 0x673034
EncounterTable* gEncounterTables;

// Number of enc_base_types.
//
// 0x673038
int _wmMaxEncBaseTypes;

// 0x67303C
int gEncounterTablesLength;

// 0x4BC890
void wmSetFlags(int* flagsPtr, int flag, int value)
{
    if (value) {
        *flagsPtr |= flag;
    } else {
        *flagsPtr &= ~flag;
    }
}

// wmWorldMap_init
// 0x4BC89C
int worldmapInit()
{
    char path[MAX_PATH];

    if (_wmGenDataInit() == -1) {
        return -1;
    }

    if (!messageListInit(&gWorldmapMessageList)) {
        return -1;
    }

    sprintf(path, "%s%s", asc_5186C8, "worldmap.msg");

    if (!messageListLoad(&gWorldmapMessageList, path)) {
        return -1;
    }

    if (worldmapConfigInit() == -1) {
        return -1;
    }

    wmGenData.viewportMaxX = WM_TILE_WIDTH * gWorldmapGridWidth - WM_VIEW_WIDTH;
    wmGenData.viewportMaxY = WM_TILE_HEIGHT * (gWorldmapTilesLength / gWorldmapGridWidth) - WM_VIEW_HEIGHT;
    _circleBlendTable = _getColorBlendTable(_colorTable[992]);

    _wmMarkSubTileRadiusVisited(wmGenData.worldPosX, wmGenData.worldPosY);
    _wmWorldMapSaveTempData();

    return 0;
}

// 0x4BC984
int _wmGenDataInit()
{
    wmGenData.didMeetFrankHorrigan = false;
    wmGenData.currentAreaId = -1;
    wmGenData.worldPosX = 173;
    wmGenData.worldPosY = 122;
    wmGenData.currentSubtile = NULL;
    wmGenData.dword_672E18 = 0;
    wmGenData.isWalking = false;
    wmGenData.walkDestinationX = -1;
    wmGenData.walkDestinationY = -1;
    wmGenData.walkDistance = 0;
    wmGenData.walkLineDelta = 0;
    wmGenData.walkLineDeltaMainAxisStep = 0;
    wmGenData.walkLineDeltaCrossAxisStep = 0;
    wmGenData.walkWorldPosMainAxisStepX = 0;
    wmGenData.walkWorldPosMainAxisStepY = 0;
    wmGenData.walkWorldPosCrossAxisStepY = 0;
    wmGenData.encounterIconIsVisible = 0;
    wmGenData.encounterMapId = -1;
    wmGenData.encounterTableId = -1;
    wmGenData.encounterEntryId = -1;
    wmGenData.encounterCursorId = -1;
    wmGenData.oldWorldPosX = 0;
    wmGenData.oldWorldPosY = 0;
    wmGenData.isInCar = false;
    wmGenData.currentCarAreaId = -1;
    wmGenData.carFuel = CAR_FUEL_MAX;
    wmGenData.carImageFrmHandle = INVALID_CACHE_ENTRY;
    wmGenData.carImageFrmWidth = 0;
    wmGenData.carImageFrmHeight = 0;
    wmGenData.carImageCurrentFrameIndex = 0;
    wmGenData.hotspotNormalFrmHandle = INVALID_CACHE_ENTRY;
    wmGenData.hotspotNormalFrmData = NULL;
    wmGenData.hotspotPressedFrmHandle = INVALID_CACHE_ENTRY;
    wmGenData.hotspotPressedFrmData = NULL;
    wmGenData.hotspotFrmWidth = 0;
    wmGenData.hotspotFrmHeight = 0;
    wmGenData.destinationMarkerFrmHandle = INVALID_CACHE_ENTRY;
    wmGenData.destinationMarkerFrmData = NULL;
    wmGenData.destinationMarkerFrmWidth = 0;
    wmGenData.destinationMarkerFrmHeight = 0;
    wmGenData.locationMarkerFrmHandle = INVALID_CACHE_ENTRY;
    wmGenData.locationMarkerFrmData = NULL;
    wmGenData.locationMarkerFrmWidth = 0;
    wmGenData.mousePressed = false;
    wmGenData.walkWorldPosCrossAxisStepX = 0;
    wmGenData.locationMarkerFrmHeight = 0;
    wmGenData.carImageFrm = NULL;

    for (int index = 0; index < WORLD_MAP_ENCOUNTER_FRM_COUNT; index++) {
        wmGenData.encounterCursorFrmHandle[index] = INVALID_CACHE_ENTRY;
        wmGenData.encounterCursorFrmData[index] = NULL;
        wmGenData.encounterCursorFrmWidths[index] = 0;
        wmGenData.encounterCursorFrmHeights[index] = 0;
    }

    wmGenData.viewportMaxY = 0;
    wmGenData.tabsBackgroundFrmHandle = INVALID_CACHE_ENTRY;
    wmGenData.tabsBackgroundFrmData = NULL;
    wmGenData.tabsBackgroundFrmWidth = 0;
    wmGenData.tabsBackgroundFrmHeight = 0;
    wmGenData.tabsOffsetY = 0;
    wmGenData.tabsBorderFrmHandle = INVALID_CACHE_ENTRY;
    wmGenData.tabsBorderFrmData = 0;
    wmGenData.dialFrmHandle = INVALID_CACHE_ENTRY;
    wmGenData.dialFrm = NULL;
    wmGenData.dialFrmWidth = 0;
    wmGenData.dialFrmHeight = 0;
    wmGenData.dialFrmCurrentFrameIndex = 0;
    wmGenData.carImageOverlayFrmHandle = INVALID_CACHE_ENTRY;
    wmGenData.carImageOverlayFrmData = NULL;
    wmGenData.carImageOverlayFrmWidth = 0;
    wmGenData.carImageOverlayFrmHeight = 0;
    wmGenData.globeOverlayFrmHandle = INVALID_CACHE_ENTRY;
    wmGenData.globeOverlayFrmData = NULL;
    wmGenData.globeOverlayFrmWidth = 0;
    wmGenData.globeOverlayFrmHeight = 0;
    wmGenData.oldTabsOffsetY = 0;
    wmGenData.tabsScrollingDelta = 0;
    wmGenData.littleRedButtonNormalFrmHandle = INVALID_CACHE_ENTRY;
    wmGenData.littleRedButtonPressedFrmHandle = INVALID_CACHE_ENTRY;
    wmGenData.littleRedButtonNormalFrmData = NULL;
    wmGenData.littleRedButtonPressedFrmData = NULL;
    wmGenData.viewportMaxX = 0;

    for (int index = 0; index < WORLDMAP_ARROW_FRM_COUNT; index++) {
        wmGenData.scrollDownButtonFrmHandle[index] = INVALID_CACHE_ENTRY;
        wmGenData.scrollUpButtonFrmHandle[index] = INVALID_CACHE_ENTRY;
        wmGenData.scrollUpButtonFrmData[index] = NULL;
        wmGenData.scrollDownButtonFrmData[index] = NULL;
    }

    wmGenData.scrollUpButtonFrmHeight = 0;
    wmGenData.scrollDownButtonFrmWidth = 0;
    wmGenData.scrollDownButtonFrmHeight = 0;
    wmGenData.monthsFrmHandle = INVALID_CACHE_ENTRY;
    wmGenData.monthsFrm = NULL;
    wmGenData.numbersFrmHandle = INVALID_CACHE_ENTRY;
    wmGenData.numbersFrm = NULL;
    wmGenData.scrollUpButtonFrmWidth = 0;

    return 0;
}

// 0x4BCBFC
int _wmGenDataReset()
{
    wmGenData.didMeetFrankHorrigan = false;
    wmGenData.currentSubtile = NULL;
    wmGenData.dword_672E18 = 0;
    wmGenData.isWalking = false;
    wmGenData.walkDistance = 0;
    wmGenData.walkLineDelta = 0;
    wmGenData.walkLineDeltaMainAxisStep = 0;
    wmGenData.walkLineDeltaCrossAxisStep = 0;
    wmGenData.walkWorldPosMainAxisStepX = 0;
    wmGenData.walkWorldPosMainAxisStepY = 0;
    wmGenData.walkWorldPosCrossAxisStepY = 0;
    wmGenData.encounterIconIsVisible = 0;
    wmGenData.mousePressed = false;
    wmGenData.currentAreaId = -1;
    wmGenData.worldPosX = 173;
    wmGenData.worldPosY = 122;
    wmGenData.walkDestinationX = -1;
    wmGenData.walkDestinationY = -1;
    wmGenData.encounterMapId = -1;
    wmGenData.encounterTableId = -1;
    wmGenData.encounterEntryId = -1;
    wmGenData.encounterCursorId = -1;
    wmGenData.currentCarAreaId = -1;
    wmGenData.carFuel = CAR_FUEL_MAX;
    wmGenData.carImageFrmHandle = INVALID_CACHE_ENTRY;
    wmGenData.tabsBackgroundFrmHandle = INVALID_CACHE_ENTRY;
    wmGenData.tabsBorderFrmHandle = INVALID_CACHE_ENTRY;
    wmGenData.dialFrmHandle = INVALID_CACHE_ENTRY;
    wmGenData.carImageOverlayFrmHandle = INVALID_CACHE_ENTRY;
    wmGenData.globeOverlayFrmHandle = INVALID_CACHE_ENTRY;
    wmGenData.littleRedButtonNormalFrmHandle = INVALID_CACHE_ENTRY;
    wmGenData.littleRedButtonPressedFrmHandle = INVALID_CACHE_ENTRY;
    wmGenData.walkWorldPosCrossAxisStepX = 0;
    wmGenData.oldWorldPosX = 0;
    wmGenData.oldWorldPosY = 0;
    wmGenData.isInCar = false;
    wmGenData.carImageFrmWidth = 0;
    wmGenData.carImageFrmHeight = 0;
    wmGenData.carImageCurrentFrameIndex = 0;
    wmGenData.tabsBackgroundFrmData = NULL;
    wmGenData.tabsBackgroundFrmHeight = 0;
    wmGenData.tabsOffsetY = 0;
    wmGenData.tabsBorderFrmData = 0;
    wmGenData.dialFrm = NULL;
    wmGenData.dialFrmWidth = 0;
    wmGenData.dialFrmHeight = 0;
    wmGenData.dialFrmCurrentFrameIndex = 0;
    wmGenData.carImageOverlayFrmData = NULL;
    wmGenData.carImageOverlayFrmWidth = 0;
    wmGenData.carImageOverlayFrmHeight = 0;
    wmGenData.globeOverlayFrmData = NULL;
    wmGenData.globeOverlayFrmWidth = 0;
    wmGenData.globeOverlayFrmHeight = 0;
    wmGenData.oldTabsOffsetY = 0;
    wmGenData.tabsScrollingDelta = 0;
    wmGenData.littleRedButtonNormalFrmData = NULL;
    wmGenData.littleRedButtonPressedFrmData = NULL;
    wmGenData.tabsBackgroundFrmWidth = 0;
    wmGenData.carImageFrm = NULL;

    for (int index = 0; index < WORLDMAP_ARROW_FRM_COUNT; index++) {
        wmGenData.scrollUpButtonFrmData[index] = NULL;
        wmGenData.scrollDownButtonFrmHandle[index] = INVALID_CACHE_ENTRY;

        wmGenData.scrollDownButtonFrmData[index] = NULL;
        wmGenData.scrollUpButtonFrmHandle[index] = INVALID_CACHE_ENTRY;
    }

    wmGenData.monthsFrmHandle = INVALID_CACHE_ENTRY;
    wmGenData.numbersFrmHandle = INVALID_CACHE_ENTRY;
    wmGenData.scrollUpButtonFrmWidth = 0;
    wmGenData.scrollUpButtonFrmHeight = 0;
    wmGenData.scrollDownButtonFrmWidth = 0;
    wmGenData.scrollDownButtonFrmHeight = 0;
    wmGenData.monthsFrm = NULL;
    wmGenData.numbersFrm = NULL;

    _wmMarkSubTileRadiusVisited(wmGenData.worldPosX, wmGenData.worldPosY);

    return 0;
}

// 0x4BCE00
void worldmapExit()
{
    if (gTerrains != NULL) {
        internal_free(gTerrains);
        gTerrains = NULL;
    }

    if (gWorldmapTiles) {
        internal_free(gWorldmapTiles);
        gWorldmapTiles = NULL;
    }

    gWorldmapGridWidth = 0;
    gWorldmapTilesLength = 0;

    if (gEncounterTables != NULL) {
        internal_free(gEncounterTables);
        gEncounterTables = NULL;
    }

    gEncounterTablesLength = 0;

    if (_wmEncBaseTypeList != NULL) {
        internal_free(_wmEncBaseTypeList);
        _wmEncBaseTypeList = NULL;
    }

    _wmMaxEncBaseTypes = 0;

    if (gCities != NULL) {
        internal_free(gCities);
        gCities = NULL;
    }

    gCitiesLength = 0;

    if (gMaps != NULL) {
        internal_free(gMaps);
    }

    gMapsLength = 0;

    if (_circleBlendTable != NULL) {
        _freeColorBlendTable(_colorTable[992]);
        _circleBlendTable = NULL;
    }

    messageListFree(&gWorldmapMessageList);
}

// 0x4BCEF8
int worldmapReset()
{
    gWorldmapOffsetX = 0;
    gWorldmapOffsetY = 0;

    _wmWorldMapLoadTempData();
    _wmMarkAllSubTiles(0);

    return _wmGenDataReset();
}

// 0x4BCF28
int worldmapSave(File* stream)
{
    int i;
    int j;
    int k;
    EncounterTable* encounter_table;
    EncounterEntry* encounter_entry;

    if (fileWriteBool(stream, wmGenData.didMeetFrankHorrigan) == -1) return -1;
    if (fileWriteInt32(stream, wmGenData.currentAreaId) == -1) return -1;
    if (fileWriteInt32(stream, wmGenData.worldPosX) == -1) return -1;
    if (fileWriteInt32(stream, wmGenData.worldPosY) == -1) return -1;
    if (fileWriteInt32(stream, wmGenData.encounterIconIsVisible) == -1) return -1;
    if (fileWriteInt32(stream, wmGenData.encounterMapId) == -1) return -1;
    if (fileWriteInt32(stream, wmGenData.encounterTableId) == -1) return -1;
    if (fileWriteInt32(stream, wmGenData.encounterEntryId) == -1) return -1;
    if (fileWriteBool(stream, wmGenData.isInCar) == -1) return -1;
    if (fileWriteInt32(stream, wmGenData.currentCarAreaId) == -1) return -1;
    if (fileWriteInt32(stream, wmGenData.carFuel) == -1) return -1;
    if (fileWriteInt32(stream, gCitiesLength) == -1) return -1;

    for (int cityIndex = 0; cityIndex < gCitiesLength; cityIndex++) {
        CityInfo* cityInfo = &(gCities[cityIndex]);
        if (fileWriteInt32(stream, cityInfo->x) == -1) return -1;
        if (fileWriteInt32(stream, cityInfo->y) == -1) return -1;
        if (fileWriteInt32(stream, cityInfo->state) == -1) return -1;
        if (fileWriteInt32(stream, cityInfo->visitedState) == -1) return -1;
        if (fileWriteInt32(stream, cityInfo->entrancesLength) == -1) return -1;

        for (int entranceIndex = 0; entranceIndex < cityInfo->entrancesLength; entranceIndex++) {
            EntranceInfo* entrance = &(cityInfo->entrances[entranceIndex]);
            if (fileWriteInt32(stream, entrance->state) == -1) return -1;
        }
    }

    if (fileWriteInt32(stream, gWorldmapTilesLength) == -1) return -1;
    if (fileWriteInt32(stream, gWorldmapGridWidth) == -1) return -1;

    for (int tileIndex = 0; tileIndex < gWorldmapTilesLength; tileIndex++) {
        TileInfo* tileInfo = &(gWorldmapTiles[tileIndex]);

        for (int column = 0; column < SUBTILE_GRID_HEIGHT; column++) {
            for (int row = 0; row < SUBTILE_GRID_WIDTH; row++) {
                SubtileInfo* subtile = &(tileInfo->subtiles[column][row]);

                if (fileWriteInt32(stream, subtile->state) == -1) return -1;
            }
        }
    }

    k = 0;
    for (i = 0; i < gEncounterTablesLength; i++) {
        encounter_table = &(gEncounterTables[i]);

        for (j = 0; j < encounter_table->entriesLength; j++) {
            encounter_entry = &(encounter_table->entries[j]);

            if (encounter_entry->counter != -1) {
                k++;
            }
        }
    }

    if (fileWriteInt32(stream, k) == -1) return -1;

    for (i = 0; i < gEncounterTablesLength; i++) {
        encounter_table = &(gEncounterTables[i]);

        for (j = 0; j < encounter_table->entriesLength; j++) {
            encounter_entry = &(encounter_table->entries[j]);

            if (encounter_entry->counter != -1) {
                if (fileWriteInt32(stream, i) == -1) return -1;
                if (fileWriteInt32(stream, j) == -1) return -1;
                if (fileWriteInt32(stream, encounter_entry->counter) == -1) return -1;
            }
        }
    }

    return 0;
}

// 0x4BD28C
int worldmapLoad(File* stream)
{
    int i;
    int j;
    int k;
    int cities_count;
    int v38;
    int v39;
    int v35;
    EncounterTable* encounter_table;
    EncounterEntry* encounter_entry;

    if (fileReadBool(stream, &(wmGenData.didMeetFrankHorrigan)) == -1) return -1;
    if (fileReadInt32(stream, &(wmGenData.currentAreaId)) == -1) return -1;
    if (fileReadInt32(stream, &(wmGenData.worldPosX)) == -1) return -1;
    if (fileReadInt32(stream, &(wmGenData.worldPosY)) == -1) return -1;
    if (fileReadInt32(stream, &(wmGenData.encounterIconIsVisible)) == -1) return -1;
    if (fileReadInt32(stream, &(wmGenData.encounterMapId)) == -1) return -1;
    if (fileReadInt32(stream, &(wmGenData.encounterTableId)) == -1) return -1;
    if (fileReadInt32(stream, &(wmGenData.encounterEntryId)) == -1) return -1;
    if (fileReadBool(stream, &(wmGenData.isInCar)) == -1) return -1;
    if (fileReadInt32(stream, &(wmGenData.currentCarAreaId)) == -1) return -1;
    if (fileReadInt32(stream, &(wmGenData.carFuel)) == -1) return -1;
    if (fileReadInt32(stream, &(cities_count)) == -1) return -1;

    for (int cityIndex = 0; cityIndex < cities_count; cityIndex++) {
        CityInfo* city = &(gCities[cityIndex]);

        if (fileReadInt32(stream, &(city->x)) == -1) return -1;
        if (fileReadInt32(stream, &(city->y)) == -1) return -1;
        if (fileReadInt32(stream, &(city->state)) == -1) return -1;
        if (fileReadInt32(stream, &(city->visitedState)) == -1) return -1;

        int entranceCount;
        if (fileReadInt32(stream, &(entranceCount)) == -1) {
            return -1;
        }

        for (int entranceIndex = 0; entranceIndex < entranceCount; entranceIndex++) {
            EntranceInfo* entrance = &(city->entrances[entranceIndex]);

            if (fileReadInt32(stream, &(entrance->state)) == -1) {
                return -1;
            }
        }
    }

    if (fileReadInt32(stream, &(v39)) == -1) return -1;
    if (fileReadInt32(stream, &(v38)) == -1) return -1;

    for (int tileIndex = 0; tileIndex < v39; tileIndex++) {
        TileInfo* tile = &(gWorldmapTiles[tileIndex]);

        for (int column = 0; column < SUBTILE_GRID_HEIGHT; column++) {
            for (int row = 0; row < SUBTILE_GRID_WIDTH; row++) {
                SubtileInfo* subtile = &(tile->subtiles[column][row]);

                if (fileReadInt32(stream, &(subtile->state)) == -1) return -1;
            }
        }
    }

    if (fileReadInt32(stream, &(v35)) == -1) return -1;

    for (i = 0; i < v35; i++) {
        if (fileReadInt32(stream, &(j)) == -1) return -1;
        encounter_table = &(gEncounterTables[j]);

        if (fileReadInt32(stream, &(k)) == -1) return -1;
        encounter_entry = &(encounter_table->entries[k]);

        if (fileReadInt32(stream, &(encounter_entry->counter)) == -1) return -1;
    }

    _wmInterfaceCenterOnParty();

    return 0;
}

// 0x4BD678
int _wmWorldMapSaveTempData()
{
    File* stream = fileOpen("worldmap.dat", "wb");
    if (stream == NULL) {
        return -1;
    }

    int rc = 0;
    if (worldmapSave(stream) == -1) {
        rc = -1;
    }

    fileClose(stream);

    return rc;
}

// 0x4BD6B4
int _wmWorldMapLoadTempData()
{
    File* stream = fileOpen("worldmap.dat", "rb");
    if (stream == NULL) {
        return -1;
    }

    int rc = 0;
    if (worldmapLoad(stream) == -1) {
        rc = -1;
    }

    fileClose(stream);

    return rc;
}

// 0x4BD6F0
int worldmapConfigInit()
{
    if (cityInit() == -1) {
        return -1;
    }

    Config config;
    if (!configInit(&config)) {
        return -1;
    }

    if (configRead(&config, "data\\worldmap.txt", true)) {
        for (int index = 0; index < ENCOUNTER_FREQUENCY_TYPE_COUNT; index++) {
            if (!configGetInt(&config, "data", gEncounterFrequencyTypeKeys[index], &(_wmFreqValues[index]))) {
                break;
            }
        }

        char* terrainTypes;
        configGetString(&config, "data", "terrain_types", &terrainTypes);
        _wmParseTerrainTypes(&config, terrainTypes);

        for (int index = 0;; index++) {
            char section[40];
            sprintf(section, "Encounter Table %d", index);

            char* lookupName;
            if (!configGetString(&config, section, "lookup_name", &lookupName)) {
                break;
            }

            if (worldmapConfigLoadEncounterTable(&config, lookupName, section) == -1) {
                return -1;
            }
        }

        if (!configGetInt(&config, "Tile Data", "num_horizontal_tiles", &gWorldmapGridWidth)) {
            showMesageBox("\nwmConfigInit::Error loading tile data!");
            return -1;
        }

        for (int tileIndex = 0; tileIndex < 9999; tileIndex++) {
            char section[40];
            sprintf(section, "Tile %d", tileIndex);

            int artIndex;
            if (!configGetInt(&config, section, "art_idx", &artIndex)) {
                break;
            }

            gWorldmapTilesLength++;

            TileInfo* worldmapTiles = (TileInfo*)internal_realloc(gWorldmapTiles, sizeof(*gWorldmapTiles) * gWorldmapTilesLength);
            if (worldmapTiles == NULL) {
                showMesageBox("\nwmConfigInit::Error loading tiles!");
                exit(1);
            }

            gWorldmapTiles = worldmapTiles;

            TileInfo* tile = &(worldmapTiles[gWorldmapTilesLength - 1]);

            // NOTE: Uninline.
            worldmapTileInfoInit(tile);

            tile->fid = buildFid(OBJ_TYPE_INTERFACE, artIndex, 0, 0, 0);

            int encounterDifficulty;
            if (configGetInt(&config, section, "encounter_difficulty", &encounterDifficulty)) {
                tile->encounterDifficultyModifier = encounterDifficulty;
            }

            char* walkMaskName;
            if (configGetString(&config, section, "walk_mask_name", &walkMaskName)) {
                strncpy(tile->walkMaskName, walkMaskName, TILE_WALK_MASK_NAME_SIZE);
            }

            for (int column = 0; column < SUBTILE_GRID_HEIGHT; column++) {
                for (int row = 0; row < SUBTILE_GRID_WIDTH; row++) {
                    char key[40];
                    sprintf(key, "%d_%d", row, column);

                    char* subtileProps;
                    if (!configGetString(&config, section, key, &subtileProps)) {
                        showMesageBox("\nwmConfigInit::Error loading tiles!");
                        exit(1);
                    }

                    if (worldmapConfigLoadSubtile(tile, row, column, subtileProps) == -1) {
                        showMesageBox("\nwmConfigInit::Error loading tiles!");
                        exit(1);
                    }
                }
            }
        }
    }

    configFree(&config);

    return 0;
}

// 0x4BD9F0
int worldmapConfigLoadEncounterTable(Config* config, char* lookupName, char* sectionKey)
{
    gEncounterTablesLength++;

    EncounterTable* encounterTables = (EncounterTable*)internal_realloc(gEncounterTables, sizeof(EncounterTable) * gEncounterTablesLength);
    if (encounterTables == NULL) {
        showMesageBox("\nwmConfigInit::Error loading Encounter Table!");
        exit(1);
    }

    gEncounterTables = encounterTables;

    EncounterTable* encounterTable = &(encounterTables[gEncounterTablesLength - 1]);

    // NOTE: Uninline.
    worldmapEncounterTableInit(encounterTable);

    encounterTable->field_28 = gEncounterTablesLength - 1;
    strncpy(encounterTable->lookupName, lookupName, 40);

    char* str;
    if (configGetString(config, sectionKey, "maps", &str)) {
        while (*str != '\0') {
            if (encounterTable->mapsLength >= 6) {
                break;
            }

            if (strParseStrFromFunc(&str, &(encounterTable->maps[encounterTable->mapsLength]), worldmapFindMapByLookupName) == -1) {
                break;
            }

            encounterTable->mapsLength++;
        }
    }

    for (;;) {
        char key[40];
        sprintf(key, "enc_%02d", encounterTable->entriesLength);

        char* str;
        if (!configGetString(config, sectionKey, key, &str)) {
            break;
        }

        if (encounterTable->entriesLength >= 40) {
            showMesageBox("\nwmConfigInit::Error: Encounter Table: Too many table indexes!!");
            exit(1);
        }

        gWorldmapConfig = config;

        if (worldmapConfigLoadEncounterEntry(&(encounterTable->entries[encounterTable->entriesLength]), str) == -1) {
            return -1;
        }

        encounterTable->entriesLength++;
    }

    return 0;
}

// 0x4BDB64
int worldmapConfigLoadEncounterEntry(EncounterEntry* entry, char* string)
{
    // NOTE: Uninline.
    if (worldmapEncounterTableEntryInit(entry) == -1) {
        return -1;
    }

    while (string != NULL && *string != '\0') {
        strParseIntWithKey(&string, "chance", &(entry->chance), ":");
        strParseIntWithKey(&string, "counter", &(entry->counter), ":");

        if (strstr(string, "special")) {
            entry->flags |= ENCOUNTER_ENTRY_SPECIAL;
            string += 8;
        }

        if (string != NULL) {
            char* pch = strstr(string, "map:");
            if (pch != NULL) {
                string = pch + 4;
                strParseStrFromFunc(&string, &(entry->map), worldmapFindMapByLookupName);
            }
        }

        if (_wmParseEncounterSubEncStr(entry, &string) == -1) {
            break;
        }

        if (string != NULL) {
            char* pch = strstr(string, "scenery:");
            if (pch != NULL) {
                string = pch + 8;
                strParseStrFromList(&string, &(entry->scenery), _wmSceneryStrs, ENCOUNTER_SCENERY_TYPE_COUNT);
            }
        }

        worldmapConfigParseCondition(&string, "if", &(entry->condition));
    }

    return 0;
}

// 0x4BDCA8
int _wmParseEncounterSubEncStr(EncounterEntry* encounterEntry, char** stringPtr)
{
    char* string = *stringPtr;
    if (strnicmp(string, "enc:", 4) != 0) {
        return -1;
    }

    // Consume "enc:".
    string += 4;

    char* comma = strstr(string, ",");
    if (comma != NULL) {
        // Comma is present, position string pointer to the next chunk.
        *stringPtr = comma + 1;
        *comma = '\0';
    } else {
        // No comma, this chunk is the last one.
        *stringPtr = NULL;
    }

    while (string != NULL) {
        ENCOUNTER_ENTRY_ENC* entry = &(encounterEntry->field_54[encounterEntry->field_50]);

        // NOTE: Uninline.
        _wmEncounterSubEncSlotInit(entry);

        if (*string == '(') {
            string++;
            entry->minQuantity = atoi(string);

            while (*string != '\0' && *string != '-') {
                string++;
            }

            if (*string == '-') {
                string++;
            }

            entry->maxQuantity = atoi(string);

            while (*string != '\0' && *string != ')') {
                string++;
            }

            if (*string == ')') {
                string++;
            }
        }

        while (*string == ' ') {
            string++;
        }

        char* end = string;
        while (*end != '\0' && *end != ' ') {
            end++;
        }

        char ch = *end;
        *end = '\0';

        if (strParseStrFromFunc(&string, &(entry->field_8), _wmParseFindSubEncTypeMatch) == -1) {
            return -1;
        }

        *end = ch;

        if (ch == ' ') {
            string++;
        }

        end = string;
        while (*end != '\0' && *end != ' ') {
            end++;
        }

        ch = *end;
        *end = '\0';

        if (*string != '\0') {
            strParseStrFromList(&string, &(entry->situation), _wmEncOpStrs, ENCOUNTER_SITUATION_COUNT);
        }

        *end = ch;

        encounterEntry->field_50++;

        while (*string == ' ') {
            string++;
        }

        if (*string == '\0') {
            string = NULL;
        }
    }

    if (comma != NULL) {
        *comma = ',';
    }

    return 0;
}

// 0x4BDE94
int _wmParseFindSubEncTypeMatch(char* str, int* valuePtr)
{
    *valuePtr = 0;

    if (stricmp(str, "player") == 0) {
        *valuePtr = -1;
        return 0;
    }

    if (_wmFindEncBaseTypeMatch(str, valuePtr) == 0) {
        return 0;
    }

    if (_wmReadEncBaseType(str, valuePtr) == 0) {
        return 0;
    }

    return -1;
}

// 0x4BDED8
int _wmFindEncBaseTypeMatch(char* str, int* valuePtr)
{
    for (int index = 0; index < _wmMaxEncBaseTypes; index++) {
        if (stricmp(_wmEncBaseTypeList[index].name, str) == 0) {
            *valuePtr = index;
            return 0;
        }
    }

    *valuePtr = -1;
    return -1;
}

// 0x4BDF34
int _wmReadEncBaseType(char* name, int* valuePtr)
{
    char section[40];
    sprintf(section, "Encounter: %s", name);

    char key[40];
    sprintf(key, "type_00");

    char* string;
    if (!configGetString(gWorldmapConfig, section, key, &string)) {
        return -1;
    }

    _wmMaxEncBaseTypes++;

    ENC_BASE_TYPE* arr = (ENC_BASE_TYPE*)internal_realloc(_wmEncBaseTypeList, sizeof(*_wmEncBaseTypeList) * _wmMaxEncBaseTypes);
    if (arr == NULL) {
        showMesageBox("\nwmConfigInit::Error Reading EncBaseType!");
        exit(1);
    }

    _wmEncBaseTypeList = arr;

    ENC_BASE_TYPE* entry = &(arr[_wmMaxEncBaseTypes - 1]);

    // NOTE: Uninline.
    _wmEncBaseTypeSlotInit(entry);

    strncpy(entry->name, name, 40);

    while (1) {
        if (_wmParseEncBaseSubTypeStr(&(entry->field_38[entry->field_34]), &string) == -1) {
            return -1;
        }

        entry->field_34++;

        sprintf(key, "type_%02d", entry->field_34);

        if (!configGetString(gWorldmapConfig, section, key, &string)) {
            int team;
            configGetInt(gWorldmapConfig, section, "team_num", &team);

            for (int index = 0; index < entry->field_34; index++) {
                ENC_BASE_TYPE_38* ptr = &(entry->field_38[index]);
                if (PID_TYPE(ptr->pid) == OBJ_TYPE_CRITTER) {
                    ptr->team = team;
                }
            }

            if (configGetString(gWorldmapConfig, section, "position", &string)) {
                strParseStrFromList(&string, &(entry->position), gEncounterFormationTypeKeys, ENCOUNTER_FORMATION_TYPE_COUNT);
                strParseIntWithKey(&string, "spacing", &(entry->spacing), ":");
                strParseIntWithKey(&string, "distance", &(entry->distance), ":");
            }

            *valuePtr = _wmMaxEncBaseTypes - 1;

            return 0;
        }
    }

    return -1;
}

// 0x4BE140
int _wmParseEncBaseSubTypeStr(ENC_BASE_TYPE_38* ptr, char** stringPtr)
{
    char* string = *stringPtr;

    // NOTE: Uninline.
    if (_wmEncBaseSubTypeSlotInit(ptr) == -1) {
        return -1;
    }

    if (strParseIntWithKey(&string, "ratio", &(ptr->ratio), ":") == 0) {
        ptr->field_2C = 0;
    }

    if (strstr(string, "dead,") == string) {
        ptr->flags |= ENCOUNTER_SUBINFO_DEAD;
        string += 5;
    }

    strParseIntWithKey(&string, "pid", &(ptr->pid), ":");
    if (ptr->pid == 0) {
        ptr->pid = -1;
    }

    strParseIntWithKey(&string, "distance", &(ptr->distance), ":");
    strParseIntWithKey(&string, "tilenum", &(ptr->tile), ":");

    for (int index = 0; index < 10; index++) {
        if (strstr(string, "item:") == NULL) {
            break;
        }

        _wmParseEncounterItemType(&string, &(ptr->items[ptr->itemsLength]), &(ptr->itemsLength), ":");
    }

    strParseIntWithKey(&string, "script", &(ptr->script), ":");
    worldmapConfigParseCondition(&string, "if", &(ptr->condition));

    return 0;
}

// NOTE: Inlined.
//
// 0x4BE2A0
int _wmEncBaseTypeSlotInit(ENC_BASE_TYPE* entry)
{
    entry->name[0] = '\0';
    entry->position = ENCOUNTER_FORMATION_TYPE_SURROUNDING;
    entry->spacing = 1;
    entry->distance = -1;
    entry->field_34 = 0;

    return 0;
}

// NOTE: Inlined.
//
// 0x4BE2C4
int _wmEncBaseSubTypeSlotInit(ENC_BASE_TYPE_38* entry)
{
    entry->field_28 = -1;
    entry->field_2C = 1;
    entry->ratio = 100;
    entry->pid = -1;
    entry->flags = 0;
    entry->distance = 0;
    entry->tile = -1;
    entry->itemsLength = 0;
    entry->script = -1;
    entry->team = -1;

    return worldmapConfigInitEncounterCondition(&(entry->condition));
}

// NOTE: Inlined.
//
// 0x4BE32C
int _wmEncounterSubEncSlotInit(ENCOUNTER_ENTRY_ENC* entry)
{
    entry->minQuantity = 1;
    entry->maxQuantity = 1;
    entry->field_8 = -1;
    entry->situation = ENCOUNTER_SITUATION_NOTHING;

    return 0;
}

// NOTE: Inlined.
//
// 0x4BE34C
int worldmapEncounterTableEntryInit(EncounterEntry* entry)
{
    entry->flags = 0;
    entry->map = -1;
    entry->scenery = ENCOUNTER_SCENERY_TYPE_NORMAL;
    entry->chance = 0;
    entry->counter = -1;
    entry->field_50 = 0;

    return worldmapConfigInitEncounterCondition(&(entry->condition));
}

// NOTE: Inlined.
//
// 0x4BE3B8
int worldmapEncounterTableInit(EncounterTable* encounterTable)
{
    encounterTable->lookupName[0] = '\0';
    encounterTable->mapsLength = 0;
    encounterTable->field_48 = 0;
    encounterTable->entriesLength = 0;

    return 0;
}

// NOTE: Inlined.
//
// 0x4BE3D4
int worldmapTileInfoInit(TileInfo* tile)
{
    tile->fid = -1;
    tile->handle = INVALID_CACHE_ENTRY;
    tile->data = NULL;
    tile->walkMaskName[0] = '\0';
    tile->walkMaskData = NULL;
    tile->encounterDifficultyModifier = 0;

    return 0;
}

// NOTE: Inlined.
//
// 0x4BE400
int worldmapTerrainInfoInit(Terrain* terrain)
{
    terrain->lookupName[0] = '\0';
    terrain->type = 0;
    terrain->mapsLength = 0;

    return 0;
}

// 0x4BE378
int worldmapConfigInitEncounterCondition(EncounterCondition* condition)
{
    condition->entriesLength = 0;

    for (int index = 0; index < 3; index++) {
        EncounterConditionEntry* conditionEntry = &(condition->entries[index]);
        conditionEntry->type = ENCOUNTER_CONDITION_TYPE_NONE;
        conditionEntry->conditionalOperator = ENCOUNTER_CONDITIONAL_OPERATOR_NONE;
        conditionEntry->param = 0;
        conditionEntry->value = 0;
    }

    for (int index = 0; index < 2; index++) {
        condition->logicalOperators[index] = ENCOUNTER_LOGICAL_OPERATOR_NONE;
    }

    return 0;
}

// 0x4BE414
int _wmParseTerrainTypes(Config* config, char* string)
{
    if (*string == '\0') {
        return -1;
    }

    int terrainCount = 1;

    char* pch = string;
    while (*pch != '\0') {
        if (*pch == ',') {
            terrainCount++;
        }
        pch++;
    }

    gTerrainsLength = terrainCount;

    gTerrains = (Terrain*)internal_malloc(sizeof(*gTerrains) * terrainCount);
    if (gTerrains == NULL) {
        return -1;
    }

    for (int index = 0; index < gTerrainsLength; index++) {
        Terrain* terrain = &(gTerrains[index]);

        // NOTE: Uninline.
        worldmapTerrainInfoInit(terrain);
    }

    strlwr(string);

    pch = string;
    for (int index = 0; index < gTerrainsLength; index++) {
        Terrain* terrain = &(gTerrains[index]);

        pch += strspn(pch, " ");

        int endPos = strcspn(pch, ",");
        char end = pch[endPos];
        pch[endPos] = '\0';

        int delimeterPos = strcspn(pch, ":");
        char delimeter = pch[delimeterPos];
        pch[delimeterPos] = '\0';

        strncpy(terrain->lookupName, pch, 40);
        terrain->type = atoi(pch + delimeterPos + 1);

        pch[delimeterPos] = delimeter;
        pch[endPos] = end;

        if (end == ',') {
            pch += endPos + 1;
        }
    }

    for (int index = 0; index < gTerrainsLength; index++) {
        _wmParseTerrainRndMaps(config, &(gTerrains[index]));
    }

    return 0;
}

// 0x4BE598
int _wmParseTerrainRndMaps(Config* config, Terrain* terrain)
{
    char section[40];
    sprintf(section, "Random Maps: %s", terrain->lookupName);

    for (;;) {
        char key[40];
        sprintf(key, "map_%02d", terrain->mapsLength);

        char* string;
        if (!configGetString(config, section, key, &string)) {
            break;
        }

        if (strParseStrFromFunc(&string, &(terrain->maps[terrain->mapsLength]), worldmapFindMapByLookupName) == -1) {
            return -1;
        }

        terrain->mapsLength++;

        if (terrain->mapsLength >= 20) {
            return -1;
        }
    }

    return 0;
}

// 0x4BE61C
int worldmapConfigLoadSubtile(TileInfo* tile, int row, int column, char* string)
{
    SubtileInfo* subtile = &(tile->subtiles[column][row]);
    subtile->state = SUBTILE_STATE_UNKNOWN;

    if (strParseStrFromFunc(&string, &(subtile->terrain), worldmapFindTerrainByLookupName) == -1) {
        return -1;
    }

    if (strParseStrFromList(&string, &(subtile->field_4), _wmFillStrs, 9) == -1) {
        return -1;
    }

    for (int index = 0; index < DAY_PART_COUNT; index++) {
        if (strParseStrFromList(&string, &(subtile->encounterChance[index]), gEncounterFrequencyTypeKeys, ENCOUNTER_FREQUENCY_TYPE_COUNT) == -1) {
            return -1;
        }
    }

    if (strParseStrFromFunc(&string, &(subtile->encounterType), worldmapFindEncounterTableByLookupName) == -1) {
        return -1;
    }

    return 0;
}

// 0x4BE6D4
int worldmapFindEncounterTableByLookupName(char* string, int* valuePtr)
{
    for (int index = 0; index < gEncounterTablesLength; index++) {
        if (stricmp(string, gEncounterTables[index].lookupName) == 0) {
            *valuePtr = index;
            return 0;
        }
    }

    debugPrint("WorldMap Error: Couldn't find match for Encounter Type!");

    *valuePtr = -1;

    return -1;
}

// 0x4BE73C
int worldmapFindTerrainByLookupName(char* string, int* valuePtr)
{
    for (int index = 0; index < gTerrainsLength; index++) {
        Terrain* terrain = &(gTerrains[index]);
        if (stricmp(string, terrain->lookupName) == 0) {
            *valuePtr = index;
            return 0;
        }
    }

    debugPrint("WorldMap Error: Couldn't find match for Terrain Type!");

    *valuePtr = -1;

    return -1;
}

// 0x4BE7A4
int _wmParseEncounterItemType(char** stringPtr, ENC_BASE_TYPE_38_48* a2, int* a3, const char* delim)
{
    char* string;
    int v2, v3;
    char tmp, tmp2;
    int v20;

    string = *stringPtr;
    v20 = 0;

    if (*string == '\0') {
        return -1;
    }

    strlwr(string);

    if (*string == ',') {
        string++;
        *stringPtr += 1;
    }

    string += strspn(string, " ");

    v2 = strcspn(string, ",");

    tmp = string[v2];
    string[v2] = '\0';

    v3 = strcspn(string, delim);
    tmp2 = string[v3];
    string[v3] = '\0';

    if (strcmp(string, "item") == 0) {
        *stringPtr += v2 + 1;
        v20 = 1;
        _wmParseItemType(string + v3 + 1, a2);
        *a3 = *a3 + 1;
    }

    string[v3] = tmp2;
    string[v2] = tmp;

    return v20 ? 0 : -1;
}

// 0x4BE888
int _wmParseItemType(char* string, ENC_BASE_TYPE_38_48* ptr)
{
    while (*string == ' ') {
        string++;
    }

    ptr->minimumQuantity = 1;
    ptr->maximumQuantity = 1;
    ptr->isEquipped = false;

    if (*string == '(') {
        string++;

        ptr->minimumQuantity = atoi(string);

        while (isdigit(*string)) {
            string++;
        }

        if (*string == '-') {
            string++;

            ptr->maximumQuantity = atoi(string);

            while (isdigit(*string)) {
                string++;
            }
        } else {
            ptr->maximumQuantity = ptr->minimumQuantity;
        }

        if (*string == ')') {
            string++;
        }
    }

    while (*string == ' ') {
        string++;
    }

    ptr->pid = atoi(string);

    while (isdigit(*string)) {
        string++;
    }

    while (*string == ' ') {
        string++;
    }

    if (strstr(string, "{wielded}") != NULL
        || strstr(string, "(wielded)") != NULL
        || strstr(string, "{worn}") != NULL
        || strstr(string, "(worn)") != NULL) {
        ptr->isEquipped = true;
    }

    return 0;
}

// 0x4BE988
int worldmapConfigParseCondition(char** stringPtr, const char* a2, EncounterCondition* condition)
{
    while (condition->entriesLength < 3) {
        EncounterConditionEntry* conditionEntry = &(condition->entries[condition->entriesLength]);
        if (worldmapConfigParseConditionEntry(stringPtr, a2, &(conditionEntry->type), &(conditionEntry->conditionalOperator), &(conditionEntry->param), &(conditionEntry->value)) == -1) {
            return -1;
        }

        condition->entriesLength++;

        char* andStatement = strstr(*stringPtr, "and");
        if (andStatement != NULL) {
            *stringPtr = andStatement + 3;
            condition->logicalOperators[condition->entriesLength - 1] = ENCOUNTER_LOGICAL_OPERATOR_AND;
            continue;
        }

        char* orStatement = strstr(*stringPtr, "or");
        if (orStatement != NULL) {
            *stringPtr = orStatement + 2;
            condition->logicalOperators[condition->entriesLength - 1] = ENCOUNTER_LOGICAL_OPERATOR_OR;
            continue;
        }

        break;
    }

    return 0;
}

// 0x4BEA24
int worldmapConfigParseConditionEntry(char** stringPtr, const char* a2, int* typePtr, int* operatorPtr, int* paramPtr, int* valuePtr)
{
    char* pch;
    int v2;
    int v3;
    char tmp;
    char tmp2;
    int v57;

    char* string = *stringPtr;

    if (string == NULL) {
        return -1;
    }

    if (*string == '\0') {
        return -1;
    }

    strlwr(string);

    if (*string == ',') {
        string++;
        *stringPtr = string;
    }

    string += strspn(string, " ");

    v2 = strcspn(string, ",");

    tmp = *(string + v2);
    *(string + v2) = '\0';

    v3 = strcspn(string, "(");
    tmp2 = *(string + v3);
    *(string + v3) = '\0';

    v57 = 0;
    if (strstr(string, a2) == string) {
        v57 = 1;
    }

    *(string + v3) = tmp2;
    *(string + v2) = tmp;

    if (v57 == 0) {
        return -1;
    }

    string += v3 + 1;

    if (strstr(string, "rand(") == string) {
        string += 5;
        *typePtr = ENCOUNTER_CONDITION_TYPE_RANDOM;
        *operatorPtr = ENCOUNTER_CONDITIONAL_OPERATOR_NONE;
        *paramPtr = atoi(string);

        pch = strstr(string, ")");
        if (pch != NULL) {
            string = pch + 1;
        }

        pch = strstr(string, ")");
        if (pch != NULL) {
            string = pch + 1;
        }

        pch = strstr(string, ",");
        if (pch != NULL) {
            string = pch + 1;
        }

        *stringPtr = string;
        return 0;
    } else if (strstr(string, "global(") == string) {
        string += 7;
        *typePtr = ENCOUNTER_CONDITION_TYPE_GLOBAL;
        *paramPtr = atoi(string);

        pch = strstr(string, ")");
        if (pch != NULL) {
            string = pch + 1;
        }

        while (*string == ' ') {
            string++;
        }

        if (worldmapConfigParseEncounterConditionalOperator(&string, operatorPtr) != -1) {
            *valuePtr = atoi(string);

            pch = strstr(string, ")");
            if (pch != NULL) {
                string = pch + 1;
            }

            pch = strstr(string, ",");
            if (pch != NULL) {
                string = pch + 1;
            }
            *stringPtr = string;
            return 0;
        }
    } else if (strstr(string, "player(level)") == string) {
        string += 13;
        *typePtr = ENCOUNTER_CONDITION_TYPE_PLAYER;

        while (*string == ' ') {
            string++;
        }

        if (worldmapConfigParseEncounterConditionalOperator(&string, operatorPtr) != -1) {
            *valuePtr = atoi(string);

            pch = strstr(string, ")");
            if (pch != NULL) {
                string = pch + 1;
            }

            pch = strstr(string, ",");
            if (pch != NULL) {
                string = pch + 1;
            }
            *stringPtr = string;
            return 0;
        }
    } else if (strstr(string, "days_played") == string) {
        string += 11;
        *typePtr = ENCOUNTER_CONDITION_TYPE_DAYS_PLAYED;

        while (*string == ' ') {
            string++;
        }

        if (worldmapConfigParseEncounterConditionalOperator(&string, operatorPtr) != -1) {
            *valuePtr = atoi(string);

            pch = strstr(string, ")");
            if (pch != NULL) {
                string = pch + 1;
            }

            pch = strstr(string, ",");
            if (pch != NULL) {
                string = pch + 1;
            }
            *stringPtr = string;
            return 0;
        }
    } else if (strstr(string, "time_of_day") == string) {
        string += 11;
        *typePtr = ENCOUNTER_CONDITION_TYPE_TIME_OF_DAY;

        while (*string == ' ') {
            string++;
        }

        if (worldmapConfigParseEncounterConditionalOperator(&string, operatorPtr) != -1) {
            *valuePtr = atoi(string);

            pch = strstr(string, ")");
            if (pch != NULL) {
                string = pch + 1;
            }

            pch = strstr(string, ",");
            if (pch != NULL) {
                string = pch + 1;
            }
            *stringPtr = string;
            return 0;
        }
    } else if (strstr(string, "enctr(num_critters)") == string) {
        string += 19;
        *typePtr = ENCOUNTER_CONDITION_TYPE_NUMBER_OF_CRITTERS;

        while (*string == ' ') {
            string++;
        }

        if (worldmapConfigParseEncounterConditionalOperator(&string, operatorPtr) != -1) {
            *valuePtr = atoi(string);

            pch = strstr(string, ")");
            if (pch != NULL) {
                string = pch + 1;
            }

            pch = strstr(string, ",");
            if (pch != NULL) {
                string = pch + 1;
            }
            *stringPtr = string;
            return 0;
        }
    } else {
        *stringPtr = string;
        return 0;
    }

    return -1;
}

// 0x4BEEBC
int worldmapConfigParseEncounterConditionalOperator(char** stringPtr, int* conditionalOperatorPtr)
{
    char* string = *stringPtr;

    *conditionalOperatorPtr = ENCOUNTER_CONDITIONAL_OPERATOR_NONE;

    int index;
    for (index = 0; index < ENCOUNTER_CONDITIONAL_OPERATOR_COUNT; index++) {
        if (strstr(string, _wmConditionalOpStrs[index]) == string) {
            break;
        }
    }

    if (index == ENCOUNTER_CONDITIONAL_OPERATOR_COUNT) {
        return -1;
    }

    *conditionalOperatorPtr = index;

    string += strlen(_wmConditionalOpStrs[index]);
    while (*string == ' ') {
        string++;
    }

    *stringPtr = string;

    return 0;
}

// NOTE: Inlined.
//
// 0x4BEF1C
int worldmapCityInfoInit(CityInfo* area)
{
    area->name[0] = '\0';
    area->areaId = -1;
    area->x = 0;
    area->y = 0;
    area->size = CITY_SIZE_LARGE;
    area->state = CITY_STATE_UNKNOWN;
    area->lockState = LOCK_STATE_UNLOCKED;
    area->visitedState = 0;
    area->mapFid = -1;
    area->labelFid = -1;
    area->entrancesLength = 0;

    return 0;
}

// 0x4BEF68
int cityInit()
{
    Config cfg;
    char section[40];
    char key[40];
    int area_idx;
    int num;
    char* str;
    CityInfo* cities;
    CityInfo* city;
    EntranceInfo* entrance;

    if (_wmMapInit() == -1) {
        return -1;
    }

    if (!configInit(&cfg)) {
        return -1;
    }

    if (configRead(&cfg, "data\\city.txt", true)) {
        area_idx = 0;
        do {
            sprintf(section, "Area %02d", area_idx);
            if (!configGetInt(&cfg, section, "townmap_art_idx", &num)) {
                break;
            }

            gCitiesLength++;

            cities = (CityInfo*)internal_realloc(gCities, sizeof(CityInfo) * gCitiesLength);
            if (cities == NULL) {
                showMesageBox("\nwmConfigInit::Error loading areas!");
                exit(1);
            }

            gCities = cities;

            city = &(cities[gCitiesLength - 1]);

            // NOTE: Uninline.
            worldmapCityInfoInit(city);

            city->areaId = area_idx;

            if (num != -1) {
                num = buildFid(OBJ_TYPE_INTERFACE, num, 0, 0, 0);
            }

            city->mapFid = num;

            if (configGetInt(&cfg, section, "townmap_label_art_idx", &num)) {
                if (num != -1) {
                    num = buildFid(OBJ_TYPE_INTERFACE, num, 0, 0, 0);
                }

                city->labelFid = num;
            }

            if (!configGetString(&cfg, section, "area_name", &str)) {
                showMesageBox("\nwmConfigInit::Error loading areas!");
                exit(1);
            }

            strncpy(city->name, str, 40);

            if (!configGetString(&cfg, section, "world_pos", &str)) {
                showMesageBox("\nwmConfigInit::Error loading areas!");
                exit(1);
            }

            if (strParseInt(&str, &(city->x)) == -1) {
                return -1;
            }

            if (strParseInt(&str, &(city->y)) == -1) {
                return -1;
            }

            if (!configGetString(&cfg, section, "start_state", &str)) {
                showMesageBox("\nwmConfigInit::Error loading areas!");
                exit(1);
            }

            if (strParseStrFromList(&str, &(city->state), _wmStateStrs, 2) == -1) {
                return -1;
            }

            if (configGetString(&cfg, section, "lock_state", &str)) {
                if (strParseStrFromList(&str, &(city->lockState), _wmStateStrs, 2) == -1) {
                    return -1;
                }
            }

            if (!configGetString(&cfg, section, "size", &str)) {
                showMesageBox("\nwmConfigInit::Error loading areas!");
                exit(1);
            }

            if (strParseStrFromList(&str, &(city->size), gCitySizeKeys, 3) == -1) {
                return -1;
            }

            while (city->entrancesLength < ENTRANCE_LIST_CAPACITY) {
                sprintf(key, "entrance_%d", city->entrancesLength);

                if (!configGetString(&cfg, section, key, &str)) {
                    break;
                }

                entrance = &(city->entrances[city->entrancesLength]);

                // NOTE: Uninline.
                worldmapCityEntranceInfoInit(entrance);

                if (strParseStrFromList(&str, &(entrance->state), _wmStateStrs, 2) == -1) {
                    return -1;
                }

                if (strParseInt(&str, &(entrance->x)) == -1) {
                    return -1;
                }

                if (strParseInt(&str, &(entrance->y)) == -1) {
                    return -1;
                }

                if (strParseStrFromFunc(&str, &(entrance->map), &worldmapFindMapByLookupName) == -1) {
                    return -1;
                }

                if (strParseInt(&str, &(entrance->elevation)) == -1) {
                    return -1;
                }

                if (strParseInt(&str, &(entrance->tile)) == -1) {
                    return -1;
                }

                if (strParseInt(&str, &(entrance->rotation)) == -1) {
                    return -1;
                }

                city->entrancesLength++;
            }

            area_idx++;
        } while (area_idx < 5000);
    }

    configFree(&cfg);

    if (gCitiesLength != CITY_COUNT) {
        showMesageBox("\nwmAreaInit::Error loading Cities!");
        exit(1);
    }

    return 0;
}

// 0x4BF3E0
int worldmapFindMapByLookupName(char* string, int* valuePtr)
{
    for (int index = 0; index < gMapsLength; index++) {
        MapInfo* map = &(gMaps[index]);
        if (stricmp(string, map->lookupName) == 0) {
            *valuePtr = index;
            return 0;
        }
    }

    debugPrint("\nWorldMap Error: Couldn't find match for Map Index!");

    *valuePtr = -1;
    return -1;
}

// NOTE: Inlined.
//
// 0x4BF448
int worldmapCityEntranceInfoInit(EntranceInfo* entrance)
{
    entrance->state = 0;
    entrance->x = 0;
    entrance->y = 0;
    entrance->map = -1;
    entrance->elevation = 0;
    entrance->tile = 0;
    entrance->rotation = 0;

    return 0;
}

// 0x4BF47C
int worldmapMapInfoInit(MapInfo* map)
{
    map->lookupName[0] = '\0';
    map->field_28 = -1;
    map->field_2C = -1;
    map->mapFileName[0] = '\0';
    map->music[0] = '\0';
    map->flags = 0x3F;
    map->ambientSoundEffectsLength = 0;
    map->startPointsLength = 0;

    return 0;
}

// 0x4BF4BC
int _wmMapInit()
{
    char* str;
    int num;
    MapInfo* maps;
    MapInfo* map;
    int j;
    MapAmbientSoundEffectInfo* sfx;
    MapStartPointInfo* rsp;

    Config config;
    if (!configInit(&config)) {
        return -1;
    }

    if (configRead(&config, "data\\maps.txt", true)) {
        for (int mapIndex = 0;; mapIndex++) {
            char section[40];
            sprintf(section, "Map %03d", mapIndex);

            if (!configGetString(&config, section, "lookup_name", &str)) {
                break;
            }

            gMapsLength++;

            maps = (MapInfo*)internal_realloc(gMaps, sizeof(*gMaps) * gMapsLength);
            if (maps == NULL) {
                showMesageBox("\nwmConfigInit::Error loading maps!");
                exit(1);
            }

            gMaps = maps;

            map = &(maps[gMapsLength - 1]);
            worldmapMapInfoInit(map);

            strncpy(map->lookupName, str, 40);

            if (!configGetString(&config, section, "map_name", &str)) {
                showMesageBox("\nwmConfigInit::Error loading maps!");
                exit(1);
            }

            strlwr(str);
            strncpy(map->mapFileName, str, 40);

            if (configGetString(&config, section, "music", &str)) {
                strncpy(map->music, str, 40);
            }

            if (configGetString(&config, section, "ambient_sfx", &str)) {
                while (str) {
                    sfx = &(map->ambientSoundEffects[map->ambientSoundEffectsLength]);
                    if (strParseKeyValue(&str, sfx->name, &(sfx->chance), ":") == -1) {
                        return -1;
                    }

                    map->ambientSoundEffectsLength++;

                    if (*str == '\0') {
                        str = NULL;
                    }

                    if (map->ambientSoundEffectsLength >= MAP_AMBIENT_SOUND_EFFECTS_CAPACITY) {
                        if (str != NULL) {
                            debugPrint("\nwmMapInit::Error reading ambient sfx.  Too many!  Str: %s, MapIdx: %d", map->lookupName, mapIndex);
                            str = NULL;
                        }
                    }
                }
            }

            if (configGetString(&config, section, "saved", &str)) {
                if (strParseStrFromList(&str, &num, _wmYesNoStrs, 2) == -1) {
                    return -1;
                }

                // NOTE: Uninline.
                wmSetFlags(&(map->flags), MAP_SAVED, num);
            }

            if (configGetString(&config, section, "dead_bodies_age", &str)) {
                if (strParseStrFromList(&str, &num, _wmYesNoStrs, 2) == -1) {
                    return -1;
                }

                // NOTE: Uninline.
                wmSetFlags(&(map->flags), MAP_DEAD_BODIES_AGE, num);
            }

            if (configGetString(&config, section, "can_rest_here", &str)) {
                if (strParseStrFromList(&str, &num, _wmYesNoStrs, 2) == -1) {
                    return -1;
                }

                // NOTE: Uninline.
                wmSetFlags(&(map->flags), MAP_CAN_REST_ELEVATION_0, num);

                if (strParseStrFromList(&str, &num, _wmYesNoStrs, 2) == -1) {
                    return -1;
                }

                // NOTE: Uninline.
                wmSetFlags(&(map->flags), MAP_CAN_REST_ELEVATION_1, num);

                if (strParseStrFromList(&str, &num, _wmYesNoStrs, 2) == -1) {
                    return -1;
                }

                // NOTE: Uninline.
                wmSetFlags(&(map->flags), MAP_CAN_REST_ELEVATION_2, num);
            }

            if (configGetString(&config, section, "pipbody_active", &str)) {
                if (strParseStrFromList(&str, &num, _wmYesNoStrs, 2) == -1) {
                    return -1;
                }

                // NOTE: Uninline.
                wmSetFlags(&(map->flags), MAP_PIPBOY_ACTIVE, num);
            }

            if (configGetString(&config, section, "random_start_point_0", &str)) {
                j = 0;
                while (str != NULL) {
                    while (*str != '\0') {
                        if (map->startPointsLength >= MAP_STARTING_POINTS_CAPACITY) {
                            break;
                        }

                        rsp = &(map->startPoints[map->startPointsLength]);

                        // NOTE: Uninline.
                        worldmapRandomStartingPointInit(rsp);

                        strParseIntWithKey(&str, "elev", &(rsp->elevation), ":");
                        strParseIntWithKey(&str, "tile_num", &(rsp->tile), ":");

                        map->startPointsLength++;
                    }

                    char key[40];
                    sprintf(key, "random_start_point_%1d", ++j);

                    if (!configGetString(&config, section, key, &str)) {
                        str = NULL;
                    }
                }
            }
        }
    }

    configFree(&config);

    return 0;
}

// NOTE: Inlined.
//
// 0x4BF954
int worldmapRandomStartingPointInit(MapStartPointInfo* rsp)
{
    rsp->elevation = 0;
    rsp->tile = -1;
    rsp->field_8 = -1;

    return 0;
}

// 0x4BF96C
int mapGetCount()
{
    return gMapsLength;
}

// 0x4BF974
int mapGetFileName(int map, char* dest)
{
    if (map == -1 || map > gMapsLength) {
        dest[0] = '\0';
        return -1;
    }

    sprintf(dest, "%s.MAP", gMaps[map].mapFileName);
    return 0;
}

// 0x4BF9BC
int mapGetIndexByFileName(char* name)
{
    strlwr(name);

    char* pch = name;
    while (*pch != '\0' && *pch != '.') {
        pch++;
    }

    bool truncated = false;
    if (*pch != '\0') {
        *pch = '\0';
        truncated = true;
    }

    int map = -1;

    for (int index = 0; index < gMapsLength; index++) {
        if (strcmp(gMaps[index].mapFileName, name) == 0) {
            map = index;
            break;
        }
    }

    if (truncated) {
        *pch = '.';
    }

    return map;
}

// 0x4BFA44
bool _wmMapIdxIsSaveable(int map_index)
{
    return (gMaps[map_index].flags & MAP_SAVED) != 0;
}

// 0x4BFA64
bool _wmMapIsSaveable()
{
    return (gMaps[gMapHeader.field_34].flags & MAP_SAVED) != 0;
}

// 0x4BFA90
bool _wmMapDeadBodiesAge()
{
    return (gMaps[gMapHeader.field_34].flags & MAP_DEAD_BODIES_AGE) != 0;
}

// 0x4BFABC
bool _wmMapCanRestHere(int elevation)
{
    int flags[3];

    // NOTE: I'm not sure why they're copied.
    static_assert(sizeof(flags) == sizeof(_can_rest_here), "wrong size");
    memcpy(flags, _can_rest_here, sizeof(flags));

    MapInfo* map = &(gMaps[gMapHeader.field_34]);

    return (map->flags & flags[elevation]) != 0;
}

// 0x4BFAFC
bool _wmMapPipboyActive()
{
    return gameMovieIsSeen(MOVIE_VSUIT);
}

// 0x4BFB08
int _wmMapMarkVisited(int mapIndex)
{
    if (mapIndex < 0 || mapIndex >= gMapsLength) {
        return -1;
    }

    MapInfo* map = &(gMaps[mapIndex]);
    if ((map->flags & MAP_SAVED) == 0) {
        return 0;
    }

    int cityIndex;
    if (_wmMatchAreaContainingMapIdx(mapIndex, &cityIndex) == -1) {
        return -1;
    }

    // NOTE: Uninline.
    wmAreaMarkVisited(cityIndex);

    return 0;
}

// 0x4BFB64
int _wmMatchEntranceFromMap(int cityIndex, int mapIndex, int* entranceIndexPtr)
{
    CityInfo* city = &(gCities[cityIndex]);

    for (int entranceIndex = 0; entranceIndex < city->entrancesLength; entranceIndex++) {
        EntranceInfo* entrance = &(city->entrances[entranceIndex]);

        if (mapIndex == entrance->map) {
            *entranceIndexPtr = entranceIndex;
            return 0;
        }
    }

    *entranceIndexPtr = -1;
    return -1;
}

// 0x4BFBE8
int _wmMatchEntranceElevFromMap(int cityIndex, int map, int elevation, int* entranceIndexPtr)
{
    CityInfo* city = &(gCities[cityIndex]);

    for (int entranceIndex = 0; entranceIndex < city->entrancesLength; entranceIndex++) {
        EntranceInfo* entrance = &(city->entrances[entranceIndex]);
        if (entrance->map == map) {
            if (elevation == -1 || entrance->elevation == -1 || elevation == entrance->elevation) {
                *entranceIndexPtr = entranceIndex;
                return 0;
            }
        }
    }

    *entranceIndexPtr = -1;
    return -1;
}

// 0x4BFC7C
int _wmMatchAreaFromMap(int mapIndex, int* cityIndexPtr)
{
    for (int cityIndex = 0; cityIndex < gCitiesLength; cityIndex++) {
        CityInfo* city = &(gCities[cityIndex]);

        for (int entranceIndex = 0; entranceIndex < city->entrancesLength; entranceIndex++) {
            EntranceInfo* entrance = &(city->entrances[entranceIndex]);
            if (mapIndex == entrance->map) {
                *cityIndexPtr = cityIndex;
                return 0;
            }
        }
    }

    *cityIndexPtr = -1;
    return -1;
}

// Mark map entrance.
//
// 0x4BFD50
int _wmMapMarkMapEntranceState(int mapIndex, int elevation, int state)
{
    if (mapIndex < 0 || mapIndex >= gMapsLength) {
        return -1;
    }

    MapInfo* map = &(gMaps[mapIndex]);
    if ((map->flags & MAP_SAVED) == 0) {
        return -1;
    }

    int cityIndex;
    if (_wmMatchAreaContainingMapIdx(mapIndex, &cityIndex) == -1) {
        return -1;
    }

    int entranceIndex;
    if (_wmMatchEntranceElevFromMap(cityIndex, mapIndex, elevation, &entranceIndex) == -1) {
        return -1;
    }

    CityInfo* city = &(gCities[cityIndex]);
    EntranceInfo* entrance = &(city->entrances[entranceIndex]);
    entrance->state = state;

    return 0;
}

// 0x4BFE0C
void _wmWorldMap()
{
    _wmWorldMapFunc(0);
}

// 0x4BFE10
int _wmWorldMapFunc(int a1)
{
    if (worldmapWindowInit() == -1) {
        worldmapWindowFree();
        return -1;
    }

    _wmMatchWorldPosToArea(wmGenData.worldPosX, wmGenData.worldPosY, &(wmGenData.currentAreaId));

    unsigned int v24 = 0;
    int map = -1;
    int v25 = 0;

    int rc = 0;
    for (;;) {
        int keyCode = _get_input();
        unsigned int tick = _get_time();

        int mouseX;
        int mouseY;
        mouseGetPosition(&mouseX, &mouseY);

        int v4 = gWorldmapOffsetX + mouseX - WM_VIEW_X;
        int v5 = gWorldmapOffsetY + mouseY - WM_VIEW_Y;

        if (keyCode == KEY_CTRL_Q || keyCode == KEY_CTRL_X || keyCode == KEY_F10) {
            showQuitConfirmationDialog();
        }

        // NOTE: Uninline.
        wmCheckGameEvents();

        if (_game_user_wants_to_quit != 0) {
            break;
        }

        int mouseEvent = mouseGetEvent();

        if (wmGenData.isWalking) {
            worldmapPerformTravel();

            if (wmGenData.isInCar) {
                worldmapPerformTravel();
                worldmapPerformTravel();
                worldmapPerformTravel();

                if (gameGetGlobalVar(GVAR_CAR_BLOWER)) {
                    worldmapPerformTravel();
                }

                if (gameGetGlobalVar(GVAR_NEW_RENO_CAR_UPGRADE)) {
                    worldmapPerformTravel();
                }

                if (gameGetGlobalVar(GVAR_NEW_RENO_SUPER_CAR)) {
                    worldmapPerformTravel();
                    worldmapPerformTravel();
                    worldmapPerformTravel();
                }

                wmGenData.carImageCurrentFrameIndex++;
                if (wmGenData.carImageCurrentFrameIndex >= artGetFrameCount(wmGenData.carImageFrm)) {
                    wmGenData.carImageCurrentFrameIndex = 0;
                }

                carConsumeFuel(100);

                if (wmGenData.carFuel <= 0) {
                    wmGenData.walkDestinationX = 0;
                    wmGenData.walkDestinationY = 0;
                    wmGenData.isWalking = false;

                    _wmMatchWorldPosToArea(v4, v5, &(wmGenData.currentAreaId));

                    wmGenData.isInCar = false;

                    if (wmGenData.currentAreaId == -1) {
                        wmGenData.currentCarAreaId = CITY_CAR_OUT_OF_GAS;

                        CityInfo* city = &(gCities[CITY_CAR_OUT_OF_GAS]);

                        CitySizeDescription* citySizeDescription = &(gCitySizeDescriptions[city->size]);
                        int worldmapX = wmGenData.worldPosX + wmGenData.hotspotFrmWidth / 2 + citySizeDescription->width / 2;
                        int worldmapY = wmGenData.worldPosY + wmGenData.hotspotFrmHeight / 2 + citySizeDescription->height / 2;
                        worldmapCitySetPos(CITY_CAR_OUT_OF_GAS, worldmapX, worldmapY);

                        city->state = CITY_STATE_KNOWN;
                        city->visitedState = 1;

                        wmGenData.currentAreaId = CITY_CAR_OUT_OF_GAS;
                    } else {
                        wmGenData.currentCarAreaId = wmGenData.currentAreaId;
                    }

                    debugPrint("\nRan outta gas!");
                }
            }

            worldmapWindowRefresh();

            if (getTicksBetween(tick, v24) > 1000) {
                if (_partyMemberRestingHeal(3)) {
                    interfaceRenderHitPoints(false);
                    v24 = tick;
                }
            }

            _wmMarkSubTileRadiusVisited(wmGenData.worldPosX, wmGenData.worldPosY);

            if (wmGenData.walkDistance <= 0) {
                wmGenData.isWalking = false;
                _wmMatchWorldPosToArea(wmGenData.worldPosX, wmGenData.worldPosY, &(wmGenData.currentAreaId));
            }

            worldmapWindowRefresh();

            if (_wmGameTimeIncrement(18000)) {
                if (_game_user_wants_to_quit != 0) {
                    break;
                }
            }

            if (wmGenData.isWalking) {
                if (_wmRndEncounterOccurred()) {
                    if (wmGenData.encounterMapId != -1) {
                        if (wmGenData.isInCar) {
                            _wmMatchAreaContainingMapIdx(wmGenData.encounterMapId, &(wmGenData.currentCarAreaId));
                        }
                        mapLoadById(wmGenData.encounterMapId);
                    }
                    break;
                }
            }
        }

        if ((mouseEvent & MOUSE_EVENT_LEFT_BUTTON_DOWN) != 0 && (mouseEvent & MOUSE_EVENT_LEFT_BUTTON_REPEAT) == 0) {
            if (_mouse_click_in(WM_VIEW_X, WM_VIEW_Y, 472, 465)) {
                if (!wmGenData.isWalking && !wmGenData.mousePressed && abs(wmGenData.worldPosX - v4) < 5 && abs(wmGenData.worldPosY - v5) < 5) {
                    wmGenData.mousePressed = true;
                    worldmapWindowRefresh();
                }
            } else {
                continue;
            }
        }

        if ((mouseEvent & MOUSE_EVENT_LEFT_BUTTON_UP) != 0) {
            if (wmGenData.mousePressed) {
                wmGenData.mousePressed = false;
                worldmapWindowRefresh();

                if (abs(wmGenData.worldPosX - v4) < 5 && abs(wmGenData.worldPosY - v5) < 5) {
                    if (wmGenData.currentAreaId != -1) {
                        CityInfo* city = &(gCities[wmGenData.currentAreaId]);
                        if (city->visitedState == 2 && city->mapFid != -1) {
                            if (worldmapCityMapViewSelect(&map) == -1) {
                                v25 = -1;
                                break;
                            }
                        } else {
                            if (_wmAreaFindFirstValidMap(&map) == -1) {
                                v25 = -1;
                                break;
                            }

                            city->visitedState = 2;
                        }
                    } else {
                        map = 0;
                    }

                    if (map != -1) {
                        if (wmGenData.isInCar) {
                            wmGenData.isInCar = false;
                            if (wmGenData.currentAreaId == -1) {
                                _wmMatchAreaContainingMapIdx(map, &(wmGenData.currentCarAreaId));
                            } else {
                                wmGenData.currentCarAreaId = wmGenData.currentAreaId;
                            }
                        }
                        mapLoadById(map);
                        break;
                    }
                }
            } else {
                if (_mouse_click_in(WM_VIEW_X, WM_VIEW_Y, 472, 465)) {
                    _wmPartyInitWalking(v4, v5);
                }

                wmGenData.mousePressed = false;
            }
        }

        // NOTE: Uninline.
        wmInterfaceScrollTabsUpdate();

        if (keyCode == KEY_UPPERCASE_T || keyCode == KEY_LOWERCASE_T) {
            if (!wmGenData.isWalking && wmGenData.currentAreaId != -1) {
                CityInfo* city = &(gCities[wmGenData.currentAreaId]);
                if (city->visitedState == 2 && city->mapFid != -1) {
                    if (worldmapCityMapViewSelect(&map) == -1) {
                        rc = -1;
                    }

                    if (map != -1) {
                        if (wmGenData.isInCar) {
                            _wmMatchAreaContainingMapIdx(map, &(wmGenData.currentCarAreaId));
                        }

                        mapLoadById(map);
                    }
                }
            }
        } else if (keyCode == KEY_HOME) {
            _wmInterfaceCenterOnParty();
        } else if (keyCode == KEY_ARROW_UP) {
            // NOTE: Uninline.
            wmInterfaceScroll(0, -1, NULL);
        } else if (keyCode == KEY_ARROW_LEFT) {
            // NOTE: Uninline.
            wmInterfaceScroll(-1, 0, NULL);
        } else if (keyCode == KEY_ARROW_DOWN) {
            // NOTE: Uninline.
            wmInterfaceScroll(0, 1, NULL);
        } else if (keyCode == KEY_ARROW_RIGHT) {
            // NOTE: Uninline.
            wmInterfaceScroll(1, 0, NULL);
        } else if (keyCode == KEY_CTRL_ARROW_UP) {
            _wmInterfaceScrollTabsStart(-27);
        } else if (keyCode == KEY_CTRL_ARROW_DOWN) {
            _wmInterfaceScrollTabsStart(27);
        } else if (keyCode >= KEY_CTRL_F1 && keyCode <= KEY_CTRL_F7) {
            int quickDestinationIndex = wmGenData.tabsOffsetY / 27 + (keyCode - KEY_CTRL_F1);
            if (quickDestinationIndex < gQuickDestinationsLength) {
                int cityIndex = gQuickDestinations[quickDestinationIndex];
                CityInfo* city = &(gCities[cityIndex]);
                if (_wmAreaIsKnown(city->areaId)) {
                    if (wmGenData.currentAreaId != cityIndex) {
                        _wmPartyInitWalking(city->x, city->y);
                        wmGenData.mousePressed = false;
                    }
                }
            }
        }

        if (map != -1 || v25 == -1) {
            break;
        }
    }

    if (worldmapWindowFree() == -1) {
        return -1;
    }

    return rc;
}

// 0x4C056C
int _wmCheckGameAreaEvents()
{
    if (wmGenData.currentAreaId == CITY_FAKE_VAULT_13_A) {
        if (wmGenData.currentAreaId < gCitiesLength) {
            gCities[CITY_FAKE_VAULT_13_A].state = CITY_STATE_UNKNOWN;
        }

        if (gCitiesLength > CITY_FAKE_VAULT_13_B) {
            gCities[CITY_FAKE_VAULT_13_B].state = CITY_STATE_KNOWN;
        }

        _wmAreaMarkVisitedState(CITY_FAKE_VAULT_13_B, 2);
    }

    return 0;
}

// 0x4C05C4
int _wmInterfaceCenterOnParty()
{
    int v0;
    int v1;

    v0 = wmGenData.worldPosX - 203;
    if ((v0 & 0x80000000) == 0) {
        if (v0 > wmGenData.viewportMaxX) {
            v0 = wmGenData.viewportMaxX;
        }
    } else {
        v0 = 0;
    }

    v1 = wmGenData.worldPosY - 200;
    if ((v1 & 0x80000000) == 0) {
        if (v1 > wmGenData.viewportMaxY) {
            v1 = wmGenData.viewportMaxY;
        }
    } else {
        v1 = 0;
    }

    gWorldmapOffsetX = v0;
    gWorldmapOffsetY = v1;

    worldmapWindowRefresh();

    return 0;
}

// NOTE: Inlined.
//
// 0x4C0624
void wmCheckGameEvents()
{
    _scriptsCheckGameEvents(NULL, gWorldmapWindow);
}

// 0x4C0634
int _wmRndEncounterOccurred()
{
    unsigned int v0 = _get_time();
    if (getTicksBetween(v0, _wmLastRndTime) < 1500) {
        return 0;
    }

    _wmLastRndTime = v0;

    if (abs(wmGenData.oldWorldPosX - wmGenData.worldPosX) < 3) {
        return 0;
    }

    if (abs(wmGenData.oldWorldPosY - wmGenData.worldPosY) < 3) {
        return 0;
    }

    int v26;
    _wmMatchWorldPosToArea(wmGenData.worldPosX, wmGenData.worldPosY, &v26);
    if (v26 != -1) {
        return 0;
    }

    if (!wmGenData.didMeetFrankHorrigan) {
        unsigned int gameTime = gameTimeGetTime();
        if (gameTime / GAME_TIME_TICKS_PER_DAY > 35) {
            wmGenData.encounterMapId = v26;
            wmGenData.didMeetFrankHorrigan = true;
            if (wmGenData.isInCar) {
                _wmMatchAreaContainingMapIdx(MAP_IN_GAME_MOVIE1, &(wmGenData.currentCarAreaId));
            }
            mapLoadById(MAP_IN_GAME_MOVIE1);
            return 1;
        }
    }

    // NOTE: Uninline.
    _wmPartyFindCurSubTile();

    int dayPart;
    int gameTimeHour = gameTimeGetHour();
    if (gameTimeHour >= 1800 || gameTimeHour < 600) {
        dayPart = DAY_PART_NIGHT;
    } else if (gameTimeHour >= 1200) {
        dayPart = DAY_PART_AFTERNOON;
    } else {
        dayPart = DAY_PART_MORNING;
    }

    int frequency = _wmFreqValues[wmGenData.currentSubtile->encounterChance[dayPart]];
    if (frequency > 0 && frequency < 100) {
        int gameDifficulty = GAME_DIFFICULTY_NORMAL;
        if (configGetInt(&gGameConfig, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_GAME_DIFFICULTY_KEY, &gameDifficulty)) {
            int modifier = frequency / 15;
            switch (gameDifficulty) {
            case GAME_DIFFICULTY_EASY:
                frequency -= modifier;
                break;
            case GAME_DIFFICULTY_HARD:
                frequency += modifier;
                break;
            }
        }
    }

    int chance = randomBetween(0, 100);
    if (chance >= frequency) {
        return 0;
    }

    _wmRndEncounterPick();

    int v8 = 1;
    wmGenData.encounterIconIsVisible = 1;
    wmGenData.encounterCursorId = 0;

    EncounterTable* encounterTable = &(gEncounterTables[wmGenData.encounterTableId]);
    EncounterEntry* encounter = &(encounterTable->entries[wmGenData.encounterEntryId]);
    if ((encounter->flags & ENCOUNTER_ENTRY_SPECIAL) != 0) {
        wmGenData.encounterCursorId = 2;
        _wmMatchAreaContainingMapIdx(wmGenData.encounterMapId, &v26);

        CityInfo* city = &(gCities[v26]);
        CitySizeDescription* citySizeDescription = &(gCitySizeDescriptions[city->size]);
        int worldmapX = wmGenData.worldPosX + wmGenData.hotspotFrmWidth / 2 + citySizeDescription->width / 2;
        int worldmapY = wmGenData.worldPosY + wmGenData.hotspotFrmHeight / 2 + citySizeDescription->height / 2;
        worldmapCitySetPos(v26, worldmapX, worldmapY);
        v8 = 3;

        if (v26 >= 0 && v26 < gCitiesLength) {
            CityInfo* city = &(gCities[v26]);
            if (city->lockState != LOCK_STATE_LOCKED) {
                city->state = CITY_STATE_KNOWN;
            }
        }
    }

    // Blinking.
    for (int index = 0; index < 7; index++) {
        wmGenData.encounterCursorId = v8 - wmGenData.encounterCursorId;

        if (worldmapWindowRefresh() == -1) {
            return -1;
        }

        coreDelay(200);
    }

    if (wmGenData.isInCar) {
        int modifiers[DAY_PART_COUNT];

        // NOTE: I'm not sure why they're copied.
        static_assert(sizeof(modifiers) == sizeof(gDayPartEncounterFrequencyModifiers), "wrong size");
        memcpy(modifiers, gDayPartEncounterFrequencyModifiers, sizeof(gDayPartEncounterFrequencyModifiers));

        frequency -= modifiers[dayPart];
    }

    bool randomEncounterIsDetected = false;
    if (frequency > chance) {
        int outdoorsman = partyGetBestSkillValue(SKILL_OUTDOORSMAN);
        Object* scanner = objectGetCarriedObjectByPid(gDude, PROTO_ID_MOTION_SENSOR);
        if (scanner != NULL) {
            if (gDude == scanner->owner) {
                outdoorsman += 20;
            }
        }

        if (outdoorsman > 95) {
            outdoorsman = 95;
        }

        TileInfo* tile;
        // NOTE: Uninline.
        _wmFindCurTileFromPos(wmGenData.worldPosX, wmGenData.worldPosY, &tile);
        debugPrint("\nEncounter Difficulty Mod: %d", tile->encounterDifficultyModifier);

        outdoorsman += tile->encounterDifficultyModifier;

        if (randomBetween(1, 100) < outdoorsman) {
            randomEncounterIsDetected = true;

            int xp = 100 - outdoorsman;
            if (xp > 0) {
                MessageListItem messageListItem;
                char* text = getmsg(&gMiscMessageList, &messageListItem, 8500);
                if (strlen(text) < 110) {
                    char formattedText[120];
                    sprintf(formattedText, text, xp);
                    displayMonitorAddMessage(formattedText);
                } else {
                    debugPrint("WorldMap: Error: Rnd Encounter string too long!");
                }

                debugPrint("WorldMap: Giving Player [%d] Experience For Catching Rnd Encounter!", xp);

                if (xp < 100) {
                    pcAddExperience(xp);
                }
            }
        }
    } else {
        randomEncounterIsDetected = true;
    }

    wmGenData.oldWorldPosX = wmGenData.worldPosX;
    wmGenData.oldWorldPosY = wmGenData.worldPosY;

    if (randomEncounterIsDetected) {
        MessageListItem messageListItem;

        const char* title = gWorldmapEncDefaultMsg[0];
        const char* body = gWorldmapEncDefaultMsg[1];

        title = getmsg(&gWorldmapMessageList, &messageListItem, 2999);
        body = getmsg(&gWorldmapMessageList, &messageListItem, 3000 + 50 * wmGenData.encounterTableId + wmGenData.encounterEntryId);
        if (showDialogBox(title, &body, 1, 169, 116, _colorTable[32328], NULL, _colorTable[32328], DIALOG_BOX_LARGE | DIALOG_BOX_YES_NO) == 0) {
            wmGenData.encounterIconIsVisible = 0;
            wmGenData.encounterMapId = -1;
            wmGenData.encounterTableId = -1;
            wmGenData.encounterEntryId = -1;
            return 0;
        }
    }

    return 1;
}

// NOTE: Inlined.
//
// 0x4C0BE4
int _wmPartyFindCurSubTile()
{
    return _wmFindCurSubTileFromPos(wmGenData.worldPosX, wmGenData.worldPosY, &(wmGenData.currentSubtile));
}

// 0x4C0C00
int _wmFindCurSubTileFromPos(int x, int y, SubtileInfo** subtile)
{
    int tileIndex = y / WM_TILE_HEIGHT * gWorldmapGridWidth + x / WM_TILE_WIDTH % gWorldmapGridWidth;
    TileInfo* tile = &(gWorldmapTiles[tileIndex]);

    int column = y % WM_TILE_HEIGHT / WM_SUBTILE_SIZE;
    int row = x % WM_TILE_WIDTH / WM_SUBTILE_SIZE;
    *subtile = &(tile->subtiles[column][row]);

    return 0;
}

// NOTE: Inlined.
//
// 0x4C0CA8
int _wmFindCurTileFromPos(int x, int y, TileInfo** tile)
{
    int tileIndex = y / WM_TILE_HEIGHT * gWorldmapGridWidth + x / WM_TILE_WIDTH % gWorldmapGridWidth;
    *tile = &(gWorldmapTiles[tileIndex]);

    return 0;
}

// 0x4C0CF4
int _wmRndEncounterPick()
{
    if (wmGenData.currentSubtile == NULL) {
        // NOTE: Uninline.
        _wmPartyFindCurSubTile();
    }

    wmGenData.encounterTableId = wmGenData.currentSubtile->encounterType;

    EncounterTable* encounterTable = &(gEncounterTables[wmGenData.encounterTableId]);

    int candidates[41];
    int candidatesLength = 0;
    int totalChance = 0;
    for (int index = 0; index < encounterTable->entriesLength; index++) {
        EncounterEntry* encounterTableEntry = &(encounterTable->entries[index]);

        bool selected = true;
        if (_wmEvalConditional(&(encounterTableEntry->condition), NULL) == 0) {
            selected = false;
        }

        if (encounterTableEntry->counter == 0) {
            selected = false;
        }

        if (selected) {
            candidates[candidatesLength++] = index;
            totalChance += encounterTableEntry->chance;
        }
    }

    int v1 = critterGetStat(gDude, STAT_LUCK) - 5;
    int v2 = randomBetween(0, totalChance) + v1;

    if (perkHasRank(gDude, PERK_EXPLORER)) {
        v2 += 2;
    }

    if (perkHasRank(gDude, PERK_RANGER)) {
        v2++;
    }

    if (perkHasRank(gDude, PERK_SCOUT)) {
        v2++;
    }

    int gameDifficulty;
    if (configGetInt(&gGameConfig, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_GAME_DIFFICULTY_KEY, &gameDifficulty)) {
        switch (gameDifficulty) {
        case GAME_DIFFICULTY_EASY:
            v2 += 5;
            if (v2 > totalChance) {
                v2 = totalChance;
            }
            break;
        case GAME_DIFFICULTY_HARD:
            v2 -= 5;
            if (v2 < 0) {
                v2 = 0;
            }
            break;
        }
    }

    int index;
    for (index = 0; index < candidatesLength; index++) {
        EncounterEntry* encounterTableEntry = &(encounterTable->entries[candidates[index]]);
        if (v2 < encounterTableEntry->chance) {
            break;
        }

        v2 -= encounterTableEntry->chance;
    }

    if (index == candidatesLength) {
        index = candidatesLength - 1;
    }

    wmGenData.encounterEntryId = candidates[index];

    EncounterEntry* encounterTableEntry = &(encounterTable->entries[wmGenData.encounterEntryId]);
    if (encounterTableEntry->counter > 0) {
        encounterTableEntry->counter--;
    }

    if (encounterTableEntry->map == -1) {
        if (encounterTable->mapsLength <= 0) {
            Terrain* terrain = &(gTerrains[wmGenData.currentSubtile->terrain]);
            int randomMapIndex = randomBetween(0, terrain->mapsLength - 1);
            wmGenData.encounterMapId = terrain->maps[randomMapIndex];
        } else {
            int randomMapIndex = randomBetween(0, encounterTable->mapsLength - 1);
            wmGenData.encounterMapId = encounterTable->maps[randomMapIndex];
        }
    } else {
        wmGenData.encounterMapId = encounterTableEntry->map;
    }

    return 0;
}

// wmSetupRandomEncounter
// 0x4C0FA4
int worldmapSetupRandomEncounter()
{
    MessageListItem messageListItem;
    char* msg;

    if (wmGenData.encounterMapId == -1) {
        return 0;
    }

    EncounterTable* encounterTable = &(gEncounterTables[wmGenData.encounterTableId]);
    EncounterEntry* encounterTableEntry = &(encounterTable->entries[wmGenData.encounterEntryId]);

    // You encounter:
    msg = getmsg(&gWorldmapMessageList, &messageListItem, 2998);
    displayMonitorAddMessage(msg);

    msg = getmsg(&gWorldmapMessageList, &messageListItem, 3000 + 50 * wmGenData.encounterTableId + wmGenData.encounterEntryId);
    displayMonitorAddMessage(msg);

    int gameDifficulty;
    switch (encounterTableEntry->scenery) {
    case ENCOUNTER_SCENERY_TYPE_NONE:
    case ENCOUNTER_SCENERY_TYPE_LIGHT:
    case ENCOUNTER_SCENERY_TYPE_NORMAL:
    case ENCOUNTER_SCENERY_TYPE_HEAVY:
        debugPrint("\nwmSetupRandomEncounter: Scenery Type: %s", _wmSceneryStrs[encounterTableEntry->scenery]);
        configGetInt(&gGameConfig, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_GAME_DIFFICULTY_KEY, &gameDifficulty);
        break;
    default:
        debugPrint("\nERROR: wmSetupRandomEncounter: invalid Scenery Type!");
        return -1;
    }

    Object* v0 = NULL;
    for (int i = 0; i < encounterTableEntry->field_50; i++) {
        ENCOUNTER_ENTRY_ENC* v3 = &(encounterTableEntry->field_54[i]);

        int v9 = randomBetween(v3->minQuantity, v3->maxQuantity);

        switch (gameDifficulty) {
        case GAME_DIFFICULTY_EASY:
            v9 -= 2;
            if (v9 < v3->minQuantity) {
                v9 = v3->minQuantity;
            }
            break;
        case GAME_DIFFICULTY_HARD:
            v9 += 2;
            break;
        }

        int partyMemberCount = _getPartyMemberCount();
        if (partyMemberCount > 2) {
            v9 += 2;
        }

        if (v9 != 0) {
            Object* v35;
            if (worldmapSetupCritters(v3->field_8, &v35, v9) == -1) {
                scriptsRequestWorldMap();
                return -1;
            }

            if (i > 0) {
                if (v0 != NULL) {
                    if (v0 != v35) {
                        if (encounterTableEntry->field_50 != 1) {
                            if (encounterTableEntry->field_50 == 2 && !isInCombat()) {
                                v0->data.critter.combat.whoHitMe = v35;
                                v35->data.critter.combat.whoHitMe = v0;

                                STRUCT_664980 combat;
                                combat.attacker = v0;
                                combat.defender = v35;
                                combat.actionPointsBonus = 0;
                                combat.accuracyBonus = 0;
                                combat.damageBonus = 0;
                                combat.minDamage = 0;
                                combat.maxDamage = 500;
                                combat.field_1C = 0;

                                _caiSetupTeamCombat(v35, v0);
                                _scripts_request_combat_locked(&combat);
                            }
                        } else {
                            if (!isInCombat()) {
                                v0->data.critter.combat.whoHitMe = gDude;

                                STRUCT_664980 combat;
                                combat.attacker = v0;
                                combat.defender = gDude;
                                combat.actionPointsBonus = 0;
                                combat.accuracyBonus = 0;
                                combat.damageBonus = 0;
                                combat.minDamage = 0;
                                combat.maxDamage = 500;
                                combat.field_1C = 0;

                                _caiSetupTeamCombat(gDude, v0);
                                _scripts_request_combat_locked(&combat);
                            }
                        }
                    }
                }
            }

            v0 = v35;
        }
    }

    return 0;
}

// wmSetupCritterObjs
// 0x4C11FC
int worldmapSetupCritters(int type_idx, Object** critterPtr, int critterCount)
{
    if (type_idx == -1) {
        return 0;
    }

    *critterPtr = 0;

    ENC_BASE_TYPE* v25 = &(_wmEncBaseTypeList[type_idx]);

    debugPrint("\nwmSetupCritterObjs: typeIdx: %d, Formation: %s", type_idx, gEncounterFormationTypeKeys[v25->position]);

    if (_wmSetupRndNextTileNumInit(v25) == -1) {
        return -1;
    }

    for (int i = 0; i < v25->field_34; i++) {
        ENC_BASE_TYPE_38* v5 = &(v25->field_38[i]);

        if (v5->pid == -1) {
            continue;
        }

        if (!_wmEvalConditional(&(v5->condition), &critterCount)) {
            continue;
        }

        int v23;
        switch (v5->field_2C) {
        case 0:
            v23 = v5->ratio * critterCount / 100;
            break;
        case 1:
            v23 = 1;
            break;
        default:
            assert(false && "Should be unreachable");
        }

        if (v23 < 1) {
            v23 = 1;
        }

        for (int j = 0; j < v23; j++) {
            int tile;
            if (_wmSetupRndNextTileNum(v25, v5, &tile) == -1) {
                debugPrint("\nERROR: wmSetupCritterObjs: wmSetupRndNextTileNum:");
                continue;
            }

            if (v5->pid == -1) {
                continue;
            }

            Object* object;
            if (objectCreateWithPid(&object, v5->pid) == -1) {
                return -1;
            }

            if (*critterPtr == NULL) {
                if (PID_TYPE(v5->pid) == OBJ_TYPE_CRITTER) {
                    *critterPtr = object;
                }
            }

            if (v5->team != -1) {
                if (PID_TYPE(object->pid) == OBJ_TYPE_CRITTER) {
                    object->data.critter.combat.team = v5->team;
                }
            }

            if (v5->script != -1) {
                if (object->sid != -1) {
                    scriptRemove(object->sid);
                    object->sid = -1;
                }

                _obj_new_sid_inst(object, SCRIPT_TYPE_CRITTER, v5->script - 1);
            }

            if (v25->position != ENCOUNTER_FORMATION_TYPE_SURROUNDING) {
                objectSetLocation(object, tile, gElevation, NULL);
            } else {
                _obj_attempt_placement(object, tile, 0, 0);
            }

            int direction = tileGetRotationTo(tile, gDude->tile);
            objectSetRotation(object, direction, NULL);

            for (int itemIndex = 0; itemIndex < v5->itemsLength; itemIndex++) {
                ENC_BASE_TYPE_38_48* v10 = &(v5->items[itemIndex]);

                int quantity;
                if (v10->maximumQuantity == v10->minimumQuantity) {
                    quantity = v10->maximumQuantity;
                } else {
                    quantity = randomBetween(v10->minimumQuantity, v10->maximumQuantity);
                }

                if (quantity == 0) {
                    continue;
                }

                Object* item;
                if (objectCreateWithPid(&item, v10->pid) == -1) {
                    return -1;
                }

                if (v10->pid == PROTO_ID_MONEY) {
                    if (perkHasRank(gDude, PERK_FORTUNE_FINDER)) {
                        quantity *= 2;
                    }
                }

                if (itemAdd(object, item, quantity) == -1) {
                    return -1;
                }

                _obj_disconnect(item, NULL);

                if (v10->isEquipped) {
                    if (_inven_wield(object, item, 1) == -1) {
                        debugPrint("\nERROR: wmSetupCritterObjs: Inven Wield Failed: %d on %s: Critter Fid: %d", item->pid, critterGetName(object), object->fid);
                    }
                }
            }
        }
    }

    return 0;
}

// 0x4C155C
int _wmSetupRndNextTileNumInit(ENC_BASE_TYPE* a1)
{
    for (int index = 0; index < 2; index++) {
        _wmRndCenterRotations[index] = 0;
        _wmRndTileDirs[index] = 0;
        _wmRndCenterTiles[index] = -1;

        if (index & 1) {
            _wmRndRotOffsets[index] = 5;
        } else {
            _wmRndRotOffsets[index] = 1;
        }
    }

    _wmRndCallCount = 0;

    switch (a1->position) {
    case ENCOUNTER_FORMATION_TYPE_SURROUNDING:
        _wmRndCenterTiles[0] = gDude->tile;
        _wmRndTileDirs[0] = randomBetween(0, ROTATION_COUNT - 1);

        _wmRndOriginalCenterTile = _wmRndCenterTiles[0];

        return 0;
    case ENCOUNTER_FORMATION_TYPE_STRAIGHT_LINE:
    case ENCOUNTER_FORMATION_TYPE_DOUBLE_LINE:
    case ENCOUNTER_FORMATION_TYPE_WEDGE:
    case ENCOUNTER_FORMATION_TYPE_CONE:
    case ENCOUNTER_FORMATION_TYPE_HUDDLE:
        if (1) {
            MapInfo* map = &(gMaps[gMapHeader.field_34]);
            if (map->startPointsLength != 0) {
                int rspIndex = randomBetween(0, map->startPointsLength - 1);
                MapStartPointInfo* rsp = &(map->startPoints[rspIndex]);

                _wmRndCenterTiles[0] = rsp->tile;
                _wmRndCenterTiles[1] = _wmRndCenterTiles[0];

                _wmRndCenterRotations[0] = rsp->field_8;
                _wmRndCenterRotations[1] = _wmRndCenterRotations[0];
            } else {
                _wmRndCenterRotations[0] = 0;
                _wmRndCenterRotations[1] = 0;

                _wmRndCenterTiles[0] = gDude->tile;
                _wmRndCenterTiles[1] = gDude->tile;
            }

            _wmRndTileDirs[0] = tileGetRotationTo(_wmRndCenterTiles[0], gDude->tile);
            _wmRndTileDirs[1] = tileGetRotationTo(_wmRndCenterTiles[1], gDude->tile);

            _wmRndOriginalCenterTile = _wmRndCenterTiles[0];

            return 0;
        }
    default:
        debugPrint("\nERROR: wmSetupCritterObjs: invalid Formation Type!");

        return -1;
    }
}

// wmSetupRndNextTileNum
// 0x4C16F0
int _wmSetupRndNextTileNum(ENC_BASE_TYPE* a1, ENC_BASE_TYPE_38* a2, int* out_tile_num)
{
    int tile_num;

    int attempt = 0;
    while (1) {
        switch (a1->position) {
        case ENCOUNTER_FORMATION_TYPE_SURROUNDING:
            if (1) {
                int distance;
                if (a2->distance != 0) {
                    distance = a2->distance;
                } else {
                    distance = randomBetween(-2, 2);

                    distance += critterGetStat(gDude, STAT_PERCEPTION);

                    if (perkHasRank(gDude, PERK_CAUTIOUS_NATURE)) {
                        distance += 3;
                    }
                }

                if (distance < 0) {
                    distance = 0;
                }

                int origin = a2->tile;
                if (origin == -1) {
                    origin = tileGetTileInDirection(gDude->tile, _wmRndTileDirs[0], distance);
                }

                if (++_wmRndTileDirs[0] >= ROTATION_COUNT) {
                    _wmRndTileDirs[0] = 0;
                }

                int randomizedDistance = randomBetween(0, distance / 2);
                int randomizedRotation = randomBetween(0, ROTATION_COUNT - 1);
                tile_num = tileGetTileInDirection(origin, (randomizedRotation + _wmRndTileDirs[0]) % ROTATION_COUNT, randomizedDistance);
            }
            break;
        case ENCOUNTER_FORMATION_TYPE_STRAIGHT_LINE:
            tile_num = _wmRndCenterTiles[_wmRndIndex];
            if (_wmRndCallCount != 0) {
                int rotation = (_wmRndRotOffsets[_wmRndIndex] + _wmRndTileDirs[_wmRndIndex]) % ROTATION_COUNT;
                int origin = tileGetTileInDirection(_wmRndCenterTiles[_wmRndIndex], rotation, a1->spacing);
                int v13 = tileGetTileInDirection(origin, (rotation + _wmRndRotOffsets[_wmRndIndex]) % ROTATION_COUNT, a1->spacing);
                _wmRndCenterTiles[_wmRndIndex] = v13;
                _wmRndIndex = 1 - _wmRndIndex;
                tile_num = v13;
            }
            break;
        case ENCOUNTER_FORMATION_TYPE_DOUBLE_LINE:
            tile_num = _wmRndCenterTiles[_wmRndIndex];
            if (_wmRndCallCount != 0) {
                int rotation = (_wmRndRotOffsets[_wmRndIndex] + _wmRndTileDirs[_wmRndIndex]) % ROTATION_COUNT;
                int origin = tileGetTileInDirection(_wmRndCenterTiles[_wmRndIndex], rotation, a1->spacing);
                int v17 = tileGetTileInDirection(origin, (rotation + _wmRndRotOffsets[_wmRndIndex]) % ROTATION_COUNT, a1->spacing);
                _wmRndCenterTiles[_wmRndIndex] = v17;
                _wmRndIndex = 1 - _wmRndIndex;
                tile_num = v17;
            }
            break;
        case ENCOUNTER_FORMATION_TYPE_WEDGE:
            tile_num = _wmRndCenterTiles[_wmRndIndex];
            if (_wmRndCallCount != 0) {
                tile_num = tileGetTileInDirection(_wmRndCenterTiles[_wmRndIndex], (_wmRndRotOffsets[_wmRndIndex] + _wmRndTileDirs[_wmRndIndex]) % ROTATION_COUNT, a1->spacing);
                _wmRndCenterTiles[_wmRndIndex] = tile_num;
                _wmRndIndex = 1 - _wmRndIndex;
            }
            break;
        case ENCOUNTER_FORMATION_TYPE_CONE:
            tile_num = _wmRndCenterTiles[_wmRndIndex];
            if (_wmRndCallCount != 0) {
                tile_num = tileGetTileInDirection(_wmRndCenterTiles[_wmRndIndex], (_wmRndTileDirs[_wmRndIndex] + 3 + _wmRndRotOffsets[_wmRndIndex]) % ROTATION_COUNT, a1->spacing);
                _wmRndCenterTiles[_wmRndIndex] = tile_num;
                _wmRndIndex = 1 - _wmRndIndex;
            }
            break;
        case ENCOUNTER_FORMATION_TYPE_HUDDLE:
            tile_num = _wmRndCenterTiles[0];
            if (_wmRndCallCount != 0) {
                _wmRndTileDirs[0] = (_wmRndTileDirs[0] + 1) % ROTATION_COUNT;
                tile_num = tileGetTileInDirection(_wmRndCenterTiles[0], _wmRndTileDirs[0], a1->spacing);
                _wmRndCenterTiles[0] = tile_num;
            }
            break;
        default:
            assert(false && "Should be unreachable");
        }

        ++attempt;
        ++_wmRndCallCount;

        if (_wmEvalTileNumForPlacement(tile_num)) {
            break;
        }

        debugPrint("\nWARNING: EVAL-TILE-NUM FAILED!");

        if (tileDistanceBetween(_wmRndOriginalCenterTile, _wmRndCenterTiles[_wmRndIndex]) > 25) {
            return -1;
        }

        if (attempt > 25) {
            return -1;
        }
    }

    debugPrint("\nwmSetupRndNextTileNum:TileNum: %d", tile_num);

    *out_tile_num = tile_num;

    return 0;
}

// 0x4C1A64
bool _wmEvalTileNumForPlacement(int tile)
{
    if (_obj_blocking_at(gDude, tile, gElevation) != NULL) {
        return false;
    }

    if (pathfinderFindPath(gDude, gDude->tile, tile, NULL, 0, _obj_shoot_blocking_at) == 0) {
        return false;
    }

    return true;
}

// 0x4C1AC8
bool _wmEvalConditional(EncounterCondition* a1, int* a2)
{
    int value;

    bool matches = true;
    for (int index = 0; index < a1->entriesLength; index++) {
        EncounterConditionEntry* ptr = &(a1->entries[index]);

        matches = true;
        switch (ptr->type) {
        case ENCOUNTER_CONDITION_TYPE_GLOBAL:
            value = gameGetGlobalVar(ptr->param);
            if (!_wmEvalSubConditional(value, ptr->conditionalOperator, ptr->value)) {
                matches = false;
            }
            break;
        case ENCOUNTER_CONDITION_TYPE_NUMBER_OF_CRITTERS:
            if (!_wmEvalSubConditional(*a2, ptr->conditionalOperator, ptr->value)) {
                matches = false;
            }
            break;
        case ENCOUNTER_CONDITION_TYPE_RANDOM:
            value = randomBetween(0, 100);
            if (value > ptr->param) {
                matches = false;
            }
            break;
        case ENCOUNTER_CONDITION_TYPE_PLAYER:
            value = pcGetStat(PC_STAT_LEVEL);
            if (!_wmEvalSubConditional(value, ptr->conditionalOperator, ptr->value)) {
                matches = false;
            }
            break;
        case ENCOUNTER_CONDITION_TYPE_DAYS_PLAYED:
            value = gameTimeGetTime();
            if (!_wmEvalSubConditional(value / GAME_TIME_TICKS_PER_DAY, ptr->conditionalOperator, ptr->value)) {
                matches = false;
            }
            break;
        case ENCOUNTER_CONDITION_TYPE_TIME_OF_DAY:
            value = gameTimeGetHour();
            if (!_wmEvalSubConditional(value / 100, ptr->conditionalOperator, ptr->value)) {
                matches = false;
            }
            break;
        }

        if (!matches) {
            // FIXME: Can overflow with all 3 conditions specified.
            if (a1->logicalOperators[index] == ENCOUNTER_LOGICAL_OPERATOR_AND) {
                break;
            }
        }
    }

    return matches;
}

// 0x4C1C0C
bool _wmEvalSubConditional(int operand1, int condionalOperator, int operand2)
{
    switch (condionalOperator) {
    case ENCOUNTER_CONDITIONAL_OPERATOR_EQUAL:
        return operand1 == operand2;
    case ENCOUNTER_CONDITIONAL_OPERATOR_NOT_EQUAL:
        return operand1 != operand2;
    case ENCOUNTER_CONDITIONAL_OPERATOR_LESS_THAN:
        return operand1 < operand2;
    case ENCOUNTER_CONDITIONAL_OPERATOR_GREATER_THAN:
        return operand1 > operand2;
    }

    return false;
}

// 0x4C1C50
bool _wmGameTimeIncrement(int a1)
{
    if (a1 == 0) {
        return false;
    }

    while (a1 != 0) {
        unsigned int gameTime = gameTimeGetTime();
        unsigned int nextEventTime = queueGetNextEventTime();
        int v1 = nextEventTime >= gameTime ? a1 : nextEventTime - gameTime;
        a1 -= v1;

        gameTimeAddTicks(v1);

        // NOTE: Uninline.
        wmInterfaceDialSyncTime(true);

        worldmapWindowRenderDate(true);

        if (queueProcessEvents()) {
            break;
        }
    }

    return true;
}

// Reads .msk file if needed.
//
// 0x4C1CE8
int _wmGrabTileWalkMask(int tile)
{
    TileInfo* tileInfo = &(gWorldmapTiles[tile]);
    if (tileInfo->walkMaskData != NULL) {
        return 0;
    }

    if (*tileInfo->walkMaskName == '\0') {
        return 0;
    }

    tileInfo->walkMaskData = (unsigned char*)internal_malloc(13200);
    if (tileInfo->walkMaskData == NULL) {
        return -1;
    }

    char path[MAX_PATH];
    sprintf(path, "data\\%s.msk", tileInfo->walkMaskName);

    File* stream = fileOpen(path, "rb");
    if (stream == NULL) {
        return -1;
    }

    int rc = 0;

    if (fileReadUInt8List(stream, tileInfo->walkMaskData, 13200) == -1) {
        rc = -1;
    }

    fileClose(stream);

    return rc;
}

// 0x4C1D9C
bool _wmWorldPosInvalid(int a1, int a2)
{
    int v3 = a2 / WM_TILE_HEIGHT * gWorldmapGridWidth + a1 / WM_TILE_WIDTH % gWorldmapGridWidth;
    if (_wmGrabTileWalkMask(v3) == -1) {
        return false;
    }

    TileInfo* tileDescription = &(gWorldmapTiles[v3]);
    unsigned char* mask = tileDescription->walkMaskData;
    if (mask == NULL) {
        return false;
    }

    // Mask length is 13200, which is 300 * 44
    // 44 * 8 is 352, which is probably left 2 bytes intact
    // TODO: Check math.
    int pos = (a2 % WM_TILE_HEIGHT) * 44 + (a1 % WM_TILE_WIDTH) / 8;
    int bit = 1 << (((a1 % WM_TILE_WIDTH) / 8) & 3);
    return (mask[pos] & bit) != 0;
}

// 0x4C1E54
void _wmPartyInitWalking(int x, int y)
{
    wmGenData.walkDestinationX = x;
    wmGenData.walkDestinationY = y;
    wmGenData.currentAreaId = -1;
    wmGenData.isWalking = true;

    int dx = abs(x - wmGenData.worldPosX);
    int dy = abs(y - wmGenData.worldPosY);

    if (dx < dy) {
        wmGenData.walkDistance = dy;
        wmGenData.walkLineDeltaMainAxisStep = 2 * dx;
        wmGenData.walkWorldPosMainAxisStepX = 0;
        wmGenData.walkLineDelta = 2 * dx - dy;
        wmGenData.walkLineDeltaCrossAxisStep = 2 * (dx - dy);
        wmGenData.walkWorldPosCrossAxisStepX = 1;
        wmGenData.walkWorldPosMainAxisStepY = 1;
        wmGenData.walkWorldPosCrossAxisStepY = 1;
    } else {
        wmGenData.walkDistance = dx;
        wmGenData.walkLineDeltaMainAxisStep = 2 * dy;
        wmGenData.walkWorldPosMainAxisStepY = 0;
        wmGenData.walkLineDelta = 2 * dy - dx;
        wmGenData.walkLineDeltaCrossAxisStep = 2 * (dy - dx);
        wmGenData.walkWorldPosMainAxisStepX = 1;
        wmGenData.walkWorldPosCrossAxisStepX = 1;
        wmGenData.walkWorldPosCrossAxisStepY = 1;
    }

    if (wmGenData.walkDestinationX < wmGenData.worldPosX) {
        wmGenData.walkWorldPosCrossAxisStepX = -wmGenData.walkWorldPosCrossAxisStepX;
        wmGenData.walkWorldPosMainAxisStepX = -wmGenData.walkWorldPosMainAxisStepX;
    }

    if (wmGenData.walkDestinationY < wmGenData.worldPosY) {
        wmGenData.walkWorldPosCrossAxisStepY = -wmGenData.walkWorldPosCrossAxisStepY;
        wmGenData.walkWorldPosMainAxisStepY = -wmGenData.walkWorldPosMainAxisStepY;
    }

    if (!_wmCursorIsVisible()) {
        _wmInterfaceCenterOnParty();
    }
}

// 0x4C1F90
void worldmapPerformTravel()
{
    if (wmGenData.walkDistance <= 0) {
        return;
    }

    _terrainCounter++;
    if (_terrainCounter > 4) {
        _terrainCounter = 1;
    }

    // NOTE: Uninline.
    _wmPartyFindCurSubTile();

    Terrain* terrain = &(gTerrains[wmGenData.currentSubtile->terrain]);
    int v1 = terrain->type - perkGetRank(gDude, PERK_PATHFINDER);
    if (v1 < 1) {
        v1 = 1;
    }

    if (_terrainCounter / v1 >= 1) {
        int v3;
        int v4;
        if (wmGenData.walkLineDelta >= 0) {
            if (_wmWorldPosInvalid(wmGenData.walkWorldPosCrossAxisStepX + wmGenData.worldPosX, wmGenData.walkWorldPosCrossAxisStepY + wmGenData.worldPosY)) {
                wmGenData.walkDestinationX = 0;
                wmGenData.walkDestinationY = 0;
                wmGenData.isWalking = false;
                _wmMatchWorldPosToArea(wmGenData.worldPosX, wmGenData.worldPosX, &(wmGenData.currentAreaId));
                wmGenData.walkDistance = 0;
                return;
            }

            v3 = wmGenData.walkWorldPosCrossAxisStepX;
            wmGenData.walkLineDelta += wmGenData.walkLineDeltaCrossAxisStep;
            wmGenData.worldPosX += wmGenData.walkWorldPosCrossAxisStepX;
            v4 = wmGenData.walkWorldPosCrossAxisStepY;
            wmGenData.worldPosY += wmGenData.walkWorldPosCrossAxisStepY;
        } else {
            if (_wmWorldPosInvalid(wmGenData.walkWorldPosMainAxisStepX + wmGenData.worldPosX, wmGenData.walkWorldPosMainAxisStepY + wmGenData.worldPosY) == 1) {
                wmGenData.walkDestinationX = 0;
                wmGenData.walkDestinationY = 0;
                wmGenData.isWalking = false;
                _wmMatchWorldPosToArea(wmGenData.worldPosX, wmGenData.worldPosX, &(wmGenData.currentAreaId));
                wmGenData.walkDistance = 0;
                return;
            }

            v3 = wmGenData.walkWorldPosMainAxisStepX;
            wmGenData.walkLineDelta += wmGenData.walkLineDeltaMainAxisStep;
            wmGenData.worldPosY += wmGenData.walkWorldPosMainAxisStepY;
            v4 = wmGenData.walkWorldPosMainAxisStepY;
            wmGenData.worldPosX += wmGenData.walkWorldPosMainAxisStepX;
        }

        worldmapWindowScroll(1, 1, v3, v4, NULL, false);

        wmGenData.walkDistance -= 1;
        if (wmGenData.walkDistance == 0) {
            wmGenData.walkDestinationY = 0;
            wmGenData.isWalking = false;
            wmGenData.walkDestinationX = 0;
        }
    }
}

// 0x4C219C
void _wmInterfaceScrollTabsStart(int a1)
{
    int i;
    int v3;

    for (i = 0; i < 7; i++) {
        buttonDisable(_wmTownMapSubButtonIds[i]);
    }

    wmGenData.oldTabsOffsetY = wmGenData.tabsOffsetY;

    v3 = wmGenData.tabsOffsetY + 7 * a1;

    if (a1 >= 0) {
        if (wmGenData.tabsBackgroundFrmHeight - 230 <= wmGenData.oldTabsOffsetY) {
            goto L11;
        } else {
            wmGenData.oldTabsOffsetY = wmGenData.tabsOffsetY + 7 * a1;
            if (v3 > wmGenData.tabsBackgroundFrmHeight - 230) {
            }
        }
    } else {
        if (wmGenData.tabsOffsetY <= 0) {
            goto L11;
        } else {
            wmGenData.oldTabsOffsetY = wmGenData.tabsOffsetY + 7 * a1;
            if (v3 < 0) {
                wmGenData.oldTabsOffsetY = 0;
            }
        }
    }

    wmGenData.tabsScrollingDelta = a1;

L11:

    // NOTE: Uninline.
    wmInterfaceScrollTabsUpdate();
}

// 0x4C2270
void _wmInterfaceScrollTabsStop()
{
    int i;

    wmGenData.tabsScrollingDelta = 0;

    for (i = 0; i < 7; i++) {
        buttonEnable(_wmTownMapSubButtonIds[i]);
    }
}

// NOTE: Inlined.
//
// 0x4C2290
void wmInterfaceScrollTabsUpdate()
{
    if (wmGenData.tabsScrollingDelta != 0) {
        wmGenData.tabsOffsetY += wmGenData.tabsScrollingDelta;
        worldmapWindowRenderChrome(1);

        if (wmGenData.tabsScrollingDelta >= 0) {
            if (wmGenData.oldTabsOffsetY <= wmGenData.tabsOffsetY) {
                // NOTE: Uninline.
                _wmInterfaceScrollTabsStop();
            }
        } else {
            if (wmGenData.oldTabsOffsetY >= wmGenData.tabsOffsetY) {
                // NOTE: Uninline.
                _wmInterfaceScrollTabsStop();
            }
        }
    }
}

// 0x4C2324
int worldmapWindowInit()
{
    int fid;
    Art* frm;
    CacheEntry* frmHandle;

    _wmLastRndTime = _get_time();
    wmGenData.oldFont = fontGetCurrent();
    fontSetCurrent(0);

    _map_save_in_game(true);

    const char* backgroundSoundFileName = wmGenData.isInCar ? "20car" : "23world";
    _gsound_background_play_level_music(backgroundSoundFileName, 12);

    indicatorBarHide();
    isoDisable();
    colorCycleDisable();
    gameMouseSetCursor(MOUSE_CURSOR_ARROW);

    int worldmapWindowX = 0;
    int worldmapWindowY = 0;
    gWorldmapWindow = windowCreate(worldmapWindowX, worldmapWindowY, WM_WINDOW_WIDTH, WM_WINDOW_HEIGHT, _colorTable[0], WINDOW_FLAG_0x04);
    if (gWorldmapWindow == -1) {
        return -1;
    }

    fid = buildFid(OBJ_TYPE_INTERFACE, 136, 0, 0, 0);
    frm = artLock(fid, &gWorldmapBoxFrmHandle);
    if (frm == NULL) {
        return -1;
    }

    gWorldmapBoxFrmWidth = artGetWidth(frm, 0, 0);
    gWorldmapBoxFrmHeight = artGetHeight(frm, 0, 0);

    artUnlock(gWorldmapBoxFrmHandle);
    gWorldmapBoxFrmHandle = INVALID_CACHE_ENTRY;

    fid = buildFid(OBJ_TYPE_INTERFACE, 136, 0, 0, 0);
    gWorldmapBoxFrmData = artLockFrameData(fid, 0, 0, &gWorldmapBoxFrmHandle);
    if (gWorldmapBoxFrmData == NULL) {
        return -1;
    }

    gWorldmapWindowBuffer = windowGetBuffer(gWorldmapWindow);
    if (gWorldmapWindowBuffer == NULL) {
        return -1;
    }

    blitBufferToBuffer(gWorldmapBoxFrmData, gWorldmapBoxFrmWidth, gWorldmapBoxFrmHeight, gWorldmapBoxFrmWidth, gWorldmapWindowBuffer, WM_WINDOW_WIDTH);

    for (int citySize = 0; citySize < CITY_SIZE_COUNT; citySize++) {
        CitySizeDescription* citySizeDescription = &(gCitySizeDescriptions[citySize]);

        fid = buildFid(OBJ_TYPE_INTERFACE, 336 + citySize, 0, 0, 0);
        citySizeDescription->fid = fid;

        frm = artLock(fid, &(citySizeDescription->handle));
        if (frm == NULL) {
            return -1;
        }

        citySizeDescription->width = artGetWidth(frm, 0, 0);
        citySizeDescription->height = artGetHeight(frm, 0, 0);

        artUnlock(citySizeDescription->handle);
        citySizeDescription->handle = INVALID_CACHE_ENTRY;

        citySizeDescription->data = artLockFrameData(fid, 0, 0, &(citySizeDescription->handle));
        // FIXME: check is obviously wrong, should be citySizeDescription->data.
        if (frm == NULL) {
            return -1;
        }
    }

    fid = buildFid(OBJ_TYPE_INTERFACE, 168, 0, 0, 0);
    frm = artLock(fid, &(wmGenData.hotspotNormalFrmHandle));
    if (frm == NULL) {
        return -1;
    }

    wmGenData.hotspotFrmWidth = artGetWidth(frm, 0, 0);
    wmGenData.hotspotFrmHeight = artGetHeight(frm, 0, 0);

    artUnlock(wmGenData.hotspotNormalFrmHandle);
    wmGenData.hotspotNormalFrmHandle = INVALID_CACHE_ENTRY;

    // hotspot1.frm - town map selector shape #1
    fid = buildFid(OBJ_TYPE_INTERFACE, 168, 0, 0, 0);
    wmGenData.hotspotNormalFrmData = artLockFrameData(fid, 0, 0, &(wmGenData.hotspotNormalFrmHandle));

    // hotspot2.frm - town map selector shape #2
    fid = buildFid(OBJ_TYPE_INTERFACE, 223, 0, 0, 0);
    wmGenData.hotspotPressedFrmData = artLockFrameData(fid, 0, 0, &(wmGenData.hotspotPressedFrmHandle));
    if (wmGenData.hotspotPressedFrmData == NULL) {
        return -1;
    }

    // wmaptarg.frm - world map move target maker #1
    fid = buildFid(OBJ_TYPE_INTERFACE, 139, 0, 0, 0);
    frm = artLock(fid, &(wmGenData.destinationMarkerFrmHandle));
    if (frm == NULL) {
        return -1;
    }

    wmGenData.destinationMarkerFrmWidth = artGetWidth(frm, 0, 0);
    wmGenData.destinationMarkerFrmHeight = artGetHeight(frm, 0, 0);

    artUnlock(wmGenData.destinationMarkerFrmHandle);
    wmGenData.destinationMarkerFrmHandle = INVALID_CACHE_ENTRY;

    // wmaploc.frm - world map location marker
    fid = buildFid(OBJ_TYPE_INTERFACE, 138, 0, 0, 0);
    frm = artLock(fid, &(wmGenData.locationMarkerFrmHandle));
    if (frm == NULL) {
        return -1;
    }

    wmGenData.locationMarkerFrmWidth = artGetWidth(frm, 0, 0);
    wmGenData.locationMarkerFrmHeight = artGetHeight(frm, 0, 0);

    artUnlock(wmGenData.locationMarkerFrmHandle);
    wmGenData.locationMarkerFrmHandle = INVALID_CACHE_ENTRY;

    // wmaptarg.frm - world map move target maker #1
    fid = buildFid(OBJ_TYPE_INTERFACE, 139, 0, 0, 0);
    wmGenData.destinationMarkerFrmData = artLockFrameData(fid, 0, 0, &(wmGenData.destinationMarkerFrmHandle));
    if (wmGenData.destinationMarkerFrmData == NULL) {
        return -1;
    }

    // wmaploc.frm - world map location marker
    fid = buildFid(OBJ_TYPE_INTERFACE, 138, 0, 0, 0);
    wmGenData.locationMarkerFrmData = artLockFrameData(fid, 0, 0, &(wmGenData.locationMarkerFrmHandle));
    if (wmGenData.locationMarkerFrmData == NULL) {
        return -1;
    }

    for (int index = 0; index < WORLD_MAP_ENCOUNTER_FRM_COUNT; index++) {
        fid = buildFid(OBJ_TYPE_INTERFACE, gWorldmapEncounterFrmIds[index], 0, 0, 0);
        frm = artLock(fid, &(wmGenData.encounterCursorFrmHandle[index]));
        if (frm == NULL) {
            return -1;
        }

        wmGenData.encounterCursorFrmWidths[index] = artGetWidth(frm, 0, 0);
        wmGenData.encounterCursorFrmHeights[index] = artGetHeight(frm, 0, 0);

        artUnlock(wmGenData.encounterCursorFrmHandle[index]);

        wmGenData.encounterCursorFrmHandle[index] = INVALID_CACHE_ENTRY;
        wmGenData.encounterCursorFrmData[index] = artLockFrameData(fid, 0, 0, &(wmGenData.encounterCursorFrmHandle[index]));
    }

    for (int index = 0; index < gWorldmapTilesLength; index++) {
        gWorldmapTiles[index].handle = INVALID_CACHE_ENTRY;
    }

    // wmtabs.frm - worldmap town tabs underlay
    fid = buildFid(OBJ_TYPE_INTERFACE, 364, 0, 0, 0);
    frm = artLock(fid, &frmHandle);
    if (frm == NULL) {
        return -1;
    }

    wmGenData.tabsBackgroundFrmWidth = artGetWidth(frm, 0, 0);
    wmGenData.tabsBackgroundFrmHeight = artGetHeight(frm, 0, 0);

    artUnlock(frmHandle);

    wmGenData.tabsBackgroundFrmData = artLockFrameData(fid, 0, 0, &(wmGenData.tabsBackgroundFrmHandle)) + wmGenData.tabsBackgroundFrmWidth * 27;
    if (wmGenData.tabsBackgroundFrmData == NULL) {
        return -1;
    }

    // wmtbedge.frm - worldmap town tabs edging overlay
    fid = buildFid(OBJ_TYPE_INTERFACE, 367, 0, 0, 0);
    wmGenData.tabsBorderFrmData = artLockFrameData(fid, 0, 0, &(wmGenData.tabsBorderFrmHandle));
    if (wmGenData.tabsBorderFrmData == NULL) {
        return -1;
    }

    // wmdial.frm - worldmap night/day dial
    fid = buildFid(OBJ_TYPE_INTERFACE, 365, 0, 0, 0);
    wmGenData.dialFrm = artLock(fid, &(wmGenData.dialFrmHandle));
    if (wmGenData.dialFrm == NULL) {
        return -1;
    }

    wmGenData.dialFrmWidth = artGetWidth(wmGenData.dialFrm, 0, 0);
    wmGenData.dialFrmHeight = artGetHeight(wmGenData.dialFrm, 0, 0);

    // wmscreen - worldmap overlay screen
    fid = buildFid(OBJ_TYPE_INTERFACE, 363, 0, 0, 0);
    frm = artLock(fid, &frmHandle);
    if (frm == NULL) {
        return -1;
    }

    wmGenData.carImageOverlayFrmWidth = artGetWidth(frm, 0, 0);
    wmGenData.carImageOverlayFrmHeight = artGetHeight(frm, 0, 0);

    artUnlock(frmHandle);

    wmGenData.carImageOverlayFrmData = artLockFrameData(fid, 0, 0, &(wmGenData.carImageOverlayFrmHandle));
    if (wmGenData.carImageOverlayFrmData == NULL) {
        return -1;
    }

    // wmglobe.frm - worldmap globe stamp overlay
    fid = buildFid(OBJ_TYPE_INTERFACE, 366, 0, 0, 0);
    frm = artLock(fid, &frmHandle);
    if (frm == NULL) {
        return -1;
    }

    wmGenData.globeOverlayFrmWidth = artGetWidth(frm, 0, 0);
    wmGenData.globeOverlayFrmHeight = artGetHeight(frm, 0, 0);

    artUnlock(frmHandle);

    wmGenData.globeOverlayFrmData = artLockFrameData(fid, 0, 0, &(wmGenData.globeOverlayFrmHandle));
    if (wmGenData.globeOverlayFrmData == NULL) {
        return -1;
    }

    // lilredup.frm - little red button up
    fid = buildFid(OBJ_TYPE_INTERFACE, 8, 0, 0, 0);
    frm = artLock(fid, &frmHandle);
    if (frm == NULL) {
        return -1;
    }

    int littleRedButtonUpWidth = artGetWidth(frm, 0, 0);
    int littleRedButtonUpHeight = artGetHeight(frm, 0, 0);

    artUnlock(frmHandle);

    wmGenData.littleRedButtonNormalFrmData = artLockFrameData(fid, 0, 0, &(wmGenData.littleRedButtonNormalFrmHandle));

    // lilreddn.frm - little red button down
    fid = buildFid(OBJ_TYPE_INTERFACE, 9, 0, 0, 0);
    wmGenData.littleRedButtonPressedFrmData = artLockFrameData(fid, 0, 0, &(wmGenData.littleRedButtonPressedFrmHandle));

    // months.frm - month strings for pip boy
    fid = buildFid(OBJ_TYPE_INTERFACE, 129, 0, 0, 0);
    wmGenData.monthsFrm = artLock(fid, &(wmGenData.monthsFrmHandle));
    if (wmGenData.monthsFrm == NULL) {
        return -1;
    }

    // numbers.frm - numbers for the hit points and fatigue counters
    fid = buildFid(OBJ_TYPE_INTERFACE, 82, 0, 0, 0);
    wmGenData.numbersFrm = artLock(fid, &(wmGenData.numbersFrmHandle));
    if (wmGenData.numbersFrm == NULL) {
        return -1;
    }

    // create town/world switch button
    buttonCreate(gWorldmapWindow,
        WM_TOWN_WORLD_SWITCH_X,
        WM_TOWN_WORLD_SWITCH_Y,
        littleRedButtonUpWidth,
        littleRedButtonUpHeight,
        -1,
        -1,
        -1,
        KEY_UPPERCASE_T,
        wmGenData.littleRedButtonNormalFrmData,
        wmGenData.littleRedButtonPressedFrmData,
        NULL,
        BUTTON_FLAG_TRANSPARENT);

    for (int index = 0; index < 7; index++) {
        _wmTownMapSubButtonIds[index] = buttonCreate(gWorldmapWindow,
            508,
            138 + 27 * index,
            littleRedButtonUpWidth,
            littleRedButtonUpHeight,
            -1,
            -1,
            -1,
            KEY_CTRL_F1 + index,
            wmGenData.littleRedButtonNormalFrmData,
            wmGenData.littleRedButtonPressedFrmData,
            NULL,
            BUTTON_FLAG_TRANSPARENT);
    }

    for (int index = 0; index < WORLDMAP_ARROW_FRM_COUNT; index++) {
        // 200 - uparwon.frm - character editor
        // 199 - uparwoff.frm - character editor
        fid = buildFid(OBJ_TYPE_INTERFACE, 200 - index, 0, 0, 0);
        frm = artLock(fid, &(wmGenData.scrollUpButtonFrmHandle[index]));
        if (frm == NULL) {
            return -1;
        }

        wmGenData.scrollUpButtonFrmWidth = artGetWidth(frm, 0, 0);
        wmGenData.scrollUpButtonFrmHeight = artGetHeight(frm, 0, 0);
        wmGenData.scrollUpButtonFrmData[index] = artGetFrameData(frm, 0, 0);
    }

    for (int index = 0; index < WORLDMAP_ARROW_FRM_COUNT; index++) {
        // 182 - dnarwon.frm - character editor
        // 181 - dnarwoff.frm - character editor
        fid = buildFid(OBJ_TYPE_INTERFACE, 182 - index, 0, 0, 0);
        frm = artLock(fid, &(wmGenData.scrollDownButtonFrmHandle[index]));
        if (frm == NULL) {
            return -1;
        }

        wmGenData.scrollDownButtonFrmWidth = artGetWidth(frm, 0, 0);
        wmGenData.scrollDownButtonFrmHeight = artGetHeight(frm, 0, 0);
        wmGenData.scrollDownButtonFrmData[index] = artGetFrameData(frm, 0, 0);
    }

    // Scroll up button.
    buttonCreate(gWorldmapWindow,
        WM_TOWN_LIST_SCROLL_UP_X,
        WM_TOWN_LIST_SCROLL_UP_Y,
        wmGenData.scrollUpButtonFrmWidth,
        wmGenData.scrollUpButtonFrmHeight,
        -1,
        -1,
        -1,
        KEY_CTRL_ARROW_UP,
        wmGenData.scrollUpButtonFrmData[WORLDMAP_ARROW_FRM_NORMAL],
        wmGenData.scrollUpButtonFrmData[WORLDMAP_ARROW_FRM_PRESSED],
        NULL,
        BUTTON_FLAG_TRANSPARENT);

    // Scroll down button.
    buttonCreate(gWorldmapWindow,
        WM_TOWN_LIST_SCROLL_DOWN_X,
        WM_TOWN_LIST_SCROLL_DOWN_Y,
        wmGenData.scrollDownButtonFrmWidth,
        wmGenData.scrollDownButtonFrmHeight,
        -1,
        -1,
        -1,
        KEY_CTRL_ARROW_DOWN,
        wmGenData.scrollDownButtonFrmData[WORLDMAP_ARROW_FRM_NORMAL],
        wmGenData.scrollDownButtonFrmData[WORLDMAP_ARROW_FRM_PRESSED],
        NULL,
        BUTTON_FLAG_TRANSPARENT);

    if (wmGenData.isInCar) {
        // wmcarmve.frm - worldmap car movie
        fid = buildFid(OBJ_TYPE_INTERFACE, 433, 0, 0, 0);
        wmGenData.carImageFrm = artLock(fid, &(wmGenData.carImageFrmHandle));
        if (wmGenData.carImageFrm == NULL) {
            return -1;
        }

        wmGenData.carImageFrmWidth = artGetWidth(wmGenData.carImageFrm, 0, 0);
        wmGenData.carImageFrmHeight = artGetHeight(wmGenData.carImageFrm, 0, 0);
    }

    tickersAdd(worldmapWindowHandleMouseScrolling);

    if (_wmMakeTabsLabelList(&gQuickDestinations, &gQuickDestinationsLength) == -1) {
        return -1;
    }

    _wmInterfaceWasInitialized = 1;

    if (worldmapWindowRefresh() == -1) {
        return -1;
    }

    win_draw(gWorldmapWindow);
    scriptsDisable();
    _scr_remove_all();

    return 0;
}

// 0x4C2E44
int worldmapWindowFree()
{
    int i;
    TileInfo* tile;

    tickersRemove(worldmapWindowHandleMouseScrolling);

    if (gWorldmapBoxFrmData != NULL) {
        artUnlock(gWorldmapBoxFrmHandle);
        gWorldmapBoxFrmData = NULL;
    }
    gWorldmapBoxFrmHandle = INVALID_CACHE_ENTRY;

    if (gWorldmapWindow != -1) {
        windowDestroy(gWorldmapWindow);
        gWorldmapWindow = -1;
    }

    if (wmGenData.hotspotNormalFrmHandle != INVALID_CACHE_ENTRY) {
        artUnlock(wmGenData.hotspotNormalFrmHandle);
    }
    wmGenData.hotspotNormalFrmData = NULL;

    if (wmGenData.hotspotPressedFrmHandle != INVALID_CACHE_ENTRY) {
        artUnlock(wmGenData.hotspotPressedFrmHandle);
    }
    wmGenData.hotspotPressedFrmData = NULL;

    if (wmGenData.destinationMarkerFrmHandle != INVALID_CACHE_ENTRY) {
        artUnlock(wmGenData.destinationMarkerFrmHandle);
    }
    wmGenData.destinationMarkerFrmData = NULL;

    if (wmGenData.locationMarkerFrmHandle != INVALID_CACHE_ENTRY) {
        artUnlock(wmGenData.locationMarkerFrmHandle);
    }
    wmGenData.locationMarkerFrmData = NULL;

    for (i = 0; i < 4; i++) {
        if (wmGenData.encounterCursorFrmHandle[i] != INVALID_CACHE_ENTRY) {
            artUnlock(wmGenData.encounterCursorFrmHandle[i]);
        }
        wmGenData.encounterCursorFrmData[i] = NULL;
    }

    for (i = 0; i < CITY_SIZE_COUNT; i++) {
        CitySizeDescription* citySizeDescription = &(gCitySizeDescriptions[i]);
        // FIXME: probably unsafe code, no check for -1
        artUnlock(citySizeDescription->handle);
        citySizeDescription->handle = INVALID_CACHE_ENTRY;
        citySizeDescription->data = NULL;
    }

    for (i = 0; i < gWorldmapTilesLength; i++) {
        tile = &(gWorldmapTiles[i]);
        if (tile->handle != INVALID_CACHE_ENTRY) {
            artUnlock(tile->handle);
            tile->handle = INVALID_CACHE_ENTRY;
            tile->data = NULL;

            if (tile->walkMaskData != NULL) {
                internal_free(tile->walkMaskData);
                tile->walkMaskData = NULL;
            }
        }
    }

    if (wmGenData.tabsBackgroundFrmData != NULL) {
        artUnlock(wmGenData.tabsBackgroundFrmHandle);
        wmGenData.tabsBackgroundFrmHandle = INVALID_CACHE_ENTRY;
        wmGenData.tabsBackgroundFrmData = NULL;
    }

    if (wmGenData.tabsBorderFrmData != NULL) {
        artUnlock(wmGenData.tabsBorderFrmHandle);
        wmGenData.tabsBorderFrmHandle = INVALID_CACHE_ENTRY;
        wmGenData.tabsBorderFrmData = NULL;
    }

    if (wmGenData.dialFrm != NULL) {
        artUnlock(wmGenData.dialFrmHandle);
        wmGenData.dialFrmHandle = INVALID_CACHE_ENTRY;
        wmGenData.dialFrm = NULL;
    }

    if (wmGenData.carImageOverlayFrmData != NULL) {
        artUnlock(wmGenData.carImageOverlayFrmHandle);
        wmGenData.carImageOverlayFrmHandle = INVALID_CACHE_ENTRY;
        wmGenData.carImageOverlayFrmData = NULL;
    }
    if (wmGenData.globeOverlayFrmData != NULL) {
        artUnlock(wmGenData.globeOverlayFrmHandle);
        wmGenData.globeOverlayFrmHandle = INVALID_CACHE_ENTRY;
        wmGenData.globeOverlayFrmData = NULL;
    }

    if (wmGenData.littleRedButtonNormalFrmData != NULL) {
        artUnlock(wmGenData.littleRedButtonNormalFrmHandle);
        wmGenData.littleRedButtonNormalFrmHandle = INVALID_CACHE_ENTRY;
        wmGenData.littleRedButtonNormalFrmData = NULL;
    }

    if (wmGenData.littleRedButtonPressedFrmData != NULL) {
        artUnlock(wmGenData.littleRedButtonPressedFrmHandle);
        wmGenData.littleRedButtonPressedFrmHandle = INVALID_CACHE_ENTRY;
        wmGenData.littleRedButtonPressedFrmData = NULL;
    }

    for (i = 0; i < 2; i++) {
        artUnlock(wmGenData.scrollUpButtonFrmHandle[i]);
        wmGenData.scrollUpButtonFrmHandle[i] = INVALID_CACHE_ENTRY;
        wmGenData.scrollUpButtonFrmData[i] = NULL;

        artUnlock(wmGenData.scrollDownButtonFrmHandle[i]);
        wmGenData.scrollDownButtonFrmHandle[i] = INVALID_CACHE_ENTRY;
        wmGenData.scrollDownButtonFrmData[i] = NULL;
    }

    wmGenData.scrollUpButtonFrmHeight = 0;
    wmGenData.scrollDownButtonFrmWidth = 0;
    wmGenData.scrollDownButtonFrmHeight = 0;
    wmGenData.scrollUpButtonFrmWidth = 0;

    if (wmGenData.monthsFrm != NULL) {
        artUnlock(wmGenData.monthsFrmHandle);
        wmGenData.monthsFrmHandle = INVALID_CACHE_ENTRY;
        wmGenData.monthsFrm = NULL;
    }

    if (wmGenData.numbersFrm != NULL) {
        artUnlock(wmGenData.numbersFrmHandle);
        wmGenData.numbersFrmHandle = INVALID_CACHE_ENTRY;
        wmGenData.numbersFrm = NULL;
    }

    if (wmGenData.carImageFrm != NULL) {
        artUnlock(wmGenData.carImageFrmHandle);
        wmGenData.carImageFrmHandle = INVALID_CACHE_ENTRY;
        wmGenData.carImageFrm = NULL;

        wmGenData.carImageFrmWidth = 0;
        wmGenData.carImageFrmHeight = 0;
    }

    wmGenData.encounterIconIsVisible = 0;
    wmGenData.encounterMapId = -1;
    wmGenData.encounterTableId = -1;
    wmGenData.encounterEntryId = -1;

    indicatorBarShow();
    isoEnable();
    colorCycleEnable();

    fontSetCurrent(wmGenData.oldFont);

    // NOTE: Uninline.
    wmFreeTabsLabelList(&gQuickDestinations, &gQuickDestinationsLength);

    _wmInterfaceWasInitialized = 0;

    scriptsEnable();

    return 0;
}

// NOTE: Inlined.
//
// 0x4C31E8
int wmInterfaceScroll(int dx, int dy, bool* successPtr)
{
    return worldmapWindowScroll(20, 20, dx, dy, successPtr, 1);
}

// FIXME: There is small bug in this function. There is [success] flag returned
// by reference so that calling code can update scrolling mouse cursor to invalid
// range. It works OK on straight directions. But in diagonals when scrolling in
// one direction is possible (and in fact occured), it will still be reported as
// error.
//
// 0x4C3200
int worldmapWindowScroll(int stepX, int stepY, int dx, int dy, bool* success, bool shouldRefresh)
{
    int v6 = gWorldmapOffsetY;
    int v7 = gWorldmapOffsetX;

    if (success != NULL) {
        *success = true;
    }

    if (dy < 0) {
        if (v6 > 0) {
            v6 -= stepY;
            if (v6 < 0) {
                v6 = 0;
            }
        } else {
            if (success != NULL) {
                *success = false;
            }
        }
    } else if (dy > 0) {
        if (v6 < wmGenData.viewportMaxY) {
            v6 += stepY;
            if (v6 > wmGenData.viewportMaxY) {
                v6 = wmGenData.viewportMaxY;
            }
        } else {
            if (success != NULL) {
                *success = false;
            }
        }
    }

    if (dx < 0) {
        if (v7 > 0) {
            v7 -= stepX;
            if (v7 < 0) {
                v7 = 0;
            }
        } else {
            if (success != NULL) {
                *success = false;
            }
        }
    } else if (dx > 0) {
        if (v7 < wmGenData.viewportMaxX) {
            v7 += stepX;
            if (v7 > wmGenData.viewportMaxX) {
                v7 = wmGenData.viewportMaxX;
            }
        } else {
            if (success != NULL) {
                *success = false;
            }
        }
    }

    gWorldmapOffsetY = v6;
    gWorldmapOffsetX = v7;

    if (shouldRefresh) {
        if (worldmapWindowRefresh() == -1) {
            return -1;
        }
    }

    return 0;
}

// 0x4C32EC
void worldmapWindowHandleMouseScrolling()
{
    int x;
    int y;
    mouseGetPosition(&x, &y);

    int dx = 0;
    if (x == 639) {
        dx = 1;
    } else if (x == 0) {
        dx = -1;
    }

    int dy = 0;
    if (y == 479) {
        dy = 1;
    } else if (y == 0) {
        dy = -1;
    }

    int oldMouseCursor = gameMouseGetCursor();
    int newMouseCursor = oldMouseCursor;

    if (dx != 0 || dy != 0) {
        if (dx > 0) {
            if (dy > 0) {
                newMouseCursor = MOUSE_CURSOR_SCROLL_SE;
            } else if (dy < 0) {
                newMouseCursor = MOUSE_CURSOR_SCROLL_NE;
            } else {
                newMouseCursor = MOUSE_CURSOR_SCROLL_E;
            }
        } else if (dx < 0) {
            if (dy > 0) {
                newMouseCursor = MOUSE_CURSOR_SCROLL_SW;
            } else if (dy < 0) {
                newMouseCursor = MOUSE_CURSOR_SCROLL_NW;
            } else {
                newMouseCursor = MOUSE_CURSOR_SCROLL_W;
            }
        } else {
            if (dy < 0) {
                newMouseCursor = MOUSE_CURSOR_SCROLL_N;
            } else if (dy > 0) {
                newMouseCursor = MOUSE_CURSOR_SCROLL_S;
            }
        }

        unsigned int tick = _get_bk_time();
        if (getTicksBetween(tick, _lastTime_2) > 50) {
            _lastTime_2 = _get_bk_time();
            // NOTE: Uninline.
            wmInterfaceScroll(dx, dy, &_couldScroll);
        }

        if (!_couldScroll) {
            newMouseCursor += 8;
        }
    } else {
        if (oldMouseCursor != MOUSE_CURSOR_ARROW) {
            newMouseCursor = MOUSE_CURSOR_ARROW;
        }
    }

    if (oldMouseCursor != newMouseCursor) {
        gameMouseSetCursor(newMouseCursor);
    }
}

// NOTE: Inlined.
//
// 0x4C340C
int wmMarkSubTileOffsetVisited(int tile, int subtileX, int subtileY, int offsetX, int offsetY)
{
    return wmMarkSubTileOffsetVisitedFunc(tile, subtileX, subtileY, offsetX, offsetY, SUBTILE_STATE_VISITED);
}

// NOTE: Inlined.
//
// 0x4C3420
int wmMarkSubTileOffsetKnown(int tile, int subtileX, int subtileY, int offsetX, int offsetY)
{
    return wmMarkSubTileOffsetVisitedFunc(tile, subtileX, subtileY, offsetX, offsetY, SUBTILE_STATE_KNOWN);
}

// 0x4C3434
int wmMarkSubTileOffsetVisitedFunc(int tile, int subtileX, int subtileY, int offsetX, int offsetY, int subtileState)
{
    int actualTile;
    int actualSubtileX;
    int actualSubtileY;
    TileInfo* tileInfo;
    SubtileInfo* subtileInfo;

    actualSubtileX = subtileX + offsetX;
    actualTile = tile;
    actualSubtileY = subtileY + offsetY;

    if (actualSubtileX >= 0) {
        if (actualSubtileX >= SUBTILE_GRID_WIDTH) {
            if (tile % gWorldmapGridWidth == gWorldmapGridWidth - 1) {
                return -1;
            }

            actualTile = tile + 1;
            actualSubtileX %= SUBTILE_GRID_WIDTH;
        }
    } else {
        if (!(tile % gWorldmapGridWidth)) {
            return -1;
        }

        actualSubtileX += SUBTILE_GRID_WIDTH;
        actualTile = tile - 1;
    }

    if (actualSubtileY >= 0) {
        if (actualSubtileY >= SUBTILE_GRID_HEIGHT) {
            if (actualTile > gWorldmapTilesLength - gWorldmapGridWidth - 1) {
                return -1;
            }

            actualTile += gWorldmapGridWidth;
            actualSubtileY %= SUBTILE_GRID_HEIGHT;
        }
    } else {
        if (actualTile < gWorldmapGridWidth) {
            return -1;
        }

        actualSubtileY += SUBTILE_GRID_HEIGHT;
        actualTile -= gWorldmapGridWidth;
    }

    tileInfo = &(gWorldmapTiles[actualTile]);
    subtileInfo = &(tileInfo->subtiles[actualSubtileY][actualSubtileX]);
    if (subtileState != SUBTILE_STATE_KNOWN || subtileInfo->state == SUBTILE_STATE_UNKNOWN) {
        subtileInfo->state = subtileState;
    }

    return 0;
}

// 0x4C3550
void _wmMarkSubTileRadiusVisited(int x, int y)
{
    int radius = 1;

    if (perkHasRank(gDude, PERK_SCOUT)) {
        radius = 2;
    }

    wmSubTileMarkRadiusVisited(x, y, radius);
}

// 0x4C35A8
int wmSubTileMarkRadiusVisited(int x, int y, int radius)
{
    int tile;
    int subtileX;
    int subtileY;
    int offsetX;
    int offsetY;
    SubtileInfo* subtile;

    tile = x / WM_TILE_WIDTH % gWorldmapGridWidth + y / WM_TILE_HEIGHT * gWorldmapGridWidth;
    subtileX = x % WM_TILE_WIDTH / WM_SUBTILE_SIZE;
    subtileY = y % WM_TILE_HEIGHT / WM_SUBTILE_SIZE;

    for (offsetY = -radius; offsetY <= radius; offsetY++) {
        for (offsetX = -radius; offsetX <= radius; offsetX++) {
            // NOTE: Uninline.
            wmMarkSubTileOffsetKnown(tile, subtileX, subtileY, offsetX, offsetY);
        }
    }

    subtile = &(gWorldmapTiles[tile].subtiles[subtileY][subtileX]);
    subtile->state = SUBTILE_STATE_VISITED;

    switch (subtile->field_4) {
    case 2:
        while (subtileY-- > 0) {
            // NOTE: Uninline.
            wmMarkSubTileOffsetVisited(tile, subtileX, subtileY, 0, 0);
        }
        break;
    case 4:
        while (subtileX-- >= 0) {
            // NOTE: Uninline.
            wmMarkSubTileOffsetVisited(tile, subtileX, subtileY, 0, 0);
        }

        if (tile % gWorldmapGridWidth > 0) {
            for (subtileX = 0; subtileX < SUBTILE_GRID_WIDTH; subtileX++) {
                // NOTE: Uninline.
                wmMarkSubTileOffsetVisited(tile - 1, subtileX, subtileY, 0, 0);
            }
        }
        break;
    }

    return 0;
}

// 0x4C3740
int _wmSubTileGetVisitedState(int x, int y, int* a3)
{
    TileInfo* tile;
    SubtileInfo* ptr;

    tile = &(gWorldmapTiles[y / WM_TILE_HEIGHT * gWorldmapGridWidth + x / WM_TILE_WIDTH % gWorldmapGridWidth]);
    ptr = &(tile->subtiles[y % WM_TILE_HEIGHT / WM_SUBTILE_SIZE][x % WM_TILE_WIDTH / WM_SUBTILE_SIZE]);
    *a3 = ptr->state;

    return 0;
}

// Load tile art if needed.
//
// 0x4C37EC
int _wmTileGrabArt(int tile_index)
{
    TileInfo* tile = &(gWorldmapTiles[tile_index]);
    if (tile->data != NULL) {
        return 0;
    }

    tile->data = artLockFrameData(tile->fid, 0, 0, &(tile->handle));
    if (tile->data != NULL) {
        return 0;
    }

    worldmapWindowFree();

    return -1;
}

// 0x4C3830
int worldmapWindowRefresh()
{
    if (_wmInterfaceWasInitialized != 1) {
        return 0;
    }

    int v17 = gWorldmapOffsetX % WM_TILE_WIDTH;
    int v18 = gWorldmapOffsetY % WM_TILE_HEIGHT;
    int v20 = WM_TILE_HEIGHT - v18;
    int v21 = WM_TILE_WIDTH * v18;
    int v19 = WM_TILE_WIDTH - v17;

    // Render tiles.
    int y = 0;
    int x = 0;
    int v0 = gWorldmapOffsetY / WM_TILE_HEIGHT * gWorldmapGridWidth + gWorldmapOffsetX / WM_TILE_WIDTH % gWorldmapGridWidth;
    while (y < WM_VIEW_HEIGHT) {
        x = 0;
        int v23 = 0;
        int height;
        while (x < WM_VIEW_WIDTH) {
            if (_wmTileGrabArt(v0) == -1) {
                return -1;
            }

            int width = WM_TILE_WIDTH;

            int srcX = 0;
            if (x == 0) {
                srcX = v17;
                width = v19;
            }

            if (width + x > WM_VIEW_WIDTH) {
                width = WM_VIEW_WIDTH - x;
            }

            height = WM_TILE_HEIGHT;
            if (y == 0) {
                height = v20;
                srcX += v21;
            }

            if (height + y > WM_VIEW_HEIGHT) {
                height = WM_VIEW_HEIGHT - y;
            }

            TileInfo* tileInfo = &(gWorldmapTiles[v0]);
            blitBufferToBuffer(tileInfo->data + srcX,
                width,
                height,
                WM_TILE_WIDTH,
                gWorldmapWindowBuffer + WM_WINDOW_WIDTH * (y + WM_VIEW_Y) + WM_VIEW_X + x,
                WM_WINDOW_WIDTH);
            v0++;

            x += width;
            v23++;
        }

        v0 += gWorldmapGridWidth - v23;
        y += height;
    }

    // Render cities.
    for (int index = 0; index < gCitiesLength; index++) {
        CityInfo* cityInfo = &(gCities[index]);
        if (cityInfo->state != CITY_STATE_UNKNOWN) {
            CitySizeDescription* citySizeDescription = &(gCitySizeDescriptions[cityInfo->size]);
            int cityX = cityInfo->x - gWorldmapOffsetX;
            int cityY = cityInfo->y - gWorldmapOffsetY;
            if (cityX >= 0 && cityX <= 472 - citySizeDescription->width
                && cityY >= 0 && cityY <= 465 - citySizeDescription->height) {
                wmInterfaceDrawCircleOverlay(cityInfo, citySizeDescription, gWorldmapWindowBuffer, cityX, cityY);
            }
        }
    }

    // Hide unknown subtiles, dim unvisited.
    int v25 = gWorldmapOffsetX / WM_TILE_WIDTH % gWorldmapGridWidth + gWorldmapOffsetY / WM_TILE_HEIGHT * gWorldmapGridWidth;
    int v30 = 0;
    while (v30 < WM_VIEW_HEIGHT) {
        int v24 = 0;
        int v33 = 0;
        int v29;
        while (v33 < WM_VIEW_WIDTH) {
            int v31 = WM_TILE_WIDTH;
            if (v33 == 0) {
                v31 = WM_TILE_WIDTH - v17;
            }

            if (v33 + v31 > WM_VIEW_WIDTH) {
                v31 = WM_VIEW_WIDTH - v33;
            }

            v29 = WM_TILE_HEIGHT;
            if (v30 == 0) {
                v29 -= v18;
            }

            if (v30 + v29 > WM_VIEW_HEIGHT) {
                v29 = WM_VIEW_HEIGHT - v30;
            }

            int v32;
            if (v30 != 0) {
                v32 = WM_VIEW_Y;
            } else {
                v32 = WM_VIEW_Y - v18;
            }

            int v13 = 0;
            int v34 = v30 + v32;

            for (int row = 0; row < SUBTILE_GRID_HEIGHT; row++) {
                int v35;
                if (v33 != 0) {
                    v35 = WM_VIEW_X;
                } else {
                    v35 = WM_VIEW_X - v17;
                }

                int v15 = v33 + v35;
                for (int column = 0; column < SUBTILE_GRID_WIDTH; column++) {
                    TileInfo* tileInfo = &(gWorldmapTiles[v25]);
                    worldmapWindowDimSubtile(tileInfo, column, row, v15, v34, 1);

                    v15 += WM_SUBTILE_SIZE;
                    v35 += WM_SUBTILE_SIZE;
                }

                v32 += WM_SUBTILE_SIZE;
                v34 += WM_SUBTILE_SIZE;
            }

            v25++;
            v24++;
            v33 += v31;
        }

        v25 += gWorldmapGridWidth - v24;
        v30 += v29;
    }

    _wmDrawCursorStopped();

    worldmapWindowRenderChrome(true);

    return 0;
}

// 0x4C3C9C
void worldmapWindowRenderDate(bool shouldRefreshWindow)
{
    int month;
    int day;
    int year;
    gameTimeGetDate(&month, &day, &year);

    month--;

    unsigned char* dest = gWorldmapWindowBuffer;

    int numbersFrmWidth = artGetWidth(wmGenData.numbersFrm, 0, 0);
    int numbersFrmHeight = artGetHeight(wmGenData.numbersFrm, 0, 0);
    unsigned char* numbersFrmData = artGetFrameData(wmGenData.numbersFrm, 0, 0);

    dest += WM_WINDOW_WIDTH * 12 + 487;
    blitBufferToBuffer(numbersFrmData + 9 * (day / 10), 9, numbersFrmHeight, numbersFrmWidth, dest, WM_WINDOW_WIDTH);
    blitBufferToBuffer(numbersFrmData + 9 * (day % 10), 9, numbersFrmHeight, numbersFrmWidth, dest + 9, WM_WINDOW_WIDTH);

    int monthsFrmWidth = artGetWidth(wmGenData.monthsFrm, 0, 0);
    unsigned char* monthsFrmData = artGetFrameData(wmGenData.monthsFrm, 0, 0);
    blitBufferToBuffer(monthsFrmData + monthsFrmWidth * 15 * month, 29, 14, 29, dest + WM_WINDOW_WIDTH + 26, WM_WINDOW_WIDTH);

    dest += 98;
    for (int index = 0; index < 4; index++) {
        dest -= 9;
        blitBufferToBuffer(numbersFrmData + 9 * (year % 10), 9, numbersFrmHeight, numbersFrmWidth, dest, WM_WINDOW_WIDTH);
        year /= 10;
    }

    int gameTimeHour = gameTimeGetHour();
    dest += 72;
    for (int index = 0; index < 4; index++) {
        blitBufferToBuffer(numbersFrmData + 9 * (gameTimeHour % 10), 9, numbersFrmHeight, numbersFrmWidth, dest, WM_WINDOW_WIDTH);
        dest -= 9;
        gameTimeHour /= 10;
    }

    if (shouldRefreshWindow) {
        Rect rect;
        rect.left = 487;
        rect.top = 12;
        rect.bottom = numbersFrmHeight + 12;
        rect.right = 630;
        win_draw_rect(gWorldmapWindow, &rect);
    }
}

// 0x4C3F00
int _wmMatchWorldPosToArea(int a1, int a2, int* a3)
{
    int v3 = a2 + WM_VIEW_Y;
    int v4 = a1 + WM_VIEW_X;

    int index;
    for (index = 0; index < gCitiesLength; index++) {
        CityInfo* city = &(gCities[index]);
        if (city->state) {
            if (v4 >= city->x && v3 >= city->y) {
                CitySizeDescription* citySizeDescription = &(gCitySizeDescriptions[city->size]);
                if (v4 <= city->x + citySizeDescription->width && v3 <= city->y + citySizeDescription->height) {
                    break;
                }
            }
        }
    }

    if (index == gCitiesLength) {
        *a3 = -1;
    } else {
        *a3 = index;
    }

    return 0;
}

// FIXME: This function does not set current font, which is a bit unusual for a
// function which draw text. I doubt it was done on purpose, likely simply
// forgotten. Because of this, city names are rendered with current font, which
// can be any, but in this case it uses default text font, not interface font.
//
// 0x4C3FA8
int wmInterfaceDrawCircleOverlay(CityInfo* city, CitySizeDescription* citySizeDescription, unsigned char* dest, int x, int y)
{
    MessageListItem messageListItem;
    char name[40];
    int nameY;
    int maxY;
    int width;

    _dark_translucent_trans_buf_to_buf(citySizeDescription->data,
        citySizeDescription->width,
        citySizeDescription->height,
        citySizeDescription->width,
        dest,
        x,
        y,
        WM_WINDOW_WIDTH,
        0x10000,
        _circleBlendTable,
        _commonGrayTable);

    nameY = y + citySizeDescription->height + 1;
    maxY = 464 - fontGetLineHeight();
    if (nameY < maxY) {
        if (_wmAreaIsKnown(city->areaId)) {
            // NOTE: Uninline.
            wmGetAreaName(city, name);
        } else {
            strncpy(name, getmsg(&gWorldmapMessageList, &messageListItem, 1004), 40);
        }

        width = fontGetStringWidth(name);
        fontDrawText(dest + WM_WINDOW_WIDTH * nameY + x + citySizeDescription->width / 2 - width / 2,
            name,
            width,
            WM_WINDOW_WIDTH,
            _colorTable[992]);
    }

    return 0;
}

// Helper function that dims specified rectangle in given buffer. It's used to
// slightly darken subtile which is known, but not visited.
//
// 0x4C40A8
void worldmapWindowDimRect(unsigned char* dest, int width, int height, int pitch)
{
    int skipY = pitch - width;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            unsigned char byte = *dest;
            unsigned int index = (byte << 8) + 75;
            *dest++ = _intensityColorTable[index];
        }
        dest += skipY;
    }
}

// 0x4C40E4
int worldmapWindowDimSubtile(TileInfo* tileInfo, int column, int row, int x, int y, int a6)
{
    SubtileInfo* subtileInfo = &(tileInfo->subtiles[row][column]);

    int destY = y;
    int destX = x;

    int height = WM_SUBTILE_SIZE;
    if (y < WM_VIEW_Y) {
        if (y < 0) {
            height = y + 29;
        } else {
            height = WM_SUBTILE_SIZE - (WM_VIEW_Y - y);
        }
        destY = WM_VIEW_Y;
    }

    if (height + y > 464) {
        height -= height + y - 464;
    }

    int width = WM_SUBTILE_SIZE * a6;
    if (x < WM_VIEW_X) {
        destX = WM_VIEW_X;
        width -= WM_VIEW_X - x;
    }

    if (width + x > 472) {
        width -= width + x - 472;
    }

    if (width > 0 && height > 0) {
        unsigned char* dest = gWorldmapWindowBuffer + WM_WINDOW_WIDTH * destY + destX;
        switch (subtileInfo->state) {
        case SUBTILE_STATE_UNKNOWN:
            bufferFill(dest, width, height, WM_WINDOW_WIDTH, _colorTable[0]);
            break;
        case SUBTILE_STATE_KNOWN:
            worldmapWindowDimRect(dest, width, height, WM_WINDOW_WIDTH);
            break;
        }
    }

    return 0;
}

// 0x4C41EC
int _wmDrawCursorStopped()
{
    unsigned char* src;
    int width;
    int height;

    if (wmGenData.walkDestinationX >= 1 || wmGenData.walkDestinationY >= 1) {

        if (wmGenData.encounterIconIsVisible == 1) {
            src = wmGenData.encounterCursorFrmData[wmGenData.encounterCursorId];
            width = wmGenData.encounterCursorFrmWidths[wmGenData.encounterCursorId];
            height = wmGenData.encounterCursorFrmHeights[wmGenData.encounterCursorId];
        } else {
            src = wmGenData.locationMarkerFrmData;
            width = wmGenData.locationMarkerFrmWidth;
            height = wmGenData.locationMarkerFrmHeight;
        }

        if (wmGenData.worldPosX >= gWorldmapOffsetX && wmGenData.worldPosX < gWorldmapOffsetX + WM_VIEW_WIDTH
            && wmGenData.worldPosY >= gWorldmapOffsetY && wmGenData.worldPosY < gWorldmapOffsetY + WM_VIEW_HEIGHT) {
            blitBufferToBufferTrans(src, width, height, width, gWorldmapWindowBuffer + WM_WINDOW_WIDTH * (WM_VIEW_Y - gWorldmapOffsetY + wmGenData.worldPosY - height / 2) + WM_VIEW_X - gWorldmapOffsetX + wmGenData.worldPosX - width / 2, WM_WINDOW_WIDTH);
        }

        if (wmGenData.walkDestinationX >= gWorldmapOffsetX && wmGenData.walkDestinationX < gWorldmapOffsetX + WM_VIEW_WIDTH
            && wmGenData.walkDestinationY >= gWorldmapOffsetY && wmGenData.walkDestinationY < gWorldmapOffsetY + WM_VIEW_HEIGHT) {
            blitBufferToBufferTrans(wmGenData.destinationMarkerFrmData, wmGenData.destinationMarkerFrmWidth, wmGenData.destinationMarkerFrmHeight, wmGenData.destinationMarkerFrmWidth, gWorldmapWindowBuffer + WM_WINDOW_WIDTH * (WM_VIEW_Y - gWorldmapOffsetY + wmGenData.walkDestinationY - wmGenData.destinationMarkerFrmHeight / 2) + WM_VIEW_X - gWorldmapOffsetX + wmGenData.walkDestinationX - wmGenData.destinationMarkerFrmWidth / 2, WM_WINDOW_WIDTH);
        }
    } else {
        if (wmGenData.encounterIconIsVisible == 1) {
            src = wmGenData.encounterCursorFrmData[wmGenData.encounterCursorId];
            width = wmGenData.encounterCursorFrmWidths[wmGenData.encounterCursorId];
            height = wmGenData.encounterCursorFrmHeights[wmGenData.encounterCursorId];
        } else {
            src = wmGenData.mousePressed ? wmGenData.hotspotPressedFrmData : wmGenData.hotspotNormalFrmData;
            width = wmGenData.hotspotFrmWidth;
            height = wmGenData.hotspotFrmHeight;
        }

        if (wmGenData.worldPosX >= gWorldmapOffsetX && wmGenData.worldPosX < gWorldmapOffsetX + WM_VIEW_WIDTH
            && wmGenData.worldPosY >= gWorldmapOffsetY && wmGenData.worldPosY < gWorldmapOffsetY + WM_VIEW_HEIGHT) {
            blitBufferToBufferTrans(src, width, height, width, gWorldmapWindowBuffer + WM_WINDOW_WIDTH * (WM_VIEW_Y - gWorldmapOffsetY + wmGenData.worldPosY - height / 2) + WM_VIEW_X - gWorldmapOffsetX + wmGenData.worldPosX - width / 2, WM_WINDOW_WIDTH);
        }
    }

    return 0;
}

// 0x4C4490
bool _wmCursorIsVisible()
{
    return wmGenData.worldPosX >= gWorldmapOffsetX
        && wmGenData.worldPosY >= gWorldmapOffsetY
        && wmGenData.worldPosX < gWorldmapOffsetX + WM_VIEW_WIDTH
        && wmGenData.worldPosY < gWorldmapOffsetY + WM_VIEW_HEIGHT;
}

// NOTE: Inlined.
//
// 0x4C44D8
int wmGetAreaName(CityInfo* city, char* name)
{
    MessageListItem messageListItem;

    getmsg(&gMapMessageList, &messageListItem, city->areaId + 1500);
    strncpy(name, messageListItem.text, 40);

    return 0;
}

// Copy city short name.
//
// 0x4C450C
int _wmGetAreaIdxName(int index, char* name)
{
    MessageListItem messageListItem;

    getmsg(&gMapMessageList, &messageListItem, 1500 + index);
    strncpy(name, messageListItem.text, 40);

    return 0;
}

// Returns true if world area is known.
//
// 0x4C453C
bool _wmAreaIsKnown(int cityIndex)
{
    if (!cityIsValid(cityIndex)) {
        return false;
    }

    CityInfo* city = &(gCities[cityIndex]);
    if (city->visitedState) {
        if (city->state == CITY_STATE_KNOWN) {
            return true;
        }
    }

    return false;
}

// 0x4C457C
int _wmAreaVisitedState(int area)
{
    if (!cityIsValid(area)) {
        return 0;
    }

    CityInfo* city = &(gCities[area]);
    if (city->visitedState && city->state == CITY_STATE_KNOWN) {
        return city->visitedState;
    }

    return 0;
}

// 0x4C45BC
bool _wmMapIsKnown(int mapIndex)
{
    int cityIndex;
    if (_wmMatchAreaFromMap(mapIndex, &cityIndex) != 0) {
        return false;
    }

    int entranceIndex;
    if (_wmMatchEntranceFromMap(cityIndex, mapIndex, &entranceIndex) != 0) {
        return false;
    }

    CityInfo* city = &(gCities[cityIndex]);
    EntranceInfo* entrance = &(city->entrances[entranceIndex]);

    if (entrance->state != 1) {
        return false;
    }

    return true;
}

// 0x4C4624
int wmAreaMarkVisited(int cityIndex)
{
    return _wmAreaMarkVisitedState(cityIndex, CITY_STATE_VISITED);
}

// 0x4C4634
bool _wmAreaMarkVisitedState(int cityIndex, int a2)
{
    if (!cityIsValid(cityIndex)) {
        return false;
    }

    CityInfo* city = &(gCities[cityIndex]);
    int v5 = city->visitedState;
    if (city->state == CITY_STATE_KNOWN && a2 != 0) {
        _wmMarkSubTileRadiusVisited(city->x, city->y);
    }

    city->visitedState = a2;

    SubtileInfo* subtile;
    if (_wmFindCurSubTileFromPos(city->x, city->y, &subtile) == -1) {
        return false;
    }

    if (a2 == 1) {
        subtile->state = SUBTILE_STATE_KNOWN;
    } else if (a2 == 2 && v5 == 0) {
        city->visitedState = 1;
    }

    return true;
}

// 0x4C46CC
bool _wmAreaSetVisibleState(int cityIndex, int state, int forceSet)
{
    if (!cityIsValid(cityIndex)) {
        return false;
    }

    CityInfo* city = &(gCities[cityIndex]);
    if (city->lockState != LOCK_STATE_LOCKED || forceSet) {
        city->state = state;
        return true;
    }

    return false;
}

// wm_area_set_pos
// 0x4C4710
int worldmapCitySetPos(int cityIndex, int x, int y)
{
    if (!cityIsValid(cityIndex)) {
        return -1;
    }

    if (x < 0 || x >= WM_TILE_WIDTH * gWorldmapGridWidth) {
        return -1;
    }

    if (y < 0 || y >= WM_TILE_HEIGHT * (gWorldmapTilesLength / gWorldmapGridWidth)) {
        return -1;
    }

    CityInfo* city = &(gCities[cityIndex]);
    city->x = x;
    city->y = y;

    return 0;
}

// Returns current town x/y.
//
// 0x4C47A4
int _wmGetPartyWorldPos(int* out_x, int* out_y)
{
    if (out_x != NULL) {
        *out_x = wmGenData.worldPosX;
    }

    if (out_y != NULL) {
        *out_y = wmGenData.worldPosY;
    }

    return 0;
}

// Returns current town.
//
// 0x4C47C0
int _wmGetPartyCurArea(int* a1)
{
    if (a1) {
        *a1 = wmGenData.currentAreaId;
        return 0;
    }

    return -1;
}

// 0x4C47D8
void _wmMarkAllSubTiles(int a1)
{
    for (int tileIndex = 0; tileIndex < gWorldmapTilesLength; tileIndex++) {
        TileInfo* tile = &(gWorldmapTiles[tileIndex]);
        for (int column = 0; column < SUBTILE_GRID_HEIGHT; column++) {
            for (int row = 0; row < SUBTILE_GRID_WIDTH; row++) {
                SubtileInfo* subtile = &(tile->subtiles[column][row]);
                subtile->state = a1;
            }
        }
    }
}

// 0x4C4850
void _wmTownMap()
{
    _wmWorldMapFunc(1);
}

// 0x4C485C
int worldmapCityMapViewSelect(int* mapIndexPtr)
{
    *mapIndexPtr = -1;

    if (worldmapCityMapViewInit() == -1) {
        worldmapCityMapViewFree();
        return -1;
    }

    if (wmGenData.currentAreaId == -1) {
        return -1;
    }

    CityInfo* city = &(gCities[wmGenData.currentAreaId]);

    for (;;) {
        int keyCode = _get_input();
        if (keyCode == KEY_CTRL_Q || keyCode == KEY_CTRL_X || keyCode == KEY_F10) {
            showQuitConfirmationDialog();
        }

        if (_game_user_wants_to_quit) {
            break;
        }

        if (keyCode != -1) {
            if (keyCode == KEY_ESCAPE) {
                break;
            }

            if (keyCode >= KEY_1 && keyCode < KEY_1 + city->entrancesLength) {
                EntranceInfo* entrance = &(city->entrances[keyCode - KEY_1]);

                *mapIndexPtr = entrance->map;

                mapSetEnteringLocation(entrance->elevation, entrance->tile, entrance->rotation);

                break;
            }

            if (keyCode >= KEY_CTRL_F1 && keyCode <= KEY_CTRL_F7) {
                int quickDestinationIndex = wmGenData.tabsOffsetY / 27 + keyCode - KEY_CTRL_F1;
                if (quickDestinationIndex < gQuickDestinationsLength) {
                    int cityIndex = gQuickDestinations[quickDestinationIndex];
                    CityInfo* city = &(gCities[cityIndex]);
                    if (!_wmAreaIsKnown(city->areaId)) {
                        break;
                    }

                    if (cityIndex != wmGenData.currentAreaId) {
                        _wmPartyInitWalking(city->x, city->y);

                        wmGenData.mousePressed = false;

                        break;
                    }
                }
            } else {
                if (keyCode == KEY_CTRL_ARROW_UP) {
                    _wmInterfaceScrollTabsStart(-27);
                } else if (keyCode == KEY_CTRL_ARROW_DOWN) {
                    _wmInterfaceScrollTabsStart(27);
                } else if (keyCode == 2069) {
                    if (worldmapCityMapViewRefresh() == -1) {
                        return -1;
                    }
                }

                if (keyCode == KEY_UPPERCASE_T || keyCode == KEY_LOWERCASE_T || keyCode == KEY_UPPERCASE_W || keyCode == KEY_LOWERCASE_W) {
                    keyCode = KEY_ESCAPE;
                }

                if (keyCode == KEY_ESCAPE) {
                    break;
                }
            }
        }
    }

    if (worldmapCityMapViewFree() == -1) {
        return -1;
    }

    return 0;
}

// 0x4C4A6C
int worldmapCityMapViewInit()
{
    _wmTownMapCurArea = wmGenData.currentAreaId;

    CityInfo* city = &(gCities[wmGenData.currentAreaId]);

    Art* mapFrm = artLock(city->mapFid, &gWorldmapCityMapFrmHandle);
    if (mapFrm == NULL) {
        return -1;
    }

    gWorldmapCityMapFrmWidth = artGetWidth(mapFrm, 0, 0);
    gWorldmapCityMapFrmHeight = artGetHeight(mapFrm, 0, 0);

    artUnlock(gWorldmapCityMapFrmHandle);
    gWorldmapCityMapFrmHandle = INVALID_CACHE_ENTRY;

    gWorldmapCityMapFrmData = artLockFrameData(city->mapFid, 0, 0, &gWorldmapCityMapFrmHandle);
    if (gWorldmapCityMapFrmData == NULL) {
        return -1;
    }

    for (int index = 0; index < city->entrancesLength; index++) {
        _wmTownMapButtonId[index] = -1;
    }

    for (int index = 0; index < city->entrancesLength; index++) {
        EntranceInfo* entrance = &(city->entrances[index]);
        if (entrance->state == 0) {
            continue;
        }

        if (entrance->x == -1 || entrance->y == -1) {
            continue;
        }

        _wmTownMapButtonId[index] = buttonCreate(gWorldmapWindow,
            entrance->x,
            entrance->y,
            wmGenData.hotspotFrmWidth,
            wmGenData.hotspotFrmHeight,
            -1,
            2069,
            -1,
            KEY_1 + index,
            wmGenData.hotspotNormalFrmData,
            wmGenData.hotspotPressedFrmData,
            NULL,
            BUTTON_FLAG_TRANSPARENT);

        if (_wmTownMapButtonId[index] == -1) {
            return -1;
        }
    }

    tickersRemove(worldmapWindowHandleMouseScrolling);

    if (worldmapCityMapViewRefresh() == -1) {
        return -1;
    }

    return 0;
}

// 0x4C4BD0
int worldmapCityMapViewRefresh()
{
    blitBufferToBuffer(gWorldmapCityMapFrmData,
        gWorldmapCityMapFrmWidth,
        gWorldmapCityMapFrmHeight,
        gWorldmapCityMapFrmWidth,
        gWorldmapWindowBuffer + WM_WINDOW_WIDTH * WM_VIEW_Y + WM_VIEW_X,
        WM_WINDOW_WIDTH);

    worldmapWindowRenderChrome(false);

    CityInfo* city = &(gCities[wmGenData.currentAreaId]);

    for (int index = 0; index < city->entrancesLength; index++) {
        EntranceInfo* entrance = &(city->entrances[index]);
        if (entrance->state == 0) {
            continue;
        }

        if (entrance->x == -1 || entrance->y == -1) {
            continue;
        }

        MessageListItem messageListItem;
        messageListItem.num = 200 + 10 * _wmTownMapCurArea + index;
        if (messageListGetItem(&gWorldmapMessageList, &messageListItem)) {
            if (messageListItem.text != NULL) {
                int width = fontGetStringWidth(messageListItem.text);
                windowDrawText(gWorldmapWindow, messageListItem.text, width, wmGenData.hotspotFrmWidth / 2 + entrance->x - width / 2, wmGenData.hotspotFrmHeight + entrance->y + 2, _colorTable[992] | 0x2010000);
            }
        }
    }

    win_draw(gWorldmapWindow);

    return 0;
}

// 0x4C4D00
int worldmapCityMapViewFree()
{
    if (gWorldmapCityMapFrmHandle != INVALID_CACHE_ENTRY) {
        artUnlock(gWorldmapCityMapFrmHandle);
        gWorldmapCityMapFrmHandle = INVALID_CACHE_ENTRY;
        gWorldmapCityMapFrmData = NULL;
        gWorldmapCityMapFrmWidth = 0;
        gWorldmapCityMapFrmHeight = 0;
    }

    if (_wmTownMapCurArea != -1) {
        CityInfo* city = &(gCities[_wmTownMapCurArea]);
        for (int index = 0; index < city->entrancesLength; index++) {
            if (_wmTownMapButtonId[index] != -1) {
                buttonDestroy(_wmTownMapButtonId[index]);
                _wmTownMapButtonId[index] = -1;
            }
        }
    }

    if (worldmapWindowRefresh() == -1) {
        return -1;
    }

    tickersAdd(worldmapWindowHandleMouseScrolling);

    return 0;
}

// 0x4C4DA4
int carConsumeFuel(int amount)
{
    if (gameGetGlobalVar(GVAR_NEW_RENO_SUPER_CAR) != 0) {
        amount -= amount * 90 / 100;
    }

    if (gameGetGlobalVar(GVAR_NEW_RENO_CAR_UPGRADE) != 0) {
        amount -= amount * 10 / 100;
    }

    if (gameGetGlobalVar(GVAR_CAR_UPGRADE_FUEL_CELL_REGULATOR) != 0) {
        amount /= 2;
    }

    wmGenData.carFuel -= amount;

    if (wmGenData.carFuel < 0) {
        wmGenData.carFuel = 0;
    }

    return 0;
}

// Returns amount of fuel that does not fit into tank.
//
// 0x4C4E34
int carAddFuel(int amount)
{
    if ((amount + wmGenData.carFuel) <= CAR_FUEL_MAX) {
        wmGenData.carFuel += amount;
        return 0;
    }

    int remaining = CAR_FUEL_MAX - wmGenData.carFuel;

    wmGenData.carFuel = CAR_FUEL_MAX;

    return remaining;
}

// 0x4C4E74
int carGetFuel()
{
    return wmGenData.carFuel;
}

// 0x4C4E7C
bool carIsEmpty()
{
    return wmGenData.carFuel <= 0;
}

// 0x4C4E8C
int carGetCity()
{
    return wmGenData.currentCarAreaId;
}

// 0x4C4E94
int _wmCarGiveToParty()
{
    MessageListItem messageListItem;
    static_assert(sizeof(messageListItem) == sizeof(gWorldmapMessageListItem), "wrong size");
    memcpy(&messageListItem, &gWorldmapMessageListItem, sizeof(MessageListItem));

    if (wmGenData.carFuel <= 0) {
        // The car is out of power.
        char* msg = getmsg(&gWorldmapMessageList, &messageListItem, 1502);
        displayMonitorAddMessage(msg);
        return -1;
    }

    wmGenData.isInCar = true;

    MapTransition transition;
    memset(&transition, 0, sizeof(transition));

    transition.map = -2;
    mapSetTransition(&transition);

    CityInfo* city = &(gCities[CITY_CAR_OUT_OF_GAS]);
    city->state = CITY_STATE_UNKNOWN;
    city->visitedState = 0;

    return 0;
}

// 0x4C4F28
int ambientSoundEffectGetLength()
{
    int mapIndex = mapGetCurrentMap();
    if (mapIndex < 0 || mapIndex >= gMapsLength) {
        return -1;
    }

    MapInfo* map = &(gMaps[mapIndex]);
    return map->ambientSoundEffectsLength;
}

// 0x4C4F5C
int ambientSoundEffectGetRandom()
{
    int mapIndex = mapGetCurrentMap();
    if (mapIndex < 0 || mapIndex >= gMapsLength) {
        return -1;
    }

    MapInfo* map = &(gMaps[mapIndex]);

    int totalChances = 0;
    for (int index = 0; index < map->ambientSoundEffectsLength; index++) {
        MapAmbientSoundEffectInfo* sfx = &(map->ambientSoundEffects[index]);
        totalChances += sfx->chance;
    }

    int chance = randomBetween(0, totalChances);
    for (int index = 0; index < map->ambientSoundEffectsLength; index++) {
        MapAmbientSoundEffectInfo* sfx = &(map->ambientSoundEffects[index]);
        if (chance >= sfx->chance) {
            chance -= sfx->chance;
            continue;
        }

        return index;
    }

    return -1;
}

// 0x4C5004
int ambientSoundEffectGetName(int ambientSoundEffectIndex, char** namePtr)
{
    if (namePtr == NULL) {
        return -1;
    }

    *namePtr = NULL;

    int mapIndex = mapGetCurrentMap();
    if (mapIndex < 0 || mapIndex >= gMapsLength) {
        return -1;
    }

    MapInfo* map = &(gMaps[mapIndex]);
    if (ambientSoundEffectIndex < 0 || ambientSoundEffectIndex >= map->ambientSoundEffectsLength) {
        return -1;
    }

    MapAmbientSoundEffectInfo* ambientSoundEffectInfo = &(map->ambientSoundEffects[ambientSoundEffectIndex]);
    *namePtr = ambientSoundEffectInfo->name;

    int v1 = 0;
    if (strcmp(ambientSoundEffectInfo->name, "brdchir1") == 0) {
        v1 = 1;
    } else if (strcmp(ambientSoundEffectInfo->name, "brdchirp") == 0) {
        v1 = 2;
    }

    if (v1 != 0) {
        int dayPart;

        int gameTimeHour = gameTimeGetHour();
        if (gameTimeHour <= 600 || gameTimeHour >= 1800) {
            dayPart = DAY_PART_NIGHT;
        } else if (gameTimeHour >= 1200) {
            dayPart = DAY_PART_AFTERNOON;
        } else {
            dayPart = DAY_PART_MORNING;
        }

        if (dayPart == DAY_PART_NIGHT) {
            *namePtr = _wmRemapSfxList[v1 - 1];
        }
    }

    return 0;
}

// 0x4C50F4
int worldmapWindowRenderChrome(bool shouldRefreshWindow)
{
    blitBufferToBufferTrans(gWorldmapBoxFrmData,
        gWorldmapBoxFrmWidth,
        gWorldmapBoxFrmHeight,
        gWorldmapBoxFrmWidth,
        gWorldmapWindowBuffer,
        WM_WINDOW_WIDTH);

    worldmapRenderQuickDestinations();

    // NOTE: Uninline.
    wmInterfaceDialSyncTime(false);

    worldmapWindowRenderDial(false);

    if (wmGenData.isInCar) {
        unsigned char* data = artGetFrameData(wmGenData.carImageFrm, wmGenData.carImageCurrentFrameIndex, 0);
        if (data == NULL) {
            return -1;
        }

        blitBufferToBuffer(data,
            wmGenData.carImageFrmWidth,
            wmGenData.carImageFrmHeight,
            wmGenData.carImageFrmWidth,
            gWorldmapWindowBuffer + WM_WINDOW_WIDTH * WM_WINDOW_CAR_Y + WM_WINDOW_CAR_X,
            WM_WINDOW_WIDTH);

        blitBufferToBufferTrans(wmGenData.carImageOverlayFrmData,
            wmGenData.carImageOverlayFrmWidth,
            wmGenData.carImageOverlayFrmHeight,
            wmGenData.carImageOverlayFrmWidth,
            gWorldmapWindowBuffer + WM_WINDOW_WIDTH * WM_WINDOW_CAR_OVERLAY_Y + WM_WINDOW_CAR_OVERLAY_X,
            WM_WINDOW_WIDTH);

        worldmapWindowRenderCarFuelBar();
    } else {
        blitBufferToBufferTrans(wmGenData.globeOverlayFrmData,
            wmGenData.globeOverlayFrmWidth,
            wmGenData.globeOverlayFrmHeight,
            wmGenData.globeOverlayFrmWidth,
            gWorldmapWindowBuffer + WM_WINDOW_WIDTH * WM_WINDOW_GLOBE_OVERLAY_Y + WM_WINDOW_GLOBE_OVERLAY_X,
            WM_WINDOW_WIDTH);
    }

    worldmapWindowRenderDate(false);

    if (shouldRefreshWindow) {
        win_draw(gWorldmapWindow);
    }

    return 0;
}

// 0x4C5244
void worldmapWindowRenderCarFuelBar()
{
    int ratio = (WM_WINDOW_CAR_FUEL_BAR_HEIGHT * wmGenData.carFuel) / CAR_FUEL_MAX;
    if ((ratio & 1) != 0) {
        ratio -= 1;
    }

    unsigned char* dest = gWorldmapWindowBuffer + WM_WINDOW_WIDTH * WM_WINDOW_CAR_FUEL_BAR_Y + WM_WINDOW_CAR_FUEL_BAR_X;

    for (int index = WM_WINDOW_CAR_FUEL_BAR_HEIGHT; index > ratio; index--) {
        *dest = 14;
        dest += 640;
    }

    while (ratio > 0) {
        *dest = 196;
        dest += WM_WINDOW_WIDTH;

        *dest = 14;
        dest += WM_WINDOW_WIDTH;

        ratio -= 2;
    }
}

// 0x4C52B0
int worldmapRenderQuickDestinations()
{
    unsigned char* v30;
    unsigned char* v0;
    int v31;
    CityInfo* city;
    Art* art;
    CacheEntry* cache_entry;
    int width;
    int height;
    unsigned char* buf;
    int v10;
    unsigned char* v11;
    unsigned char* v12;
    int v32;
    unsigned char* v13;

    blitBufferToBufferTrans(wmGenData.tabsBackgroundFrmData + wmGenData.tabsBackgroundFrmWidth * wmGenData.tabsOffsetY + 9, 119, 178, wmGenData.tabsBackgroundFrmWidth, gWorldmapWindowBuffer + WM_WINDOW_WIDTH * 135 + 501, WM_WINDOW_WIDTH);

    v30 = gWorldmapWindowBuffer + WM_WINDOW_WIDTH * 138 + 530;
    v0 = gWorldmapWindowBuffer + WM_WINDOW_WIDTH * 138 + 530 - WM_WINDOW_WIDTH * (wmGenData.tabsOffsetY % 27);
    v31 = wmGenData.tabsOffsetY / 27;

    if (v31 < gQuickDestinationsLength) {
        city = &(gCities[gQuickDestinations[v31]]);
        if (city->labelFid != -1) {
            art = artLock(city->labelFid, &cache_entry);
            if (art == NULL) {
                return -1;
            }

            width = artGetWidth(art, 0, 0);
            height = artGetHeight(art, 0, 0);
            buf = artGetFrameData(art, 0, 0);
            if (buf == NULL) {
                return -1;
            }

            v10 = height - wmGenData.tabsOffsetY % 27;
            v11 = buf + width * (wmGenData.tabsOffsetY % 27);

            v12 = v0;
            if (v0 < v30 - WM_WINDOW_WIDTH) {
                v12 = v30 - WM_WINDOW_WIDTH;
            }

            blitBufferToBuffer(v11, width, v10, width, v12, WM_WINDOW_WIDTH);
            artUnlock(cache_entry);
            cache_entry = INVALID_CACHE_ENTRY;
        }
    }

    v13 = v0 + WM_WINDOW_WIDTH * 27;
    v32 = v31 + 6;

    for (int v14 = v31 + 1; v14 < v32; v14++) {
        if (v14 < gQuickDestinationsLength) {
            city = &(gCities[gQuickDestinations[v14]]);
            if (city->labelFid != -1) {
                art = artLock(city->labelFid, &cache_entry);
                if (art == NULL) {
                    return -1;
                }

                width = artGetWidth(art, 0, 0);
                height = artGetHeight(art, 0, 0);
                buf = artGetFrameData(art, 0, 0);
                if (buf == NULL) {
                    return -1;
                }

                blitBufferToBuffer(buf, width, height, width, v13, WM_WINDOW_WIDTH);
                artUnlock(cache_entry);

                cache_entry = INVALID_CACHE_ENTRY;
            }
        }
        v13 += WM_WINDOW_WIDTH * 27;
    }

    if (v31 + 6 < gQuickDestinationsLength) {
        city = &(gCities[gQuickDestinations[v31 + 6]]);
        if (city->labelFid != -1) {
            art = artLock(city->labelFid, &cache_entry);
            if (art == NULL) {
                return -1;
            }

            width = artGetWidth(art, 0, 0);
            height = artGetHeight(art, 0, 0);
            buf = artGetFrameData(art, 0, 0);
            if (buf == NULL) {
                return -1;
            }

            blitBufferToBuffer(buf, width, height, width, v13, WM_WINDOW_WIDTH);
            artUnlock(cache_entry);

            cache_entry = INVALID_CACHE_ENTRY;
        }
    }

    blitBufferToBufferTrans(wmGenData.tabsBorderFrmData, 119, 178, 119, gWorldmapWindowBuffer + WM_WINDOW_WIDTH * 135 + 501, WM_WINDOW_WIDTH);

    return 0;
}

// Creates array of cities available as quick destinations.
//
// 0x4C55D4
int _wmMakeTabsLabelList(int** quickDestinationsPtr, int* quickDestinationsLengthPtr)
{
    int* quickDestinations = *quickDestinationsPtr;

    // NOTE: Uninline.
    wmFreeTabsLabelList(quickDestinationsPtr, quickDestinationsLengthPtr);

    int capacity = 10;

    quickDestinations = (int*)internal_malloc(sizeof(*quickDestinations) * capacity);
    *quickDestinationsPtr = quickDestinations;

    if (quickDestinations == NULL) {
        return -1;
    }

    int quickDestinationsLength = *quickDestinationsLengthPtr;
    for (int index = 0; index < gCitiesLength; index++) {
        if (_wmAreaIsKnown(index) && gCities[index].labelFid != -1) {
            quickDestinationsLength++;
            *quickDestinationsLengthPtr = quickDestinationsLength;

            if (capacity <= quickDestinationsLength) {
                capacity += 10;

                quickDestinations = (int*)internal_realloc(quickDestinations, sizeof(*quickDestinations) * capacity);
                if (quickDestinations == NULL) {
                    return -1;
                }

                *quickDestinationsPtr = quickDestinations;
            }

            quickDestinations[quickDestinationsLength - 1] = index;
        }
    }

    qsort(quickDestinations, quickDestinationsLength, sizeof(*quickDestinations), worldmapCompareCitiesByName);

    return 0;
}

// 0x4C56C8
int worldmapCompareCitiesByName(const void* a1, const void* a2)
{
    int v1 = *(int*)a1;
    int v2 = *(int*)a2;

    CityInfo* city1 = &(gCities[v1]);
    CityInfo* city2 = &(gCities[v2]);

    return stricmp(city1->name, city2->name);
}

// NOTE: Inlined.
//
// 0x4C5710
int wmFreeTabsLabelList(int** quickDestinationsListPtr, int* quickDestinationsLengthPtr)
{
    if (*quickDestinationsListPtr != NULL) {
        internal_free(*quickDestinationsListPtr);
        *quickDestinationsListPtr = NULL;
    }

    *quickDestinationsLengthPtr = 0;

    return 0;
}

// 0x4C5734
void worldmapWindowRenderDial(bool shouldRefreshWindow)
{
    unsigned char* data = artGetFrameData(wmGenData.dialFrm, wmGenData.dialFrmCurrentFrameIndex, 0);
    blitBufferToBufferTrans(data,
        wmGenData.dialFrmWidth,
        wmGenData.dialFrmHeight,
        wmGenData.dialFrmWidth,
        gWorldmapWindowBuffer + WM_WINDOW_WIDTH * WM_WINDOW_DIAL_Y + WM_WINDOW_DIAL_X,
        WM_WINDOW_WIDTH);

    if (shouldRefreshWindow) {
        Rect rect;
        rect.left = WM_WINDOW_DIAL_X;
        rect.top = WM_WINDOW_DIAL_Y - 1;
        rect.right = rect.left + wmGenData.dialFrmWidth;
        rect.bottom = rect.top + wmGenData.dialFrmHeight;
        win_draw_rect(gWorldmapWindow, &rect);
    }
}

// NOTE: Inlined.
//
// 0x4C57BC
void wmInterfaceDialSyncTime(bool shouldRefreshWindow)
{
    int gameHour;
    int frame;

    gameHour = gameTimeGetHour();
    frame = (gameHour / 100 + 12) % artGetFrameCount(wmGenData.dialFrm);
    if (frame != wmGenData.dialFrmCurrentFrameIndex) {
        wmGenData.dialFrmCurrentFrameIndex = frame;
        worldmapWindowRenderDial(shouldRefreshWindow);
    }
}

// 0x4C5804
int _wmAreaFindFirstValidMap(int* out_a1)
{
    *out_a1 = -1;

    if (wmGenData.currentAreaId == -1) {
        return -1;
    }

    CityInfo* city = &(gCities[wmGenData.currentAreaId]);
    if (city->entrancesLength == 0) {
        return -1;
    }

    for (int index = 0; index < city->entrancesLength; index++) {
        EntranceInfo* entrance = &(city->entrances[index]);
        if (entrance->state != 0) {
            *out_a1 = entrance->map;
            return 0;
        }
    }

    EntranceInfo* entrance = &(city->entrances[0]);
    entrance->state = 1;

    *out_a1 = entrance->map;
    return 0;
}

// 0x4C58C0
int worldmapStartMapMusic()
{
    do {
        int mapIndex = mapGetCurrentMap();
        if (mapIndex == -1 || mapIndex >= gMapsLength) {
            break;
        }

        MapInfo* map = &(gMaps[mapIndex]);
        if (strlen(map->music) == 0) {
            break;
        }

        if (_gsound_background_play_level_music(map->music, 12) == -1) {
            break;
        }

        return 0;
    } while (0);

    debugPrint("\nWorldMap Error: Couldn't start map Music!");

    return -1;
}

// wmSetMapMusic
// 0x4C5928
int worldmapSetMapMusic(int mapIndex, const char* name)
{
    if (mapIndex == -1 || mapIndex >= gMapsLength) {
        return -1;
    }

    if (name == NULL) {
        return -1;
    }

    debugPrint("\nwmSetMapMusic: %d, %s", mapIndex, name);

    MapInfo* map = &(gMaps[mapIndex]);

    strncpy(map->music, name, 40);
    map->music[39] = '\0';

    if (mapGetCurrentMap() == mapIndex) {
        backgroundSoundDelete();
        worldmapStartMapMusic();
    }

    return 0;
}

// 0x4C59A4
int _wmMatchAreaContainingMapIdx(int mapIndex, int* cityIndexPtr)
{
    *cityIndexPtr = 0;

    for (int cityIndex = 0; cityIndex < gCitiesLength; cityIndex++) {
        CityInfo* cityInfo = &(gCities[cityIndex]);
        for (int entranceIndex = 0; entranceIndex < cityInfo->entrancesLength; entranceIndex++) {
            EntranceInfo* entranceInfo = &(cityInfo->entrances[entranceIndex]);
            if (entranceInfo->map == mapIndex) {
                *cityIndexPtr = cityIndex;
                return 0;
            }
        }
    }

    return -1;
}

// 0x4C5A1C
int _wmTeleportToArea(int cityIndex)
{
    if (!cityIsValid(cityIndex)) {
        return -1;
    }

    wmGenData.currentAreaId = cityIndex;
    wmGenData.walkDestinationX = 0;
    wmGenData.walkDestinationY = 0;
    wmGenData.isWalking = false;

    CityInfo* city = &(gCities[cityIndex]);
    wmGenData.worldPosX = city->x;
    wmGenData.worldPosY = city->y;

    return 0;
}
