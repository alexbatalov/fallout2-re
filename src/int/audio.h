#ifndef FALLOUT_INT_AUDIO_H_
#define FALLOUT_INT_AUDIO_H_

#include "audio_file.h"

int audioOpen(const char* fname, int mode, ...);
int audioCloseFile(int fileHandle);
int audioRead(int fileHandle, void* buffer, unsigned int size);
long audioSeek(int fileHandle, long offset, int origin);
long audioFileSize(int fileHandle);
long audioTell(int fileHandle);
int audioWrite(int handle, const void* buf, unsigned int size);
int initAudio(AudioFileIsCompressedProc* isCompressedProc);
void audioClose();

#endif /* FALLOUT_INT_AUDIO_H_ */
