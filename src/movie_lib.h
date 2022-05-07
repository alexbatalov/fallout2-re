#ifndef MOVIE_LIB_H
#define MOVIE_LIB_H

#include "memory_defs.h"

#define DIRECTDRAW_VERSION 0x0300
#include <ddraw.h>

#define DIRECTSOUND_VERSION 0x0300
#include <dsound.h>

#include <stdbool.h>

typedef struct STRUCT_6B3690 {
    void* field_0;
    int field_4;
    int field_8;
} STRUCT_6B3690;

#pragma pack(2)
typedef struct Mve {
    char sig[20];
    short field_14;
    short field_16;
    short field_18;
    int field_1A;
} Mve;
#pragma pack()

typedef bool MovieReadProc(int fileHandle, void* buffer, int count);

typedef struct STRUCT_4F6930 {
    int field_0;
    MovieReadProc* readProc;
    STRUCT_6B3690 field_8;
    int fileHandle;
    int field_18;
    LPDIRECTDRAWSURFACE field_24;
    LPDIRECTDRAWSURFACE field_28;
    int field_2C;
    unsigned char* field_30;
    unsigned char* field_34;
    unsigned char field_38;
    unsigned char field_39;
    unsigned char field_3A;
    unsigned char field_3B;
    int field_3C;
    int field_40;
    int field_44;
    int field_48;
    int field_4C;
    int field_50;
} STRUCT_4F6930;

extern int dword_51EBD8;
extern int dword_51EBDC;
extern unsigned short word_51EBE0[256];
extern LPDIRECTDRAW gMovieLibDirectDraw;
extern int dword_51EDE4;
extern int dword_51EDE8;
extern int dword_51EDEC;
extern LPDIRECTSOUND gMovieLibDirectSound;
extern LPDIRECTSOUNDBUFFER gMovieLibDirectSoundBuffer;
extern int gMovieLibVolume;
extern int gMovieLibPan;
extern LPDIRECTDRAWSURFACE gMovieDirectDrawSurface1;
extern LPDIRECTDRAWSURFACE gMovieDirectDrawSurface2;
extern void (*off_51EE08)(LPDIRECTDRAWSURFACE, int, int, int, int, int, int, int, int);
extern int dword_51EE0C;
extern void (*off_51EE14)(unsigned char*, int, int);
extern int dword_51EE18;
extern int dword_51EE1C;
extern BOOL dword_51EE20;
extern int dword_51F018[256];
extern unsigned short word_51F418[256];
extern unsigned short word_51F618[256];
extern unsigned int dword_51F818[16];
extern unsigned int dword_51F858[256];
extern unsigned int dword_51FC58[256];

extern int dword_6B3660;
extern DSBCAPS stru_6B3668;
extern int dword_6B367C;
extern int dword_6B3680;
extern int dword_6B3684;
extern int dword_6B3688;
extern STRUCT_6B3690 stru_6B3690;
extern int dword_6B369C;
extern int dword_6B36A0;
extern int dword_6B36A4;
extern int dword_6B36A8;
extern int dword_6B36AC;
extern int dword_6B36B0;
extern unsigned char byte_6B36B8[768];
extern MallocProc* gMovieLibMallocProc;
extern int (*off_6B39BC)();
extern int dword_6B39C0;
extern int dword_6B39C4;
extern int dword_6B39C8;
extern int off_6B39CC;
extern int dword_6B39D0;
extern FreeProc* gMovieLibFreeProc;
extern int dword_6B39D8;
extern unsigned char* off_6B39DC;
extern int dword_6B39E0[60];
extern int dword_6B3AD0;
extern int dword_6B3AD4;
extern int dword_6B3AD8;
extern int dword_6B3ADC;
extern MovieReadProc* gMovieLibReadProc;
extern int dword_6B3AE4;
extern int dword_6B3AE8;
extern int dword_6B3CEC;
extern int dword_6B3CF0;
extern int dword_6B3CF4;
extern int dword_6B3CF8;
extern int dword_6B3CFC;
extern int dword_6B3D00;
extern int dword_6B3D04;
extern int dword_6B3D08;
extern unsigned char byte_6B3D0C[768];
extern unsigned char byte_6B400C;
extern unsigned char byte_6B400D;
extern int dword_6B400E;
extern int dword_6B4012;
extern unsigned char byte_6B4016;
extern int dword_6B4017;
extern int dword_6B401B;
extern int dword_6B401F;
extern int dword_6B4023;
extern int dword_6B4027;
extern int dword_6B402B;
extern int dword_6B402F;
extern unsigned char* gMovieDirectDrawSurfaceBuffer1;
extern unsigned char* gMovieDirectDrawSurfaceBuffer2;
extern int dword_6B403B;
extern int dword_6B403F;

void movieLibSetMemoryProcs(MallocProc* mallocProc, FreeProc* freeProc);
void movieLibSetReadProc(MovieReadProc* readProc);
void _MVE_MemInit(STRUCT_6B3690* a1, int a2, void* a3);
void _MVE_MemFree(STRUCT_6B3690* a1);
void movieLibSetDirectSound(LPDIRECTSOUND ds);
void movieLibSetVolume(int volume);
void movieLibSetPan(int pan);
void _MVE_sfSVGA(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9);
void _MVE_sfCallbacks(void (*fn)(LPDIRECTDRAWSURFACE, int, int, int, int, int, int, int, int));
void sub_4F4A00(LPDIRECTDRAWSURFACE a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9);
void movieLibSetPaletteEntriesProc(void (*fn)(unsigned char*, int, int));
int _sub_4F4B5();
void movieLibSetDirectDraw(LPDIRECTDRAW dd);
void _MVE_rmCallbacks(int (*fn)());
void _sub_4F4BB(int a1);
void _MVE_rmFrameCounts(int* a1, int* a2);
int _MVE_rmPrepMovie(int fileHandle, int a2, int a3, char a4);
int _ioReset(int fileHandle);
void* _ioRead(int size);
void* _MVE_MemAlloc(STRUCT_6B3690* a1, unsigned int a2);
unsigned char* _ioNextRecord();
void _sub_4F4DD();
int _MVE_rmHoldMovie();
int _syncWait();
void _MVE_sndPause();
int _MVE_rmStepMovie();
int _syncInit(int a1, int a2);
void _syncReset(int a1);
int _MVE_sndConfigure(int a1, int a2, int a3, int a4, int a5, int a6);
void _MVE_syncSync();
void _MVE_sndReset();
void _MVE_sndSync();
int _syncWaitLevel(int a1);
void _CallsSndBuff_Loc(unsigned char* a1, int a2);
int _MVE_sndAdd(unsigned char* dest, unsigned char** src_ptr, int a3, int a4, int a5);
void _MVE_sndResume();
int _nfConfig(int a1, int a2, int a3, int a4);
bool movieLockSurfaces();
void movieUnlockSurfaces();
void movieSwapSurfaces();
void _sfShowFrame(int a1, int a2, int a3);
void _do_nothing_(int a1, int a2, unsigned short* a3);
void sub_4F6090(int a1, int a2);
void _SetPalette_(int a1, int a2);
void _palMakeSynthPalette(int a1, int a2, int a3, int a4, int a5, int a6);
void _palLoadPalette(unsigned char* palette, int a2, int a3);
void _MVE_rmEndMovie();
void _syncRelease();
void _MVE_ReleaseMem();
void _ioRelease();
void _MVE_sndRelease();
void _nfRelease();
void _frLoad(STRUCT_4F6930* a1);
void _frSave(STRUCT_4F6930* a1);
void _MVE_frClose(STRUCT_4F6930* a1);
int _MVE_sndDecompM16(unsigned short* a1, unsigned char* a2, int a3, int a4);
int _MVE_sndDecompS16(unsigned short* a1, unsigned char* a2, int a3, int a4);
void _nfPkConfig();
void _nfPkDecomp(unsigned char* buf, unsigned char* a2, int a3, int a4, int a5, int a6);

#endif /* MOVIE_LIB_H */
