#ifndef TILE_H
#define TILE_H

#include "geometry.h"
#include "map.h"
#include "obj_types.h"

#include <stdbool.h>

#define TILE_SET_CENTER_FLAG_0x01 0x01
#define TILE_SET_CENTER_FLAG_0x02 0x02

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

typedef void(TileWindowRefreshProc)(Rect* rect);
typedef void(TileWindowRefreshElevationProc)(Rect* rect, int elevation);

extern double const dbl_50E7C7;

extern bool dword_51D950;
extern bool dword_51D954;
extern bool dword_51D958;
extern int dword_51D95C;
extern int dword_51D960;
extern TileWindowRefreshElevationProc* off_51D964;
extern bool gTileEnabled;
extern const int dword_51D96C[6];
extern const int dword_51D984[6];
extern STRUCT_51D99C stru_51D99C[13];
extern STRUCT_51DA04 stru_51DA04[13];
extern STRUCT_51DA6C stru_51DA6C[10];
extern STRUCT_51DB0C stru_51DB0C[5];
extern STRUCT_51DB48 stru_51DB48[5];

extern int dword_668224[3280];
extern int dword_66B564[2][6];
extern int dword_66B594[2][6];
extern unsigned char byte_66B5C4[512];
extern unsigned char byte_66B7C4[512];
extern unsigned char byte_66B9C4[512];
extern int dword_66BBC4;
extern int dword_66BBC8;
extern int dword_66BBCC;
extern int dword_66BBD0;
extern Rect gTileWindowRect;
extern unsigned char byte_66BBE4[512];
extern int dword_66BDE4;
extern int dword_66BDE8;
extern int dword_66BDEC;
extern int dword_66BDF0;
extern TileWindowRefreshProc* gTileWindowRefreshProc;
extern int dword_66BDF8;
extern int dword_66BDFC;
extern int gSquareGridSize;
extern int gHexGridWidth;
extern TileData** dword_66BE08;
extern unsigned char* gTileWindowBuffer;
extern int gHexGridHeight;
extern int gTileWindowHeight;
extern int dword_66BE18;
extern int dword_66BE1C;
extern int gHexGridSize;
extern int gSquareGridHeight;
extern int gTileWindowPitch;
extern int gSquareGridWidth;
extern int gTileWindowWidth;
extern int gCenterTile;

int tileInit(TileData** a1, int squareGridWidth, int squareGridHeight, int hexGridWidth, int hexGridHeight, unsigned char* buf, int windowWidth, int windowHeight, int windowPitch, TileWindowRefreshProc* windowRefreshProc);
void tile_set_border(int a1, int a2, int a3, int a4);
void sub_4B129C();
void tileReset();
void tileExit();
void tileDisable();
void tileEnable();
void tileWindowRefreshRect(Rect* rect, int elevation);
void tileWindowRefresh();
int tileSetCenter(int a1, int a2);
void refresh_mapper(Rect* rect, int elevation);
void refresh_game(Rect* rect, int elevation);
int tile_roof_visible();
int tileToScreenXY(int tile, int* x, int* y, int elevation);
int tileFromScreenXY(int x, int y, int elevation);
int tileDistanceBetween(int a1, int a2);
bool tile_in_front_of(int tile1, int tile2);
bool tile_to_right_of(int tile1, int tile2);
int tileGetTileInDirection(int tile, int rotation, int distance);
int tileGetRotationTo(int a1, int a2);
int tile_num_beyond(int from, int to, int distance);
int tile_on_edge(int a1);
void tile_enable_scroll_blocking();
void tile_disable_scroll_blocking();
bool tile_get_scroll_blocking();
void tile_enable_scroll_limiting();
void tile_disable_scroll_limiting();
bool tile_get_scroll_limiting();
int square_coord(int a1, int* a2, int* a3, int elevation);
int squareTileToScreenXY(int a1, int* a2, int* a3, int elevation);
int square_num(int x, int y, int elevation);
void square_xy(int a1, int a2, int elevation, int* a3, int* a4);
void square_xy_roof(int a1, int a2, int elevation, int* a3, int* a4);
void tileRenderRoofsInRect(Rect* rect, int elevation);
void roof_fill_on(int x, int y, int elevation);
void tile_fill_roof(int x, int y, int elevation, int a4);
void sub_4B23DC(int x, int y, int elevation);
void tileRenderRoof(int fid, int x, int y, Rect* rect, int light);
void tileRenderFloorsInRect(Rect* rect, int elevation);
bool square_roof_intersect(int x, int y, int elevation);
void grid_render(Rect* rect, int elevation);
void draw_grid(int tile, int elevation, Rect* rect);
void tileRenderFloor(int fid, int x, int y, Rect* rect);
int tile_make_line(int currentCenterTile, int newCenterTile, int* tiles, int tilesCapacity);
int tile_scroll_to(int tile, int flags);

#endif /* TILE_H */
