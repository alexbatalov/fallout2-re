#ifndef FALLOUT_PLIB_GNW_GNW_H_
#define FALLOUT_PLIB_GNW_GNW_H_

#include <stdbool.h>
#include <stddef.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "plib/gnw/rect.h"

#define MAX_WINDOW_COUNT (50)

// The maximum number of radio groups.
#define RADIO_GROUP_LIST_CAPACITY (64)

// The maximum number of buttons in one radio group.
#define RADIO_GROUP_BUTTON_LIST_CAPACITY (64)

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

typedef enum WindowFlags {
    WINDOW_FLAG_0x01 = 0x01,
    WINDOW_FLAG_0x02 = 0x02,
    WINDOW_FLAG_0x04 = 0x04,
    WINDOW_HIDDEN = 0x08,
    WINDOW_FLAG_0x10 = 0x10,
    WINDOW_FLAG_0x20 = 0x20,
    WINDOW_FLAG_0x40 = 0x40,
    WINDOW_FLAG_0x80 = 0x80,
    WINDOW_FLAG_0x0100 = 0x0100,
} WindowFlags;

typedef enum ButtonFlags {
    BUTTON_FLAG_0x01 = 0x01,
    BUTTON_FLAG_0x02 = 0x02,
    BUTTON_FLAG_0x04 = 0x04,
    BUTTON_FLAG_DISABLED = 0x08,
    BUTTON_FLAG_0x10 = 0x10,
    BUTTON_FLAG_TRANSPARENT = 0x20,
    BUTTON_FLAG_0x40 = 0x40,
    BUTTON_FLAG_0x010000 = 0x010000,
    BUTTON_FLAG_0x020000 = 0x020000,
    BUTTON_FLAG_0x040000 = 0x040000,
    BUTTON_FLAG_RIGHT_MOUSE_BUTTON_CONFIGURED = 0x080000,
} ButtonFlags;

typedef struct MenuPulldown {
    Rect rect;
    int keyCode;
    int itemsLength;
    char** items;
    int field_1C;
    int field_20;
} MenuPulldown;

typedef struct MenuBar {
    int win;
    Rect rect;
    int pulldownsLength;
    MenuPulldown pulldowns[15];
    int borderColor;
    int backgroundColor;
} MenuBar;

typedef void WindowBlitProc(unsigned char* src, int width, int height, int srcPitch, unsigned char* dest, int destPitch);

typedef struct Button Button;
typedef struct RadioGroup RadioGroup;

typedef struct Window {
    int id;
    int flags;
    Rect rect;
    int width;
    int height;
    int field_20;
    // rand
    int field_24;
    // rand
    int field_28;
    unsigned char* buffer;
    Button* buttonListHead;
    Button* field_34;
    Button* field_38;
    MenuBar* menuBar;
    WindowBlitProc* blitProc;
} Window;

typedef void ButtonCallback(int btn, int keyCode);

typedef struct Button {
    int id;
    int flags;
    Rect rect;
    int mouseEnterEventCode;
    int mouseExitEventCode;
    int lefMouseDownEventCode;
    int leftMouseUpEventCode;
    int rightMouseDownEventCode;
    int rightMouseUpEventCode;
    unsigned char* mouseUpImage;
    unsigned char* mouseDownImage;
    unsigned char* mouseHoverImage;
    unsigned char* field_3C;
    unsigned char* field_40;
    unsigned char* field_44;
    unsigned char* currentImage;
    unsigned char* mask;
    ButtonCallback* mouseEnterProc;
    ButtonCallback* mouseExitProc;
    ButtonCallback* leftMouseDownProc;
    ButtonCallback* leftMouseUpProc;
    ButtonCallback* rightMouseDownProc;
    ButtonCallback* rightMouseUpProc;
    ButtonCallback* onPressed;
    ButtonCallback* onUnpressed;
    RadioGroup* radioGroup;
    Button* prev;
    Button* next;
} Button;

typedef struct RadioGroup {
    int field_0;
    int field_4;
    void (*field_8)(int);
    int buttonsLength;
    Button* buttons[RADIO_GROUP_BUTTON_LIST_CAPACITY];
} RadioGroup;

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
int win_register_button(int win, int x, int y, int width, int height, int mouseEnterEventCode, int mouseExitEventCode, int mouseDownEventCode, int mouseUpEventCode, unsigned char* up, unsigned char* dn, unsigned char* hover, int flags);
int win_register_text_button(int win, int x, int y, int mouseEnterEventCode, int mouseExitEventCode, int mouseDownEventCode, int mouseUpEventCode, const char* title, int flags);
int win_register_button_disable(int btn, unsigned char* up, unsigned char* down, unsigned char* hover);
int win_register_button_image(int btn, unsigned char* up, unsigned char* down, unsigned char* hover, int a5);
int win_register_button_func(int btn, ButtonCallback* mouseEnterProc, ButtonCallback* mouseExitProc, ButtonCallback* mouseDownProc, ButtonCallback* mouseUpProc);
int win_register_right_button(int btn, int rightMouseDownEventCode, int rightMouseUpEventCode, ButtonCallback* rightMouseDownProc, ButtonCallback* rightMouseUpProc);
int win_register_button_sound_func(int btn, ButtonCallback* onPressed, ButtonCallback* onUnpressed);
int win_register_button_mask(int btn, unsigned char* mask);
Button* button_create(int win, int x, int y, int width, int height, int mouseEnterEventCode, int mouseExitEventCode, int mouseDownEventCode, int mouseUpEventCode, int flags, unsigned char* up, unsigned char* dn, unsigned char* hover);
bool win_button_down(int btn);
int GNW_check_buttons(Window* window, int* out_a2);
int win_button_winID(int btn);
int win_last_button_winID();
int win_delete_button(int btn);
void GNW_delete_button(Button* ptr);
void win_delete_button_win(int btn, int inputEvent);
int button_new_id();
int win_enable_button(int btn);
int win_disable_button(int btn);
int win_set_button_rest_state(int btn, bool a2, int a3);
int win_group_check_buttons(int a1, int* a2, int a3, void (*a4)(int));
int win_group_radio_buttons(int a1, int* a2);
void GNW_button_refresh(Window* window, Rect* rect);
int win_button_press_and_release(int btn);

#endif /* FALLOUT_PLIB_GNW_GNW_H_ */
