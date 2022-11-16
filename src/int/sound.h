#ifndef FALLOUT_INT_SOUND_H_
#define FALLOUT_INT_SOUND_H_

#include "memory_defs.h"
#include "plib/gnw/winmain.h"

#define SOUND_FLAG_SOUND_IS_DONE 0x01
#define SOUND_FLAG_SOUND_IS_PLAYING 0x02
#define SOUND_FLAG_SOUND_IS_FADING 0x04
#define SOUND_FLAG_SOUND_IS_PAUSED 0x08

#define VOLUME_MIN 0
#define VOLUME_MAX 0x7FFF

typedef enum SoundError {
    SOUND_NO_ERROR = 0,
    SOUND_SOS_DRIVER_NOT_LOADED = 1,
    SOUND_SOS_INVALID_POINTER = 2,
    SOUND_SOS_DETECT_INITIALIZED = 3,
    SOUND_SOS_FAIL_ON_FILE_OPEN = 4,
    SOUND_SOS_MEMORY_FAIL = 5,
    SOUND_SOS_INVALID_DRIVER_ID = 6,
    SOUND_SOS_NO_DRIVER_FOUND = 7,
    SOUND_SOS_DETECTION_FAILURE = 8,
    SOUND_SOS_DRIVER_LOADED = 9,
    SOUND_SOS_INVALID_HANDLE = 10,
    SOUND_SOS_NO_HANDLES = 11,
    SOUND_SOS_PAUSED = 12,
    SOUND_SOS_NO_PAUSED = 13,
    SOUND_SOS_INVALID_DATA = 14,
    SOUND_SOS_DRV_FILE_FAIL = 15,
    SOUND_SOS_INVALID_PORT = 16,
    SOUND_SOS_INVALID_IRQ = 17,
    SOUND_SOS_INVALID_DMA = 18,
    SOUND_SOS_INVALID_DMA_IRQ = 19,
    SOUND_NO_DEVICE = 20,
    SOUND_NOT_INITIALIZED = 21,
    SOUND_NO_SOUND = 22,
    SOUND_FUNCTION_NOT_SUPPORTED = 23,
    SOUND_NO_BUFFERS_AVAILABLE = 24,
    SOUND_FILE_NOT_FOUND = 25,
    SOUND_ALREADY_PLAYING = 26,
    SOUND_NOT_PLAYING = 27,
    SOUND_ALREADY_PAUSED = 28,
    SOUND_NOT_PAUSED = 29,
    SOUND_INVALID_HANDLE = 30,
    SOUND_NO_MEMORY_AVAILABLE = 31,
    SOUND_UNKNOWN_ERROR = 32,
    SOUND_ERR_COUNT,
} SoundError;

typedef char*(SoundFileNameMangler)(char*);
typedef int SoundOpenProc(const char* filePath, int flags, ...);
typedef int SoundCloseProc(int fileHandle);
typedef int SoundReadProc(int fileHandle, void* buf, unsigned int size);
typedef int SoundWriteProc(int fileHandle, const void* buf, unsigned int size);
typedef long SoundSeekProc(int fileHandle, long offset, int origin);
typedef long SoundTellProc(int fileHandle);
typedef long SoundFileLengthProc(int fileHandle);

typedef struct SoundFileIO {
    SoundOpenProc* open;
    SoundCloseProc* close;
    SoundReadProc* read;
    SoundWriteProc* write;
    SoundSeekProc* seek;
    SoundTellProc* tell;
    SoundFileLengthProc* filelength;
    int fd;
} SoundFileIO;

typedef void SoundCallback(void* userData, int a2);

typedef struct Sound {
    SoundFileIO io;
    unsigned char* field_20;
    LPDIRECTSOUNDBUFFER directSoundBuffer;
    DSBUFFERDESC directSoundBufferDescription;
    int field_3C;
    // flags
    int field_40;
    int field_44;
    // pause pos
    int field_48;
    int volume;
    int field_50;
    int field_54;
    int field_58;
    int field_5C;
    // file size
    int field_60;
    int field_64;
    int field_68;
    int readLimit;
    int field_70;
    DWORD field_74;
    int field_78;
    int field_7C;
    int field_80;
    // callback data
    void* callbackUserData;
    SoundCallback* callback;
    int field_8C;
    void (*field_90)(int);
    struct Sound* next;
    struct Sound* prev;
} Sound;

extern LPDIRECTSOUNDBUFFER primaryDSBuffer;
extern LPDIRECTSOUND soundDSObject;

void soundRegisterAlloc(MallocProc* mallocProc, ReallocProc* reallocProc, FreeProc* freeProc);
const char* soundError(int err);
int soundInit(int a1, int a2, int a3, int a4, int rate);
void soundClose();
Sound* soundAllocate(int a1, int a2);
int soundLoad(Sound* sound, char* filePath);
int soundRewind(Sound* sound);
int soundSetData(Sound* sound, unsigned char* buf, int size);
int soundPlay(Sound* sound);
int soundStop(Sound* sound);
int soundDelete(Sound* sound);
int soundContinue(Sound* sound);
bool soundPlaying(Sound* sound);
bool soundDone(Sound* sound);
bool soundPaused(Sound* sound);
int soundType(Sound* sound, int a2);
int soundLength(Sound* sound);
int soundLoop(Sound* sound, int a2);
int soundVolumeHMItoDirectSound(int a1);
int soundVolume(Sound* sound, int volume);
int soundGetVolume(Sound* sound);
int soundSetCallback(Sound* sound, SoundCallback* callback, void* userData);
int soundSetChannel(Sound* sound, int channels);
int soundSetReadLimit(Sound* sound, int readLimit);
int soundPause(Sound* sound);
int soundUnpause(Sound* sound);
int soundSetFileIO(Sound* sound, SoundOpenProc* openProc, SoundCloseProc* closeProc, SoundReadProc* readProc, SoundWriteProc* writeProc, SoundSeekProc* seekProc, SoundTellProc* tellProc, SoundFileLengthProc* fileLengthProc);
void soundMgrDelete(Sound* sound);
int soundSetMasterVolume(int value);
int soundGetPosition(Sound* sound);
int soundSetPosition(Sound* sound, int a2);
int soundFade(Sound* sound, int duration, int targetVolume);
void soundFlushAllSounds();
void soundContinueAll();
int soundSetDefaultFileIO(SoundOpenProc* openProc, SoundCloseProc* closeProc, SoundReadProc* readProc, SoundWriteProc* writeProc, SoundSeekProc* seekProc, SoundTellProc* tellProc, SoundFileLengthProc* fileLengthProc);

#endif /* FALLOUT_INT_SOUND_H_ */
