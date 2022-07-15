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
    int width;
    int height;
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

extern int _holdTime;
extern int _checkRegionEnable;
extern int _winTOS;
extern int gCurrentManagedWindowIndex;
extern INITVIDEOFN _gfx_init[12];
extern Size _sizes_x[12];

extern int _winStack[MANAGED_WINDOW_COUNT];
extern char _alphaBlendTable[64 * 256];
extern ManagedWindow gManagedWindows[MANAGED_WINDOW_COUNT];

extern void(*_selectWindowFunc)(int, ManagedWindow*);
extern int _xres;
extern int _yres;
extern int _currentHighlightColorR;
extern int gWidgetFont;
extern int _currentTextColorG;
extern int _currentTextColorB;
extern int gWidgetTextFlags;
extern int _currentTextColorR;
extern int _currentHighlightColorG;
extern int _currentHighlightColorB;

int sub_4B6DE8(const char* regionName, int a2);
int _windowWidth();
int _windowHeight();
bool _windowDraw();
bool _deleteWindow(const char* windowName);
int sub_4B7AC4(const char* windowName, int x, int y, int width, int height);
int sub_4B7E7C(const char* windowName, int x, int y, int width, int height);
int sub_4B7F3C(const char* windowName, int x, int y, int width, int height, int a6, int flags);
int _windowOutput(char* string);
bool sub_4B814C(int x, int y);
bool _selectWindowID(int index);
int _selectWindow(const char* windowName);
unsigned char* _windowGetBuffer();
int _pushWindow(const char* windowName);
int _popWindow();
void _windowPrintBuf(int win, char* string, int stringLength, int width, int maxY, int x, int y, int flags, int textAlignment);
char** _windowWordWrap(char* string, int maxLength, int a3, int* substringListLengthPtr);
void _windowFreeWordList(char** substringList, int substringListLength);
void _windowWrapLineWithSpacing(int win, char* string, int width, int height, int x, int y, int flags, int textAlignment, int a9);
void _windowWrapLine(int win, char* string, int width, int height, int x, int y, int flags, int textAlignment);
bool _windowPrintRect(char* string, int a2, int textAlignment);
bool _windowFormatMessage(char* string, int x, int y, int width, int height, int textAlignment);
bool sub_4B99C8(const char* buttonName, int x, int y, int width, int height, int flags);
void sub_4B8C68(const char* fileName);
void sub_4B8CA8(const char* fileName);
int _windowGetXres();
int _windowGetYres();
void _removeProgramReferences_3(Program* program);
void _initWindow(int resolution, int a2);
void _windowClose();
bool _windowDeleteButton(const char* buttonName);
bool _windowSetButtonFlag(const char* buttonName, int value);
bool sub_4B9DD0(const char* buttonName, const char* a2, const char* a3, const char* a4);
bool _windowAddButtonProc(const char* buttonName, Program* program, int a3, int a4, int a5, int a6);
bool _windowAddButtonRightProc(const char* buttonName, Program* program, int a3, int a4);
bool sub_4BA34C(const char* buttonName, const char* text);
bool sub_4BA364(const char* buttonName, const char* text, int a3, int a4, int a5, int a6);
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
void sub_4BBFC4(unsigned char* src, int srcWidth, int srcHeight, unsigned char* dest, int destWidth, int destHeight);

#endif /* WINDOW_H */
