#include "game/display.h"

#include <stdbool.h>
#include <string.h>

#include "game/art.h"
#include "color.h"
#include "game/combat.h"
#include "core.h"
#include "draw.h"
#include "game/gmouse.h"
#include "game/gsound.h"
#include "geometry.h"
#include "interface.h"
#include "memory.h"
#include "text_font.h"
#include "window_manager.h"

// The maximum number of lines display monitor can hold. Once this value
// is reached earlier messages are thrown away.
#define DISPLAY_MONITOR_LINES_CAPACITY 100

// The maximum length of a string in display monitor (in characters).
#define DISPLAY_MONITOR_LINE_LENGTH 80

#define DISPLAY_MONITOR_X 23
#define DISPLAY_MONITOR_Y 24
#define DISPLAY_MONITOR_WIDTH 167
#define DISPLAY_MONITOR_HEIGHT 60

#define DISPLAY_MONITOR_HALF_HEIGHT (DISPLAY_MONITOR_HEIGHT / 2)

#define DISPLAY_MONITOR_FONT 101

#define DISPLAY_MONITOR_BEEP_DELAY 500U

// 0x51850C
static bool disp_init = false;

// The rectangle that display monitor occupies in the main interface window.
//
// 0x518510
static Rect disp_rect = {
    DISPLAY_MONITOR_X,
    DISPLAY_MONITOR_Y,
    DISPLAY_MONITOR_X + DISPLAY_MONITOR_WIDTH - 1,
    DISPLAY_MONITOR_Y + DISPLAY_MONITOR_HEIGHT - 1,
};

// 0x518520
static int dn_bid = -1;

// 0x518524
static int up_bid = -1;

// 0x56DBFC
static char disp_str[DISPLAY_MONITOR_LINES_CAPACITY][DISPLAY_MONITOR_LINE_LENGTH];

// 0x56FB3C
static unsigned char* disp_buf;

// 0x56FB40
static int max_disp_ptr;

// 0x56FB44
static bool display_enabled;

// 0x56FB48
static int disp_curr;

// 0x56FB4C
static int intface_full_wid;

// 0x56FB50
static int max_ptr;

// 0x56FB54
static int disp_start;

// 0x431610
int display_init()
{
    if (!disp_init) {
        int oldFont = fontGetCurrent();
        fontSetCurrent(DISPLAY_MONITOR_FONT);

        max_ptr = DISPLAY_MONITOR_LINES_CAPACITY;
        max_disp_ptr = DISPLAY_MONITOR_HEIGHT / fontGetLineHeight();
        disp_start = 0;
        disp_curr = 0;
        fontSetCurrent(oldFont);

        disp_buf = (unsigned char*)internal_malloc(DISPLAY_MONITOR_WIDTH * DISPLAY_MONITOR_HEIGHT);
        if (disp_buf == NULL) {
            return -1;
        }

        CacheEntry* backgroundFrmHandle;
        int backgroundFid = art_id(OBJ_TYPE_INTERFACE, 16, 0, 0, 0);
        Art* backgroundFrm = art_ptr_lock(backgroundFid, &backgroundFrmHandle);
        if (backgroundFrm == NULL) {
            internal_free(disp_buf);
            return -1;
        }

        unsigned char* backgroundFrmData = art_frame_data(backgroundFrm, 0, 0);
        intface_full_wid = art_frame_width(backgroundFrm, 0, 0);
        blitBufferToBuffer(backgroundFrmData + intface_full_wid * DISPLAY_MONITOR_Y + DISPLAY_MONITOR_X,
            DISPLAY_MONITOR_WIDTH,
            DISPLAY_MONITOR_HEIGHT,
            intface_full_wid,
            disp_buf,
            DISPLAY_MONITOR_WIDTH);

        art_ptr_unlock(backgroundFrmHandle);

        up_bid = buttonCreate(gInterfaceBarWindow,
            DISPLAY_MONITOR_X,
            DISPLAY_MONITOR_Y,
            DISPLAY_MONITOR_WIDTH,
            DISPLAY_MONITOR_HALF_HEIGHT,
            -1,
            -1,
            -1,
            -1,
            NULL,
            NULL,
            NULL,
            0);
        if (up_bid != -1) {
            buttonSetMouseCallbacks(up_bid,
                display_arrow_up,
                display_arrow_restore,
                display_scroll_up,
                NULL);
        }

        dn_bid = buttonCreate(gInterfaceBarWindow,
            DISPLAY_MONITOR_X,
            DISPLAY_MONITOR_Y + DISPLAY_MONITOR_HALF_HEIGHT,
            DISPLAY_MONITOR_WIDTH,
            DISPLAY_MONITOR_HEIGHT - DISPLAY_MONITOR_HALF_HEIGHT,
            -1,
            -1,
            -1,
            -1,
            NULL,
            NULL,
            NULL,
            0);
        if (dn_bid != -1) {
            buttonSetMouseCallbacks(dn_bid,
                display_arrow_down,
                display_arrow_restore,
                display_scroll_down,
                NULL);
        }

        display_enabled = true;
        disp_init = true;

        // NOTE: Uninline.
        display_clear();
    }

    return 0;
}

// 0x431800
int display_reset()
{
    // NOTE: Uninline.
    display_clear();

    return 0;
}

// 0x43184C
void display_exit()
{
    if (disp_init) {
        internal_free(disp_buf);
        disp_init = false;
    }
}

// 0x43186C
void display_print(char* str)
{
    // 0x56FB58
    static unsigned int last_time;

    if (!disp_init) {
        return;
    }

    int oldFont = fontGetCurrent();
    fontSetCurrent(DISPLAY_MONITOR_FONT);

    char knob = '\x95';

    char knobString[2];
    knobString[0] = knob;
    knobString[1] = '\0';
    int knobWidth = fontGetStringWidth(knobString);

    if (!isInCombat()) {
        unsigned int now = _get_bk_time();
        if (getTicksBetween(now, last_time) >= DISPLAY_MONITOR_BEEP_DELAY) {
            last_time = now;
            soundPlayFile("monitor");
        }
    }

    // TODO: Refactor these two loops.
    char* v1 = NULL;
    while (true) {
        while (fontGetStringWidth(str) < DISPLAY_MONITOR_WIDTH - max_disp_ptr - knobWidth) {
            char* temp = disp_str[disp_start];
            int length;
            if (knob != '\0') {
                *temp++ = knob;
                length = DISPLAY_MONITOR_LINE_LENGTH - 2;
                knob = '\0';
                knobWidth = 0;
            } else {
                length = DISPLAY_MONITOR_LINE_LENGTH - 1;
            }
            strncpy(temp, str, length);
            disp_str[disp_start][DISPLAY_MONITOR_LINE_LENGTH - 1] = '\0';
            disp_start = (disp_start + 1) % max_ptr;

            if (v1 == NULL) {
                fontSetCurrent(oldFont);
                disp_curr = disp_start;
                display_redraw();
                return;
            }

            str = v1 + 1;
            *v1 = ' ';
            v1 = NULL;
        }

        char* space = strrchr(str, ' ');
        if (space == NULL) {
            break;
        }

        if (v1 != NULL) {
            *v1 = ' ';
        }

        v1 = space;
        if (space != NULL) {
            *space = '\0';
        }
    }

    char* temp = disp_str[disp_start];
    int length;
    if (knob != '\0') {
        temp++;
        disp_str[disp_start][0] = knob;
        length = DISPLAY_MONITOR_LINE_LENGTH - 2;
        knob = '\0';
    } else {
        length = DISPLAY_MONITOR_LINE_LENGTH - 1;
    }
    strncpy(temp, str, length);

    disp_str[disp_start][DISPLAY_MONITOR_LINE_LENGTH - 1] = '\0';
    disp_start = (disp_start + 1) % max_ptr;

    fontSetCurrent(oldFont);
    disp_curr = disp_start;
    display_redraw();
}

// NOTE: Inlined.
//
// 0x431A2C
void display_clear()
{
    int index;

    if (disp_init) {
        for (index = 0; index < max_ptr; index++) {
            disp_str[index][0] = '\0';
        }

        disp_start = 0;
        disp_curr = 0;
        display_redraw();
    }
}

// 0x431A78
void display_redraw()
{
    if (!disp_init) {
        return;
    }

    unsigned char* buf = windowGetBuffer(gInterfaceBarWindow);
    if (buf == NULL) {
        return;
    }

    buf += intface_full_wid * DISPLAY_MONITOR_Y + DISPLAY_MONITOR_X;
    blitBufferToBuffer(disp_buf,
        DISPLAY_MONITOR_WIDTH,
        DISPLAY_MONITOR_HEIGHT,
        DISPLAY_MONITOR_WIDTH,
        buf,
        intface_full_wid);

    int oldFont = fontGetCurrent();
    fontSetCurrent(DISPLAY_MONITOR_FONT);

    for (int index = 0; index < max_disp_ptr; index++) {
        int stringIndex = (disp_curr + max_ptr + index - max_disp_ptr) % max_ptr;
        fontDrawText(buf + index * intface_full_wid * fontGetLineHeight(), disp_str[stringIndex], DISPLAY_MONITOR_WIDTH, intface_full_wid, colorTable[992]);

        // Even though the display monitor is rectangular, it's graphic is not.
        // To give a feel of depth it's covered by some metal canopy and
        // considered inclined outwards. This way earlier messages appear a
        // little bit far from player's perspective. To implement this small
        // detail the destination buffer is incremented by 1.
        buf++;
    }

    win_draw_rect(gInterfaceBarWindow, &disp_rect);
    fontSetCurrent(oldFont);
}

// 0x431B70
void display_scroll_up(int btn, int keyCode)
{
    if ((max_ptr + disp_curr - 1) % max_ptr != disp_start) {
        disp_curr = (max_ptr + disp_curr - 1) % max_ptr;
        display_redraw();
    }
}

// 0x431B9C
void display_scroll_down(int btn, int keyCode)
{
    if (disp_curr != disp_start) {
        disp_curr = (disp_curr + 1) % max_ptr;
        display_redraw();
    }
}

// 0x431BC8
void display_arrow_up(int btn, int keyCode)
{
    gmouse_set_cursor(MOUSE_CURSOR_SMALL_ARROW_UP);
}

// 0x431BD4
void display_arrow_down(int btn, int keyCode)
{
    gmouse_set_cursor(MOUSE_CURSOR_SMALL_ARROW_DOWN);
}

// 0x431BE0
void display_arrow_restore(int btn, int keyCode)
{
    gmouse_set_cursor(MOUSE_CURSOR_ARROW);
}

// 0x431BEC
void display_disable()
{
    if (display_enabled) {
        buttonDisable(dn_bid);
        buttonDisable(up_bid);
        display_enabled = false;
    }
}

// 0x431C14
void display_enable()
{
    if (!display_enabled) {
        buttonEnable(dn_bid);
        buttonEnable(up_bid);
        display_enabled = true;
    }
}
