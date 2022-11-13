#include "game/automap.h"

#include <stdio.h>
#include <string.h>

#include "color.h"
#include "game/config.h"
#include "core.h"
#include "game/bmpdlog.h"
#include "debug.h"
#include "draw.h"
#include "game/game.h"
#include "game/gconfig.h"
#include "game/gmouse.h"
#include "game/gsound.h"
#include "game/graphlib.h"
#include "item.h"
#include "map.h"
#include "memory.h"
#include "object.h"
#include "text_font.h"
#include "window_manager.h"

#define AUTOMAP_OFFSET_COUNT (AUTOMAP_MAP_COUNT * ELEVATION_COUNT)

#define AUTOMAP_WINDOW_X 75
#define AUTOMAP_WINDOW_Y 0
#define AUTOMAP_WINDOW_WIDTH 519
#define AUTOMAP_WINDOW_HEIGHT 480

#define AUTOMAP_PIPBOY_VIEW_X 238
#define AUTOMAP_PIPBOY_VIEW_Y 105

// View options for rendering automap for map window. These are stored in
// [autoflags] and is saved in save game file.
typedef enum AutomapFlags {
    // NOTE: This is a special flag to denote the map is activated in the game (as
    // opposed to the mapper). It's always on. Turning it off produces nice color
    // coded map with all objects and their types visible, however there is no way
    // you can do it within the game UI.
    AUTOMAP_IN_GAME = 0x01,

    // High details is on.
    AUTOMAP_WTH_HIGH_DETAILS = 0x02,

    // Scanner is active.
    AUTOMAP_WITH_SCANNER = 0x04,
} AutomapFlags;

typedef enum AutomapFrm {
    AUTOMAP_FRM_BACKGROUND,
    AUTOMAP_FRM_BUTTON_UP,
    AUTOMAP_FRM_BUTTON_DOWN,
    AUTOMAP_FRM_SWITCH_UP,
    AUTOMAP_FRM_SWITCH_DOWN,
    AUTOMAP_FRM_COUNT,
} AutomapFrm;

static void draw_top_down_map(int window, int elevation, unsigned char* backgroundData, int flags);
static int WriteAM_Entry(File* stream);
static int AM_ReadEntry(int map, int elevation);
static int WriteAM_Header(File* stream);
static int AM_ReadMainHeader(File* stream);
static void decode_map_data(int elevation);
static int am_pip_init();
static int copy_file_data(File* stream1, File* stream2, int length);

// 0x41ADE0
static const int defam[AUTOMAP_MAP_COUNT][ELEVATION_COUNT] = {
    { -1, -1, -1 },
    { -1, -1, -1 },
    { -1, -1, -1 },
};

// 0x41B560
static const int displayMapList[AUTOMAP_MAP_COUNT] = {
    -1,
    -1,
    -1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    -1,
    -1,
    0,
    0,
    0,
    0,
    0,
    -1,
    -1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    0,
    0,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    0,
    0,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    0,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    0,
    -1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    0,
    0,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
    -1,
};

// 0x5108C4
static int autoflags = 0;

// 0x56CB18
static AutomapHeader amdbhead;

// 0x56D2A0
static AutomapEntry amdbsubhead;

// 0x56D2A8
static unsigned char* cmpbuf;

// 0x56D2A8
static unsigned char* ambuf;

// automap_init
// 0x41B7F4
int automap_init()
{
    autoflags = 0;
    am_pip_init();
    return 0;
}

// 0x41B808
int automap_reset()
{
    autoflags = 0;
    am_pip_init();
    return 0;
}

// 0x41B81C
void automap_exit()
{
    char* masterPatchesPath;
    if (config_get_string(&game_config, GAME_CONFIG_SYSTEM_KEY, GAME_CONFIG_MASTER_PATCHES_KEY, &masterPatchesPath)) {
        char path[MAX_PATH];
        sprintf(path, "%s\\%s\\%s", masterPatchesPath, "MAPS", AUTOMAP_DB);
        remove(path);
    }
}

// 0x41B87C
int automap_load(File* stream)
{
    return fileReadInt32(stream, &autoflags);
}

// 0x41B898
int automap_save(File* stream)
{
    return fileWriteInt32(stream, autoflags);
}

// 0x41B8B4
int automapDisplayMap(int map)
{
    return displayMapList[map];
}

// 0x41B8BC
void automap(bool isInGame, bool isUsingScanner)
{
    // 0x41B7E0
    static const int frmIds[AUTOMAP_FRM_COUNT] = {
        171, // automap.frm - automap window
        8, // lilredup.frm - little red button up
        9, // lilreddn.frm - little red button down
        172, // autoup.frm - switch up
        173, // autodwn.frm - switch down
    };

    unsigned char* frmData[AUTOMAP_FRM_COUNT];
    CacheEntry* frmHandle[AUTOMAP_FRM_COUNT];
    for (int index = 0; index < AUTOMAP_FRM_COUNT; index++) {
        int fid = art_id(OBJ_TYPE_INTERFACE, frmIds[index], 0, 0, 0);
        frmData[index] = art_ptr_lock_data(fid, 0, 0, &(frmHandle[index]));
        if (frmData[index] == NULL) {
            while (--index >= 0) {
                art_ptr_unlock(frmHandle[index]);
            }
            return;
        }
    }

    int color;
    if (isInGame) {
        color = colorTable[8456];
        _obj_process_seen();
    } else {
        color = colorTable[22025];
    }

    int oldFont = fontGetCurrent();
    fontSetCurrent(101);

    int automapWindowX = AUTOMAP_WINDOW_X;
    int automapWindowY = AUTOMAP_WINDOW_Y;
    int window = windowCreate(automapWindowX, automapWindowY, AUTOMAP_WINDOW_WIDTH, AUTOMAP_WINDOW_HEIGHT, color, WINDOW_FLAG_0x10 | WINDOW_FLAG_0x04);

    int scannerBtn = buttonCreate(window, 111, 454, 15, 16, -1, -1, -1, KEY_LOWERCASE_S, frmData[AUTOMAP_FRM_BUTTON_UP], frmData[AUTOMAP_FRM_BUTTON_DOWN], NULL, BUTTON_FLAG_TRANSPARENT);
    if (scannerBtn != -1) {
        buttonSetCallbacks(scannerBtn, gsound_red_butt_press, gsound_red_butt_release);
    }

    int cancelBtn = buttonCreate(window, 277, 454, 15, 16, -1, -1, -1, KEY_ESCAPE, frmData[AUTOMAP_FRM_BUTTON_UP], frmData[AUTOMAP_FRM_BUTTON_DOWN], NULL, BUTTON_FLAG_TRANSPARENT);
    if (cancelBtn != -1) {
        buttonSetCallbacks(cancelBtn, gsound_red_butt_press, gsound_red_butt_release);
    }

    int switchBtn = buttonCreate(window, 457, 340, 42, 74, -1, -1, KEY_LOWERCASE_L, KEY_LOWERCASE_H, frmData[AUTOMAP_FRM_SWITCH_UP], frmData[AUTOMAP_FRM_SWITCH_DOWN], NULL, BUTTON_FLAG_TRANSPARENT | BUTTON_FLAG_0x01);
    if (switchBtn != -1) {
        buttonSetCallbacks(switchBtn, gsound_toggle_butt_press, gsound_toggle_butt_release);
    }

    if ((autoflags & AUTOMAP_WTH_HIGH_DETAILS) == 0) {
        _win_set_button_rest_state(switchBtn, 1, 0);
    }

    int elevation = gElevation;

    autoflags &= AUTOMAP_WTH_HIGH_DETAILS;

    if (isInGame) {
        autoflags |= AUTOMAP_IN_GAME;
    }

    if (isUsingScanner) {
        autoflags |= AUTOMAP_WITH_SCANNER;
    }

    draw_top_down_map(window, elevation, frmData[AUTOMAP_FRM_BACKGROUND], autoflags);

    bool isoWasEnabled = isoDisable();
    gmouse_set_cursor(MOUSE_CURSOR_ARROW);

    bool done = false;
    while (!done) {
        bool needsRefresh = false;

        // FIXME: There is minor bug in the interface - pressing H/L to toggle
        // high/low details does not update switch state.
        int keyCode = _get_input();
        switch (keyCode) {
        case KEY_TAB:
        case KEY_ESCAPE:
        case KEY_UPPERCASE_A:
        case KEY_LOWERCASE_A:
            done = true;
            break;
        case KEY_UPPERCASE_H:
        case KEY_LOWERCASE_H:
            if ((autoflags & AUTOMAP_WTH_HIGH_DETAILS) == 0) {
                autoflags |= AUTOMAP_WTH_HIGH_DETAILS;
                needsRefresh = true;
            }
            break;
        case KEY_UPPERCASE_L:
        case KEY_LOWERCASE_L:
            if ((autoflags & AUTOMAP_WTH_HIGH_DETAILS) != 0) {
                autoflags &= ~AUTOMAP_WTH_HIGH_DETAILS;
                needsRefresh = true;
            }
            break;
        case KEY_UPPERCASE_S:
        case KEY_LOWERCASE_S:
            if (elevation != gElevation) {
                elevation = gElevation;
                needsRefresh = true;
            }

            if ((autoflags & AUTOMAP_WITH_SCANNER) == 0) {
                Object* scanner = NULL;

                Object* item1 = critterGetItem1(gDude);
                if (item1 != NULL && item1->pid == PROTO_ID_MOTION_SENSOR) {
                    scanner = item1;
                } else {
                    Object* item2 = critterGetItem2(gDude);
                    if (item2 != NULL && item2->pid == PROTO_ID_MOTION_SENSOR) {
                        scanner = item2;
                    }
                }

                if (scanner != NULL && miscItemGetCharges(scanner) > 0) {
                    needsRefresh = true;
                    autoflags |= AUTOMAP_WITH_SCANNER;
                    miscItemConsumeCharge(scanner);
                } else {
                    gsound_play_sfx_file("iisxxxx1");

                    MessageListItem messageListItem;
                    // 17 - The motion sensor is not installed.
                    // 18 - The motion sensor has no charges remaining.
                    const char* title = getmsg(&misc_message_file, &messageListItem, scanner != NULL ? 18 : 17);
                    dialog_out(title, NULL, 0, 165, 140, colorTable[32328], NULL, colorTable[32328], 0);
                }
            }

            break;
        case KEY_CTRL_Q:
        case KEY_ALT_X:
        case KEY_F10:
            game_quit_with_confirm();
            break;
        case KEY_F12:
            takeScreenshot();
            break;
        }

        if (game_user_wants_to_quit != 0) {
            break;
        }

        if (needsRefresh) {
            draw_top_down_map(window, elevation, frmData[AUTOMAP_FRM_BACKGROUND], autoflags);
            needsRefresh = false;
        }
    }

    if (isoWasEnabled) {
        isoEnable();
    }

    windowDestroy(window);
    fontSetCurrent(oldFont);

    for (int index = 0; index < AUTOMAP_FRM_COUNT; index++) {
        art_ptr_unlock(frmHandle[index]);
    }
}

// Renders automap in Map window.
//
// 0x41BD1C
static void draw_top_down_map(int window, int elevation, unsigned char* backgroundData, int flags)
{
    int color;
    if ((flags & AUTOMAP_IN_GAME) != 0) {
        color = colorTable[8456];
    } else {
        color = colorTable[22025];
    }

    windowFill(window, 0, 0, AUTOMAP_WINDOW_WIDTH, AUTOMAP_WINDOW_HEIGHT, color);
    windowDrawBorder(window);

    unsigned char* windowBuffer = windowGetBuffer(window);
    blitBufferToBuffer(backgroundData, AUTOMAP_WINDOW_WIDTH, AUTOMAP_WINDOW_HEIGHT, AUTOMAP_WINDOW_WIDTH, windowBuffer, AUTOMAP_WINDOW_WIDTH);

    for (Object* object = objectFindFirstAtElevation(elevation); object != NULL; object = objectFindNextAtElevation()) {
        if (object->tile == -1) {
            continue;
        }

        int objectType = FID_TYPE(object->fid);
        unsigned char objectColor;

        if ((flags & AUTOMAP_IN_GAME) != 0) {
            if (objectType == OBJ_TYPE_CRITTER
                && (object->flags & OBJECT_HIDDEN) == 0
                && (flags & AUTOMAP_WITH_SCANNER) != 0
                && (object->data.critter.combat.results & DAM_DEAD) == 0) {
                objectColor = colorTable[31744];
            } else {
                if ((object->flags & OBJECT_SEEN) == 0) {
                    continue;
                }

                if (object->pid == PROTO_ID_0x2000031) {
                    objectColor = colorTable[32328];
                } else if (objectType == OBJ_TYPE_WALL) {
                    objectColor = colorTable[992];
                } else if (objectType == OBJ_TYPE_SCENERY
                    && (flags & AUTOMAP_WTH_HIGH_DETAILS) != 0
                    && object->pid != PROTO_ID_0x2000158) {
                    objectColor = colorTable[480];
                } else if (object == gDude) {
                    objectColor = colorTable[31744];
                } else {
                    objectColor = colorTable[0];
                }
            }
        }

        int v10 = -2 * (object->tile % 200) - 10 + AUTOMAP_WINDOW_WIDTH * (2 * (object->tile / 200) + 9) - 60;
        if ((flags & AUTOMAP_IN_GAME) == 0) {
            switch (objectType) {
            case OBJ_TYPE_ITEM:
                objectColor = colorTable[6513];
                break;
            case OBJ_TYPE_CRITTER:
                objectColor = colorTable[28672];
                break;
            case OBJ_TYPE_SCENERY:
                objectColor = colorTable[448];
                break;
            case OBJ_TYPE_WALL:
                objectColor = colorTable[12546];
                break;
            case OBJ_TYPE_MISC:
                objectColor = colorTable[31650];
                break;
            default:
                objectColor = colorTable[0];
            }
        }

        if (objectColor != colorTable[0]) {
            unsigned char* v12 = windowBuffer + v10;
            if ((flags & AUTOMAP_IN_GAME) != 0) {
                if (*v12 != colorTable[992] || objectColor != colorTable[480]) {
                    v12[0] = objectColor;
                    v12[1] = objectColor;
                }

                if (object == gDude) {
                    v12[-1] = objectColor;
                    v12[-AUTOMAP_WINDOW_WIDTH] = objectColor;
                    v12[AUTOMAP_WINDOW_WIDTH] = objectColor;
                }
            } else {
                v12[0] = objectColor;
                v12[1] = objectColor;
                v12[AUTOMAP_WINDOW_WIDTH] = objectColor;
                v12[AUTOMAP_WINDOW_WIDTH + 1] = objectColor;

                v12[AUTOMAP_WINDOW_WIDTH - 1] = objectColor;
                v12[AUTOMAP_WINDOW_WIDTH + 2] = objectColor;
                v12[AUTOMAP_WINDOW_WIDTH * 2] = objectColor;
                v12[AUTOMAP_WINDOW_WIDTH * 2 + 1] = objectColor;
            }
        }
    }

    int textColor;
    if ((flags & AUTOMAP_IN_GAME) != 0) {
        textColor = colorTable[992];
    } else {
        textColor = colorTable[12546];
    }

    if (mapGetCurrentMap() != -1) {
        char* areaName = mapGetCityName(mapGetCurrentMap());
        windowDrawText(window, areaName, 240, 150, 380, textColor | 0x2000000);

        char* mapName = mapGetName(mapGetCurrentMap(), elevation);
        windowDrawText(window, mapName, 240, 150, 396, textColor | 0x2000000);
    }

    win_draw(window);
}

// Renders automap in Pipboy window.
//
// 0x41C004
int draw_top_down_map_pipboy(int window, int map, int elevation)
{
    unsigned char* windowBuffer = windowGetBuffer(window) + 640 * AUTOMAP_PIPBOY_VIEW_Y + AUTOMAP_PIPBOY_VIEW_X;

    unsigned char wallColor = colorTable[992];
    unsigned char sceneryColor = colorTable[480];

    ambuf = (unsigned char*)internal_malloc(11024);
    if (ambuf == NULL) {
        debugPrint("\nAUTOMAP: Error allocating data buffer!\n");
        return -1;
    }

    if (AM_ReadEntry(map, elevation) == -1) {
        internal_free(ambuf);
        return -1;
    }

    int v1 = 0;
    unsigned char v2 = 0;
    unsigned char* ptr = ambuf;

    // FIXME: This loop is implemented incorrectly. Automap requires 400x400 px,
    // but it's top offset is 105, which gives max y 505. It only works because
    // lower portions of automap data contains zeroes. If it doesn't this loop
    // will try to set pixels outside of window buffer, which usually leads to
    // crash.
    for (int y = 0; y < HEX_GRID_HEIGHT; y++) {
        for (int x = 0; x < HEX_GRID_WIDTH; x++) {
            v1 -= 1;
            if (v1 <= 0) {
                v1 = 4;
                v2 = *ptr++;
            }

            switch ((v2 & 0xC0) >> 6) {
            case 1:
                *windowBuffer++ = wallColor;
                *windowBuffer++ = wallColor;
                break;
            case 2:
                *windowBuffer++ = sceneryColor;
                *windowBuffer++ = sceneryColor;
                break;
            default:
                windowBuffer += 2;
                break;
            }

            v2 <<= 2;
        }

        windowBuffer += 640 + 240;
    }

    internal_free(ambuf);

    return 0;
}

// automap_pip_save
// 0x41C0F0
int automap_pip_save()
{
    int map = mapGetCurrentMap();
    int elevation = gElevation;

    int entryOffset = amdbhead.offsets[map][elevation];
    if (entryOffset < 0) {
        return 0;
    }

    debugPrint("\nAUTOMAP: Saving AutoMap DB index %d, level %d\n", map, elevation);

    bool dataBuffersAllocated = false;
    ambuf = (unsigned char*)internal_malloc(11024);
    if (ambuf != NULL) {
        cmpbuf = (unsigned char*)internal_malloc(11024);
        if (cmpbuf != NULL) {
            dataBuffersAllocated = true;
        }
    }

    if (!dataBuffersAllocated) {
        // FIXME: Leaking ambuf.
        debugPrint("\nAUTOMAP: Error allocating data buffers!\n");
        return -1;
    }

    // NOTE: Not sure about the size.
    char path[256];
    sprintf(path, "%s\\%s", "MAPS", AUTOMAP_DB);

    File* stream1 = fileOpen(path, "r+b");
    if (stream1 == NULL) {
        debugPrint("\nAUTOMAP: Error opening automap database file!\n");
        debugPrint("Error continued: automap_pip_save: path: %s", path);
        internal_free(ambuf);
        internal_free(cmpbuf);
        return -1;
    }

    if (AM_ReadMainHeader(stream1) == -1) {
        debugPrint("\nAUTOMAP: Error reading automap database file header!\n");
        internal_free(ambuf);
        internal_free(cmpbuf);
        fileClose(stream1);
        return -1;
    }

    decode_map_data(elevation);

    int compressedDataSize = CompLZS(ambuf, cmpbuf, 10000);
    if (compressedDataSize == -1) {
        amdbsubhead.dataSize = 10000;
        amdbsubhead.isCompressed = 0;
    } else {
        amdbsubhead.dataSize = compressedDataSize;
        amdbsubhead.isCompressed = 1;
    }

    if (entryOffset != 0) {
        sprintf(path, "%s\\%s", "MAPS", AUTOMAP_TMP);

        File* stream2 = fileOpen(path, "wb");
        if (stream2 == NULL) {
            debugPrint("\nAUTOMAP: Error creating temp file!\n");
            internal_free(ambuf);
            internal_free(cmpbuf);
            fileClose(stream1);
            return -1;
        }

        fileRewind(stream1);

        if (copy_file_data(stream1, stream2, entryOffset) == -1) {
            debugPrint("\nAUTOMAP: Error copying file data!\n");
            fileClose(stream1);
            fileClose(stream2);
            internal_free(ambuf);
            internal_free(cmpbuf);
            return -1;
        }

        if (WriteAM_Entry(stream2) == -1) {
            fileClose(stream1);
            internal_free(ambuf);
            internal_free(cmpbuf);
            return -1;
        }

        int nextEntryDataSize;
        if (fileReadInt32(stream1, &nextEntryDataSize) == -1) {
            debugPrint("\nAUTOMAP: Error reading database #1!\n");
            fileClose(stream1);
            fileClose(stream2);
            internal_free(ambuf);
            internal_free(cmpbuf);
            return -1;
        }

        int automapDataSize = fileGetSize(stream1);
        if (automapDataSize == -1) {
            debugPrint("\nAUTOMAP: Error reading database #2!\n");
            fileClose(stream1);
            fileClose(stream2);
            internal_free(ambuf);
            internal_free(cmpbuf);
            return -1;
        }

        int nextEntryOffset = entryOffset + nextEntryDataSize + 5;
        if (automapDataSize != nextEntryOffset) {
            if (fileSeek(stream1, nextEntryOffset, SEEK_SET) == -1) {
                debugPrint("\nAUTOMAP: Error writing temp data!\n");
                fileClose(stream1);
                fileClose(stream2);
                internal_free(ambuf);
                internal_free(cmpbuf);
                return -1;
            }

            if (copy_file_data(stream1, stream2, automapDataSize - nextEntryOffset) == -1) {
                debugPrint("\nAUTOMAP: Error copying file data!\n");
                fileClose(stream1);
                fileClose(stream2);
                internal_free(ambuf);
                internal_free(cmpbuf);
                return -1;
            }
        }

        int diff = amdbsubhead.dataSize - nextEntryDataSize;
        for (int map = 0; map < AUTOMAP_MAP_COUNT; map++) {
            for (int elevation = 0; elevation < ELEVATION_COUNT; elevation++) {
                if (amdbhead.offsets[map][elevation] > entryOffset) {
                    amdbhead.offsets[map][elevation] += diff;
                }
            }
        }

        amdbhead.dataSize += diff;

        if (WriteAM_Header(stream2) == -1) {
            fileClose(stream1);
            internal_free(ambuf);
            internal_free(cmpbuf);
            return -1;
        }

        fileSeek(stream2, 0, SEEK_END);
        fileClose(stream2);
        fileClose(stream1);
        internal_free(ambuf);
        internal_free(cmpbuf);

        char* masterPatchesPath;
        if (!config_get_string(&game_config, GAME_CONFIG_SYSTEM_KEY, GAME_CONFIG_MASTER_PATCHES_KEY, &masterPatchesPath)) {
            debugPrint("\nAUTOMAP: Error reading config info!\n");
            return -1;
        }

        // NOTE: Not sure about the size.
        char automapDbPath[512];
        sprintf(automapDbPath, "%s\\%s\\%s", masterPatchesPath, "MAPS", AUTOMAP_DB);
        if (remove(automapDbPath) != 0) {
            debugPrint("\nAUTOMAP: Error removing database!\n");
            return -1;
        }

        // NOTE: Not sure about the size.
        char automapTmpPath[512];
        sprintf(automapTmpPath, "%s\\%s\\%s", masterPatchesPath, "MAPS", AUTOMAP_TMP);
        if (rename(automapTmpPath, automapDbPath) != 0) {
            debugPrint("\nAUTOMAP: Error renaming database!\n");
            return -1;
        }
    } else {
        bool proceed = true;
        if (fileSeek(stream1, 0, SEEK_END) != -1) {
            if (fileTell(stream1) != amdbhead.dataSize) {
                proceed = false;
            }
        } else {
            proceed = false;
        }

        if (!proceed) {
            debugPrint("\nAUTOMAP: Error reading automap database file header!\n");
            internal_free(ambuf);
            internal_free(cmpbuf);
            fileClose(stream1);
            return -1;
        }

        if (WriteAM_Entry(stream1) == -1) {
            internal_free(ambuf);
            internal_free(cmpbuf);
            return -1;
        }

        amdbhead.offsets[map][elevation] = amdbhead.dataSize;
        amdbhead.dataSize += amdbsubhead.dataSize + 5;

        if (WriteAM_Header(stream1) == -1) {
            internal_free(ambuf);
            internal_free(cmpbuf);
            return -1;
        }

        fileSeek(stream1, 0, SEEK_END);
        fileClose(stream1);
        internal_free(ambuf);
        internal_free(cmpbuf);
    }

    return 1;
}

// Saves automap entry into stream.
//
// 0x41C844
static int WriteAM_Entry(File* stream)
{
    unsigned char* buffer;
    if (amdbsubhead.isCompressed == 1) {
        buffer = cmpbuf;
    } else {
        buffer = ambuf;
    }

    if (_db_fwriteLong(stream, amdbsubhead.dataSize) == -1) {
        goto err;
    }

    if (fileWriteUInt8(stream, amdbsubhead.isCompressed) == -1) {
        goto err;
    }

    if (fileWriteUInt8List(stream, buffer, amdbsubhead.dataSize) == -1) {
        goto err;
    }

    return 0;

err:

    debugPrint("\nAUTOMAP: Error writing automap database entry data!\n");
    fileClose(stream);

    return -1;
}

// 0x41C8CC
static int AM_ReadEntry(int map, int elevation)
{
    cmpbuf = NULL;

    char path[MAX_PATH];
    sprintf(path, "%s\\%s", "MAPS", AUTOMAP_DB);

    bool success = true;

    File* stream = fileOpen(path, "r+b");
    if (stream == NULL) {
        debugPrint("\nAUTOMAP: Error opening automap database file!\n");
        debugPrint("Error continued: AM_ReadEntry: path: %s", path);
        return -1;
    }

    if (AM_ReadMainHeader(stream) == -1) {
        debugPrint("\nAUTOMAP: Error reading automap database header!\n");
        fileClose(stream);
        return -1;
    }

    if (amdbhead.offsets[map][elevation] <= 0) {
        success = false;
        goto out;
    }

    if (fileSeek(stream, amdbhead.offsets[map][elevation], SEEK_SET) == -1) {
        success = false;
        goto out;
    }

    if (_db_freadInt(stream, &(amdbsubhead.dataSize)) == -1) {
        success = false;
        goto out;
    }

    if (fileReadUInt8(stream, &(amdbsubhead.isCompressed)) == -1) {
        success = false;
        goto out;
    }

    if (amdbsubhead.isCompressed == 1) {
        cmpbuf = (unsigned char*)internal_malloc(11024);
        if (cmpbuf == NULL) {
            debugPrint("\nAUTOMAP: Error allocating decompression buffer!\n");
            fileClose(stream);
            return -1;
        }

        if (fileReadUInt8List(stream, cmpbuf, amdbsubhead.dataSize) == -1) {
            success = 0;
            goto out;
        }

        if (DecodeLZS(cmpbuf, ambuf, 10000) == -1) {
            debugPrint("\nAUTOMAP: Error decompressing DB entry!\n");
            fileClose(stream);
            return -1;
        }
    } else {
        if (fileReadUInt8List(stream, ambuf, amdbsubhead.dataSize) == -1) {
            success = false;
            goto out;
        }
    }

out:

    fileClose(stream);

    if (!success) {
        debugPrint("\nAUTOMAP: Error reading automap database entry data!\n");

        return -1;
    }

    if (cmpbuf != NULL) {
        internal_free(cmpbuf);
    }

    return 0;
}

// Saves automap.db header.
//
// 0x41CAD8
static int WriteAM_Header(File* stream)
{
    fileRewind(stream);

    if (fileWriteUInt8(stream, amdbhead.version) == -1) {
        goto err;
    }

    if (_db_fwriteLong(stream, amdbhead.dataSize) == -1) {
        goto err;
    }

    if (_db_fwriteLongCount(stream, (int*)amdbhead.offsets, AUTOMAP_OFFSET_COUNT) == -1) {
        goto err;
    }

    return 0;

err:

    debugPrint("\nAUTOMAP: Error writing automap database header!\n");

    fileClose(stream);

    return -1;
}

// Loads automap.db header.
//
// 0x41CB50
static int AM_ReadMainHeader(File* stream)
{

    if (fileReadUInt8(stream, &(amdbhead.version)) == -1) {
        return -1;
    }

    if (_db_freadInt(stream, &(amdbhead.dataSize)) == -1) {
        return -1;
    }

    if (_db_freadIntCount(stream, (int*)amdbhead.offsets, AUTOMAP_OFFSET_COUNT) == -1) {
        return -1;
    }

    if (amdbhead.version != 1) {
        return -1;
    }

    return 0;
}

// 0x41CBA4
static void decode_map_data(int elevation)
{
    memset(ambuf, 0, SQUARE_GRID_SIZE);

    _obj_process_seen();

    Object* object = objectFindFirstAtElevation(elevation);
    while (object != NULL) {
        if (object->tile != -1 && (object->flags & OBJECT_SEEN) != 0) {
            int contentType;

            int objectType = FID_TYPE(object->fid);
            if (objectType == OBJ_TYPE_SCENERY && object->pid != PROTO_ID_0x2000158) {
                contentType = 2;
            } else if (objectType == OBJ_TYPE_WALL) {
                contentType = 1;
            } else {
                contentType = 0;
            }

            if (contentType != 0) {
                int v1 = 200 - object->tile % 200;
                int v2 = v1 / 4 + 50 * (object->tile / 200);
                int v3 = 2 * (3 - v1 % 4);
                ambuf[v2] &= ~(0x03 << v3);
                ambuf[v2] |= (contentType << v3);
            }
        }
        object = objectFindNextAtElevation();
    }
}

// 0x41CC98
static int am_pip_init()
{
    amdbhead.version = 1;
    amdbhead.dataSize = 1925;
    memcpy(amdbhead.offsets, defam, sizeof(defam));

    char path[MAX_PATH];
    sprintf(path, "%s\\%s", "MAPS", AUTOMAP_DB);

    File* stream = fileOpen(path, "wb");
    if (stream == NULL) {
        debugPrint("\nAUTOMAP: Error creating automap database file!\n");
        return -1;
    }

    if (WriteAM_Header(stream) == -1) {
        return -1;
    }

    fileClose(stream);

    return 0;
}

// NOTE: Unused.
//
// 0x41CD34
int YesWriteIndex(int mapIndex, int elevation)
{
    if (mapIndex < AUTOMAP_MAP_COUNT && elevation < ELEVATION_COUNT && mapIndex >= 0 && elevation >= 0) {
        return defam[mapIndex][elevation] >= 0;
    }

    return 0;
}

// Copy data from stream1 to stream2.
//
// 0x41CD6C
static int copy_file_data(File* stream1, File* stream2, int length)
{
    void* buffer = internal_malloc(0xFFFF);
    if (buffer == NULL) {
        return -1;
    }

    // NOTE: Original code is slightly different, but does the same thing.
    while (length != 0) {
        int chunkLength = min(length, 0xFFFF);

        if (fileRead(buffer, chunkLength, 1, stream1) != 1) {
            break;
        }

        if (fileWrite(buffer, chunkLength, 1, stream2) != 1) {
            break;
        }

        length -= chunkLength;
    }

    internal_free(buffer);

    if (length != 0) {
        return -1;
    }

    return 0;
}

// 0x41CE74
int ReadAMList(AutomapHeader** automapHeaderPtr)
{
    char path[MAX_PATH];
    sprintf(path, "%s\\%s", "MAPS", AUTOMAP_DB);

    File* stream = fileOpen(path, "rb");
    if (stream == NULL) {
        debugPrint("\nAUTOMAP: Error opening database file for reading!\n");
        debugPrint("Error continued: ReadAMList: path: %s", path);
        return -1;
    }

    if (AM_ReadMainHeader(stream) == -1) {
        debugPrint("\nAUTOMAP: Error reading automap database header pt2!\n");
        fileClose(stream);
        return -1;
    }

    fileClose(stream);

    *automapHeaderPtr = &amdbhead;

    return 0;
}
