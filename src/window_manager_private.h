#ifndef WINDOW_MANAGER_PRIVATE_H
#define WINDOW_MANAGER_PRIVATE_H

#include "geometry.h"

#include <stdbool.h>

typedef struct MenuBar MenuBar;

typedef struct STRUCT_6B2340 {
    int field_0;
    int field_4;
} STRUCT_6B2340;

typedef struct STRUCT_6B2370 {
    int field_0;
    // win
    int field_4;
    int field_8;
} STRUCT_6B2370;

extern int _wd;
extern MenuBar* _curr_menu;
extern bool _tm_watch_active;

extern STRUCT_6B2340 _tm_location[5];
extern int _tm_text_x;
extern int _tm_h;
extern STRUCT_6B2370 _tm_queue[5];
extern int _tm_persistence;
extern int _scr_center_x;
extern int _tm_text_y;
extern int _tm_kill;
extern int _tm_add;
extern int _curry;
extern int _currx;
extern char gProgramWindowTitle[256];

int sub_4DA6C0(const char* title, char** fileList, int fileListLength, int a4, int x, int y, int a7);
int sub_4DA70C(const char* title, char** fileList, int fileListLength, int a4, int x, int y, int a7, int a8);
int _win_get_str(char* dest, int length, const char* title, int x, int y);
int _win_msg(const char* string, int x, int y, int flags);
int _create_pull_down(char** stringList, int stringListLength, int x, int y, int a5, int a6, Rect* rect);
int _win_debug(char* string);
void _win_debug_delete(int btn, int keyCode);
int _win_register_menu_bar(int win, int x, int y, int width, int height, int borderColor, int backgroundColor);
int _win_register_menu_pulldown(int win, int x, char* title, int keyCode, int itemsLength, char** items, int a7, int a8);
void _win_delete_menu_bar(int win);
int _find_first_letter(int ch, char** stringList, int stringListLength);
int _win_width_needed(char** fileNameList, int fileNameListLength);
int _win_input_str(int win, char* dest, int maxLength, int x, int y, int textColor, int backgroundColor);
int sub_4DBD04(int win, Rect* rect, char** items, int itemsLength, int a5, int a6, MenuBar* menuBar, int pulldownIndex);
int _GNW_process_menu(MenuBar* menuBar, int pulldownIndex);
int _calc_max_field_chars_wcursor(int a1, int a2);
void _GNW_intr_init();
void _GNW_intr_exit();
void _tm_watch_msgs();
void _tm_kill_msg();
void _tm_kill_out_of_order(int a1);
void _tm_click_response(int btn);
int _tm_index_active(int a1);

#endif /* WINDOW_MANAGER_PRIVATE_H */
