#ifndef CORE_H
#define CORE_H

#include <stdbool.h>

#include "db.h"
#include "dinput.h"
#include "geometry.h"
#include "kb.h"
#include "window.h"

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

typedef int(PauseHandler)();
typedef int(ScreenshotHandler)(int width, int height, unsigned char* buffer, unsigned char* palette);

extern void (*_idle_func)();
extern void (*_focus_func)(int);
extern int gKeyboardKeyRepeatRate;
extern int gKeyboardKeyRepeatDelay;
extern bool _keyboard_hooked;
extern unsigned char gMouseDefaultCursor[MOUSE_DEFAULT_CURSOR_SIZE];
extern int _mouse_idling;
extern unsigned char* gMouseCursorData;
extern unsigned char* _mouse_shape;
extern unsigned char* _mouse_fptr;
extern double gMouseSensitivity;
extern unsigned int _ticker_;
extern int gMouseButtonsState;

extern LPDIRECTDRAW gDirectDraw;
extern LPDIRECTDRAWSURFACE gDirectDrawSurface1;
extern LPDIRECTDRAWSURFACE gDirectDrawSurface2;
extern LPDIRECTDRAWPALETTE gDirectDrawPalette;
extern void (*_update_palette_func)();
extern bool gMmxEnabled;
extern bool gMmxProbed;

extern unsigned char gNormalizedQwertyKeys[256];
extern InputEvent gInputEventQueue[40];
extern STRUCT_6ABF50 _GNW95_key_time_stamps[256];
extern int _input_mx;
extern int _input_my;
extern HHOOK _GNW95_keyboardHandle;
extern bool gPaused;
extern int gScreenshotKeyCode;
extern int _using_msec_timer;
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
extern int _raw_x;
extern int gMouseCursorHeight;
extern int _raw_y;
extern int _raw_buttons;
extern int gMouseCursorY;
extern int gMouseCursorX;
extern int _mouse_disabled;
extern int gMouseEvent;
extern unsigned int _mouse_speed;
extern int _mouse_curr_frame;
extern bool gMouseInitialized;
extern int gMouseCursorPitch;
extern int gMouseCursorWidth;
extern int _mouse_num_frames;
extern int _mouse_hoty;
extern int _mouse_hotx;
extern unsigned int _mouse_idle_start_time;
extern WindowDrawingProc2* _mouse_blit_trans;
extern WINDOWDRAWINGPROC _mouse_blit;
extern unsigned char _mouse_trans;
extern int gMouseRightButtonDownTimestamp;
extern int gMouseLeftButtonDownTimestamp;
extern int gMousePreviousEvent;
extern unsigned short gSixteenBppPalette[256];
extern Rect _scr_size;
extern int gRedMask;
extern int gGreenMask;
extern int gBlueMask;
extern int gBlueShift;
extern int gRedShift;
extern int gGreenShift;
extern void (*_scr_blit)(unsigned char* src, int src_pitch, int a3, int src_x, int src_y, int src_width, int src_height, int dest_x, int dest_y);
extern void (*_zero_mem)();
extern bool gMmxSupported;
extern unsigned char gLastVideoModePalette[268];

int coreInit(int a1);
void coreExit();
int _get_input();
void _process_bk();
void enqueueInputEvent(int a1);
int dequeueInputEvent();
void inputEventQueueReset();
void tickersExecute();
void tickersAdd(TickerProc* fn);
void tickersRemove(TickerProc* fn);
void tickersEnable();
void tickersDisable();
void pauseGame();
int pauseHandlerDefaultImpl();
void pauseHandlerConfigure(int keyCode, PauseHandler* fn);
void takeScreenshot();
void screenshotBlitter(unsigned char* src, int src_pitch, int a3, int x, int y, int width, int height, int dest_x, int dest_y);
int screenshotHandlerDefaultImpl(int width, int height, unsigned char* data, unsigned char* palette);
void screenshotHandlerConfigure(int keyCode, ScreenshotHandler* handler);
unsigned int _get_time();
void coreDelayProcessingEvents(unsigned int ms);
void coreDelay(unsigned int ms);
unsigned int getTicksSince(unsigned int a1);
unsigned int getTicksBetween(unsigned int a1, unsigned int a2);
unsigned int _get_bk_time();
void buildNormalizedQwertyKeys();
void _GNW95_hook_input(int a1);
int _GNW95_input_init();
int _GNW95_hook_keyboard(int a1);
LRESULT CALLBACK _GNW95_keyboard_hook(int nCode, WPARAM wParam, LPARAM lParam);
void _GNW95_process_message();
void _GNW95_clear_time_stamps();
void _GNW95_process_key(KeyboardData* data);
void _GNW95_lost_focus();
int mouseInit();
void mouseFree();
void mousePrepareDefaultCursor();
int mouseSetFrame(unsigned char* a1, int width, int height, int pitch, int a5, int a6, int a7);
void _mouse_anim();
void mouseShowCursor();
void mouseHideCursor();
void _mouse_info();
void _mouse_simulate_input(int delta_x, int delta_y, int buttons);
bool _mouse_in(int left, int top, int right, int bottom);
bool _mouse_click_in(int left, int top, int right, int bottom);
void mouseGetRect(Rect* rect);
void mouseGetPosition(int* out_x, int* out_y);
void _mouse_set_position(int a1, int a2);
void _mouse_clip();
int mouseGetEvent();
bool cursorIsHidden();
void _mouse_get_raw_state(int* out_x, int* out_y, int* out_buttons);
void mouseSetSensitivity(double value);
void mmxSetEnabled(bool a1);
int _init_mode_320_200();
int _init_mode_320_400();
int _init_mode_640_480_16();
int _init_mode_640_480();
int _init_mode_640_400();
int _init_mode_800_600();
int _init_mode_1024_768();
int _init_mode_1280_1024();
void _get_start_mode_();
void _zero_vid_mem();
int _GNW95_init_mode_ex(int width, int height, int bpp);
int _init_vesa_mode(int width, int height);
int _GNW95_init_window();
int getShiftForBitMask(int mask);
int directDrawInit(int width, int height, int bpp);
void directDrawFree();
void directDrawSetPaletteInRange(unsigned char* a1, int a2, int a3);
void directDrawSetPalette(unsigned char* palette);
unsigned char* directDrawGetPalette();
void _GNW95_ShowRect(unsigned char* src, int src_pitch, int a3, int src_x, int src_y, int src_width, int src_height, int dest_x, int dest_y);
void _GNW95_MouseShowRect16(unsigned char* src, int srcPitch, int a3, int srcX, int srcY, int srcWidth, int srcHeight, int destX, int destY);
void _GNW95_ShowRect16(unsigned char* src, int srcPitch, int a3, int srcX, int srcY, int srcWidth, int srcHeight, int destX, int destY);
void _GNW95_MouseShowTransRect16(unsigned char* src, int srcPitch, int a3, int srcX, int srcY, int srcWidth, int srcHeight, int destX, int destY, unsigned char keyColor);
void _GNW95_zero_vid_mem();

#endif /* CORE_H */
