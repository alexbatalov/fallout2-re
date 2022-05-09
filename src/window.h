#ifndef WINDOW_H
#define WINDOW_H

#include "geometry.h"
#include "interpreter.h"
#include "region.h"

#include <stdbool.h>

#define MANAGED_WINDOW_COUNT (16)

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

typedef struct ManagedWindow {
    char name[32];
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
} ManagedWindow;

typedef int (*INITVIDEOFN)();

extern int dword_51DCAC;
extern int dword_51DCB0;
extern int dword_51DCB4;
extern int gCurrentManagedWindowIndex;
extern INITVIDEOFN off_51DCBC[12];
extern Size stru_51DD1C[12];

extern int dword_66E770[MANAGED_WINDOW_COUNT];
extern char byte_66E7B0[64 * 256];
extern ManagedWindow gManagedWindows[MANAGED_WINDOW_COUNT];

extern void(*off_672D78)(int, ManagedWindow*);
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

bool sub_4B7680();
bool _selectWindowID(int index);
void _windowPrintBuf(int win, char* string, int stringLength, int width, int maxY, int x, int y, int flags, int textAlignment);
char** _windowWordWrap(char* string, int maxLength, int a3, int* substringListLengthPtr);
void _windowFreeWordList(char** substringList, int substringListLength);
void _windowWrapLineWithSpacing(int win, char* string, int width, int height, int x, int y, int flags, int textAlignment, int a9);
void _windowWrapLine(int win, char* string, int width, int height, int x, int y, int flags, int textAlignment);
bool sub_4B8920(char* string, int a2, int textAlignment);
int _windowGetXres();
int _windowGetYres();
void _removeProgramReferences_3(Program* program);
void _initWindow(int resolution, int a2);
void _windowClose();
bool _windowDeleteButton(const char* buttonName);
bool _windowSetButtonFlag(const char* buttonName, int value);
bool _windowAddButtonProc(const char* buttonName, Program* program, int a3, int a4, int a5, int a6);
bool _windowAddButtonRightProc(const char* buttonName, Program* program, int a3, int a4);
void _windowEndRegion();
bool _windowCheckRegionExists(const char* regionName);
bool _windowStartRegion(int initialCapacity);
bool _windowAddRegionPoint(int x, int y, bool a3);
bool _windowAddRegionProc(const char* regionName, Program* program, int a3, int a4, int a5, int a6);
bool _windowAddRegionRightProc(const char* regionName, Program* program, int a3, int a4);
bool _windowSetRegionFlag(const char* regionName, int value);
bool _windowAddRegionName(const char* regionName);
bool _windowDeleteRegion(const char* regionName);
void _updateWindows();
int _windowMoviePlaying();
bool _windowSetMovieFlags(int flags);
bool _windowPlayMovie(char* filePath);
bool _windowPlayMovieRect(char* filePath, int a2, int a3, int a4, int a5);
void _windowStopMovie();

#endif /* WINDOW_H */
