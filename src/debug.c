#include "debug.h"

#include "memory.h"
#include "window_manager_private.h"

#include <stdarg.h>
#include <stdlib.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// 0x51DEF8
FILE* off_51DEF8 = NULL;

// 0x51DEFC
int dword_51DEFC = 0;

// 0x51DF00
int dword_51DF00 = 0;

// 0x51DF04
DebugPrintProc* gDebugPrintProc = NULL;

// 0x4C6CD0
void GNW_debug_init()
{
    atexit(debug_exit);
}

// 0x4C6CDC
void debug_register_mono()
{
    if (gDebugPrintProc != debug_mono) {
        if (off_51DEF8 != NULL) {
            fclose(off_51DEF8);
            off_51DEF8 = NULL;
        }

        gDebugPrintProc = debug_mono;
        debug_clear();
    }
}

// 0x4C6D18
void debug_register_log(const char* fileName, const char* mode)
{
    if ((mode[0] == 'w' && mode[1] == 'a') && mode[1] == 't') {
        if (off_51DEF8 != NULL) {
            fclose(off_51DEF8);
        }

        off_51DEF8 = fopen(fileName, mode);
        gDebugPrintProc = debug_log;
    }
}

// 0x4C6D5C
void debug_register_screen()
{
    if (gDebugPrintProc != debug_screen) {
        if (off_51DEF8 != NULL) {
            fclose(off_51DEF8);
            off_51DEF8 = NULL;
        }

        gDebugPrintProc = debug_screen;
    }
}

// 0x4C6D90
void debug_register_env()
{
    const char* type = getenv("DEBUGACTIVE");
    if (type == NULL) {
        return;
    }

    char* copy = internal_malloc(strlen(type) + 1);
    if (copy == NULL) {
        return;
    }

    strcpy(copy, type);
    strlwr(copy);

    if (strcmp(copy, "mono") == 0) {
        // NOTE: Uninline.
        debug_register_mono();
    } else if (strcmp(copy, "log") == 0) {
        debug_register_log("debug.log", "wt");
    } else if (strcmp(copy, "screen") == 0) {
        // NOTE: Uninline.
        debug_register_screen();
    } else if (strcmp(copy, "gnw") == 0) {
        if (gDebugPrintProc != win_debug) {
            if (off_51DEF8 != NULL) {
                fclose(off_51DEF8);
                off_51DEF8 = NULL;
            }

            gDebugPrintProc = win_debug;
        }
    }

    internal_free(copy);
}

// 0x4C6F18
void debug_register_func(DebugPrintProc* proc)
{
    if (gDebugPrintProc != proc) {
        if (off_51DEF8 != NULL) {
            fclose(off_51DEF8);
            off_51DEF8 = NULL;
        }

        gDebugPrintProc = proc;
    }
}

// 0x4C6F48
int debugPrint(const char* format, ...)
{
    va_list args;
    va_start(args, format);

    int rc;

    if (gDebugPrintProc != NULL) {
        char string[260];
        vsprintf(string, format, args);

        rc = gDebugPrintProc(string);
    } else {
#ifdef _DEBUG
        char string[260];
        vsprintf(string, format, args);
        OutputDebugStringA(string);
#endif
        rc = -1;
    }

    va_end(args);

    return rc;
}

// 0x4C6F94
int debug_puts(char* string)
{
    if (gDebugPrintProc != NULL) {
        return gDebugPrintProc(string);
    }

    return -1;
}

// 0x4C6FAC
void debug_clear()
{
    // TODO: Something with segments.
}

// 0x4C7004
int debug_mono(char* string)
{
    if (gDebugPrintProc == debug_mono) {
        while (*string != '\0') {
            char ch = *string++;
            debug_putc(ch);
        }
    }
    return 0;
}

// 0x4C7028
int debug_log(char* string)
{
    if (gDebugPrintProc == debug_log) {
        if (off_51DEF8 == NULL) {
            return -1;
        }

        if (fprintf(off_51DEF8, string) < 0) {
            return -1;
        }

        if (fflush(off_51DEF8) == EOF) {
            return -1;
        }
    }

    return 0;
}

// 0x4C7068
int debug_screen(char* string)
{
    if (gDebugPrintProc == debug_screen) {
        printf(string);
    }

    return 0;
}

// 0x4C709C
void debug_putc()
{
    // TODO: Something with segments.
}

// 0x4C71AC
void debug_scroll()
{
    // TODO: Something with segments.
}

// 0x4C71E8
void debug_exit(void)
{
    if (off_51DEF8 != NULL) {
        fclose(off_51DEF8);
    }
}
