#ifndef FALLOUT_PLIB_COLOR_COLOR_H_
#define FALLOUT_PLIB_COLOR_COLOR_H_

#include <stdbool.h>
#include <stdlib.h>

#include "memory_defs.h"

#define COLOR_PALETTE_STACK_CAPACITY 16

typedef unsigned char Color;
typedef long ColorRGB;
typedef unsigned char ColorIndex;

typedef const char*(ColorNameMangleFunc)(const char*);
typedef void(fade_bk_func)();

typedef int(ColorOpenFunc)(const char* path, int mode);
typedef int(ColorReadFunc)(int fd, void* buffer, size_t size);
typedef int(ColorCloseFunc)(int fd);

typedef struct ColorPaletteStackEntry {
    unsigned char mappedColors[256];
    unsigned char cmap[768];
    unsigned char colorTable[32768];
} ColorPaletteStackEntry;

extern unsigned char cmap[768];

extern unsigned char mappedColor[256];
extern Color colorMixAddTable[256][256];
extern unsigned char intensityColorTable[256][256];
extern Color colorMixMulTable[256][256];
extern unsigned char colorTable[32768];

void colorInitIO(ColorOpenFunc* openProc, ColorReadFunc* readProc, ColorCloseFunc* closeProc);
void colorSetNameMangler(ColorNameMangleFunc* c);
Color colorMixAdd(Color a, Color b);
Color colorMixMul(Color a, Color b);
int calculateColor(int a1, int a2);
Color RGB2Color(ColorRGB c);
int Color2RGB(int a1);
void fadeSystemPalette(unsigned char* oldPalette, unsigned char* newPalette, int steps);
void colorSetFadeBkFunc(fade_bk_func* callback);
void setBlackSystemPalette();
void setSystemPalette(unsigned char* palette);
unsigned char* getSystemPalette();
void setSystemPaletteEntries(unsigned char* a1, int a2, int a3);
void setSystemPaletteEntry(int entry, unsigned char r, unsigned char g, unsigned char b);
void getSystemPaletteEntry(int entry, unsigned char* r, unsigned char* g, unsigned char* b);
bool loadColorTable(const char* path);
char* colorError();
void setColorPalette(unsigned char* pal);
void setColorPaletteEntry(int entry, unsigned char r, unsigned char g, unsigned char b);
void getColorPaletteEntry(int entry, unsigned char* r, unsigned char* g, unsigned char* b);
unsigned char* getColorBlendTable(int ch);
void freeColorBlendTable(int a1);
void colorRegisterAlloc(MallocProc* mallocProc, ReallocProc* reallocProc, FreeProc* freeProc);
void colorGamma(double value);
double colorGetGamma();
int colorMappedColor(ColorIndex i);
bool colorPushColorPalette();
bool colorPopColorPalette();
bool initColors();
void colorsClose();
unsigned char* getColorPalette();

#endif /* FALLOUT_PLIB_COLOR_COLOR_H_ */
