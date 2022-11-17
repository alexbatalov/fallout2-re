#include "plib/gnw/gnw.h"

#include "plib/color/color.h"
#include "plib/gnw/input.h"
#include "plib/db/db.h"
#include "plib/gnw/button.h"
#include "plib/gnw/debug.h"
#include "plib/gnw/grbuf.h"
#include "plib/gnw/memory.h"
#include "game/palette.h"
#include "plib/gnw/text.h"
#include "plib/gnw/vcr.h"
#include "plib/gnw/intrface.h"
#include "plib/gnw/svga.h"
#include "plib/gnw/winmain.h"

static void win_free(int win);
static void win_clip(Window* window, RectPtr* rectListNodePtr, unsigned char* a3);
static void refresh_all(Rect* rect, unsigned char* a2);
static int colorOpen(const char* path, int flags);
static int colorRead(int fd, void* buf, size_t count);
static int colorClose(int fd);

// 0x51E3D8
static bool GNW95_already_running = false;

// 0x51E3DC
static HANDLE GNW95_title_mutex = INVALID_HANDLE_VALUE;

// 0x51E3E0
bool GNW_win_init_flag = false;

// 0x51E3E4
int GNW_wcolor[6] = {
    0,
    0,
    0,
    0,
    0,
    0,
};

// 0x51E3FC
static unsigned char* screen_buffer = NULL;

// 0x6ADD90
static int window_index[MAX_WINDOW_COUNT];

// 0x6ADE58
static Window* window[MAX_WINDOW_COUNT];

// 0x6ADF20
static VideoSystemExitProc* video_reset;

// 0x6ADF24
static int num_windows;

// 0x6ADF28
static int window_flags;

// 0x6ADF2C
static bool buffering;

// 0x6ADF30
static int bk_color;

// 0x6ADF34
static VideoSystemInitProc* video_set;

// 0x6ADF38
static int doing_refresh_all;

// 0x6ADF3C
void* GNW_texture;

// 0x4D5C30
int win_init(VideoSystemInitProc* videoSystemInitProc, VideoSystemExitProc* videoSystemExitProc, int a3)
{
    CloseHandle(GNW95_mutex);
    GNW95_mutex = INVALID_HANDLE_VALUE;

    if (GNW95_already_running) {
        return WINDOW_MANAGER_ERR_ALREADY_RUNNING;
    }

    if (GNW95_title_mutex == INVALID_HANDLE_VALUE) {
        return WINDOW_MANAGER_ERR_TITLE_NOT_SET;
    }

    if (GNW_win_init_flag) {
        return WINDOW_MANAGER_ERR_WINDOW_SYSTEM_ALREADY_INITIALIZED;
    }

    for (int index = 0; index < MAX_WINDOW_COUNT; index++) {
        window_index[index] = -1;
    }

    if (db_total() == 0) {
        if (db_init(NULL, 0, "", 1) == -1) {
            return WINDOW_MANAGER_ERR_INITIALIZING_DEFAULT_DATABASE;
        }
    }

    if (GNW_text_init() == -1) {
        return WINDOW_MANAGER_ERR_INITIALIZING_TEXT_FONTS;
    }

    reset_mode();

    video_set = videoSystemInitProc;
    video_reset = GNW95_reset_mode;

    int rc = videoSystemInitProc();
    if (rc == -1) {
        if (video_reset != NULL) {
            video_reset();
        }

        return WINDOW_MANAGER_ERR_INITIALIZING_VIDEO_MODE;
    }

    if (rc == 8) {
        return WINDOW_MANAGER_ERR_8;
    }

    if (a3 & 1) {
        screen_buffer = (unsigned char*)mem_malloc((scr_size.lry - scr_size.uly + 1) * (scr_size.lrx - scr_size.ulx + 1));
        if (screen_buffer == NULL) {
            if (video_reset != NULL) {
                video_reset();
            } else {
                GNW95_reset_mode();
            }

            return WINDOW_MANAGER_ERR_NO_MEMORY;
        }
    }

    buffering = false;
    doing_refresh_all = 0;

    colorInitIO(colorOpen, colorRead, colorClose);
    colorRegisterAlloc(mem_malloc, mem_realloc, mem_free);

    if (!initColors()) {
        unsigned char* palette = (unsigned char*)mem_malloc(768);
        if (palette == NULL) {
            if (video_reset != NULL) {
                video_reset();
            } else {
                GNW95_reset_mode();
            }

            if (screen_buffer != NULL) {
                mem_free(screen_buffer);
            }

            return WINDOW_MANAGER_ERR_NO_MEMORY;
        }

        buf_fill(palette, 768, 1, 768, 0);

        // TODO: Incomplete.
        // _colorBuildColorTable(getSystemPalette(), palette);

        mem_free(palette);
    }

    GNW_debug_init();

    if (GNW_input_init(a3) == -1) {
        return WINDOW_MANAGER_ERR_INITIALIZING_INPUT;
    }

    GNW_intr_init();

    Window* w = window[0] = (Window*)mem_malloc(sizeof(*w));
    if (w == NULL) {
        if (video_reset != NULL) {
            video_reset();
        } else {
            GNW95_reset_mode();
        }

        if (screen_buffer != NULL) {
            mem_free(screen_buffer);
        }

        return WINDOW_MANAGER_ERR_NO_MEMORY;
    }

    w->id = 0;
    w->flags = 0;
    w->rect.ulx = scr_size.ulx;
    w->rect.uly = scr_size.uly;
    w->rect.lrx = scr_size.lrx;
    w->rect.lry = scr_size.lry;
    w->width = scr_size.lrx - scr_size.ulx + 1;
    w->height = scr_size.lry - scr_size.uly + 1;
    w->field_24 = 0;
    w->field_28 = 0;
    w->buffer = NULL;
    w->buttonListHead = NULL;
    w->field_34 = NULL;
    w->field_38 = 0;
    w->menuBar = NULL;

    num_windows = 1;
    GNW_win_init_flag = 1;
    GNW_wcolor[3] = 21140;
    GNW_wcolor[4] = 32747;
    GNW_wcolor[5] = 31744;
    window_index[0] = 0;
    GNW_texture = NULL;
    bk_color = 0;
    GNW_wcolor[0] = 10570;
    window_flags = a3;
    GNW_wcolor[2] = 8456;
    GNW_wcolor[1] = 15855;

    atexit(win_exit);

    return WINDOW_MANAGER_OK;
}

// 0x4D616C
void win_exit(void)
{
    // 0x51E400
    static bool insideWinExit = false;

    if (!insideWinExit) {
        insideWinExit = true;
        if (GNW_win_init_flag) {
            GNW_intr_exit();

            for (int index = num_windows - 1; index >= 0; index--) {
                win_free(window[index]->id);
            }

            if (GNW_texture != NULL) {
                mem_free(GNW_texture);
            }

            if (screen_buffer != NULL) {
                mem_free(screen_buffer);
            }

            if (video_reset != NULL) {
                video_reset();
            }

            GNW_input_exit();
            GNW_rect_exit();
            GNW_text_exit();
            colorsClose();

            GNW_win_init_flag = false;

            CloseHandle(GNW95_title_mutex);
            GNW95_title_mutex = INVALID_HANDLE_VALUE;
        }
        insideWinExit = false;
    }
}

// win_add
// 0x4D6238
int win_add(int x, int y, int width, int height, int a4, int flags)
{
    int v23;
    int v25, v26;
    Window* tmp;

    if (!GNW_win_init_flag) {
        return -1;
    }

    if (num_windows == MAX_WINDOW_COUNT) {
        return -1;
    }

    if (width > rectGetWidth(&scr_size)) {
        return -1;
    }

    if (height > rectGetHeight(&scr_size)) {
        return -1;
    }

    Window* w = window[num_windows] = (Window*)mem_malloc(sizeof(*w));
    if (w == NULL) {
        return -1;
    }

    w->buffer = (unsigned char*)mem_malloc(width * height);
    if (w->buffer == NULL) {
        mem_free(w);
        return -1;
    }

    int index = 1;
    while (GNW_find(index) != NULL) {
        index++;
    }

    w->id = index;

    if ((flags & WINDOW_FLAG_0x01) != 0) {
        flags |= window_flags;
    }

    w->width = width;
    w->height = height;
    w->flags = flags;
    w->field_24 = rand() & 0xFFFE;
    w->field_28 = rand() & 0xFFFE;

    if (a4 == 256) {
        if (GNW_texture == NULL) {
            a4 = colorTable[GNW_wcolor[0]];
        }
    } else if ((a4 & 0xFF00) != 0) {
        int colorIndex = (a4 & 0xFF) - 1;
        a4 = (a4 & ~0xFFFF) | colorTable[GNW_wcolor[colorIndex]];
    }

    w->buttonListHead = 0;
    w->field_34 = 0;
    w->field_38 = 0;
    w->menuBar = NULL;
    w->blitProc = trans_buf_to_buf;
    w->field_20 = a4;
    window_index[index] = num_windows;
    num_windows++;

    win_fill(index, 0, 0, width, height, a4);

    w->flags |= WINDOW_HIDDEN;
    win_move(index, x, y);
    w->flags = flags;

    if ((flags & WINDOW_FLAG_0x04) == 0) {
        v23 = num_windows - 2;
        while (v23 > 0) {
            if (!(window[v23]->flags & WINDOW_FLAG_0x04)) {
                break;
            }
            v23--;
        }

        if (v23 != num_windows - 2) {
            v25 = v23 + 1;
            v26 = num_windows - 1;
            while (v26 > v25) {
                tmp = window[v26 - 1];
                window[v26] = tmp;
                window_index[tmp->id] = v26;
                v26--;
            }

            window[v25] = w;
            window_index[index] = v25;
        }
    }

    return index;
}

// 0x4D6468
void win_delete(int win)
{
    Window* w = GNW_find(win);

    if (!GNW_win_init_flag) {
        return;
    }

    if (w == NULL) {
        return;
    }

    Rect rect;
    rectCopy(&rect, &(w->rect));

    int v1 = window_index[w->id];
    win_free(win);

    window_index[win] = -1;

    for (int index = v1; index < num_windows - 1; index++) {
        window[index] = window[index + 1];
        window_index[window[index]->id] = index;
    }

    num_windows--;

    // NOTE: Uninline.
    win_refresh_all(&rect);
}

// 0x4D650C
static void win_free(int win)
{
    Window* w = GNW_find(win);
    if (w == NULL) {
        return;
    }

    if (w->buffer != NULL) {
        mem_free(w->buffer);
    }

    if (w->menuBar != NULL) {
        mem_free(w->menuBar);
    }

    Button* curr = w->buttonListHead;
    while (curr != NULL) {
        Button* next = curr->next;
        GNW_delete_button(curr);
        curr = next;
    }

    mem_free(w);
}

// 0x4D6558
void win_buffering(bool a1)
{
    if (screen_buffer != NULL) {
        buffering = a1;
    }
}

// 0x4D6568
void win_border(int win)
{
    if (!GNW_win_init_flag) {
        return;
    }

    Window* w = GNW_find(win);
    if (w == NULL) {
        return;
    }

    lighten_buf(w->buffer + 5, w->width - 10, 5, w->width);
    lighten_buf(w->buffer, 5, w->height, w->width);
    lighten_buf(w->buffer + w->width - 5, 5, w->height, w->width);
    lighten_buf(w->buffer + w->width * (w->height - 5) + 5, w->width - 10, 5, w->width);

    draw_box(w->buffer, w->width, 0, 0, w->width - 1, w->height - 1, colorTable[0]);

    draw_shaded_box(w->buffer, w->width, 1, 1, w->width - 2, w->height - 2, colorTable[GNW_wcolor[1]], colorTable[GNW_wcolor[2]]);
    draw_shaded_box(w->buffer, w->width, 5, 5, w->width - 6, w->height - 6, colorTable[GNW_wcolor[2]], colorTable[GNW_wcolor[1]]);
}

// 0x4D684C
void win_print(int win, char* str, int a3, int x, int y, int a6)
{
    int v7;
    int v14;
    unsigned char* buf;
    int v27;

    Window* w = GNW_find(win);
    v7 = a3;

    if (!GNW_win_init_flag) {
        return;
    }

    if (w == NULL) {
        return;
    }

    if (a3 == 0) {
        if (a6 & 0x040000) {
            v7 = text_mono_width(str);
        } else {
            v7 = text_width(str);
        }
    }

    if (v7 + x > w->width) {
        if (!(a6 & 0x04000000)) {
            return;
        }

        v7 = w->width - x;
    }

    buf = w->buffer + x + y * w->width;

    v14 = text_height();
    if (v14 + y > w->height) {
        return;
    }

    if (!(a6 & 0x02000000)) {
        if (w->field_20 == 256 && GNW_texture != NULL) {
            buf_texture(buf, v7, text_height(), w->width, GNW_texture, w->field_24 + x, w->field_28 + y);
        } else {
            buf_fill(buf, v7, text_height(), w->width, w->field_20);
        }
    }

    if ((a6 & 0xFF00) != 0) {
        int colorIndex = (a6 & 0xFF) - 1;
        v27 = (a6 & ~0xFFFF) | colorTable[GNW_wcolor[colorIndex]];
    } else {
        v27 = a6;
    }

    text_to_buf(buf, str, v7, w->width, v27);

    if (a6 & 0x01000000) {
        // TODO: Check.
        Rect rect;
        rect.ulx = w->rect.ulx + x;
        rect.uly = w->rect.uly + y;
        rect.lrx = rect.ulx + v7;
        rect.lry = rect.uly + text_height();
        GNW_win_refresh(w, &rect, NULL);
    }
}

// 0x4D69DC
void win_text(int win, char** fileNameList, int fileNameListLength, int maxWidth, int x, int y, int flags)
{
    Window* w = GNW_find(win);

    if (!GNW_win_init_flag) {
        return;
    }

    if (w == NULL) {
        return;
    }

    int width = w->width;
    unsigned char* ptr = w->buffer + y * width + x;
    int lineHeight = text_height();

    int step = width * lineHeight;
    int v1 = lineHeight / 2;
    int v2 = v1 + 1;
    int v3 = maxWidth - 1;

    for (int index = 0; index < fileNameListLength; index++) {
        char* fileName = fileNameList[index];
        if (*fileName != '\0') {
            win_print(win, fileName, maxWidth, x, y, flags);
        } else {
            if (maxWidth != 0) {
                draw_line(ptr, width, 0, v1, v3, v1, colorTable[GNW_wcolor[2]]);
                draw_line(ptr, width, 0, v2, v3, v2, colorTable[GNW_wcolor[1]]);
            }
        }

        ptr += step;
        y += lineHeight;
    }
}

// 0x4D6B24
void win_line(int win, int left, int top, int right, int bottom, int color)
{
    Window* w = GNW_find(win);

    if (!GNW_win_init_flag) {
        return;
    }

    if (w == NULL) {
        return;
    }

    if ((color & 0xFF00) != 0) {
        int colorIndex = (color & 0xFF) - 1;
        color = (color & ~0xFFFF) | colorTable[GNW_wcolor[colorIndex]];
    }

    draw_line(w->buffer, w->width, left, top, right, bottom, color);
}

// 0x4D6B88
void win_box(int win, int left, int top, int right, int bottom, int color)
{
    Window* w = GNW_find(win);

    if (!GNW_win_init_flag) {
        return;
    }

    if (w == NULL) {
        return;
    }

    if ((color & 0xFF00) != 0) {
        int colorIndex = (color & 0xFF) - 1;
        color = (color & ~0xFFFF) | colorTable[GNW_wcolor[colorIndex]];
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

    draw_box(w->buffer, w->width, left, top, right, bottom, color);
}

// 0x4D6CC8
void win_fill(int win, int x, int y, int width, int height, int a6)
{
    Window* w = GNW_find(win);

    if (!GNW_win_init_flag) {
        return;
    }

    if (w == NULL) {
        return;
    }

    if (a6 == 256) {
        if (GNW_texture != NULL) {
            buf_texture(w->buffer + w->width * y + x, width, height, w->width, GNW_texture, x + w->field_24, y + w->field_28);
        } else {
            a6 = colorTable[GNW_wcolor[0]] & 0xFF;
        }
    } else if ((a6 & 0xFF00) != 0) {
        int colorIndex = (a6 & 0xFF) - 1;
        a6 = (a6 & ~0xFFFF) | colorTable[GNW_wcolor[colorIndex]];
    }

    if (a6 < 256) {
        buf_fill(w->buffer + w->width * y + x, width, height, w->width, a6);
    }
}

// 0x4D6DAC
void win_show(int win)
{
    Window* w;
    int v3;
    int v5;
    int v7;
    Window* v6;

    w = GNW_find(win);
    v3 = window_index[w->id];

    if (!GNW_win_init_flag) {
        return;
    }

    if (w->flags & WINDOW_HIDDEN) {
        w->flags &= ~WINDOW_HIDDEN;
        if (v3 == num_windows - 1) {
            GNW_win_refresh(w, &(w->rect), NULL);
        }
    }

    v5 = num_windows - 1;
    if (v3 < v5 && !(w->flags & WINDOW_FLAG_0x02)) {
        v7 = v3;
        while (v3 < v5 && ((w->flags & WINDOW_FLAG_0x04) || !(window[v7 + 1]->flags & WINDOW_FLAG_0x04))) {
            v6 = window[v7 + 1];
            window[v7] = v6;
            v7++;
            window_index[v6->id] = v3++;
        }

        window[v3] = w;
        window_index[w->id] = v3;
        GNW_win_refresh(w, &(w->rect), NULL);
    }
}

// 0x4D6E64
void win_hide(int win)
{
    if (!GNW_win_init_flag) {
        return;
    }

    Window* w = GNW_find(win);
    if (w == NULL) {
        return;
    }

    if ((w->flags & WINDOW_HIDDEN) == 0) {
        w->flags |= WINDOW_HIDDEN;
        refresh_all(&(w->rect), NULL);
    }
}

// 0x4D6EA0
void win_move(int win, int x, int y)
{
    Window* w = GNW_find(win);

    if (!GNW_win_init_flag) {
        return;
    }

    if (w == NULL) {
        return;
    }

    Rect rect;
    rectCopy(&rect, &(w->rect));

    if (x < 0) {
        x = 0;
    }

    if (y < 0) {
        y = 0;
    }

    if ((w->flags & WINDOW_FLAG_0x0100) != 0) {
        x += 2;
    }

    if (x + w->width - 1 > scr_size.lrx) {
        x = scr_size.lrx - w->width + 1;
    }

    if (y + w->height - 1 > scr_size.lry) {
        y = scr_size.lry - w->height + 1;
    }

    if ((w->flags & WINDOW_FLAG_0x0100) != 0) {
        // TODO: Not sure what this means.
        x &= ~0x03;
    }

    w->rect.ulx = x;
    w->rect.uly = y;
    w->rect.lrx = w->width + x - 1;
    w->rect.lry = w->height + y - 1;

    if ((w->flags & WINDOW_HIDDEN) == 0) {
        GNW_win_refresh(w, &(w->rect), NULL);

        if (GNW_win_init_flag) {
            refresh_all(&rect, NULL);
        }
    }
}

// 0x4D6F5C
void win_draw(int win)
{
    Window* w = GNW_find(win);

    if (!GNW_win_init_flag) {
        return;
    }

    if (w == NULL) {
        return;
    }

    GNW_win_refresh(w, &(w->rect), NULL);
}

// 0x4D6F80
void win_draw_rect(int win, const Rect* rect)
{
    Window* w = GNW_find(win);

    if (!GNW_win_init_flag) {
        return;
    }

    if (w == NULL) {
        return;
    }

    Rect newRect;
    rectCopy(&newRect, rect);
    rectOffset(&newRect, w->rect.ulx, w->rect.uly);

    GNW_win_refresh(w, &newRect, NULL);
}

// 0x4D6FD8
void GNW_win_refresh(Window* w, Rect* rect, unsigned char* a3)
{
    RectPtr v26, v20, v23, v24;
    int dest_pitch;

    // TODO: Get rid of this.
    dest_pitch = 0;

    if ((w->flags & WINDOW_HIDDEN) != 0) {
        return;
    }

    if ((w->flags & WINDOW_FLAG_0x20) && buffering && !doing_refresh_all) {
        // TODO: Incomplete.
    } else {
        v26 = rect_malloc();
        if (v26 == NULL) {
            return;
        }

        v26->next = NULL;

        v26->rect.ulx = max(w->rect.ulx, rect->ulx);
        v26->rect.uly = max(w->rect.uly, rect->uly);
        v26->rect.lrx = min(w->rect.lrx, rect->lrx);
        v26->rect.lry = min(w->rect.lry, rect->lry);

        if (v26->rect.lrx >= v26->rect.ulx && v26->rect.lry >= v26->rect.uly) {
            if (a3) {
                dest_pitch = rect->lrx - rect->ulx + 1;
            }

            win_clip(w, &v26, a3);

            if (w->id) {
                v20 = v26;
                while (v20) {
                    GNW_button_refresh(w, &(v20->rect));

                    if (a3) {
                        if (buffering && (w->flags & WINDOW_FLAG_0x20)) {
                            w->blitProc(w->buffer + v20->rect.ulx - w->rect.ulx + (v20->rect.uly - w->rect.uly) * w->width,
                                v20->rect.lrx - v20->rect.ulx + 1,
                                v20->rect.lry - v20->rect.uly + 1,
                                w->width,
                                a3 + dest_pitch * (v20->rect.uly - rect->uly) + v20->rect.ulx - rect->ulx,
                                dest_pitch);
                        } else {
                            buf_to_buf(
                                w->buffer + v20->rect.ulx - w->rect.ulx + (v20->rect.uly - w->rect.uly) * w->width,
                                v20->rect.lrx - v20->rect.ulx + 1,
                                v20->rect.lry - v20->rect.uly + 1,
                                w->width,
                                a3 + dest_pitch * (v20->rect.uly - rect->uly) + v20->rect.ulx - rect->ulx,
                                dest_pitch);
                        }
                    } else {
                        if (buffering) {
                            if (w->flags & WINDOW_FLAG_0x20) {
                                w->blitProc(
                                    w->buffer + v20->rect.ulx - w->rect.ulx + (v20->rect.uly - w->rect.uly) * w->width,
                                    v20->rect.lrx - v20->rect.ulx + 1,
                                    v20->rect.lry - v20->rect.uly + 1,
                                    w->width,
                                    screen_buffer + v20->rect.uly * (scr_size.lrx - scr_size.ulx + 1) + v20->rect.ulx,
                                    scr_size.lrx - scr_size.ulx + 1);
                            } else {
                                buf_to_buf(
                                    w->buffer + v20->rect.ulx - w->rect.ulx + (v20->rect.uly - w->rect.uly) * w->width,
                                    v20->rect.lrx - v20->rect.ulx + 1,
                                    v20->rect.lry - v20->rect.uly + 1,
                                    w->width,
                                    screen_buffer + v20->rect.uly * (scr_size.lrx - scr_size.ulx + 1) + v20->rect.ulx,
                                    scr_size.lrx - scr_size.ulx + 1);
                            }
                        } else {
                            scr_blit(
                                w->buffer + v20->rect.ulx - w->rect.ulx + (v20->rect.uly - w->rect.uly) * w->width,
                                w->width,
                                v20->rect.lry - v20->rect.lry + 1,
                                0,
                                0,
                                v20->rect.lrx - v20->rect.ulx + 1,
                                v20->rect.lry - v20->rect.uly + 1,
                                v20->rect.ulx,
                                v20->rect.uly);
                        }
                    }

                    v20 = v20->next;
                }
            } else {
                rectdata* v16 = v26;
                while (v16 != NULL) {
                    int width = v16->rect.lrx - v16->rect.ulx + 1;
                    int height = v16->rect.lry - v16->rect.uly + 1;
                    unsigned char* buf = (unsigned char*)mem_malloc(width * height);
                    if (buf != NULL) {
                        buf_fill(buf, width, height, width, bk_color);
                        if (dest_pitch != 0) {
                            buf_to_buf(
                                buf,
                                width,
                                height,
                                width,
                                a3 + dest_pitch * (v16->rect.uly - rect->uly) + v16->rect.ulx - rect->ulx,
                                dest_pitch);
                        } else {
                            if (buffering) {
                                buf_to_buf(buf,
                                    width,
                                    height,
                                    width,
                                    screen_buffer + v16->rect.uly * (scr_size.lrx - scr_size.ulx + 1) + v16->rect.ulx,
                                    scr_size.lrx - scr_size.ulx + 1);
                            } else {
                                scr_blit(buf, width, height, 0, 0, width, height, v16->rect.ulx, v16->rect.uly);
                            }
                        }

                        mem_free(buf);
                    }
                    v16 = v16->next;
                }
            }

            v23 = v26;
            while (v23) {
                v24 = v23->next;

                if (buffering && !a3) {
                    scr_blit(
                        screen_buffer + v23->rect.ulx + (scr_size.lrx - scr_size.ulx + 1) * v23->rect.uly,
                        scr_size.lrx - scr_size.ulx + 1,
                        v23->rect.lry - v23->rect.uly + 1,
                        0,
                        0,
                        v23->rect.lrx - v23->rect.ulx + 1,
                        v23->rect.lry - v23->rect.uly + 1,
                        v23->rect.ulx,
                        v23->rect.uly);
                }

                rect_free(v23);

                v23 = v24;
            }

            if (!doing_refresh_all && a3 == NULL && mouse_hidden() == 0) {
                if (mouse_in(rect->ulx, rect->uly, rect->lrx, rect->lry)) {
                    mouse_show();
                }
            }
        } else {
            rect_free(v26);
        }
    }
}

// 0x4D759C
void win_refresh_all(Rect* rect)
{
    if (GNW_win_init_flag) {
        refresh_all(rect, NULL);
    }
}

// 0x4D75B0
static void win_clip(Window* w, RectPtr* rectListNodePtr, unsigned char* a3)
{
    int win;

    for (win = window_index[w->id] + 1; win < num_windows; win++) {
        if (*rectListNodePtr == NULL) {
            break;
        }

        // TODO: Review.
        Window* w = window[win];
        if (!(w->flags & WINDOW_HIDDEN)) {
            if (!buffering || !(w->flags & WINDOW_FLAG_0x20)) {
                rect_clip_list(rectListNodePtr, &(w->rect));
            } else {
                if (!doing_refresh_all) {
                    GNW_win_refresh(w, &(w->rect), NULL);
                    rect_clip_list(rectListNodePtr, &(w->rect));
                }
            }
        }
    }

    if (a3 == screen_buffer || a3 == NULL) {
        if (mouse_hidden() == 0) {
            Rect rect;
            mouse_get_rect(&rect);
            rect_clip_list(rectListNodePtr, &rect);
        }
    }
}

// 0x4D765C
void win_drag(int win)
{
    Window* w = GNW_find(win);

    if (!GNW_win_init_flag) {
        return;
    }

    if (w == NULL) {
        return;
    }

    win_show(win);

    Rect rect;
    rectCopy(&rect, &(w->rect));

    GNW_do_bk_process();

    if (vcr_update() != 3) {
        mouse_info();
    }

    if ((w->flags & WINDOW_FLAG_0x0100) && (w->rect.ulx & 3)) {
        win_move(w->id, w->rect.ulx, w->rect.uly);
    }
}

// 0x4D77F8
void win_get_mouse_buf(unsigned char* a1)
{
    Rect rect;
    mouse_get_rect(&rect);
    refresh_all(&rect, a1);
}

// 0x4D7814
static void refresh_all(Rect* rect, unsigned char* a2)
{
    doing_refresh_all = 1;

    for (int index = 0; index < num_windows; index++) {
        GNW_win_refresh(window[index], rect, a2);
    }

    doing_refresh_all = 0;

    if (a2 == NULL) {
        if (!mouse_hidden()) {
            if (mouse_in(rect->ulx, rect->uly, rect->lrx, rect->lry)) {
                mouse_show();
            }
        }
    }
}

// 0x4D7888
Window* GNW_find(int win)
{
    int v0;

    if (win == -1) {
        return NULL;
    }

    v0 = window_index[win];
    if (v0 == -1) {
        return NULL;
    }

    return window[v0];
}

// win_get_buf
// 0x4D78B0
unsigned char* win_get_buf(int win)
{
    Window* w = GNW_find(win);

    if (!GNW_win_init_flag) {
        return NULL;
    }

    if (w == NULL) {
        return NULL;
    }

    return w->buffer;
}

// 0x4D78CC
int win_get_top_win(int x, int y)
{
    for (int index = num_windows - 1; index >= 0; index--) {
        Window* w = window[index];
        if (x >= w->rect.ulx && x <= w->rect.lrx
            && y >= w->rect.uly && y <= w->rect.lry) {
            return w->id;
        }
    }

    return -1;
}

// 0x4D7918
int win_width(int win)
{
    Window* w = GNW_find(win);

    if (!GNW_win_init_flag) {
        return -1;
    }

    if (w == NULL) {
        return -1;
    }

    return w->width;
}

// 0x4D7934
int win_height(int win)
{
    Window* w = GNW_find(win);

    if (!GNW_win_init_flag) {
        return -1;
    }

    if (w == NULL) {
        return -1;
    }

    return w->height;
}

// 0x4D7950
int win_get_rect(int win, Rect* rect)
{
    Window* w = GNW_find(win);

    if (!GNW_win_init_flag) {
        return -1;
    }

    if (w == NULL) {
        return -1;
    }

    rectCopy(rect, &(w->rect));

    return 0;
}

// 0x4D797C
int win_check_all_buttons()
{
    if (!GNW_win_init_flag) {
        return -1;
    }

    int v1 = -1;
    for (int index = num_windows - 1; index >= 1; index--) {
        if (GNW_check_buttons(window[index], &v1) == 0) {
            break;
        }

        if ((window[index]->flags & WINDOW_FLAG_0x10) != 0) {
            break;
        }
    }

    return v1;
}

// 0x4D79DC
Button* GNW_find_button(int btn, Window** windowPtr)
{
    for (int index = 0; index < num_windows; index++) {
        Window* w = window[index];
        Button* button = w->buttonListHead;
        while (button != NULL) {
            if (button->id == btn) {
                if (windowPtr != NULL) {
                    *windowPtr = w;
                }

                return button;
            }
            button = button->next;
        }
    }

    return NULL;
}

// 0x4D7A34
int GNW_check_menu_bars(int a1)
{
    if (!GNW_win_init_flag) {
        return -1;
    }

    int v1 = a1;
    for (int index = num_windows - 1; index >= 1; index--) {
        Window* w = window[index];
        if (w->menuBar != NULL) {
            for (int pulldownIndex = 0; pulldownIndex < w->menuBar->pulldownsLength; pulldownIndex++) {
                if (v1 == w->menuBar->pulldowns[pulldownIndex].keyCode) {
                    v1 = GNW_process_menu(w->menuBar, pulldownIndex);
                    break;
                }
            }
        }

        if ((w->flags & 0x10) != 0) {
            break;
        }
    }

    return v1;
}

// 0x4D80D8
void win_set_minimized_title(const char* title)
{
    if (title == NULL) {
        return;
    }

    if (GNW95_title_mutex == INVALID_HANDLE_VALUE) {
        GNW95_title_mutex = CreateMutexA(NULL, TRUE, title);
        if (GetLastError() != ERROR_SUCCESS) {
            GNW95_already_running = true;
            return;
        }
    }

    strncpy(GNW95_title, title, 256);
    GNW95_title[256 - 1] = '\0';

    if (GNW95_hwnd != NULL) {
        SetWindowTextA(GNW95_hwnd, GNW95_title);
    }
}

// [open] implementation for palette operations backed by [XFile].
//
// 0x4D8174
static int colorOpen(const char* path, int flags)
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

    File* stream = db_fopen(path, mode);
    if (stream != NULL) {
        return (int)stream;
    }

    return -1;
}

// [read] implementation for palette file operations backed by [XFile].
//
// 0x4D81E8
static int colorRead(int fd, void* buf, size_t count)
{
    return db_fread(buf, 1, count, (File*)fd);
}

// [close] implementation for palette file operations backed by [XFile].
//
// 0x4D81E0
static int colorClose(int fd)
{
    return db_fclose((File*)fd);
}

// 0x4D8200
bool GNWSystemError(const char* text)
{
    HCURSOR cursor = LoadCursorA(GNW95_hInstance, MAKEINTRESOURCEA(IDC_ARROW));
    HCURSOR prev = SetCursor(cursor);
    ShowCursor(TRUE);
    MessageBoxA(NULL, text, NULL, MB_ICONSTOP);
    ShowCursor(FALSE);
    SetCursor(prev);
    return true;
}
