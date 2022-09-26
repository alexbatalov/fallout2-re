#ifndef COLOR_H
#define COLOR_H

#include <stdbool.h>
#include <stdlib.h>

#include "memory_defs.h"

#define COLOR_PALETTE_STACK_CAPACITY 16

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
extern unsigned char colorMixAddTable[65536];
extern unsigned char intensityColorTable[65536];
extern unsigned char colorMixMulTable[65536];
extern unsigned char colorTable[32768];

void colorInitIO(ColorOpenFunc* openProc, ColorReadFunc* readProc, ColorCloseFunc* closeProc);
int calculateColor(int a1, int a2);
int Color2RGB(int a1);
void fadeSystemPalette(unsigned char* oldPalette, unsigned char* newPalette, int steps);
void colorSetFadeBkFunc(fade_bk_func* callback);
void setSystemPalette(unsigned char* palette);
unsigned char* getSystemPalette();
void setSystemPaletteEntries(unsigned char* a1, int a2, int a3);
bool loadColorTable(const char* path);
char* colorError();
unsigned char* getColorBlendTable(int ch);
void freeColorBlendTable(int a1);
void colorRegisterAlloc(MallocProc* mallocProc, ReallocProc* reallocProc, FreeProc* freeProc);
void colorGamma(double value);
bool colorPushColorPalette();
bool colorPopColorPalette();
bool initColors();
void colorsClose();
unsigned char* getColorPalette();

#endif /* COLOR_H */
