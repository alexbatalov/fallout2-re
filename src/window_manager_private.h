#ifndef WINDOW_MANAGER_PRIVATE_H
#define WINDOW_MANAGER_PRIVATE_H

#include <stdbool.h>

typedef struct struc_177 struc_177;

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

extern int dword_51E414;
extern int dword_51E418;
extern bool dword_51E41C;

extern STRUCT_6B2340 stru_6B2340[5];
extern int dword_6B2368;
extern int dword_6B236C;
extern STRUCT_6B2370 stru_6B2370[5];
extern int dword_6B23AC;
extern int dword_6B23B0;
extern int dword_6B23B4;
extern int dword_6B23B8;
extern int dword_6B23BC;
extern int dword_6B23C0;
extern int dword_6B23C4;
extern char gProgramWindowTitle[256];

int win_debug(char* a1);
void win_debug_delete();
int win_register_menu_bar(int win, int x, int y, int width, int height, int a6, int a7);
int win_register_menu_pulldown(int win, int x, char* str, int a4);
int win_width_needed(char** fileNameList, int fileNameListLength);
int GNW_process_menu(struc_177* ptr, int i);
int calc_max_field_chars_wcursor(int a1, int a2);
void GNW_intr_init();
void GNW_intr_exit();
void tm_watch_msgs();
void tm_kill_msg();
void tm_kill_out_of_order(int a1);
void tm_click_response(int btn);
int tm_index_active(int a1);

#endif /* WINDOW_MANAGER_PRIVATE_H */
