#include "window.h"

#include "core.h"
#include "interpreter_lib.h"
#include "memory_manager.h"
#include "mouse_manager.h"
#include "movie.h"
#include "text_font.h"
#include "widget.h"
#include "window_manager.h"

#include <stdio.h>

// 0x51DCAC
int dword_51DCAC = 250;

// 0x51DCB0
int dword_51DCB0 = 1;

// 0x51DCB4
int dword_51DCB4 = -1;

// 051DCB8
int dword_51DCB8 = -1;

// 0x51DCBC
INITVIDEOFN off_51DCBC[12] = {
    sub_4CAD08,
    sub_4CAD64,
    sub_4CAD5C,
    sub_4CAD40,
    sub_4CAD5C,
    sub_4CAD94,
    sub_4CAD5C,
    sub_4CADA8,
    sub_4CAD5C,
    sub_4CADBC,
    sub_4CAD5C,
    sub_4CADD0,
};

// 0x51DD1C
Size stru_51DD1C[12] = {
    { 320, 200 },
    { 640, 480 },
    { 640, 240 },
    { 320, 400 },
    { 640, 200 },
    { 640, 400 },
    { 800, 300 },
    { 800, 600 },
    { 1024, 384 },
    { 1024, 768 },
    { 1280, 512 },
    { 1280, 1024 },
};

// 0x51DD7C
int dword_51DD7C = 0;

// 0x51DD80
int dword_51DD80 = -1;

// 0x51DD84
int dword_51DD84 = 1;

// 0x66E770
int dword_66E770[16];

// 0x66E7B0
char byte_66E7B0[64 * 256];

// 0x6727B0
STRUCT_6727B0 stru_6727B0[16];

// 0x672D7C
int dword_672D7C;

// 0x672D88
int dword_672D88;

// Highlight color (maybe r).
//
// 0x672D8C
int dword_672D8C;

// 0x672D90
int gWidgetFont;

// Text color (maybe g).
//
// 0x672DA0
int dword_672DA0;

// text color (maybe b).
//
// 0x672DA4
int dword_672DA4;

// 0x672DA8
int gWidgetTextFlags;

// Text color (maybe r)
//
// 0x672DAC
int dword_672DAC;

// highlight color (maybe g)
//
// 0x672DB0
int dword_672DB0;

// Highlight color (maybe b).
//
// 0x672DB4
int dword_672DB4;

// 0x4B8414
void sub_4B8414(int win, char* str, int a3, int a4, int a5, int a6, int a7, int a8, int a9)
{
    char* v11;
    int v13;
    int v14;
    unsigned char* buf;

    if (a7 + fontGetLineHeight() > a5) {
        return;
    }

    if (a3 > 255) {
        a3 = 255;
    }

    v11 = internal_malloc_safe(a3 + 1, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1078
    strncpy(v11, str, a3);
    v11[a3] = '\0';

    v13 = fontGetStringWidth(v11);
    v14 = fontGetLineHeight();
    if (v13 && v14) {
        if (a8 & (0x01 << 16)) {
            v13++;
            v14++;
        }

        buf = internal_calloc_safe(v13, v14, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1093
        fontDrawText(buf, v11, v13, v13, a8);

        // TODO: Incomplete.

        if (a7 & (0x02 << 24)) {
        } else {
        }

        internal_free_safe(str, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1130
        internal_free_safe(v11, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1131
    } else {
        internal_free_safe(v11, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1085
    }
}

// 0x4B8638
char** sub_4B8638(char* a1, int a2, int a3, int* a4)
{
    char** strings;
    int strings_count;
    int len;

    if (a1 == NULL) {
        *a4 = 0;
        return NULL;
    }

    // TODO: Incomplete.

    return NULL;
}

// 0x4B880C
void sub_4B880C(char** strings, int strings_count)
{
    int i;

    if (strings == NULL) {
        return;
    }

    for (i = 0; i < strings_count; i++) {
        internal_free_safe(strings[i], __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1200
    }

    internal_free_safe(strings, __FILE__, __LINE__); // "..\\int\\WINDOW.C", 1201
}

// 0x4B8854
void sub_4B8854(int win, char* a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9)
{
    int strings_count;
    char** strings;
    int i;
    char* str;
    int v13;

    if (a2 == NULL) {
        return;
    }

    strings = sub_4B8638(a2, a3, 0, &strings_count);

    for (i = 0; i < strings_count; i++) {
        v13 = a6 + i * (a9 + fontGetLineHeight());
        sub_4B8414(win, strings[i], strlen(strings[i]), a3, a4 + a6, a5, v13, a7, a8);
    }

    sub_4B880C(strings, strings_count);
}

// 0x4B88FC
void sub_4B88FC(int win, char* a2, int a3, int a4, int a5, int a6, int a7, int a8)
{
    sub_4B8854(win, a2, a3, a4, a5, a6, a7, a8, 0);
}

// 0x4B9048
int sub_4B9048()
{
    return dword_672D7C;
}

// 0x4B9050
int sub_4B9050()
{
    return dword_672D88;
}

// 0x4B9058
void sub_4B9058(Program* program)
{
    // TODO: Incomplete.
}

// 0x4B9190
void sub_4B9190(int resolution, int a2)
{
    char err[MAX_PATH];
    int rc;
    int i, j;

    sub_466F6C(sub_4B9058);

    dword_672DAC = 0;
    dword_672DA0 = 0;
    dword_672DA4 = 0;
    dword_672D8C = 0;
    dword_672DB0 = 0;
    gWidgetTextFlags = 0x2010000;

    dword_672D88 = stru_51DD1C[resolution].width; // screen height
    dword_672DB4 = 0;
    dword_672D7C = stru_51DD1C[resolution].height; // screen width

    for (int i = 0; i < 16; i++) {
        stru_6727B0[i].window = -1;
    }

    rc = windowManagerInit(off_51DCBC[resolution], directDrawFree, a2);
    if (rc != WINDOW_MANAGER_OK) {
        switch (rc) {
        case WINDOW_MANAGER_ERR_INITIALIZING_VIDEO_MODE:
            sprintf(err, "Error initializing video mode %dx%d\n", dword_672D7C, dword_672D88);
            showMesageBox(err);
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_NO_MEMORY:
            sprintf(err, "Not enough memory to initialize video mode\n");
            showMesageBox(err);
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_INITIALIZING_TEXT_FONTS:
            sprintf(err, "Couldn't find/load text fonts\n");
            showMesageBox(err);
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_WINDOW_SYSTEM_ALREADY_INITIALIZED:
            sprintf(err, "Attempt to initialize window system twice\n");
            showMesageBox(err);
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_WINDOW_SYSTEM_NOT_INITIALIZED:
            sprintf(err, "Window system not initialized\n");
            showMesageBox(err);
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_CURRENT_WINDOWS_TOO_BIG:
            sprintf(err, "Current windows are too big for new resolution\n");
            showMesageBox(err);
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_INITIALIZING_DEFAULT_DATABASE:
            sprintf(err, "Error initializing default database.\n");
            showMesageBox(err);
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_8:
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_ALREADY_RUNNING:
            sprintf(err, "Program already running.\n");
            showMesageBox(err);
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_TITLE_NOT_SET:
            sprintf(err, "Program title not set.\n");
            showMesageBox(err);
            exit(1);
            break;
        case WINDOW_MANAGER_ERR_INITIALIZING_INPUT:
            sprintf(err, "Failure initializing input devices.\n");
            showMesageBox(err);
            exit(1);
            break;
        default:
            sprintf(err, "Unknown error code %d\n", rc);
            showMesageBox(err);
            exit(1);
            break;
        }
    }

    gWidgetFont = 100;
    fontSetCurrent(100);

    sub_48568C();

    sub_485288(sub_4670B8);

    for (i = 0; i < 64; i++) {
        for (j = 0; j < 256; j++) {
            byte_66E7B0[(i << 8) + j] = ((i * j) >> 9);
        }
    }
}

// 0x4B947C
void sub_4B947C()
{
    // TODO: Incomplete, but required for graceful exit.

    for (int index = 0; index < 16; index++) {
        STRUCT_6727B0* ptr = &(stru_6727B0[index]);
        if (ptr->window != -1) {
            // sub_4B78A4(ptr);
        }
    }

    dbExit();
    windowManagerExit();
}

// 0x4BB220
void sub_4BB220()
{
    sub_487BEC();
    // TODO: Incomplete.
    // sub_485704();
    // sub_4B6A54();
    sub_4B5C24();
}
