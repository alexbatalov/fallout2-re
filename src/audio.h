#ifndef AUDIO_H
#define AUDIO_H

#include "audio_file.h"

extern AudioFileIsCompressedProc* off_5108BC;

extern int gAudioListLength;
extern AudioFile* gAudioList;

bool sub_41A2B0(char* filePath);
int audioSoundDecoderReadHandler(int fileHandle, void* buf, unsigned int size);
int audioOpen(const char* fname, int mode, ...);
int audioClose(int fileHandle);
int audioRead(int fileHandle, void* buffer, unsigned int size);
int audioSeek(int fileHandle, long offset, int origin);
long audioGetSize(int fileHandle);
long audioTell(int fileHandle);
int audioWrite(int handle, const void* buf, unsigned int size);
int audioInit(AudioFileIsCompressedProc* isCompressedProc);
void audioExit();

#endif /* AUDIO_H */
