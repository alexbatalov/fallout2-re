#ifndef FALLOUT_INT_AUDIOF_H_
#define FALLOUT_INT_AUDIOF_H_

#include <stdbool.h>

#include "sound_decoder.h"

typedef enum AudioFileFlags {
    AUDIO_FILE_IN_USE = 0x01,
    AUDIO_FILE_COMPRESSED = 0x02,
} AudioFileFlags;

typedef struct AudioFile {
    int flags;
    int fileHandle;
    SoundDecoder* soundDecoder;
    int fileSize;
    int field_10;
    int field_14;
    int position;
} AudioFile;

typedef bool(AudioFileIsCompressedProc)(char* filePath);

int audiofOpen(const char* fname, int flags, ...);
int audiofCloseFile(int a1);
int audiofRead(int a1, void* buf, unsigned int size);
long audiofSeek(int handle, long offset, int origin);
long audiofFileSize(int a1);
long audiofTell(int a1);
int audiofWrite(int handle, const void* buf, unsigned int size);
int initAudiof(AudioFileIsCompressedProc* isCompressedProc);
void audiofClose();

#endif /* FALLOUT_INT_AUDIOF_H_ */
