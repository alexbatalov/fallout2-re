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
void sub_4F4890(STRUCT_6B3690* a1, int a2, void* a3);
void sub_4F48C0(STRUCT_6B3690* a1);
void movieLibSetDirectSound(LPDIRECTSOUND ds);
void movieLibSetVolume(int volume);
void movieLibSetPan(int pan);
void sub_4F4940(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9);
void sub_4F49F0(void (*fn)(LPDIRECTDRAWSURFACE, int, int, int, int, int, int, int, int));
void sub_4F4A00(LPDIRECTDRAWSURFACE a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9);
void movieLibSetPaletteEntriesProc(void (*fn)(unsigned char*, int, int));
int sub_4F4B50();
void movieLibSetDirectDraw(LPDIRECTDRAW dd);
void sub_4F4B90(int (*fn)());
void sub_4F4BB0(int a1);
void sub_4F4BD0(int* a1, int* a2);
int sub_4F4BF0(int fileHandle, int a2, int a3, char a4);
int sub_4F4C90(int fileHandle);
void* sub_4F4D00(int size);
void* sub_4F4D40(STRUCT_6B3690* a1, unsigned int a2);
unsigned char* sub_4F4DA0();
void sub_4F4DD0();
int sub_4F4E20();
int sub_4F4E40();
void sub_4F4EA0();
int sub_4F4EC0();
int sub_4F54F0(int a1, int a2);
void sub_4F5540(int a1);
int sub_4F5570(int a1, int a2, int a3, int a4, int a5, int a6);
void sub_4F56C0();
void sub_4F56F0();
void sub_4F5720();
int sub_4F59B0(int a1);
void sub_4F5A00(unsigned char* a1, int a2);
int sub_4F5B70(unsigned char* dest, unsigned char** src_ptr, int a3, int a4, int a5);
void sub_4F5CA0();
int sub_4F5CB0(int a1, int a2, int a3, int a4);
bool movieLockSurfaces();
void movieUnlockSurfaces();
void movieSwapSurfaces();
void sub_4F5F40(int a1, int a2, int a3);
void sub_4F6080(int a1, int a2, unsigned short* a3);
void sub_4F6090(int a1, int a2);
void sub_4F60C0(int a1, int a2);
void sub_4F60F0(int a1, int a2, int a3, int a4, int a5, int a6);
void sub_4F6210(unsigned char* palette, int a2, int a3);
void sub_4F6240();
void sub_4F6270();
void sub_4F6350();
void sub_4F6370();
void sub_4F6380();
void sub_4F6390();
void sub_4F6550(STRUCT_4F6930* a1);
void sub_4F6610(STRUCT_4F6930* a1);
void sub_4F6930(STRUCT_4F6930* a1);
int sub_4F697C(unsigned short* a1, unsigned char* a2, int a3, int a4);
int sub_4F69AD(unsigned short* a1, unsigned char* a2, int a3, int a4);
void sub_4F731D();
void sub_4F7359(unsigned char* buf, unsigned char* a2, int a3, int a4, int a5, int a6);

#endif /* MOVIE_LIB_H */
