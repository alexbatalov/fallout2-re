#ifndef SOUND_DECODER_H
#define SOUND_DECODER_H

#include <stdbool.h>
#include <stddef.h>

#define SOUND_DECODER_IN_BUFFER_SIZE (512)

typedef int(SoundDecoderReadProc)(int fileHandle, void* buffer, unsigned int size);

typedef struct SoundDecoder {
    SoundDecoderReadProc* readProc;
    int fd;
    unsigned char* bufferIn;
    size_t bufferInSize;

    // Next input byte.
    unsigned char* nextIn;

    // Number of bytes remaining in the input buffer.
    int remainingInSize;

    // Bit accumulator.
    int hold;

    // Number of bits in bit accumulator.
    int bits;
    int field_20;
    int field_24;
    int field_28;
    int field_2C;
    unsigned char* field_30;
    unsigned char* field_34;
    int field_38;
    int field_3C;
    int field_40;
    int field_44;
    int field_48;
    unsigned char* field_4C;
    int field_50;
} SoundDecoder;

#if _WIN32
static_assert(sizeof(SoundDecoder) == 84, "wrong size");
#endif

typedef int (*DECODINGPROC)(SoundDecoder* soundDecoder, int offset, int bits);

extern int gSoundDecodersCount;
extern bool dword_51E32C;
extern DECODINGPROC off_51E330[32];
extern unsigned char byte_6AD960[128];
extern unsigned char byte_6AD9E0[32];
extern unsigned short word_6ADA00[128];
extern unsigned char* off_6ADB00;
extern unsigned char* off_6ADB04;

bool soundDecoderPrepare(SoundDecoder* a1, SoundDecoderReadProc* readProc, int fileHandle);
unsigned char soundDecoderReadNextChunk(SoundDecoder* a1);
void init_pack_tables();

int sub_4D3D9C(SoundDecoder* soundDecoder, int offset, int bits);
int sub_4D3DA0(SoundDecoder* soundDecoder, int offset, int bits);
int sub_4D3DC8(SoundDecoder* soundDecoder, int offset, int bits);
int sub_4D3E90(SoundDecoder* soundDecoder, int offset, int bits);
int sub_4D3F98(SoundDecoder* soundDecoder, int offset, int bits);
int sub_4D4068(SoundDecoder* soundDecoder, int offset, int bits);
int sub_4D4158(SoundDecoder* soundDecoder, int offset, int bits);
int sub_4D4254(SoundDecoder* soundDecoder, int offset, int bits);
int sub_4D4338(SoundDecoder* soundDecoder, int offset, int bits);
int sub_4D4434(SoundDecoder* soundDecoder, int offset, int bits);
int sub_4D4584(SoundDecoder* soundDecoder, int offset, int bits);
int sub_4D4698(SoundDecoder* soundDecoder, int offset, int bits);
int sub_4D47A4(SoundDecoder* soundDecoder, int offset, int bits);
int sub_4D4870(SoundDecoder* soundDecoder, int offset, int bits);

int sub_4D493C(SoundDecoder* ptr);
void untransform_subband0(unsigned char* a1, unsigned char* a2, int a3, int a4);
void untransform_subband(unsigned char* a1, unsigned char* a2, int a3, int a4);
void untransform_all(SoundDecoder* a1);
size_t soundDecoderDecode(SoundDecoder* soundDecoder, void* buffer, size_t size);
void soundDecoderFree(SoundDecoder* soundDecoder);
SoundDecoder* soundDecoderInit(SoundDecoderReadProc* readProc, int fileHandle, int* out_a3, int* out_a4, int* out_a5);

#endif /* SOUND_DECODER_H */
