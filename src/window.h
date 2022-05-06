#ifndef WINDOW_H
#define WINDOW_H

#include "geometry.h"
#include "interpreter.h"
#include "region.h"

#include <stdbool.h>

typedef void (*WINDOWDRAWINGPROC)(unsigned char* src, int src_pitch, int a3, int src_x, int src_y, int src_width, int src_height, int dest_x, int dest_y);
typedef void WindowDrawingProc2(unsigned char* buf, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, unsigned char a10);

typedef enum TextAlignment {
    TEXT_ALIGNMENT_LEFT,
    TEXT_ALIGNMENT_RIGHT,
    TEXT_ALIGNMENT_CENTER,
} TextAlignment;

typedef struct STRUCT_6727B0 {
    char field_0[32];
    int window;
    int field_24;
    int field_28;
    Region** regions;
    int currentRegionIndex;
    int regionsLength;
    int field_38;
    int field_3C;
    int field_40;
    int field_44;
    int field_48;
    int field_4C;
    int field_50;
    float field_54;
    float field_58;
} STRUCT_6727B0;

typedef int (*INITVIDEOFN)();

extern int dword_51DCAC;
extern int dword_51DCB0;
extern int dword_51DCB4;
extern int dword_51DCB8;
extern INITVIDEOFN off_51DCBC[12];
extern Size stru_51DD1C[12];

extern int dword_66E770[16];
extern char byte_66E7B0[64 * 256];
extern STRUCT_6727B0 stru_6727B0[16];

extern int dword_672D7C;
extern int dword_672D88;
extern int dword_672D8C;
extern int gWidgetFont;
extern int dword_672DA0;
extern int dword_672DA4;
extern int gWidgetTextFlags;
extern int dword_672DAC;
extern int dword_672DB0;
extern int dword_672DB4;

void sub_4B8414(int win, char* string, int stringLength, int width, int maxY, int x, int y, int flags, int textAlignment);
char** sub_4B8638(char* string, int maxLength, int a3, int* substringListLengthPtr);
void sub_4B880C(char** substringList, int substringListLength);
void sub_4B8854(int win, char* string, int width, int height, int x, int y, int flags, int textAlignment, int a9);
void sub_4B88FC(int win, char* string, int width, int height, int x, int y, int flags, int textAlignment);
int sub_4B9048();
int sub_4B9050();
void sub_4B9058(Program* program);
void sub_4B9190(int resolution, int a2);
void sub_4B947C();
bool sub_4BA988(const char* regionName);
bool sub_4BA9FC(int initialCapacity);
bool sub_4BAB68(int x, int y, bool a3);
bool sub_4BADC0(const char* regionName, int a2, int a3, int a4, int a5, int a6);
bool sub_4BAE8C(const char* regionName, int a2, int a3, int a4);
bool sub_4BAF2C(const char* regionName, int value);
bool sub_4BAFA8(const char* regionName);
bool sub_4BB0A8(const char* regionName);
void sub_4BB220();
int sub_4BB234();
bool sub_4BB23C(int flags);
bool sub_4BB24C(char* filePath);
bool sub_4BB280(char* filePath, int a2, int a3, int a4, int a5);
void sub_4BB2C4();

#endif /* WINDOW_H */
