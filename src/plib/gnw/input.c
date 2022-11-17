#include "plib/gnw/input.h"

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
#include "plib/gnw/winmain.h"

typedef struct GNW95RepeatStruct {
    // Time when appropriate key was pressed down or -1 if it's up.
    TOCKS time;
    unsigned short count;
} GNW95RepeatStruct;

typedef struct inputdata {
    // This is either logical key or input event id, which can be either
    // character code pressed or some other numbers used throughout the
    // game interface.
    int input;
    int mx;
    int my;
} inputdata;

typedef struct funcdata {
    unsigned int flags;
    BackgroundProcess* f;
    struct funcdata* next;
} funcdata;

typedef funcdata* FuncPtr;

static int get_input_buffer();
static void pause_game();
static int default_pause_window();
static void buf_blit(unsigned char* src, int src_pitch, int a3, int x, int y, int width, int height, int dest_x, int dest_y);
static void GNW95_build_key_map();
static int GNW95_hook_keyboard(int hook);
static void GNW95_process_key(dxinput_key_data* data);

// NOT USED.
static IdleFunc* idle_func = NULL;

// NOT USED.
static FocusFunc* focus_func = NULL;

// 0x51E23C
static int GNW95_repeat_rate = 80;

// 0x51E240
static int GNW95_repeat_delay = 500;

// A map of DIK_* constants normalized for QWERTY keyboard.
//
// 0x6ABC70
unsigned char GNW95_key_map[256];

// Ring buffer of input events.
//
// Looks like this buffer does not support overwriting of values. Once the
// buffer is full it will not overwrite values until they are dequeued.
//
// 0x6ABD70
static inputdata input_buffer[40];

// 0x6ABF50
GNW95RepeatStruct GNW95_key_time_stamps[256];

// 0x6AC750
static int input_mx;

// 0x6AC754
static int input_my;

// 0x6AC758
static HHOOK GNW95_keyboardHandle;

// 0x6AC75C
static bool game_paused;

// 0x6AC760
static int screendump_key;

// 0x6AC764
static int using_msec_timer;

// 0x6AC768
static int pause_key;

// 0x6AC76C
static ScreenDumpFunc* screendump_func;

// 0x6AC770
static int input_get;

// 0x6AC774
static unsigned char* screendump_buf;

// 0x6AC778
static PauseWinFunc* pause_win_func;

// 0x6AC77C
static int input_put;

// 0x6AC780
static bool bk_disabled;

// 0x6AC784
static FuncPtr bk_list;

// 0x6AC788
static unsigned int bk_process_time;

// 0x4C8A70
int GNW_input_init(int use_msec_timer)
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

    if (GNW95_input_init() == -1) {
        return -1;
    }

    GNW95_hook_input(1);
    GNW95_build_key_map();
    GNW95_clear_time_stamps();

    using_msec_timer = use_msec_timer;
    input_put = 0;
    input_get = -1;
    input_mx = -1;
    input_my = -1;
    bk_disabled = 0;
    game_paused = false;
    pause_key = KEY_ALT_P;
    pause_win_func = default_pause_window;
    screendump_func = default_screendump;
    bk_list = NULL;
    screendump_key = KEY_ALT_C;

    return 0;
}

// 0x4C8B40
void GNW_input_exit()
{
    // NOTE: Uninline.
    GNW95_input_exit();
    GNW_mouse_exit();
    GNW_kb_restore();
    dxinput_exit();

    FuncPtr curr = bk_list;
    while (curr != NULL) {
        FuncPtr next = curr->next;
        mem_free(curr);
        curr = next;
    }
}

// 0x4C8B78
int get_input()
{
    int v3;

    GNW95_process_message();

    if (!GNW95_isActive) {
        GNW95_lost_focus();
    }

    process_bk();

    v3 = get_input_buffer();
    if (v3 == -1 && mouse_get_buttons() & 0x33) {
        mouse_get_position(&input_mx, &input_my);
        return -2;
    } else {
        return GNW_check_menu_bars(v3);
    }

    return -1;
}

// 0x4C8BDC
void process_bk()
{
    int v1;

    GNW_do_bk_process();

    if (vcr_update() != 3) {
        mouse_info();
    }

    v1 = win_check_all_buttons();
    if (v1 != -1) {
        GNW_add_input_buffer(v1);
        return;
    }

    v1 = kb_getch();
    if (v1 != -1) {
        GNW_add_input_buffer(v1);
        return;
    }
}

// 0x4C8C04
void GNW_add_input_buffer(int a1)
{
    if (a1 == -1) {
        return;
    }

    if (a1 == pause_key) {
        pause_game();
        return;
    }

    if (a1 == screendump_key) {
        dump_screen();
        return;
    }

    if (input_put == input_get) {
        return;
    }

    inputdata* inputEvent = &(input_buffer[input_put]);
    inputEvent->input = a1;

    mouse_get_position(&(inputEvent->mx), &(inputEvent->my));

    input_put++;

    if (input_put == 40) {
        input_put = 0;
        return;
    }

    if (input_get == -1) {
        input_get = 0;
    }
}

// 0x4C8C9C
static int get_input_buffer()
{
    if (input_get == -1) {
        return -1;
    }

    inputdata* inputEvent = &(input_buffer[input_get]);
    int eventCode = inputEvent->input;
    input_mx = inputEvent->mx;
    input_my = inputEvent->my;

    input_get++;

    if (input_get == 40) {
        input_get = 0;
    }

    if (input_get == input_put) {
        input_get = -1;
        input_put = 0;
    }

    return eventCode;
}

// 0x4C8D04
void flush_input_buffer()
{
    input_get = -1;
    input_put = 0;
}

// 0x4C8D1C
void GNW_do_bk_process()
{
    if (game_paused) {
        return;
    }

    if (bk_disabled) {
        return;
    }

    bk_process_time = get_time();

    FuncPtr curr = bk_list;
    FuncPtr* currPtr = &(bk_list);

    while (curr != NULL) {
        FuncPtr next = curr->next;
        if (curr->flags & 1) {
            *currPtr = next;

            mem_free(curr);
        } else {
            curr->f();
            currPtr = &(curr->next);
        }
        curr = next;
    }
}

// 0x4C8D74
void add_bk_process(BackgroundProcess* f)
{
    FuncPtr fp;

    fp = bk_list;
    while (fp != NULL) {
        if (fp->f == f) {
            if ((fp->flags & 0x01) != 0) {
                fp->flags &= ~0x01;
                return;
            }
        }
        fp = fp->next;
    }

    fp = (FuncPtr)mem_malloc(sizeof(*fp));
    fp->flags = 0;
    fp->f = f;
    fp->next = bk_list;
    bk_list = fp;
}

// 0x4C8DC4
void remove_bk_process(BackgroundProcess* f)
{
    FuncPtr fp;

    fp = bk_list;
    while (fp != NULL) {
        if (fp->f == f) {
            fp->flags |= 0x01;
            return;
        }
        fp = fp->next;
    }
}

// 0x4C8DE4
void enable_bk()
{
    bk_disabled = false;
}

// 0x4C8DF0
void disable_bk()
{
    bk_disabled = true;
}

// 0x4C8DFC
static void pause_game()
{
    if (!game_paused) {
        game_paused = true;

        int win = pause_win_func();

        while (get_input() != KEY_ESCAPE) {
        }

        game_paused = false;
        win_delete(win);
    }
}

// 0x4C8E38
static int default_pause_window()
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
void register_pause(int new_pause_key, PauseWinFunc* new_pause_win_func)
{
    pause_key = new_pause_key;

    if (new_pause_win_func == NULL) {
        new_pause_win_func = default_pause_window;
    }

    pause_win_func = new_pause_win_func;
}

// 0x4C8F4C
void dump_screen()
{
    int width = scr_size.lrx - scr_size.ulx + 1;
    int height = scr_size.lry - scr_size.uly + 1;
    screendump_buf = (unsigned char*)mem_malloc(width * height);
    if (screendump_buf == NULL) {
        return;
    }

    ScreenBlitFunc* v0 = scr_blit;
    scr_blit = buf_blit;

    ScreenBlitFunc* v2 = mouse_blit;
    mouse_blit = buf_blit;

    ScreenTransBlitFunc* v1 = mouse_blit_trans;
    mouse_blit_trans = NULL;

    win_refresh_all(&scr_size);

    mouse_blit_trans = v1;
    mouse_blit = v2;
    scr_blit = v0;

    unsigned char* palette = getSystemPalette();
    screendump_func(width, height, screendump_buf, palette);
    mem_free(screendump_buf);
}

// 0x4C8FF0
static void buf_blit(unsigned char* src, int srcPitch, int a3, int srcX, int srcY, int width, int height, int destX, int destY)
{
    int destWidth = scr_size.lrx - scr_size.ulx + 1;
    buf_to_buf(src + srcPitch * srcY + srcX, width, height, srcPitch, screendump_buf + destWidth * destY + destX, destWidth);
}

// 0x4C9048
int default_screendump(int width, int height, unsigned char* data, unsigned char* palette)
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
void register_screendump(int new_screendump_key, ScreenDumpFunc* new_screendump_func)
{
    screendump_key = new_screendump_key;

    if (new_screendump_func == NULL) {
        new_screendump_func = default_screendump;
    }

    screendump_func = new_screendump_func;
}

// 0x4C9370
TOCKS get_time()
{
#pragma warning(suppress : 28159)
    return GetTickCount();
}

// 0x4C937C
void pause_for_tocks(unsigned int delay)
{
    // NOTE: Uninline.
    unsigned int start = get_time();
    unsigned int end = get_time();

    // NOTE: Uninline.
    unsigned int diff = elapsed_tocks(end, start);
    while (diff < delay) {
        process_bk();

        end = get_time();

        // NOTE: Uninline.
        diff = elapsed_tocks(end, start);
    }
}

// 0x4C93B8
void block_for_tocks(unsigned int ms)
{
#pragma warning(suppress : 28159)
    unsigned int start = GetTickCount();
    unsigned int diff;
    do {
        // NOTE: Uninline
        diff = elapsed_time(start);
    } while (diff < ms);
}

// 0x4C93E0
unsigned int elapsed_time(unsigned int start)
{
#pragma warning(suppress : 28159)
    unsigned int end = GetTickCount();

    // NOTE: Uninline.
    return elapsed_tocks(end, start);
}

// 0x4C9400
unsigned int elapsed_tocks(unsigned int end, unsigned int start)
{
    if (start > end) {
        return INT_MAX;
    } else {
        return end - start;
    }
}

// 0x4C9410
unsigned int get_bk_time()
{
    return bk_process_time;
}

// 0x4C9490
static void GNW95_build_key_map()
{
    unsigned char* keys = GNW95_key_map;
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
void GNW95_hook_input(int hook)
{
    GNW95_hook_keyboard(hook);

    if (hook) {
        dxinput_acquire_mouse();
    } else {
        dxinput_unacquire_mouse();
    }
}

// 0x4C9C20
int GNW95_input_init()
{
    return 0;
}

// NOTE: Inlined.
//
// 0x4C9C24
void GNW95_input_exit()
{
    GNW95_hook_keyboard(0);
}

// 0x4C9C28
static int GNW95_hook_keyboard(int hook)
{
    // 0x51E244
    static bool hooked = false;

    if (hook == hooked) {
        return 0;
    }

    if (!hook) {
        dxinput_unacquire_keyboard();

        UnhookWindowsHookEx(GNW95_keyboardHandle);

        kb_clear();

        hooked = hook;

        return 0;
    }

    if (dxinput_acquire_keyboard()) {
        GNW95_keyboardHandle = SetWindowsHookExA(WH_KEYBOARD, GNW95_keyboard_hook, 0, GetCurrentThreadId());
        kb_clear();
        hooked = hook;

        return 0;
    }

    return -1;
}

// 0x4C9C4C
LRESULT CALLBACK GNW95_keyboard_hook(int nCode, WPARAM wParam, LPARAM lParam)
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

    return CallNextHookEx(GNW95_keyboardHandle, nCode, wParam, lParam);
}

// 0x4C9CF0
void GNW95_process_message()
{
    if (GNW95_isActive && !kb_is_disabled()) {
        dxinput_key_data data;
        while (dxinput_read_keyboard_buffer(&data)) {
            GNW95_process_key(&data);
        }

        // NOTE: Uninline
        TOCKS now = get_time();

        for (int key = 0; key < 256; key++) {
            GNW95RepeatStruct* ptr = &(GNW95_key_time_stamps[key]);
            if (ptr->time != -1) {
                int elapsedTime = ptr->time > now ? INT_MAX : now - ptr->time;
                int delay = ptr->count == 0 ? GNW95_repeat_delay : GNW95_repeat_rate;
                if (elapsedTime > delay) {
                    data.code = key;
                    data.state = 1;
                    GNW95_process_key(&data);

                    ptr->time = now;
                    ptr->count++;
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
void GNW95_clear_time_stamps()
{
    for (int index = 0; index < 256; index++) {
        GNW95_key_time_stamps[index].time = -1;
        GNW95_key_time_stamps[index].count = 0;
    }
}

// 0x4C9E14
static void GNW95_process_key(dxinput_key_data* data)
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

    int qwertyKey = GNW95_key_map[data->code & 0xFF];

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

        GNW95RepeatStruct* ptr = &(GNW95_key_time_stamps[data->code & 0xFF]);
        if (data->state == 1) {
            ptr->time = get_time();
            ptr->count = 0;
        } else {
            qwertyKey |= 0x80;
            ptr->time = -1;
        }

        kb_simulate_key(qwertyKey);
    }
}

// 0x4C9EEC
void GNW95_lost_focus()
{
    if (focus_func != NULL) {
        focus_func(0);
    }

    while (!GNW95_isActive) {
        GNW95_process_message();

        if (idle_func != NULL) {
            idle_func();
        }
    }

    if (focus_func != NULL) {
        focus_func(1);
    }
}
