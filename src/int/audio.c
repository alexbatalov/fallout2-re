#include "int/audio.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "plib/db/db.h"
#include "plib/gnw/debug.h"
#include "int/memdbg.h"
#include "int/sound.h"

static bool defaultCompressionFunc(char* filePath);
static int decodeRead(int fileHandle, void* buf, unsigned int size);

// 0x5108BC
static AudioFileIsCompressedProc* queryCompressedFunc = defaultCompressionFunc;

// 0x56CB00
static int numAudio;

// 0x56CB04
static AudioFile* audio;

// 0x41A2B0
bool defaultCompressionFunc(char* filePath)
{
    char* pch = strrchr(filePath, '.');
    if (pch != NULL) {
        strcpy(pch + 1, "war");
    }

    return false;
}

// 0x41A2D0
int decodeRead(int fileHandle, void* buffer, unsigned int size)
{
    return db_fread(buffer, 1, size, (File*)fileHandle);
}

// 0x41A2EC
int audioOpen(const char* fname, int flags, ...)
{
    char path[80];
    sprintf(path, fname);

    int compression;
    if (queryCompressedFunc(path)) {
        compression = 2;
    } else {
        compression = 0;
    }

    char mode[4];
    memset(mode, 0, 4);

    // NOTE: Original implementation is slightly different, it uses separate
    // variable to track index where to set 't' and 'b'.
    char* pm = mode;
    if (flags & 1) {
        *pm++ = 'w';
    } else if (flags & 2) {
        *pm++ = 'w';
        *pm++ = '+';
    } else {
        *pm++ = 'r';
    }

    if (flags & 0x100) {
        *pm++ = 't';
    } else if (flags & 0x200) {
        *pm++ = 'b';
    }

    File* stream = db_fopen(path, mode);
    if (stream == NULL) {
        debug_printf("AudioOpen: Couldn't open %s for read\n", path);
        return -1;
    }

    int index;
    for (index = 0; index < numAudio; index++) {
        if ((audio[index].flags & AUDIO_FILE_IN_USE) == 0) {
            break;
        }
    }

    if (index == numAudio) {
        if (audio != NULL) {
            audio = (AudioFile*)myrealloc(audio, sizeof(*audio) * (numAudio + 1), __FILE__, __LINE__); // "..\int\audio.c", 216
        } else {
            audio = (AudioFile*)mymalloc(sizeof(*audio), __FILE__, __LINE__); // "..\int\audio.c", 218
        }
        numAudio++;
    }

    AudioFile* audioFile = &(audio[index]);
    audioFile->flags = AUDIO_FILE_IN_USE;
    audioFile->fileHandle = (int)stream;

    if (compression == 2) {
        audioFile->flags |= AUDIO_FILE_COMPRESSED;
        audioFile->soundDecoder = soundDecoderInit(decodeRead, audioFile->fileHandle, &(audioFile->field_14), &(audioFile->field_10), &(audioFile->fileSize));
        audioFile->fileSize *= 2;
    } else {
        audioFile->fileSize = db_filelength(stream);
    }

    audioFile->position = 0;

    return index + 1;
}

// 0x41A50C
int audioCloseFile(int fileHandle)
{
    AudioFile* audioFile = &(audio[fileHandle - 1]);
    db_fclose((File*)audioFile->fileHandle);

    if ((audioFile->flags & AUDIO_FILE_COMPRESSED) != 0) {
        soundDecoderFree(audioFile->soundDecoder);
    }

    memset(audioFile, 0, sizeof(AudioFile));

    return 0;
}

// 0x41A574
int audioRead(int fileHandle, void* buffer, unsigned int size)
{
    AudioFile* audioFile = &(audio[fileHandle - 1]);

    int bytesRead;
    if ((audioFile->flags & AUDIO_FILE_COMPRESSED) != 0) {
        bytesRead = soundDecoderDecode(audioFile->soundDecoder, buffer, size);
    } else {
        bytesRead = db_fread(buffer, 1, size, (File*)audioFile->fileHandle);
    }

    audioFile->position += bytesRead;

    return bytesRead;
}

// 0x41A5E0
long audioSeek(int fileHandle, long offset, int origin)
{
    int pos;
    unsigned char* buf;
    int v10;

    AudioFile* audioFile = &(audio[fileHandle - 1]);

    switch (origin) {
    case SEEK_SET:
        pos = offset;
        break;
    case SEEK_CUR:
        pos = offset + audioFile->position;
        break;
    case SEEK_END:
        pos = offset + audioFile->fileSize;
        break;
    default:
        assert(false && "Should be unreachable");
    }

    if ((audioFile->flags & AUDIO_FILE_COMPRESSED) != 0) {
        if (pos < audioFile->position) {
            soundDecoderFree(audioFile->soundDecoder);
            db_fseek((File*)audioFile->fileHandle, 0, SEEK_SET);
            audioFile->soundDecoder = soundDecoderInit(decodeRead, audioFile->fileHandle, &(audioFile->field_14), &(audioFile->field_10), &(audioFile->fileSize));
            audioFile->position = 0;
            audioFile->fileSize *= 2;

            if (pos != 0) {
                buf = (unsigned char*)mymalloc(4096, __FILE__, __LINE__); // "..\int\audio.c", 361
                while (pos > 4096) {
                    pos -= 4096;
                    audioRead(fileHandle, buf, 4096);
                }

                if (pos != 0) {
                    audioRead(fileHandle, buf, pos);
                }

                myfree(buf, __FILE__, __LINE__); // // "..\int\audio.c", 367
            }
        } else {
            buf = (unsigned char*)mymalloc(1024, __FILE__, __LINE__); // "..\int\audio.c", 321
            v10 = audioFile->position - pos;
            while (v10 > 1024) {
                v10 -= 1024;
                audioRead(fileHandle, buf, 1024);
            }

            if (v10 != 0) {
                audioRead(fileHandle, buf, v10);
            }

            // TODO: Probably leaks memory.
        }

        return audioFile->position;
    } else {
        return db_fseek((File*)audioFile->fileHandle, offset, origin);
    }
}

// 0x41A78C
long audioFileSize(int fileHandle)
{
    AudioFile* audioFile = &(audio[fileHandle - 1]);
    return audioFile->fileSize;
}

// 0x41A7A8
long audioTell(int fileHandle)
{
    AudioFile* audioFile = &(audio[fileHandle - 1]);
    return audioFile->position;
}

// 0x41A7C4
int audioWrite(int handle, const void* buf, unsigned int size)
{
    debug_printf("AudioWrite shouldn't be ever called\n");
    return 0;
}

// 0x41A7D4
int initAudio(AudioFileIsCompressedProc* isCompressedProc)
{
    queryCompressedFunc = isCompressedProc;
    audio = NULL;
    numAudio = 0;

    return soundSetDefaultFileIO(audioOpen, audioCloseFile, audioRead, audioWrite, audioSeek, audioTell, audioFileSize);
}

// 0x41A818
void audioClose()
{
    if (audio != NULL) {
        myfree(audio, __FILE__, __LINE__); // "..\int\audio.c", 406
    }

    numAudio = 0;
    audio = NULL;
}
