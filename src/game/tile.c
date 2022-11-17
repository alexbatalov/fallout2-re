#include "game/tile.h"

#include <assert.h>
#include <string.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include "plib/color/color.h"
#include "game/config.h"
#include "core.h"
#include "plib/gnw/debug.h"
#include "plib/gnw/grbuf.h"
#include "game/gconfig.h"
#include "game/gmouse.h"
#include "game/light.h"
#include "game/map.h"
#include "game/object.h"

#define TILE_IS_VALID(tile) ((tile) >= 0 && (tile) < grid_size)

typedef struct STRUCT_51D99C {
    int field_0;
    int field_4;
} STRUCT_51D99C;

typedef struct STRUCT_51DA04 {
    int field_0;
    int field_4;
} STRUCT_51DA04;

typedef struct STRUCT_51DA6C {
    int field_0;
    int field_4;
    int field_8;
    int field_C; // something with light level?
} STRUCT_51DA6C;

typedef struct STRUCT_51DB0C {
    int field_0;
    int field_4;
    int field_8;
} STRUCT_51DB0C;

typedef struct STRUCT_51DB48 {
    int field_0;
    int field_4;
    int field_8;
} STRUCT_51DB48;

static void refresh_mapper(Rect* rect, int elevation);
static void refresh_game(Rect* rect, int elevation);
static bool tile_on_edge(int tile);
static void roof_fill_on(int x, int y, int elevation);
static void roof_fill_off(int x, int y, int elevation);
static void roof_draw(int fid, int x, int y, Rect* rect, int light);

// 0x51D950
static bool borderInitialized = false;

// 0x51D954
static bool scroll_blocking_on = true;

// 0x51D958
static bool scroll_limiting_on = true;

// 0x51D95C
static bool show_roof = true;

// 0x51D960
static bool show_grid = false;

// 0x51D964
static TileWindowRefreshElevationProc* tile_refresh = refresh_game;

// 0x51D968
static bool refresh_enabled = true;

// 0x51D96C
int off_tile[2][6] = {
    {
        16,
        32,
        16,
        -16,
        -32,
        -16,
    },
    {
        -12,
        0,
        12,
        12,
        0,
        -12,
    }
};

// 0x51D99C
static STRUCT_51D99C rightside_up_table[13] = {
    { -1, 2 },
    { 78, 2 },
    { 76, 6 },
    { 73, 8 },
    { 71, 10 },
    { 68, 14 },
    { 65, 16 },
    { 63, 18 },
    { 61, 20 },
    { 58, 24 },
    { 55, 26 },
    { 53, 28 },
    { 50, 32 },
};

// 0x51DA04
static STRUCT_51DA04 upside_down_table[13] = {
    { 0, 32 },
    { 48, 32 },
    { 49, 30 },
    { 52, 26 },
    { 55, 24 },
    { 57, 22 },
    { 60, 18 },
    { 63, 16 },
    { 65, 14 },
    { 67, 12 },
    { 70, 8 },
    { 73, 6 },
    { 75, 4 },
};

// 0x51DA6C
static STRUCT_51DA6C verticies[10] = {
    { 16, -1, -201, 0 },
    { 48, -2, -2, 0 },
    { 960, 0, 0, 0 },
    { 992, 199, -1, 0 },
    { 1024, 198, 198, 0 },
    { 1936, 200, 200, 0 },
    { 1968, 399, 199, 0 },
    { 2000, 398, 398, 0 },
    { 2912, 400, 400, 0 },
    { 2944, 599, 399, 0 },
};

// 0x51DB0C
static STRUCT_51DB0C rightside_up_triangles[5] = {
    { 2, 3, 0 },
    { 3, 4, 1 },
    { 5, 6, 3 },
    { 6, 7, 4 },
    { 8, 9, 6 },
};

// 0x51DB48
static STRUCT_51DB48 upside_down_triangles[5] = {
    { 0, 3, 1 },
    { 2, 5, 3 },
    { 3, 6, 4 },
    { 5, 8, 6 },
    { 6, 9, 7 },
};

// 0x668224
static int intensity_map[3280];

// 0x66B564
static int dir_tile2[2][6];

// Deltas to perform tile calculations in given direction.
//
// 0x66B594
static int dir_tile[2][6];

// 0x66B5C4
static unsigned char tile_grid_blocked[512];

// 0x66B7C4
static unsigned char tile_grid_occupied[512];

// 0x66B9C4
static unsigned char tile_mask[512];

// 0x66BBC4
static Rect tile_border;

// 0x66BBD4
static Rect buf_rect;

// 0x66BBE4
static unsigned char tile_grid[32 * 16];

// 0x66BDE4
static int square_y;

// 0x66BDE8
static int square_x;

// 0x66BDEC
static int square_offx;

// 0x66BDF0
static int square_offy;

// 0x66BDF4
static TileWindowRefreshProc* blit;

// 0x66BDF8
static int tile_offy;

// 0x66BDFC
static int tile_offx;

// 0x66BE00
static int square_size;

// Number of tiles horizontally.
//
// Currently this value is always 200.
//
// 0x66BE04
static int grid_width;

// 0x66BE08
static TileData** squares;

// 0x66BE0C
static unsigned char* buf;

// Number of tiles vertically.
//
// Currently this value is always 200.
//
// 0x66BE10
static int grid_length;

// 0x66BE14
static int buf_length;

// 0x66BE18
static int tile_x;

// 0x66BE1C
static int tile_y;

// The number of tiles in the hex grid.
//
// 0x66BE20
static int grid_size;

// 0x66BE24
static int square_length;

// 0x66BE28
static int buf_full;

// 0x66BE2C
static int square_width;

// 0x66BE30
static int buf_width;

// 0x66BE34
int tile_center_tile;

// 0x4B0C40
int tile_init(TileData** a1, int squareGridWidth, int squareGridHeight, int hexGridWidth, int hexGridHeight, unsigned char* buffer, int windowWidth, int windowHeight, int windowPitch, TileWindowRefreshProc* windowRefreshProc)
{
    int v11;
    int v12;
    int v13;

    int v20;
    int v21;
    int v22;
    int v23;
    int v24;
    int v25;

    square_width = squareGridWidth;
    squares = a1;
    grid_length = hexGridHeight;
    square_length = squareGridHeight;
    grid_width = hexGridWidth;
    dir_tile[0][0] = -1;
    dir_tile[0][4] = 1;
    dir_tile[1][1] = -1;
    grid_size = hexGridWidth * hexGridHeight;
    dir_tile[1][3] = 1;
    buf = buffer;
    dir_tile2[0][0] = -1;
    buf_width = windowWidth;
    dir_tile2[0][3] = -1;
    buf_length = windowHeight;
    dir_tile2[1][1] = 1;
    buf_full = windowPitch;
    dir_tile2[1][2] = 1;
    buf_rect.lrx = windowWidth - 1;
    square_size = squareGridHeight * squareGridWidth;
    buf_rect.lry = windowHeight - 1;
    buf_rect.ulx = 0;
    blit = windowRefreshProc;
    buf_rect.uly = 0;
    dir_tile[0][1] = hexGridWidth - 1;
    dir_tile[0][2] = hexGridWidth;
    show_grid = 0;
    dir_tile[0][3] = hexGridWidth + 1;
    dir_tile[1][2] = hexGridWidth;
    dir_tile2[0][4] = hexGridWidth;
    dir_tile2[0][5] = hexGridWidth;
    dir_tile[0][5] = -hexGridWidth;
    dir_tile[1][0] = -hexGridWidth - 1;
    dir_tile[1][4] = 1 - hexGridWidth;
    dir_tile[1][5] = -hexGridWidth;
    dir_tile2[0][1] = -hexGridWidth - 1;
    dir_tile2[1][4] = -hexGridWidth;
    dir_tile2[0][2] = hexGridWidth - 1;
    dir_tile2[1][5] = -hexGridWidth;
    dir_tile2[1][0] = hexGridWidth + 1;
    dir_tile2[1][3] = 1 - hexGridWidth;

    v11 = 0;
    v12 = 0;
    do {
        v13 = 64;
        do {
            tile_mask[v12++] = v13 > v11;
            v13 -= 4;
        } while (v13);

        do {
            tile_mask[v12++] = v13 > v11 ? 2 : 0;
            v13 += 4;
        } while (v13 != 64);

        v11 += 16;
    } while (v11 != 64);

    v11 = 0;
    do {
        v13 = 0;
        do {
            tile_mask[v12++] = 0;
            v13++;
        } while (v13 < 32);
        v11++;
    } while (v11 < 8);

    v11 = 0;
    do {
        v13 = 0;
        do {
            tile_mask[v12++] = v13 > v11 ? 0 : 3;
            v13 += 4;
        } while (v13 != 64);

        v13 = 64;
        do {
            tile_mask[v12++] = v13 > v11 ? 0 : 4;
            v13 -= 4;
        } while (v13);

        v11 += 16;
    } while (v11 != 64);

    buf_fill(tile_grid, 32, 16, 32, 0);
    draw_line(tile_grid, 32, 16, 0, 31, 4, colorTable[4228]);
    draw_line(tile_grid, 32, 31, 4, 31, 12, colorTable[4228]);
    draw_line(tile_grid, 32, 31, 12, 16, 15, colorTable[4228]);
    draw_line(tile_grid, 32, 0, 12, 16, 15, colorTable[4228]);
    draw_line(tile_grid, 32, 0, 4, 0, 12, colorTable[4228]);
    draw_line(tile_grid, 32, 16, 0, 0, 4, colorTable[4228]);

    buf_fill(tile_grid_occupied, 32, 16, 32, 0);
    draw_line(tile_grid_occupied, 32, 16, 0, 31, 4, colorTable[31]);
    draw_line(tile_grid_occupied, 32, 31, 4, 31, 12, colorTable[31]);
    draw_line(tile_grid_occupied, 32, 31, 12, 16, 15, colorTable[31]);
    draw_line(tile_grid_occupied, 32, 0, 12, 16, 15, colorTable[31]);
    draw_line(tile_grid_occupied, 32, 0, 4, 0, 12, colorTable[31]);
    draw_line(tile_grid_occupied, 32, 16, 0, 0, 4, colorTable[31]);

    buf_fill(tile_grid_blocked, 32, 16, 32, 0);
    draw_line(tile_grid_blocked, 32, 16, 0, 31, 4, colorTable[31744]);
    draw_line(tile_grid_blocked, 32, 31, 4, 31, 12, colorTable[31744]);
    draw_line(tile_grid_blocked, 32, 31, 12, 16, 15, colorTable[31744]);
    draw_line(tile_grid_blocked, 32, 0, 12, 16, 15, colorTable[31744]);
    draw_line(tile_grid_blocked, 32, 0, 4, 0, 12, colorTable[31744]);
    draw_line(tile_grid_blocked, 32, 16, 0, 0, 4, colorTable[31744]);

    for (v20 = 0; v20 < 16; v20++) {
        v21 = v20 * 32;
        v22 = 31;
        v23 = v21 + 31;

        if (tile_grid_blocked[v23] == 0) {
            do {
                --v22;
                --v23;
            } while (v22 > 0 && tile_grid_blocked[v23] == 0);
        }

        v24 = v21;
        v25 = 0;
        if (tile_grid_blocked[v21] == 0) {
            do {
                ++v25;
                ++v24;
            } while (v25 < 32 && tile_grid_blocked[v24] == 0);
        }

        draw_line(tile_grid_blocked, 32, v25, v20, v22, v20, colorTable[31744]);
    }

    tile_set_center(hexGridWidth * (hexGridHeight / 2) + hexGridWidth / 2, TILE_SET_CENTER_FLAG_IGNORE_SCROLL_RESTRICTIONS);
    tile_set_border(windowWidth, windowHeight, hexGridWidth, hexGridHeight);

    char* executable;
    config_get_string(&game_config, GAME_CONFIG_SYSTEM_KEY, GAME_CONFIG_EXECUTABLE_KEY, &executable);
    if (stricmp(executable, "mapper") == 0) {
        tile_refresh = refresh_mapper;
    }

    return 0;
}

// 0x4B11E4
void tile_set_border(int windowWidth, int windowHeight, int hexGridWidth, int hexGridHeight)
{
    int v1 = tile_num(-320, -240, 0);
    int v2 = tile_num(-320, windowHeight + 240, 0);

    tile_border.ulx = abs(hexGridWidth - 1 - v2 % hexGridWidth - tile_x) + 6;
    tile_border.uly = abs(tile_y - v1 / hexGridWidth) + 7;
    tile_border.lrx = hexGridWidth - tile_border.ulx - 1;
    tile_border.lry = hexGridHeight - tile_border.uly - 1;

    if ((tile_border.ulx & 1) == 0) {
        tile_border.ulx++;
    }

    if ((tile_border.lrx & 1) == 0) {
        tile_border.ulx--;
    }

    borderInitialized = true;
}

// NOTE: Uncollapsed 0x4B129C.
void tile_reset()
{
}

// NOTE: Uncollapsed 0x4B129C.
void tile_exit()
{
}

// 0x4B12A8
void tile_disable_refresh()
{
    refresh_enabled = false;
}

// 0x4B12B4
void tile_enable_refresh()
{
    refresh_enabled = true;
}

// 0x4B12C0
void tile_refresh_rect(Rect* rect, int elevation)
{
    if (refresh_enabled) {
        if (elevation == map_elevation) {
            tile_refresh(rect, elevation);
        }
    }
}

// 0x4B12D8
void tile_refresh_display()
{
    if (refresh_enabled) {
        tile_refresh(&buf_rect, map_elevation);
    }
}

// 0x4B12F8
int tile_set_center(int tile, int flags)
{
    if (!TILE_IS_VALID(tile)) {
        return -1;
    }

    if ((flags & TILE_SET_CENTER_FLAG_IGNORE_SCROLL_RESTRICTIONS) == 0) {
        if (scroll_limiting_on) {
            int tileScreenX;
            int tileScreenY;
            tile_coord(tile, &tileScreenX, &tileScreenY, map_elevation);

            int dudeScreenX;
            int dudeScreenY;
            tile_coord(obj_dude->tile, &dudeScreenX, &dudeScreenY, map_elevation);

            int dx = abs(dudeScreenX - tileScreenX);
            int dy = abs(dudeScreenY - tileScreenY);

            if (dx > abs(dudeScreenX - tile_offx)
                || dy > abs(dudeScreenY - tile_offy)) {
                if (dx >= 480 || dy >= 400) {
                    return -1;
                }
            }
        }

        if (scroll_blocking_on) {
            if (obj_scroll_blocking_at(tile, map_elevation) == 0) {
                return -1;
            }
        }
    }

    int tile_x = grid_width - 1 - tile % grid_width;
    int tile_y = tile / grid_width;

    if (borderInitialized) {
        if (tile_x <= tile_border.ulx || tile_x >= tile_border.lrx || tile_y <= tile_border.uly || tile_y >= tile_border.lry) {
            return -1;
        }
    }

    tile_y = tile_y;
    tile_offx = (buf_width - 32) / 2;
    tile_x = tile_x;
    tile_offy = (buf_length - 16) / 2;

    if (tile_x & 1) {
        tile_x -= 1;
        tile_offx -= 32;
    }

    square_x = tile_x / 2;
    square_y = tile_y / 2;
    square_offx = tile_offx - 16;
    square_offy = tile_offy - 2;

    if (tile_y & 1) {
        square_offy -= 12;
        square_offx -= 16;
    }

    tile_center_tile = tile;

    if ((flags & TILE_SET_CENTER_REFRESH_WINDOW) != 0) {
        // NOTE: Uninline.
        tile_refresh_display();
    }

    return 0;
}

// 0x4B1554
static void refresh_mapper(Rect* rect, int elevation)
{
    Rect rectToUpdate;

    if (rect_inside_bound(rect, &buf_rect, &rectToUpdate) == -1) {
        return;
    }

    buf_fill(buf + buf_full * rectToUpdate.uly + rectToUpdate.ulx,
        rectToUpdate.lrx - rectToUpdate.ulx + 1,
        rectToUpdate.lry - rectToUpdate.uly + 1,
        buf_full,
        0);

    square_render_floor(&rectToUpdate, elevation);
    grid_render(&rectToUpdate, elevation);
    obj_render_pre_roof(&rectToUpdate, elevation);
    square_render_roof(&rectToUpdate, elevation);
    obj_render_post_roof(&rectToUpdate, elevation);
    blit(&rectToUpdate);
}

// 0x4B15E8
static void refresh_game(Rect* rect, int elevation)
{
    Rect rectToUpdate;

    if (rect_inside_bound(rect, &buf_rect, &rectToUpdate) == -1) {
        return;
    }

    square_render_floor(&rectToUpdate, elevation);
    obj_render_pre_roof(&rectToUpdate, elevation);
    square_render_roof(&rectToUpdate, elevation);
    obj_render_post_roof(&rectToUpdate, elevation);
    blit(&rectToUpdate);
}

// 0x4B1634
void tile_toggle_roof(int a1)
{
    show_roof = 1 - show_roof;

    if (a1) {
        // NOTE: Uninline.
        tile_refresh_display();
    }
}

// 0x4B166C
int tile_roof_visible()
{
    return show_roof;
}

// 0x4B1674
int tile_coord(int tile, int* screenX, int* screenY, int elevation)
{
    int v3;
    int v4;
    int v5;
    int v6;

    if (!TILE_IS_VALID(tile)) {
        return -1;
    }

    v3 = grid_width - 1 - tile % grid_width;
    v4 = tile / grid_width;

    *screenX = tile_offx;
    *screenY = tile_offy;

    v5 = (v3 - tile_x) / -2;
    *screenX += 48 * ((v3 - tile_x) / 2);
    *screenY += 12 * v5;

    if (v3 & 1) {
        if (v3 <= tile_x) {
            *screenX -= 16;
            *screenY += 12;
        } else {
            *screenX += 32;
        }
    }

    v6 = v4 - tile_y;
    *screenX += 16 * v6;
    *screenY += 12 * v6;

    return 0;
}

// 0x4B1754
int tile_num(int screenX, int screenY, int elevation)
{
    int v2;
    int v3;
    int v4;
    int v5;
    int v6;
    int v7;
    int v8;
    int v9;
    int v10;
    int v11;
    int v12;

    v2 = screenY - tile_offy;
    if (v2 >= 0) {
        v3 = v2 / 12;
    } else {
        v3 = (v2 + 1) / 12 - 1;
    }

    v4 = screenX - tile_offx - 16 * v3;
    v5 = v2 - 12 * v3;

    if (v4 >= 0) {
        v6 = v4 / 64;
    } else {
        v6 = (v4 + 1) / 64 - 1;
    }

    v7 = v6 + v3;
    v8 = v4 - (v6 * 64);
    v9 = 2 * v6;

    if (v8 >= 32) {
        v8 -= 32;
        v9++;
    }

    v10 = tile_y + v7;
    v11 = tile_x + v9;

    switch (tile_mask[32 * v5 + v8]) {
    case 2:
        v11++;
        if (v11 & 1) {
            v10--;
        }
        break;
    case 1:
        v10--;
        break;
    case 3:
        v11--;
        if (!(v11 & 1)) {
            v10++;
        }
        break;
    case 4:
        v10++;
        break;
    default:
        break;
    }

    v12 = grid_width - 1 - v11;
    if (v12 >= 0 && v12 < grid_width && v10 >= 0 && v10 < grid_length) {
        return grid_width * v10 + v12;
    }

    return -1;
}

// tile_distance
// 0x4B185C
int tile_dist(int tile1, int tile2)
{
    int i;
    int v9;
    int v8;
    int v2;

    if (tile1 == -1) {
        return 9999;
    }

    if (tile2 == -1) {
        return 9999;
    }

    int x1;
    int y1;
    tile_coord(tile2, &x1, &y1, 0);

    v2 = tile1;
    for (i = 0; v2 != tile2; i++) {
        // TODO: Looks like inlined rotation_to_tile.
        int x2;
        int y2;
        tile_coord(v2, &x2, &y2, 0);

        int dx = x1 - x2;
        int dy = y1 - y2;

        if (x1 == x2) {
            if (dy < 0) {
                v9 = 0;
            } else {
                v9 = 2;
            }
        } else {
            v8 = (int)trunc(atan2((double)-dy, (double)dx) * 180.0 * 0.3183098862851122);

            v9 = 360 - (v8 + 180) - 90;
            if (v9 < 0) {
                v9 += 360;
            }

            v9 /= 60;

            if (v9 >= 6) {
                v9 = 5;
            }
        }

        v2 += dir_tile[v2 % grid_width & 1][v9];
    }

    return i;
}

// 0x4B1994
bool tile_in_front_of(int tile1, int tile2)
{
    int x1;
    int y1;
    tile_coord(tile1, &x1, &y1, 0);

    int x2;
    int y2;
    tile_coord(tile2, &x2, &y2, 0);

    int dx = x2 - x1;
    int dy = y2 - y1;

    return (double)dx <= (double)dy * -4.0;
}

// 0x4B1A00
bool tile_to_right_of(int tile1, int tile2)
{
    int x1;
    int y1;
    tile_coord(tile1, &x1, &y1, 0);

    int x2;
    int y2;
    tile_coord(tile2, &x2, &y2, 0);

    int dx = x2 - x1;
    int dy = y2 - y1;

    // NOTE: the value below looks like 4/3, which is 0x3FF55555555555, but it's
    // binary value is slightly different: 0x3FF55555555556. This difference plays
    // important role as seen right in the beginning of the game, comparing tiles
    // 17488 (0x4450) and 15288 (0x3BB8).
    return (double)dx <= (double)dy * 1.3333333333333335;
}

// tile_num_in_direction
// 0x4B1A6C
int tile_num_in_direction(int tile, int rotation, int distance)
{
    int newTile = tile;
    for (int index = 0; index < distance; index++) {
        if (tile_on_edge(newTile)) {
            break;
        }

        int parity = (newTile % grid_width) & 1;
        newTile += dir_tile[parity][rotation];
    }

    return newTile;
}

// rotation_to_tile
// 0x4B1ABC
int tile_dir(int tile1, int tile2)
{
    int x1;
    int y1;
    tile_coord(tile1, &x1, &y1, 0);

    int x2;
    int y2;
    tile_coord(tile2, &x2, &y2, 0);

    int dy = y2 - y1;
    x2 -= x1;
    y2 -= y1;

    if (x2 != 0) {
        // TODO: Check.
        int v6 = (int)trunc(atan2((double)-dy, (double)x2) * 180.0 * 0.3183098862851122);
        int v7 = 360 - (v6 + 180) - 90;
        if (v7 < 0) {
            v7 += 360;
        }

        v7 /= 60;

        if (v7 >= ROTATION_COUNT) {
            v7 = ROTATION_NW;
        }
        return v7;
    }

    return dy < 0 ? ROTATION_NE : ROTATION_SE;
}

// 0x4B1B84
int tile_num_beyond(int from, int to, int distance)
{
    if (distance <= 0 || from == to) {
        return from;
    }

    int fromX;
    int fromY;
    tile_coord(from, &fromX, &fromY, 0);
    fromX += 16;
    fromY += 8;

    int toX;
    int toY;
    tile_coord(to, &toX, &toY, 0);
    toX += 16;
    toY += 8;

    int deltaX = toX - fromX;
    int deltaY = toY - fromY;

    int v27 = 2 * abs(deltaX);

    int stepX = 0;
    if (deltaX > 0)
        stepX = 1;
    else if (deltaX < 0)
        stepX = -1;

    int v26 = 2 * abs(deltaY);

    int stepY = 0;
    if (deltaY > 0)
        stepY = 1;
    else if (deltaY < 0)
        stepY = -1;

    int v28 = from;
    int tileX = fromX;
    int tileY = fromY;

    int v6 = 0;

    if (v27 > v26) {
        int middle = v26 - v27 / 2;
        while (true) {
            int tile = tile_num(tileX, tileY, 0);
            if (tile != v28) {
                v6 += 1;
                if (v6 == distance || tile_on_edge(tile)) {
                    return tile;
                }

                v28 = tile;
            }

            if (middle >= 0) {
                middle -= v27;
                tileY += stepY;
            }

            middle += v26;
            tileX += stepX;
        }
    } else {
        int middle = v27 - v26 / 2;
        while (true) {
            int tile = tile_num(tileX, tileY, 0);
            if (tile != v28) {
                v6 += 1;
                if (v6 == distance || tile_on_edge(tile)) {
                    return tile;
                }

                v28 = tile;
            }

            if (middle >= 0) {
                middle -= v26;
                tileX += stepX;
            }

            middle += v27;
            tileY += stepY;
        }
    }

    assert(false && "Should be unreachable");
}

// 0x4B1D20
static bool tile_on_edge(int tile)
{
    if (!TILE_IS_VALID(tile)) {
        return false;
    }

    if (tile < grid_width) {
        return true;
    }

    if (tile >= grid_size - grid_width) {
        return true;
    }

    if (tile % grid_width == 0) {
        return true;
    }

    if (tile % grid_width == grid_width - 1) {
        return true;
    }

    return false;
}

// 0x4B1D80
void tile_enable_scroll_blocking()
{
    scroll_blocking_on = true;
}

// 0x4B1D8C
void tile_disable_scroll_blocking()
{
    scroll_blocking_on = false;
}

// 0x4B1D98
bool tile_get_scroll_blocking()
{
    return scroll_blocking_on;
}

// 0x4B1DA0
void tile_enable_scroll_limiting()
{
    scroll_limiting_on = true;
}

// 0x4B1DAC
void tile_disable_scroll_limiting()
{
    scroll_limiting_on = false;
}

// 0x4B1DB8
bool tile_get_scroll_limiting()
{
    return scroll_limiting_on;
}

// 0x4B1DC0
int square_coord(int squareTile, int* coordX, int* coordY, int elevation)
{
    int v5;
    int v6;
    int v7;
    int v8;
    int v9;

    if (squareTile < 0 || squareTile >= square_size) {
        return -1;
    }

    v5 = square_width - 1 - squareTile % square_width;
    v6 = squareTile / square_width;
    v7 = square_x;

    *coordX = square_offx;
    *coordY = square_offy;

    v8 = v5 - v7;
    *coordX += 48 * v8;
    *coordY -= 12 * v8;

    v9 = v6 - square_y;
    *coordX += 32 * v9;
    *coordY += 24 * v9;

    return 0;
}

// 0x4B1E60
int square_coord_roof(int squareTile, int* screenX, int* screenY, int elevation)
{
    int v5;
    int v6;
    int v7;
    int v8;
    int v9;
    int v10;

    if (squareTile < 0 || squareTile >= square_size) {
        return -1;
    }

    v5 = square_width - 1 - squareTile % square_width;
    v6 = squareTile / square_width;
    v7 = square_x;
    *screenX = square_offx;
    *screenY = square_offy;

    v8 = v5 - v7;
    *screenX += 48 * v8;
    *screenY -= 12 * v8;

    v9 = v6 - square_y;
    *screenX += 32 * v9;
    v10 = 24 * v9 + *screenY;
    *screenY = v10;
    *screenY = v10 - 96;

    return 0;
}

// 0x4B1F04
int square_num(int screenX, int screenY, int elevation)
{
    int coordY;
    int coordX;

    square_xy(screenX, screenY, elevation, &coordX, &coordY);

    if (coordX >= 0 && coordX < square_width && coordY >= 0 && coordY < square_length) {
        return coordX + square_width * coordY;
    }

    return -1;
}

// NOTE: Unused.
//
// 0x4B1F4C
int square_num_roof(int screenX, int screenY, int elevation)
{
    int x;
    int y;

    square_xy_roof(screenX, screenY, elevation, &x, &y);

    if (x >= 0 && x < square_width && y >= 0 && y < square_length) {
        return x + square_width * y;
    }

    return -1;
}

// 0x4B1F94
void square_xy(int screenX, int screenY, int elevation, int* coordX, int* coordY)
{
    int v4;
    int v5;
    int v6;
    int v8;

    v4 = screenX - square_offx;
    v5 = screenY - square_offy - 12;
    v6 = 3 * v4 - 4 * v5;
    *coordX = v6 >= 0 ? (v6 / 192) : ((v6 + 1) / 192 - 1);

    v8 = 4 * v5 + v4;
    *coordY = v8 >= 0 ? (v8 / 128) : ((v8 + 1) / 128 - 1);

    *coordX += square_x;
    *coordY += square_y;

    *coordX = square_width - 1 - *coordX;
}

// 0x4B203C
void square_xy_roof(int screenX, int screenY, int elevation, int* coordX, int* coordY)
{
    int v4;
    int v5;
    int v6;
    int v8;

    v4 = screenX - square_offx;
    v5 = screenY + 96 - square_offy - 12;
    v6 = 3 * v4 - 4 * v5;

    *coordX = (v6 >= 0) ? (v6 / 192) : ((v6 + 1) / 192 - 1);

    v8 = 4 * v5 + v4;
    *coordY = v8 >= 0 ? (v8 / 128) : ((v8 + 1) / 128 - 1);

    *coordX += square_x;
    *coordY += square_y;

    *coordX = square_width - 1 - *coordX;
}

// 0x4B20E8
void square_render_roof(Rect* rect, int elevation)
{
    if (!show_roof) {
        return;
    }

    int temp;
    int minY;
    int minX;
    int maxX;
    int maxY;

    square_xy_roof(rect->ulx, rect->uly, elevation, &temp, &minY);
    square_xy_roof(rect->lrx, rect->uly, elevation, &minX, &temp);
    square_xy_roof(rect->ulx, rect->lry, elevation, &maxX, &temp);
    square_xy_roof(rect->lrx, rect->lry, elevation, &temp, &maxY);

    if (minX < 0) {
        minX = 0;
    }

    if (minX >= square_width) {
        minX = square_width - 1;
    }

    if (minY < 0) {
        minY = 0;
    }

    // FIXME: Probably a bug - testing X, then changing Y.
    if (minX >= square_length) {
        minY = square_length - 1;
    }

    int light = light_get_ambient();

    int baseSquareTile = square_width * minY;

    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            int squareTile = baseSquareTile + x;
            int frmId = squares[elevation]->field_0[squareTile];
            frmId >>= 16;
            if ((((frmId & 0xF000) >> 12) & 0x01) == 0) {
                int fid = art_id(OBJ_TYPE_TILE, frmId & 0xFFF, 0, 0, 0);
                if (fid != art_id(OBJ_TYPE_TILE, 1, 0, 0, 0)) {
                    int screenX;
                    int screenY;
                    square_coord_roof(squareTile, &screenX, &screenY, elevation);
                    roof_draw(fid, screenX, screenY, rect, light);
                }
            }
        }
        baseSquareTile += square_width;
    }
}

// 0x4B22D0
static void roof_fill_on(int a1, int a2, int elevation)
{
    while ((a1 >= 0 && a1 < square_width) && (a2 >= 0 && a2 < square_length)) {
        int squareTile = square_width * a2 + a1;
        int value = squares[elevation]->field_0[squareTile];
        int upper = (value >> 16) & 0xFFFF;

        int id = upper & 0xFFF;
        if (art_id(OBJ_TYPE_TILE, id, 0, 0, 0) == art_id(OBJ_TYPE_TILE, 1, 0, 0, 0)) {
            break;
        }

        int flag = (upper & 0xF000) >> 12;
        if ((flag & 0x01) == 0) {
            break;
        }

        flag &= ~0x01;

        squares[elevation]->field_0[squareTile] = (value & 0xFFFF) | (((flag << 12) | id) << 16);

        roof_fill_on(a1 - 1, a2, elevation);
        roof_fill_on(a1 + 1, a2, elevation);
        roof_fill_on(a1, a2 - 1, elevation);

        a2++;
    }
}

// 0x4B23D4
void tile_fill_roof(int a1, int a2, int elevation, int a4)
{
    if (a4) {
        roof_fill_on(a1, a2, elevation);
    } else {
        roof_fill_off(a1, a2, elevation);
    }
}

// 0x4B23DC
static void roof_fill_off(int a1, int a2, int elevation)
{
    while ((a1 >= 0 && a1 < square_width) && (a2 >= 0 && a2 < square_length)) {
        int squareTile = square_width * a2 + a1;
        int value = squares[elevation]->field_0[squareTile];
        int upper = (value >> 16) & 0xFFFF;

        int id = upper & 0xFFF;
        if (art_id(OBJ_TYPE_TILE, id, 0, 0, 0) == art_id(OBJ_TYPE_TILE, 1, 0, 0, 0)) {
            break;
        }

        int flag = (upper & 0xF000) >> 12;
        if ((flag & 0x03) != 0) {
            break;
        }

        flag |= 0x01;

        squares[elevation]->field_0[squareTile] = (value & 0xFFFF) | (((flag << 12) | id) << 16);

        roof_fill_off(a1 - 1, a2, elevation);
        roof_fill_off(a1 + 1, a2, elevation);
        roof_fill_off(a1, a2 - 1, elevation);

        a2++;
    }
}

// 0x4B24E0
static void roof_draw(int fid, int x, int y, Rect* rect, int light)
{
    CacheEntry* tileFrmHandle;
    Art* tileFrm = art_ptr_lock(fid, &tileFrmHandle);
    if (tileFrm == NULL) {
        return;
    }

    int tileWidth = art_frame_width(tileFrm, 0, 0);
    int tileHeight = art_frame_length(tileFrm, 0, 0);

    Rect tileRect;
    tileRect.ulx = x;
    tileRect.uly = y;
    tileRect.lrx = x + tileWidth - 1;
    tileRect.lry = y + tileHeight - 1;

    if (rect_inside_bound(&tileRect, rect, &tileRect) == 0) {
        unsigned char* tileFrmBuffer = art_frame_data(tileFrm, 0, 0);
        tileFrmBuffer += tileWidth * (tileRect.uly - y) + (tileRect.ulx - x);

        CacheEntry* eggFrmHandle;
        Art* eggFrm = art_ptr_lock(obj_egg->fid, &eggFrmHandle);
        if (eggFrm != NULL) {
            int eggWidth = art_frame_width(eggFrm, 0, 0);
            int eggHeight = art_frame_length(eggFrm, 0, 0);

            int eggScreenX;
            int eggScreenY;
            tile_coord(obj_egg->tile, &eggScreenX, &eggScreenY, obj_egg->elevation);

            eggScreenX += 16;
            eggScreenY += 8;

            eggScreenX += eggFrm->xOffsets[0];
            eggScreenY += eggFrm->yOffsets[0];

            eggScreenX += obj_egg->x;
            eggScreenY += obj_egg->y;

            Rect eggRect;
            eggRect.ulx = eggScreenX - eggWidth / 2;
            eggRect.uly = eggScreenY - eggHeight + 1;
            eggRect.lrx = eggRect.ulx + eggWidth - 1;
            eggRect.lry = eggScreenY;

            obj_egg->sx = eggRect.ulx;
            obj_egg->sy = eggRect.uly;

            Rect intersectedRect;
            if (rect_inside_bound(&eggRect, &tileRect, &intersectedRect) == 0) {
                Rect rects[4];

                rects[0].ulx = tileRect.ulx;
                rects[0].uly = tileRect.uly;
                rects[0].lrx = tileRect.lrx;
                rects[0].lry = intersectedRect.uly - 1;

                rects[1].ulx = tileRect.ulx;
                rects[1].uly = intersectedRect.uly;
                rects[1].lrx = intersectedRect.ulx - 1;
                rects[1].lry = intersectedRect.lry;

                rects[2].ulx = intersectedRect.lrx + 1;
                rects[2].uly = intersectedRect.uly;
                rects[2].lrx = tileRect.lrx;
                rects[2].lry = intersectedRect.lry;

                rects[3].ulx = tileRect.ulx;
                rects[3].uly = intersectedRect.lry + 1;
                rects[3].lrx = tileRect.lrx;
                rects[3].lry = tileRect.lry;

                for (int i = 0; i < 4; i++) {
                    Rect* cr = &(rects[i]);
                    if (cr->ulx <= cr->lrx && cr->uly <= cr->lry) {
                        dark_trans_buf_to_buf(tileFrmBuffer + tileWidth * (cr->uly - tileRect.uly) + (cr->ulx - tileRect.ulx),
                            cr->lrx - cr->ulx + 1,
                            cr->lry - cr->uly + 1,
                            tileWidth,
                            buf,
                            cr->ulx,
                            cr->uly,
                            buf_full,
                            light);
                    }
                }

                unsigned char* eggBuf = art_frame_data(eggFrm, 0, 0);
                intensity_mask_buf_to_buf(tileFrmBuffer + tileWidth * (intersectedRect.uly - tileRect.uly) + (intersectedRect.ulx - tileRect.ulx),
                    intersectedRect.lrx - intersectedRect.ulx + 1,
                    intersectedRect.lry - intersectedRect.uly + 1,
                    tileWidth,
                    buf + buf_full * intersectedRect.uly + intersectedRect.ulx,
                    buf_full,
                    eggBuf + eggWidth * (intersectedRect.uly - eggRect.uly) + (intersectedRect.ulx - eggRect.ulx),
                    eggWidth,
                    light);
            } else {
                dark_trans_buf_to_buf(tileFrmBuffer, tileRect.lrx - tileRect.ulx + 1, tileRect.lry - tileRect.uly + 1, tileWidth, buf, tileRect.ulx, tileRect.uly, buf_full, light);
            }

            art_ptr_unlock(eggFrmHandle);
        }
    }

    art_ptr_unlock(tileFrmHandle);
}

// 0x4B2944
void square_render_floor(Rect* rect, int elevation)
{
    int minY;
    int maxX;
    int maxY;
    int minX;
    int temp;

    square_xy(rect->ulx, rect->uly, elevation, &temp, &minY);
    square_xy(rect->lrx, rect->uly, elevation, &minX, &temp);
    square_xy(rect->ulx, rect->lry, elevation, &maxX, &temp);
    square_xy(rect->lrx, rect->lry, elevation, &temp, &maxY);

    if (minX < 0) {
        minX = 0;
    }

    if (minX >= square_width) {
        minX = square_width - 1;
    }

    if (minY < 0) {
        minY = 0;
    }

    if (minX >= square_length) {
        minY = square_length - 1;
    }

    light_get_ambient();

    temp = square_width * minY;
    for (int v15 = minY; v15 <= maxY; v15++) {
        for (int i = minX; i <= maxX; i++) {
            int v3 = temp + i;
            int frmId = squares[elevation]->field_0[v3];
            if ((((frmId & 0xF000) >> 12) & 0x01) == 0) {
                int v12;
                int v13;
                square_coord(v3, &v12, &v13, elevation);
                int fid = art_id(OBJ_TYPE_TILE, frmId & 0xFFF, 0, 0, 0);
                floor_draw(fid, v12, v13, rect);
            }
        }
        temp += square_width;
    }
}

// 0x4B2B10
bool square_roof_intersect(int x, int y, int elevation)
{
    if (!show_roof) {
        return false;
    }

    bool result = false;

    int tileX;
    int tileY;
    square_xy_roof(x, y, elevation, &tileX, &tileY);

    TileData* ptr = squares[elevation];
    int idx = square_width * tileY + tileX;
    int upper = ptr->field_0[square_width * tileY + tileX] >> 16;
    int fid = art_id(OBJ_TYPE_TILE, upper & 0xFFF, 0, 0, 0);
    if (fid != art_id(OBJ_TYPE_TILE, 1, 0, 0, 0)) {
        if ((((upper & 0xF000) >> 12) & 1) == 0) {
            int fid = art_id(OBJ_TYPE_TILE, upper & 0xFFF, 0, 0, 0);
            CacheEntry* handle;
            Art* art = art_ptr_lock(fid, &handle);
            if (art != NULL) {
                unsigned char* data = art_frame_data(art, 0, 0);
                if (data != NULL) {
                    int v18;
                    int v17;
                    square_coord_roof(idx, &v18, &v17, elevation);

                    int width = art_frame_width(art, 0, 0);
                    if (data[width * (y - v17) + x - v18] != 0) {
                        result = true;
                    }
                }
                art_ptr_unlock(handle);
            }
        }
    }

    return result;
}

// NOTE: Unused.
//
// 0x4B2E60
void grid_toggle()
{
    show_grid = 1 - show_grid;
}

// NOTE: Unused.
//
// 0x4B2E78
void grid_on()
{
    show_grid = 1;
}

// NOTE: Unused.
//
// 0x4B2E84
void grid_off()
{
    show_grid = 0;
}

// 0x4B2E90
int get_grid_flag()
{
    return show_grid;
}

// 0x4B2E98
void grid_render(Rect* rect, int elevation)
{
    if (!show_grid) {
        return;
    }

    for (int y = rect->uly - 12; y < rect->lry + 12; y += 6) {
        for (int x = rect->ulx - 32; x < rect->lrx + 32; x += 16) {
            int tile = tile_num(x, y, elevation);
            draw_grid(tile, elevation, rect);
        }
    }
}

// 0x4B2EF0
void grid_draw(int tile, int elevation)
{
    Rect rect;

    tile_coord(tile, &(rect.ulx), &(rect.uly), elevation);

    rect.lrx = rect.ulx + 32 - 1;
    rect.lry = rect.uly + 16 - 1;
    if (rect_inside_bound(&rect, &buf_rect, &rect) != -1) {
        draw_grid(tile, elevation, &rect);
        blit(&rect);
    }
}

// 0x4B2F4C
void draw_grid(int tile, int elevation, Rect* rect)
{
    if (tile == -1) {
        return;
    }

    int x;
    int y;
    tile_coord(tile, &x, &y, elevation);

    Rect r;
    r.ulx = x;
    r.uly = y;
    r.lrx = x + 32 - 1;
    r.lry = y + 16 - 1;

    if (rect_inside_bound(&r, rect, &r) == -1) {
        return;
    }

    if (obj_blocking_at(NULL, tile, elevation) != NULL) {
        trans_buf_to_buf(tile_grid_blocked + 32 * (r.uly - y) + (r.ulx - x),
            r.lrx - r.ulx + 1,
            r.lry - r.uly + 1,
            32,
            buf + buf_full * r.uly + r.ulx,
            buf_full);
        return;
    }

    if (obj_occupied(tile, elevation)) {
        trans_buf_to_buf(tile_grid_occupied + 32 * (r.uly - y) + (r.ulx - x),
            r.lrx - r.ulx + 1,
            r.lry - r.uly + 1,
            32,
            buf + buf_full * r.uly + r.ulx,
            buf_full);
        return;
    }

    translucent_trans_buf_to_buf(tile_grid_occupied + 32 * (r.uly - y) + (r.ulx - x),
        r.lrx - r.ulx + 1,
        r.lry - r.uly + 1,
        32,
        buf + buf_full * r.uly + r.ulx,
        0,
        0,
        buf_full,
        wallBlendTable,
        commonGrayTable);
}

// 0x4B30C4
void floor_draw(int fid, int x, int y, Rect* rect)
{
    if (art_get_disable(FID_TYPE(fid)) != 0) {
        return;
    }

    CacheEntry* cacheEntry;
    Art* art = art_ptr_lock(fid, &cacheEntry);
    if (art == NULL) {
        return;
    }

    int elev = map_elevation;
    int left = rect->ulx;
    int top = rect->uly;
    int width = rect->lrx - rect->ulx + 1;
    int height = rect->lry - rect->uly + 1;
    int frameWidth;
    int frameHeight;
    int v15;
    int v76;
    int v77;
    int v78;
    int v79;

    int savedX = x;
    int savedY = y;

    if (left < 0) {
        left = 0;
    }

    if (top < 0) {
        top = 0;
    }

    if (left + width > buf_width) {
        width = buf_width - left;
    }

    if (top + height > buf_length) {
        height = buf_length - top;
    }

    if (x >= buf_width || x > rect->lrx || y >= buf_length || y > rect->lry) goto out;

    frameWidth = art_frame_width(art, 0, 0);
    frameHeight = art_frame_length(art, 0, 0);

    if (left < x) {
        v79 = 0;
        int v12 = left + width;
        v77 = frameWidth + x <= v12 ? frameWidth : v12 - x;
    } else {
        v79 = left - x;
        x = left;
        v77 = frameWidth - v79;
        if (v77 > width) {
            v77 = width;
        }
    }

    if (top < y) {
        int v14 = height + top;
        v78 = 0;
        v76 = frameHeight + y <= v14 ? frameHeight : v14 - y;
    } else {
        v78 = top - y;
        y = top;
        v76 = frameHeight - v78;
        if (v76 > height) {
            v76 = height;
        }
    }

    if (v77 <= 0 || v76 <= 0) goto out;

    v15 = tile_num(savedX, savedY + 13, map_elevation);
    if (v15 != -1) {
        int v17 = light_get_ambient();
        for (int i = v15 & 1; i < 10; i++) {
            // NOTE: calling light_get_tile two times, probably a result of using __min kind macro
            int v21 = light_get_tile(elev, v15 + verticies[i].field_4);
            if (v21 <= v17) {
                v21 = v17;
            }

            verticies[i].field_C = v21;
        }

        int v23 = 0;
        for (int i = 0; i < 9; i++) {
            if (verticies[i + 1].field_C != verticies[i].field_C) {
                break;
            }

            v23++;
        }

        if (v23 == 9) {
            unsigned char* frame_data = art_frame_data(art, 0, 0);
            dark_trans_buf_to_buf(frame_data + frameWidth * v78 + v79, v77, v76, frameWidth, buf, x, y, buf_full, verticies[0].field_C);
            goto out;
        }

        for (int i = 0; i < 5; i++) {
            STRUCT_51DB0C* ptr_51DB0C = &(rightside_up_triangles[i]);
            int v32 = verticies[ptr_51DB0C->field_8].field_C;
            int v33 = verticies[ptr_51DB0C->field_8].field_0;
            int v34 = verticies[ptr_51DB0C->field_4].field_C - verticies[ptr_51DB0C->field_0].field_C;
            // TODO: Probably wrong.
            int v35 = v34 / 32;
            int v36 = (verticies[ptr_51DB0C->field_0].field_C - v32) / 13;
            int* v37 = &(intensity_map[v33]);
            if (v35 != 0) {
                if (v36 != 0) {
                    for (int i = 0; i < 13; i++) {
                        int v41 = v32;
                        int v42 = rightside_up_table[i].field_4;
                        v37 += rightside_up_table[i].field_0;
                        for (int j = 0; j < v42; j++) {
                            *v37++ = v41;
                            v41 += v35;
                        }
                        v32 += v36;
                    }
                } else {
                    for (int i = 0; i < 13; i++) {
                        int v38 = v32;
                        int v39 = rightside_up_table[i].field_4;
                        v37 += rightside_up_table[i].field_0;
                        for (int j = 0; j < v39; j++) {
                            *v37++ = v38;
                            v38 += v35;
                        }
                    }
                }
            } else {
                if (v36 != 0) {
                    for (int i = 0; i < 13; i++) {
                        int v46 = rightside_up_table[i].field_4;
                        v37 += rightside_up_table[i].field_0;
                        for (int j = 0; j < v46; j++) {
                            *v37++ = v32;
                        }
                        v32 += v36;
                    }
                } else {
                    for (int i = 0; i < 13; i++) {
                        int v44 = rightside_up_table[i].field_4;
                        v37 += rightside_up_table[i].field_0;
                        for (int j = 0; j < v44; j++) {
                            *v37++ = v32;
                        }
                    }
                }
            }
        }

        for (int i = 0; i < 5; i++) {
            STRUCT_51DB48* ptr_51DB48 = &(upside_down_triangles[i]);
            int v50 = verticies[ptr_51DB48->field_0].field_C;
            int v51 = verticies[ptr_51DB48->field_0].field_0;
            int v52 = verticies[ptr_51DB48->field_8].field_C - v50;
            // TODO: Probably wrong.
            int v53 = v52 / 32;
            int v54 = (verticies[ptr_51DB48->field_4].field_C - v50) / 13;
            int* v55 = &(intensity_map[v51]);
            if (v53 != 0) {
                if (v54 != 0) {
                    for (int i = 0; i < 13; i++) {
                        int v59 = v50;
                        int v60 = upside_down_table[i].field_4;
                        v55 += upside_down_table[i].field_0;
                        for (int j = 0; j < v60; j++) {
                            *v55++ = v59;
                            v59 += v53;
                        }
                        v50 += v54;
                    }
                } else {
                    for (int i = 0; i < 13; i++) {
                        int v56 = v50;
                        int v57 = upside_down_table[i].field_4;
                        v55 += upside_down_table[i].field_0;
                        for (int j = 0; j < v57; j++) {
                            *v55++ = v56;
                            v56 += v53;
                        }
                    }
                }
            } else {
                if (v54 != 0) {
                    for (int i = 0; i < 13; i++) {
                        int v64 = upside_down_table[i].field_4;
                        v55 += upside_down_table[i].field_0;
                        for (int j = 0; j < v64; j++) {
                            *v55++ = v50;
                        }
                        v50 += v54;
                    }
                } else {
                    for (int i = 0; i < 13; i++) {
                        int v62 = upside_down_table[i].field_4;
                        v55 += upside_down_table[i].field_0;
                        for (int j = 0; j < v62; j++) {
                            *v55++ = v50;
                        }
                    }
                }
            }
        }

        unsigned char* v66 = buf + buf_full * y + x;
        unsigned char* v67 = art_frame_data(art, 0, 0) + frameWidth * v78 + v79;
        int* v68 = &(intensity_map[160 + 80 * v78]) + v79;
        int v86 = frameWidth - v77;
        int v85 = buf_full - v77;
        int v87 = 80 - v77;

        while (--v76 != -1) {
            for (int kk = 0; kk < v77; kk++) {
                if (*v67 != 0) {
                    *v66 = intensityColorTable[*v67][*v68 >> 9];
                }
                v67++;
                v68++;
                v66++;
            }
            v66 += v85;
            v68 += v87;
            v67 += v86;
        }
    }

out:

    art_ptr_unlock(cacheEntry);
}

// 0x4B372C
int tile_make_line(int from, int to, int* tiles, int tilesCapacity)
{
    if (tilesCapacity <= 1) {
        return 0;
    }

    int count = 0;

    int fromX;
    int fromY;
    tile_coord(from, &fromX, &fromY, map_elevation);
    fromX += 16;
    fromY += 8;

    int toX;
    int toY;
    tile_coord(to, &toX, &toY, map_elevation);
    toX += 16;
    toY += 8;

    tiles[count++] = from;

    int stepX;
    int deltaX = toX - fromX;
    if (deltaX > 0)
        stepX = 1;
    else if (deltaX < 0)
        stepX = -1;
    else
        stepX = 0;

    int stepY;
    int deltaY = toY - fromY;
    if (deltaY > 0)
        stepY = 1;
    else if (deltaY < 0)
        stepY = -1;
    else
        stepY = 0;

    int v28 = 2 * abs(toX - fromX);
    int v27 = 2 * abs(toY - fromY);

    int tileX = fromX;
    int tileY = fromY;

    if (v28 <= v27) {
        int middleX = v28 - v27 / 2;
        while (true) {
            int tile = tile_num(tileX, tileY, map_elevation);
            tiles[count] = tile;

            if (tile == to) {
                count++;
                break;
            }

            if (tile != tiles[count - 1] && (count == 1 || tile != tiles[count - 2])) {
                count++;
                if (count == tilesCapacity) {
                    break;
                }
            }

            if (tileY == toY) {
                break;
            }

            if (middleX >= 0) {
                tileX += stepX;
                middleX -= v27;
            }

            middleX += v28;
            tileY += stepY;
        }
    } else {
        int middleY = v27 - v28 / 2;
        while (true) {
            int tile = tile_num(tileX, tileY, map_elevation);
            tiles[count] = tile;

            if (tile == to) {
                count++;
                break;
            }

            if (tile != tiles[count - 1] && (count == 1 || tile != tiles[count - 2])) {
                count++;
                if (count == tilesCapacity) {
                    break;
                }
            }

            if (tileX == toX) {
                return count;
            }

            if (middleY >= 0) {
                tileY += stepY;
                middleY -= v28;
            }

            middleY += v27;
            tileX += stepX;
        }
    }

    return count;
}

// 0x4B3924
int tile_scroll_to(int tile, int flags)
{
    if (tile == tile_center_tile) {
        return -1;
    }

    int oldCenterTile = tile_center_tile;

    int v9[200];
    int count = tile_make_line(tile_center_tile, tile, v9, 200);
    if (count == 0) {
        return -1;
    }

    int index = 1;
    for (; index < count; index++) {
        if (tile_set_center(v9[index], 0) == -1) {
            break;
        }
    }

    int rc = 0;
    if ((flags & 0x01) != 0) {
        if (index != count) {
            tile_set_center(oldCenterTile, 0);
            rc = -1;
        }
    }

    if ((flags & 0x02) != 0) {
        // NOTE: Uninline.
        tile_refresh_display();
    }

    return rc;
}
