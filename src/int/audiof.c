#include "int/audiof.h"

#include <assert.h>
#include <io.h>
#include <stdio.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "plib/gnw/debug.h"
#include "int/memdbg.h"
#include "int/sound.h"

static_assert(sizeof(AudioFile) == 28, "wrong size");

static bool defaultCompressionFunc(char* filePath);
static int decodeRead(int fileHandle, void* buffer, unsigned int size);

// 0x5108C0
static AudioFileIsCompressedProc* queryCompressedFunc = defaultCompressionFunc;

// 0x56CB10
static AudioFile* audiof;

// 0x56CB14
static int numAudiof;

// 0x41A850
static bool defaultCompressionFunc(char* filePath)
{
    char* pch = strrchr(filePath, '.');
    if (pch != NULL) {
        strcpy(pch + 1, "war");
    }

    return false;
}

// 0x41A870
static int decodeRead(int fileHandle, void* buffer, unsigned int size)
{
    return fread(buffer, 1, size, (FILE*)fileHandle);
}

// 0x41A88C
int audiofOpen(const char* fname, int flags, ...)
{
    char path[MAX_PATH];
    strcpy(path, fname);

    int compression;
    if (queryCompressedFunc(path)) {
        compression = 2;
    } else {
        compression = 0;
    }

    char mode[4];
    memset(mode, '\0', 4);

    // NOTE: Original implementation is slightly different, it uses separate
    // variable to track index where to set 't' and 'b'.
    char* pm = mode;
    if (flags & 0x01) {
        *pm++ = 'w';
    } else if (flags & 0x02) {
        *pm++ = 'w';
        *pm++ = '+';
    } else {
        *pm++ = 'r';
    }

    if (flags & 0x0100) {
        *pm++ = 't';
    } else if (flags & 0x0200) {
        *pm++ = 'b';
    }

    FILE* stream = fopen(path, mode);
    if (stream == NULL) {
        return -1;
    }

    int index;
    for (index = 0; index < numAudiof; index++) {
        if ((audiof[index].flags & AUDIO_FILE_IN_USE) == 0) {
            break;
        }
    }

    if (index == numAudiof) {
        if (audiof != NULL) {
            audiof = (AudioFile*)myrealloc(audiof, sizeof(*audiof) * (numAudiof + 1), __FILE__, __LINE__); // "..\int\audiof.c", 207
        } else {
            audiof = (AudioFile*)mymalloc(sizeof(*audiof), __FILE__, __LINE__); // "..\int\audiof.c", 209
        }
        numAudiof++;
    }

    AudioFile* audioFile = &(audiof[index]);
    audioFile->flags = AUDIO_FILE_IN_USE;
    audioFile->fileHandle = (int)stream;

    if (compression == 2) {
        audioFile->flags |= AUDIO_FILE_COMPRESSED;
        audioFile->soundDecoder = soundDecoderInit(decodeRead, audioFile->fileHandle, &(audioFile->field_14), &(audioFile->field_10), &(audioFile->fileSize));
        audioFile->fileSize *= 2;
    } else {
        audioFile->fileSize = filelength(fileno(stream));
    }

    audioFile->position = 0;

    return index + 1;
}

// 0x41AAA0
int audiofCloseFile(int fileHandle)
{
    AudioFile* audioFile = &(audiof[fileHandle - 1]);
    fclose((FILE*)audioFile->fileHandle);

    if ((audioFile->flags & AUDIO_FILE_COMPRESSED) != 0) {
        soundDecoderFree(audioFile->soundDecoder);
    }

    // Reset audio file (which also resets it's use flag).
    memset(audioFile, 0, sizeof(*audioFile));

    return 0;
}

// 0x41AB08
int audiofRead(int fileHandle, void* buffer, unsigned int size)
{

    AudioFile* ptr = &(audiof[fileHandle - 1]);

    int bytesRead;
    if ((ptr->flags & AUDIO_FILE_COMPRESSED) != 0) {
        bytesRead = soundDecoderDecode(ptr->soundDecoder, buffer, size);
    } else {
        bytesRead = fread(buffer, 1, size, (FILE*)ptr->fileHandle);
    }

    ptr->position += bytesRead;

    return bytesRead;
}

// 0x41AB74
long audiofSeek(int fileHandle, long offset, int origin)
{
    void* buf;
    int remaining;
    int a4;

    AudioFile* audioFile = &(audiof[fileHandle - 1]);

    switch (origin) {
    case SEEK_SET:
        a4 = offset;
        break;
    case SEEK_CUR:
        a4 = audioFile->fileSize + offset;
        break;
    case SEEK_END:
        a4 = audioFile->position + offset;
        break;
    default:
        assert(false && "Should be unreachable");
    }

    if ((audioFile->flags & AUDIO_FILE_COMPRESSED) != 0) {
        if (a4 <= audioFile->position) {
            soundDecoderFree(audioFile->soundDecoder);

            fseek((FILE*)audioFile->fileHandle, 0, 0);

            audioFile->soundDecoder = soundDecoderInit(decodeRead, audioFile->fileHandle, &(audioFile->field_14), &(audioFile->field_10), &(audioFile->fileSize));
            audioFile->fileSize *= 2;
            audioFile->position = 0;

            if (a4) {
                buf = mymalloc(4096, __FILE__, __LINE__); // "..\int\audiof.c", 364
                while (a4 > 4096) {
                    audiofRead(fileHandle, buf, 4096);
                    a4 -= 4096;
                }
                if (a4 != 0) {
                    audiofRead(fileHandle, buf, a4);
                }
                myfree(buf, __FILE__, __LINE__); // "..\int\audiof.c", 370
            }
        } else {
            buf = mymalloc(0x400, __FILE__, __LINE__); // "..\int\audiof.c", 316
            remaining = audioFile->position - a4;
            while (remaining > 1024) {
                audiofRead(fileHandle, buf, 1024);
                remaining -= 1024;
            }
            if (remaining != 0) {
                audiofRead(fileHandle, buf, remaining);
            }
            // TODO: Obiously leaks memory.
        }
        return audioFile->position;
    }

    return fseek((FILE*)audioFile->fileHandle, offset, origin);
}

// 0x41AD20
long audiofFileSize(int fileHandle)
{
    AudioFile* audioFile = &(audiof[fileHandle - 1]);
    return audioFile->fileSize;
}

// 0x41AD3C
long audiofTell(int fileHandle)
{
    AudioFile* audioFile = &(audiof[fileHandle - 1]);
    return audioFile->position;
}

// 0x41AD58
int audiofWrite(int fileHandle, const void* buffer, unsigned int size)
{
    debug_printf("AudiofWrite shouldn't be ever called\n");
    return 0;
}

// 0x41AD68
int initAudiof(AudioFileIsCompressedProc* isCompressedProc)
{
    queryCompressedFunc = isCompressedProc;
    audiof = NULL;
    numAudiof = 0;

    return soundSetDefaultFileIO(audiofOpen, audiofCloseFile, audiofRead, audiofWrite, audiofSeek, audiofTell, audiofFileSize);
}

// 0x41ADAC
void audiofClose()
{
    if (audiof != NULL) {
        myfree(audiof, __FILE__, __LINE__); // "..\int\audiof.c", 405
    }

    numAudiof = 0;
    audiof = NULL;
}
