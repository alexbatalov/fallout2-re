#include "mouse.h"

#include "color.h"
#include "core.h"
#include "dinput.h"
#include "memory.h"
#include "vcr.h"

static void mouse_colorize();
static void mouse_anim();
static void mouse_clip();

// The default mouse cursor buffer.
//
// Initially it contains color codes, which will be replaced at startup
// according to loaded palette.
//
// Available color codes:
// - 0: transparent
// - 1: white
// - 15:  black
//
// 0x51E250
static unsigned char or_mask[MOUSE_DEFAULT_CURSOR_SIZE] = {
    // clang-format off
    1,  1,  1,  1,  1,  1,  1, 0,
    1, 15, 15, 15, 15, 15,  1, 0,
    1, 15, 15, 15, 15,  1,  1, 0,
    1, 15, 15, 15, 15,  1,  1, 0,
    1, 15, 15, 15, 15, 15,  1, 1,
    1, 15,  1,  1, 15, 15, 15, 1,
    1,  1,  1,  1,  1, 15, 15, 1,
    0,  0,  0,  0,  1,  1,  1, 1,
    // clang-format on
};

// 0x51E290
static int mouse_idling = 0;

// 0x51E294
static unsigned char* mouse_buf = NULL;

// 0x51E298
static unsigned char* mouse_shape = NULL;

// 0x51E29C
static unsigned char* mouse_fptr = NULL;

// 0x51E2A0
static double mouse_sensitivity = 1.0;

// 0x51E2AC
static int last_buttons = 0;

// 0x6AC790
static bool mouse_is_hidden;

// 0x6AC794
static int raw_x;

// 0x6AC798
static int mouse_length;

// 0x6AC79C
static int raw_y;

// 0x6AC7A0
static int raw_buttons;

// 0x6AC7A4
static int mouse_y;

// 0x6AC7A8
static int mouse_x;

// 0x6AC7AC
static bool mouse_disabled;

// 0x6AC7B0
static int mouse_buttons;

// 0x6AC7B4
static unsigned int mouse_speed;

// 0x6AC7B8
static int mouse_curr_frame;

// 0x6AC7BC
static bool have_mouse;

// 0x6AC7C0
static int mouse_full;

// 0x6AC7C4
static int mouse_width;

// 0x6AC7C8
static int mouse_num_frames;

// 0x6AC7CC
static int mouse_hoty;

// 0x6AC7D0
static int mouse_hotx;

// 0x6AC7D4
unsigned int mouse_idle_start_time;

// 0x6AC7D8
ScreenTransBlitFunc* mouse_blit_trans;

// 0x6AC7DC
ScreenBlitFunc* mouse_blit;

// 0x6AC7E0
static char mouse_trans;

// 0x4C9F40
int GNW_mouse_init()
{
    have_mouse = false;
    mouse_disabled = false;

    mouse_is_hidden = true;

    mouse_colorize();

    if (mouse_set_shape(NULL, 0, 0, 0, 0, 0, 0) == -1) {
        return -1;
    }

    if (!mouseDeviceAcquire()) {
        return -1;
    }

    have_mouse = true;
    mouse_x = _scr_size.right / 2;
    mouse_y = _scr_size.bottom / 2;
    raw_x = _scr_size.right / 2;
    raw_y = _scr_size.bottom / 2;
    mouse_idle_start_time = _get_time();

    return 0;
}

// 0x4C9FD8
void GNW_mouse_exit()
{
    mouseDeviceUnacquire();

    if (mouse_buf != NULL) {
        internal_free(mouse_buf);
        mouse_buf = NULL;
    }

    if (mouse_fptr != NULL) {
        tickersRemove(mouse_anim);
        mouse_fptr = NULL;
    }
}

// 0x4CA01C
static void mouse_colorize()
{
    for (int index = 0; index < 64; index++) {
        switch (or_mask[index]) {
        case 0:
            or_mask[index] = colorTable[0];
            break;
        case 1:
            or_mask[index] = colorTable[8456];
            break;
        case 15:
            or_mask[index] = colorTable[32767];
            break;
        }
    }
}

// NOTE: Unused.
//
// 0x4CA064
void mouse_get_shape(unsigned char** buf, int* width, int* length, int* full, int* hotx, int* hoty, char* trans)
{
    *buf = mouse_shape;
    *width = mouse_width;
    *length = mouse_length;
    *full = mouse_full;
    *hotx = mouse_hotx;
    *hoty = mouse_hoty;
    *trans = mouse_trans;
}

// 0x4CA0AC
int mouse_set_shape(unsigned char* buf, int width, int length, int full, int hotx, int hoty, char trans)
{
    Rect rect;
    unsigned char* v9;
    int v11, v12;
    int v7, v8;

    v7 = hotx;
    v8 = hoty;
    v9 = buf;

    if (buf == NULL) {
        // NOTE: Original code looks tail recursion optimization.
        return mouse_set_shape(or_mask, MOUSE_DEFAULT_CURSOR_WIDTH, MOUSE_DEFAULT_CURSOR_HEIGHT, MOUSE_DEFAULT_CURSOR_WIDTH, 1, 1, colorTable[0]);
    }

    bool cursorWasHidden = mouse_is_hidden;
    if (!mouse_is_hidden && have_mouse) {
        mouse_is_hidden = true;
        mouse_get_rect(&rect);
        windowRefreshAll(&rect);
    }

    if (width != mouse_width || length != mouse_length) {
        unsigned char* buf = (unsigned char*)internal_malloc(width * length);
        if (buf == NULL) {
            if (!cursorWasHidden) {
                mouse_show();
            }
            return -1;
        }

        if (mouse_buf != NULL) {
            internal_free(mouse_buf);
        }

        mouse_buf = buf;
    }

    mouse_width = width;
    mouse_length = length;
    mouse_full = full;
    mouse_shape = v9;
    mouse_trans = trans;

    if (mouse_fptr) {
        tickersRemove(mouse_anim);
        mouse_fptr = NULL;
    }

    v11 = mouse_hotx - v7;
    mouse_hotx = v7;

    mouse_x += v11;

    v12 = mouse_hoty - v8;
    mouse_hoty = v8;

    mouse_y += v12;

    mouse_clip();

    if (!cursorWasHidden) {
        mouse_show();
    }

    raw_x = mouse_x;
    raw_y = mouse_y;

    return 0;
}

// NOTE: Unused.
//
// 0x4CA20
int mouse_get_anim(unsigned char** frames, int* num_frames, int* width, int* length, int* hotx, int* hoty, char* trans, int* speed)
{
    if (mouse_fptr == NULL) {
        return -1;
    }

    *frames = mouse_fptr;
    *num_frames = mouse_num_frames;
    *width = mouse_width;
    *length = mouse_length;
    *hotx = mouse_hotx;
    *hoty = mouse_hoty;
    *trans = mouse_trans;
    *speed = mouse_speed;

    return 0;
}

// NOTE: Unused.
//
// 0x4CA26C
int mouse_set_anim_frames(unsigned char* frames, int num_frames, int start_frame, int width, int length, int hotx, int hoty, char trans, int speed)
{
    if (mouse_set_shape(frames + start_frame * width * length, width, length, width, hotx, hoty, trans) == -1) {
        return -1;
    }

    mouse_fptr = frames;
    mouse_num_frames = num_frames;
    mouse_curr_frame = start_frame;
    mouse_speed = speed;

    tickersAdd(mouse_anim);

    return 0;
}

// 0x4CA2D0
static void mouse_anim()
{
    // 0x51E2A8
    static unsigned int ticker = 0;

    if (getTicksSince(ticker) >= mouse_speed) {
        ticker = _get_time();

        if (++mouse_curr_frame == mouse_num_frames) {
            mouse_curr_frame = 0;
        }

        mouse_shape = mouse_width * mouse_curr_frame * mouse_length + mouse_fptr;

        if (!mouse_is_hidden) {
            mouse_show();
        }
    }
}

// 0x4CA34C
void mouse_show()
{
    int i;
    unsigned char* v2;
    int v7, v8;
    int v9, v10;
    int v4;
    unsigned char v6;
    int v3;

    v2 = mouse_buf;
    if (have_mouse) {
        if (!mouse_blit_trans || !mouse_is_hidden) {
            _win_get_mouse_buf(mouse_buf);
            v2 = mouse_buf;
            v3 = 0;

            for (i = 0; i < mouse_length; i++) {
                for (v4 = 0; v4 < mouse_width; v4++) {
                    v6 = mouse_shape[i * mouse_full + v4];
                    if (v6 != mouse_trans) {
                        v2[v3] = v6;
                    }
                    v3++;
                }
            }
        }

        if (mouse_x >= _scr_size.left) {
            if (mouse_width + mouse_x - 1 <= _scr_size.right) {
                v8 = mouse_width;
                v7 = 0;
            } else {
                v7 = 0;
                v8 = _scr_size.right - mouse_x + 1;
            }
        } else {
            v7 = _scr_size.left - mouse_x;
            v8 = mouse_width - (_scr_size.left - mouse_x);
        }

        if (mouse_y >= _scr_size.top) {
            if (mouse_length + mouse_y - 1 <= _scr_size.bottom) {
                v9 = 0;
                v10 = mouse_length;
            } else {
                v9 = 0;
                v10 = _scr_size.bottom - mouse_y + 1;
            }
        } else {
            v9 = _scr_size.top - mouse_y;
            v10 = mouse_length - (_scr_size.top - mouse_y);
        }

        mouse_buf = v2;
        if (mouse_blit_trans && mouse_is_hidden) {
            mouse_blit_trans(mouse_shape, mouse_full, mouse_length, v7, v9, v8, v10, v7 + mouse_x, v9 + mouse_y, mouse_trans);
        } else {
            mouse_blit(mouse_buf, mouse_width, mouse_length, v7, v9, v8, v10, v7 + mouse_x, v9 + mouse_y);
        }

        v2 = mouse_buf;
        mouse_is_hidden = false;
    }
    mouse_buf = v2;
}

// 0x4CA534
void mouse_hide()
{
    Rect rect;

    if (have_mouse) {
        if (!mouse_is_hidden) {
            rect.left = mouse_x;
            rect.top = mouse_y;
            rect.right = mouse_x + mouse_width - 1;
            rect.bottom = mouse_y + mouse_length - 1;

            mouse_is_hidden = true;
            windowRefreshAll(&rect);
        }
    }
}

// 0x4CA59C
void mouse_info()
{
    if (!have_mouse) {
        return;
    }

    if (mouse_is_hidden) {
        return;
    }

    if (mouse_disabled) {
        return;
    }

    int x;
    int y;
    int buttons = 0;

    MouseData mouseData;
    if (mouseDeviceGetData(&mouseData)) {
        x = mouseData.x;
        y = mouseData.y;

        if (mouseData.buttons[0] == 1) {
            buttons |= MOUSE_STATE_LEFT_BUTTON_DOWN;
        }

        if (mouseData.buttons[1] == 1) {
            buttons |= MOUSE_STATE_RIGHT_BUTTON_DOWN;
        }
    } else {
        x = 0;
        y = 0;
    }

    // Adjust for mouse senstivity.
    x = (int)(x * mouse_sensitivity);
    y = (int)(y * mouse_sensitivity);

    if (vcr_state == VCR_STATE_PLAYING) {
        if (((vcr_terminate_flags & VCR_TERMINATE_ON_MOUSE_PRESS) != 0 && buttons != 0)
            || ((vcr_terminate_flags & VCR_TERMINATE_ON_MOUSE_MOVE) != 0 && (x != 0 || y != 0))) {
            vcr_terminated_condition = VCR_PLAYBACK_COMPLETION_REASON_TERMINATED;
            vcr_stop();
            return;
        }
        x = 0;
        y = 0;
        buttons = last_buttons;
    }

    mouse_simulate_input(x, y, buttons);
}

// 0x4CA698
void mouse_simulate_input(int delta_x, int delta_y, int buttons)
{
    // 0x6AC7E4
    static unsigned int right_time;

    // 0x6AC7E8
    static unsigned int left_time;

    // 0x6AC7EC
    static int old;

    if (!have_mouse || mouse_is_hidden) {
        return;
    }

    if (delta_x || delta_y || buttons != last_buttons) {
        if (vcr_state == 0) {
            if (vcr_buffer_index == VCR_BUFFER_CAPACITY - 1) {
                vcr_dump_buffer();
            }

            VcrEntry* vcrEntry = &(vcr_buffer[vcr_buffer_index]);
            vcrEntry->type = VCR_ENTRY_TYPE_MOUSE_EVENT;
            vcrEntry->time = vcr_time;
            vcrEntry->counter = vcr_counter;
            vcrEntry->mouseEvent.dx = delta_x;
            vcrEntry->mouseEvent.dy = delta_y;
            vcrEntry->mouseEvent.buttons = buttons;

            vcr_buffer_index++;
        }
    } else {
        if (last_buttons == 0) {
            if (!mouse_idling) {
                mouse_idle_start_time = _get_time();
                mouse_idling = 1;
            }

            last_buttons = 0;
            raw_buttons = 0;
            mouse_buttons = 0;

            return;
        }
    }

    mouse_idling = 0;
    last_buttons = buttons;
    old = mouse_buttons;
    mouse_buttons = 0;

    if ((old & MOUSE_EVENT_LEFT_BUTTON_DOWN_REPEAT) != 0) {
        if ((buttons & 0x01) != 0) {
            mouse_buttons |= MOUSE_EVENT_LEFT_BUTTON_REPEAT;

            if (getTicksSince(left_time) > BUTTON_REPEAT_TIME) {
                mouse_buttons |= MOUSE_EVENT_LEFT_BUTTON_DOWN;
                left_time = _get_time();
            }
        } else {
            mouse_buttons |= MOUSE_EVENT_LEFT_BUTTON_UP;
        }
    } else {
        if ((buttons & 0x01) != 0) {
            mouse_buttons |= MOUSE_EVENT_LEFT_BUTTON_DOWN;
            left_time = _get_time();
        }
    }

    if ((old & MOUSE_EVENT_RIGHT_BUTTON_DOWN_REPEAT) != 0) {
        if ((buttons & 0x02) != 0) {
            mouse_buttons |= MOUSE_EVENT_RIGHT_BUTTON_REPEAT;
            if (getTicksSince(right_time) > BUTTON_REPEAT_TIME) {
                mouse_buttons |= MOUSE_EVENT_RIGHT_BUTTON_DOWN;
                right_time = _get_time();
            }
        } else {
            mouse_buttons |= MOUSE_EVENT_RIGHT_BUTTON_UP;
        }
    } else {
        if (buttons & 0x02) {
            mouse_buttons |= MOUSE_EVENT_RIGHT_BUTTON_DOWN;
            right_time = _get_time();
        }
    }

    raw_buttons = mouse_buttons;

    if (delta_x != 0 || delta_y != 0) {
        Rect mouseRect;
        mouseRect.left = mouse_x;
        mouseRect.top = mouse_y;
        mouseRect.right = mouse_width + mouse_x - 1;
        mouseRect.bottom = mouse_length + mouse_y - 1;

        mouse_x += delta_x;
        mouse_y += delta_y;
        mouse_clip();

        windowRefreshAll(&mouseRect);

        mouse_show();

        raw_x = mouse_x;
        raw_y = mouse_y;
    }
}

// 0x4CA8C8
bool mouse_in(int left, int top, int right, int bottom)
{
    if (!have_mouse) {
        return false;
    }

    return mouse_length + mouse_y > top
        && right >= mouse_x
        && mouse_width + mouse_x > left
        && bottom >= mouse_y;
}

// 0x4CA934
bool mouse_click_in(int left, int top, int right, int bottom)
{
    if (!have_mouse) {
        return false;
    }

    return mouse_hoty + mouse_y >= top
        && mouse_hotx + mouse_x <= right
        && mouse_hotx + mouse_x >= left
        && mouse_hoty + mouse_y <= bottom;
}

// 0x4CA9A0
void mouse_get_rect(Rect* rect)
{
    rect->left = mouse_x;
    rect->top = mouse_y;
    rect->right = mouse_width + mouse_x - 1;
    rect->bottom = mouse_length + mouse_y - 1;
}

// 0x4CA9DC
void mouse_get_position(int* xPtr, int* yPtr)
{
    *xPtr = mouse_hotx + mouse_x;
    *yPtr = mouse_hoty + mouse_y;
}

// 0x4CAA04
void mouse_set_position(int a1, int a2)
{
    mouse_x = a1 - mouse_hotx;
    mouse_y = a2 - mouse_hoty;
    raw_y = a2 - mouse_hoty;
    raw_x = a1 - mouse_hotx;
    mouse_clip();
}

// 0x4CAA38
static void mouse_clip()
{
    if (mouse_hotx + mouse_x < _scr_size.left) {
        mouse_x = _scr_size.left - mouse_hotx;
    } else if (mouse_hotx + mouse_x > _scr_size.right) {
        mouse_x = _scr_size.right - mouse_hotx;
    }

    if (mouse_hoty + mouse_y < _scr_size.top) {
        mouse_y = _scr_size.top - mouse_hoty;
    } else if (mouse_hoty + mouse_y > _scr_size.bottom) {
        mouse_y = _scr_size.bottom - mouse_hoty;
    }
}

// 0x4CAAA0
int mouse_get_buttons()
{
    return mouse_buttons;
}

// 0x4CAAA8
bool mouse_hidden()
{
    return mouse_is_hidden;
}

// NOTE: Unused.
//
// 0x4CAAB0
void mouse_get_hotspot(int* hotx, int* hoty)
{
    *hotx = mouse_hotx;
    *hoty = mouse_hoty;
}

// NOTE: Unused.
//
// 0x4CAAC4
void mouse_set_hotspot(int hotx, int hoty)
{
    bool mh;

    mh = mouse_is_hidden;
    if (!mouse_is_hidden) {
        mouse_hide();
    }

    mouse_x += mouse_hotx - hotx;
    mouse_y += mouse_hoty - hoty;
    mouse_hotx = hotx;
    mouse_hoty = hoty;

    if (mh) {
        mouse_show();
    }
}

// NOTE: Unused.
//
// 0x4CAB54
bool mouse_query_exist()
{
    return have_mouse;
}

// 0x4CAB5C
void mouse_get_raw_state(int* out_x, int* out_y, int* out_buttons)
{
    MouseData mouseData;
    if (!mouseDeviceGetData(&mouseData)) {
        mouseData.x = 0;
        mouseData.y = 0;
        mouseData.buttons[0] = (mouse_buttons & MOUSE_EVENT_LEFT_BUTTON_DOWN) != 0;
        mouseData.buttons[1] = (mouse_buttons & MOUSE_EVENT_RIGHT_BUTTON_DOWN) != 0;
    }

    raw_buttons = 0;
    raw_x += mouseData.x;
    raw_y += mouseData.y;

    if (mouseData.buttons[0] != 0) {
        raw_buttons |= MOUSE_EVENT_LEFT_BUTTON_DOWN;
    }

    if (mouseData.buttons[1] != 0) {
        raw_buttons |= MOUSE_EVENT_RIGHT_BUTTON_DOWN;
    }

    *out_x = raw_x;
    *out_y = raw_y;
    *out_buttons = raw_buttons;
}

// NOTE: Unused.
//
// 0x4CAC1C
void mouse_disable()
{
    mouse_disabled = true;
}

// NOTE: Unused.
//
// 0x4CAC28
void mouse_enable()
{
    mouse_disabled = false;
}

// NOTE: Unused.
//
// 0x4CAC34
bool mouse_is_disabled()
{
    return mouse_disabled;
}

// 0x4CAC3C
void mouse_set_sensitivity(double value)
{
    if (value > 0 && value < 2.0) {
        mouse_sensitivity = value;
    }
}

// NOTE: Unused
//
// 0x4CAC6C
double mouse_get_sensitivity()
{
    return mouse_sensitivity;
}

// NOTE: Unused.
//
// 0x4CAC74
unsigned int mouse_elapsed_time()
{
    if (mouse_idling) {
        if (have_mouse && !mouse_is_hidden && !mouse_disabled) {
            return getTicksSince(mouse_idle_start_time);
        }
        mouse_idling = false;
    }
    return 0;
}

// NOTE: Unused.
//
// 0x4CAC74
void mouse_reset_elapsed_time()
{
    if (mouse_idling) {
        mouse_idling = false;
    }
}
