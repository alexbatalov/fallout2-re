#ifndef COLOR_H
#define COLOR_H

#include "memory_defs.h"

#include <stdbool.h>
#include <stdlib.h>

typedef void(ColorTransitionCallback)();

typedef int(ColorPaletteFileOpenProc)(const char* path, int mode);
typedef int(ColorPaletteFileReadProc)(int fd, void* buffer, size_t size);
typedef int(ColorPaletteCloseProc)(int fd);

extern char byte_50F930[];
extern char byte_50F95C[];

extern char* off_51DF10;
extern bool dword_51DF14;
extern double gBrightness;
extern ColorTransitionCallback* gColorPaletteTransitionCallback;
extern MallocProc* gColorPaletteMallocProc;
extern ReallocProc* gColorPaletteReallocProc;
extern FreeProc* gColorPaletteFreeProc;
extern void (*off_51DF30)();
extern unsigned char stru_51DF34[768];

extern unsigned char stru_673090[256 * 3];
extern unsigned char byte_673390[64];
extern unsigned char* dword_6733D0[256];
extern unsigned char byte_6737D0[256];
extern unsigned char byte_6738D0[65536];
extern unsigned char byte_6838D0[65536];
extern unsigned char byte_6938D0[65536];
extern unsigned char byte_6A38D0[32768];
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
int sub_4C72B4(int a1, int a2);
int sub_4C72E0(int a1);
void colorPaletteFadeBetween(unsigned char* oldPalette, unsigned char* newPalette, int steps);
void colorPaletteSetTransitionCallback(ColorTransitionCallback* callback);
void sub_4C73E4(unsigned char* palette);
unsigned char* sub_4C7420();
void sub_4C7428(unsigned char* a1, int a2, int a3);
void sub_4C7550(int a1);
void sub_4C7658();
void sub_4C769C(int a1);
bool colorPaletteLoad(const char* path);
char* sub_4C7AB4();
void sub_4C7B44(unsigned char* ptr, unsigned char ch);
void sub_4C7D90();
unsigned char* sub_4C7DC0(int ch);
void sub_4C7E20(int a1);
void colorPaletteSetMemoryProcs(MallocProc* mallocProc, ReallocProc* reallocProc, FreeProc* freeProc);
void colorSetBrightness(double value);
bool sub_4C89CC();
void sub_4C8A18();

#endif /* COLOR_H */