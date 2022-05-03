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
void sub_4C6CD0()
{
    atexit(sub_4C71E8);
}

// 0x4C6CDC
void sub_4C6CDC()
{
    if (gDebugPrintProc != sub_4C7004) {
        if (off_51DEF8 != NULL) {
            fclose(off_51DEF8);
            off_51DEF8 = NULL;
        }

        gDebugPrintProc = sub_4C7004;
        sub_4C6FAC();
    }
}

// 0x4C6D18
void sub_4C6D18(const char* fileName, const char* mode)
{
    if ((mode[0] == 'w' && mode[1] == 'a') && mode[1] == 't') {
        if (off_51DEF8 != NULL) {
            fclose(off_51DEF8);
        }

        off_51DEF8 = fopen(fileName, mode);
        gDebugPrintProc = sub_4C7028;
    }
}

// 0x4C6D5C
void sub_4C6D5C()
{
    if (gDebugPrintProc != sub_4C7068) {
        if (off_51DEF8 != NULL) {
            fclose(off_51DEF8);
            off_51DEF8 = NULL;
        }

        gDebugPrintProc = sub_4C7068;
    }
}

// 0x4C6D90
void sub_4C6D90()
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
        sub_4C6CDC();
    } else if (strcmp(copy, "log") == 0) {
        sub_4C6D18("debug.log", "wt");
    } else if (strcmp(copy, "screen") == 0) {
        // NOTE: Uninline.
        sub_4C6D5C();
    } else if (strcmp(copy, "gnw") == 0) {
        if (gDebugPrintProc != sub_4DC30C) {
            if (off_51DEF8 != NULL) {
                fclose(off_51DEF8);
                off_51DEF8 = NULL;
            }

            gDebugPrintProc = sub_4DC30C;
        }
    }

    internal_free(copy);
}

// 0x4C6F18
void sub_4C6F18(DebugPrintProc* proc)
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
int sub_4C6F94(char* string)
{
    if (gDebugPrintProc != NULL) {
        return gDebugPrintProc(string);
    }

    return -1;
}

// 0x4C6FAC
void sub_4C6FAC()
{
    // TODO: Something with segments.
}

// 0x4C7004
int sub_4C7004(char* string)
{
    if (gDebugPrintProc == sub_4C7004) {
        while (*string != '\0') {
            char ch = *string++;
            sub_4C709C(ch);
        }
    }
    return 0;
}

// 0x4C7028
int sub_4C7028(char* string)
{
    if (gDebugPrintProc == sub_4C7028) {
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
int sub_4C7068(char* string)
{
    if (gDebugPrintProc == sub_4C7068) {
        printf(string);
    }

    return 0;
}

// 0x4C709C
void sub_4C709C()
{
    // TODO: Something with segments.
}

// 0x4C71AC
void sub_4C71AC()
{
    // TODO: Something with segments.
}

// 0x4C71E8
void sub_4C71E8(void)
{
    if (off_51DEF8 != NULL) {
        fclose(off_51DEF8);
    }
}
