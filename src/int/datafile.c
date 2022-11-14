#include "int/datafile.h"

#include <string.h>

#include "color.h"
#include "db.h"
#include "int/memdbg.h"
#include "pcx.h"

static char* defaultMangleName(char* path);

// 0x5184AC
static DatafileLoader* loadFunc = NULL;

// 0x5184B0
static DatafileNameMangler* mangleName = defaultMangleName;

// 0x56D7E0
static unsigned char pal[768];

// 0x42EE70
static char* defaultMangleName(char* path)
{
    return path;
}

// NOTE: Unused.
//
// 0x42EE74
void datafileSetFilenameFunc(DatafileNameMangler* mangler)
{
    mangleName = mangler;
}

// NOTE: Unused.
//
// 0x42EE7C
void setBitmapLoadFunc(DatafileLoader* loader)
{
    loadFunc = loader;
}

// 0x42EE84
void datafileConvertData(unsigned char* data, unsigned char* palette, int width, int height)
{
    unsigned char indexedPalette[256];

    indexedPalette[0] = 0;
    for (int index = 1; index < 256; index++) {
        // TODO: Check.
        int r = palette[index * 3 + 2] >> 3;
        int g = palette[index * 3 + 1] >> 3;
        int b = palette[index * 3] >> 3;
        int colorTableIndex = (r << 10) | (g << 5) | b;
        indexedPalette[index] = colorTable[colorTableIndex];
    }

    int size = width * height;
    for (int index = 0; index < size; index++) {
        data[index] = indexedPalette[data[index]];
    }
}

// NOTE: Unused.
//
// 0x42EEF8
void datafileConvertDataVGA(unsigned char* data, unsigned char* palette, int width, int height)
{
    unsigned char indexedPalette[256];

    indexedPalette[0] = 0;
    for (int index = 1; index < 256; index++) {
        // TODO: Check.
        int r = palette[index * 3 + 2] >> 1;
        int g = palette[index * 3 + 1] >> 1;
        int b = palette[index * 3] >> 1;
        int colorTableIndex = (r << 10) | (g << 5) | b;
        indexedPalette[index] = colorTable[colorTableIndex];
    }

    int size = width * height;
    for (int index = 0; index < size; index++) {
        data[index] = indexedPalette[data[index]];
    }
}

// 0x42EF60
unsigned char* loadRawDataFile(char* path, int* widthPtr, int* heightPtr)
{
    char* mangledPath = mangleName(path);
    char* dot = strrchr(mangledPath, '.');
    if (dot != NULL) {
        if (stricmp(dot + 1, "pcx") == 0) {
            return pcxRead(mangledPath, widthPtr, heightPtr, pal);
        }
    }

    if (loadFunc != NULL) {
        return loadFunc(mangledPath, pal, widthPtr, heightPtr);
    }

    return NULL;
}

// 0x42EFCC
unsigned char* loadDataFile(char* path, int* widthPtr, int* heightPtr)
{
    unsigned char* v1 = loadRawDataFile(path, widthPtr, heightPtr);
    if (v1 != NULL) {
        datafileConvertData(v1, pal, *widthPtr, *heightPtr);
    }
    return v1;
}

// NOTE: Unused
//
// 0x42EFF4
unsigned char* load256Palette(char* path)
{
    int width;
    int height;
    unsigned char* v3 = loadRawDataFile(path, &width, &height);
    if (v3 != NULL) {
        internal_free_safe(v3, __FILE__, __LINE__); // "..\\int\\DATAFILE.C", 148
        return pal;
    }

    return NULL;
}

// NOTE: Unused.
//
// 0x42F024
void trimBuffer(unsigned char* data, int* widthPtr, int* heightPtr)
{
    int width = *widthPtr;
    int height = *heightPtr;
    unsigned char* temp = (unsigned char*)internal_malloc_safe(width * height, __FILE__, __LINE__); // "..\\int\\DATAFILE.C", 157

    // NOTE: Original code does not initialize `x`.
    int y = 0;
    int x = 0;
    unsigned char* src1 = data;

    for (y = 0; y < height; y++) {
        if (*src1 == 0) {
            break;
        }

        unsigned char* src2 = src1;
        for (x = 0; x < width; x++) {
            if (*src2 == 0) {
                break;
            }

            *temp++ = *src2++;
        }

        src1 += width;
    }

    memcpy(data, temp, x * y);
    internal_free_safe(temp, __FILE__, __LINE__); // // "..\\int\\DATAFILE.C", 171
}

// 0x42F0E4
unsigned char* datafileGetPalette()
{
    return pal;
}

// NOTE: Unused.
//
// 0x42F0EC
unsigned char* datafileLoadBlock(char* path, int* sizePtr)
{
    const char* mangledPath = mangleName(path);
    File* stream = fileOpen(mangledPath, "rb");
    if (stream == NULL) {
        return NULL;
    }

    int size = fileGetSize(stream);
    void* data = internal_malloc_safe(size, __FILE__, __LINE__); // "..\\int\\DATAFILE.C", 185
    if (data == NULL) {
        // NOTE: This code is unreachable, internal_malloc_safe never fails.
        // Otherwise it leaks stream.
        *sizePtr = 0;
        return NULL;
    }

    fileRead(data, 1, size, stream);
    fileClose(stream);
    *sizePtr = size;
    return data;
}
