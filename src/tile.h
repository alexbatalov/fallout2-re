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
void sub_4B11E4(int a1, int a2, int a3, int a4);
void sub_4B129C();
void tileReset();
void tileExit();
void tileDisable();
void tileEnable();
void tileWindowRefreshRect(Rect* rect, int elevation);
void tileWindowRefresh();
int tileSetCenter(int a1, int a2);
void sub_4B1554(Rect* rect, int elevation);
void sub_4B15E8(Rect* rect, int elevation);
int sub_4B166C();
int tileToScreenXY(int tile, int* x, int* y, int elevation);
int tileFromScreenXY(int x, int y, int elevation);
int tileDistanceBetween(int a1, int a2);
bool sub_4B1994(int tile1, int tile2);
bool sub_4B1A00(int tile1, int tile2);
int tileGetTileInDirection(int tile, int rotation, int distance);
int tileGetRotationTo(int a1, int a2);
int sub_4B1B84(int from, int to, int distance);
int sub_4B1D20(int a1);
void sub_4B1D80();
void sub_4B1D8C();
bool sub_4B1D98();
void sub_4B1DA0();
void sub_4B1DAC();
bool sub_4B1DB8();
int sub_4B1DC0(int a1, int* a2, int* a3, int elevation);
int squareTileToScreenXY(int a1, int* a2, int* a3, int elevation);
int sub_4B1F04(int x, int y, int elevation);
void sub_4B1F94(int a1, int a2, int elevation, int* a3, int* a4);
void sub_4B203C(int a1, int a2, int elevation, int* a3, int* a4);
void tileRenderRoofsInRect(Rect* rect, int elevation);
void sub_4B22D0(int x, int y, int elevation);
void sub_4B23D4(int x, int y, int elevation, int a4);
void sub_4B23DC(int x, int y, int elevation);
void tileRenderRoof(int fid, int x, int y, Rect* rect, int light);
void tileRenderFloorsInRect(Rect* rect, int elevation);
bool sub_4B2B10(int x, int y, int elevation);
void sub_4B2E98(Rect* rect, int elevation);
void sub_4B2F4C(int tile, int elevation, Rect* rect);
void tileRenderFloor(int fid, int x, int y, Rect* rect);
int sub_4B372C(int currentCenterTile, int newCenterTile, int* tiles, int tilesCapacity);
int sub_4B3924(int tile, int flags);

#endif /* TILE_H */
