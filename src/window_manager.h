#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include "geometry.h"

#include <stdbool.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define MAX_WINDOW_COUNT (50)

// The maximum number of radio groups.
#define RADIO_GROUP_LIST_CAPACITY (64)

// The maximum number of buttons in one radio group.
#define RADIO_GROUP_BUTTON_LIST_CAPACITY (64)

typedef enum WindowManagerErr {
    WINDOW_MANAGER_OK = 0,
    WINDOW_MANAGER_ERR_INITIALIZING_VIDEO_MODE = 1,
    WINDOW_MANAGER_ERR_NO_MEMORY = 2,
    WINDOW_MANAGER_ERR_INITIALIZING_TEXT_FONTS = 3,
    WINDOW_MANAGER_ERR_WINDOW_SYSTEM_ALREADY_INITIALIZED = 4,
    WINDOW_MANAGER_ERR_WINDOW_SYSTEM_NOT_INITIALIZED = 5,
    WINDOW_MANAGER_ERR_CURRENT_WINDOWS_TOO_BIG = 6,
    WINDOW_MANAGER_ERR_INITIALIZING_DEFAULT_DATABASE = 7,

    // Unknown fatal error.
    //
    // NOTE: When this error code returned from window system initialization, the
    // game simply exits without any debug message. There is no way to figure out
    // it's meaning.
    WINDOW_MANAGER_ERR_8 = 8,
    WINDOW_MANAGER_ERR_ALREADY_RUNNING = 9,
    WINDOW_MANAGER_ERR_TITLE_NOT_SET = 10,
    WINDOW_MANAGER_ERR_INITIALIZING_INPUT = 11,
} WindowManagerErr;

typedef enum WindowFlags {
    WINDOW_FLAG_0x01 = 0x01,
    WINDOW_FLAG_0x02 = 0x02,
    WINDOW_FLAG_0x04 = 0x04,
    WINDOW_HIDDEN = 0x08,
    WINDOW_FLAG_0x10 = 0x10,
    WINDOW_FLAG_0x20 = 0x20,
    WINDOW_FLAG_0x40 = 0x40,
    WINDOW_FLAG_0x80 = 0x80,
    WINDOW_FLAG_0x0100 = 0x0100,
} WindowFlags;

typedef enum ButtonFlags {
    BUTTON_FLAG_0x01 = 0x01,
    BUTTON_FLAG_0x02 = 0x02,
    BUTTON_FLAG_0x04 = 0x04,
    BUTTON_FLAG_DISABLED = 0x08,
    BUTTON_FLAG_0x10 = 0x10,
    BUTTON_FLAG_TRANSPARENT = 0x20,
    BUTTON_FLAG_0x40 = 0x40,
    BUTTON_FLAG_0x010000 = 0x010000,
    BUTTON_FLAG_0x020000 = 0x020000,
    BUTTON_FLAG_0x040000 = 0x040000,
    BUTTON_FLAG_RIGHT_MOUSE_BUTTON_CONFIGURED = 0x080000,
} ButtonFlags;

typedef struct struc_176 {
    int field_0;
    int field_4;
    int field_8;
    int field_C;
    int field_10;
    int field_14;
    int field_18;
    int field_1C;
    int field_20;
} struc_176;

typedef struct struc_177 {
    int win;
    Rect rect;
    int entriesCount;
    struc_176 entries[15];
    int field_234;
    int field_238;
} struc_177;

typedef void WindowBlitProc(unsigned char* src, int width, int height, int srcPitch, unsigned char* dest, int destPitch);

typedef struct Button Button;
typedef struct RadioGroup RadioGroup;

typedef struct Window {
    int id;
    int flags;
    Rect rect;
    int width;
    int height;
    int field_20;
    // rand
    int field_24;
    // rand
    int field_28;
    unsigned char* buffer;
    Button* buttonListHead;
    Button* field_34;
    Button* field_38;
    struc_177* field_3C;
    WindowBlitProc* blitProc;
} Window;

typedef void ButtonCallback(int btn, int keyCode);

typedef struct Button {
    int id;
    int flags;
    Rect rect;
    int mouseEnterEventCode;
    int mouseExitEventCode;
    int lefMouseDownEventCode;
    int leftMouseUpEventCode;
    int rightMouseDownEventCode;
    int rightMouseUpEventCode;
    unsigned char* mouseUpImage;
    unsigned char* mouseDownImage;
    unsigned char* mouseHoverImage;
    unsigned char* field_3C;
    unsigned char* field_40;
    unsigned char* field_44;
    unsigned char* currentImage;
    unsigned char* mask;
    ButtonCallback* mouseEnterProc;
    ButtonCallback* mouseExitProc;
    ButtonCallback* leftMouseDownProc;
    ButtonCallback* leftMouseUpProc;
    ButtonCallback* rightMouseDownProc;
    ButtonCallback* rightMouseUpProc;
    ButtonCallback* onPressed;
    ButtonCallback* onUnpressed;
    RadioGroup* radioGroup;
    Button* prev;
    Button* next;
} Button;

typedef struct RadioGroup {
    int field_0;
    int field_4;
    void (*field_8)(int);
    int buttonsLength;
    Button* buttons[RADIO_GROUP_BUTTON_LIST_CAPACITY];
} RadioGroup;

typedef int(VideoSystemInitProc)();
typedef void(VideoSystemExitProc)();

char byte_50FA30[];

extern bool dword_51E3D8;
extern HANDLE dword_51E3DC;
extern bool gWindowSystemInitialized;
extern int dword_51E3E4[6];
extern unsigned char* off_51E3FC;
extern bool dword_51E400;
extern int dword_51E404;

extern int gOrderedWindowIds[MAX_WINDOW_COUNT];
extern Window* gWindows[MAX_WINDOW_COUNT];
extern VideoSystemExitProc* gVideoSystemExitProc;
extern int gWindowsLength;
extern int dword_6ADF28;
extern bool dword_6ADF2C;
extern int dword_6ADF30;
extern VideoSystemInitProc* gVideoSystemInitProc;
extern int dword_6ADF38;
extern void* off_6ADF3C;
extern RadioGroup gRadioGroups[RADIO_GROUP_LIST_CAPACITY];

int windowManagerInit(VideoSystemInitProc* videoSystemInitProc, VideoSystemExitProc* videoSystemExitProc, int a3);
void windowManagerExit(void);
int windowCreate(int x, int y, int width, int height, int a4, int flags);
void windowDestroy(int win);
void windowFree(int win);
void sub_4D6558(bool a1);
void windowDrawBorder(int win);
void windowDrawText(int win, char* str, int a3, int x, int y, int a6);
void windowDrawLine(int win, int left, int top, int right, int bottom, int color);
void windowDrawRect(int win, int left, int top, int right, int bottom, int color);
void windowFill(int win, int x, int y, int width, int height, int a6);
void windowUnhide(int win);
void windowHide(int win);
void sub_4D6EA0(int win_index, int x, int y);
void windowRefresh(int win);
void windowRefreshRect(int win, const Rect* rect);
void sub_4D6FD8(Window* window, Rect* rect, unsigned char* a3);
void windowRefreshAll(Rect* rect);
void sub_4D75B0(Window* window, RectListNode** rect, unsigned char* a3);
void sub_4D765C(int win);
void sub_4D77F8(unsigned char* a1);
void sub_4D7814(Rect* rect, unsigned char* a2);
Window* windowGetWindow(int win);
unsigned char* windowGetBuffer(int win);
int windowGetAtPoint(int x, int y);
int windowGetWidth(int win);
int windowGetHeight(int win);
int windowGetRect(int win, Rect* rect);
int sub_4D797C();
Button* buttonGetButton(int btn, Window** out_win);
int sub_4D7A34(int a1);
void programWindowSetTitle(const char* title);
int paletteOpenFileImpl(const char* path, int flags);
int paletteReadFileImpl(int fd, void* buf, size_t count);
int paletteCloseFileImpl(int fd);
bool showMesageBox(const char* str);
int buttonCreate(int win, int x, int y, int width, int height, int mouseEnterEventCode, int mouseExitEventCode, int mouseDownEventCode, int mouseUpEventCode, unsigned char* up, unsigned char* dn, unsigned char* hover, int flags);
int sub_4D8674(int btn, unsigned char* up, unsigned char* down, unsigned char* hover);
int sub_4D86A8(int btn, unsigned char* up, unsigned char* down, unsigned char* hover, int a5);
int buttonSetMouseCallbacks(int btn, ButtonCallback* mouseEnterProc, ButtonCallback* mouseExitProc, ButtonCallback* mouseDownProc, ButtonCallback* mouseUpProc);
int buttonSetRightMouseCallbacks(int btn, int rightMouseDownEventCode, int rightMouseUpEventCode, ButtonCallback* rightMouseDownProc, ButtonCallback* rightMouseUpProc);
int buttonSetCallbacks(int btn, ButtonCallback* onPressed, ButtonCallback* onUnpressed);
int buttonSetMask(int btn, unsigned char* mask);
Button* buttonCreateInternal(int win, int x, int y, int width, int height, int mouseEnterEventCode, int mouseExitEventCode, int mouseDownEventCode, int mouseUpEventCode, int flags, unsigned char* up, unsigned char* dn, unsigned char* hover);
bool sub_4D89E4(int btn);
int sub_4D8A10(Window* window, int* out_a2);
bool sub_4D9214(Button* button, Rect* rect);
int buttonGetWindowId(int btn);
int sub_4D92B4();
int buttonDestroy(int btn);
void buttonFree(Button* ptr);
int buttonEnable(int btn);
int buttonDisable(int btn);
int sub_4D9554(int btn, bool a2, int a3);
int sub_4D962C(int a1, int* a2, int a3, void (*a4)(int));
int sub_4D96EC(int a1, int* a2);
int sub_4D9744(Button* button);
void sub_4D9808(Button* button, Window* window, unsigned char* data, int a4, Rect* a5, int a6);
void sub_4D9A58(Window* window, Rect* rect);
int sub_4D9AA0(int btn);

#endif /* WINDOW_MANAGER_H */