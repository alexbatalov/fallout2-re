#ifndef CORE_H
#define CORE_H

#include "db.h"
#include "dinput.h"
#include "geometry.h"
#include "window.h"

#include <stdbool.h>

#define MOUSE_DEFAULT_CURSOR_WIDTH 8
#define MOUSE_DEFAULT_CURSOR_HEIGHT 8
#define MOUSE_DEFAULT_CURSOR_SIZE (MOUSE_DEFAULT_CURSOR_WIDTH * MOUSE_DEFAULT_CURSOR_HEIGHT)

#define MOUSE_STATE_LEFT_BUTTON_DOWN 0x01
#define MOUSE_STATE_RIGHT_BUTTON_DOWN 0x02

#define MOUSE_EVENT_LEFT_BUTTON_DOWN 0x01
#define MOUSE_EVENT_RIGHT_BUTTON_DOWN 0x02
#define MOUSE_EVENT_LEFT_BUTTON_REPEAT 0x04
#define MOUSE_EVENT_RIGHT_BUTTON_REPEAT 0x08
#define MOUSE_EVENT_LEFT_BUTTON_UP 0x10
#define MOUSE_EVENT_RIGHT_BUTTON_UP 0x20
#define MOUSE_EVENT_ANY_BUTTON_DOWN (MOUSE_EVENT_LEFT_BUTTON_DOWN | MOUSE_EVENT_RIGHT_BUTTON_DOWN)
#define MOUSE_EVENT_ANY_BUTTON_REPEAT (MOUSE_EVENT_LEFT_BUTTON_REPEAT | MOUSE_EVENT_RIGHT_BUTTON_REPEAT)
#define MOUSE_EVENT_ANY_BUTTON_UP (MOUSE_EVENT_LEFT_BUTTON_UP | MOUSE_EVENT_RIGHT_BUTTON_UP)
#define MOUSE_EVENT_LEFT_BUTTON_DOWN_REPEAT (MOUSE_EVENT_LEFT_BUTTON_DOWN | MOUSE_EVENT_LEFT_BUTTON_REPEAT)
#define MOUSE_EVENT_RIGHT_BUTTON_DOWN_REPEAT (MOUSE_EVENT_RIGHT_BUTTON_DOWN | MOUSE_EVENT_RIGHT_BUTTON_REPEAT)

#define BUTTON_REPEAT_TIME 250

#define KEY_STATE_UP 0
#define KEY_STATE_DOWN 1
#define KEY_STATE_REPEAT 2

#define MODIFIER_KEY_STATE_NUM_LOCK 0x01
#define MODIFIER_KEY_STATE_CAPS_LOCK 0x02
#define MODIFIER_KEY_STATE_SCROLL_LOCK 0x04

#define KEYBOARD_EVENT_MODIFIER_CAPS_LOCK 0x0001
#define KEYBOARD_EVENT_MODIFIER_NUM_LOCK 0x0002
#define KEYBOARD_EVENT_MODIFIER_SCROLL_LOCK 0x0004
#define KEYBOARD_EVENT_MODIFIER_LEFT_SHIFT 0x0008
#define KEYBOARD_EVENT_MODIFIER_RIGHT_SHIFT 0x0010
#define KEYBOARD_EVENT_MODIFIER_LEFT_ALT 0x0020
#define KEYBOARD_EVENT_MODIFIER_RIGHT_ALT 0x0040
#define KEYBOARD_EVENT_MODIFIER_LEFT_CONTROL 0x0080
#define KEYBOARD_EVENT_MODIFIER_RIGHT_CONTROL 0x0100
#define KEYBOARD_EVENT_MODIFIER_ANY_SHIFT (KEYBOARD_EVENT_MODIFIER_LEFT_SHIFT | KEYBOARD_EVENT_MODIFIER_RIGHT_SHIFT)

#define KEY_QUEUE_SIZE 64

typedef enum Key {
    KEY_ESCAPE = '\x1b',
    KEY_TAB = '\x09',
    KEY_BACKSPACE = '\x08',
    KEY_RETURN = '\r',

    KEY_SPACE = ' ',
    KEY_EXCLAMATION = '!',
    KEY_QUOTE = '"',
    KEY_NUMBER_SIGN = '#',
    KEY_DOLLAR = '$',
    KEY_PERCENT = '%',
    KEY_AMPERSAND = '&',
    KEY_SINGLE_QUOTE = '\'',
    KEY_PAREN_LEFT = '(',
    KEY_PAREN_RIGHT = ')',
    KEY_ASTERISK = '*',
    KEY_PLUS = '+',
    KEY_COMMA = ',',
    KEY_MINUS = '-',
    KEY_DOT = '.',
    KEY_SLASH = '/',
    KEY_0 = '0',
    KEY_1 = '1',
    KEY_2 = '2',
    KEY_3 = '3',
    KEY_4 = '4',
    KEY_5 = '5',
    KEY_6 = '6',
    KEY_7 = '7',
    KEY_8 = '8',
    KEY_9 = '9',
    KEY_COLON = ':',
    KEY_SEMICOLON = ';',
    KEY_LESS = '<',
    KEY_EQUAL = '=',
    KEY_GREATER = '>',
    KEY_QUESTION = '?',
    KEY_AT = '@',
    KEY_UPPERCASE_A = 'A',
    KEY_UPPERCASE_B = 'B',
    KEY_UPPERCASE_C = 'C',
    KEY_UPPERCASE_D = 'D',
    KEY_UPPERCASE_E = 'E',
    KEY_UPPERCASE_F = 'F',
    KEY_UPPERCASE_G = 'G',
    KEY_UPPERCASE_H = 'H',
    KEY_UPPERCASE_I = 'I',
    KEY_UPPERCASE_J = 'J',
    KEY_UPPERCASE_K = 'K',
    KEY_UPPERCASE_L = 'L',
    KEY_UPPERCASE_M = 'M',
    KEY_UPPERCASE_N = 'N',
    KEY_UPPERCASE_O = 'O',
    KEY_UPPERCASE_P = 'P',
    KEY_UPPERCASE_Q = 'Q',
    KEY_UPPERCASE_R = 'R',
    KEY_UPPERCASE_S = 'S',
    KEY_UPPERCASE_T = 'T',
    KEY_UPPERCASE_U = 'U',
    KEY_UPPERCASE_V = 'V',
    KEY_UPPERCASE_W = 'W',
    KEY_UPPERCASE_X = 'X',
    KEY_UPPERCASE_Y = 'Y',
    KEY_UPPERCASE_Z = 'Z',

    KEY_BRACKET_LEFT = '[',
    KEY_BACKSLASH = '\\',
    KEY_BRACKET_RIGHT = ']',
    KEY_CARET = '^',
    KEY_UNDERSCORE = '_',

    KEY_GRAVE = '`',
    KEY_LOWERCASE_A = 'a',
    KEY_LOWERCASE_B = 'b',
    KEY_LOWERCASE_C = 'c',
    KEY_LOWERCASE_D = 'd',
    KEY_LOWERCASE_E = 'e',
    KEY_LOWERCASE_F = 'f',
    KEY_LOWERCASE_G = 'g',
    KEY_LOWERCASE_H = 'h',
    KEY_LOWERCASE_I = 'i',
    KEY_LOWERCASE_J = 'j',
    KEY_LOWERCASE_K = 'k',
    KEY_LOWERCASE_L = 'l',
    KEY_LOWERCASE_M = 'm',
    KEY_LOWERCASE_N = 'n',
    KEY_LOWERCASE_O = 'o',
    KEY_LOWERCASE_P = 'p',
    KEY_LOWERCASE_Q = 'q',
    KEY_LOWERCASE_R = 'r',
    KEY_LOWERCASE_S = 's',
    KEY_LOWERCASE_T = 't',
    KEY_LOWERCASE_U = 'u',
    KEY_LOWERCASE_V = 'v',
    KEY_LOWERCASE_W = 'w',
    KEY_LOWERCASE_X = 'x',
    KEY_LOWERCASE_Y = 'y',
    KEY_LOWERCASE_Z = 'z',
    KEY_BRACE_LEFT = '{',
    KEY_BAR = '|',
    KEY_BRACE_RIGHT = '}',
    KEY_TILDE = '~',
    KEY_DEL = 127,

    KEY_ALT_Q = 272,
    KEY_ALT_W = 273,
    KEY_ALT_E = 274,
    KEY_ALT_R = 275,
    KEY_ALT_T = 276,
    KEY_ALT_Y = 277,
    KEY_ALT_U = 278,
    KEY_ALT_I = 279,
    KEY_ALT_O = 280,
    KEY_ALT_P = 281,
    KEY_ALT_A = 286,
    KEY_ALT_S = 287,
    KEY_ALT_D = 288,
    KEY_ALT_F = 289,
    KEY_ALT_G = 290,
    KEY_ALT_H = 291,
    KEY_ALT_J = 292,
    KEY_ALT_K = 293,
    KEY_ALT_L = 294,
    KEY_ALT_Z = 300,
    KEY_ALT_X = 301,
    KEY_ALT_C = 302,
    KEY_ALT_V = 303,
    KEY_ALT_B = 304,
    KEY_ALT_N = 305,
    KEY_ALT_M = 306,

    KEY_CTRL_Q = 17,
    KEY_CTRL_W = 23,
    KEY_CTRL_E = 5,
    KEY_CTRL_R = 18,
    KEY_CTRL_T = 20,
    KEY_CTRL_Y = 25,
    KEY_CTRL_U = 21,
    KEY_CTRL_I = 9,
    KEY_CTRL_O = 15,
    KEY_CTRL_P = 16,
    KEY_CTRL_A = 1,
    KEY_CTRL_S = 19,
    KEY_CTRL_D = 4,
    KEY_CTRL_F = 6,
    KEY_CTRL_G = 7,
    KEY_CTRL_H = 8,
    KEY_CTRL_J = 10,
    KEY_CTRL_K = 11,
    KEY_CTRL_L = 12,
    KEY_CTRL_Z = 26,
    KEY_CTRL_X = 24,
    KEY_CTRL_C = 3,
    KEY_CTRL_V = 22,
    KEY_CTRL_B = 2,
    KEY_CTRL_N = 14,
    KEY_CTRL_M = 13,

    KEY_F1 = 315,
    KEY_F2 = 316,
    KEY_F3 = 317,
    KEY_F4 = 318,
    KEY_F5 = 319,
    KEY_F6 = 320,
    KEY_F7 = 321,
    KEY_F8 = 322,
    KEY_F9 = 323,
    KEY_F10 = 324,
    KEY_F11 = 389,
    KEY_F12 = 390,

    KEY_SHIFT_F1 = 340,
    KEY_SHIFT_F2 = 341,
    KEY_SHIFT_F3 = 342,
    KEY_SHIFT_F4 = 343,
    KEY_SHIFT_F5 = 344,
    KEY_SHIFT_F6 = 345,
    KEY_SHIFT_F7 = 346,
    KEY_SHIFT_F8 = 347,
    KEY_SHIFT_F9 = 348,
    KEY_SHIFT_F10 = 349,
    KEY_SHIFT_F11 = 391,
    KEY_SHIFT_F12 = 392,

    KEY_CTRL_F1 = 350,
    KEY_CTRL_F2 = 351,
    KEY_CTRL_F3 = 352,
    KEY_CTRL_F4 = 353,
    KEY_CTRL_F5 = 354,
    KEY_CTRL_F6 = 355,
    KEY_CTRL_F7 = 356,
    KEY_CTRL_F8 = 357,
    KEY_CTRL_F9 = 358,
    KEY_CTRL_F10 = 359,
    KEY_CTRL_F11 = 393,
    KEY_CTRL_F12 = 394,

    KEY_ALT_F1 = 360,
    KEY_ALT_F2 = 361,
    KEY_ALT_F3 = 362,
    KEY_ALT_F4 = 363,
    KEY_ALT_F5 = 364,
    KEY_ALT_F6 = 365,
    KEY_ALT_F7 = 366,
    KEY_ALT_F8 = 367,
    KEY_ALT_F9 = 368,
    KEY_ALT_F10 = 369,
    KEY_ALT_F11 = 395,
    KEY_ALT_F12 = 396,

    KEY_HOME = 327,
    KEY_CTRL_HOME = 375,
    KEY_ALT_HOME = 407,

    KEY_PAGE_UP = 329,
    KEY_CTRL_PAGE_UP = 388,
    KEY_ALT_PAGE_UP = 409,

    KEY_INSERT = 338,
    KEY_CTRL_INSERT = 402,
    KEY_ALT_INSERT = 418,

    KEY_DELETE = 339,
    KEY_CTRL_DELETE = 403,
    KEY_ALT_DELETE = 419,

    KEY_END = 335,
    KEY_CTRL_END = 373,
    KEY_ALT_END = 415,

    KEY_PAGE_DOWN = 337,
    KEY_ALT_PAGE_DOWN = 417,
    KEY_CTRL_PAGE_DOWN = 374,

    KEY_ARROW_UP = 328,
    KEY_CTRL_ARROW_UP = 397,
    KEY_ALT_ARROW_UP = 408,

    KEY_ARROW_DOWN = 336,
    KEY_CTRL_ARROW_DOWN = 401,
    KEY_ALT_ARROW_DOWN = 416,

    KEY_ARROW_LEFT = 331,
    KEY_CTRL_ARROW_LEFT = 371,
    KEY_ALT_ARROW_LEFT = 411,

    KEY_ARROW_RIGHT = 333,
    KEY_CTRL_ARROW_RIGHT = 372,
    KEY_ALT_ARROW_RIGHT = 413,

    KEY_CTRL_BACKSLASH = 192,

    KEY_NUMBERPAD_5 = 332,
    KEY_CTRL_NUMBERPAD_5 = 399,
    KEY_ALT_NUMBERPAD_5 = 9999,

    KEY_FIRST_INPUT_CHARACTER = KEY_SPACE,
    KEY_LAST_INPUT_CHARACTER = KEY_LOWERCASE_Z,
} Key;

typedef enum KeyboardLayout {
    KEYBOARD_LAYOUT_QWERTY,
    KEYBOARD_LAYOUT_FRENCH,
    KEYBOARD_LAYOUT_GERMAN,
    KEYBOARD_LAYOUT_ITALIAN,
    KEYBOARD_LAYOUT_SPANISH,
} KeyboardLayout;

typedef struct STRUCT_6ABF50 {
    // Time when appropriate key was pressed down or -1 if it's up.
    int tick;
    int repeatCount;
} STRUCT_6ABF50;

typedef struct InputEvent {
    // This is either logical key or input event id, which can be either
    // character code pressed or some other numbers used throughout the
    // game interface.
    int logicalKey;
    int mouseX;
    int mouseY;
} InputEvent;

typedef void TickerProc();

typedef struct TickerListNode {
    int flags;
    TickerProc* proc;
    struct TickerListNode* next;
} TickerListNode;

typedef struct STRUCT_51E2F0 {
    int type;
    int field_4;
    int field_8;
    union {
        struct {
            int type_1_field_C;
            int type_1_field_10;
            int type_1_field_14;
        };
        struct {
            short type_2_field_C;
        };
        struct {
            int dx;
            int dy;
            int buttons;
        };
    };
} STRUCT_51E2F0;

static_assert(sizeof(STRUCT_51E2F0) == 24, "wrong size");

typedef struct LogicalKeyEntry {
    short field_0;
    short unmodified;
    short shift;
    short lmenu;
    short rmenu;
    short ctrl;
} LogicalKeyEntry;

typedef struct KeyboardEvent {
    unsigned char scanCode;
    unsigned short modifiers;
} KeyboardEvent;

typedef int(PauseHandler)();
typedef int(ScreenshotHandler)(int width, int height, unsigned char* buffer, unsigned char* palette);

extern void (*off_51E234)();
extern void (*off_51E238)(int);
extern int gKeyboardKeyRepeatRate;
extern int gKeyboardKeyRepeatDelay;
extern bool dword_51E244;
extern unsigned char gMouseDefaultCursor[MOUSE_DEFAULT_CURSOR_SIZE];
extern int dword_51E290;
extern unsigned char* gMouseCursorData;
extern unsigned char* dword_51E298;
extern unsigned char* dword_51E29C;
extern double gMouseSensitivity;
extern unsigned int dword_51E2A8;
extern int gMouseButtonsState;

extern LPDIRECTDRAW gDirectDraw;
extern LPDIRECTDRAWSURFACE gDirectDrawSurface1;
extern LPDIRECTDRAWSURFACE gDirectDrawSurface2;
extern LPDIRECTDRAWPALETTE gDirectDrawPalette;
extern void (*off_51E2C4)();
extern bool gMmxEnabled;
extern bool gMmxProbed;

extern unsigned char byte_51E2D0;
extern bool gKeyboardDisabled;
extern int dword_51E2D8;
extern int dword_51E2DC;
extern int gKeyboardEventQueueWriteIndex;
extern int gKeyboardEventQueueReadIndex;
extern short word_51E2E8;
extern int gModifierKeysState;
extern int (*off_51E2EC)();
extern STRUCT_51E2F0* off_51E2F0;
extern int dword_51E2F4;
extern int dword_51E2F8;
extern int dword_51E2FC;
extern int dword_51E300;
extern int dword_51E304;
extern int dword_51E308;
extern int dword_51E30C;
extern int dword_51E310;
extern File* dword_51E314;
extern int dword_51E318;

extern unsigned char gNormalizedQwertyKeys[256];
extern InputEvent gInputEventQueue[40];
extern STRUCT_6ABF50 stru_6ABF50[256];
extern int dword_6AC750;
extern int dword_6AC754;
extern HHOOK dword_6AC758;
extern bool gPaused;
extern int gScreenshotKeyCode;
extern int dword_6AC764;
extern int gPauseKeyCode;
extern ScreenshotHandler* gScreenshotHandler;
extern int gInputEventQueueReadIndex;
extern unsigned char* gScreenshotBuffer;
extern PauseHandler* gPauseHandler;
extern int gInputEventQueueWriteIndex;
extern bool gRunLoopDisabled;
extern TickerListNode* gTickerListHead;
extern unsigned int gTickerLastTimestamp;
extern bool gCursorIsHidden;
extern int dword_6AC794;
extern int gMouseCursorHeight;
extern int dword_6AC79C;
extern int dword_6AC7A0;
extern int gMouseCursorY;
extern int gMouseCursorX;
extern int dword_6AC7AC;
extern int gMouseEvent;
extern unsigned int dword_6AC7B4;
extern int dword_6AC7B8;
extern bool gMouseInitialized;
extern int gMouseCursorPitch;
extern int gMouseCursorWidth;
extern int dword_6AC7C8;
extern int dword_6AC7CC;
extern int dword_6AC7D0;
extern unsigned int dword_6AC7D4;
extern WindowDrawingProc2* off_6AC7D8;
extern WINDOWDRAWINGPROC off_6AC7DC;
extern unsigned char byte_6AC7E0;
extern int gMouseRightButtonDownTimestamp;
extern int gMouseLeftButtonDownTimestamp;
extern int gMousePreviousEvent;
extern unsigned short gSixteenBppPalette[256];
extern Rect stru_6AC9F0;
extern int gRedMask;
extern int gGreenMask;
extern int gBlueMask;
extern int gBlueShift;
extern int gRedShift;
extern int gGreenShift;
extern void (*off_6ACA18)(unsigned char* src, int src_pitch, int a3, int src_x, int src_y, int src_width, int src_height, int dest_x, int dest_y);
extern void (*dword_6ACA1C)();
extern bool gMmxSupported;
extern unsigned char gLastVideoModePalette[268];
extern KeyboardEvent gKeyboardEventsQueue[KEY_QUEUE_SIZE];
extern LogicalKeyEntry gLogicalKeyEntries[256];
extern unsigned char gPressedPhysicalKeys[256];
extern unsigned int dword_6AD930;
extern KeyboardEvent gLastKeyboardEvent;
extern int gKeyboardLayout;
extern unsigned char gPressedPhysicalKeysCount;

int coreInit(int a1);
void coreExit();
int sub_4C8B78();
void sub_4C8BDC();
void enqueueInputEvent(int a1);
int dequeueInputEvent();
void inputEventQueueReset();
void tickersExecute();
void tickersAdd(TickerProc* fn);
void tickersRemove(TickerProc* fn);
void tickersEnable();
void tickersDisable();
void pause();
int pauseHandlerDefaultImpl();
void pauseHandlerConfigure(int keyCode, PauseHandler* fn);
void takeScreenshot();
void screenshotBlitter(unsigned char* src, int src_pitch, int a3, int x, int y, int width, int height, int dest_x, int dest_y);
int screenshotHandlerDefaultImpl(int width, int height, unsigned char* data, unsigned char* palette);
void screenshotHandlerConfigure(int keyCode, ScreenshotHandler* handler);
unsigned int sub_4C9370();
void coreDelayProcessingEvents(unsigned int ms);
void coreDelay(unsigned int ms);
unsigned int getTicksSince(unsigned int a1);
unsigned int getTicksBetween(unsigned int a1, unsigned int a2);
unsigned int sub_4C9410();
void buildNormalizedQwertyKeys();
void sub_4C9BB4(int a1);
int sub_4C9C20();
int sub_4C9C28(int a1);
LRESULT CALLBACK sub_4C9C4C(int nCode, WPARAM wParam, LPARAM lParam);
void sub_4C9CF0();
void sub_4C9DF0();
void sub_4C9E14(KeyboardData* data);
void sub_4C9EEC();
int mouseInit();
void mouseFree();
void mousePrepareDefaultCursor();
int mouseSetFrame(unsigned char* a1, int width, int height, int pitch, int a5, int a6, int a7);
void sub_4CA2D0();
void mouseShowCursor();
void mouseHideCursor();
void sub_4CA59C();
void sub_4CA698(int delta_x, int delta_y, int buttons);
bool sub_4CA8C8(int left, int top, int right, int bottom);
bool sub_4CA934(int left, int top, int right, int bottom);
void mouseGetRect(Rect* rect);
void mouseGetPosition(int* out_x, int* out_y);
void sub_4CAA04(int a1, int a2);
void sub_4CAA38();
int mouseGetEvent();
bool cursorIsHidden();
void sub_4CAB5C(int* out_x, int* out_y, int* out_buttons);
void mouseSetSensitivity(double value);
void mmxSetEnabled(bool a1);
int sub_4CAD08();
int sub_4CAD40();
int sub_4CAD5C();
int sub_4CAD64();
int sub_4CAD94();
int sub_4CADA8();
int sub_4CADBC();
int sub_4CADD0();
void sub_4CADF8();
void sub_4CADFC();
int sub_4CAE1C(int width, int height, int bpp);
int sub_4CAECC(int width, int height);
int sub_4CAEDC();
int getShiftForBitMask(int mask);
int directDrawInit(int width, int height, int bpp);
void directDrawFree();
void directDrawSetPaletteInRange(unsigned char* a1, int a2, int a3);
void directDrawSetPalette(unsigned char* palette);
unsigned char* directDrawGetPalette();
void sub_4CB850(unsigned char* src, int src_pitch, int a3, int src_x, int src_y, int src_width, int src_height, int dest_x, int dest_y);
void sub_4CB93C(unsigned char* src, int srcPitch, int a3, int srcX, int srcY, int srcWidth, int srcHeight, int destX, int destY);
void sub_4CBA44(unsigned char* src, int srcPitch, int a3, int srcX, int srcY, int srcWidth, int srcHeight, int destX, int destY);
void sub_4CBAB0(unsigned char* src, int srcPitch, int a3, int srcX, int srcY, int srcWidth, int srcHeight, int destX, int destY, unsigned char keyColor);
void sub_4CBBC8();
int keyboardInit();
void keyboardFree();
void keyboardReset();
int sub_4CBDE8();
void keyboardDisable();
void keyboardEnable();
int keyboardIsDisabled();
void keyboardSetLayout(int new_language);
int keyboardGetLayout();
void sub_4CBF68(int a1);
int sub_4CC2F0();
int keyboardDequeueLogicalKeyCode();
void keyboardBuildQwertyConfiguration();
void sub_4D24F8();
int keyboardPeekEvent(int index, KeyboardEvent** keyboardEventPtr);
int sub_4D28F4();
int sub_4D2918();
int sub_4D2930();
bool sub_4D2CD0();
int sub_4D2CF0();
bool sub_4D2E00(STRUCT_51E2F0* ptr, File* stream);
bool sub_4D2EE4(STRUCT_51E2F0* ptr, File* stream);

#endif /* CORE_H */
