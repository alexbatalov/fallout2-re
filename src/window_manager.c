#include "window_manager.h"

#include "color.h"
#include "core.h"
#include "db.h"
#include "debug.h"
#include "draw.h"
#include "memory.h"
#include "palette.h"
#include "text_font.h"
#include "window_manager_private.h"

#include <intrin.h>

static_assert(sizeof(struc_177) == 572, "wrong size");
static_assert(sizeof(Window) == 68, "wrong size");
static_assert(sizeof(Button) == 124, "wrong size");

// 0x50FA30
char byte_50FA30[] = "";

// 0x51E3D8
bool dword_51E3D8 = false;

// 0x51E3DC
HANDLE dword_51E3DC = INVALID_HANDLE_VALUE;

// 0x51E3E0
bool gWindowSystemInitialized = false;

// 0x51E3E4
int dword_51E3E4[6] = {
    0,
    0,
    0,
    0,
    0,
    0,
};

// 0x51E3FC
unsigned char* off_51E3FC = NULL;

// 0x51E400
bool dword_51E400 = false;

// 0x51E404
int dword_51E404 = -1;

// 0x6ADD90
int gOrderedWindowIds[MAX_WINDOW_COUNT];

// 0x6ADE58
Window* gWindows[MAX_WINDOW_COUNT];

// 0x6ADF20
VideoSystemExitProc* gVideoSystemExitProc;

// 0x6ADF24
int gWindowsLength;

// 0x6ADF28
int dword_6ADF28;

// 0x6ADF2C
bool dword_6ADF2C;

// 0x6ADF30
int dword_6ADF30;

// 0x6ADF34
VideoSystemInitProc* gVideoSystemInitProc;

// 0x6ADF38
int dword_6ADF38;

// 0x6ADF3C
void* off_6ADF3C;

// 0x6ADF40
RadioGroup gRadioGroups[RADIO_GROUP_LIST_CAPACITY];

// 0x4D5C30
int windowManagerInit(VideoSystemInitProc* videoSystemInitProc, VideoSystemExitProc* videoSystemExitProc, int a3)
{
    CloseHandle(dword_51E448);
    dword_51E448 = INVALID_HANDLE_VALUE;

    if (dword_51E3D8) {
        return WINDOW_MANAGER_ERR_ALREADY_RUNNING;
    }

    if (dword_51E3DC == INVALID_HANDLE_VALUE) {
        return WINDOW_MANAGER_ERR_TITLE_NOT_SET;
    }

    if (gWindowSystemInitialized) {
        return WINDOW_MANAGER_ERR_WINDOW_SYSTEM_ALREADY_INITIALIZED;
    }

    __stosd((unsigned long*)gOrderedWindowIds, -1, MAX_WINDOW_COUNT);

    if (!sub_4C5D58()) {
        if (dbOpen(NULL, 0, byte_50FA30, 1) == -1) {
            return WINDOW_MANAGER_ERR_INITIALIZING_DEFAULT_DATABASE;
        }
    }

    if (textFontsInit() == -1) {
        return WINDOW_MANAGER_ERR_INITIALIZING_TEXT_FONTS;
    }

    sub_4CADF8();

    gVideoSystemInitProc = videoSystemInitProc;
    gVideoSystemExitProc = directInputFree;

    int rc = videoSystemInitProc();
    if (rc == -1) {
        if (gVideoSystemExitProc != NULL) {
            gVideoSystemExitProc();
        }

        return WINDOW_MANAGER_ERR_INITIALIZING_VIDEO_MODE;
    }

    if (rc == 8) {
        return WINDOW_MANAGER_ERR_8;
    }

    if (a3 & 1) {
        off_51E3FC = internal_malloc((stru_6AC9F0.bottom - stru_6AC9F0.top + 1) * (stru_6AC9F0.right - stru_6AC9F0.left + 1));
        if (off_51E3FC == NULL) {
            if (gVideoSystemExitProc != NULL) {
                gVideoSystemExitProc();
            } else {
                directDrawFree();
            }

            return WINDOW_MANAGER_ERR_NO_MEMORY;
        }
    }

    dword_6ADF2C = false;
    dword_6ADF38 = 0;

    colorPaletteSetFileIO(paletteOpenFileImpl, paletteReadFileImpl, paletteCloseFileImpl);
    colorPaletteSetMemoryProcs(internal_malloc, internal_realloc, internal_free);

    if (!sub_4C89CC()) {
        unsigned char* palette = internal_malloc(768);
        if (palette == NULL) {
            if (gVideoSystemExitProc != NULL) {
                gVideoSystemExitProc();
            } else {
                directDrawFree();
            }

            if (off_51E3FC != NULL) {
                internal_free(off_51E3FC);
            }

            return WINDOW_MANAGER_ERR_NO_MEMORY;
        }

        bufferFill(palette, 768, 1, 768, 0);

        // TODO: Incomplete.
        // sub_4C7F28(sub_4C7420(), palette);

        internal_free(palette);
    }

    sub_4C6CD0();

    if (coreInit(a3) == -1) {
        return WINDOW_MANAGER_ERR_INITIALIZING_INPUT;
    }

    sub_4DD3EC();

    Window* window = gWindows[0] = internal_malloc(sizeof(*window));
    if (window == NULL) {
        if (gVideoSystemExitProc != NULL) {
            gVideoSystemExitProc();
        } else {
            directDrawFree();
        }

        if (off_51E3FC != NULL) {
            internal_free(off_51E3FC);
        }

        return WINDOW_MANAGER_ERR_NO_MEMORY;
    }

    window->id = 0;
    window->flags = 0;
    window->rect.left = stru_6AC9F0.left;
    window->rect.top = stru_6AC9F0.top;
    window->rect.right = stru_6AC9F0.right;
    window->rect.bottom = stru_6AC9F0.bottom;
    window->width = stru_6AC9F0.right - stru_6AC9F0.left + 1;
    window->height = stru_6AC9F0.bottom - stru_6AC9F0.top + 1;
    window->field_24 = 0;
    window->field_28 = 0;
    window->buffer = NULL;
    window->buttonListHead = NULL;
    window->field_34 = NULL;
    window->field_38 = 0;
    window->field_3C = 0;

    gWindowsLength = 1;
    gWindowSystemInitialized = 1;
    dword_51E3E4[3] = 21140;
    dword_51E3E4[4] = 32747;
    dword_51E3E4[5] = 31744;
    gOrderedWindowIds[0] = 0;
    off_6ADF3C = NULL;
    dword_6ADF30 = 0;
    dword_51E3E4[0] = 10570;
    dword_6ADF28 = a3;
    dword_51E3E4[2] = 8456;
    dword_51E3E4[1] = 15855;

    atexit(windowManagerExit);

    return WINDOW_MANAGER_OK;
}

// 0x4D616C
void windowManagerExit(void)
{
    if (!dword_51E400) {
        dword_51E400 = true;
        if (gWindowSystemInitialized) {
            sub_4DD4A4();

            for (int index = gWindowsLength - 1; index >= 0; index--) {
                windowFree(gWindows[index]->id);
            }

            if (off_6ADF3C != NULL) {
                internal_free(off_6ADF3C);
            }

            if (off_51E3FC != NULL) {
                internal_free(off_51E3FC);
            }

            if (gVideoSystemExitProc != NULL) {
                gVideoSystemExitProc();
            }

            coreExit();
            sub_4C6900();
            textFontsExit();
            sub_4C8A18();

            gWindowSystemInitialized = false;

            CloseHandle(dword_51E3DC);
            dword_51E3DC = INVALID_HANDLE_VALUE;
        }
        dword_51E400 = false;
    }
}

// win_add
// 0x4D6238
int windowCreate(int x, int y, int width, int height, int a4, int flags)
{
    int v23;
    int v25, v26;
    Window* tmp;

    if (!gWindowSystemInitialized) {
        return -1;
    }

    if (gWindowsLength == MAX_WINDOW_COUNT) {
        return -1;
    }

    if (width > rectGetWidth(&stru_6AC9F0)) {
        return -1;
    }

    if (height > rectGetHeight(&stru_6AC9F0)) {
        return -1;
    }

    Window* window = gWindows[gWindowsLength] = internal_malloc(sizeof(*window));
    if (window == NULL) {
        return -1;
    }

    window->buffer = internal_malloc(width * height);
    if (window->buffer == NULL) {
        internal_free(window);
        return -1;
    }

    int index = 1;
    while (windowGetWindow(index) != NULL) {
        index++;
    }

    window->id = index;

    if ((flags & WINDOW_FLAG_0x01) != 0) {
        flags |= dword_6ADF28;
    }

    window->width = width;
    window->height = height;
    window->flags = flags;
    window->field_24 = rand() & 0xFFFE;
    window->field_28 = rand() & 0xFFFE;

    if (a4 == 256) {
        if (off_6ADF3C == NULL) {
            a4 = byte_6A38D0[dword_51E3E4[0]];
        }
    } else if ((a4 & 0xFF00) != 0) {
        int v1 = (a4 & 0xFF00) >> 8;
        a4 = (a4 & ~0xFFFF) | byte_6A38D0[dword_51E3E4[v1]];
    }

    window->buttonListHead = 0;
    window->field_34 = 0;
    window->field_38 = 0;
    window->field_3C = 0;
    window->blitProc = blitBufferToBufferTrans;
    window->field_20 = a4;
    gOrderedWindowIds[index] = gWindowsLength;
    gWindowsLength++;

    windowFill(index, 0, 0, width, height, a4);

    window->flags |= WINDOW_HIDDEN;
    sub_4D6EA0(index, x, y);
    window->flags = flags;

    if ((flags & WINDOW_FLAG_0x04) == 0) {
        v23 = gWindowsLength - 2;
        while (v23 > 0) {
            if (!(gWindows[v23]->flags & WINDOW_FLAG_0x04)) {
                break;
            }
            v23--;
        }

        if (v23 != gWindowsLength - 2) {
            v25 = v23 + 1;
            v26 = gWindowsLength - 1;
            while (v26 > v25) {
                tmp = gWindows[v26 - 1];
                gWindows[v26] = tmp;
                gOrderedWindowIds[tmp->id] = v26;
                v26--;
            }

            gWindows[v25] = window;
            gOrderedWindowIds[index] = v25;
        }
    }

    return index;
}

// win_remove
// 0x4D6468
void windowDestroy(int win)
{
    Window* window = windowGetWindow(win);

    if (!gWindowSystemInitialized) {
        return;
    }

    if (window == NULL) {
        return;
    }

    Rect rect;
    rectCopy(&rect, &(window->rect));

    int v1 = gOrderedWindowIds[window->id];
    windowFree(win);

    gOrderedWindowIds[win] = -1;

    for (int index = v1; index < gWindowsLength - 1; index++) {
        gWindows[index] = gWindows[index + 1];
        gOrderedWindowIds[gWindows[index]->id] = index;
    }

    gWindowsLength--;

    // NOTE: Uninline.
    windowRefreshAll(&rect);
}

// 0x4D650C
void windowFree(int win)
{
    Window* window = windowGetWindow(win);
    if (window == NULL) {
        return;
    }

    if (window->buffer != NULL) {
        internal_free(window->buffer);
    }

    if (window->field_3C != NULL) {
        internal_free(window->field_3C);
    }

    Button* curr = window->buttonListHead;
    while (curr != NULL) {
        Button* next = curr->next;
        buttonFree(curr);
        curr = next;
    }

    internal_free(window);
}

// 0x4D6558
void sub_4D6558(bool a1)
{
    if (off_51E3FC != NULL) {
        dword_6ADF2C = a1;
    }
}

// 0x4D6568
void windowDrawBorder(int win)
{
    if (!gWindowSystemInitialized) {
        return;
    }

    Window* window = windowGetWindow(win);
    if (window == NULL) {
        return;
    }

    sub_4D3A48(window->buffer + 5, window->width - 10, 5, window->width);
    sub_4D3A48(window->buffer, 5, window->height, window->width);
    sub_4D3A48(window->buffer + window->width - 5, 5, window->height, window->width);
    sub_4D3A48(window->buffer + window->width * (window->height - 5) + 5, window->width - 10, 5, window->width);

    bufferDrawRect(window->buffer, window->width, 0, 0, window->width - 1, window->height - 1, byte_6A38D0[0]);

    bufferDrawRectShadowed(window->buffer, window->width, 1, 1, window->width - 2, window->height - 2, byte_6A38D0[dword_51E3E4[1]], byte_6A38D0[dword_51E3E4[2]]);
    bufferDrawRectShadowed(window->buffer, window->width, 5, 5, window->width - 6, window->height - 6, byte_6A38D0[dword_51E3E4[2]], byte_6A38D0[dword_51E3E4[1]]);
}

// 0x4D684C
void windowDrawText(int win, char* str, int a3, int x, int y, int a6)
{
    int v7;
    int v14;
    unsigned char* buf;
    int v27;

    Window* window = windowGetWindow(win);
    v7 = a3;

    if (!gWindowSystemInitialized) {
        return;
    }

    if (window == NULL) {
        return;
    }

    if (a3 == 0) {
        if (a6 & 0x040000) {
            v7 = fontGetMonospacedStringWidth(str);
        } else {
            v7 = fontGetStringWidth(str);
        }
    }

    if (v7 + x > window->width) {
        if (!(a6 & 0x04000000)) {
            return;
        }

        v7 = window->width - x;
    }

    buf = window->buffer + x + y * window->width;

    v14 = fontGetLineHeight();
    if (v14 + y > window->height) {
        return;
    }

    if (!(a6 & 0x02000000)) {
        if (window->field_20 == 256 && off_6ADF3C != NULL) {
            sub_4D38E0(buf, v7, fontGetLineHeight(), window->width, off_6ADF3C, window->field_24 + x, window->field_28 + y);
        } else {
            bufferFill(buf, v7, fontGetLineHeight(), window->width, window->field_20);
        }
    }

    if (a6 & 0xFF00) {
        int t = (a6 & 0xFF00) >> 8;
        v27 = (a6 & ~0xFFFF) | byte_6A38D0[dword_51E3E4[t]];
    } else {
        v27 = a6;
    }

    fontDrawText(buf, str, v7, window->width, v27);

    if (a6 & 0x01000000) {
        // TODO: Check.
        Rect rect;
        rect.left = window->rect.left + x;
        rect.top = window->rect.top + y;
        rect.right = rect.left + v7;
        rect.bottom = rect.top + fontGetLineHeight();
        sub_4D6FD8(window, &rect, NULL);
    }
}

// 0x4D6B24
void windowDrawLine(int win, int left, int top, int right, int bottom, int color)
{
    Window* window = windowGetWindow(win);

    if (!gWindowSystemInitialized) {
        return;
    }

    if (window == NULL) {
        return;
    }

    if (color & 0xFF00) {
        int t = (color & 0xFF00) >> 8;
        color = (color & ~0xFFFF) | byte_6A38D0[dword_51E3E4[t]];
    }

    bufferDrawLine(window->buffer, window->width, left, top, right, bottom, color);
}

// 0x4D6B88
void windowDrawRect(int win, int left, int top, int right, int bottom, int color)
{
    Window* window = windowGetWindow(win);

    if (!gWindowSystemInitialized) {
        return;
    }

    if (window == NULL) {
        return;
    }

    if ((color & 0xFF00) != 0) {
        int v1 = (color & 0xFF00) >> 8;
        color = (color & ~0xFFFF) | byte_6A38D0[dword_51E3E4[v1]];
    }

    if (right < left) {
        int tmp = left;
        left = right;
        right = tmp;
    }

    if (bottom < top) {
        int tmp = top;
        top = bottom;
        bottom = tmp;
    }

    bufferDrawRect(window->buffer, window->width, left, top, right, bottom, color);
}

// 0x4D6CC8
void windowFill(int win, int x, int y, int width, int height, int a6)
{
    Window* window = windowGetWindow(win);

    if (!gWindowSystemInitialized) {
        return;
    }

    if (window == NULL) {
        return;
    }

    if (a6 == 256) {
        if (off_6ADF3C != NULL) {
            sub_4D38E0(window->buffer + window->width * y + x, width, height, window->width, off_6ADF3C, x + window->field_24, y + window->field_28);
        } else {
            a6 = byte_6A38D0[dword_51E3E4[0]] & 0xFF;
        }
    } else if ((a6 & 0xFF00) != 0) {
        int v1 = (a6 & 0xFF00) >> 8;
        a6 = (a6 & ~0xFFFF) | byte_6A38D0[dword_51E3E4[v1]];
    }

    if (a6 < 256) {
        bufferFill(window->buffer + window->width * y + x, width, height, window->width, a6);
    }
}

// 0x4D6DAC
void windowUnhide(int win)
{
    Window* window;
    int v3;
    int v5;
    int v7;
    Window* v6;

    window = windowGetWindow(win);
    v3 = gOrderedWindowIds[window->id];

    if (!gWindowSystemInitialized) {
        return;
    }

    if (window->flags & WINDOW_HIDDEN) {
        window->flags &= ~WINDOW_HIDDEN;
        if (v3 == gWindowsLength - 1) {
            sub_4D6FD8(window, &(window->rect), NULL);
        }
    }

    v5 = gWindowsLength - 1;
    if (v3 < v5 && !(window->flags & WINDOW_FLAG_0x02)) {
        v7 = v3;
        while (v3 < v5 && ((window->flags & WINDOW_FLAG_0x04) || !(gWindows[v7 + 1]->flags & WINDOW_FLAG_0x04))) {
            v6 = gWindows[v7 + 1];
            gWindows[v7] = v6;
            v7++;
            gOrderedWindowIds[v6->id] = v3++;
        }

        gWindows[v3] = window;
        gOrderedWindowIds[window->id] = v3;
        sub_4D6FD8(window, &(window->rect), NULL);
    }
}

// 0x4D6E64
void windowHide(int win)
{
    if (!gWindowSystemInitialized) {
        return;
    }

    Window* window = windowGetWindow(win);
    if (window == NULL) {
        return;
    }

    if ((window->flags & WINDOW_HIDDEN) == 0) {
        window->flags |= WINDOW_HIDDEN;
        sub_4D7814(&(window->rect), NULL);
    }
}

// 0x4D6EA0
void sub_4D6EA0(int win, int x, int y)
{
    Window* window = windowGetWindow(win);

    if (!gWindowSystemInitialized) {
        return;
    }

    if (window == NULL) {
        return;
    }

    Rect rect;
    rectCopy(&rect, &(window->rect));

    if (x < 0) {
        x = 0;
    }

    if (y < 0) {
        y = 0;
    }

    if ((window->flags & WINDOW_FLAG_0x0100) != 0) {
        x += 2;
    }

    if (x + window->width - 1 > stru_6AC9F0.right) {
        x = stru_6AC9F0.right - window->width + 1;
    }

    if (y + window->height - 1 > stru_6AC9F0.bottom) {
        y = stru_6AC9F0.bottom - window->height + 1;
    }

    if ((window->flags & WINDOW_FLAG_0x0100) != 0) {
        // TODO: Not sure what this means.
        x &= ~0x03;
    }

    window->rect.left = x;
    window->rect.top = y;
    window->rect.right = window->width + x - 1;
    window->rect.bottom = window->height + y - 1;

    if ((window->flags & WINDOW_HIDDEN) == 0) {
        sub_4D6FD8(window, &(window->rect), NULL);

        if (gWindowSystemInitialized) {
            sub_4D7814(&rect, NULL);
        }
    }
}

// 0x4D6F5C
void windowRefresh(int win)
{
    Window* window = windowGetWindow(win);

    if (!gWindowSystemInitialized) {
        return;
    }

    if (window == NULL) {
        return;
    }

    sub_4D6FD8(window, &(window->rect), NULL);
}

// 0x4D6F80
void windowRefreshRect(int win, const Rect* rect)
{
    Window* window = windowGetWindow(win);

    if (!gWindowSystemInitialized) {
        return;
    }

    if (window == NULL) {
        return;
    }

    Rect newRect;
    rectCopy(&newRect, rect);
    rectOffset(&newRect, window->rect.left, window->rect.top);

    sub_4D6FD8(window, &newRect, NULL);
}

// 0x4D6FD8
void sub_4D6FD8(Window* window, Rect* rect, unsigned char* a3)
{
    RectListNode *v26, *v20, *v23, *v24;
    int dest_pitch;

    // TODO: Get rid of this.
    dest_pitch = 0;

    if ((window->flags & WINDOW_HIDDEN) != 0) {
        return;
    }

    if ((window->flags & WINDOW_FLAG_0x20) && dword_6ADF2C && !dword_6ADF38) {
        // TODO: Incomplete.
    } else {
        v26 = sub_4C6BB8();
        if (v26 == NULL) {
            return;
        }

        v26->next = NULL;

        v26->rect.left = max(window->rect.left, rect->left);
        v26->rect.top = max(window->rect.top, rect->top);
        v26->rect.right = min(window->rect.right, rect->right);
        v26->rect.bottom = min(window->rect.bottom, rect->bottom);

        if (v26->rect.right >= v26->rect.left && v26->rect.bottom >= v26->rect.top) {
            if (a3) {
                dest_pitch = rect->right - rect->left + 1;
            }

            sub_4D75B0(window, &v26, a3);

            if (window->id) {
                v20 = v26;
                while (v20) {
                    sub_4D9A58(window, &(v20->rect));

                    if (a3) {
                        if (dword_6ADF2C && (window->flags & WINDOW_FLAG_0x20)) {
                            window->blitProc(window->buffer + v20->rect.left - window->rect.left + (v20->rect.top - window->rect.top) * window->width,
                                v20->rect.right - v20->rect.left + 1,
                                v20->rect.bottom - v20->rect.top + 1,
                                window->width,
                                a3 + dest_pitch * (v20->rect.top - rect->top) + v20->rect.left - rect->left,
                                dest_pitch);
                        } else {
                            blitBufferToBuffer(
                                window->buffer + v20->rect.left - window->rect.left + (v20->rect.top - window->rect.top) * window->width,
                                v20->rect.right - v20->rect.left + 1,
                                v20->rect.bottom - v20->rect.top + 1,
                                window->width,
                                a3 + dest_pitch * (v20->rect.top - rect->top) + v20->rect.left - rect->left,
                                dest_pitch);
                        }
                    } else {
                        if (dword_6ADF2C) {
                            if (window->flags & WINDOW_FLAG_0x20) {
                                window->blitProc(
                                    window->buffer + v20->rect.left - window->rect.left + (v20->rect.top - window->rect.top) * window->width,
                                    v20->rect.right - v20->rect.left + 1,
                                    v20->rect.bottom - v20->rect.top + 1,
                                    window->width,
                                    off_51E3FC + v20->rect.top * (stru_6AC9F0.right - stru_6AC9F0.left + 1) + v20->rect.left,
                                    stru_6AC9F0.right - stru_6AC9F0.left + 1);
                            } else {
                                blitBufferToBuffer(
                                    window->buffer + v20->rect.left - window->rect.left + (v20->rect.top - window->rect.top) * window->width,
                                    v20->rect.right - v20->rect.left + 1,
                                    v20->rect.bottom - v20->rect.top + 1,
                                    window->width,
                                    off_51E3FC + v20->rect.top * (stru_6AC9F0.right - stru_6AC9F0.left + 1) + v20->rect.left,
                                    stru_6AC9F0.right - stru_6AC9F0.left + 1);
                            }
                        } else {
                            off_6ACA18(
                                window->buffer + v20->rect.left - window->rect.left + (v20->rect.top - window->rect.top) * window->width,
                                window->width,
                                window->rect.bottom - v20->rect.bottom + 1,
                                0,
                                0,
                                v20->rect.right - v20->rect.left + 1,
                                v20->rect.bottom - v20->rect.top + 1,
                                v20->rect.left,
                                v20->rect.top);
                        }
                    }

                    v20 = v20->next;
                }
            } else {
                RectListNode* v16 = v26;
                while (v16 != NULL) {
                    int width = v16->rect.right - v16->rect.left + 1;
                    int height = v16->rect.bottom - v16->rect.top + 1;
                    unsigned char* buf = internal_malloc(width * height);
                    if (buf != NULL) {
                        bufferFill(buf, width, height, width, dword_6ADF30);
                        if (dest_pitch != 0) {
                            blitBufferToBuffer(
                                buf,
                                width,
                                height,
                                width,
                                a3 + dest_pitch * (v16->rect.top - rect->top) + v16->rect.left - rect->left,
                                dest_pitch);
                        } else {
                            if (dword_6ADF2C) {
                                blitBufferToBuffer(buf,
                                    width,
                                    height,
                                    width,
                                    off_51E3FC + v16->rect.top * (stru_6AC9F0.right - stru_6AC9F0.left + 1) + v16->rect.left,
                                    stru_6AC9F0.right - stru_6AC9F0.left + 1);
                            } else {
                                off_6ACA18(buf, width, height, 0, 0, width, height, v16->rect.left, v16->rect.top);
                            }
                        }

                        internal_free(buf);
                    }
                    v16 = v16->next;
                }
            }

            v23 = v26;
            while (v23) {
                v24 = v23->next;

                if (dword_6ADF2C && !a3) {
                    off_6ACA18(
                        off_51E3FC + v23->rect.left + (stru_6AC9F0.right - stru_6AC9F0.left + 1) * v23->rect.top,
                        stru_6AC9F0.right - stru_6AC9F0.left + 1,
                        v23->rect.bottom - v23->rect.top + 1,
                        0,
                        0,
                        v23->rect.right - v23->rect.left + 1,
                        v23->rect.bottom - v23->rect.top + 1,
                        v23->rect.left,
                        v23->rect.top);
                }

                sub_4C6C04(v23);

                v23 = v24;
            }

            if (!dword_6ADF38 && a3 == NULL && cursorIsHidden() == 0) {
                if (sub_4CA8C8(rect->left, rect->top, rect->right, rect->bottom)) {
                    mouseShowCursor();
                }
            }
        } else {
            sub_4C6C04(v26);
        }
    }
}

// 0x4D759C
void windowRefreshAll(Rect* rect)
{
    if (gWindowSystemInitialized) {
        sub_4D7814(rect, NULL);
    }
}

// 0x4D75B0
void sub_4D75B0(Window* window, RectListNode** rectListNodePtr, unsigned char* a3)
{
    int win;

    for (win = gOrderedWindowIds[window->id] + 1; win < gWindowsLength; win++) {
        if (*rectListNodePtr == NULL) {
            break;
        }

        Window* window = gWindows[win];
        if (!(window->flags & WINDOW_HIDDEN)) {
            if (!dword_6ADF2C || !(window->flags & WINDOW_FLAG_0x20)) {
                sub_4C6924(rectListNodePtr, &(window->rect));
            }

            if (!dword_6ADF38) {
                sub_4D6FD8(window, &(window->rect), NULL);
                sub_4C6924(rectListNodePtr, &(window->rect));
            }
        }
    }

    if (a3 == off_51E3FC || a3 == NULL) {
        if (cursorIsHidden() == 0) {
            Rect rect;
            mouseGetRect(&rect);
            sub_4C6924(rectListNodePtr, &rect);
        }
    }
}

// 0x4D765C
void sub_4D765C(int win)
{
    // TODO: Probably somehow related to self-run functionality, skip for now.
    Window* window = windowGetWindow(win);

    if (!gWindowSystemInitialized) {
        return;
    }

    if (window == NULL) {
        return;
    }

    windowUnhide(win);

    Rect rect;
    rectCopy(&rect, &(window->rect));

    tickersExecute();

    if (sub_4D2930() != 3) {
        sub_4CA59C();
    }

    if ((window->flags & WINDOW_FLAG_0x0100) && (window->rect.left & 3)) {
        sub_4D6EA0(window->id, window->rect.left, window->rect.top);
    }
}

// 0x4D77F8
void sub_4D77F8(unsigned char* a1)
{
    Rect rect;
    mouseGetRect(&rect);
    sub_4D7814(&rect, a1);
}

// 0x4D7814
void sub_4D7814(Rect* rect, unsigned char* a2)
{
    dword_6ADF38 = 1;

    for (int index = 0; index < gWindowsLength; index++) {
        sub_4D6FD8(gWindows[index], rect, a2);
    }

    dword_6ADF38 = 0;

    if (a2 == NULL) {
        if (!cursorIsHidden()) {
            if (sub_4CA8C8(rect->left, rect->top, rect->right, rect->bottom)) {
                mouseShowCursor();
            }
        }
    }
}

// 0x4D7888
Window* windowGetWindow(int win)
{
    int v0;

    if (win == -1) {
        return NULL;
    }

    v0 = gOrderedWindowIds[win];
    if (v0 == -1) {
        return NULL;
    }

    return gWindows[v0];
}

// win_get_buf
// 0x4D78B0
unsigned char* windowGetBuffer(int win)
{
    Window* window = windowGetWindow(win);

    if (!gWindowSystemInitialized) {
        return NULL;
    }

    if (window == NULL) {
        return NULL;
    }

    return window->buffer;
}

// 0x4D78CC
int windowGetAtPoint(int x, int y)
{
    for (int index = gWindowsLength - 1; index >= 0; index--) {
        Window* window = gWindows[index];
        if (x >= window->rect.left && x <= window->rect.right
            && y >= window->rect.top && y <= window->rect.bottom) {
            return window->id;
        }
    }

    return -1;
}

// 0x4D7918
int windowGetWidth(int win)
{
    Window* window = windowGetWindow(win);

    if (!gWindowSystemInitialized) {
        return -1;
    }

    if (window == NULL) {
        return -1;
    }

    return window->width;
}

// 0x4D7934
int windowGetHeight(int win)
{
    Window* window = windowGetWindow(win);

    if (!gWindowSystemInitialized) {
        return -1;
    }

    if (window == NULL) {
        return -1;
    }

    return window->height;
}

// win_get_rect
// 0x4D7950
int windowGetRect(int win, Rect* rect)
{
    Window* window = windowGetWindow(win);

    if (!gWindowSystemInitialized) {
        return -1;
    }

    if (window == NULL) {
        return -1;
    }

    rectCopy(rect, &(window->rect));

    return 0;
}

// 0x4D797C
int sub_4D797C()
{
    if (!gWindowSystemInitialized) {
        return -1;
    }

    int v1 = -1;
    for (int index = gWindowsLength - 1; index >= 1; index--) {
        if (sub_4D8A10(gWindows[index], &v1) == 0) {
            break;
        }

        if ((gWindows[index]->flags & WINDOW_FLAG_0x10) != 0) {
            break;
        }
    }

    return v1;
}

// 0x4D79DC
Button* buttonGetButton(int btn, Window** windowPtr)
{
    for (int index = 0; index < gWindowsLength; index++) {
        Window* window = gWindows[index];
        Button* button = window->buttonListHead;
        while (button != NULL) {
            if (button->id == btn) {
                if (windowPtr != NULL) {
                    *windowPtr = window;
                }

                return button;
            }
            button = button->next;
        }
    }

    return NULL;
}

// 0x4D7A34
int sub_4D7A34(int a1)
{
    if (!gWindowSystemInitialized) {
        return -1;
    }

    int v1 = a1;
    for (int index = gWindowsLength - 1; index >= 1; index--) {
        Window* window = gWindows[index];
        if (window->field_3C != NULL) {
            for (int v2 = 0; v2 < window->field_3C->entriesCount; v2++) {
                if (v1 == window->field_3C->entries[v2].field_10) {
                    v1 = sub_4DC930(window->field_3C, v2);
                    break;
                }
            }
        }

        if ((window->flags & 0x10) != 0) {
            break;
        }
    }

    return v1;
}

// 0x4D80D8
void programWindowSetTitle(const char* title)
{
    if (title == NULL) {
        return;
    }

    if (dword_51E3DC == INVALID_HANDLE_VALUE) {
        dword_51E3DC = CreateMutexA(NULL, TRUE, title);
        if (GetLastError() != ERROR_SUCCESS) {
            dword_51E3D8 = true;
            return;
        }
    }

    strncpy(gProgramWindowTitle, title, 256);
    gProgramWindowTitle[256 - 1] = '\0';

    if (gProgramWindow != NULL) {
        SetWindowTextA(gProgramWindow, gProgramWindowTitle);
    }
}

// [open] implementation for palette operations backed by [XFile].
//
// 0x4D8174
int paletteOpenFileImpl(const char* path, int flags)
{
    char mode[4];
    memset(mode, 0, sizeof(mode));

    if ((flags & 0x01) != 0) {
        mode[0] = 'w';
    } else if ((flags & 0x10) != 0) {
        mode[0] = 'a';
    } else {
        mode[0] = 'r';
    }

    if ((flags & 0x100) != 0) {
        mode[1] = 't';
    } else if ((flags & 0x200) != 0) {
        mode[1] = 'b';
    }

    File* stream = fileOpen(path, mode);
    if (stream != NULL) {
        return (int)stream;
    }

    return -1;
}

// [read] implementation for palette file operations backed by [XFile].
//
// 0x4D81E8
int paletteReadFileImpl(int fd, void* buf, size_t count)
{
    return fileRead(buf, 1, count, (File*)fd);
}

// [close] implementation for palette file operations backed by [XFile].
//
// 0x4D81E0
int paletteCloseFileImpl(int fd)
{
    return fileClose((File*)fd);
}

// 0x4D8200
bool showMesageBox(const char* text)
{
    HCURSOR cursor = LoadCursorA(gInstance, MAKEINTRESOURCEA(IDC_ARROW));
    HCURSOR prev = SetCursor(cursor);
    ShowCursor(TRUE);
    MessageBoxA(NULL, text, NULL, MB_ICONSTOP);
    ShowCursor(FALSE);
    SetCursor(prev);
    return true;
}

// 0x4D8260
int buttonCreate(int win, int x, int y, int width, int height, int mouseEnterEventCode, int mouseExitEventCode, int mouseDownEventCode, int mouseUpEventCode, unsigned char* up, unsigned char* dn, unsigned char* hover, int flags)
{
    Window* window = windowGetWindow(win);

    if (!gWindowSystemInitialized) {
        return -1;
    }

    if (window == NULL) {
        return -1;
    }

    if (up == NULL && (dn != NULL || hover != NULL)) {
        return -1;
    }

    Button* button = buttonCreateInternal(win, x, y, width, height, mouseEnterEventCode, mouseExitEventCode, mouseDownEventCode, mouseUpEventCode, flags | BUTTON_FLAG_0x010000, up, dn, hover);
    if (button == NULL) {
        return -1;
    }

    sub_4D9808(button, window, button->mouseUpImage, 0, NULL, 0);

    return button->id;
}

// 0x4D8674
int sub_4D8674(int btn, unsigned char* up, unsigned char* down, unsigned char* hover)
{
    if (!gWindowSystemInitialized) {
        return -1;
    }

    Button* button = buttonGetButton(btn, NULL);
    if (button == NULL) {
        return -1;
    }

    button->field_3C = up;
    button->field_40 = down;
    button->field_44 = hover;

    return 0;
}

// 0x4D86A8
int sub_4D86A8(int btn, unsigned char* up, unsigned char* down, unsigned char* hover, int a5)
{
    if (!gWindowSystemInitialized) {
        return -1;
    }

    if (up == NULL && (down != NULL || hover != NULL)) {
        return -1;
    }

    Window* window;
    Button* button = buttonGetButton(btn, &window);
    if (button == NULL) {
        return -1;
    }

    if (!(button->flags & BUTTON_FLAG_0x010000)) {
        return -1;
    }

    unsigned char* data = button->currentImage;
    if (data == button->mouseUpImage) {
        button->currentImage = up;
    } else if (data == button->mouseDownImage) {
        button->currentImage = down;
    } else if (data == button->mouseHoverImage) {
        button->currentImage = hover;
    }

    button->mouseUpImage = up;
    button->mouseDownImage = down;
    button->mouseHoverImage = hover;

    sub_4D9808(button, window, button->currentImage, a5, NULL, 0);

    return 0;
}

// Sets primitive callbacks on the button.
//
// 0x4D8758
int buttonSetMouseCallbacks(int btn, ButtonCallback* mouseEnterProc, ButtonCallback* mouseExitProc, ButtonCallback* mouseDownProc, ButtonCallback* mouseUpProc)
{
    if (!gWindowSystemInitialized) {
        return -1;
    }

    Button* button = buttonGetButton(btn, NULL);
    if (button == NULL) {
        return -1;
    }

    button->mouseEnterProc = mouseEnterProc;
    button->mouseExitProc = mouseExitProc;
    button->leftMouseDownProc = mouseDownProc;
    button->leftMouseUpProc = mouseUpProc;

    return 0;
}

// 0x4D8798
int buttonSetRightMouseCallbacks(int btn, int rightMouseDownEventCode, int rightMouseUpEventCode, ButtonCallback* rightMouseDownProc, ButtonCallback* rightMouseUpProc)
{
    if (!gWindowSystemInitialized) {
        return -1;
    }

    Button* button = buttonGetButton(btn, NULL);
    if (button == NULL) {
        return -1;
    }

    button->rightMouseDownEventCode = rightMouseDownEventCode;
    button->rightMouseUpEventCode = rightMouseUpEventCode;
    button->rightMouseDownProc = rightMouseDownProc;
    button->rightMouseUpProc = rightMouseUpProc;

    if (rightMouseDownEventCode != -1 || rightMouseUpEventCode != -1 || rightMouseDownProc != NULL || rightMouseUpProc != NULL) {
        button->flags |= BUTTON_FLAG_RIGHT_MOUSE_BUTTON_CONFIGURED;
    } else {
        button->flags &= ~BUTTON_FLAG_RIGHT_MOUSE_BUTTON_CONFIGURED;
    }

    return 0;
}

// Sets button state callbacks.
// [a2] - when button is transitioning to pressed state
// [a3] - when button is returned to unpressed state
//
// The changes in the state are tied to graphical state, therefore these callbacks are not generated for
// buttons with no graphics.
//
// These callbacks can be triggered several times during tracking if mouse leaves button's rectangle without releasing mouse buttons.
//
// 0x4D87F8
int buttonSetCallbacks(int btn, ButtonCallback* onPressed, ButtonCallback* onUnpressed)
{
    if (!gWindowSystemInitialized) {
        return -1;
    }

    Button* button = buttonGetButton(btn, NULL);
    if (button == NULL) {
        return -1;
    }

    button->onPressed = onPressed;
    button->onUnpressed = onUnpressed;

    return 0;
}

// 0x4D8828
int buttonSetMask(int btn, unsigned char* mask)
{
    if (!gWindowSystemInitialized) {
        return -1;
    }

    Button* button = buttonGetButton(btn, NULL);
    if (button == NULL) {
        return -1;
    }

    button->mask = mask;

    return 0;
}

// 0x4D8854
Button* buttonCreateInternal(int win, int x, int y, int width, int height, int mouseEnterEventCode, int mouseExitEventCode, int mouseDownEventCode, int mouseUpEventCode, int flags, unsigned char* up, unsigned char* dn, unsigned char* hover)
{
    Window* window = windowGetWindow(win);
    if (window == NULL) {
        return NULL;
    }

    Button* button = internal_malloc(sizeof(*button));
    if (button == NULL) {
        return NULL;
    }

    if ((flags & BUTTON_FLAG_0x01) == 0) {
        if ((flags & BUTTON_FLAG_0x02) != 0) {
            flags &= ~BUTTON_FLAG_0x02;
        }

        if ((flags & BUTTON_FLAG_0x04) != 0) {
            flags &= ~BUTTON_FLAG_0x04;
        }
    }

    int index = 0;
    while (buttonGetButton(index, NULL) != NULL) {
        index++;
    }

    button->id = index;
    button->flags = flags;
    button->rect.left = x;
    button->rect.top = y;
    button->rect.right = x + width - 1;
    button->rect.bottom = y + height - 1;
    button->mouseEnterEventCode = mouseEnterEventCode;
    button->mouseExitEventCode = mouseExitEventCode;
    button->lefMouseDownEventCode = mouseDownEventCode;
    button->leftMouseUpEventCode = mouseUpEventCode;
    button->rightMouseDownEventCode = -1;
    button->rightMouseUpEventCode = -1;
    button->mouseUpImage = up;
    button->mouseDownImage = dn;
    button->mouseHoverImage = hover;
    button->field_3C = NULL;
    button->field_40 = NULL;
    button->field_44 = NULL;
    button->currentImage = NULL;
    button->mask = NULL;
    button->mouseEnterProc = NULL;
    button->mouseExitProc = NULL;
    button->leftMouseDownProc = NULL;
    button->leftMouseUpProc = NULL;
    button->rightMouseDownProc = NULL;
    button->rightMouseUpProc = NULL;
    button->onPressed = NULL;
    button->onUnpressed = NULL;
    button->radioGroup = NULL;
    button->prev = NULL;

    button->next = window->buttonListHead;
    if (button->next != NULL) {
        button->next->prev = button;
    }
    window->buttonListHead = button;

    return button;
}

// 0x4D89E4
bool sub_4D89E4(int btn)
{
    if (!gWindowSystemInitialized) {
        return false;
    }

    Button* button = buttonGetButton(btn, NULL);
    if (button == NULL) {
        return false;
    }

    if ((button->flags & BUTTON_FLAG_0x01) != 0 && (button->flags & BUTTON_FLAG_0x020000) != 0) {
        return true;
    }

    return false;
}

// 0x4D8A10
int sub_4D8A10(Window* window, int* keyCodePtr)
{
    Rect v58;
    Button* field_34;
    Button* field_38;
    Button* button;

    if ((window->flags & WINDOW_HIDDEN) != 0) {
        return -1;
    }

    button = window->buttonListHead;
    field_34 = window->field_34;
    field_38 = window->field_38;

    if (field_34 != NULL) {
        rectCopy(&v58, &(field_34->rect));
        rectOffset(&v58, window->rect.left, window->rect.top);
    } else if (field_38 != NULL) {
        rectCopy(&v58, &(field_38->rect));
        rectOffset(&v58, window->rect.left, window->rect.top);
    }

    *keyCodePtr = -1;

    if (sub_4CA934(window->rect.left, window->rect.top, window->rect.right, window->rect.bottom)) {
        int mouseEvent = mouseGetEvent();
        if ((window->flags & WINDOW_FLAG_0x40) || (mouseEvent & MOUSE_EVENT_LEFT_BUTTON_DOWN) == 0) {
            if (mouseEvent == 0) {
                window->field_38 = NULL;
            }
        } else {
            windowUnhide(window->id);
        }

        if (field_34 != NULL) {
            if (!sub_4D9214(field_34, &v58)) {
                if (!(field_34->flags & BUTTON_FLAG_DISABLED)) {
                    *keyCodePtr = field_34->mouseExitEventCode;
                }

                if ((field_34->flags & BUTTON_FLAG_0x01) && (field_34->flags & BUTTON_FLAG_0x020000)) {
                    sub_4D9808(field_34, window, field_34->mouseDownImage, 1, NULL, 1);
                } else {
                    sub_4D9808(field_34, window, field_34->mouseUpImage, 1, NULL, 1);
                }

                window->field_34 = NULL;

                dword_51E404 = window->id;

                if (!(field_34->flags & BUTTON_FLAG_DISABLED)) {
                    if (field_34->mouseExitProc != NULL) {
                        field_34->mouseExitProc(field_34->id, *keyCodePtr);
                        if (!(field_34->flags & BUTTON_FLAG_0x40)) {
                            *keyCodePtr = -1;
                        }
                    }
                }
                return 0;
            }
            button = field_34;
        } else if (field_38 != NULL) {
            if (sub_4D9214(field_38, &v58)) {
                if (!(field_38->flags & BUTTON_FLAG_DISABLED)) {
                    *keyCodePtr = field_38->mouseEnterEventCode;
                }

                if ((field_38->flags & BUTTON_FLAG_0x01) && (field_38->flags & BUTTON_FLAG_0x020000)) {
                    sub_4D9808(field_38, window, field_38->mouseDownImage, 1, NULL, 1);
                } else {
                    sub_4D9808(field_38, window, field_38->mouseUpImage, 1, NULL, 1);
                }

                window->field_34 = field_38;

                dword_51E404 = window->id;

                if (!(field_38->flags & BUTTON_FLAG_DISABLED)) {
                    if (field_38->mouseEnterProc != NULL) {
                        field_38->mouseEnterProc(field_38->id, *keyCodePtr);
                        if (!(field_38->flags & BUTTON_FLAG_0x40)) {
                            *keyCodePtr = -1;
                        }
                    }
                }
                return 0;
            }
        }

        int v25 = dword_51E404;
        if (dword_51E404 != -1 && dword_51E404 != window->id) {
            Window* v26 = windowGetWindow(dword_51E404);
            if (v26 != NULL) {
                dword_51E404 = -1;

                Button* v28 = v26->field_34;
                if (v28 != NULL) {
                    if (!(v28->flags & BUTTON_FLAG_DISABLED)) {
                        *keyCodePtr = v28->mouseExitEventCode;
                    }

                    if ((v28->flags & BUTTON_FLAG_0x01) && (v28->flags & BUTTON_FLAG_0x020000)) {
                        sub_4D9808(v28, v26, v28->mouseDownImage, 1, NULL, 1);
                    } else {
                        sub_4D9808(v28, v26, v28->mouseUpImage, 1, NULL, 1);
                    }

                    v26->field_38 = NULL;
                    v26->field_34 = NULL;

                    if (!(v28->flags & BUTTON_FLAG_DISABLED)) {
                        if (v28->mouseExitProc != NULL) {
                            v28->mouseExitProc(v28->id, *keyCodePtr);
                            if (!(v28->flags & BUTTON_FLAG_0x40)) {
                                *keyCodePtr = -1;
                            }
                        }
                    }
                    return 0;
                }
            }
        }

        ButtonCallback* cb = NULL;

        while (button != NULL) {
            if (!(button->flags & BUTTON_FLAG_DISABLED)) {
                rectCopy(&v58, &(button->rect));
                rectOffset(&v58, window->rect.left, window->rect.top);
                if (sub_4D9214(button, &v58)) {
                    if (!(button->flags & BUTTON_FLAG_DISABLED)) {
                        if ((mouseEvent & MOUSE_EVENT_ANY_BUTTON_DOWN) != 0) {
                            if ((mouseEvent & MOUSE_EVENT_RIGHT_BUTTON_DOWN) != 0 && (button->flags & BUTTON_FLAG_RIGHT_MOUSE_BUTTON_CONFIGURED) == 0) {
                                button = NULL;
                                break;
                            }

                            if (button != window->field_34 && button != window->field_38) {
                                break;
                            }

                            window->field_38 = button;
                            window->field_34 = button;

                            if ((button->flags & BUTTON_FLAG_0x01) != 0) {
                                if ((button->flags & BUTTON_FLAG_0x02) != 0) {
                                    if ((button->flags & BUTTON_FLAG_0x020000) != 0) {
                                        if (!(button->flags & BUTTON_FLAG_0x04)) {
                                            if (button->radioGroup != NULL) {
                                                button->radioGroup->field_4--;
                                            }

                                            if ((mouseEvent & MOUSE_EVENT_LEFT_BUTTON_DOWN) != 0) {
                                                *keyCodePtr = button->leftMouseUpEventCode;
                                                cb = button->leftMouseUpProc;
                                            } else {
                                                *keyCodePtr = button->rightMouseUpEventCode;
                                                cb = button->rightMouseUpProc;
                                            }

                                            button->flags &= ~BUTTON_FLAG_0x020000;
                                        }
                                    } else {
                                        if (sub_4D9744(button) == -1) {
                                            button = NULL;
                                            break;
                                        }

                                        if ((mouseEvent & MOUSE_EVENT_LEFT_BUTTON_DOWN) != 0) {
                                            *keyCodePtr = button->lefMouseDownEventCode;
                                            cb = button->leftMouseDownProc;
                                        } else {
                                            *keyCodePtr = button->rightMouseDownEventCode;
                                            cb = button->rightMouseDownProc;
                                        }

                                        button->flags |= BUTTON_FLAG_0x020000;
                                    }
                                }
                            } else {
                                if (sub_4D9744(button) == -1) {
                                    button = NULL;
                                    break;
                                }

                                if ((mouseEvent & MOUSE_EVENT_LEFT_BUTTON_DOWN) != 0) {
                                    *keyCodePtr = button->lefMouseDownEventCode;
                                    cb = button->leftMouseDownProc;
                                } else {
                                    *keyCodePtr = button->rightMouseDownEventCode;
                                    cb = button->rightMouseDownProc;
                                }
                            }

                            sub_4D9808(button, window, button->mouseDownImage, 1, NULL, 1);
                            break;
                        }

                        Button* v49 = window->field_38;
                        if (button == v49 && (mouseEvent & MOUSE_EVENT_ANY_BUTTON_UP) != 0) {
                            window->field_38 = NULL;
                            window->field_34 = v49;

                            if (v49->flags & BUTTON_FLAG_0x01) {
                                if (!(v49->flags & BUTTON_FLAG_0x02)) {
                                    if (v49->flags & BUTTON_FLAG_0x020000) {
                                        if (!(v49->flags & BUTTON_FLAG_0x04)) {
                                            if (v49->radioGroup != NULL) {
                                                v49->radioGroup->field_4--;
                                            }

                                            if ((mouseEvent & MOUSE_EVENT_LEFT_BUTTON_UP) != 0) {
                                                *keyCodePtr = button->leftMouseUpEventCode;
                                                cb = button->leftMouseUpProc;
                                            } else {
                                                *keyCodePtr = button->rightMouseUpEventCode;
                                                cb = button->rightMouseUpProc;
                                            }

                                            button->flags &= ~BUTTON_FLAG_0x020000;
                                        }
                                    } else {
                                        if (sub_4D9744(v49) == -1) {
                                            button = NULL;
                                            sub_4D9808(v49, window, v49->mouseUpImage, 1, NULL, 1);
                                            break;
                                        }

                                        if ((mouseEvent & MOUSE_EVENT_LEFT_BUTTON_UP) != 0) {
                                            *keyCodePtr = v49->lefMouseDownEventCode;
                                            cb = v49->leftMouseDownProc;
                                        } else {
                                            *keyCodePtr = v49->rightMouseDownEventCode;
                                            cb = v49->rightMouseDownProc;
                                        }

                                        v49->flags |= BUTTON_FLAG_0x020000;
                                    }
                                }
                            } else {
                                if (v49->flags & BUTTON_FLAG_0x020000) {
                                    if (v49->radioGroup != NULL) {
                                        v49->radioGroup->field_4--;
                                    }
                                }

                                if ((mouseEvent & MOUSE_EVENT_LEFT_BUTTON_UP) != 0) {
                                    *keyCodePtr = v49->leftMouseUpEventCode;
                                    cb = v49->leftMouseUpProc;
                                } else {
                                    *keyCodePtr = v49->rightMouseUpEventCode;
                                    cb = v49->rightMouseUpProc;
                                }
                            }

                            if (button->mouseHoverImage != NULL) {
                                sub_4D9808(button, window, button->mouseHoverImage, 1, NULL, 1);
                            } else {
                                sub_4D9808(button, window, button->mouseUpImage, 1, NULL, 1);
                            }
                            break;
                        }
                    }

                    if (window->field_34 == NULL && mouseEvent == 0) {
                        window->field_34 = button;
                        if (!(button->flags & BUTTON_FLAG_DISABLED)) {
                            *keyCodePtr = button->mouseEnterEventCode;
                            cb = button->mouseEnterProc;
                        }

                        sub_4D9808(button, window, button->mouseHoverImage, 1, NULL, 1);
                    }
                    break;
                }
            }
            button = button->next;
        }

        if (button != NULL) {
            if ((button->flags & BUTTON_FLAG_0x10) != 0
                && (mouseEvent & MOUSE_EVENT_ANY_BUTTON_DOWN) != 0
                && (mouseEvent & MOUSE_EVENT_ANY_BUTTON_REPEAT) == 0) {
                sub_4D765C(window->id);
                sub_4D9808(button, window, button->mouseUpImage, 1, NULL, 1);
            }
        } else if ((window->flags & WINDOW_FLAG_0x80) != 0) {
            v25 |= mouseEvent << 8;
            if ((mouseEvent & MOUSE_EVENT_ANY_BUTTON_DOWN) != 0
                && (mouseEvent & MOUSE_EVENT_ANY_BUTTON_REPEAT) == 0) {
                sub_4D765C(window->id);
            }
        }

        dword_51E404 = window->id;

        if (button != NULL) {
            if (cb != NULL) {
                cb(button->id, *keyCodePtr);
                if (!(button->flags & BUTTON_FLAG_0x40)) {
                    *keyCodePtr = -1;
                }
            }
        }

        return 0;
    }

    if (field_34 != NULL) {
        *keyCodePtr = field_34->mouseExitEventCode;

        unsigned char* data;
        if ((field_34->flags & BUTTON_FLAG_0x01) && (field_34->flags & BUTTON_FLAG_0x020000)) {
            data = field_34->mouseDownImage;
        } else {
            data = field_34->mouseUpImage;
        }

        sub_4D9808(field_34, window, data, 1, NULL, 1);

        window->field_34 = NULL;
    }

    if (*keyCodePtr != -1) {
        dword_51E404 = window->id;

        if ((field_34->flags & BUTTON_FLAG_DISABLED) == 0) {
            if (field_34->mouseExitProc != NULL) {
                field_34->mouseExitProc(field_34->id, *keyCodePtr);
                if (!(field_34->flags & BUTTON_FLAG_0x40)) {
                    *keyCodePtr = -1;
                }
            }
        }
        return 0;
    }

    if (field_34 != NULL) {
        if ((field_34->flags & BUTTON_FLAG_DISABLED) == 0) {
            if (field_34->mouseExitProc != NULL) {
                field_34->mouseExitProc(field_34->id, *keyCodePtr);
            }
        }
    }

    return -1;
}

// 0x4D9214
bool sub_4D9214(Button* button, Rect* rect)
{
    if (!sub_4CA934(rect->left, rect->top, rect->right, rect->bottom)) {
        return false;
    }

    if (button->mask == NULL) {
        return true;
    }

    int x;
    int y;
    mouseGetPosition(&x, &y);
    x -= rect->left;
    y -= rect->top;

    int width = button->rect.right - button->rect.left + 1;
    return button->mask[width * y + x] != 0;
}

// 0x4D927C
int buttonGetWindowId(int btn)
{
    if (!gWindowSystemInitialized) {
        return -1;
    }

    Window* window;
    if (buttonGetButton(btn, &window) == NULL) {
        return -1;
    }

    return window->id;
}

// 0x4D92B4
int sub_4D92B4()
{
    return dword_51E404;
}

// 0x4D92BC
int buttonDestroy(int btn)
{
    if (!gWindowSystemInitialized) {
        return -1;
    }

    Window* window;
    Button* button = buttonGetButton(btn, &window);
    if (button == NULL) {
        return -1;
    }

    if (button->prev != NULL) {
        button->prev->next = button->next;
    } else {
        window->buttonListHead = button->next;
    }

    if (button->next != NULL) {
        button->next->prev = button->prev;
    }

    windowFill(window->id, button->rect.left, button->rect.top, button->rect.right - button->rect.left + 1, button->rect.bottom - button->rect.top + 1, window->field_20);

    if (button == window->field_34) {
        window->field_34 = NULL;
    }

    if (button == window->field_38) {
        window->field_38 = NULL;
    }

    buttonFree(button);

    return 0;
}

// 0x4D9374
void buttonFree(Button* button)
{
    if ((button->flags & BUTTON_FLAG_0x010000) == 0) {
        if (button->mouseUpImage != NULL) {
            internal_free(button->mouseUpImage);
        }

        if (button->mouseDownImage != NULL) {
            internal_free(button->mouseDownImage);
        }

        if (button->mouseHoverImage != NULL) {
            internal_free(button->mouseHoverImage);
        }

        if (button->field_3C != NULL) {
            internal_free(button->field_3C);
        }

        if (button->field_40 != NULL) {
            internal_free(button->field_40);
        }

        if (button->field_44 != NULL) {
            internal_free(button->field_44);
        }
    }

    RadioGroup* radioGroup = button->radioGroup;
    if (radioGroup != NULL) {
        for (int index = 0; index < radioGroup->buttonsLength; index++) {
            if (button == radioGroup->buttons[index]) {
                for (; index < radioGroup->buttonsLength - 1; index++) {
                    radioGroup->buttons[index] = radioGroup->buttons[index + 1];
                }

                radioGroup->buttonsLength--;

                break;
            }
        }
    }

    internal_free(button);
}

// 0x4D9474
int buttonEnable(int btn)
{
    if (!gWindowSystemInitialized) {
        return -1;
    }

    Window* window;
    Button* button = buttonGetButton(btn, &window);
    if (button == NULL) {
        return -1;
    }

    if ((button->flags & BUTTON_FLAG_DISABLED) != 0) {
        button->flags &= ~BUTTON_FLAG_DISABLED;
        sub_4D9808(button, window, button->currentImage, 1, NULL, 0);
    }

    return 0;
}

// 0x4D94D0
int buttonDisable(int btn)
{
    if (!gWindowSystemInitialized) {
        return -1;
    }

    Window* window;
    Button* button = buttonGetButton(btn, &window);
    if (button == NULL) {
        return -1;
    }

    if ((button->flags & BUTTON_FLAG_DISABLED) == 0) {
        button->flags |= BUTTON_FLAG_DISABLED;

        sub_4D9808(button, window, button->currentImage, 1, NULL, 0);

        if (button == window->field_34) {
            if (window->field_34->mouseExitEventCode != -1) {
                enqueueInputEvent(window->field_34->mouseExitEventCode);
                window->field_34 = NULL;
            }
        }
    }

    return 0;
}

// 0x4D9554
int sub_4D9554(int btn, bool a2, int a3)
{
    if (!gWindowSystemInitialized) {
        return -1;
    }

    Window* window;
    Button* button = buttonGetButton(btn, &window);
    if (button == NULL) {
        return -1;
    }

    if ((button->flags & BUTTON_FLAG_0x01) != 0) {
        int keyCode = -1;

        if ((button->flags & BUTTON_FLAG_0x020000) != 0) {
            if (!a2) {
                button->flags &= ~BUTTON_FLAG_0x020000;

                if ((a3 & 0x02) == 0) {
                    sub_4D9808(button, window, button->mouseUpImage, 1, NULL, 0);
                }

                if (button->radioGroup != NULL) {
                    button->radioGroup->field_4--;
                }

                keyCode = button->leftMouseUpEventCode;
            }
        } else {
            if (a2) {
                button->flags |= BUTTON_FLAG_0x020000;

                if ((a3 & 0x02) == 0) {
                    sub_4D9808(button, window, button->mouseDownImage, 1, NULL, 0);
                }

                if (button->radioGroup != NULL) {
                    button->radioGroup->field_4++;
                }

                keyCode = button->lefMouseDownEventCode;
            }
        }

        if (keyCode != -1) {
            if ((a3 & 0x01) != 0) {
                enqueueInputEvent(keyCode);
            }
        }
    }

    return 0;
}

// 0x4D962C
int sub_4D962C(int buttonCount, int* btns, int a3, void (*a4)(int))
{
    if (!gWindowSystemInitialized) {
        return -1;
    }

    if (buttonCount >= RADIO_GROUP_BUTTON_LIST_CAPACITY) {
        return -1;
    }

    for (int groupIndex = 0; groupIndex < RADIO_GROUP_LIST_CAPACITY; groupIndex++) {
        RadioGroup* radioGroup = &(gRadioGroups[groupIndex]);
        if (radioGroup->buttonsLength == 0) {
            radioGroup->field_4 = 0;

            for (int buttonIndex = 0; buttonIndex < buttonCount; buttonIndex++) {
                Button* button = buttonGetButton(btns[buttonIndex], NULL);
                if (button == NULL) {
                    return -1;
                }

                radioGroup->buttons[buttonIndex] = button;

                button->radioGroup = radioGroup;

                if ((button->flags & BUTTON_FLAG_0x020000) != 0) {
                    radioGroup->field_4++;
                }
            }

            radioGroup->buttonsLength = buttonCount;
            radioGroup->field_0 = a3;
            radioGroup->field_8 = a4;
            return 0;
        }
    }

    return -1;
}

// 0x4D96EC
int sub_4D96EC(int count, int* btns)
{
    if (!gWindowSystemInitialized) {
        return -1;
    }

    if (sub_4D962C(count, btns, 1, NULL) == -1) {
        return -1;
    }

    Button* button = buttonGetButton(btns[0], NULL);
    RadioGroup* radioGroup = button->radioGroup;

    for (int index = 0; index < radioGroup->buttonsLength; index++) {
        Button* v1 = radioGroup->buttons[index];
        v1->flags |= BUTTON_FLAG_0x040000;
    }

    return 0;
}

// 0x4D9744
int sub_4D9744(Button* button)
{
    if (button->radioGroup == NULL) {
        return 0;
    }

    if ((button->flags & BUTTON_FLAG_0x040000) != 0) {
        if (button->radioGroup->field_4 > 0) {
            for (int index = 0; index < button->radioGroup->buttonsLength; index++) {
                Button* v1 = button->radioGroup->buttons[index];
                if ((v1->flags & BUTTON_FLAG_0x020000) != 0) {
                    v1->flags &= ~BUTTON_FLAG_0x020000;

                    Window* window;
                    buttonGetButton(v1->id, &window);
                    sub_4D9808(v1, window, v1->mouseUpImage, 1, NULL, 1);

                    if (v1->leftMouseUpProc != NULL) {
                        v1->leftMouseUpProc(v1->id, v1->leftMouseUpEventCode);
                    }
                }
            }
        }

        if ((button->flags & BUTTON_FLAG_0x020000) == 0) {
            button->radioGroup->field_4++;
        }

        return 0;
    }

    if (button->radioGroup->field_4 < button->radioGroup->field_0) {
        if ((button->flags & BUTTON_FLAG_0x020000) == 0) {
            button->radioGroup->field_4++;
        }

        return 0;
    }

    if (button->radioGroup->field_8 != NULL) {
        button->radioGroup->field_8(button->id);
    }

    return -1;
}

// 0x4D9808
void sub_4D9808(Button* button, Window* window, unsigned char* data, int a4, Rect* a5, int a6)
{
    unsigned char* previousImage = NULL;
    if (data != NULL) {
        Rect v2;
        rectCopy(&v2, &(button->rect));
        rectOffset(&v2, window->rect.left, window->rect.top);

        Rect v3;
        if (a5 != NULL) {
            if (rectIntersection(&v2, a5, &v2) == -1) {
                return;
            }

            rectCopy(&v3, &v2);
            rectOffset(&v3, -window->rect.left, -window->rect.top);
        } else {
            rectCopy(&v3, &(button->rect));
        }

        if (data == button->mouseUpImage && (button->flags & BUTTON_FLAG_0x020000)) {
            data = button->mouseDownImage;
        }

        if (button->flags & BUTTON_FLAG_DISABLED) {
            if (data == button->mouseUpImage) {
                data = button->field_3C;
            } else if (data == button->mouseDownImage) {
                data = button->field_40;
            } else if (data == button->mouseHoverImage) {
                data = button->field_44;
            }
        } else {
            if (data == button->field_3C) {
                data = button->mouseUpImage;
            } else if (data == button->field_40) {
                data = button->mouseDownImage;
            } else if (data == button->field_44) {
                data = button->mouseHoverImage;
            }
        }

        if (data) {
            if (a4 == 0) {
                int width = button->rect.right - button->rect.left + 1;
                if ((button->flags & BUTTON_FLAG_TRANSPARENT) != 0) {
                    blitBufferToBufferTrans(
                        data + (v3.top - button->rect.top) * width + v3.left - button->rect.left,
                        v3.right - v3.left + 1,
                        v3.bottom - v3.top + 1,
                        width,
                        window->buffer + window->width * v3.top + v3.left,
                        window->width);
                } else {
                    blitBufferToBuffer(
                        data + (v3.top - button->rect.top) * width + v3.left - button->rect.left,
                        v3.right - v3.left + 1,
                        v3.bottom - v3.top + 1,
                        width,
                        window->buffer + window->width * v3.top + v3.left,
                        window->width);
                }
            }

            previousImage = button->currentImage;
            button->currentImage = data;

            if (a4 != 0) {
                sub_4D6FD8(window, &v2, 0);
            }
        }
    }

    if (a6) {
        if (previousImage != data) {
            if (data == button->mouseDownImage && button->onPressed != NULL) {
                button->onPressed(button->id, button->lefMouseDownEventCode);
            } else if (data == button->mouseUpImage && button->onUnpressed != NULL) {
                button->onUnpressed(button->id, button->leftMouseUpEventCode);
            }
        }
    }
}

// 0x4D9A58
void sub_4D9A58(Window* window, Rect* rect)
{
    Button* button = window->buttonListHead;
    if (button != NULL) {
        while (button->next != NULL) {
            button = button->next;
        }
    }

    while (button != NULL) {
        sub_4D9808(button, window, button->currentImage, 0, rect, 0);
        button = button->prev;
    }
}

// 0x4D9AA0
int sub_4D9AA0(int btn)
{
    if (!gWindowSystemInitialized) {
        return -1;
    }

    Window* window;
    Button* button = buttonGetButton(btn, &window);
    if (button == NULL) {
        return -1;
    }

    sub_4D9808(button, window, button->mouseDownImage, 1, NULL, 1);

    if (button->leftMouseDownProc != NULL) {
        button->leftMouseDownProc(btn, button->lefMouseDownEventCode);

        if ((button->flags & BUTTON_FLAG_0x40) != 0) {
            enqueueInputEvent(button->lefMouseDownEventCode);
        }
    } else {
        if (button->lefMouseDownEventCode != -1) {
            enqueueInputEvent(button->lefMouseDownEventCode);
        }
    }

    sub_4D9808(button, window, button->mouseUpImage, 1, NULL, 1);

    if (button->leftMouseUpProc != NULL) {
        button->leftMouseUpProc(btn, button->leftMouseUpEventCode);

        if ((button->flags & BUTTON_FLAG_0x40) != 0) {
            enqueueInputEvent(button->leftMouseUpEventCode);
        }
    } else {
        if (button->leftMouseUpEventCode != -1) {
            enqueueInputEvent(button->leftMouseUpEventCode);
        }
    }

    return 0;
}
