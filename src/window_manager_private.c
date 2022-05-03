#include "window_manager_private.h"

#include "core.h"
#include "memory.h"
#include "text_font.h"
#include "window_manager.h"

#include <stdio.h>

// 0x51E414
int dword_51E414 = -1;

// 0x51E418
int dword_51E418 = 0;

// 0x51E41C
bool dword_51E41C = false;

// 0x6B2340
STRUCT_6B2340 stru_6B2340[5];

// 0x6B2368
int dword_6B2368;

// 0x6B236C
int dword_6B236C;

// 0x6B2370
STRUCT_6B2370 stru_6B2370[5];

// 0x6B23AC
int dword_6B23AC;

// 0x6B23B0
int dword_6B23B0;

// 0x6B23B4
int dword_6B23B4;

// 0x6B23B8
int dword_6B23B8;

// 0x6B23BC
int dword_6B23BC;

// x
//
// 0x6B23C0
int dword_6B23C0;

// y
//
// 0x6B23C4
int dword_6B23C4;

// 0x6B23D0
char gProgramWindowTitle[256];

// 0x4DC30C
int sub_4DC30C(char* a1)
{
    if (!gWindowSystemInitialized) {
        return -1;
    }

    // TODO: Incomplete.

    windowRefresh(dword_51E414);

    return 0;
}

// 0x4DC65C
void sub_4DC65C()
{
    windowDestroy(dword_51E414);
    dword_51E414 = -1;
}

// 0x4DC674
int sub_4DC674(int win, int x, int y, int width, int height, int a6, int a7)
{
    Window* window = windowGetWindow(win);

    if (!gWindowSystemInitialized) {
        return -1;
    }

    if (window == NULL) {
        return -1;
    }

    if (window->field_3C != NULL) {
        return -1;
    }

    int right = x + width;
    if (right > window->width) {
        return -1;
    }

    int bottom = y + height;
    if (bottom > window->height) {
        return -1;
    }

    struc_177* v14 = window->field_3C = internal_malloc(sizeof(struc_177));
    if (v14 == NULL) {
        return -1;
    }

    v14->win = win;
    v14->rect.left = x;
    v14->rect.top = y;
    v14->rect.right = right - 1;
    v14->rect.bottom = bottom - 1;
    v14->entriesCount = 0;
    v14->field_234 = a6;
    v14->field_238 = a7;

    windowFill(win, x, y, width, height, a7);
    windowDrawRect(win, x, y, right - 1, bottom - 1, a6);

    return 0;
}

// 0x4DC768
int sub_4DC768(int win, int x, char* str, int a4)
{
    Window* window = windowGetWindow(win);

    if (!gWindowSystemInitialized) {
        return -1;
    }

    if (window == NULL) {
        return -1;
    }

    struc_177* field_3C = window->field_3C;
    if (field_3C == NULL) {
        return -1;
    }

    if (window->field_3C->entriesCount == 15) {
        return -1;
    }

    int btn = buttonCreate(win,
        field_3C->rect.left + x,
        (field_3C->rect.top + field_3C->rect.bottom - fontGetLineHeight()) / 2,
        fontGetStringWidth(str),
        fontGetLineHeight(),
        -1,
        -1,
        a4,
        -1,
        NULL,
        NULL,
        NULL,
        0);
    if (btn == -1) {
        return -1;
    }

    // TODO: Incomplete.

    return 0;
}

// 0x4DC930
int sub_4DC930(struc_177* ptr, int i)
{
    // TODO: Incomplete

    return 0;
}

// Calculates max length of string needed to represent a1 or a2.
//
// 0x4DD03C
int sub_4DD03C(int a1, int a2)
{
    char* str = internal_malloc(17);
    if (str == NULL) {
        return -1;
    }

    sprintf(str, "%d", a1);
    int len1 = strlen(str);

    sprintf(str, "%d", a2);
    int len2 = strlen(str);

    internal_free(str);

    return max(len1, len2) + 1;
}

// 0x4DD3EC
void sub_4DD3EC()
{
    int v1, v2;
    int i;

    dword_6B23AC = 3000;
    dword_6B23BC = 0;
    dword_6B23B8 = -1;
    dword_6B23B0 = stru_6AC9F0.right / 2;

    if (stru_6AC9F0.bottom >= 479) {
        dword_6B23B4 = 16;
        dword_6B2368 = 16;
    } else {
        dword_6B23B4 = 10;
        dword_6B2368 = 10;
    }

    dword_6B236C = 2 * dword_6B23B4 + fontGetLineHeight();

    v1 = stru_6AC9F0.bottom >> 3;
    v2 = stru_6AC9F0.bottom >> 2;

    for (i = 0; i < 5; i++) {
        stru_6B2340[i].field_4 = v1 * i + v2;
        stru_6B2340[i].field_0 = 0;
    }
}

// 0x4DD4A4
void sub_4DD4A4()
{
    tickersRemove(sub_4DD66C);
    while (dword_6B23B8 != -1) {
        sub_4DD6C0();
    }
}

// 0x4DD66C
void sub_4DD66C()
{
    if (dword_51E41C) {
        return;
    }

    dword_51E41C = 1;
    while (dword_6B23B8 != -1) {
        if (getTicksSince(stru_6B2370[dword_6B23B8].field_0) < dword_6B23AC) {
            break;
        }

        sub_4DD6C0();
    }
    dword_51E41C = 0;
}

// 0x4DD6C0
void sub_4DD6C0()
{
    int v0;

    v0 = dword_6B23B8;
    if (v0 != -1) {
        windowDestroy(stru_6B2370[dword_6B23B8].field_4);
        stru_6B2340[stru_6B2370[dword_6B23B8].field_8].field_0 = 0;

        if (v0 == 5) {
            v0 = 0;
        }

        if (v0 == dword_6B23BC) {
            dword_6B23BC = 0;
            dword_6B23B8 = -1;
            tickersRemove(sub_4DD66C);
            v0 = dword_6B23B8;
        }
    }

    dword_6B23B8 = v0;
}

// 0x4DD744
void sub_4DD744(int a1)
{
    int v7;
    int v6;

    if (dword_6B23B8 == -1) {
        return;
    }

    if (!sub_4DD870(a1)) {
        return;
    }

    windowDestroy(stru_6B2370[a1].field_4);

    stru_6B2340[stru_6B2370[a1].field_8].field_0 = 0;

    if (a1 != dword_6B23B8) {
        v6 = a1;
        do {
            v7 = v6 - 1;
            if (v7 < 0) {
                v7 = 4;
            }

            memcpy(&(stru_6B2370[v6]), &(stru_6B2370[v7]), sizeof(STRUCT_6B2370));
            v6 = v7;
        } while (v7 != dword_6B23B8);
    }

    if (++dword_6B23B8 == 5) {
        dword_6B23B8 = 0;
    }

    if (dword_6B23BC == dword_6B23B8) {
        dword_6B23BC = 0;
        dword_6B23B8 = -1;
        tickersRemove(sub_4DD66C);
    }
}

// 0x4DD82C
void sub_4DD82C(int btn)
{
    int win;
    int v3;

    if (dword_6B23B8 == -1) {
        return;
    }

    win = buttonGetWindowId(btn);
    v3 = dword_6B23B8;
    while (win != stru_6B2370[v3].field_4) {
        v3++;
        if (v3 == 5) {
            v3 = 0;
        }

        if (v3 == dword_6B23B8 || !sub_4DD870(v3))
            return;
    }

    sub_4DD744(v3);
}

// 0x4DD870
int sub_4DD870(int a1)
{
    if (dword_6B23B8 != dword_6B23BC) {
        if (dword_6B23B8 >= dword_6B23BC) {
            if (a1 >= dword_6B23BC && a1 < dword_6B23B8)
                return 0;
        } else if (a1 < dword_6B23B8 || a1 >= dword_6B23BC) {
            return 0;
        }
    }
    return 1;
}
