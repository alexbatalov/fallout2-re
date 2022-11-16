#ifndef FALLOUT_PLIB_GNW_INTRFACE_H_
#define FALLOUT_PLIB_GNW_INTRFACE_H_

#include <stdbool.h>

#include "plib/gnw/rect.h"

typedef struct MenuBar MenuBar;

typedef void(SelectFunc)(char** items, int index);

int win_list_select(const char* title, char** fileList, int fileListLength, SelectFunc* callback, int x, int y, int a7);
int win_list_select_at(const char* title, char** fileList, int fileListLength, SelectFunc* callback, int x, int y, int a7, int a8);
int win_get_str(char* dest, int length, const char* title, int x, int y);
int win_msg(const char* string, int x, int y, int flags);
int win_pull_down(char** items, int itemsLength, int x, int y, int a5);
int win_debug(char* string);
int win_register_menu_bar(int win, int x, int y, int width, int height, int borderColor, int backgroundColor);
int win_register_menu_pulldown(int win, int x, char* title, int keyCode, int itemsLength, char** items, int a7, int a8);
void win_delete_menu_bar(int win);
int win_width_needed(char** fileNameList, int fileNameListLength);
int win_input_str(int win, char* dest, int maxLength, int x, int y, int textColor, int backgroundColor);
int GNW_process_menu(MenuBar* menuBar, int pulldownIndex);
void GNW_intr_init();
void GNW_intr_exit();

#endif /* FALLOUT_PLIB_GNW_INTRFACE_H_ */
