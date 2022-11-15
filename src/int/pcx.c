#include "int/pcx.h"

#include "db.h"
#include "int/memdbg.h"

typedef struct PcxHeader {
    unsigned char identifier;
    unsigned char version;
    unsigned char encoding;
    unsigned char bitsPerPixel;
    short minX;
    short minY;
    short maxX;
    short maxY;
    short horizontalResolution;
    short verticalResolution;
    unsigned char palette[48];
    unsigned char reserved1;
    unsigned char planeCount;
    short bytesPerLine;
    short paletteType;
    short horizontalScreenSize;
    short verticalScreenSize;
    unsigned char reserved2[54];
} PcxHeader;

static short getWord(File* stream);
static void readPcxHeader(PcxHeader* pcxHeader, File* stream);
static int pcxDecodeScanline(unsigned char* data, int size, File* stream);
static int readPcxVgaPalette(PcxHeader* pcxHeader, unsigned char* palette, File* stream);

// 0x519DC8
static unsigned char runcount = 0;

// 0x519DC9
static unsigned char runvalue = 0;

// NOTE: Inlined.
//
// 0x4961B0
static short getWord(File* stream)
{
    short value;
    fileRead(&value, sizeof(value), 1, stream);
    return value;
}

// 0x4961D4
static void readPcxHeader(PcxHeader* pcxHeader, File* stream)
{
    pcxHeader->identifier = fileReadChar(stream);
    pcxHeader->version = fileReadChar(stream);
    pcxHeader->encoding = fileReadChar(stream);
    pcxHeader->bitsPerPixel = fileReadChar(stream);
    pcxHeader->minX = getWord(stream);
    pcxHeader->minY = getWord(stream);
    pcxHeader->maxX = getWord(stream);
    pcxHeader->maxY = getWord(stream);
    pcxHeader->horizontalResolution = getWord(stream);
    pcxHeader->verticalResolution = getWord(stream);

    for (int index = 0; index < 48; index++) {
        pcxHeader->palette[index] = fileReadChar(stream);
    }

    pcxHeader->reserved1 = fileReadChar(stream);
    pcxHeader->planeCount = fileReadChar(stream);
    pcxHeader->bytesPerLine = getWord(stream);
    pcxHeader->paletteType = getWord(stream);
    pcxHeader->horizontalScreenSize = getWord(stream);
    pcxHeader->verticalScreenSize = getWord(stream);

    for (int index = 0; index < 54; index++) {
        pcxHeader->reserved2[index] = fileReadChar(stream);
    }
}

// 0x49636C
static int pcxDecodeScanline(unsigned char* data, int size, File* stream)
{
    unsigned char runLength = runcount;
    unsigned char value = runvalue;

    int uncompressedSize = 0;
    int index = 0;
    do {
        uncompressedSize += runLength;
        while (runLength > 0 && index < size) {
            data[index] = value;
            runLength--;
            index++;
        }

        runcount = runLength;
        runvalue = value;

        if (runLength != 0) {
            uncompressedSize -= runLength;
            break;
        }

        value = fileReadChar(stream);
        if ((value & 0xC0) == 0xC0) {
            runcount = value & 0x3F;
            value = fileReadChar(stream);
            runLength = runcount;
        } else {
            runLength = 1;
        }
    } while (index < size);

    runcount = runLength;
    runvalue = value;

    return uncompressedSize;
}

// 0x49641C
static int readPcxVgaPalette(PcxHeader* pcxHeader, unsigned char* palette, File* stream)
{
    if (pcxHeader->version != 5) {
        return 0;
    }

    long pos = fileTell(stream);
    long size = fileGetSize(stream);
    fileSeek(stream, size - 769, SEEK_SET);
    if (fileReadChar(stream) != 12) {
        fileSeek(stream, pos, SEEK_SET);
        return 0;
    }

    for (int index = 0; index < 768; index++) {
        palette[index] = fileReadChar(stream);
    }

    fileSeek(stream, pos, SEEK_SET);

    return 1;
}

// 0x496494
unsigned char* loadPCX(const char* path, int* widthPtr, int* heightPtr, unsigned char* palette)
{
    File* stream = fileOpen(path, "rb");
    if (stream == NULL) {
        return NULL;
    }

    PcxHeader pcxHeader;
    readPcxHeader(&pcxHeader, stream);

    int width = pcxHeader.maxX - pcxHeader.minX + 1;
    int height = pcxHeader.maxY - pcxHeader.minY + 1;

    *widthPtr = width;
    *heightPtr = height;

    int bytesPerLine = pcxHeader.planeCount * pcxHeader.bytesPerLine;
    unsigned char* data = mymalloc(bytesPerLine * height, __FILE__, __LINE__); // "..\\int\\PCX.C", 195
    if (data == NULL) {
        // NOTE: This code is unreachable, internal_malloc_safe never fails.
        fileClose(stream);
        return NULL;
    }

    runcount = 0;
    runvalue = 0;

    unsigned char* ptr = data;
    for (int y = 0; y < height; y++) {
        pcxDecodeScanline(ptr, bytesPerLine, stream);
        ptr += width;
    }

    readPcxVgaPalette(&pcxHeader, palette, stream);

    fileClose(stream);

    return data;
}