#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

typedef int(DebugPrintProc)(char* string);

extern FILE* off_51DEF8;
extern int dword_51DEFC;
extern int dword_51DF00;
extern DebugPrintProc* gDebugPrintProc;

void GNW_debug_init();
void debug_register_mono();
void debug_register_log(const char* fileName, const char* mode);
void debug_register_screen();
void debug_register_env();
void debug_register_func(DebugPrintProc* proc);
int debugPrint(const char* format, ...);
int debug_puts(char* string);
void debug_clear();
int debug_mono(char* string);
int debug_log(char* string);
int debug_screen(char* string);
void debug_putc();
void debug_exit(void);

#endif /* DEBUG_H */
