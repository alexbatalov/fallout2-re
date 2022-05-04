#include "tile.h"

#include "color.h"
#include "config.h"
#include "core.h"
#include "debug.h"
#include "draw.h"
#include "game_config.h"
#include "game_mouse.h"
#include "light.h"
#include "map.h"
#include "object.h"

#define _USE_MATH_DEFINES
#include <math.h>

// 0x50E7C7
double const dbl_50E7C7 = -4.0;

// 0x51D950
bool dword_51D950 = false;

// 0x51D954
bool dword_51D954 = true;

// 0x51D958
bool dword_51D958 = true;

// 0x51D95C
int dword_51D95C = 1;

// 0x51D960
int dword_51D960 = 0;

// 0x51D964
TileWindowRefreshElevationProc* off_51D964 = sub_4B15E8;

// 0x51D968
bool gTileEnabled = true;

// 0x51D96C
const int dword_51D96C[6] = {
    16,
    32,
    16,
    -16,
    -32,
    -16,
};

// 0x51D984
const int dword_51D984[6] = {
    -12,
    0,
    12,
    12,
    0,
    -12,
};

// 0x51D99C
STRUCT_51D99C stru_51D99C[13] = {
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
STRUCT_51DA04 stru_51DA04[13] = {
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
STRUCT_51DA6C stru_51DA6C[10] = {
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
STRUCT_51DB0C stru_51DB0C[5] = {
    { 2, 3, 0 },
    { 3, 4, 1 },
    { 5, 6, 3 },
    { 6, 7, 4 },
    { 8, 9, 6 },
};

// 0x51DB48
STRUCT_51DB48 stru_51DB48[5] = {
    { 0, 3, 1 },
    { 2, 5, 3 },
    { 3, 6, 4 },
    { 5, 8, 6 },
    { 6, 9, 7 },
};

// 0x668224
int dword_668224[3280];

// 0x66B564
int dword_66B564[2][6];

// Deltas to perform tile calculations in given direction.
//
// 0x66B594
int dword_66B594[2][6];

// 0x66B5C4
unsigned char byte_66B5C4[512];

// 0x66B7C4
unsigned char byte_66B7C4[512];

// 0x66B9C4
unsigned char byte_66B9C4[512];

// 0x66BBC4
int dword_66BBC4 = 0;

// 0x66BBC8
int dword_66BBC8 = 0;

// 0x66BBCC
int dword_66BBCC = 0;

// 0x66BBD0
int dword_66BBD0 = 0;

// 0x66BBD4
Rect gTileWindowRect;

// 0x66BBE4
unsigned char byte_66BBE4[32 * 16];

// 0x66BDE4
int dword_66BDE4;

// 0x66BDE8
int dword_66BDE8;

// 0x66BDEC
int dword_66BDEC;

// 0x66BDF0
int dword_66BDF0;

// 0x66BDF4
TileWindowRefreshProc* gTileWindowRefreshProc;

// 0x66BDF8
int dword_66BDF8;

// 0x66BDFC
int dword_66BDFC;

// 0x66BE00
int gSquareGridSize;

// Number of tiles horizontally.
//
// Currently this value is always 200.
//
// 0x66BE04
int gHexGridWidth;

// 0x66BE08
TileData** dword_66BE08;

// 0x66BE0C
unsigned char* gTileWindowBuffer;

// Number of tiles vertically.
//
// Currently this value is always 200.
//
// 0x66BE10
int gHexGridHeight;

// 0x66BE14
int gTileWindowHeight;

// 0x66BE18
int dword_66BE18;

// 0x66BE1C
int dword_66BE1C;

// The number of tiles in the hex grid.
//
// 0x66BE20
int gHexGridSize;

// 0x66BE24
int gSquareGridHeight;

// 0x66BE28
int gTileWindowPitch;

// 0x66BE2C
int gSquareGridWidth;

// 0x66BE30
int gTileWindowWidth;

// 0x66BE34
int gCenterTile;

// 0x4B0C40
int tileInit(TileData** a1, int squareGridWidth, int squareGridHeight, int hexGridWidth, int hexGridHeight, unsigned char* buf, int windowWidth, int windowHeight, int windowPitch, TileWindowRefreshProc* windowRefreshProc)
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

    gSquareGridWidth = squareGridWidth;
    dword_66BE08 = a1;
    gHexGridHeight = hexGridHeight;
    gSquareGridHeight = squareGridHeight;
    gHexGridWidth = hexGridWidth;
    dword_66B594[0][0] = -1;
    dword_66B594[0][4] = 1;
    dword_66B594[1][1] = -1;
    gHexGridSize = hexGridWidth * hexGridHeight;
    dword_66B594[1][3] = 1;
    gTileWindowBuffer = buf;
    dword_66B564[0][0] = -1;
    gTileWindowWidth = windowWidth;
    dword_66B564[0][3] = -1;
    gTileWindowHeight = windowHeight;
    dword_66B564[1][1] = 1;
    gTileWindowPitch = windowPitch;
    dword_66B564[1][2] = 1;
    gTileWindowRect.right = windowWidth - 1;
    gSquareGridSize = squareGridHeight * squareGridWidth;
    gTileWindowRect.bottom = windowHeight - 1;
    gTileWindowRect.left = 0;
    gTileWindowRefreshProc = windowRefreshProc;
    gTileWindowRect.top = 0;
    dword_66B594[0][1] = hexGridWidth - 1;
    dword_66B594[0][2] = hexGridWidth;
    dword_51D960 = 0;
    dword_66B594[0][3] = hexGridWidth + 1;
    dword_66B594[1][2] = hexGridWidth;
    dword_66B564[0][4] = hexGridWidth;
    dword_66B564[0][5] = hexGridWidth;
    dword_66B594[0][5] = -hexGridWidth;
    dword_66B594[1][0] = -hexGridWidth - 1;
    dword_66B594[1][4] = 1 - hexGridWidth;
    dword_66B594[1][5] = -hexGridWidth;
    dword_66B564[0][1] = -hexGridWidth - 1;
    dword_66B564[1][4] = -hexGridWidth;
    dword_66B564[0][2] = hexGridWidth - 1;
    dword_66B564[1][5] = -hexGridWidth;
    dword_66B564[1][0] = hexGridWidth + 1;
    dword_66B564[1][3] = 1 - hexGridWidth;

    v11 = 0;
    v12 = 0;
    do {
        v13 = 64;
        do {
            byte_66B9C4[v12++] = v13 > v11;
            v13 -= 4;
        } while (v13);

        do {
            byte_66B9C4[v12++] = v13 > v11 ? 2 : 0;
            v13 += 4;
        } while (v13 != 64);

        v11 += 16;
    } while (v11 != 64);

    v11 = 0;
    do {
        v13 = 0;
        do {
            byte_66B9C4[v12++] = 0;
            v13++;
        } while (v13 < 32);
        v11++;
    } while (v11 < 8);

    v11 = 0;
    do {
        v13 = 0;
        do {
            byte_66B9C4[v12++] = v13 > v11 ? 0 : 3;
            v13 += 4;
        } while (v13 != 64);

        v13 = 64;
        do {
            byte_66B9C4[v12++] = v13 > v11 ? 0 : 4;
            v13 -= 4;
        } while (v13);

        v11 += 16;
    } while (v11 != 64);

    bufferFill(byte_66BBE4, 32, 16, 32, 0);
    bufferDrawLine(byte_66BBE4, 32, 16, 0, 31, 4, byte_6A38D0[4228]);
    bufferDrawLine(byte_66BBE4, 32, 31, 4, 31, 12, byte_6A38D0[4228]);
    bufferDrawLine(byte_66BBE4, 32, 31, 12, 16, 15, byte_6A38D0[4228]);
    bufferDrawLine(byte_66BBE4, 32, 0, 12, 16, 15, byte_6A38D0[4228]);
    bufferDrawLine(byte_66BBE4, 32, 0, 4, 0, 12, byte_6A38D0[4228]);
    bufferDrawLine(byte_66BBE4, 32, 16, 0, 0, 4, byte_6A38D0[4228]);

    bufferFill(byte_66B7C4, 32, 16, 32, 0);
    bufferDrawLine(byte_66B7C4, 32, 16, 0, 31, 4, byte_6A38D0[31]);
    bufferDrawLine(byte_66B7C4, 32, 31, 4, 31, 12, byte_6A38D0[31]);
    bufferDrawLine(byte_66B7C4, 32, 31, 12, 16, 15, byte_6A38D0[31]);
    bufferDrawLine(byte_66B7C4, 32, 0, 12, 16, 15, byte_6A38D0[31]);
    bufferDrawLine(byte_66B7C4, 32, 0, 4, 0, 12, byte_6A38D0[31]);
    bufferDrawLine(byte_66B7C4, 32, 16, 0, 0, 4, byte_6A38D0[31]);

    bufferFill(byte_66B5C4, 32, 16, 32, 0);
    bufferDrawLine(byte_66B5C4, 32, 16, 0, 31, 4, byte_6A38D0[31744]);
    bufferDrawLine(byte_66B5C4, 32, 31, 4, 31, 12, byte_6A38D0[31744]);
    bufferDrawLine(byte_66B5C4, 32, 31, 12, 16, 15, byte_6A38D0[31744]);
    bufferDrawLine(byte_66B5C4, 32, 0, 12, 16, 15, byte_6A38D0[31744]);
    bufferDrawLine(byte_66B5C4, 32, 0, 4, 0, 12, byte_6A38D0[31744]);
    bufferDrawLine(byte_66B5C4, 32, 16, 0, 0, 4, byte_6A38D0[31744]);

    for (v20 = 0; v20 < 16; v20++) {
        v21 = v20 * 32;
        v22 = 31;
        v23 = v21 + 31;

        if (byte_66B5C4[v23] == 0) {
            do {
                --v22;
                --v23;
            } while (v22 > 0 && byte_66B5C4[v23] == 0);
        }

        v24 = v21;
        v25 = 0;
        if (byte_66B5C4[v21] == 0) {
            do {
                ++v25;
                ++v24;
            } while (v25 < 32 && byte_66B5C4[v24] == 0);
        }

        bufferDrawLine(byte_66B5C4, 32, v25, v20, v22, v20, byte_6A38D0[31744]);
    }

    tileSetCenter(hexGridWidth * (hexGridHeight / 2) + hexGridWidth / 2, 2);
    sub_4B11E4(windowWidth, windowHeight, hexGridWidth, hexGridHeight);

    char* executable;
    configGetString(&gGameConfig, GAME_CONFIG_SYSTEM_KEY, GAME_CONFIG_EXECUTABLE_KEY, &executable);
    if (stricmp(executable, "mapper") == 0) {
        off_51D964 = sub_4B1554;
    }

    return 0;
}

// 0x4B11E4
void sub_4B11E4(int windowWidth, int windowHeight, int hexGridWidth, int hexGridHeight)
{
    int v1 = tileFromScreenXY(-320, -240, 0);
    int v2 = tileFromScreenXY(-320, windowHeight + 240, 0);

    dword_66BBC4 = abs(hexGridWidth - 1 - v2 % hexGridWidth - dword_66BE18) + 6;
    dword_66BBC8 = abs(dword_66BE1C - v1 / hexGridWidth) + 7;
    dword_66BBCC = hexGridWidth - dword_66BBC4 - 1;
    dword_66BBD0 = hexGridHeight - dword_66BBC8 - 1;

    if ((dword_66BBC4 & 1) == 0) {
        dword_66BBC4++;
    }

    if ((dword_66BBCC & 1) == 0) {
        dword_66BBC4--;
    }

    dword_51D950 = true;
}

// NOTE: Collapsed.
//
// 0x4B129C
void sub_4B129C()
{
}

// NOTE: Uncollapsed 0x4B129C.
void tileReset()
{
    sub_4B129C();
}

// NOTE: Uncollapsed 0x4B129C.
void tileExit()
{
    sub_4B129C();
}

// 0x4B12A8
void tileDisable()
{
    gTileEnabled = false;
}

// 0x4B12B4
void tileEnable()
{
    gTileEnabled = true;
}

// 0x4B12C0
void tileWindowRefreshRect(Rect* rect, int elevation)
{
    if (gTileEnabled) {
        if (elevation == gElevation) {
            off_51D964(rect, elevation);
        }
    }
}

// 0x4B12D8
void tileWindowRefresh()
{
    if (gTileEnabled) {
        off_51D964(&gTileWindowRect, gElevation);
    }
}

// 0x4B12F8
int tileSetCenter(int tile, int flags)
{
    if (tile < 0 || tile >= gHexGridSize) {
        return -1;
    }

    if ((dword_51D958 & ((flags & TILE_SET_CENTER_FLAG_0x02) == 0)) != 0) {
        int tileScreenX;
        int tileScreenY;
        tileToScreenXY(tile, &tileScreenX, &tileScreenY, gElevation);

        int dudeScreenX;
        int dudeScreenY;
        tileToScreenXY(gDude->tile, &dudeScreenX, &dudeScreenY, gElevation);

        int dx = abs(dudeScreenX - tileScreenX);
        int dy = abs(dudeScreenY - tileScreenY);

        if (dx > abs(dudeScreenX - dword_66BDFC)
            || dy > abs(dudeScreenY - dword_66BDF8)) {
            if (dx >= 480 || dy >= 400) {
                return -1;
            }
        }
    }

    if ((dword_51D954 & ((flags & TILE_SET_CENTER_FLAG_0x02) == 0)) != 0) {
        if (sub_48BB44(tile, gElevation) == 0) {
            return -1;
        }
    }

    int v9 = gHexGridWidth - 1 - tile % gHexGridWidth;
    int v10 = tile / gHexGridWidth;

    if (dword_51D950) {
        if (v9 <= dword_66BBC4 || v9 >= dword_66BBCC || v10 <= dword_66BBC8 || v10 >= dword_66BBD0) {
            return -1;
        }
    }

    dword_66BE1C = v10;
    dword_66BDFC = (gTileWindowWidth - 32) / 2;
    dword_66BE18 = v9;
    dword_66BDF8 = (gTileWindowHeight - 16) / 2;

    if (v9 & 1) {
        dword_66BE18 -= 1;
        dword_66BDFC -= 32;
    }

    dword_66BDE8 = dword_66BE18 / 2;
    dword_66BDE4 = dword_66BE1C / 2;
    dword_66BDEC = dword_66BDFC - 16;
    dword_66BDF0 = dword_66BDF8 - 2;

    if (dword_66BE1C & 1) {
        dword_66BDF0 -= 12;
        dword_66BDEC -= 16;
    }

    gCenterTile = tile;

    if (flags & TILE_SET_CENTER_FLAG_0x01) {
        if (gTileEnabled) {
            off_51D964(&gTileWindowRect, gElevation);
        }
    }

    return 0;
}

// 0x4B1554
void sub_4B1554(Rect* rect, int elevation)
{
    Rect rectToUpdate;

    if (rectIntersection(rect, &gTileWindowRect, &rectToUpdate) == -1) {
        return;
    }

    bufferFill(gTileWindowBuffer + gTileWindowPitch * rectToUpdate.top + rectToUpdate.left,
        rectToUpdate.right - rectToUpdate.left + 1,
        rectToUpdate.bottom - rectToUpdate.top + 1,
        gTileWindowPitch,
        0);

    tileRenderFloorsInRect(&rectToUpdate, elevation);
    sub_4B2E98(&rectToUpdate, elevation);
    sub_489550(&rectToUpdate, elevation);
    tileRenderRoofsInRect(&rectToUpdate, elevation);
    sub_4897EC(&rectToUpdate, elevation);
    gTileWindowRefreshProc(&rectToUpdate);
}

// 0x4B15E8
void sub_4B15E8(Rect* rect, int elevation)
{
    Rect rectToUpdate;

    if (rectIntersection(rect, &gTileWindowRect, &rectToUpdate) == -1) {
        return;
    }

    tileRenderFloorsInRect(&rectToUpdate, elevation);
    sub_489550(&rectToUpdate, elevation);
    tileRenderRoofsInRect(&rectToUpdate, elevation);
    sub_4897EC(&rectToUpdate, elevation);
    gTileWindowRefreshProc(&rectToUpdate);
}

// 0x4B166C
int sub_4B166C()
{
    return dword_51D95C;
}

// 0x4B1674
int tileToScreenXY(int tile, int* screenX, int* screenY, int elevation)
{
    int v3;
    int v4;
    int v5;
    int v6;

    if (tile < 0 || tile >= gHexGridSize) {
        return -1;
    }

    v3 = gHexGridWidth - 1 - tile % gHexGridWidth;
    v4 = tile / gHexGridWidth;

    *screenX = dword_66BDFC;
    *screenY = dword_66BDF8;

    v5 = (v3 - dword_66BE18) / -2;
    *screenX += 48 * ((v3 - dword_66BE18) / 2);
    *screenY += 12 * v5;

    if (v3 & 1) {
        if (v3 <= dword_66BE18) {
            *screenX -= 16;
            *screenY += 12;
        } else {
            *screenX += 32;
        }
    }

    v6 = v4 - dword_66BE1C;
    *screenX += 16 * v6;
    *screenY += 12 * v6;

    return 0;
}

// 0x4B1754
int tileFromScreenXY(int screenX, int screenY, int elevation)
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

    v2 = screenY - dword_66BDF8;
    if (v2 >= 0) {
        v3 = v2 / 12;
    } else {
        v3 = (v2 + 1) / 12 - 1;
    }

    v4 = screenX - dword_66BDFC - 16 * v3;
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

    v10 = dword_66BE1C + v7;
    v11 = dword_66BE18 + v9;

    switch (byte_66B9C4[32 * v5 + v8]) {
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

    v12 = gHexGridWidth - 1 - v11;
    if (v12 >= 0 && v12 < gHexGridWidth && v10 >= 0 && v10 < gHexGridHeight) {
        return gHexGridWidth * v10 + v12;
    }

    return -1;
}

// tile_distance
// 0x4B185C
int tileDistanceBetween(int tile1, int tile2)
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
    tileToScreenXY(tile2, &x1, &y1, 0);

    v2 = tile1;
    for (i = 0; v2 != tile2; i++) {
        // TODO: Looks like inlined rotation_to_tile.
        int x2;
        int y2;
        tileToScreenXY(v2, &x2, &y2, 0);

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

        v2 += dword_66B594[v2 % gHexGridWidth & 1][v9];
    }

    return i;
}

// 0x4B1994
bool sub_4B1994(int tile1, int tile2)
{
    int x1;
    int y1;
    tileToScreenXY(tile1, &x1, &y1, 0);

    int x2;
    int y2;
    tileToScreenXY(tile2, &x2, &y2, 0);

    int dx = x2 - x1;
    int dy = y2 - y1;

    return (double)dx <= (double)dy * dbl_50E7C7;
}

// 0x4B1A00
bool sub_4B1A00(int tile1, int tile2)
{
    int x1;
    int y1;
    tileToScreenXY(tile1, &x1, &y1, 0);

    int x2;
    int y2;
    tileToScreenXY(tile2, &x2, &y2, 0);

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
int tileGetTileInDirection(int tile, int rotation, int distance)
{
    int newTile = tile;
    for (int index = 0; index < distance; index++) {
        if (sub_4B1D20(newTile)) {
            break;
        }

        int parity = (newTile % gHexGridWidth) & 1;
        newTile += dword_66B594[parity][rotation];
    }

    return newTile;
}

// rotation_to_tile
// 0x4B1ABC
int tileGetRotationTo(int tile1, int tile2)
{
    int x1;
    int y1;
    tileToScreenXY(tile1, &x1, &y1, 0);

    int x2;
    int y2;
    tileToScreenXY(tile2, &x2, &y2, 0);

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
int sub_4B1B84(int from, int to, int distance)
{
    if (distance <= 0 || from == to) {
        return from;
    }

    int fromX;
    int fromY;
    tileToScreenXY(from, &fromX, &fromY, 0);
    fromX += 16;
    fromY += 8;

    int toX;
    int toY;
    tileToScreenXY(to, &toX, &toY, 0);
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
            int tile = tileFromScreenXY(tileX, tileY, 0);
            if (tile != v28) {
                v6 += 1;
                if (v6 == distance || sub_4B1D20(tile)) {
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
            int tile = tileFromScreenXY(tileX, tileY, 0);
            if (tile != v28) {
                v6 += 1;
                if (v6 == distance || sub_4B1D20(tile)) {
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

    __assume(0);
}

// Probably returns true if tile is a border.
//
// 0x4B1D20
int sub_4B1D20(int tile)
{
    if (tile < 0 || tile >= gHexGridSize) {
        return 0;
    }

    if (tile < gHexGridWidth) {
        return 1;
    }

    if (tile >= gHexGridSize - gHexGridWidth) {
        return 1;
    }

    if (tile % gHexGridWidth == 0) {
        return 1;
    }

    if (tile % gHexGridWidth == gHexGridWidth - 1) {
        return 1;
    }

    return 0;
}

// 0x4B1D80
void sub_4B1D80()
{
    dword_51D954 = true;
}

// 0x4B1D8C
void sub_4B1D8C()
{
    dword_51D954 = false;
}

// 0x4B1D98
bool sub_4B1D98()
{
    return dword_51D954;
}

// 0x4B1DA0
void sub_4B1DA0()
{
    dword_51D958 = true;
}

// 0x4B1DAC
void sub_4B1DAC()
{
    dword_51D958 = false;
}

// 0x4B1DB8
bool sub_4B1DB8()
{
    return dword_51D958;
}

// 0x4B1DC0
int sub_4B1DC0(int a1, int* a2, int* a3, int elevation)
{
    int v5;
    int v6;
    int v7;
    int v8;
    int v9;

    if (a1 < 0 || a1 >= gSquareGridSize) {
        return -1;
    }

    v5 = gSquareGridWidth - 1 - a1 % gSquareGridWidth;
    v6 = a1 / gSquareGridWidth;
    v7 = dword_66BDE8;

    *a2 = dword_66BDEC;
    *a3 = dword_66BDF0;

    v8 = v5 - v7;
    *a2 += 48 * v8;
    *a3 -= 12 * v8;

    v9 = v6 - dword_66BDE4;
    *a2 += 32 * v9;
    *a3 += 24 * v9;

    return 0;
}

// 0x4B1E60
int squareTileToScreenXY(int a1, int* a2, int* a3, int elevation)
{
    int v5;
    int v6;
    int v7;
    int v8;
    int v9;
    int v10;

    if (a1 < 0 || a1 >= gSquareGridSize) {
        return -1;
    }

    v5 = gSquareGridWidth - 1 - a1 % gSquareGridWidth;
    v6 = a1 / gSquareGridWidth;
    v7 = dword_66BDE8;
    *a2 = dword_66BDEC;
    *a3 = dword_66BDF0;

    v8 = v5 - v7;
    *a2 += 48 * v8;
    *a3 -= 12 * v8;

    v9 = v6 - dword_66BDE4;
    *a2 += 32 * v9;
    v10 = 24 * v9 + *a3;
    *a3 = v10;
    *a3 = v10 - 96;

    return 0;
}

// 0x4B1F04
int sub_4B1F04(int x, int y, int elevation)
{
    int v5;
    int v6;

    sub_4B1F94(x, y, elevation, &v6, &v5);

    if (v6 >= 0 && v6 < gSquareGridWidth && v5 >= 0 && v5 < gSquareGridHeight) {
        return v6 + gSquareGridWidth * v5;
    }

    return -1;
}

// 0x4B1F94
void sub_4B1F94(int a1, int a2, int elevation, int* a3, int* a4)
{
    int v4;
    int v5;
    int v6;
    int v8;

    v4 = a1 - dword_66BDEC;
    v5 = a2 - dword_66BDF0 - 12;
    v6 = 3 * v4 - 4 * v5;
    *a3 = v6 >= 0 ? (v6 / 192) : ((v6 + 1) / 192 - 1);

    v8 = 4 * v5 + v4;
    *a4 = v8 >= 0
        ? ((v8 - ((v8 >> 31) << 7)) >> 7)
        : ((((v8 + 1) - (((v8 + 1) >> 31) << 7)) >> 7) - 1);

    *a3 += dword_66BDE8;
    *a4 += dword_66BDE4;

    *a3 = gSquareGridWidth - 1 - *a3;
}

// 0x4B203C
void sub_4B203C(int a1, int a2, int elevation, int* a3, int* a4)
{
    int v4;
    int v5;
    int v6;
    int v8;

    v4 = a1 - dword_66BDEC;
    v5 = a2 + 96 - dword_66BDF0 - 12;
    v6 = 3 * v4 - 4 * v5;

    *a3 = (v6 >= 0) ? (v6 / 192) : ((v6 + 1) / 192 - 1);

    v8 = 4 * v5 + v4;
    *a4 = (v8 >= 0)
        ? ((v8 - ((v8 >> 31) << 7)) >> 7)
        : ((((v8 + 1) - (((v8 + 1) >> 31) << 7)) >> 7) - 1);

    *a3 += dword_66BDE8;
    *a4 += dword_66BDE4;

    *a3 = gSquareGridWidth - 1 - *a3;
}

// 0x4B20E8
void tileRenderRoofsInRect(Rect* rect, int elevation)
{
    if (!dword_51D95C) {
        return;
    }

    int temp;
    int minY;
    int minX;
    int maxX;
    int maxY;

    sub_4B203C(rect->left, rect->top, elevation, &temp, &minY);
    sub_4B203C(rect->right, rect->top, elevation, &minX, &temp);
    sub_4B203C(rect->left, rect->bottom, elevation, &maxX, &temp);
    sub_4B203C(rect->right, rect->bottom, elevation, &temp, &maxY);

    if (minX < 0) {
        minX = 0;
    }

    if (minX >= gSquareGridWidth) {
        minX = gSquareGridWidth - 1;
    }

    if (minY < 0) {
        minY = 0;
    }

    // FIXME: Probably a bug - testing X, then changing Y.
    if (minX >= gSquareGridHeight) {
        minY = gSquareGridHeight - 1;
    }

    int light = lightGetLightLevel();

    int baseSquareTile = gSquareGridWidth * minY;

    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            int squareTile = baseSquareTile + x;
            int frmId = dword_66BE08[elevation]->field_0[squareTile];
            frmId >>= 16;
            if ((((frmId & 0xF000) >> 12) & 0x01) == 0) {
                int fid = buildFid(4, frmId & 0xFFF, 0, 0, 0);
                if (fid != buildFid(4, 1, 0, 0, 0)) {
                    int screenX;
                    int screenY;
                    squareTileToScreenXY(squareTile, &screenX, &screenY, elevation);
                    tileRenderRoof(fid, screenX, screenY, rect, light);
                }
            }
        }
        baseSquareTile += gSquareGridWidth;
    }
}

// 0x4B22D0
void sub_4B22D0(int a1, int a2, int elevation)
{
    while ((a1 >= 0 && a1 < gSquareGridWidth) && (a2 >= 0 && a2 < gSquareGridHeight)) {
        int squareTile = gSquareGridWidth * a2 + a1;
        int value = dword_66BE08[elevation]->field_0[squareTile];
        int upper = (value >> 16) & 0xFFFF;

        int id = upper & 0xFFF;
        if (buildFid(4, id, 0, 0, 0) == buildFid(4, 1, 0, 0, 0)) {
            break;
        }

        int flag = (upper & 0xF000) >> 12;
        if ((flag & 0x01) == 0) {
            break;
        }

        flag &= ~0x01;

        dword_66BE08[elevation]->field_0[squareTile] = (value & 0xFFFF) | (((flag << 12) | id) << 16);

        sub_4B22D0(a1 - 1, a2, elevation);
        sub_4B22D0(a1 + 1, a2, elevation);
        sub_4B22D0(a1, a2 - 1, elevation);

        a2++;
    }
}

// 0x4B23D4
void sub_4B23D4(int a1, int a2, int elevation, int a4)
{
    if (a4) {
        sub_4B22D0(a1, a2, elevation);
    } else {
        sub_4B23DC(a1, a2, elevation);
    }
}

// 0x4B23DC
void sub_4B23DC(int a1, int a2, int elevation)
{
    while ((a1 >= 0 && a1 < gSquareGridWidth) && (a2 >= 0 && a2 < gSquareGridHeight)) {
        int squareTile = gSquareGridWidth * a2 + a1;
        int value = dword_66BE08[elevation]->field_0[squareTile];
        int upper = (value >> 16) & 0xFFFF;

        int id = upper & 0xFFF;
        if (buildFid(4, id, 0, 0, 0) == buildFid(4, 1, 0, 0, 0)) {
            break;
        }

        int flag = (upper & 0xF000) >> 12;
        if ((flag & 0x03) != 0) {
            break;
        }

        flag |= 0x01;

        dword_66BE08[elevation]->field_0[squareTile] = (value & 0xFFFF) | (((flag << 12) | id) << 16);

        sub_4B23DC(a1 - 1, a2, elevation);
        sub_4B23DC(a1 + 1, a2, elevation);
        sub_4B23DC(a1, a2 - 1, elevation);

        a2++;
    }
}

// 0x4B24E0
void tileRenderRoof(int fid, int x, int y, Rect* rect, int light)
{
    CacheEntry* tileFrmHandle;
    Art* tileFrm = artLock(fid, &tileFrmHandle);
    if (tileFrm == NULL) {
        return;
    }

    int tileWidth = artGetWidth(tileFrm, 0, 0);
    int tileHeight = artGetHeight(tileFrm, 0, 0);

    Rect tileRect;
    tileRect.left = x;
    tileRect.top = y;
    tileRect.right = x + tileWidth - 1;
    tileRect.bottom = y + tileHeight - 1;

    if (rectIntersection(&tileRect, rect, &tileRect) == 0) {
        unsigned char* tileFrmBuffer = artGetFrameData(tileFrm, 0, 0);
        tileFrmBuffer += tileWidth * (tileRect.top - y) + (tileRect.left - x);

        CacheEntry* eggFrmHandle;
        Art* eggFrm = artLock(gEgg->fid, &eggFrmHandle);
        if (eggFrm != NULL) {
            int eggWidth = artGetWidth(eggFrm, 0, 0);
            int eggHeight = artGetHeight(eggFrm, 0, 0);

            int eggScreenX;
            int eggScreenY;
            tileToScreenXY(gEgg->tile, &eggScreenX, &eggScreenY, gEgg->elevation);

            eggScreenX += 16;
            eggScreenY += 8;

            eggScreenX += eggFrm->xOffsets[0];
            eggScreenY += eggFrm->yOffsets[0];

            eggScreenX += gEgg->x;
            eggScreenY += gEgg->y;

            Rect eggRect;
            eggRect.left = eggScreenX - eggWidth / 2;
            eggRect.top = eggScreenY - eggHeight + 1;
            eggRect.right = eggRect.left + eggWidth - 1;
            eggRect.bottom = eggScreenY;

            gEgg->sx = eggRect.left;
            gEgg->sy = eggRect.top;

            Rect intersectedRect;
            if (rectIntersection(&eggRect, &tileRect, &intersectedRect) == 0) {
                Rect rects[4];

                rects[0].left = tileRect.left;
                rects[0].top = tileRect.top;
                rects[0].right = tileRect.right;
                rects[0].bottom = intersectedRect.top - 1;

                rects[1].left = tileRect.left;
                rects[1].top = intersectedRect.top;
                rects[1].right = intersectedRect.left - 1;
                rects[1].bottom = intersectedRect.bottom;

                rects[2].left = intersectedRect.right + 1;
                rects[2].top = intersectedRect.top;
                rects[2].right = tileRect.right;
                rects[2].bottom = intersectedRect.bottom;

                rects[3].left = tileRect.left;
                rects[3].top = intersectedRect.bottom + 1;
                rects[3].right = tileRect.right;
                rects[3].bottom = tileRect.bottom;

                for (int i = 0; i < 4; i++) {
                    Rect* cr = &(rects[i]);
                    if (cr->left <= cr->right && cr->top <= cr->bottom) {
                        sub_48BEFC(tileFrmBuffer + tileWidth * (cr->top - tileRect.top) + (cr->left - tileRect.left),
                            cr->right - cr->left + 1,
                            cr->bottom - cr->top + 1,
                            tileWidth,
                            gTileWindowBuffer,
                            cr->left,
                            cr->top,
                            gTileWindowPitch,
                            light);
                    }
                }

                unsigned char* eggBuf = artGetFrameData(eggFrm, 0, 0);
                sub_48C03C(tileFrmBuffer + tileWidth * (intersectedRect.top - tileRect.top) + (intersectedRect.left - tileRect.left),
                    intersectedRect.right - intersectedRect.left + 1,
                    intersectedRect.bottom - intersectedRect.top + 1,
                    tileWidth,
                    gTileWindowBuffer + gTileWindowPitch * intersectedRect.top + intersectedRect.left,
                    gTileWindowPitch,
                    eggBuf + eggWidth * (intersectedRect.top - eggRect.top) + (intersectedRect.left - eggRect.left),
                    eggWidth,
                    light);
            } else {
                sub_48BEFC(tileFrmBuffer, tileRect.right - tileRect.left + 1, tileRect.bottom - tileRect.top + 1, tileWidth, gTileWindowBuffer, tileRect.left, tileRect.top, gTileWindowPitch, light);
            }

            artUnlock(eggFrmHandle);
        }
    }

    artUnlock(tileFrmHandle);
}

// 0x4B2944
void tileRenderFloorsInRect(Rect* rect, int elevation)
{
    int v9;
    int v8;
    int v7;
    int v10;
    int v11;

    sub_4B1F94(rect->left, rect->top, elevation, &v11, &v9);
    sub_4B1F94(rect->right, rect->top, elevation, &v10, &v11);
    sub_4B1F94(rect->left, rect->bottom, elevation, &v8, &v11);
    sub_4B1F94(rect->right, rect->bottom, elevation, &v11, &v7);

    if (v10 < 0) {
        v10 = 0;
    }

    if (v10 >= gSquareGridWidth) {
        v10 = gSquareGridWidth - 1;
    }

    if (v9 < 0) {
        v9 = 0;
    }

    if (v10 >= gSquareGridHeight) {
        v9 = gSquareGridHeight - 1;
    }

    lightGetLightLevel();

    v11 = gSquareGridWidth * v9;
    for (int v15 = v9; v15 <= v7; v15++) {
        for (int i = v10; i <= v8; i++) {
            int v3 = v11 + i;
            int frmId = dword_66BE08[elevation]->field_0[v3];
            if ((((frmId & 0xF000) >> 12) & 0x01) == 0) {
                int v12;
                int v13;
                sub_4B1DC0(v3, &v12, &v13, elevation);
                int fid = buildFid(4, frmId & 0xFFF, 0, 0, 0);
                tileRenderFloor(fid, v12, v13, rect);
            }
        }
        v11 += gSquareGridWidth;
    }
}

// 0x4B2B10
bool sub_4B2B10(int x, int y, int elevation)
{
    if (!dword_51D95C) {
        return false;
    }

    bool result = false;

    int tileX;
    int tileY;
    sub_4B203C(x, y, elevation, &tileX, &tileY);

    TileData* ptr = dword_66BE08[elevation];
    int idx = gSquareGridWidth * tileY + tileX;
    int upper = ptr->field_0[gSquareGridWidth * tileY + tileX] >> 16;
    int fid = buildFid(4, upper & 0xFFF, 0, 0, 0);
    if (fid != buildFid(4, 1, 0, 0, 0)) {
        if ((((upper & 0xF000) >> 12) & 1) == 0) {
            int fid = buildFid(4, upper & 0xFFF, 0, 0, 0);
            CacheEntry* handle;
            Art* art = artLock(fid, &handle);
            if (art != NULL) {
                unsigned char* data = artGetFrameData(art, 0, 0);
                if (data != NULL) {
                    int v18;
                    int v17;
                    squareTileToScreenXY(idx, &v18, &v17, elevation);

                    int width = artGetWidth(art, 0, 0);
                    if (data[width * (y - v17) + x - v18] != 0) {
                        result = true;
                    }
                }
                artUnlock(handle);
            }
        }
    }

    return result;
}

// 0x4B2E98
void sub_4B2E98(Rect* rect, int elevation)
{
    if (!dword_51D960) {
        return;
    }

    for (int y = rect->top - 12; y < rect->bottom + 12; y += 6) {
        for (int x = rect->left - 32; x < rect->right + 32; x += 16) {
            int tile = tileFromScreenXY(x, y, elevation);
            sub_4B2F4C(tile, elevation, rect);
        }
    }
}

// 0x4B2F4C
void sub_4B2F4C(int tile, int elevation, Rect* rect)
{
    if (tile == -1) {
        return;
    }

    int x;
    int y;
    tileToScreenXY(tile, &x, &y, elevation);

    Rect r;
    r.left = x;
    r.top = y;
    r.right = x + 32 - 1;
    r.bottom = y + 16 - 1;

    if (rectIntersection(&r, rect, &r) == -1) {
        return;
    }

    if (sub_48B848(NULL, tile, elevation) != NULL) {
        blitBufferToBufferTrans(byte_66B5C4 + 32 * (r.top - y) + (r.left - x),
            r.right - r.left + 1,
            r.bottom - r.top + 1,
            32,
            gTileWindowBuffer + gTileWindowPitch * r.top + r.left,
            gTileWindowPitch);
        return;
    }

    if (sub_48B7F8(tile, elevation)) {
        blitBufferToBufferTrans(byte_66B7C4 + 32 * (r.top - y) + (r.left - x),
            r.right - r.left + 1,
            r.bottom - r.top + 1,
            32,
            gTileWindowBuffer + gTileWindowPitch * r.top + r.left,
            gTileWindowPitch);
        return;
    }

    sub_48BDD8(byte_66B7C4 + 32 * (r.top - y) + (r.left - x),
        r.right - r.left + 1,
        r.bottom - r.top + 1,
        32,
        gTileWindowBuffer + gTileWindowPitch * r.top + r.left,
        0,
        0,
        gTileWindowPitch,
        dword_519780,
        byte_660FA0);
}

// 0x4B30C4
void tileRenderFloor(int fid, int x, int y, Rect* rect)
{
    if (artIsObjectTypeHidden((fid & 0xF000000) >> 24) != 0) {
        return;
    }

    CacheEntry* cacheEntry;
    Art* art = artLock(fid, &cacheEntry);
    if (art == NULL) {
        return;
    }

    int elev = gElevation;
    int left = rect->left;
    int top = rect->top;
    int width = rect->right - rect->left + 1;
    int height = rect->bottom - rect->top + 1;
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

    if (left + width > gTileWindowWidth) {
        width = gTileWindowWidth - left;
    }

    if (top + height > gTileWindowHeight) {
        height = gTileWindowHeight - top;
    }

    if (x >= gTileWindowWidth || x > rect->right || y >= gTileWindowHeight || y > rect->bottom) goto out;

    frameWidth = artGetWidth(art, 0, 0);
    frameHeight = artGetHeight(art, 0, 0);

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

    v15 = tileFromScreenXY(savedX, savedY + 13, gElevation);
    if (v15 != -1) {
        int v17 = lightGetLightLevel();
        for (int i = v15 & 1; i < 10; i++) {
            // NOTE: calling sub_47A980 two times, probably a result of using __min kind macro
            int v21 = sub_47A980(elev, v15 + stru_51DA6C[i].field_4);
            if (v21 <= v17) {
                v21 = v17;
            }

            stru_51DA6C[i].field_C = v21;
        }

        int v23 = 0;
        for (int i = 0; i < 9; i++) {
            if (stru_51DA6C[i + 1].field_C != stru_51DA6C[i].field_C) {
                break;
            }

            v23++;
        }

        if (v23 == 9) {
            unsigned char* buf = artGetFrameData(art, 0, 0);
            sub_48BEFC(buf + frameWidth * v78 + v79, v77, v76, frameWidth, gTileWindowBuffer, x, y, gTileWindowPitch, stru_51DA6C[0].field_C);
            goto out;
        }

        for (int i = 0; i < 5; i++) {
            STRUCT_51DB0C* ptr_51DB0C = &(stru_51DB0C[i]);
            int v32 = stru_51DA6C[ptr_51DB0C->field_8].field_C;
            int v33 = stru_51DA6C[ptr_51DB0C->field_8].field_0;
            int v34 = stru_51DA6C[ptr_51DB0C->field_4].field_C - stru_51DA6C[ptr_51DB0C->field_0].field_C;
            // TODO: Probably wrong.
            int v35 = v34 / 32;
            int v36 = (stru_51DA6C[ptr_51DB0C->field_0].field_C - v32) / 13;
            int* v37 = &(dword_668224[v33]);
            if (v35 != 0) {
                if (v36 != 0) {
                    for (int i = 0; i < 13; i++) {
                        int v41 = v32;
                        int v42 = stru_51D99C[i].field_4;
                        v37 += stru_51D99C[i].field_0;
                        for (int j = 0; j < v42; j++) {
                            *v37++ = v41;
                            v41 += v35;
                        }
                        v32 += v36;
                    }
                } else {
                    for (int i = 0; i < 13; i++) {
                        int v38 = v32;
                        int v39 = stru_51D99C[i].field_4;
                        v37 += stru_51D99C[i].field_0;
                        for (int j = 0; j < v39; j++) {
                            *v37++ = v38;
                            v38 += v35;
                        }
                    }
                }
            } else {
                if (v36 != 0) {
                    for (int i = 0; i < 13; i++) {
                        int v46 = stru_51D99C[i].field_4;
                        v37 += stru_51D99C[i].field_0;
                        for (int j = 0; j < v46; j++) {
                            *v37++ = v32;
                        }
                        v32 += v36;
                    }
                } else {
                    for (int i = 0; i < 13; i++) {
                        int v44 = stru_51D99C[i].field_4;
                        v37 += stru_51D99C[i].field_0;
                        for (int j = 0; j < v44; j++) {
                            *v37++ = v32;
                        }
                    }
                }
            }
        }

        for (int i = 0; i < 5; i++) {
            STRUCT_51DB48* ptr_51DB48 = &(stru_51DB48[i]);
            int v50 = stru_51DA6C[ptr_51DB48->field_0].field_C;
            int v51 = stru_51DA6C[ptr_51DB48->field_0].field_0;
            int v52 = stru_51DA6C[ptr_51DB48->field_8].field_C - v50;
            // TODO: Probably wrong.
            int v53 = v52 / 32;
            int v54 = (stru_51DA6C[ptr_51DB48->field_4].field_C - v50) / 13;
            int* v55 = &(dword_668224[v51]);
            if (v53 != 0) {
                if (v54 != 0) {
                    for (int i = 0; i < 13; i++) {
                        int v59 = v50;
                        int v60 = stru_51DA04[i].field_4;
                        v55 += stru_51DA04[i].field_0;
                        for (int j = 0; j < v60; j++) {
                            *v55++ = v59;
                            v59 += v53;
                        }
                        v50 += v54;
                    }
                } else {
                    for (int i = 0; i < 13; i++) {
                        int v56 = v50;
                        int v57 = stru_51DA04[i].field_4;
                        v55 += stru_51DA04[i].field_0;
                        for (int j = 0; j < v57; j++) {
                            *v55++ = v56;
                            v56 += v53;
                        }
                    }
                }
            } else {
                if (v54 != 0) {
                    for (int i = 0; i < 13; i++) {
                        int v64 = stru_51DA04[i].field_4;
                        v55 += stru_51DA04[i].field_0;
                        for (int j = 0; j < v64; j++) {
                            *v55++ = v50;
                        }
                        v50 += v54;
                    }
                } else {
                    for (int i = 0; i < 13; i++) {
                        int v62 = stru_51DA04[i].field_4;
                        v55 += stru_51DA04[i].field_0;
                        for (int j = 0; j < v62; j++) {
                            *v55++ = v50;
                        }
                    }
                }
            }
        }

        unsigned char* v66 = gTileWindowBuffer + gTileWindowPitch * y + x;
        unsigned char* v67 = artGetFrameData(art, 0, 0) + frameWidth * v78 + v79;
        int* v68 = &(dword_668224[160 + 80 * v78]) + v79;
        int v86 = frameWidth - v77;
        int v85 = gTileWindowPitch - v77;
        int v87 = 80 - v77;

        while (--v76 != -1) {
            for (int kk = 0; kk < v77; kk++) {
                if (*v67 != 0) {
                    int t = (*v67 << 8) + (*v68 >> 9);
                    *v66 = byte_6838D0[t];
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

    artUnlock(cacheEntry);
}

// 0x4B372C
int sub_4B372C(int from, int to, int* tiles, int tilesCapacity)
{
    if (tilesCapacity <= 1) {
        return 0;
    }

    int count = 0;

    int fromX;
    int fromY;
    tileToScreenXY(from, &fromX, &fromY, gElevation);
    fromX += 16;
    fromY += 8;

    int toX;
    int toY;
    tileToScreenXY(to, &toX, &toY, gElevation);
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
            int tile = tileFromScreenXY(tileX, tileY, gElevation);
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
            int tile = tileFromScreenXY(tileX, tileY, gElevation);
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
int sub_4B3924(int tile, int flags)
{
    if (tile == gCenterTile) {
        return -1;
    }

    int oldCenterTile = gCenterTile;

    int v9[200];
    int count = sub_4B372C(gCenterTile, tile, v9, 200);
    if (count == 0) {
        return -1;
    }

    int index = 1;
    for (; index < count; index++) {
        if (tileSetCenter(v9[index], 0) == -1) {
            break;
        }
    }

    int rc = 0;
    if ((flags & 0x01) != 0) {
        if (index != count) {
            tileSetCenter(oldCenterTile, 0);
            rc = -1;
        }
    }

    if ((flags & 0x02) != 0) {
        if (gTileEnabled) {
            off_51D964(&gTileWindowRect, gElevation);
        }
    }

    return rc;
}