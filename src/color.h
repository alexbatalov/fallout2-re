#ifndef COLOR_H
#define COLOR_H

#include <stdbool.h>
#include <stdlib.h>

#include "memory_defs.h"

#define COLOR_PALETTE_STACK_CAPACITY 16

typedef const char*(ColorFileNameManger)(const char*);
typedef void(ColorTransitionCallback)();

typedef int(ColorPaletteFileOpenProc)(const char* path, int mode);
typedef int(ColorPaletteFileReadProc)(int fd, void* buffer, size_t size);
typedef int(ColorPaletteCloseProc)(int fd);

typedef struct ColorPaletteStackEntry {
    unsigned char mappedColors[256];
    unsigned char cmap[768];
    unsigned char colorTable[32768];
} ColorPaletteStackEntry;

extern char _aColor_cNoError[];
extern char _aColor_cColorTa[];
extern char _aColor_cColorpa[];
extern char aColor_cColor_0[];

extern char* _errorStr;
extern bool _colorsInited;
extern double gBrightness;
extern ColorTransitionCallback* gColorPaletteTransitionCallback;
extern MallocProc* gColorPaletteMallocProc;
extern ReallocProc* gColorPaletteReallocProc;
extern FreeProc* gColorPaletteFreeProc;
extern ColorFileNameManger* gColorFileNameMangler;
extern unsigned char _cmap[768];

extern ColorPaletteStackEntry* gColorPaletteStack[COLOR_PALETTE_STACK_CAPACITY];
extern unsigned char _systemCmap[256 * 3];
extern unsigned char _currentGammaTable[64];
extern unsigned char* _blendTable[256];
extern unsigned char _mappedColor[256];
extern unsigned char _colorMixAddTable[65536];
extern unsigned char _intensityColorTable[65536];
extern unsigned char _colorMixMulTable[65536];
extern unsigned char _colorTable[32768];
extern int gColorPaletteStackSize;
extern ColorPaletteFileReadProc* gColorPaletteFileReadProc;
extern ColorPaletteCloseProc* gColorPaletteFileCloseProc;
extern ColorPaletteFileOpenProc* gColorPaletteFileOpenProc;

int colorPaletteFileOpen(const char* filePath, int flags);
int colorPaletteFileRead(int fd, void* buffer, size_t size);
int colorPaletteFileClose(int fd);
void colorPaletteSetFileIO(ColorPaletteFileOpenProc* openProc, ColorPaletteFileReadProc* readProc, ColorPaletteCloseProc* closeProc);
void* colorPaletteMallocDefaultImpl(size_t size);
void* colorPaletteReallocDefaultImpl(void* ptr, size_t size);
void colorPaletteFreeDefaultImpl(void* ptr);
int _calculateColor(int a1, int a2);
int _Color2RGB_(int a1);
void colorPaletteFadeBetween(unsigned char* oldPalette, unsigned char* newPalette, int steps);
void colorPaletteSetTransitionCallback(ColorTransitionCallback* callback);
void _setSystemPalette(unsigned char* palette);
unsigned char* _getSystemPalette();
void _setSystemPaletteEntries(unsigned char* a1, int a2, int a3);
void _setIntensityTableColor(int a1);
void _setIntensityTables();
void _setMixTableColor(int a1);
bool colorPaletteLoad(const char* path);
char* _colorError();
void _buildBlendTable(unsigned char* ptr, unsigned char ch);
void _rebuildColorBlendTables();
unsigned char* _getColorBlendTable(int ch);
void _freeColorBlendTable(int a1);
void colorPaletteSetMemoryProcs(MallocProc* mallocProc, ReallocProc* reallocProc, FreeProc* freeProc);
void colorSetBrightness(double value);
bool colorPushColorPalette();
bool colorPopColorPalette();
bool _initColors();
void _colorsClose();

#endif /* COLOR_H */
