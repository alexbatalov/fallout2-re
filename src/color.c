#include "color.h"

#include "core.h"

#include <math.h>

// 0x50F930
char byte_50F930[] = "color.c: No errors\n";

// 0x50F95C
char byte_50F95C[] = "color.c: color table not found\n";

// 0x51DF10
char* off_51DF10 = byte_50F930;

// 0x51DF14
bool dword_51DF14 = false;

// 0x51DF18
double gBrightness = 1.0;

// 0x51DF20
ColorTransitionCallback* gColorPaletteTransitionCallback = NULL;

// 0x51DF24
MallocProc* gColorPaletteMallocProc = colorPaletteMallocDefaultImpl;

// 0x51DF28
ReallocProc* gColorPaletteReallocProc = colorPaletteReallocDefaultImpl;

// 0x51DF2C
FreeProc* gColorPaletteFreeProc = colorPaletteFreeDefaultImpl;

// NOTE: This value is never set, so it's impossible to understand it's
// meaning.
void (*off_51DF30)() = NULL;

// 0x51DF34
unsigned char stru_51DF34[768] = {
    0x3F, 0x3F, 0x3F
};

// 0x673090
unsigned char stru_673090[256 * 3];

// 0x673390
unsigned char byte_673390[64];

// 0x6733D0
unsigned char* dword_6733D0[256];

// 0x6737D0
unsigned char byte_6737D0[256];

// 0x6738D0
unsigned char byte_6738D0[65536];

// 0x6838D0
unsigned char byte_6838D0[65536];

// 0x6938D0
unsigned char byte_6938D0[65536];

// 0x6A38D0
unsigned char byte_6A38D0[32768];

// 0x6AB928
ColorPaletteFileReadProc* gColorPaletteFileReadProc;

// 0x6AB92C
ColorPaletteCloseProc* gColorPaletteFileCloseProc;

// 0x6AB930
ColorPaletteFileOpenProc* gColorPaletteFileOpenProc;

// NOTE: Inlined.
//
// 0x4C7200
int colorPaletteFileOpen(const char* filePath, int flags)
{
    if (gColorPaletteFileOpenProc != NULL) {
        return gColorPaletteFileOpenProc(filePath, flags);
    }

    return -1;
}

// NOTE: Inlined.
//
// 0x4C7218
int colorPaletteFileRead(int fd, void* buffer, size_t size)
{
    if (gColorPaletteFileReadProc != NULL) {
        return gColorPaletteFileReadProc(fd, buffer, size);
    }

    return -1;
}

// NOTE: Inlined.
//
// 0x4C7230
int colorPaletteFileClose(int fd)
{
    if (gColorPaletteFileCloseProc != NULL) {
        return gColorPaletteFileCloseProc(fd);
    }

    return -1;
}

// 0x4C7248
void colorPaletteSetFileIO(ColorPaletteFileOpenProc* openProc, ColorPaletteFileReadProc* readProc, ColorPaletteCloseProc* closeProc)
{
    gColorPaletteFileOpenProc = openProc;
    gColorPaletteFileReadProc = readProc;
    gColorPaletteFileCloseProc = closeProc;
}

// 0x4C725C
void* colorPaletteMallocDefaultImpl(size_t size)
{
    return malloc(size);
}

// 0x4C7264
void* colorPaletteReallocDefaultImpl(void* ptr, size_t size)
{
    return realloc(ptr, size);
}

// 0x4C726C
void colorPaletteFreeDefaultImpl(void* ptr)
{
    free(ptr);
}

// 0x4C72B4
int calculateColor(int a1, int a2)
{
    int v1 = (a1 >> 9) + ((a2 & 0xFF) << 8);
    return byte_6838D0[v1];
}

// 0x4C72E0
int sub_4C72E0(int a1)
{
    int v1, v2, v3;

    v1 = stru_51DF34[3 * a1] >> 1;
    v2 = stru_51DF34[3 * a1 + 1] >> 1;
    v3 = stru_51DF34[3 * a1 + 2] >> 1;

    return (((v1 << 5) | v2) << 5) | v3;
}

// Performs animated palette transition.
//
// 0x4C7320
void colorPaletteFadeBetween(unsigned char* oldPalette, unsigned char* newPalette, int steps)
{
    for (int step = 0; step < steps; step++) {
        unsigned char palette[768];

        for (int index = 0; index < 768; index++) {
            palette[index] = oldPalette[index] - (oldPalette[index] - newPalette[index]) * step / steps;
        }

        if (gColorPaletteTransitionCallback != NULL) {
            if (step % 128 == 0) {
                gColorPaletteTransitionCallback();
            }
        }

        setSystemPalette(palette);
    }

    setSystemPalette(newPalette);
}

// 0x4C73D4
void colorPaletteSetTransitionCallback(ColorTransitionCallback* callback)
{
    gColorPaletteTransitionCallback = callback;
}

// 0x4C73E4
void setSystemPalette(unsigned char* palette)
{
    unsigned char newPalette[768];

    for (int index = 0; index < 768; index++) {
        newPalette[index] = byte_673390[palette[index]];
        stru_673090[index] = palette[index];
    }

    directDrawSetPalette(newPalette);
}

// 0x4C7420
unsigned char* getSystemPalette()
{
    return stru_673090;
}

// 0x4C7428
void setSystemPaletteEntries(unsigned char* palette, int start, int end)
{
    unsigned char newPalette[768];

    int length = end - start + 1;
    for (int index = 0; index < length; index++) {
        newPalette[index * 3] = byte_673390[palette[index * 3]];
        newPalette[index * 3 + 1] = byte_673390[palette[index * 3 + 1]];
        newPalette[index * 3 + 2] = byte_673390[palette[index * 3 + 2]];

        stru_673090[start * 3 + index * 3] = palette[index * 3];
        stru_673090[start * 3 + index * 3 + 1] = palette[index * 3 + 1];
        stru_673090[start * 3 + index * 3 + 2] = palette[index * 3 + 2];
    }

    directDrawSetPaletteInRange(newPalette, start, end - start + 1);
}

// 0x4C7550
void setIntensityTableColor(int a1)
{
    int v1, v2, v3, v4, v5, v6, v7, v8, v9, v10;

    v5 = 0;
    v10 = a1 << 8;

    for (int index = 0; index < 128; index++) {
        v1 = (sub_4C72E0(a1) & 0x7C00) >> 10;
        v2 = (sub_4C72E0(a1) & 0x3E0) >> 5;
        v3 = (sub_4C72E0(a1) & 0x1F);

        v4 = (((v1 * v5) >> 16) << 10) | (((v2 * v5) >> 16) << 5) | ((v3 * v5) >> 16);
        byte_6838D0[index + v10] = byte_6A38D0[v4];

        v6 = v1 + (((0x1F - v1) * v5) >> 16);
        v7 = v2 + (((0x1F - v2) * v5) >> 16);
        v8 = v3 + (((0x1F - v3) * v5) >> 16);

        v9 = (v6 << 10) | (v7 << 5) | v8;
        byte_6838D0[0x7F + index + 1 + v10] = byte_6A38D0[v9];

        v5 += 0x200;
    }
}

// 0x4C7658
void setIntensityTables()
{
    for (int index = 0; index < 256; index++) {
        if (byte_6737D0[index] != 0) {
            setIntensityTableColor(index);
        } else {
            memset(byte_6838D0 + index * 256, 0, 256);
        }
    }
}

// 0x4C769C
void setMixTableColor(int a1)
{
    int i;
    int v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19;
    int v20, v21, v22, v23, v24, v25, v26, v27, v28, v29;

    v1 = a1 << 8;

    for (i = 0; i < 256; i++) {
        if (byte_6737D0[a1] && byte_6737D0[i]) {
            v2 = (sub_4C72E0(a1) & 0x7C00) >> 10;
            v3 = (sub_4C72E0(a1) & 0x3E0) >> 5;
            v4 = (sub_4C72E0(a1) & 0x1F);

            v5 = (sub_4C72E0(i) & 0x7C00) >> 10;
            v6 = (sub_4C72E0(i) & 0x3E0) >> 5;
            v7 = (sub_4C72E0(i) & 0x1F);

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
                v12 = byte_6A38D0[paletteIndex];
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
                v18 = byte_6A38D0[v17];

                v19 = (int)((((double)v11 + (-31.0)) * 0.0078125 + 1.0) * 65536.0);
                v12 = calculateColor(v19, v18);
            }

            byte_6738D0[v1 + i] = v12;

            v20 = (sub_4C72E0(a1) & 0x7C00) >> 10;
            v21 = (sub_4C72E0(a1) & 0x3E0) >> 5;
            v22 = (sub_4C72E0(a1) & 0x1F);

            v23 = (sub_4C72E0(i) & 0x7C00) >> 10;
            v24 = (sub_4C72E0(i) & 0x3E0) >> 5;
            v25 = (sub_4C72E0(i) & 0x1F);

            v26 = (v20 * v23) >> 5;
            v27 = (v21 * v24) >> 5;
            v28 = (v22 * v25) >> 5;

            v29 = (v26 << 10) | (v27 << 5) | v28;
            byte_6938D0[v1 + i] = byte_6A38D0[v29];
        } else {
            if (byte_6737D0[i]) {
                byte_6738D0[v1 + i] = i;
                byte_6938D0[v1 + i] = i;
            } else {
                byte_6738D0[v1 + i] = a1;
                byte_6938D0[v1 + i] = a1;
            }
        }
    }
}

// 0x4C78E4
bool colorPaletteLoad(const char* path)
{
    if (off_51DF30) {
        off_51DF30();
    }

    // NOTE: Uninline.
    int fd = colorPaletteFileOpen(path, 0x200);
    if (fd == -1) {
        off_51DF10 = byte_50F95C;
        return false;
    }

    for (int index = 0; index < 256; index++) {
        unsigned char r;
        unsigned char g;
        unsigned char b;

        // NOTE: Uninline.
        colorPaletteFileRead(fd, &r, sizeof(r));

        // NOTE: Uninline.
        colorPaletteFileRead(fd, &g, sizeof(g));

        // NOTE: Uninline.
        colorPaletteFileRead(fd, &b, sizeof(b));

        if (r <= 0x3F && g <= 0x3F && b <= 0x3F) {
            byte_6737D0[index] = 1;
        } else {
            r = 0;
            g = 0;
            b = 0;
            byte_6737D0[index] = 0;
        }

        stru_51DF34[index * 3] = r;
        stru_51DF34[index * 3 + 1] = g;
        stru_51DF34[index * 3 + 2] = b;
    }

    // NOTE: Uninline.
    colorPaletteFileRead(fd, byte_6A38D0, 0x8000);

    unsigned int type;
    // NOTE: Uninline.
    colorPaletteFileRead(fd, &type, sizeof(type));

    // NOTE: The value is "NEWC". Original code uses cmp opcode, not stricmp,
    // or comparing characters one-by-one.
    if (type == 0x4E455743) {
        // NOTE: Uninline.
        colorPaletteFileRead(fd, byte_6838D0, 0x10000);

        // NOTE: Uninline.
        colorPaletteFileRead(fd, byte_6738D0, 0x10000);

        // NOTE: Uninline.
        colorPaletteFileRead(fd, byte_6938D0, 0x10000);
    } else {
        setIntensityTables();

        for (int index = 0; index < 256; index++) {
            setMixTableColor(index);
        }
    }

    rebuildColorBlendTables();

    // NOTE: Uninline.
    colorPaletteFileClose(fd);

    return true;
}

// 0x4C7AB4
char* colorError()
{
    return off_51DF10;
}

// 0x4C7B44
void buildBlendTable(unsigned char* ptr, unsigned char ch)
{
    int r, g, b;
    int i, j;
    int v12, v14, v16;
    unsigned char* beg;

    beg = ptr;

    r = (sub_4C72E0(ch) & 0x7C00) >> 10;
    g = (sub_4C72E0(ch) & 0x3E0) >> 5;
    b = (sub_4C72E0(ch) & 0x1F);

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
            v12 = (sub_4C72E0(i) & 0x7C00) >> 10;
            v14 = (sub_4C72E0(i) & 0x3E0) >> 5;
            v16 = (sub_4C72E0(i) & 0x1F);
            int index = 0;
            index |= (r_2 + v12 * v31) / 7 << 10;
            index |= (g_2 + v14 * v31) / 7 << 5;
            index |= (b_2 + v16 * v31) / 7;
            ptr[i] = byte_6A38D0[index];
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
void rebuildColorBlendTables()
{
    int i;

    for (i = 0; i < 256; i++) {
        if (dword_6733D0[i]) {
            buildBlendTable(dword_6733D0[i], i);
        }
    }
}

// 0x4C7DC0
unsigned char* getColorBlendTable(int ch)
{
    unsigned char* ptr;

    if (dword_6733D0[ch] == NULL) {
        ptr = (unsigned char*)gColorPaletteMallocProc(4100);
        *(int*)ptr = 1;
        dword_6733D0[ch] = ptr + 4;
        buildBlendTable(dword_6733D0[ch], ch);
    }

    ptr = dword_6733D0[ch];
    *(int*)((unsigned char*)ptr - 4) = *(int*)((unsigned char*)ptr - 4) + 1;

    return ptr;
}

// 0x4C7E20
void freeColorBlendTable(int a1)
{
    unsigned char* v2 = dword_6733D0[a1];
    if (v2 != NULL) {
        int* count = (int*)(v2 - sizeof(int));
        *count -= 1;
        if (*count == 0) {
            gColorPaletteFreeProc(count);
            dword_6733D0[a1] = NULL;
        }
    }
}

// 0x4C7E58
void colorPaletteSetMemoryProcs(MallocProc* mallocProc, ReallocProc* reallocProc, FreeProc* freeProc)
{
    gColorPaletteMallocProc = mallocProc;
    gColorPaletteReallocProc = reallocProc;
    gColorPaletteFreeProc = freeProc;
}

// 0x4C7E6C
void colorSetBrightness(double value)
{
    gBrightness = value;

    for (int i = 0; i < 64; i++) {
        double value = pow(i, gBrightness);
        byte_673390[i] = (unsigned char)min(max(value, 0.0), 63.0);
    }

    setSystemPalette(stru_673090);
}

// 0x4C89CC
bool initColors()
{
    if (dword_51DF14) {
        return true;
    }

    dword_51DF14 = true;

    colorSetBrightness(1.0);

    if (!colorPaletteLoad("color.pal")) {
        return false;
    }

    setSystemPalette(stru_51DF34);

    return true;
}

// 0x4C8A18
void colorsClose()
{
    for (int index = 0; index < 256; index++) {
        freeColorBlendTable(index);
    }

    // TODO: Incomplete.
}
