#ifndef FALLOUT_PLIB_GNW_GNW_H_
#define FALLOUT_PLIB_GNW_GNW_H_

#include <stdbool.h>
#include <stddef.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "plib/gnw/gnw_types.h"
#include "plib/gnw/rect.h"

#define MAX_WINDOW_COUNT (50)

// The maximum number of radio groups.
#define RADIO_GROUP_LIST_CAPACITY (64)

typedef enum WindowManagerErr {
    WINDOW_MANAGER_OK = 0,
    WINDOW_MANAGER_ERR_INITIALIZING_VIDEO_MODE = 1,
    WINDOW_MANAGER_ERR_NO_MEMORY = 2,
    WINDOW_MANAGER_ERR_INITIALIZING_TEXT_FONTS = 3,
    WINDOW_MANAGER_ERR_WINDOW_SYSTEM_ALREADY_INITIALIZED = 4,
    WINDOW_MANAGER_ERR_WINDOW_SYSTEM_NOT_INITIALIZED = 5,
    WINDOW_MANAGER_ERR_CURRENT_WINDOWS_TOO_BIG = 6,
    WINDOW_MANAGER_ERR_INITIALIZING_DEFAULT_DATABASE = 7,

    // Unknown fatal error.
    //
    // NOTE: When this error code returned from window system initialization, the
    // game simply exits without any debug message. There is no way to figure out
    // it's meaning.
    WINDOW_MANAGER_ERR_8 = 8,
    WINDOW_MANAGER_ERR_ALREADY_RUNNING = 9,
    WINDOW_MANAGER_ERR_TITLE_NOT_SET = 10,
    WINDOW_MANAGER_ERR_INITIALIZING_INPUT = 11,
} WindowManagerErr;

typedef int(VideoSystemInitProc)();
typedef void(VideoSystemExitProc)();

extern bool GNW_win_init_flag;
extern int GNW_wcolor[6];
extern unsigned char* screen_buffer;

extern void* GNW_texture;

int win_init(VideoSystemInitProc* videoSystemInitProc, VideoSystemExitProc* videoSystemExitProc, int a3);
void win_exit(void);
int win_add(int x, int y, int width, int height, int a4, int flags);
void win_delete(int win);
void win_buffering(bool a1);
void win_border(int win);
void win_print(int win, char* str, int a3, int x, int y, int a6);
void win_text(int win, char** fileNameList, int fileNameListLength, int maxWidth, int x, int y, int flags);
void win_line(int win, int left, int top, int right, int bottom, int color);
void win_box(int win, int left, int top, int right, int bottom, int color);
void win_fill(int win, int x, int y, int width, int height, int a6);
void win_show(int win);
void win_hide(int win);
void win_move(int win_index, int x, int y);
void win_draw(int win);
void win_draw_rect(int win, const Rect* rect);
void GNW_win_refresh(Window* window, Rect* rect, unsigned char* a3);
void win_refresh_all(Rect* rect);
void win_drag(int win);
void win_get_mouse_buf(unsigned char* a1);
Window* GNW_find(int win);
unsigned char* win_get_buf(int win);
int win_get_top_win(int x, int y);
int win_width(int win);
int win_height(int win);
int win_get_rect(int win, Rect* rect);
int win_check_all_buttons();
Button* GNW_find_button(int btn, Window** out_win);
int GNW_check_menu_bars(int a1);
void win_set_minimized_title(const char* title);
bool GNWSystemError(const char* str);

#endif /* FALLOUT_PLIB_GNW_GNW_H_ */
