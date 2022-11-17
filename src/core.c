#include "core.h"

#include "plib/color/color.h"
#include "plib/gnw/button.h"
#include "plib/gnw/dxinput.h"
#include "plib/gnw/grbuf.h"
#include "plib/gnw/memory.h"
#include "mmx.h"
#include "plib/gnw/text.h"
#include "plib/gnw/vcr.h"
#include "plib/gnw/gnw.h"
#include "plib/gnw/intrface.h"
#include "plib/gnw/svga.h"

// NOT USED.
void (*_idle_func)() = NULL;

// NOT USED.
void (*_focus_func)(int) = NULL;

// 0x51E23C
int gKeyboardKeyRepeatRate = 80;

// 0x51E240
int gKeyboardKeyRepeatDelay = 500;

// 0x51E244
bool _keyboard_hooked = false;

// A map of DIK_* constants normalized for QWERTY keyboard.
//
// 0x6ABC70
unsigned char gNormalizedQwertyKeys[256];

// Ring buffer of input events.
//
// Looks like this buffer does not support overwriting of values. Once the
// buffer is full it will not overwrite values until they are dequeued.
//
// 0x6ABD70
InputEvent gInputEventQueue[40];

// 0x6ABF50
STRUCT_6ABF50 _GNW95_key_time_stamps[256];

// 0x6AC750
int _input_mx;

// 0x6AC754
int _input_my;

// 0x6AC758
HHOOK _GNW95_keyboardHandle;

// 0x6AC75C
bool gPaused;

// 0x6AC760
int gScreenshotKeyCode;

// 0x6AC764
int _using_msec_timer;

// 0x6AC768
int gPauseKeyCode;

// 0x6AC76C
ScreenshotHandler* gScreenshotHandler;

// 0x6AC770
int gInputEventQueueReadIndex;

// 0x6AC774
unsigned char* gScreenshotBuffer;

// 0x6AC778
PauseHandler* gPauseHandler;

// 0x6AC77C
int gInputEventQueueWriteIndex;

// 0x6AC780
bool gRunLoopDisabled;

// 0x6AC784
TickerListNode* gTickerListHead;

// 0x6AC788
unsigned int gTickerLastTimestamp;

// 0x4C8A70
int coreInit(int a1)
{
    if (!dxinput_init()) {
        return -1;
    }

    if (GNW_kb_set() == -1) {
        return -1;
    }

    if (GNW_mouse_init() == -1) {
        return -1;
    }

    if (_GNW95_input_init() == -1) {
        return -1;
    }

    _GNW95_hook_input(1);
    buildNormalizedQwertyKeys();
    _GNW95_clear_time_stamps();

    _using_msec_timer = a1;
    gInputEventQueueWriteIndex = 0;
    gInputEventQueueReadIndex = -1;
    _input_mx = -1;
    _input_my = -1;
    gRunLoopDisabled = 0;
    gPaused = false;
    gPauseKeyCode = KEY_ALT_P;
    gPauseHandler = pauseHandlerDefaultImpl;
    gScreenshotHandler = screenshotHandlerDefaultImpl;
    gTickerListHead = NULL;
    gScreenshotKeyCode = KEY_ALT_C;

    return 0;
}

// 0x4C8B40
void coreExit()
{
    _GNW95_hook_keyboard(0);
    _GNW95_input_init();
    GNW_mouse_exit();
    GNW_kb_restore();
    dxinput_exit();

    TickerListNode* curr = gTickerListHead;
    while (curr != NULL) {
        TickerListNode* next = curr->next;
        mem_free(curr);
        curr = next;
    }
}

// 0x4C8B78
int _get_input()
{
    int v3;

    _GNW95_process_message();

    if (!GNW95_isActive) {
        _GNW95_lost_focus();
    }

    _process_bk();

    v3 = dequeueInputEvent();
    if (v3 == -1 && mouse_get_buttons() & 0x33) {
        mouse_get_position(&_input_mx, &_input_my);
        return -2;
    } else {
        return GNW_check_menu_bars(v3);
    }

    return -1;
}

// 0x4C8BDC
void _process_bk()
{
    int v1;

    tickersExecute();

    if (vcr_update() != 3) {
        mouse_info();
    }

    v1 = win_check_all_buttons();
    if (v1 != -1) {
        enqueueInputEvent(v1);
        return;
    }

    v1 = kb_getch();
    if (v1 != -1) {
        enqueueInputEvent(v1);
        return;
    }
}

// 0x4C8C04
void enqueueInputEvent(int a1)
{
    if (a1 == -1) {
        return;
    }

    if (a1 == gPauseKeyCode) {
        pauseGame();
        return;
    }

    if (a1 == gScreenshotKeyCode) {
        takeScreenshot();
        return;
    }

    if (gInputEventQueueWriteIndex == gInputEventQueueReadIndex) {
        return;
    }

    InputEvent* inputEvent = &(gInputEventQueue[gInputEventQueueWriteIndex]);
    inputEvent->logicalKey = a1;

    mouse_get_position(&(inputEvent->mouseX), &(inputEvent->mouseY));

    gInputEventQueueWriteIndex++;

    if (gInputEventQueueWriteIndex == 40) {
        gInputEventQueueWriteIndex = 0;
        return;
    }

    if (gInputEventQueueReadIndex == -1) {
        gInputEventQueueReadIndex = 0;
    }
}

// 0x4C8C9C
int dequeueInputEvent()
{
    if (gInputEventQueueReadIndex == -1) {
        return -1;
    }

    InputEvent* inputEvent = &(gInputEventQueue[gInputEventQueueReadIndex]);
    int eventCode = inputEvent->logicalKey;
    _input_mx = inputEvent->mouseX;
    _input_my = inputEvent->mouseY;

    gInputEventQueueReadIndex++;

    if (gInputEventQueueReadIndex == 40) {
        gInputEventQueueReadIndex = 0;
    }

    if (gInputEventQueueReadIndex == gInputEventQueueWriteIndex) {
        gInputEventQueueReadIndex = -1;
        gInputEventQueueWriteIndex = 0;
    }

    return eventCode;
}

// 0x4C8D04
void inputEventQueueReset()
{
    gInputEventQueueReadIndex = -1;
    gInputEventQueueWriteIndex = 0;
}

// 0x4C8D1C
void tickersExecute()
{
    if (gPaused) {
        return;
    }

    if (gRunLoopDisabled) {
        return;
    }

#pragma warning(suppress : 28159)
    gTickerLastTimestamp = GetTickCount();

    TickerListNode* curr = gTickerListHead;
    TickerListNode** currPtr = &(gTickerListHead);

    while (curr != NULL) {
        TickerListNode* next = curr->next;
        if (curr->flags & 1) {
            *currPtr = next;

            mem_free(curr);
        } else {
            curr->proc();
            currPtr = &(curr->next);
        }
        curr = next;
    }
}

// 0x4C8D74
void tickersAdd(TickerProc* proc)
{
    TickerListNode* curr = gTickerListHead;
    while (curr != NULL) {
        if (curr->proc == proc) {
            if ((curr->flags & 0x01) != 0) {
                curr->flags &= ~0x01;
                return;
            }
        }
        curr = curr->next;
    }

    curr = (TickerListNode*)mem_malloc(sizeof(*curr));
    curr->flags = 0;
    curr->proc = proc;
    curr->next = gTickerListHead;
    gTickerListHead = curr;
}

// 0x4C8DC4
void tickersRemove(TickerProc* proc)
{
    TickerListNode* curr = gTickerListHead;
    while (curr != NULL) {
        if (curr->proc == proc) {
            curr->flags |= 0x01;
            return;
        }
        curr = curr->next;
    }
}

// 0x4C8DE4
void tickersEnable()
{
    gRunLoopDisabled = false;
}

// 0x4C8DF0
void tickersDisable()
{
    gRunLoopDisabled = true;
}

// 0x4C8DFC
void pauseGame()
{
    if (!gPaused) {
        gPaused = true;

        int win = gPauseHandler();

        while (_get_input() != KEY_ESCAPE) {
        }

        gPaused = false;
        win_delete(win);
    }
}

// 0x4C8E38
int pauseHandlerDefaultImpl()
{
    int windowWidth = text_width("Paused") + 32;
    int windowHeight = 3 * text_height() + 16;

    int win = win_add((rectGetWidth(&scr_size) - windowWidth) / 2,
        (rectGetHeight(&scr_size) - windowHeight) / 2,
        windowWidth,
        windowHeight,
        256,
        WINDOW_FLAG_0x10 | WINDOW_FLAG_0x04);
    if (win == -1) {
        return -1;
    }

    win_border(win);

    unsigned char* windowBuffer = win_get_buf(win);
    text_to_buf(windowBuffer + 8 * windowWidth + 16,
        "Paused",
        windowWidth,
        windowWidth,
        colorTable[31744]);

    win_register_text_button(win,
        (windowWidth - text_width("Done") - 16) / 2,
        windowHeight - 8 - text_height() - 6,
        -1,
        -1,
        -1,
        KEY_ESCAPE,
        "Done",
        0);

    win_draw(win);

    return win;
}

// 0x4C8F34
void pauseHandlerConfigure(int keyCode, PauseHandler* handler)
{
    gPauseKeyCode = keyCode;

    if (handler == NULL) {
        handler = pauseHandlerDefaultImpl;
    }

    gPauseHandler = handler;
}

// 0x4C8F4C
void takeScreenshot()
{
    int width = scr_size.lrx - scr_size.ulx + 1;
    int height = scr_size.lry - scr_size.uly + 1;
    gScreenshotBuffer = (unsigned char*)mem_malloc(width * height);
    if (gScreenshotBuffer == NULL) {
        return;
    }

    ScreenBlitFunc* v0 = scr_blit;
    scr_blit = screenshotBlitter;

    ScreenBlitFunc* v2 = mouse_blit;
    mouse_blit = screenshotBlitter;

    ScreenTransBlitFunc* v1 = mouse_blit_trans;
    mouse_blit_trans = NULL;

    win_refresh_all(&scr_size);

    mouse_blit_trans = v1;
    mouse_blit = v2;
    scr_blit = v0;

    unsigned char* palette = getSystemPalette();
    gScreenshotHandler(width, height, gScreenshotBuffer, palette);
    mem_free(gScreenshotBuffer);
}

// 0x4C8FF0
void screenshotBlitter(unsigned char* src, int srcPitch, int a3, int srcX, int srcY, int width, int height, int destX, int destY)
{
    int destWidth = scr_size.lrx - scr_size.ulx + 1;
    buf_to_buf(src + srcPitch * srcY + srcX, width, height, srcPitch, gScreenshotBuffer + destWidth * destY + destX, destWidth);
}

// 0x4C9048
int screenshotHandlerDefaultImpl(int width, int height, unsigned char* data, unsigned char* palette)
{
    char fileName[16];
    FILE* stream;
    int index;
    unsigned int intValue;
    unsigned short shortValue;

    for (index = 0; index < 100000; index++) {
        sprintf(fileName, "scr%.5d.bmp", index);

        stream = fopen(fileName, "rb");
        if (stream == NULL) {
            break;
        }

        fclose(stream);
    }

    if (index == 100000) {
        return -1;
    }

    stream = fopen(fileName, "wb");
    if (stream == NULL) {
        return -1;
    }

    // bfType
    shortValue = 0x4D42;
    fwrite(&shortValue, sizeof(shortValue), 1, stream);

    // bfSize
    // 14 - sizeof(BITMAPFILEHEADER)
    // 40 - sizeof(BITMAPINFOHEADER)
    // 1024 - sizeof(RGBQUAD) * 256
    intValue = width * height + 14 + 40 + 1024;
    fwrite(&intValue, sizeof(intValue), 1, stream);

    // bfReserved1
    shortValue = 0;
    fwrite(&shortValue, sizeof(shortValue), 1, stream);

    // bfReserved2
    shortValue = 0;
    fwrite(&shortValue, sizeof(shortValue), 1, stream);

    // bfOffBits
    intValue = 14 + 40 + 1024;
    fwrite(&intValue, sizeof(intValue), 1, stream);

    // biSize
    intValue = 40;
    fwrite(&intValue, sizeof(intValue), 1, stream);

    // biWidth
    intValue = width;
    fwrite(&intValue, sizeof(intValue), 1, stream);

    // biHeight
    intValue = height;
    fwrite(&intValue, sizeof(intValue), 1, stream);

    // biPlanes
    shortValue = 1;
    fwrite(&shortValue, sizeof(shortValue), 1, stream);

    // biBitCount
    shortValue = 8;
    fwrite(&shortValue, sizeof(shortValue), 1, stream);

    // biCompression
    intValue = 0;
    fwrite(&intValue, sizeof(intValue), 1, stream);

    // biSizeImage
    intValue = 0;
    fwrite(&intValue, sizeof(intValue), 1, stream);

    // biXPelsPerMeter
    intValue = 0;
    fwrite(&intValue, sizeof(intValue), 1, stream);

    // biYPelsPerMeter
    intValue = 0;
    fwrite(&intValue, sizeof(intValue), 1, stream);

    // biClrUsed
    intValue = 0;
    fwrite(&intValue, sizeof(intValue), 1, stream);

    // biClrImportant
    intValue = 0;
    fwrite(&intValue, sizeof(intValue), 1, stream);

    for (int index = 0; index < 256; index++) {
        unsigned char rgbReserved = 0;
        unsigned char rgbRed = palette[index * 3] << 2;
        unsigned char rgbGreen = palette[index * 3 + 1] << 2;
        unsigned char rgbBlue = palette[index * 3 + 2] << 2;

        fwrite(&rgbBlue, sizeof(rgbBlue), 1, stream);
        fwrite(&rgbGreen, sizeof(rgbGreen), 1, stream);
        fwrite(&rgbRed, sizeof(rgbRed), 1, stream);
        fwrite(&rgbReserved, sizeof(rgbReserved), 1, stream);
    }

    for (int y = height - 1; y >= 0; y--) {
        unsigned char* dataPtr = data + y * width;
        fwrite(dataPtr, 1, width, stream);
    }

    fflush(stream);
    fclose(stream);

    return 0;
}

// 0x4C9358
void screenshotHandlerConfigure(int keyCode, ScreenshotHandler* handler)
{
    gScreenshotKeyCode = keyCode;

    if (handler == NULL) {
        handler = screenshotHandlerDefaultImpl;
    }

    gScreenshotHandler = handler;
}

// 0x4C9370
unsigned int _get_time()
{
#pragma warning(suppress : 28159)
    return GetTickCount();
}

// 0x4C937C
void coreDelayProcessingEvents(unsigned int delay)
{
    // NOTE: Uninline.
    unsigned int start = _get_time();
    unsigned int end = _get_time();

    // NOTE: Uninline.
    unsigned int diff = getTicksBetween(end, start);
    while (diff < delay) {
        _process_bk();

        end = _get_time();

        // NOTE: Uninline.
        diff = getTicksBetween(end, start);
    }
}

// 0x4C93B8
void coreDelay(unsigned int ms)
{
#pragma warning(suppress : 28159)
    unsigned int start = GetTickCount();
    unsigned int diff;
    do {
        // NOTE: Uninline
        diff = getTicksSince(start);
    } while (diff < ms);
}

// 0x4C93E0
unsigned int getTicksSince(unsigned int start)
{
#pragma warning(suppress : 28159)
    unsigned int end = GetTickCount();

    // NOTE: Uninline.
    return getTicksBetween(end, start);
}

// 0x4C9400
unsigned int getTicksBetween(unsigned int end, unsigned int start)
{
    if (start > end) {
        return INT_MAX;
    } else {
        return end - start;
    }
}

// 0x4C9410
unsigned int _get_bk_time()
{
    return gTickerLastTimestamp;
}

// 0x4C9490
void buildNormalizedQwertyKeys()
{
    unsigned char* keys = gNormalizedQwertyKeys;
    int k;

    keys[DIK_ESCAPE] = DIK_ESCAPE;
    keys[DIK_1] = DIK_1;
    keys[DIK_2] = DIK_2;
    keys[DIK_3] = DIK_3;
    keys[DIK_4] = DIK_4;
    keys[DIK_5] = DIK_5;
    keys[DIK_6] = DIK_6;
    keys[DIK_7] = DIK_7;
    keys[DIK_8] = DIK_8;
    keys[DIK_9] = DIK_9;
    keys[DIK_0] = DIK_0;

    switch (kb_layout) {
    case 0:
        k = DIK_MINUS;
        break;
    case 1:
        k = DIK_6;
        break;
    default:
        k = DIK_SLASH;
        break;
    }
    keys[DIK_MINUS] = k;

    switch (kb_layout) {
    case 1:
        k = DIK_0;
        break;
    default:
        k = DIK_EQUALS;
        break;
    }
    keys[DIK_EQUALS] = k;

    keys[DIK_BACK] = DIK_BACK;
    keys[DIK_TAB] = DIK_TAB;

    switch (kb_layout) {
    case 1:
        k = DIK_A;
        break;
    default:
        k = DIK_Q;
        break;
    }
    keys[DIK_Q] = k;

    switch (kb_layout) {
    case 1:
        k = DIK_Z;
        break;
    default:
        k = DIK_W;
        break;
    }
    keys[DIK_W] = k;

    keys[DIK_E] = DIK_E;
    keys[DIK_R] = DIK_R;
    keys[DIK_T] = DIK_T;

    switch (kb_layout) {
    case 0:
    case 1:
    case 3:
    case 4:
        k = DIK_Y;
        break;
    default:
        k = DIK_Z;
        break;
    }
    keys[DIK_Y] = k;

    keys[DIK_U] = DIK_U;
    keys[DIK_I] = DIK_I;
    keys[DIK_O] = DIK_O;
    keys[DIK_P] = DIK_P;

    switch (kb_layout) {
    case 0:
    case 3:
    case 4:
        k = DIK_LBRACKET;
        break;
    case 1:
        k = DIK_5;
        break;
    default:
        k = DIK_8;
        break;
    }
    keys[DIK_LBRACKET] = k;

    switch (kb_layout) {
    case 0:
    case 3:
    case 4:
        k = DIK_RBRACKET;
        break;
    case 1:
        k = DIK_MINUS;
        break;
    default:
        k = DIK_9;
        break;
    }
    keys[DIK_RBRACKET] = k;

    keys[DIK_RETURN] = DIK_RETURN;
    keys[DIK_LCONTROL] = DIK_LCONTROL;

    switch (kb_layout) {
    case 1:
        k = DIK_Q;
        break;
    default:
        k = DIK_A;
        break;
    }
    keys[DIK_A] = k;

    keys[DIK_S] = DIK_S;
    keys[DIK_D] = DIK_D;
    keys[DIK_F] = DIK_F;
    keys[DIK_G] = DIK_G;
    keys[DIK_H] = DIK_H;
    keys[DIK_J] = DIK_J;
    keys[DIK_K] = DIK_K;
    keys[DIK_L] = DIK_L;

    switch (kb_layout) {
    case 0:
        k = DIK_SEMICOLON;
        break;
    default:
        k = DIK_COMMA;
        break;
    }
    keys[DIK_SEMICOLON] = k;

    switch (kb_layout) {
    case 0:
        k = DIK_APOSTROPHE;
        break;
    case 1:
        k = DIK_4;
        break;
    default:
        k = DIK_MINUS;
        break;
    }
    keys[DIK_APOSTROPHE] = k;

    switch (kb_layout) {
    case 0:
        k = DIK_GRAVE;
        break;
    case 1:
        k = DIK_2;
        break;
    case 3:
    case 4:
        k = 0;
        break;
    default:
        k = DIK_RBRACKET;
        break;
    }
    keys[DIK_GRAVE] = k;

    keys[DIK_LSHIFT] = DIK_LSHIFT;

    switch (kb_layout) {
    case 0:
        k = DIK_BACKSLASH;
        break;
    case 1:
        k = DIK_8;
        break;
    case 3:
    case 4:
        k = DIK_GRAVE;
        break;
    default:
        k = DIK_Y;
        break;
    }
    keys[DIK_BACKSLASH] = k;

    switch (kb_layout) {
    case 0:
    case 3:
    case 4:
        k = DIK_Z;
        break;
    case 1:
        k = DIK_W;
        break;
    default:
        k = DIK_Y;
        break;
    }
    keys[DIK_Z] = k;

    keys[DIK_X] = DIK_X;
    keys[DIK_C] = DIK_C;
    keys[DIK_V] = DIK_V;
    keys[DIK_B] = DIK_B;
    keys[DIK_N] = DIK_N;

    switch (kb_layout) {
    case 1:
        k = DIK_SEMICOLON;
        break;
    default:
        k = DIK_M;
        break;
    }
    keys[DIK_M] = k;

    switch (kb_layout) {
    case 1:
        k = DIK_M;
        break;
    default:
        k = DIK_COMMA;
        break;
    }
    keys[DIK_COMMA] = k;

    switch (kb_layout) {
    case 1:
        k = DIK_COMMA;
        break;
    default:
        k = DIK_PERIOD;
        break;
    }
    keys[DIK_PERIOD] = k;

    switch (kb_layout) {
    case 0:
        k = DIK_SLASH;
        break;
    case 1:
        k = DIK_PERIOD;
        break;
    default:
        k = DIK_7;
        break;
    }
    keys[DIK_SLASH] = k;

    keys[DIK_RSHIFT] = DIK_RSHIFT;
    keys[DIK_MULTIPLY] = DIK_MULTIPLY;
    keys[DIK_SPACE] = DIK_SPACE;
    keys[DIK_LMENU] = DIK_LMENU;
    keys[DIK_CAPITAL] = DIK_CAPITAL;
    keys[DIK_F1] = DIK_F1;
    keys[DIK_F2] = DIK_F2;
    keys[DIK_F3] = DIK_F3;
    keys[DIK_F4] = DIK_F4;
    keys[DIK_F5] = DIK_F5;
    keys[DIK_F6] = DIK_F6;
    keys[DIK_F7] = DIK_F7;
    keys[DIK_F8] = DIK_F8;
    keys[DIK_F9] = DIK_F9;
    keys[DIK_F10] = DIK_F10;
    keys[DIK_NUMLOCK] = DIK_NUMLOCK;
    keys[DIK_SCROLL] = DIK_SCROLL;
    keys[DIK_NUMPAD7] = DIK_NUMPAD7;
    keys[DIK_NUMPAD9] = DIK_NUMPAD9;
    keys[DIK_NUMPAD8] = DIK_NUMPAD8;
    keys[DIK_SUBTRACT] = DIK_SUBTRACT;
    keys[DIK_NUMPAD4] = DIK_NUMPAD4;
    keys[DIK_NUMPAD5] = DIK_NUMPAD5;
    keys[DIK_NUMPAD6] = DIK_NUMPAD6;
    keys[DIK_ADD] = DIK_ADD;
    keys[DIK_NUMPAD1] = DIK_NUMPAD1;
    keys[DIK_NUMPAD2] = DIK_NUMPAD2;
    keys[DIK_NUMPAD3] = DIK_NUMPAD3;
    keys[DIK_NUMPAD0] = DIK_NUMPAD0;
    keys[DIK_DECIMAL] = DIK_DECIMAL;
    keys[DIK_F11] = DIK_F11;
    keys[DIK_F12] = DIK_F12;
    keys[DIK_F13] = -1;
    keys[DIK_F14] = -1;
    keys[DIK_F15] = -1;
    keys[DIK_KANA] = -1;
    keys[DIK_CONVERT] = -1;
    keys[DIK_NOCONVERT] = -1;
    keys[DIK_YEN] = -1;
    keys[DIK_NUMPADEQUALS] = -1;
    keys[DIK_PREVTRACK] = -1;
    keys[DIK_AT] = -1;
    keys[DIK_COLON] = -1;
    keys[DIK_UNDERLINE] = -1;
    keys[DIK_KANJI] = -1;
    keys[DIK_STOP] = -1;
    keys[DIK_AX] = -1;
    keys[DIK_UNLABELED] = -1;
    keys[DIK_NUMPADENTER] = DIK_NUMPADENTER;
    keys[DIK_RCONTROL] = DIK_RCONTROL;
    keys[DIK_NUMPADCOMMA] = -1;
    keys[DIK_DIVIDE] = DIK_DIVIDE;
    keys[DIK_SYSRQ] = 84;
    keys[DIK_RMENU] = DIK_RMENU;
    keys[DIK_HOME] = DIK_HOME;
    keys[DIK_UP] = DIK_UP;
    keys[DIK_PRIOR] = DIK_PRIOR;
    keys[DIK_LEFT] = DIK_LEFT;
    keys[DIK_RIGHT] = DIK_RIGHT;
    keys[DIK_END] = DIK_END;
    keys[DIK_DOWN] = DIK_DOWN;
    keys[DIK_NEXT] = DIK_NEXT;
    keys[DIK_INSERT] = DIK_INSERT;
    keys[DIK_DELETE] = DIK_DELETE;
    keys[DIK_LWIN] = -1;
    keys[DIK_RWIN] = -1;
    keys[DIK_APPS] = -1;
}

// 0x4C9BB4
void _GNW95_hook_input(int a1)
{
    _GNW95_hook_keyboard(a1);

    if (a1) {
        dxinput_acquire_mouse();
    } else {
        dxinput_unacquire_mouse();
    }
}

// 0x4C9C20
int _GNW95_input_init()
{
    return 0;
}

// 0x4C9C28
int _GNW95_hook_keyboard(int a1)
{
    if (a1 == _keyboard_hooked) {
        return 0;
    }

    if (!a1) {
        dxinput_unacquire_keyboard();

        UnhookWindowsHookEx(_GNW95_keyboardHandle);

        kb_clear();

        _keyboard_hooked = a1;

        return 0;
    }

    if (dxinput_acquire_keyboard()) {
        _GNW95_keyboardHandle = SetWindowsHookExA(WH_KEYBOARD, _GNW95_keyboard_hook, 0, GetCurrentThreadId());
        kb_clear();
        _keyboard_hooked = a1;

        return 0;
    }

    return -1;
}

// 0x4C9C4C
LRESULT CALLBACK _GNW95_keyboard_hook(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0) {
        if (wParam == VK_DELETE && lParam & 0x20000000 && GetAsyncKeyState(VK_CONTROL) & 0x80000000)
            return 0;

        if (wParam == VK_ESCAPE && GetAsyncKeyState(VK_CONTROL) & 0x80000000)
            return 0;

        if (wParam == VK_RETURN && lParam & 0x20000000)
            return 0;

        if (wParam == VK_NUMLOCK || wParam == VK_CAPITAL || wParam == VK_SCROLL) {
            // TODO: Get rid of this goto.
            goto next;
        }

        return 1;
    }

next:

    return CallNextHookEx(_GNW95_keyboardHandle, nCode, wParam, lParam);
}

// 0x4C9CF0
void _GNW95_process_message()
{
    if (GNW95_isActive && !kb_is_disabled()) {
        dxinput_key_data data;
        while (dxinput_read_keyboard_buffer(&data)) {
            _GNW95_process_key(&data);
        }

        // NOTE: Uninline
        int tick = _get_time();

        for (int key = 0; key < 256; key++) {
            STRUCT_6ABF50* ptr = &(_GNW95_key_time_stamps[key]);
            if (ptr->tick != -1) {
                int elapsedTime = ptr->tick > tick ? INT_MAX : tick - ptr->tick;
                int delay = ptr->repeatCount == 0 ? gKeyboardKeyRepeatDelay : gKeyboardKeyRepeatRate;
                if (elapsedTime > delay) {
                    data.code = key;
                    data.state = 1;
                    _GNW95_process_key(&data);

                    ptr->tick = tick;
                    ptr->repeatCount++;
                }
            }
        }
    }

    MSG msg;
    while (PeekMessageA(&msg, NULL, 0, 0, 0)) {
        if (GetMessageA(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    }
}

// 0x4C9DF0
void _GNW95_clear_time_stamps()
{
    for (int index = 0; index < 256; index++) {
        _GNW95_key_time_stamps[index].tick = -1;
        _GNW95_key_time_stamps[index].repeatCount = 0;
    }
}

// 0x4C9E14
void _GNW95_process_key(dxinput_key_data* data)
{
    short key = data->code & 0xFF;

    switch (key) {
    case DIK_NUMPADENTER:
    case DIK_RCONTROL:
    case DIK_DIVIDE:
    case DIK_RMENU:
    case DIK_HOME:
    case DIK_UP:
    case DIK_PRIOR:
    case DIK_LEFT:
    case DIK_RIGHT:
    case DIK_END:
    case DIK_DOWN:
    case DIK_NEXT:
    case DIK_INSERT:
    case DIK_DELETE:
        key |= 0x0100;
        break;
    }

    int qwertyKey = gNormalizedQwertyKeys[data->code & 0xFF];

    if (vcr_state == VCR_STATE_PLAYING) {
        if ((vcr_terminate_flags & VCR_TERMINATE_ON_KEY_PRESS) != 0) {
            vcr_terminated_condition = VCR_PLAYBACK_COMPLETION_REASON_TERMINATED;
            vcr_stop();
        }
    } else {
        if ((key & 0x0100) != 0) {
            kb_simulate_key(224);
            qwertyKey -= 0x80;
        }

        STRUCT_6ABF50* ptr = &(_GNW95_key_time_stamps[data->code & 0xFF]);
        if (data->state == 1) {
            ptr->tick = _get_time();
            ptr->repeatCount = 0;
        } else {
            qwertyKey |= 0x80;
            ptr->tick = -1;
        }

        kb_simulate_key(qwertyKey);
    }
}

// 0x4C9EEC
void _GNW95_lost_focus()
{
    if (_focus_func != NULL) {
        _focus_func(0);
    }

    while (!GNW95_isActive) {
        _GNW95_process_message();

        if (_idle_func != NULL) {
            _idle_func();
        }
    }

    if (_focus_func != NULL) {
        _focus_func(1);
    }
}
