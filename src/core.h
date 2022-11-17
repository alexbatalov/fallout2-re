#ifndef CORE_H
#define CORE_H

#include <stdbool.h>

#include "db.h"
#include "plib/gnw/dxinput.h"
#include "plib/gnw/rect.h"
#include "plib/gnw/kb.h"
#include "plib/gnw/mouse.h"
#include "window.h"

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
void _GNW95_process_key(dxinput_key_data* data);
void _GNW95_lost_focus();

#endif /* CORE_H */
