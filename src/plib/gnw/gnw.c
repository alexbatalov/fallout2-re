#include "plib/gnw/gnw.h"

#include "plib/color/color.h"
#include "core.h"
#include "db.h"
#include "plib/gnw/debug.h"
#include "draw.h"
#include "plib/gnw/memory.h"
#include "game/palette.h"
#include "plib/gnw/text.h"
#include "plib/gnw/vcr.h"
#include "plib/gnw/intrface.h"

static void win_free(int win);
static void win_clip(Window* window, RectPtr* rectListNodePtr, unsigned char* a3);
static void refresh_all(Rect* rect, unsigned char* a2);
static int colorOpen(const char* path, int flags);
static int colorRead(int fd, void* buf, size_t count);
static int colorClose(int fd);
static bool button_under_mouse(Button* button, Rect* rect);
static int button_check_group(Button* button);
static void button_draw(Button* button, Window* window, unsigned char* data, int a4, Rect* a5, int a6);

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

// 0x51E404
static int last_button_winID = -1;

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

// 0x6ADF40
static RadioGroup btn_grp[RADIO_GROUP_LIST_CAPACITY];

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

    if (_db_total() == 0) {
        if (dbOpen(NULL, 0, "", 1) == -1) {
            return WINDOW_MANAGER_ERR_INITIALIZING_DEFAULT_DATABASE;
        }
    }

    if (GNW_text_init() == -1) {
        return WINDOW_MANAGER_ERR_INITIALIZING_TEXT_FONTS;
    }

    _get_start_mode_();

    video_set = videoSystemInitProc;
    video_reset = dxinput_exit;

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
        screen_buffer = (unsigned char*)mem_malloc((_scr_size.lry - _scr_size.uly + 1) * (_scr_size.lrx - _scr_size.ulx + 1));
        if (screen_buffer == NULL) {
            if (video_reset != NULL) {
                video_reset();
            } else {
                directDrawFree();
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
                directDrawFree();
            }

            if (screen_buffer != NULL) {
                mem_free(screen_buffer);
            }

            return WINDOW_MANAGER_ERR_NO_MEMORY;
        }

        bufferFill(palette, 768, 1, 768, 0);

        // TODO: Incomplete.
        // _colorBuildColorTable(getSystemPalette(), palette);

        mem_free(palette);
    }

    GNW_debug_init();

    if (coreInit(a3) == -1) {
        return WINDOW_MANAGER_ERR_INITIALIZING_INPUT;
    }

    GNW_intr_init();

    Window* w = window[0] = (Window*)mem_malloc(sizeof(*w));
    if (w == NULL) {
        if (video_reset != NULL) {
            video_reset();
        } else {
            directDrawFree();
        }

        if (screen_buffer != NULL) {
            mem_free(screen_buffer);
        }

        return WINDOW_MANAGER_ERR_NO_MEMORY;
    }

    w->id = 0;
    w->flags = 0;
    w->rect.ulx = _scr_size.ulx;
    w->rect.uly = _scr_size.uly;
    w->rect.lrx = _scr_size.lrx;
    w->rect.lry = _scr_size.lry;
    w->width = _scr_size.lrx - _scr_size.ulx + 1;
    w->height = _scr_size.lry - _scr_size.uly + 1;
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

            coreExit();
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

    if (width > rectGetWidth(&_scr_size)) {
        return -1;
    }

    if (height > rectGetHeight(&_scr_size)) {
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
    w->blitProc = blitBufferToBufferTrans;
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

    _lighten_buf(w->buffer + 5, w->width - 10, 5, w->width);
    _lighten_buf(w->buffer, 5, w->height, w->width);
    _lighten_buf(w->buffer + w->width - 5, 5, w->height, w->width);
    _lighten_buf(w->buffer + w->width * (w->height - 5) + 5, w->width - 10, 5, w->width);

    bufferDrawRect(w->buffer, w->width, 0, 0, w->width - 1, w->height - 1, colorTable[0]);

    bufferDrawRectShadowed(w->buffer, w->width, 1, 1, w->width - 2, w->height - 2, colorTable[GNW_wcolor[1]], colorTable[GNW_wcolor[2]]);
    bufferDrawRectShadowed(w->buffer, w->width, 5, 5, w->width - 6, w->height - 6, colorTable[GNW_wcolor[2]], colorTable[GNW_wcolor[1]]);
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
            _buf_texture(buf, v7, text_height(), w->width, GNW_texture, w->field_24 + x, w->field_28 + y);
        } else {
            bufferFill(buf, v7, text_height(), w->width, w->field_20);
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
                bufferDrawLine(ptr, width, 0, v1, v3, v1, colorTable[GNW_wcolor[2]]);
                bufferDrawLine(ptr, width, 0, v2, v3, v2, colorTable[GNW_wcolor[1]]);
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

    bufferDrawLine(w->buffer, w->width, left, top, right, bottom, color);
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

    bufferDrawRect(w->buffer, w->width, left, top, right, bottom, color);
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
            _buf_texture(w->buffer + w->width * y + x, width, height, w->width, GNW_texture, x + w->field_24, y + w->field_28);
        } else {
            a6 = colorTable[GNW_wcolor[0]] & 0xFF;
        }
    } else if ((a6 & 0xFF00) != 0) {
        int colorIndex = (a6 & 0xFF) - 1;
        a6 = (a6 & ~0xFFFF) | colorTable[GNW_wcolor[colorIndex]];
    }

    if (a6 < 256) {
        bufferFill(w->buffer + w->width * y + x, width, height, w->width, a6);
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

    if (x + w->width - 1 > _scr_size.lrx) {
        x = _scr_size.lrx - w->width + 1;
    }

    if (y + w->height - 1 > _scr_size.lry) {
        y = _scr_size.lry - w->height + 1;
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
                            blitBufferToBuffer(
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
                                    screen_buffer + v20->rect.uly * (_scr_size.lrx - _scr_size.ulx + 1) + v20->rect.ulx,
                                    _scr_size.lrx - _scr_size.ulx + 1);
                            } else {
                                blitBufferToBuffer(
                                    w->buffer + v20->rect.ulx - w->rect.ulx + (v20->rect.uly - w->rect.uly) * w->width,
                                    v20->rect.lrx - v20->rect.ulx + 1,
                                    v20->rect.lry - v20->rect.uly + 1,
                                    w->width,
                                    screen_buffer + v20->rect.uly * (_scr_size.lrx - _scr_size.ulx + 1) + v20->rect.ulx,
                                    _scr_size.lrx - _scr_size.ulx + 1);
                            }
                        } else {
                            _scr_blit(
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
                        bufferFill(buf, width, height, width, bk_color);
                        if (dest_pitch != 0) {
                            blitBufferToBuffer(
                                buf,
                                width,
                                height,
                                width,
                                a3 + dest_pitch * (v16->rect.uly - rect->uly) + v16->rect.ulx - rect->ulx,
                                dest_pitch);
                        } else {
                            if (buffering) {
                                blitBufferToBuffer(buf,
                                    width,
                                    height,
                                    width,
                                    screen_buffer + v16->rect.uly * (_scr_size.lrx - _scr_size.ulx + 1) + v16->rect.ulx,
                                    _scr_size.lrx - _scr_size.ulx + 1);
                            } else {
                                _scr_blit(buf, width, height, 0, 0, width, height, v16->rect.ulx, v16->rect.uly);
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
                    _scr_blit(
                        screen_buffer + v23->rect.ulx + (_scr_size.lrx - _scr_size.ulx + 1) * v23->rect.uly,
                        _scr_size.lrx - _scr_size.ulx + 1,
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

    tickersExecute();

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

    File* stream = fileOpen(path, mode);
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
    return fileRead(buf, 1, count, (File*)fd);
}

// [close] implementation for palette file operations backed by [XFile].
//
// 0x4D81E0
static int colorClose(int fd)
{
    return fileClose((File*)fd);
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

// 0x4D8260
int win_register_button(int win, int x, int y, int width, int height, int mouseEnterEventCode, int mouseExitEventCode, int mouseDownEventCode, int mouseUpEventCode, unsigned char* up, unsigned char* dn, unsigned char* hover, int flags)
{
    Window* w = GNW_find(win);

    if (!GNW_win_init_flag) {
        return -1;
    }

    if (w == NULL) {
        return -1;
    }

    if (up == NULL && (dn != NULL || hover != NULL)) {
        return -1;
    }

    Button* button = button_create(win, x, y, width, height, mouseEnterEventCode, mouseExitEventCode, mouseDownEventCode, mouseUpEventCode, flags | BUTTON_FLAG_0x010000, up, dn, hover);
    if (button == NULL) {
        return -1;
    }

    button_draw(button, w, button->mouseUpImage, 0, NULL, 0);

    return button->id;
}

// 0x4D8308
int win_register_text_button(int win, int x, int y, int mouseEnterEventCode, int mouseExitEventCode, int mouseDownEventCode, int mouseUpEventCode, const char* title, int flags)
{
    Window* w = GNW_find(win);

    if (!GNW_win_init_flag) {
        return -1;
    }

    if (w == NULL) {
        return -1;
    }

    int buttonWidth = text_width(title) + 16;
    int buttonHeight = text_height() + 7;
    unsigned char* normal = (unsigned char*)mem_malloc(buttonWidth * buttonHeight);
    if (normal == NULL) {
        return -1;
    }

    unsigned char* pressed = (unsigned char*)mem_malloc(buttonWidth * buttonHeight);
    if (pressed == NULL) {
        mem_free(normal);
        return -1;
    }

    if (w->field_20 == 256 && GNW_texture != NULL) {
        // TODO: Incomplete.
    } else {
        bufferFill(normal, buttonWidth, buttonHeight, buttonWidth, w->field_20);
        bufferFill(pressed, buttonWidth, buttonHeight, buttonWidth, w->field_20);
    }

    _lighten_buf(normal, buttonWidth, buttonHeight, buttonWidth);

    text_to_buf(normal + buttonWidth * 3 + 8, title, buttonWidth, buttonWidth, colorTable[GNW_wcolor[3]]);
    bufferDrawRectShadowed(normal,
        buttonWidth,
        2,
        2,
        buttonWidth - 3,
        buttonHeight - 3,
        colorTable[GNW_wcolor[1]],
        colorTable[GNW_wcolor[2]]);
    bufferDrawRectShadowed(normal,
        buttonWidth,
        1,
        1,
        buttonWidth - 2,
        buttonHeight - 2,
        colorTable[GNW_wcolor[1]],
        colorTable[GNW_wcolor[2]]);
    bufferDrawRect(normal, buttonWidth, 0, 0, buttonWidth - 1, buttonHeight - 1, colorTable[0]);

    text_to_buf(pressed + buttonWidth * 4 + 9, title, buttonWidth, buttonWidth, colorTable[GNW_wcolor[3]]);
    bufferDrawRectShadowed(pressed,
        buttonWidth,
        2,
        2,
        buttonWidth - 3,
        buttonHeight - 3,
        colorTable[GNW_wcolor[2]],
        colorTable[GNW_wcolor[1]]);
    bufferDrawRectShadowed(pressed,
        buttonWidth,
        1,
        1,
        buttonWidth - 2,
        buttonHeight - 2,
        colorTable[GNW_wcolor[2]],
        colorTable[GNW_wcolor[1]]);
    bufferDrawRect(pressed, buttonWidth, 0, 0, buttonWidth - 1, buttonHeight - 1, colorTable[0]);

    Button* button = button_create(win,
        x,
        y,
        buttonWidth,
        buttonHeight,
        mouseEnterEventCode,
        mouseExitEventCode,
        mouseDownEventCode,
        mouseUpEventCode,
        flags,
        normal,
        pressed,
        NULL);
    if (button == NULL) {
        mem_free(normal);
        mem_free(pressed);
        return -1;
    }

    button_draw(button, w, button->mouseUpImage, 0, NULL, 0);

    return button->id;
}

// 0x4D8674
int win_register_button_disable(int btn, unsigned char* up, unsigned char* down, unsigned char* hover)
{
    if (!GNW_win_init_flag) {
        return -1;
    }

    Button* button = GNW_find_button(btn, NULL);
    if (button == NULL) {
        return -1;
    }

    button->field_3C = up;
    button->field_40 = down;
    button->field_44 = hover;

    return 0;
}

// 0x4D86A8
int win_register_button_image(int btn, unsigned char* up, unsigned char* down, unsigned char* hover, int a5)
{
    if (!GNW_win_init_flag) {
        return -1;
    }

    if (up == NULL && (down != NULL || hover != NULL)) {
        return -1;
    }

    Window* w;
    Button* button = GNW_find_button(btn, &w);
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

    button_draw(button, w, button->currentImage, a5, NULL, 0);

    return 0;
}

// Sets primitive callbacks on the button.
//
// 0x4D8758
int win_register_button_func(int btn, ButtonCallback* mouseEnterProc, ButtonCallback* mouseExitProc, ButtonCallback* mouseDownProc, ButtonCallback* mouseUpProc)
{
    if (!GNW_win_init_flag) {
        return -1;
    }

    Button* button = GNW_find_button(btn, NULL);
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
int win_register_right_button(int btn, int rightMouseDownEventCode, int rightMouseUpEventCode, ButtonCallback* rightMouseDownProc, ButtonCallback* rightMouseUpProc)
{
    if (!GNW_win_init_flag) {
        return -1;
    }

    Button* button = GNW_find_button(btn, NULL);
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
int win_register_button_sound_func(int btn, ButtonCallback* onPressed, ButtonCallback* onUnpressed)
{
    if (!GNW_win_init_flag) {
        return -1;
    }

    Button* button = GNW_find_button(btn, NULL);
    if (button == NULL) {
        return -1;
    }

    button->onPressed = onPressed;
    button->onUnpressed = onUnpressed;

    return 0;
}

// 0x4D8828
int win_register_button_mask(int btn, unsigned char* mask)
{
    if (!GNW_win_init_flag) {
        return -1;
    }

    Button* button = GNW_find_button(btn, NULL);
    if (button == NULL) {
        return -1;
    }

    button->mask = mask;

    return 0;
}

// 0x4D8854
Button* button_create(int win, int x, int y, int width, int height, int mouseEnterEventCode, int mouseExitEventCode, int mouseDownEventCode, int mouseUpEventCode, int flags, unsigned char* up, unsigned char* dn, unsigned char* hover)
{
    Window* w = GNW_find(win);
    if (w == NULL) {
        return NULL;
    }

    Button* button = (Button*)mem_malloc(sizeof(*button));
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

    // NOTE: Uninline.
    int buttonId = button_new_id();

    button->id = buttonId;
    button->flags = flags;
    button->rect.ulx = x;
    button->rect.uly = y;
    button->rect.lrx = x + width - 1;
    button->rect.lry = y + height - 1;
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

    button->next = w->buttonListHead;
    if (button->next != NULL) {
        button->next->prev = button;
    }
    w->buttonListHead = button;

    return button;
}

// 0x4D89E4
bool win_button_down(int btn)
{
    if (!GNW_win_init_flag) {
        return false;
    }

    Button* button = GNW_find_button(btn, NULL);
    if (button == NULL) {
        return false;
    }

    if ((button->flags & BUTTON_FLAG_0x01) != 0 && (button->flags & BUTTON_FLAG_0x020000) != 0) {
        return true;
    }

    return false;
}

// 0x4D8A10
int GNW_check_buttons(Window* w, int* keyCodePtr)
{
    Rect v58;
    Button* field_34;
    Button* field_38;
    Button* button;

    if ((w->flags & WINDOW_HIDDEN) != 0) {
        return -1;
    }

    button = w->buttonListHead;
    field_34 = w->field_34;
    field_38 = w->field_38;

    if (field_34 != NULL) {
        rectCopy(&v58, &(field_34->rect));
        rectOffset(&v58, w->rect.ulx, w->rect.uly);
    } else if (field_38 != NULL) {
        rectCopy(&v58, &(field_38->rect));
        rectOffset(&v58, w->rect.ulx, w->rect.uly);
    }

    *keyCodePtr = -1;

    if (mouse_click_in(w->rect.ulx, w->rect.uly, w->rect.lrx, w->rect.lry)) {
        int mouseEvent = mouse_get_buttons();
        if ((w->flags & WINDOW_FLAG_0x40) || (mouseEvent & MOUSE_EVENT_LEFT_BUTTON_DOWN) == 0) {
            if (mouseEvent == 0) {
                w->field_38 = NULL;
            }
        } else {
            win_show(w->id);
        }

        if (field_34 != NULL) {
            if (!button_under_mouse(field_34, &v58)) {
                if (!(field_34->flags & BUTTON_FLAG_DISABLED)) {
                    *keyCodePtr = field_34->mouseExitEventCode;
                }

                if ((field_34->flags & BUTTON_FLAG_0x01) && (field_34->flags & BUTTON_FLAG_0x020000)) {
                    button_draw(field_34, w, field_34->mouseDownImage, 1, NULL, 1);
                } else {
                    button_draw(field_34, w, field_34->mouseUpImage, 1, NULL, 1);
                }

                w->field_34 = NULL;

                last_button_winID = w->id;

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
            if (button_under_mouse(field_38, &v58)) {
                if (!(field_38->flags & BUTTON_FLAG_DISABLED)) {
                    *keyCodePtr = field_38->mouseEnterEventCode;
                }

                if ((field_38->flags & BUTTON_FLAG_0x01) && (field_38->flags & BUTTON_FLAG_0x020000)) {
                    button_draw(field_38, w, field_38->mouseDownImage, 1, NULL, 1);
                } else {
                    button_draw(field_38, w, field_38->mouseUpImage, 1, NULL, 1);
                }

                w->field_34 = field_38;

                last_button_winID = w->id;

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

        int v25 = last_button_winID;
        if (last_button_winID != -1 && last_button_winID != w->id) {
            Window* v26 = GNW_find(last_button_winID);
            if (v26 != NULL) {
                last_button_winID = -1;

                Button* v28 = v26->field_34;
                if (v28 != NULL) {
                    if (!(v28->flags & BUTTON_FLAG_DISABLED)) {
                        *keyCodePtr = v28->mouseExitEventCode;
                    }

                    if ((v28->flags & BUTTON_FLAG_0x01) && (v28->flags & BUTTON_FLAG_0x020000)) {
                        button_draw(v28, v26, v28->mouseDownImage, 1, NULL, 1);
                    } else {
                        button_draw(v28, v26, v28->mouseUpImage, 1, NULL, 1);
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
                rectOffset(&v58, w->rect.ulx, w->rect.uly);
                if (button_under_mouse(button, &v58)) {
                    if (!(button->flags & BUTTON_FLAG_DISABLED)) {
                        if ((mouseEvent & MOUSE_EVENT_ANY_BUTTON_DOWN) != 0) {
                            if ((mouseEvent & MOUSE_EVENT_RIGHT_BUTTON_DOWN) != 0 && (button->flags & BUTTON_FLAG_RIGHT_MOUSE_BUTTON_CONFIGURED) == 0) {
                                button = NULL;
                                break;
                            }

                            if (button != w->field_34 && button != w->field_38) {
                                break;
                            }

                            w->field_38 = button;
                            w->field_34 = button;

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
                                        if (button_check_group(button) == -1) {
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
                                if (button_check_group(button) == -1) {
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

                            button_draw(button, w, button->mouseDownImage, 1, NULL, 1);
                            break;
                        }

                        Button* v49 = w->field_38;
                        if (button == v49 && (mouseEvent & MOUSE_EVENT_ANY_BUTTON_UP) != 0) {
                            w->field_38 = NULL;
                            w->field_34 = v49;

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
                                        if (button_check_group(v49) == -1) {
                                            button = NULL;
                                            button_draw(v49, w, v49->mouseUpImage, 1, NULL, 1);
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
                                button_draw(button, w, button->mouseHoverImage, 1, NULL, 1);
                            } else {
                                button_draw(button, w, button->mouseUpImage, 1, NULL, 1);
                            }
                            break;
                        }
                    }

                    if (w->field_34 == NULL && mouseEvent == 0) {
                        w->field_34 = button;
                        if (!(button->flags & BUTTON_FLAG_DISABLED)) {
                            *keyCodePtr = button->mouseEnterEventCode;
                            cb = button->mouseEnterProc;
                        }

                        button_draw(button, w, button->mouseHoverImage, 1, NULL, 1);
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
                win_drag(w->id);
                button_draw(button, w, button->mouseUpImage, 1, NULL, 1);
            }
        } else if ((w->flags & WINDOW_FLAG_0x80) != 0) {
            v25 |= mouseEvent << 8;
            if ((mouseEvent & MOUSE_EVENT_ANY_BUTTON_DOWN) != 0
                && (mouseEvent & MOUSE_EVENT_ANY_BUTTON_REPEAT) == 0) {
                win_drag(w->id);
            }
        }

        last_button_winID = w->id;

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

        button_draw(field_34, w, data, 1, NULL, 1);

        w->field_34 = NULL;
    }

    if (*keyCodePtr != -1) {
        last_button_winID = w->id;

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
static bool button_under_mouse(Button* button, Rect* rect)
{
    if (!mouse_click_in(rect->ulx, rect->uly, rect->lrx, rect->lry)) {
        return false;
    }

    if (button->mask == NULL) {
        return true;
    }

    int x;
    int y;
    mouse_get_position(&x, &y);
    x -= rect->ulx;
    y -= rect->uly;

    int width = button->rect.lrx - button->rect.ulx + 1;
    return button->mask[width * y + x] != 0;
}

// 0x4D927C
int win_button_winID(int btn)
{
    if (!GNW_win_init_flag) {
        return -1;
    }

    Window* w;
    if (GNW_find_button(btn, &w) == NULL) {
        return -1;
    }

    return w->id;
}

// 0x4D92B4
int win_last_button_winID()
{
    return last_button_winID;
}

// 0x4D92BC
int win_delete_button(int btn)
{
    if (!GNW_win_init_flag) {
        return -1;
    }

    Window* w;
    Button* button = GNW_find_button(btn, &w);
    if (button == NULL) {
        return -1;
    }

    if (button->prev != NULL) {
        button->prev->next = button->next;
    } else {
        w->buttonListHead = button->next;
    }

    if (button->next != NULL) {
        button->next->prev = button->prev;
    }

    win_fill(w->id, button->rect.ulx, button->rect.uly, button->rect.lrx - button->rect.ulx + 1, button->rect.lry - button->rect.uly + 1, w->field_20);

    if (button == w->field_34) {
        w->field_34 = NULL;
    }

    if (button == w->field_38) {
        w->field_38 = NULL;
    }

    GNW_delete_button(button);

    return 0;
}

// 0x4D9374
void GNW_delete_button(Button* button)
{
    if ((button->flags & BUTTON_FLAG_0x010000) == 0) {
        if (button->mouseUpImage != NULL) {
            mem_free(button->mouseUpImage);
        }

        if (button->mouseDownImage != NULL) {
            mem_free(button->mouseDownImage);
        }

        if (button->mouseHoverImage != NULL) {
            mem_free(button->mouseHoverImage);
        }

        if (button->field_3C != NULL) {
            mem_free(button->field_3C);
        }

        if (button->field_40 != NULL) {
            mem_free(button->field_40);
        }

        if (button->field_44 != NULL) {
            mem_free(button->field_44);
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

    mem_free(button);
}

// NOTE: Unused.
//
// 0x4D9430
void win_delete_button_win(int btn, int inputEvent)
{
    Button* button;
    Window* w;

    button = GNW_find_button(btn, &w);
    if (button != NULL) {
        win_delete(w->id);
        enqueueInputEvent(inputEvent);
    }
}

// NOTE: Inlined.
//
// 0x4D9458
int button_new_id()
{
    int btn;

    btn = 1;
    while (GNW_find_button(btn, NULL) != NULL) {
        btn++;
    }

    return btn;
}

// 0x4D9474
int win_enable_button(int btn)
{
    if (!GNW_win_init_flag) {
        return -1;
    }

    Window* w;
    Button* button = GNW_find_button(btn, &w);
    if (button == NULL) {
        return -1;
    }

    if ((button->flags & BUTTON_FLAG_DISABLED) != 0) {
        button->flags &= ~BUTTON_FLAG_DISABLED;
        button_draw(button, w, button->currentImage, 1, NULL, 0);
    }

    return 0;
}

// 0x4D94D0
int win_disable_button(int btn)
{
    if (!GNW_win_init_flag) {
        return -1;
    }

    Window* w;
    Button* button = GNW_find_button(btn, &w);
    if (button == NULL) {
        return -1;
    }

    if ((button->flags & BUTTON_FLAG_DISABLED) == 0) {
        button->flags |= BUTTON_FLAG_DISABLED;

        button_draw(button, w, button->currentImage, 1, NULL, 0);

        if (button == w->field_34) {
            if (w->field_34->mouseExitEventCode != -1) {
                enqueueInputEvent(w->field_34->mouseExitEventCode);
                w->field_34 = NULL;
            }
        }
    }

    return 0;
}

// 0x4D9554
int win_set_button_rest_state(int btn, bool a2, int a3)
{
    if (!GNW_win_init_flag) {
        return -1;
    }

    Window* w;
    Button* button = GNW_find_button(btn, &w);
    if (button == NULL) {
        return -1;
    }

    if ((button->flags & BUTTON_FLAG_0x01) != 0) {
        int keyCode = -1;

        if ((button->flags & BUTTON_FLAG_0x020000) != 0) {
            if (!a2) {
                button->flags &= ~BUTTON_FLAG_0x020000;

                if ((a3 & 0x02) == 0) {
                    button_draw(button, w, button->mouseUpImage, 1, NULL, 0);
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
                    button_draw(button, w, button->mouseDownImage, 1, NULL, 0);
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
int win_group_check_buttons(int buttonCount, int* btns, int a3, void (*a4)(int))
{
    if (!GNW_win_init_flag) {
        return -1;
    }

    if (buttonCount >= RADIO_GROUP_BUTTON_LIST_CAPACITY) {
        return -1;
    }

    for (int groupIndex = 0; groupIndex < RADIO_GROUP_LIST_CAPACITY; groupIndex++) {
        RadioGroup* radioGroup = &(btn_grp[groupIndex]);
        if (radioGroup->buttonsLength == 0) {
            radioGroup->field_4 = 0;

            for (int buttonIndex = 0; buttonIndex < buttonCount; buttonIndex++) {
                Button* button = GNW_find_button(btns[buttonIndex], NULL);
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
int win_group_radio_buttons(int count, int* btns)
{
    if (!GNW_win_init_flag) {
        return -1;
    }

    if (win_group_check_buttons(count, btns, 1, NULL) == -1) {
        return -1;
    }

    Button* button = GNW_find_button(btns[0], NULL);
    RadioGroup* radioGroup = button->radioGroup;

    for (int index = 0; index < radioGroup->buttonsLength; index++) {
        Button* v1 = radioGroup->buttons[index];
        v1->flags |= BUTTON_FLAG_0x040000;
    }

    return 0;
}

// 0x4D9744
static int button_check_group(Button* button)
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

                    Window* w;
                    GNW_find_button(v1->id, &w);
                    button_draw(v1, w, v1->mouseUpImage, 1, NULL, 1);

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
static void button_draw(Button* button, Window* w, unsigned char* data, int a4, Rect* a5, int a6)
{
    unsigned char* previousImage = NULL;
    if (data != NULL) {
        Rect v2;
        rectCopy(&v2, &(button->rect));
        rectOffset(&v2, w->rect.ulx, w->rect.uly);

        Rect v3;
        if (a5 != NULL) {
            if (rect_inside_bound(&v2, a5, &v2) == -1) {
                return;
            }

            rectCopy(&v3, &v2);
            rectOffset(&v3, -w->rect.ulx, -w->rect.uly);
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
                int width = button->rect.lrx - button->rect.ulx + 1;
                if ((button->flags & BUTTON_FLAG_TRANSPARENT) != 0) {
                    blitBufferToBufferTrans(
                        data + (v3.uly - button->rect.uly) * width + v3.ulx - button->rect.ulx,
                        v3.lrx - v3.ulx + 1,
                        v3.lry - v3.uly + 1,
                        width,
                        w->buffer + w->width * v3.uly + v3.ulx,
                        w->width);
                } else {
                    blitBufferToBuffer(
                        data + (v3.uly - button->rect.uly) * width + v3.ulx - button->rect.ulx,
                        v3.lrx - v3.ulx + 1,
                        v3.lry - v3.uly + 1,
                        width,
                        w->buffer + w->width * v3.uly + v3.ulx,
                        w->width);
                }
            }

            previousImage = button->currentImage;
            button->currentImage = data;

            if (a4 != 0) {
                GNW_win_refresh(w, &v2, 0);
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
void GNW_button_refresh(Window* w, Rect* rect)
{
    Button* button = w->buttonListHead;
    if (button != NULL) {
        while (button->next != NULL) {
            button = button->next;
        }
    }

    while (button != NULL) {
        button_draw(button, w, button->currentImage, 0, rect, 0);
        button = button->prev;
    }
}

// 0x4D9AA0
int win_button_press_and_release(int btn)
{
    if (!GNW_win_init_flag) {
        return -1;
    }

    Window* w;
    Button* button = GNW_find_button(btn, &w);
    if (button == NULL) {
        return -1;
    }

    button_draw(button, w, button->mouseDownImage, 1, NULL, 1);

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

    button_draw(button, w, button->mouseUpImage, 1, NULL, 1);

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
