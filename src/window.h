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

typedef struct ManagedButton {
    int btn;
    int field_4;
    int field_8;
    int field_C;
    int field_10;
    int flags;
    int field_18;
    char name[32];
    Program* program;
    void* field_40;
    void* field_44;
    void* field_48;
    void* field_4C;
    void* field_50;
    int field_54;
    int field_58;
    int field_5C;
    int field_60;
    int field_64;
    int field_68;
    int field_6C;
    int field_70;
    int field_74;
    int field_78;
} ManagedButton;
static_assert(sizeof(ManagedButton) == 0x7C, "wrong size");

typedef struct STRUCT_6727B0 {
    char field_0[32];
    int window;
    int field_24;
    int field_28;
    Region** regions;
    int currentRegionIndex;
    int regionsLength;
    int field_38;
    ManagedButton* buttons;
    int buttonsLength;
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

extern void(*off_672D78)(int, STRUCT_6727B0*);
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

bool selectWindowID(int index);
void windowPrintBuf(int win, char* string, int stringLength, int width, int maxY, int x, int y, int flags, int textAlignment);
char** windowWordWrap(char* string, int maxLength, int a3, int* substringListLengthPtr);
void windowFreeWordList(char** substringList, int substringListLength);
void windowWrapLineWithSpacing(int win, char* string, int width, int height, int x, int y, int flags, int textAlignment, int a9);
void windowWrapLine(int win, char* string, int width, int height, int x, int y, int flags, int textAlignment);
int windowGetXres();
int windowGetYres();
void removeProgramReferences_3(Program* program);
void initWindow(int resolution, int a2);
void windowClose();
bool windowDeleteButton(const char* buttonName);
bool windowSetButtonFlag(const char* buttonName, int value);
bool windowAddButtonProc(const char* buttonName, Program* program, int a3, int a4, int a5, int a6);
bool windowAddButtonRightProc(const char* buttonName, Program* program, int a3, int a4);
void windowEndRegion();
bool windowCheckRegionExists(const char* regionName);
bool windowStartRegion(int initialCapacity);
bool windowAddRegionPoint(int x, int y, bool a3);
bool windowAddRegionProc(const char* regionName, Program* program, int a3, int a4, int a5, int a6);
bool windowAddRegionRightProc(const char* regionName, Program* program, int a3, int a4);
bool windowSetRegionFlag(const char* regionName, int value);
bool windowAddRegionName(const char* regionName);
bool windowDeleteRegion(const char* regionName);
void updateWindows();
int windowMoviePlaying();
bool windowSetMovieFlags(int flags);
bool windowPlayMovie(char* filePath);
bool windowPlayMovieRect(char* filePath, int a2, int a3, int a4, int a5);
void windowStopMovie();

#endif /* WINDOW_H */
