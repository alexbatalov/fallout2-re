#include "color.h"

#include <math.h>
#include <string.h>

#include "core.h"

static int colorOpen(const char* filePath, int flags);
static int colorRead(int fd, void* buffer, size_t size);
static int colorClose(int fd);
static void* defaultMalloc(size_t size);
static void* defaultRealloc(void* ptr, size_t size);
static void defaultFree(void* ptr);
static void setIntensityTableColor(int a1);
static void setIntensityTables();
static void setMixTableColor(int a1);
static void buildBlendTable(unsigned char* ptr, unsigned char ch);
static void rebuildColorBlendTables();

// 0x50F930
static char _aColor_cNoError[] = "color.c: No errors\n";

// 0x50F95C
static char _aColor_cColorTa[] = "color.c: color table not found\n";

// 0x50F984
static char _aColor_cColorpa[] = "color.c: colorpalettestack overflow";

// 0x50F9AC
static char aColor_cColor_0[] = "color.c: colorpalettestack underflow";

// 0x51DF10
static char* errorStr = _aColor_cNoError;

// 0x51DF14
static bool colorsInited = false;

// 0x51DF18
static double currentGamma = 1.0;

// 0x51DF20
static fade_bk_func* colorFadeBkFuncP = NULL;

// 0x51DF24
static MallocProc* mallocPtr = defaultMalloc;

// 0x51DF28
static ReallocProc* reallocPtr = defaultRealloc;

// 0x51DF2C
static FreeProc* freePtr = defaultFree;

// 0x51DF30
static ColorNameMangleFunc* colorNameMangler = NULL;

// 0x51DF34
unsigned char cmap[768] = {
    0x3F, 0x3F, 0x3F
};

// 0x673050
static ColorPaletteStackEntry* colorPaletteStack[COLOR_PALETTE_STACK_CAPACITY];

// 0x673090
static unsigned char systemCmap[256 * 3];

// 0x673390
static unsigned char currentGammaTable[64];

// 0x6733D0
static unsigned char* blendTable[256];

// 0x6737D0
unsigned char mappedColor[256];

// 0x6738D0
unsigned char colorMixAddTable[65536];

// 0x6838D0
unsigned char intensityColorTable[65536];

// 0x6938D0
unsigned char colorMixMulTable[65536];

// 0x6A38D0
unsigned char colorTable[32768];

// 0x6AB8D0
static int tos;

// 0x6AB928
static ColorReadFunc* readFunc;

// 0x6AB92C
static ColorCloseFunc* closeFunc;

// 0x6AB930
static ColorOpenFunc* openFunc;

// NOTE: Inlined.
//
// 0x4C7200
static int colorOpen(const char* filePath, int flags)
{
    if (openFunc != NULL) {
        return openFunc(filePath, flags);
    }

    return -1;
}

// NOTE: Inlined.
//
// 0x4C7218
static int colorRead(int fd, void* buffer, size_t size)
{
    if (readFunc != NULL) {
        return readFunc(fd, buffer, size);
    }

    return -1;
}

// NOTE: Inlined.
//
// 0x4C7230
static int colorClose(int fd)
{
    if (closeFunc != NULL) {
        return closeFunc(fd);
    }

    return -1;
}

// 0x4C7248
void colorInitIO(ColorOpenFunc* openProc, ColorReadFunc* readProc, ColorCloseFunc* closeProc)
{
    openFunc = openProc;
    readFunc = readProc;
    closeFunc = closeProc;
}

// 0x4C725C
static void* defaultMalloc(size_t size)
{
    return malloc(size);
}

// 0x4C7264
static void* defaultRealloc(void* ptr, size_t size)
{
    return realloc(ptr, size);
}

// 0x4C726C
static void defaultFree(void* ptr)
{
    free(ptr);
}

// 0x4C72B4
int calculateColor(int a1, int a2)
{
    int v1 = (a1 >> 9) + ((a2 & 0xFF) << 8);
    return intensityColorTable[v1];
}

// 0x4C72E0
int Color2RGB(int a1)
{
    int v1, v2, v3;

    v1 = cmap[3 * a1] >> 1;
    v2 = cmap[3 * a1 + 1] >> 1;
    v3 = cmap[3 * a1 + 2] >> 1;

    return (((v1 << 5) | v2) << 5) | v3;
}

// Performs animated palette transition.
//
// 0x4C7320
void fadeSystemPalette(unsigned char* oldPalette, unsigned char* newPalette, int steps)
{
    for (int step = 0; step < steps; step++) {
        unsigned char palette[768];

        for (int index = 0; index < 768; index++) {
            palette[index] = oldPalette[index] - (oldPalette[index] - newPalette[index]) * step / steps;
        }

        if (colorFadeBkFuncP != NULL) {
            if (step % 128 == 0) {
                colorFadeBkFuncP();
            }
        }

        setSystemPalette(palette);
    }

    setSystemPalette(newPalette);
}

// 0x4C73D4
void colorSetFadeBkFunc(fade_bk_func* callback)
{
    colorFadeBkFuncP = callback;
}

// 0x4C73E4
void setSystemPalette(unsigned char* palette)
{
    unsigned char newPalette[768];

    for (int index = 0; index < 768; index++) {
        newPalette[index] = currentGammaTable[palette[index]];
        systemCmap[index] = palette[index];
    }

    directDrawSetPalette(newPalette);
}

// 0x4C7420
unsigned char* getSystemPalette()
{
    return systemCmap;
}

// 0x4C7428
void setSystemPaletteEntries(unsigned char* palette, int start, int end)
{
    unsigned char newPalette[768];

    int length = end - start + 1;
    for (int index = 0; index < length; index++) {
        newPalette[index * 3] = currentGammaTable[palette[index * 3]];
        newPalette[index * 3 + 1] = currentGammaTable[palette[index * 3 + 1]];
        newPalette[index * 3 + 2] = currentGammaTable[palette[index * 3 + 2]];

        systemCmap[start * 3 + index * 3] = palette[index * 3];
        systemCmap[start * 3 + index * 3 + 1] = palette[index * 3 + 1];
        systemCmap[start * 3 + index * 3 + 2] = palette[index * 3 + 2];
    }

    directDrawSetPaletteInRange(newPalette, start, end - start + 1);
}

// 0x4C7550
static void setIntensityTableColor(int a1)
{
    int v1, v2, v3, v4, v5, v6, v7, v8, v9, v10;

    v5 = 0;
    v10 = a1 << 8;

    for (int index = 0; index < 128; index++) {
        v1 = (Color2RGB(a1) & 0x7C00) >> 10;
        v2 = (Color2RGB(a1) & 0x3E0) >> 5;
        v3 = (Color2RGB(a1) & 0x1F);

        v4 = (((v1 * v5) >> 16) << 10) | (((v2 * v5) >> 16) << 5) | ((v3 * v5) >> 16);
        intensityColorTable[index + v10] = colorTable[v4];

        v6 = v1 + (((0x1F - v1) * v5) >> 16);
        v7 = v2 + (((0x1F - v2) * v5) >> 16);
        v8 = v3 + (((0x1F - v3) * v5) >> 16);

        v9 = (v6 << 10) | (v7 << 5) | v8;
        intensityColorTable[0x7F + index + 1 + v10] = colorTable[v9];

        v5 += 0x200;
    }
}

// 0x4C7658
static void setIntensityTables()
{
    for (int index = 0; index < 256; index++) {
        if (mappedColor[index] != 0) {
            setIntensityTableColor(index);
        } else {
            memset(intensityColorTable + index * 256, 0, 256);
        }
    }
}

// 0x4C769C
static void setMixTableColor(int a1)
{
    int i;
    int v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19;
    int v20, v21, v22, v23, v24, v25, v26, v27, v28, v29;

    v1 = a1 << 8;

    for (i = 0; i < 256; i++) {
        if (mappedColor[a1] && mappedColor[i]) {
            v2 = (Color2RGB(a1) & 0x7C00) >> 10;
            v3 = (Color2RGB(a1) & 0x3E0) >> 5;
            v4 = (Color2RGB(a1) & 0x1F);

            v5 = (Color2RGB(i) & 0x7C00) >> 10;
            v6 = (Color2RGB(i) & 0x3E0) >> 5;
            v7 = (Color2RGB(i) & 0x1F);

            v8 = v2 + v5;
            v9 = v3 + v6;
            v10 = v4 + v7;

            v11 = v8;

            if (v9 > v11) {
                v11 = v9;
            }

            if (v10 > v11) {
                v11 = v10;
            }

            if (v11 <= 0x1F) {
                int paletteIndex = (v8 << 10) | (v9 << 5) | v10;
                v12 = colorTable[paletteIndex];
            } else {
                v13 = v11 - 0x1F;

                v14 = v8 - v13;
                v15 = v9 - v13;
                v16 = v10 - v13;

                if (v14 < 0) {
                    v14 = 0;
                }

                if (v15 < 0) {
                    v15 = 0;
                }

                if (v16 < 0) {
                    v16 = 0;
                }

                v17 = (v14 << 10) | (v15 << 5) | v16;
                v18 = colorTable[v17];

                v19 = (int)((((double)v11 + (-31.0)) * 0.0078125 + 1.0) * 65536.0);
                v12 = calculateColor(v19, v18);
            }

            colorMixAddTable[v1 + i] = v12;

            v20 = (Color2RGB(a1) & 0x7C00) >> 10;
            v21 = (Color2RGB(a1) & 0x3E0) >> 5;
            v22 = (Color2RGB(a1) & 0x1F);

            v23 = (Color2RGB(i) & 0x7C00) >> 10;
            v24 = (Color2RGB(i) & 0x3E0) >> 5;
            v25 = (Color2RGB(i) & 0x1F);

            v26 = (v20 * v23) >> 5;
            v27 = (v21 * v24) >> 5;
            v28 = (v22 * v25) >> 5;

            v29 = (v26 << 10) | (v27 << 5) | v28;
            colorMixMulTable[v1 + i] = colorTable[v29];
        } else {
            if (mappedColor[i]) {
                colorMixAddTable[v1 + i] = i;
                colorMixMulTable[v1 + i] = i;
            } else {
                colorMixAddTable[v1 + i] = a1;
                colorMixMulTable[v1 + i] = a1;
            }
        }
    }
}

// 0x4C78E4
bool loadColorTable(const char* path)
{
    if (colorNameMangler != NULL) {
        path = colorNameMangler(path);
    }

    // NOTE: Uninline.
    int fd = colorOpen(path, 0x200);
    if (fd == -1) {
        errorStr = _aColor_cColorTa;
        return false;
    }

    for (int index = 0; index < 256; index++) {
        unsigned char r;
        unsigned char g;
        unsigned char b;

        // NOTE: Uninline.
        colorRead(fd, &r, sizeof(r));

        // NOTE: Uninline.
        colorRead(fd, &g, sizeof(g));

        // NOTE: Uninline.
        colorRead(fd, &b, sizeof(b));

        if (r <= 0x3F && g <= 0x3F && b <= 0x3F) {
            mappedColor[index] = 1;
        } else {
            r = 0;
            g = 0;
            b = 0;
            mappedColor[index] = 0;
        }

        cmap[index * 3] = r;
        cmap[index * 3 + 1] = g;
        cmap[index * 3 + 2] = b;
    }

    // NOTE: Uninline.
    colorRead(fd, colorTable, 0x8000);

    unsigned int type;
    // NOTE: Uninline.
    colorRead(fd, &type, sizeof(type));

    // NOTE: The value is "NEWC". Original code uses cmp opcode, not stricmp,
    // or comparing characters one-by-one.
    if (type == 0x4E455743) {
        // NOTE: Uninline.
        colorRead(fd, intensityColorTable, 0x10000);

        // NOTE: Uninline.
        colorRead(fd, colorMixAddTable, 0x10000);

        // NOTE: Uninline.
        colorRead(fd, colorMixMulTable, 0x10000);
    } else {
        setIntensityTables();

        for (int index = 0; index < 256; index++) {
            setMixTableColor(index);
        }
    }

    rebuildColorBlendTables();

    // NOTE: Uninline.
    colorClose(fd);

    return true;
}

// 0x4C7AB4
char* colorError()
{
    return errorStr;
}

// 0x4C7B44
static void buildBlendTable(unsigned char* ptr, unsigned char ch)
{
    int r, g, b;
    int i, j;
    int v12, v14, v16;
    unsigned char* beg;

    beg = ptr;

    r = (Color2RGB(ch) & 0x7C00) >> 10;
    g = (Color2RGB(ch) & 0x3E0) >> 5;
    b = (Color2RGB(ch) & 0x1F);

    for (i = 0; i < 256; i++) {
        ptr[i] = i;
    }

    ptr += 256;

    int b_1 = b;
    int v31 = 6;
    int g_1 = g;
    int r_1 = r;

    int b_2 = b_1;
    int g_2 = g_1;
    int r_2 = r_1;

    for (j = 0; j < 7; j++) {
        for (i = 0; i < 256; i++) {
            v12 = (Color2RGB(i) & 0x7C00) >> 10;
            v14 = (Color2RGB(i) & 0x3E0) >> 5;
            v16 = (Color2RGB(i) & 0x1F);
            int index = 0;
            index |= (r_2 + v12 * v31) / 7 << 10;
            index |= (g_2 + v14 * v31) / 7 << 5;
            index |= (b_2 + v16 * v31) / 7;
            ptr[i] = colorTable[index];
        }
        v31--;
        ptr += 256;
        r_2 += r_1;
        g_2 += g_1;
        b_2 += b_1;
    }

    int v18 = 0;
    for (j = 0; j < 6; j++) {
        int v20 = v18 / 7 + 0xFFFF;

        for (i = 0; i < 256; i++) {
            ptr[i] = calculateColor(v20, ch);
        }

        v18 += 0x10000;
        ptr += 256;
    }
}

// 0x4C7D90
static void rebuildColorBlendTables()
{
    int i;

    for (i = 0; i < 256; i++) {
        if (blendTable[i]) {
            buildBlendTable(blendTable[i], i);
        }
    }
}

// 0x4C7DC0
unsigned char* getColorBlendTable(int ch)
{
    unsigned char* ptr;

    if (blendTable[ch] == NULL) {
        ptr = (unsigned char*)mallocPtr(4100);
        *(int*)ptr = 1;
        blendTable[ch] = ptr + 4;
        buildBlendTable(blendTable[ch], ch);
    }

    ptr = blendTable[ch];
    *(int*)((unsigned char*)ptr - 4) = *(int*)((unsigned char*)ptr - 4) + 1;

    return ptr;
}

// 0x4C7E20
void freeColorBlendTable(int a1)
{
    unsigned char* v2 = blendTable[a1];
    if (v2 != NULL) {
        int* count = (int*)(v2 - sizeof(int));
        *count -= 1;
        if (*count == 0) {
            freePtr(count);
            blendTable[a1] = NULL;
        }
    }
}

// 0x4C7E58
void colorRegisterAlloc(MallocProc* mallocProc, ReallocProc* reallocProc, FreeProc* freeProc)
{
    mallocPtr = mallocProc;
    reallocPtr = reallocProc;
    freePtr = freeProc;
}

// 0x4C7E6C
void colorGamma(double value)
{
    currentGamma = value;

    for (int i = 0; i < 64; i++) {
        double value = pow(i, currentGamma);
        currentGammaTable[i] = (unsigned char)min(max(value, 0.0), 63.0);
    }

    setSystemPalette(systemCmap);
}

// NOTE: Unused.
//
// 0x4C8828
static bool colorPushColorPalette()
{
    if (tos >= COLOR_PALETTE_STACK_CAPACITY) {
        errorStr = _aColor_cColorpa;
        return false;
    }

    ColorPaletteStackEntry* entry = (ColorPaletteStackEntry*)malloc(sizeof(*entry));
    colorPaletteStack[tos] = entry;

    memcpy(entry->mappedColors, mappedColor, sizeof(mappedColor));
    memcpy(entry->cmap, cmap, sizeof(cmap));
    memcpy(entry->colorTable, colorTable, sizeof(colorTable));

    tos++;

    return true;
}

// NOTE: Unused.
//
// 0x4C88E0
static bool colorPopColorPalette()
{
    if (tos == 0) {
        errorStr = aColor_cColor_0;
        return false;
    }

    tos--;

    ColorPaletteStackEntry* entry = colorPaletteStack[tos];

    memcpy(mappedColor, entry->mappedColors, sizeof(mappedColor));
    memcpy(cmap, entry->cmap, sizeof(cmap));
    memcpy(colorTable, entry->colorTable, sizeof(colorTable));

    free(entry);
    colorPaletteStack[tos] = NULL;

    setIntensityTables();

    for (int index = 0; index < 256; index++) {
        setMixTableColor(index);
    }

    rebuildColorBlendTables();

    return true;
}

// 0x4C89CC
bool initColors()
{
    if (colorsInited) {
        return true;
    }

    colorsInited = true;

    colorGamma(1.0);

    if (!loadColorTable("color.pal")) {
        return false;
    }

    setSystemPalette(cmap);

    return true;
}

// 0x4C8A18
void colorsClose()
{
    for (int index = 0; index < 256; index++) {
        freeColorBlendTable(index);
    }

    for (int index = 0; index < tos; index++) {
        free(colorPaletteStack[index]);
    }

    tos = 0;
}

// 0x4C8A64
unsigned char* getColorPalette()
{
    return cmap;
}
