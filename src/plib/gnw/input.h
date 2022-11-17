#ifndef FALLOUT_PLIB_GNW_INPUT_H_
#define FALLOUT_PLIB_GNW_INPUT_H_

#include <stdbool.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "plib/gnw/kb.h"
#include "plib/gnw/mouse.h"

typedef unsigned long TOCKS;

typedef void(IdleFunc)();
typedef void(FocusFunc)(int);
typedef void(BackgroundProcess)();
typedef int(PauseWinFunc)();
typedef int(ScreenDumpFunc)(int width, int height, unsigned char* buffer, unsigned char* palette);

int GNW_input_init(int use_msec_timer);
void GNW_input_exit();
int get_input();
void process_bk();
void GNW_add_input_buffer(int a1);
void flush_input_buffer();
void GNW_do_bk_process();
void add_bk_process(BackgroundProcess* f);
void remove_bk_process(BackgroundProcess* f);
void enable_bk();
void disable_bk();
void register_pause(int new_pause_key, PauseWinFunc* new_pause_win_func);
void dump_screen();
int default_screendump(int width, int height, unsigned char* data, unsigned char* palette);
void register_screendump(int new_screendump_key, ScreenDumpFunc* new_screendump_func);
TOCKS get_time();
void pause_for_tocks(unsigned int ms);
void block_for_tocks(unsigned int ms);
unsigned int elapsed_time(unsigned int a1);
unsigned int elapsed_tocks(unsigned int a1, unsigned int a2);
unsigned int get_bk_time();
void GNW95_hook_input(int hook);
int GNW95_input_init();
void GNW95_input_exit();
LRESULT CALLBACK GNW95_keyboard_hook(int nCode, WPARAM wParam, LPARAM lParam);
void GNW95_process_message();
void GNW95_clear_time_stamps();
void GNW95_lost_focus();

#endif /* FALLOUT_PLIB_GNW_INPUT_H_ */
