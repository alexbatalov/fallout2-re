#include "game/artload.h"

#include "plib/gnw/memory.h"

static int art_readSubFrameData(unsigned char* data, File* stream, int count);
static int art_readFrameData(Art* art, File* stream);

// 0x419D60
static int art_readSubFrameData(unsigned char* data, File* stream, int count)
{
    unsigned char* ptr = data;
    for (int index = 0; index < count; index++) {
        ArtFrame* frame = (ArtFrame*)ptr;

        if (fileReadInt16(stream, &(frame->width)) == -1) return -1;
        if (fileReadInt16(stream, &(frame->height)) == -1) return -1;
        if (fileReadInt32(stream, &(frame->size)) == -1) return -1;
        if (fileReadInt16(stream, &(frame->x)) == -1) return -1;
        if (fileReadInt16(stream, &(frame->y)) == -1) return -1;
        if (fileRead(ptr + sizeof(ArtFrame), frame->size, 1, stream) != 1) return -1;

        ptr += sizeof(ArtFrame) + frame->size;
    }

    return 0;
}

// 0x419E1C
static int art_readFrameData(Art* art, File* stream)
{
    if (fileReadInt32(stream, &(art->field_0)) == -1) return -1;
    if (fileReadInt16(stream, &(art->framesPerSecond)) == -1) return -1;
    if (fileReadInt16(stream, &(art->actionFrame)) == -1) return -1;
    if (fileReadInt16(stream, &(art->frameCount)) == -1) return -1;
    if (fileReadInt16List(stream, art->xOffsets, ROTATION_COUNT) == -1) return -1;
    if (fileReadInt16List(stream, art->yOffsets, ROTATION_COUNT) == -1) return -1;
    if (fileReadInt32List(stream, art->dataOffsets, ROTATION_COUNT) == -1) return -1;
    if (fileReadInt32(stream, &(art->field_3A)) == -1) return -1;

    return 0;
}

// NOTE: Unused.
//
// 0x419EC0
int load_frame(const char* path, Art** artPtr)
{
    int size;
    File* stream;
    int index;

    if (dbGetFileSize(path, &size) == -1) {
        return -2;
    }

    *artPtr = (Art*)mem_malloc(size);
    if (*artPtr == NULL) {
        return -1;
    }

    stream = fileOpen(path, "rb");
    if (stream == NULL) {
        return -2;
    }

    if (art_readFrameData(*artPtr, stream) != 0) {
        fileClose(stream);
        mem_free(*artPtr);
        return -3;
    }

    for (index = 0; index < ROTATION_COUNT; index++) {
        if (index == 0 || (*artPtr)->dataOffsets[index - 1] != (*artPtr)->dataOffsets[index]) {
            if (art_readSubFrameData((unsigned char*)(*artPtr) + sizeof(Art) + (*artPtr)->dataOffsets[index], stream, (*artPtr)->frameCount) != 0) {
                break;
            }
        }
    }

    if (index < ROTATION_COUNT) {
        fileClose(stream);
        mem_free(*artPtr);
        return -5;
    }

    fileClose(stream);

    return 0;
}

// 0x419FC0
int load_frame_into(const char* path, unsigned char* data)
{
    File* stream = fileOpen(path, "rb");
    if (stream == NULL) {
        return -2;
    }

    Art* art = (Art*)data;
    if (art_readFrameData(art, stream) != 0) {
        fileClose(stream);
        return -3;
    }

    for (int index = 0; index < ROTATION_COUNT; index++) {
        if (index == 0 || art->dataOffsets[index - 1] != art->dataOffsets[index]) {
            if (art_readSubFrameData(data + sizeof(Art) + art->dataOffsets[index], stream, art->frameCount) != 0) {
                fileClose(stream);
                return -5;
            }
        }
    }

    fileClose(stream);
    return 0;
}

// NOTE: Unused.
//
// 0x41A070
int art_writeSubFrameData(unsigned char* data, File* stream, int count)
{
    unsigned char* ptr = data;
    for (int index = 0; index < count; index++) {
        ArtFrame* frame = (ArtFrame*)ptr;

        if (fileWriteInt16(stream, frame->width) == -1) return -1;
        if (fileWriteInt16(stream, frame->height) == -1) return -1;
        if (fileWriteInt32(stream, frame->size) == -1) return -1;
        if (fileWriteInt16(stream, frame->x) == -1) return -1;
        if (fileWriteInt16(stream, frame->y) == -1) return -1;
        if (fileWrite(ptr + sizeof(ArtFrame), frame->size, 1, stream) != 1) return -1;

        ptr += sizeof(ArtFrame) + frame->size;
    }

    return 0;
}

// NOTE: Unused.
//
// 0x41A138
int art_writeFrameData(Art* art, File* stream)
{
    if (fileWriteInt32(stream, art->field_0) == -1) return -1;
    if (fileWriteInt16(stream, art->framesPerSecond) == -1) return -1;
    if (fileWriteInt16(stream, art->actionFrame) == -1) return -1;
    if (fileWriteInt16(stream, art->frameCount) == -1) return -1;
    if (fileWriteInt16List(stream, art->xOffsets, ROTATION_COUNT) == -1) return -1;
    if (fileWriteInt16List(stream, art->yOffsets, ROTATION_COUNT) == -1) return -1;
    if (fileWriteInt32List(stream, art->dataOffsets, ROTATION_COUNT) == -1) return -1;
    if (fileWriteInt32(stream, art->field_3A) == -1) return -1;

    return 0;
}

// NOTE: Unused.
//
// 0x41A1E8
int save_frame(const char* path, unsigned char* data)
{
    if (data == NULL) {
        return -1;
    }

    File* stream = fileOpen(path, "wb");
    if (stream == NULL) {
        return -1;
    }

    Art* art = (Art*)data;
    if (art_writeFrameData(art, stream) == -1) {
        fileClose(stream);
        return -1;
    }

    for (int index = 0; index < ROTATION_COUNT; index++) {
        if (index == 0 || art->dataOffsets[index - 1] != art->dataOffsets[index]) {
            if (art_writeSubFrameData(data + sizeof(Art) + art->dataOffsets[index], stream, art->frameCount) != 0) {
                fileClose(stream);
                return -1;
            }
        }
    }

    fileClose(stream);
    return 0;
}
