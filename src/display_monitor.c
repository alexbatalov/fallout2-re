#include "display_monitor.h"

#include "art.h"
#include "color.h"
#include "combat.h"
#include "core.h"
#include "draw.h"
#include "game_mouse.h"
#include "game_sound.h"
#include "interface.h"
#include "memory.h"
#include "text_font.h"
#include "window_manager.h"

// 0x51850C
bool gDisplayMonitorInitialized = false;

// The rectangle that display monitor occupies in the main interface window.
//
// 0x518510
const Rect gDisplayMonitorRect = {
    DISPLAY_MONITOR_X,
    DISPLAY_MONITOR_Y,
    DISPLAY_MONITOR_X + DISPLAY_MONITOR_WIDTH - 1,
    DISPLAY_MONITOR_Y + DISPLAY_MONITOR_HEIGHT - 1,
};

// 0x518520
int gDisplayMonitorScrollDownButton = -1;

// 0x518524
int gDisplayMonitorScrollUpButton = -1;

// 0x56DBFC
char gDisplayMonitorLines[DISPLAY_MONITOR_LINES_CAPACITY][DISPLAY_MONITOR_LINE_LENGTH];

// 0x56FB3C
unsigned char* gDisplayMonitorBackgroundFrmData;

// 0x56FB40
int dword_56FB40;

// 0x56FB44
bool gDisplayMonitorEnabled;

// 0x56FB48
int dword_56FB48;

// 0x56FB4C
int dword_56FB4C;

// 0x56FB50
int gDisplayMonitorLinesCapacity;

// 0x56FB54
int dword_56FB54;

// 0x56FB58
unsigned int gDisplayMonitorLastBeepTimestamp;

// 0x431610
int displayMonitorInit()
{
    if (!gDisplayMonitorInitialized) {
        int oldFont = fontGetCurrent();
        fontSetCurrent(DISPLAY_MONITOR_FONT);

        gDisplayMonitorLinesCapacity = DISPLAY_MONITOR_LINES_CAPACITY;
        dword_56FB40 = DISPLAY_MONITOR_HEIGHT / fontGetLineHeight();
        dword_56FB54 = 0;
        dword_56FB48 = 0;
        fontSetCurrent(oldFont);

        gDisplayMonitorBackgroundFrmData = internal_malloc(DISPLAY_MONITOR_WIDTH * DISPLAY_MONITOR_HEIGHT);
        if (gDisplayMonitorBackgroundFrmData == NULL) {
            return -1;
        }

        CacheEntry* backgroundFrmHandle;
        int backgroundFid = buildFid(6, 16, 0, 0, 0);
        Art* backgroundFrm = artLock(backgroundFid, &backgroundFrmHandle);
        if (backgroundFrm == NULL) {
            internal_free(gDisplayMonitorBackgroundFrmData);
            return -1;
        }

        unsigned char* backgroundFrmData = artGetFrameData(backgroundFrm, 0, 0);
        dword_56FB4C = artGetWidth(backgroundFrm, 0, 0);
        blitBufferToBuffer(backgroundFrmData + dword_56FB4C * DISPLAY_MONITOR_Y + DISPLAY_MONITOR_X,
            DISPLAY_MONITOR_WIDTH,
            DISPLAY_MONITOR_HEIGHT,
            dword_56FB4C,
            gDisplayMonitorBackgroundFrmData,
            DISPLAY_MONITOR_WIDTH);

        artUnlock(backgroundFrmHandle);

        gDisplayMonitorScrollUpButton = buttonCreate(gInterfaceBarWindow,
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
        if (gDisplayMonitorScrollUpButton != -1) {
            buttonSetMouseCallbacks(gDisplayMonitorScrollUpButton,
                displayMonitorScrollUpOnMouseEnter,
                displayMonitorOnMouseExit,
                displayMonitorScrollUpOnMouseDown,
                NULL);
        }

        gDisplayMonitorScrollDownButton = buttonCreate(gInterfaceBarWindow,
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
        if (gDisplayMonitorScrollDownButton != -1) {
            buttonSetMouseCallbacks(gDisplayMonitorScrollDownButton,
                displayMonitorScrollDownOnMouseEnter,
                displayMonitorOnMouseExit,
                displayMonitorScrollDownOnMouseDown,
                NULL);
        }

        gDisplayMonitorEnabled = true;
        gDisplayMonitorInitialized = true;

        for (int index = 0; index < gDisplayMonitorLinesCapacity; index++) {
            gDisplayMonitorLines[index][0] = '\0';
        }

        dword_56FB54 = 0;
        dword_56FB48 = 0;

        displayMonitorRefresh();
    }

    return 0;
}

// 0x431800
int displayMonitorReset()
{
    if (gDisplayMonitorInitialized) {
        for (int index = 0; index < gDisplayMonitorLinesCapacity; index++) {
            gDisplayMonitorLines[index][0] = '\0';
        }

        dword_56FB54 = 0;
        dword_56FB48 = 0;
        displayMonitorRefresh();
    }
    return 0;
}

// 0x43184C
void displayMonitorExit()
{
    if (gDisplayMonitorInitialized) {
        internal_free(gDisplayMonitorBackgroundFrmData);
        gDisplayMonitorInitialized = false;
    }
}

// 0x43186C
void displayMonitorAddMessage(char* str)
{
    if (!gDisplayMonitorInitialized) {
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
        if (getTicksBetween(now, gDisplayMonitorLastBeepTimestamp) >= DISPLAY_MONITOR_BEEP_DELAY) {
            gDisplayMonitorLastBeepTimestamp = now;
            soundPlayFile("monitor");
        }
    }

    // TODO: Refactor these two loops.
    char* v1 = NULL;
    while (true) {
        while (fontGetStringWidth(str) < DISPLAY_MONITOR_WIDTH - dword_56FB40 - knobWidth) {
            char* temp = gDisplayMonitorLines[dword_56FB54];
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
            gDisplayMonitorLines[dword_56FB54][DISPLAY_MONITOR_LINE_LENGTH - 1] = '\0';
            dword_56FB54 = (dword_56FB54 + 1) % gDisplayMonitorLinesCapacity;

            if (v1 == NULL) {
                fontSetCurrent(oldFont);
                dword_56FB48 = dword_56FB54;
                displayMonitorRefresh();
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

    char* temp = gDisplayMonitorLines[dword_56FB54];
    int length;
    if (knob != '\0') {
        temp++;
        gDisplayMonitorLines[dword_56FB54][0] = knob;
        length = DISPLAY_MONITOR_LINE_LENGTH - 2;
        knob = '\0';
    } else {
        length = DISPLAY_MONITOR_LINE_LENGTH - 1;
    }
    strncpy(temp, str, length);

    gDisplayMonitorLines[dword_56FB54][DISPLAY_MONITOR_LINE_LENGTH - 1] = '\0';
    dword_56FB54 = (dword_56FB54 + 1) % gDisplayMonitorLinesCapacity;

    fontSetCurrent(oldFont);
    dword_56FB48 = dword_56FB54;
    displayMonitorRefresh();
}

// 0x431A78
void displayMonitorRefresh()
{
    if (!gDisplayMonitorInitialized) {
        return;
    }

    unsigned char* buf = windowGetBuffer(gInterfaceBarWindow);
    if (buf == NULL) {
        return;
    }

    buf += dword_56FB4C * DISPLAY_MONITOR_Y + DISPLAY_MONITOR_X;
    blitBufferToBuffer(gDisplayMonitorBackgroundFrmData,
        DISPLAY_MONITOR_WIDTH,
        DISPLAY_MONITOR_HEIGHT,
        DISPLAY_MONITOR_WIDTH,
        buf,
        dword_56FB4C);

    int oldFont = fontGetCurrent();
    fontSetCurrent(DISPLAY_MONITOR_FONT);

    for (int index = 0; index < dword_56FB40; index++) {
        int stringIndex = (dword_56FB48 + gDisplayMonitorLinesCapacity + index - dword_56FB40) % gDisplayMonitorLinesCapacity;
        fontDrawText(buf + index * dword_56FB4C * fontGetLineHeight(), gDisplayMonitorLines[stringIndex], DISPLAY_MONITOR_WIDTH, dword_56FB4C, byte_6A38D0[992]);

        // Even though the display monitor is rectangular, it's graphic is not.
        // To give a feel of depth it's covered by some metal canopy and
        // considered inclined outwards. This way earlier messages appear a
        // little bit far from player's perspective. To implement this small
        // detail the destination buffer is incremented by 1.
        buf++;
    }

    windowRefreshRect(gInterfaceBarWindow, &gDisplayMonitorRect);
    fontSetCurrent(oldFont);
}

// 0x431B70
void displayMonitorScrollUpOnMouseDown(int btn, int keyCode)
{
    if ((gDisplayMonitorLinesCapacity + dword_56FB48 - 1) % gDisplayMonitorLinesCapacity != dword_56FB54) {
        dword_56FB48 = (gDisplayMonitorLinesCapacity + dword_56FB48 - 1) % gDisplayMonitorLinesCapacity;
        displayMonitorRefresh();
    }
}

// 0x431B9C
void displayMonitorScrollDownOnMouseDown(int btn, int keyCode)
{
    if (dword_56FB48 != dword_56FB54) {
        dword_56FB48 = (dword_56FB48 + 1) % gDisplayMonitorLinesCapacity;
        displayMonitorRefresh();
    }
}

// 0x431BC8
void displayMonitorScrollUpOnMouseEnter(int btn, int keyCode)
{
    gameMouseSetCursor(MOUSE_CURSOR_SMALL_ARROW_UP);
}

// 0x431BD4
void displayMonitorScrollDownOnMouseEnter(int btn, int keyCode)
{
    gameMouseSetCursor(MOUSE_CURSOR_SMALL_ARROW_DOWN);
}

// 0x431BE0
void displayMonitorOnMouseExit(int btn, int keyCode)
{
    gameMouseSetCursor(MOUSE_CURSOR_ARROW);
}

// 0x431BEC
void displayMonitorDisable()
{
    if (gDisplayMonitorEnabled) {
        buttonDisable(gDisplayMonitorScrollDownButton);
        buttonDisable(gDisplayMonitorScrollUpButton);
        gDisplayMonitorEnabled = false;
    }
}

// 0x431C14
void displayMonitorEnable()
{
    if (!gDisplayMonitorEnabled) {
        buttonEnable(gDisplayMonitorScrollDownButton);
        buttonEnable(gDisplayMonitorScrollUpButton);
        gDisplayMonitorEnabled = true;
    }
}
