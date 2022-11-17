#include "int/intlib.h"

#include <stdio.h>

#include "int/window.h"
#include "plib/color/color.h"
#include "plib/gnw/input.h"
#include "int/datafile.h"
#include "plib/gnw/debug.h"
#include "int/dialog.h"
#include "int/support/intextra.h"
#include "int/memdbg.h"
#include "int/mousemgr.h"
#include "int/nevs.h"
#include "int/share1.h"
#include "int/sound.h"
#include "plib/gnw/text.h"
#include "plib/gnw/intrface.h"

#define INT_LIB_SOUNDS_CAPACITY 32
#define INT_LIB_KEY_HANDLERS_CAPACITY 256

typedef struct IntLibKeyHandlerEntry {
    Program* program;
    int proc;
} IntLibKeyHandlerEntry;

static void op_fillwin3x3(Program* program);
static void op_format(Program* program);
static void op_print(Program* program);
static void op_selectfilelist(Program* program);
static void op_tokenize(Program* program);
static void op_printrect(Program* program);
static void op_selectwin(Program* program);
static void op_display(Program* program);
static void op_displayraw(Program* program);
static void interpretFadePaletteBK(unsigned char* oldPalette, unsigned char* newPalette, int a3, float duration, int shouldProcessBk);
static void op_fadein(Program* program);
static void op_fadeout(Program* program);
static void op_movieflags(Program* program);
static void op_playmovie(Program* program);
static void op_playmovierect(Program* program);
static void op_stopmovie(Program* program);
static void op_addregionproc(Program* program);
static void op_addregionrightproc(Program* program);
static void op_createwin(Program* program);
static void op_resizewin(Program* program);
static void op_scalewin(Program* program);
static void op_deletewin(Program* program);
static void op_saystart(Program* program);
static void op_deleteregion(Program* program);
static void op_activateregion(Program* program);
static void op_checkregion(Program* program);
static void op_addregion(Program* program);
static void op_saystartpos(Program* program);
static void op_sayreplytitle(Program* program);
static void op_saygotoreply(Program* program);
static void op_sayreply(Program* program);
static void op_sayoption(Program* program);
static int checkDialog(Program* program);
static void op_sayend(Program* program);
static void op_saygetlastpos(Program* program);
static void op_sayquit(Program* program);
static void op_saymessagetimeout(Program* program);
static void op_saymessage(Program* program);
static void op_gotoxy(Program* program);
static void op_addbuttonflag(Program* program);
static void op_addregionflag(Program* program);
static void op_addbutton(Program* program);
static void op_addbuttontext(Program* program);
static void op_addbuttongfx(Program* program);
static void op_addbuttonproc(Program* program);
static void op_addbuttonrightproc(Program* program);
static void op_showwin(Program* program);
static void op_deletebutton(Program* program);
static void op_fillwin(Program* program);
static void op_fillrect(Program* program);
static void op_hidemouse(Program* program);
static void op_showmouse(Program* program);
static void op_mouseshape(Program* program);
static void op_setglobalmousefunc(Program* Program);
static void op_displaygfx(Program* program);
static void op_loadpalettetable(Program* program);
static void op_addNamedEvent(Program* program);
static void op_addNamedHandler(Program* program);
static void op_clearNamed(Program* program);
static void op_signalNamed(Program* program);
static void op_addkey(Program* program);
static void op_deletekey(Program* program);
static void op_refreshmouse(Program* program);
static void op_setfont(Program* program);
static void op_settextflags(Program* program);
static void op_settextcolor(Program* program);
static void op_sayoptioncolor(Program* program);
static void op_sayreplycolor(Program* program);
static void op_sethighlightcolor(Program* program);
static void op_sayreplywindow(Program* program);
static void op_sayreplyflags(Program* program);
static void op_sayoptionflags(Program* program);
static void op_sayoptionwindow(Program* program);
static void op_sayborder(Program* program);
static void op_sayscrollup(Program* program);
static void op_sayscrolldown(Program* program);
static void op_saysetspacing(Program* program);
static void op_sayrestart(Program* program);
static void soundCallbackInterpret(void* userData, int a2);
static int soundDeleteInterpret(int value);
static int soundPauseInterpret(int value);
static int soundRewindInterpret(int value);
static int soundUnpauseInterpret(int value);
static void op_soundplay(Program* program);
static void op_soundpause(Program* program);
static void op_soundresume(Program* program);
static void op_soundstop(Program* program);
static void op_soundrewind(Program* program);
static void op_sounddelete(Program* program);
static void op_setoneoptpause(Program* program);
static bool intLibDoInput(int key);

// 0x519038
static int TimeOut = 0;

// 0x59D5D0
static Sound* interpretSounds[INT_LIB_SOUNDS_CAPACITY];

// 0x59D650
static unsigned char blackPal[256 * 3];

// 0x59D950
static IntLibKeyHandlerEntry inputProc[INT_LIB_KEY_HANDLERS_CAPACITY];

// 0x59E150
static bool currentlyFadedIn;

// 0x59E154
static int anyKeyOffset;

// 0x59E158
static int numCallbacks;

// 0x59E15C
static Program* anyKeyProg;

// 0x59E160
static IntLibProgramDeleteCallback** callbacks;

// 0x59E164
static int sayStartingPosition;

// 0x461780
static void op_fillwin3x3(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        interpretError("Invalid type given to fillwin3x3");
    }

    char* fileName = interpretGetString(program, opcode, data);
    char* mangledFileName = interpretMangleName(fileName);

    int imageWidth;
    int imageHeight;
    unsigned char* imageData = loadDataFile(mangledFileName, &imageWidth, &imageHeight);
    if (imageData == NULL) {
        interpretError("cannot load 3x3 file '%s'", mangledFileName);
    }

    selectWindowID(program->windowId);

    alphaBltBufRect(imageData,
        imageWidth,
        imageHeight,
        windowGetBuffer(),
        windowWidth(),
        windowHeight());

    myfree(imageData, __FILE__, __LINE__); // "..\\int\\INTLIB.C", 94
}

// 0x461850
static void op_format(Program* program)
{
    opcode_t opcode[6];
    int data[6];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 6; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid arg 6 given to format\n");
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid arg 5 given to format\n");
    }

    if ((opcode[2] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid arg 4 given to format\n");
    }

    if ((opcode[3] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid arg 3 given to format\n");
    }

    if ((opcode[4] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid arg 2 given to format\n");
    }

    if ((opcode[5] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        interpretError("Invalid arg 1 given to format\n");
    }

    char* string = interpretGetString(program, opcode[5], data[5]);
    int x = data[4];
    int y = data[3];
    int width = data[2];
    int height = data[1];
    int textAlignment = data[0];

    if (!windowFormatMessage(string, x, y, width, height, textAlignment)) {
        interpretError("Error formatting message\n");
    }
}

// 0x461A5C
static void op_print(Program* program)
{
    selectWindowID(program->windowId);

    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    switch (opcode & VALUE_TYPE_MASK) {
    case VALUE_TYPE_STRING:
        interpretOutput("%s", interpretGetString(program, opcode, data));
        break;
    case VALUE_TYPE_FLOAT:
        interpretOutput("%.5f", *((float*)&data));
        break;
    case VALUE_TYPE_INT:
        interpretOutput("%d", data);
        break;
    }
}

// 0x461B10
static void op_selectfilelist(Program* program)
{
    program->flags |= PROGRAM_FLAG_0x20;

    opcode_t opcode[2];
    int data[2];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        interpretError("Error, invalid arg 2 given to selectfilelist");
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        interpretError("Error, invalid arg 1 given to selectfilelist");
    }

    char* pattern = interpretGetString(program, opcode[0], data[0]);
    char* title = interpretGetString(program, opcode[1], data[1]);

    int fileListLength;
    char** fileList = getFileList(interpretMangleName(pattern), &fileListLength);
    if (fileList != NULL && fileListLength != 0) {
        int selectedIndex = win_list_select(title,
            fileList,
            fileListLength,
            NULL,
            320 - text_width(title) / 2,
            200,
            colorTable[0x7FFF] | 0x10000);

        if (selectedIndex != -1) {
            interpretPushLong(program, interpretAddString(program, fileList[selectedIndex]));
            interpretPushShort(program, VALUE_TYPE_DYNAMIC_STRING);
        } else {
            interpretPushLong(program, 0);
            interpretPushShort(program, VALUE_TYPE_INT);
        }

        freeFileList(fileList);
    } else {
        interpretPushLong(program, 0);
        interpretPushShort(program, VALUE_TYPE_INT);
    }

    program->flags &= ~PROGRAM_FLAG_0x20;
}

// 0x461CA0
static void op_tokenize(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    opcode[0] = interpretPopShort(program);
    data[0] = interpretPopLong(program);

    if (opcode[0] == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode[0], data[0]);
    }

    if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Error, invalid arg 3 to tokenize.");
    }

    opcode[1] = interpretPopShort(program);
    data[1] = interpretPopLong(program);

    if (opcode[1] == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode[1], data[1]);
    }

    char* prev = NULL;
    if ((opcode[1] & VALUE_TYPE_MASK) == VALUE_TYPE_INT) {
        if (data[1] != 0) {
            interpretError("Error, invalid arg 2 to tokenize. (only accept 0 for int value)");
        }
    } else if ((opcode[1] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        prev = interpretGetString(program, opcode[1], data[1]);
    } else {
        interpretError("Error, invalid arg 2 to tokenize. (string)");
    }

    opcode[2] = interpretPopShort(program);
    data[2] = interpretPopLong(program);

    if (opcode[2] == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode[2], data[2]);
    }

    if ((opcode[2] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        interpretError("Error, invalid arg 1 to tokenize.");
    }

    char* string = interpretGetString(program, opcode[2], data[2]);
    char* temp = NULL;

    if (prev != NULL) {
        char* start = strstr(string, prev);
        if (start != NULL) {
            start += strlen(prev);
            while (*start != data[0] && *start != '\0') {
                start++;
            }
        }

        if (*start == data[0]) {
            int length = 0;
            char* end = start + 1;
            while (*end != data[0] && *end != '\0') {
                end++;
                length++;
            }

            temp = (char*)mycalloc(1, length + 1, __FILE__, __LINE__); // "..\\int\\INTLIB.C, 230
            strncpy(temp, start, length);
            interpretPushLong(program, interpretAddString(program, temp));
            interpretPushShort(program, VALUE_TYPE_DYNAMIC_STRING);
        } else {
            interpretPushLong(program, 0);
            interpretPushShort(program, VALUE_TYPE_INT);
        }
    } else {
        int length = 0;
        char* end = string;
        while (*end != data[0] && *end != '\0') {
            end++;
            length++;
        }

        if (string != NULL) {
            temp = (char*)mycalloc(1, length + 1, __FILE__, __LINE__); // "..\\int\\INTLIB.C", 248
            strncpy(temp, string, length);
            interpretPushLong(program, interpretAddString(program, temp));
            interpretPushShort(program, VALUE_TYPE_DYNAMIC_STRING);
        } else {
            interpretPushLong(program, 0);
            interpretPushShort(program, VALUE_TYPE_INT);
        }
    }

    if (temp != NULL) {
        myfree(temp, __FILE__, __LINE__); // "..\\int\\INTLIB.C" , 260
    }
}

// 0x461F1C
static void op_printrect(Program* program)
{
    selectWindowID(program->windowId);

    opcode_t opcode[3];
    int data[3];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_INT || data[0] > 2) {
        interpretError("Invalid arg 3 given to printrect, expecting int");
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid arg 2 given to printrect, expecting int");
    }

    char string[80];
    switch (opcode[2] & VALUE_TYPE_MASK) {
    case VALUE_TYPE_STRING:
        sprintf(string, "%s", interpretGetString(program, opcode[2], data[2]));
        break;
    case VALUE_TYPE_FLOAT:
        sprintf(string, "%.5f", *((float*)&data[2]));
        break;
    case VALUE_TYPE_INT:
        sprintf(string, "%d", data[2]);
        break;
    }

    if (!windowPrintRect(string, data[1], data[0])) {
        interpretError("Error in printrect");
    }
}

// 0x46209C
static void op_selectwin(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        interpretError("Invalid type given to select");
    }

    const char* windowName = interpretGetString(program, opcode, data);
    int win = pushWindow(windowName);
    if (win == -1) {
        interpretError("Error selecing window %s\n", interpretGetString(program, opcode, data));
    }

    program->windowId = win;

    interpretOutputFunc(windowOutput);
}

// 0x46213C
static void op_display(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        interpretError("Invalid type given to display");
    }

    char* fileName = interpretGetString(program, opcode, data);

    selectWindowID(program->windowId);

    char* mangledFileName = interpretMangleName(fileName);
    displayFile(mangledFileName);
}

// 0x4621B4
static void op_displayraw(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        interpretError("Invalid type given to displayraw");
    }

    char* fileName = interpretGetString(program, opcode, data);

    selectWindowID(program->windowId);

    char* mangledFileName = interpretMangleName(fileName);
    displayFileRaw(mangledFileName);
}

// 0x46222C
static void interpretFadePaletteBK(unsigned char* oldPalette, unsigned char* newPalette, int a3, float duration, int shouldProcessBk)
{
    unsigned int time;
    unsigned int previousTime;
    unsigned int delta;
    int step;
    int steps;
    int index;
    unsigned char palette[256 * 3];

    time = get_time();
    previousTime = time;
    steps = (int)duration;
    step = 0;
    delta = 0;

    if (duration != 0.0) {
        while (step < steps) {
            if (delta != 0) {
                for (index = 0; index < 768; index++) {
                    palette[index] = oldPalette[index] - (oldPalette[index] - newPalette[index]) * step / steps;
                }

                setSystemPalette(palette);

                previousTime = time;
                step += delta;
            }

            if (shouldProcessBk) {
                process_bk();
            }

            time = get_time();
            delta = time - previousTime;
        }
    }

    setSystemPalette(newPalette);
}

// NOTE: Unused.
//
// 0x462330
void interpretFadePalette(unsigned char* oldPalette, unsigned char* newPalette, int a3, float duration)
{
    interpretFadePaletteBK(oldPalette, newPalette, a3, duration, 1);
}

// NOTE: Unused.
int intlibGetFadeIn()
{
    return currentlyFadedIn;
}

// NOTE: Inlined.
//
// 0x462348
void interpretFadeOut(float duration)
{
    int cursorWasHidden;

    cursorWasHidden = mouse_hidden();
    mouse_hide();

    interpretFadePaletteBK(getSystemPalette(), blackPal, 64, duration, 1);

    if (!cursorWasHidden) {
        mouse_show();
    }
}

// NOTE: Inlined.
//
// 0x462380
void interpretFadeIn(float duration)
{
    interpretFadePaletteBK(blackPal, cmap, 64, duration, 1);
}

// NOTE: Unused.
//
// 0x4623A4
void interpretFadeOutNoBK(float duration)
{
    int cursorWasHidden;

    cursorWasHidden = mouse_hidden();
    mouse_hide();

    interpretFadePaletteBK(getSystemPalette(), blackPal, 64, duration, 0);

    if (!cursorWasHidden) {
        mouse_show();
    }
}

// NOTE: Unused.
//
// 0x4623DC
void interpretFadeInNoBK(float duration)
{
    interpretFadePaletteBK(blackPal, cmap, 64, duration, 0);
}

// 0x462400
static void op_fadein(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid type given to fadein\n");
    }

    program->flags |= PROGRAM_FLAG_0x20;

    setSystemPalette(blackPal);

    // NOTE: Uninline.
    interpretFadeIn((float)data);

    currentlyFadedIn = true;

    program->flags &= ~PROGRAM_FLAG_0x20;
}

// 0x4624B4
static void op_fadeout(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        // FIXME: Wrong function name, should be fadeout.
        interpretError("Invalid type given to fadein\n");
    }

    program->flags |= PROGRAM_FLAG_0x20;

    // NOTE: Uninline.
    interpretFadeOut((float)data);

    currentlyFadedIn = false;

    program->flags &= ~PROGRAM_FLAG_0x20;
}

// 0x462570
int checkMovie(Program* program)
{
    if (dialogGetDialogDepth() > 0) {
        return 1;
    }

    return windowMoviePlaying();
}

// 0x462584
static void op_movieflags(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if (!windowSetMovieFlags(data)) {
        interpretError("Error setting movie flags\n");
    }
}

// 0x4625D0
static void op_playmovie(Program* program)
{
    // 0x59E168
    static char name[100];

    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        interpretError("Invalid type given to playmovie");
    }

    strcpy(name, interpretGetString(program, opcode, data));

    if (strrchr(name, '.') == NULL) {
        strcat(name, ".mve");
    }

    selectWindowID(program->windowId);

    program->flags |= PROGRAM_IS_WAITING;
    program->checkWaitFunc = checkMovie;

    char* mangledFileName = interpretMangleName(name);
    if (!windowPlayMovie(mangledFileName)) {
        interpretError("Error playing movie");
    }
}

// 0x4626C4
static void op_playmovierect(Program* program)
{
    // 0x59E1CC
    static char name[100];

    opcode_t opcode[5];
    int data[5];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 5; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[4] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        // FIXME: Wrong function name, should playmovierect.
        interpretError("Invalid arg given to playmovie");
    }

    strcpy(name, interpretGetString(program, opcode[4], data[4]));

    if (strrchr(name, '.') == NULL) {
        strcat(name, ".mve");
    }

    selectWindowID(program->windowId);

    program->checkWaitFunc = checkMovie;
    program->flags |= PROGRAM_IS_WAITING;

    char* mangledFileName = interpretMangleName(name);
    if (!windowPlayMovieRect(mangledFileName, data[3], data[2], data[1], data[0])) {
        interpretError("Error playing movie");
    }
}

// 0x46287C
static void op_stopmovie(Program* program)
{
    windowStopMovie();
    program->flags |= PROGRAM_FLAG_0x40;
}

// 0x462890
static void op_deleteregion(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        if ((opcode & VALUE_TYPE_MASK) == VALUE_TYPE_INT && data != -1) {
            interpretError("Invalid type given to deleteregion");
        }
    }

    selectWindowID(program->windowId);

    const char* regionName = data != -1 ? interpretGetString(program, opcode, data) : NULL;
    windowDeleteRegion(regionName);
}

// 0x462924
static void op_activateregion(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    const char* regionName = interpretGetString(program, opcode[1], data[1]);
    windowActivateRegion(regionName, data[0]);
}

// 0x4629A0
static void op_checkregion(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        interpretError("Invalid arg 1 given to checkregion();\n");
    }

    const char* regionName = interpretGetString(program, opcode, data);

    bool regionExists = windowCheckRegionExists(regionName);
    interpretPushLong(program, regionExists);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x462A1C
static void op_addregion(Program* program)
{
    opcode_t opcode;
    int data;

    opcode = interpretPopShort(program);
    data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid number of elements given to region");
    }

    int args = data;

    if (args < 2) {
        interpretError("addregion call without enough points!");
    }

    selectWindowID(program->windowId);

    windowStartRegion(args / 2);

    while (args >= 2) {
        opcode = interpretPopShort(program);
        data = interpretPopLong(program);

        if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode, data);
        }

        if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("Invalid y value given to region");
        }

        int y = data;

        opcode = interpretPopShort(program);
        data = interpretPopLong(program);

        if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode, data);
        }

        if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("Invalid x value given to region");
        }

        int x = data;

        y = (y * windowGetYres() + 479) / 480;
        x = (x * windowGetXres() + 639) / 640;
        args -= 2;

        windowAddRegionPoint(x, y, true);
    }

    if (args == 0) {
        interpretError("Unnamed regions not allowed\n");
        windowEndRegion();
    } else {
        opcode = interpretPopShort(program);
        data = interpretPopLong(program);

        if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode, data);
        }

        if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_STRING && opcode == VALUE_TYPE_INT) {
            if (data != 0) {
                interpretError("Invalid name given to region");
            }
        }

        const char* regionName = interpretGetString(program, opcode, data);
        windowAddRegionName(regionName);
        windowEndRegion();
    }
}

// 0x462C10
static void op_addregionproc(Program* program)
{
    opcode_t opcode[5];
    int data[5];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 5; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid procedure 4 name given to addregionproc");
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid procedure 3 name given to addregionproc");
    }

    if ((opcode[2] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid procedure 2 name given to addregionproc");
    }

    if ((opcode[3] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid procedure 1 name given to addregionproc");
    }

    if ((opcode[4] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        interpretError("Invalid name given to addregionproc");
    }

    const char* regionName = interpretGetString(program, opcode[4], data[4]);
    selectWindowID(program->windowId);

    if (!windowAddRegionProc(regionName, program, data[3], data[2], data[1], data[0])) {
        interpretError("Error setting procedures to region %s\n", regionName);
    }
}

// 0x462DDC
static void op_addregionrightproc(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid procedure 2 name given to addregionrightproc");
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid procedure 1 name given to addregionrightproc");
    }

    if ((opcode[2] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        interpretError("Invalid name given to addregionrightproc");
    }

    const char* regionName = interpretGetString(program, opcode[2], data[2]);
    selectWindowID(program->windowId);

    if (!windowAddRegionRightProc(regionName, program, data[1], data[0])) {
        interpretError("ErrorError setting right button procedures to region %s\n", regionName);
    }
}

// 0x462F08
static void op_createwin(Program* program)
{
    opcode_t opcode[5];
    int data[5];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 5; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    const char* windowName = interpretGetString(program, opcode[4], data[4]);
    int x = (data[3] * windowGetXres() + 639) / 640;
    int y = (data[2] * windowGetYres() + 479) / 480;
    int width = (data[1] * windowGetXres() + 639) / 640;
    int height = (data[0] * windowGetYres() + 479) / 480;

    if (createWindow(windowName, x, y, width, height, colorTable[0], 0) == -1) {
        interpretError("Couldn't create window.");
    }
}

// 0x46308C
static void op_resizewin(Program* program)
{
    opcode_t opcode[5];
    int data[5];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 5; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    const char* windowName = interpretGetString(program, opcode[4], data[4]);
    int x = (data[3] * windowGetXres() + 639) / 640;
    int y = (data[2] * windowGetYres() + 479) / 480;
    int width = (data[1] * windowGetXres() + 639) / 640;
    int height = (data[0] * windowGetYres() + 479) / 480;

    if (resizeWindow(windowName, x, y, width, height) == -1) {
        interpretError("Couldn't resize window.");
    }
}

// 0x463204
static void op_scalewin(Program* program)
{
    opcode_t opcode[5];
    int data[5];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 5; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    const char* windowName = interpretGetString(program, opcode[4], data[4]);
    int x = (data[3] * windowGetXres() + 639) / 640;
    int y = (data[2] * windowGetYres() + 479) / 480;
    int width = (data[1] * windowGetXres() + 639) / 640;
    int height = (data[0] * windowGetYres() + 479) / 480;

    if (scaleWindow(windowName, x, y, width, height) == -1) {
        interpretError("Couldn't scale window.");
    }
}

// 0x46337C
static void op_deletewin(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    const char* windowName = interpretGetString(program, opcode, data);

    if (!deleteWindow(windowName)) {
        interpretError("Error deleting window %s\n", windowName);
    }

    program->windowId = popWindow();
}

// 0x4633E4
static void op_saystart(Program* program)
{
    sayStartingPosition = 0;

    program->flags |= PROGRAM_FLAG_0x20;
    int rc = dialogStart(program);
    program->flags &= ~PROGRAM_FLAG_0x20;

    if (rc != 0) {
        interpretError("Error starting dialog.");
    }
}

// 0x463430
static void op_saystartpos(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    sayStartingPosition = data;

    program->flags |= PROGRAM_FLAG_0x20;
    int rc = dialogStart(program);
    program->flags &= ~PROGRAM_FLAG_0x20;

    if (rc != 0) {
        interpretError("Error starting dialog.");
    }
}

// 0x46349C
static void op_sayreplytitle(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    char* string = NULL;
    if ((opcode & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        string = interpretGetString(program, opcode, data);
    }

    if (dialogTitle(string) != 0) {
        interpretError("Error setting title.");
    }
}

// 0x463510
static void op_saygotoreply(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    char* string = NULL;
    if ((opcode & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        string = interpretGetString(program, opcode, data);
    }

    if (dialogGotoReply(string) != 0) {
        interpretError("Error during goto, couldn't find reply target %s", string);
    }
}

// 0x463584
static void op_sayreply(Program* program)
{
    program->flags |= PROGRAM_FLAG_0x20;

    opcode_t opcode[2];
    int data[2];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    const char* v1;
    if ((opcode[1] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        v1 = interpretGetString(program, opcode[1], data[1]);
    } else {
        v1 = NULL;
    }

    if ((opcode[0] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        const char* v2 = interpretGetString(program, opcode[0], data[0]);
        if (dialogOption(v1, v2) != 0) {
            program->flags &= ~PROGRAM_FLAG_0x20;
            interpretError("Error setting option.");
        }
    } else if ((opcode[0] & VALUE_TYPE_MASK) == VALUE_TYPE_INT) {
        if (dialogOptionProc(v1, data[0]) != 0) {
            program->flags &= ~PROGRAM_FLAG_0x20;
            interpretError("Error setting option.");
        }
    } else {
        interpretError("Invalid arg 2 to sayOption");
    }

    program->flags &= ~PROGRAM_FLAG_0x20;
}

// 0x4636A0
static void op_sayoption(Program* program)
{
    program->flags |= PROGRAM_FLAG_0x20;

    opcode_t opcode[2];
    int data[2];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    const char* v1;
    if ((opcode[1] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        v1 = interpretGetString(program, opcode[1], data[1]);
    } else {
        v1 = NULL;
    }

    const char* v2;
    if ((opcode[0] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        v2 = interpretGetString(program, opcode[0], data[0]);
    } else {
        v2 = NULL;
    }

    if (dialogReply(v1, v2) != 0) {
        program->flags &= ~PROGRAM_FLAG_0x20;
        interpretError("Error setting option.");
    }

    program->flags &= ~PROGRAM_FLAG_0x20;
}

// 0x46378C
static int checkDialog(Program* program)
{
    program->flags |= PROGRAM_FLAG_0x40;
    return dialogGetDialogDepth() != -1;
}

// 0x4637A4
static void op_sayend(Program* program)
{
    program->flags |= PROGRAM_FLAG_0x20;
    int rc = dialogGo(sayStartingPosition);
    program->flags &= ~PROGRAM_FLAG_0x20;

    if (rc == -2) {
        program->checkWaitFunc = checkDialog;
        program->flags |= PROGRAM_IS_WAITING;
    }
}

// 0x4637EC
static void op_saygetlastpos(Program* program)
{
    int value = dialogGetExitPoint();
    interpretPushLong(program, value);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x463810
static void op_sayquit(Program* program)
{
    if (dialogQuit() != 0) {
        interpretError("Error quitting option.");
    }
}

// NOTE: Unused.
//
// 0x463828
int getTimeOut()
{
    return TimeOut;
}

// NOTE: Unused.
//
// 0x463830
void setTimeOut(int value)
{
    TimeOut = value;
}

// 0x463838
static void op_saymessagetimeout(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    // TODO: What the hell is this?
    if ((opcode & VALUE_TYPE_MASK) == 0x4000) {
        interpretError("sayMsgTimeout:  invalid var type passed.");
    }

    TimeOut = data;
}

// 0x463890
static void op_saymessage(Program* program)
{
    program->flags |= PROGRAM_FLAG_0x20;

    opcode_t opcode[2];
    int data[2];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    const char* v1;
    if ((opcode[1] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        v1 = interpretGetString(program, opcode[1], data[1]);
    } else {
        v1 = NULL;
    }

    const char* v2;
    if ((opcode[0] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        v2 = interpretGetString(program, opcode[0], data[0]);
    } else {
        v2 = NULL;
    }

    if (dialogMessage(v1, v2, TimeOut) != 0) {
        program->flags &= ~PROGRAM_FLAG_0x20;
        interpretError("Error setting option.");
    }

    program->flags &= ~PROGRAM_FLAG_0x20;
}

// 0x463980
static void op_gotoxy(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_INT
        || (opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid operand given to gotoxy");
    }

    selectWindowID(program->windowId);

    int x = data[1];
    int y = data[0];
    windowGotoXY(x, y);
}

// 0x463A38
static void op_addbuttonflag(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid arg 2 given to addbuttonflag");
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        interpretError("Invalid arg 1 given to addbuttonflag");
    }

    const char* buttonName = interpretGetString(program, opcode[1], data[1]);
    if (!windowSetButtonFlag(buttonName, data[0])) {
        // NOTE: Original code calls interpretGetString one more time with the
        // same params.
        interpretError("Error setting flag on button %s", buttonName);
    }
}

// 0x463B10
static void op_addregionflag(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid arg 2 given to addregionflag");
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        interpretError("Invalid arg 1 given to addregionflag");
    }

    const char* regionName = interpretGetString(program, opcode[1], data[1]);
    if (!windowSetRegionFlag(regionName, data[0])) {
        // NOTE: Original code calls interpretGetString one more time with the
        // same params.
        interpretError("Error setting flag on region %s", regionName);
    }
}

// 0x463BE8
static void op_addbutton(Program* program)
{
    opcode_t opcode[5];
    int data[5];

    opcode[0] = interpretPopShort(program);
    data[0] = interpretPopLong(program);

    if (opcode[0] == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode[0], data[0]);
    }

    if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid height given to addbutton");
    }

    opcode[1] = interpretPopShort(program);
    data[1] = interpretPopLong(program);

    if (opcode[1] == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode[1], data[1]);
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid width given to addbutton");
    }

    opcode[2] = interpretPopShort(program);
    data[2] = interpretPopLong(program);

    if (opcode[2] == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode[2], data[2]);
    }

    if ((opcode[2] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid y given to addbutton");
    }

    opcode[3] = interpretPopShort(program);
    data[3] = interpretPopLong(program);

    if (opcode[3] == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode[3], data[3]);
    }

    if ((opcode[3] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid x given to addbutton");
    }

    opcode[4] = interpretPopShort(program);
    data[4] = interpretPopLong(program);

    if (opcode[4] == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode[4], data[4]);
    }

    if ((opcode[4] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        interpretError("Invalid name given to addbutton");
    }

    selectWindowID(program->windowId);

    int height = (data[0] * windowGetYres() + 479) / 480;
    int width = (data[1] * windowGetXres() + 639) / 640;
    int y = (data[2] * windowGetYres() + 479) / 480;
    int x = (data[3] * windowGetXres() + 639) / 640;
    const char* buttonName = interpretGetString(program, opcode[4], data[4]);

    windowAddButton(buttonName, x, y, width, height, 0);
}

// 0x463DF4
static void op_addbuttontext(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        interpretError("Invalid text string given to addbuttontext");
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        interpretError("Invalid name string given to addbuttontext");
    }

    const char* text = interpretGetString(program, opcode[0], data[0]);
    const char* buttonName = interpretGetString(program, opcode[1], data[1]);

    if (!windowAddButtonText(buttonName, text)) {
        interpretError("Error setting text to button %s\n",
            interpretGetString(program, opcode[1], data[1]));
    }
}

// 0x463EEC
static void op_addbuttongfx(Program* program)
{
    opcode_t opcode[4];
    int data[4];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 4; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if (((opcode[2] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING || ((opcode[2] & VALUE_TYPE_MASK) == VALUE_TYPE_INT && data[2] == 0))
        || ((opcode[1] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING || ((opcode[1] & VALUE_TYPE_MASK) == VALUE_TYPE_INT && data[1] == 0))
        || ((opcode[0] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING || ((opcode[0] & VALUE_TYPE_MASK) == VALUE_TYPE_INT && data[0] == 0))) {

        if ((opcode[3] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
            // FIXME: Wrong function name, should be addbuttongfx.
            interpretError("Invalid name given to addbuttontext");
        }

        const char* buttonName = interpretGetString(program, opcode[3], data[3]);
        char* pressedFileName = interpretMangleName(interpretGetString(program, opcode[2], data[2]));
        char* normalFileName = interpretMangleName(interpretGetString(program, opcode[1], data[1]));
        char* hoverFileName = interpretMangleName(interpretGetString(program, opcode[0], data[0]));

        selectWindowID(program->windowId);

        if (!windowAddButtonGfx(buttonName, pressedFileName, normalFileName, hoverFileName)) {
            interpretError("Error setting graphics to button %s\n", buttonName);
        }
    } else {
        interpretError("Invalid filename given to addbuttongfx");
    }
}

// 0x4640DC
static void op_addbuttonproc(Program* program)
{
    opcode_t opcode[5];
    int data[5];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 5; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid procedure 4 name given to addbuttonproc");
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid procedure 3 name given to addbuttonproc");
    }

    if ((opcode[2] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid procedure 2 name given to addbuttonproc");
    }

    if ((opcode[3] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid procedure 1 name given to addbuttonproc");
    }

    if ((opcode[4] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        interpretError("Invalid name given to addbuttonproc");
    }

    const char* buttonName = interpretGetString(program, opcode[4], data[4]);
    selectWindowID(program->windowId);

    if (!windowAddButtonProc(buttonName, program, data[3], data[2], data[1], data[0])) {
        interpretError("Error setting procedures to button %s\n", buttonName);
    }
}

// 0x4642A8
static void op_addbuttonrightproc(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid procedure 2 name given to addbuttonrightproc");
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid procedure 1 name given to addbuttonrightproc");
    }

    if ((opcode[2] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        interpretError("Invalid name given to addbuttonrightproc");
    }

    const char* regionName = interpretGetString(program, opcode[2], data[2]);
    selectWindowID(program->windowId);

    if (!windowAddRegionRightProc(regionName, program, data[1], data[0])) {
        interpretError("Error setting right button procedures to button %s\n", regionName);
    }
}

// 0x4643D4
static void op_showwin(Program* program)
{
    selectWindowID(program->windowId);
    windowDraw();
}

// 0x4643E4
static void op_deletebutton(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        if ((opcode & VALUE_TYPE_MASK) == VALUE_TYPE_INT && data != -1) {
            interpretError("Invalid type given to delete button");
        }
    }

    selectWindowID(program->windowId);

    if ((opcode & VALUE_TYPE_MASK) == VALUE_TYPE_INT) {
        if (windowDeleteButton(NULL)) {
            return;
        }
    } else {
        const char* buttonName = interpretGetString(program, opcode, data);
        if (windowDeleteButton(buttonName)) {
            return;
        }
    }

    interpretError("Error deleting button");
}

// 0x46449C
static void op_fillwin(Program* program)
{
    opcode_t opcode[3];
    int data[3];
    float* floats = (float*)data;

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[2] & VALUE_TYPE_MASK) != VALUE_TYPE_FLOAT) {
        if ((opcode[2] & VALUE_TYPE_MASK) == VALUE_TYPE_INT) {
            if (data[2] == 1) {
                floats[2] = 1.0;
            } else if (data[2] != 0) {
                interpretError("Invalid red value given to fillwin");
            }
        }
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_FLOAT) {
        if ((opcode[1] & VALUE_TYPE_MASK) == VALUE_TYPE_INT) {
            if (data[1] == 1) {
                floats[1] = 1.0;
            } else if (data[1] != 0) {
                interpretError("Invalid green value given to fillwin");
            }
        }
    }

    if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_FLOAT) {
        if ((opcode[0] & VALUE_TYPE_MASK) == VALUE_TYPE_INT) {
            if (data[0] == 1) {
                floats[0] = 1.0;
            } else if (data[0] != 0) {
                interpretError("Invalid blue value given to fillwin");
            }
        }
    }

    selectWindowID(program->windowId);

    windowFill(floats[2], floats[1], floats[0]);
}

// 0x4645FC
static void op_fillrect(Program* program)
{
    opcode_t opcode[7];
    int data[7];
    float* floats = (float*)data;

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 7; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[2] & VALUE_TYPE_MASK) != VALUE_TYPE_FLOAT) {
        if ((opcode[2] & VALUE_TYPE_MASK) == VALUE_TYPE_INT) {
            if (data[2] == 1) {
                floats[2] = 1.0;
            } else if (data[2] != 0) {
                interpretError("Invalid red value given to fillrect");
            }
        }
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_FLOAT) {
        if ((opcode[1] & VALUE_TYPE_MASK) == VALUE_TYPE_INT) {
            if (data[1] == 1) {
                floats[1] = 1.0;
            } else if (data[1] != 0) {
                interpretError("Invalid green value given to fillrect");
            }
        }
    }

    if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_FLOAT) {
        if ((opcode[0] & VALUE_TYPE_MASK) == VALUE_TYPE_INT) {
            if (data[0] == 1) {
                floats[0] = 1.0;
            } else if (data[0] != 0) {
                interpretError("Invalid blue value given to fillrect");
            }
        }
    }

    if ((opcode[6] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid arg 1 given to fillrect");
    }

    if ((opcode[5] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid arg 2 given to fillrect");
    }

    if ((opcode[4] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid arg 3 given to fillrect");
    }
    if ((opcode[3] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid arg 4 given to fillrect");
    }

    selectWindowID(program->windowId);

    windowFillRect(data[6], data[5], data[4], data[3], floats[2], floats[1], floats[0]);
}

// 0x46489C
static void op_hidemouse(Program* program)
{
    mouse_hide();
}

// 0x4648A4
static void op_showmouse(Program* program)
{
    mouse_show();
}

// 0x4648AC
static void op_mouseshape(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid arg 3 given to mouseshape");
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid arg 2 given to mouseshape");
    }

    if ((opcode[2] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        interpretError("Invalid filename given to mouseshape");
    }

    char* fileName = interpretGetString(program, opcode[2], data[2]);
    if (!mouseSetMouseShape(fileName, data[1], data[0])) {
        interpretError("Error loading mouse shape.");
    }
}

// 0x4649C4
static void op_setglobalmousefunc(Program* Program)
{
    interpretError("setglobalmousefunc not defined");
}

// 0x4649D4
static void op_displaygfx(Program* program)
{
    opcode_t opcode[5];
    int data[5];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 5; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    char* fileName = interpretGetString(program, opcode[4], data[4]);
    char* mangledFileName = interpretMangleName(fileName);
    windowDisplay(mangledFileName, data[3], data[2], data[1], data[0]);
}

// 0x464ADC
static void op_loadpalettetable(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        if ((opcode & VALUE_TYPE_MASK) == VALUE_TYPE_INT && data != -1) {
            interpretError("Invalid type given to loadpalettetable");
        }
    }

    char* path = interpretGetString(program, opcode, data);
    if (!loadColorTable(path)) {
        interpretError(colorError());
    }
}

// 0x464B54
static void op_addNamedEvent(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        interpretError("Invalid type given to addnamedevent");
    }

    const char* v1 = interpretGetString(program, opcode[1], data[1]);
    nevs_addevent(v1, program, data[0], NEVS_TYPE_EVENT);
}

// 0x464BE8
static void op_addNamedHandler(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        interpretError("Invalid type given to addnamedhandler");
    }

    const char* v1 = interpretGetString(program, opcode[1], data[1]);
    nevs_addevent(v1, program, data[0], NEVS_TYPE_HANDLER);
}

// 0x464C80
static void op_clearNamed(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        interpretError("Invalid type given to clearnamed");
    }

    char* string = interpretGetString(program, opcode, data);
    nevs_clearevent(string);
}

// 0x464CE4
static void op_signalNamed(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        interpretError("Invalid type given to signalnamed");
    }

    char* str = interpretGetString(program, opcode, data);
    nevs_signal(str);
}

// 0x464D48
static void op_addkey(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    for (int arg = 0; arg < 2; arg++) {
        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("Invalid arg %d given to addkey", arg + 1);
        }
    }

    int key = data[1];
    int proc = data[0];

    if (key == -1) {
        anyKeyOffset = proc;
        anyKeyProg = program;
    } else {
        if (key > INT_LIB_KEY_HANDLERS_CAPACITY - 1) {
            interpretError("Key out of range");
        }

        inputProc[key].program = program;
        inputProc[key].proc = proc;
    }
}

// 0x464E24
static void op_deletekey(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid arg 1 given to deletekey");
    }

    int key = data;

    if (key == -1) {
        anyKeyOffset = 0;
        anyKeyProg = NULL;
    } else {
        if (key > INT_LIB_KEY_HANDLERS_CAPACITY - 1) {
            interpretError("Key out of range");
        }

        inputProc[key].program = NULL;
        inputProc[key].proc = 0;
    }
}

// 0x464EB0
static void op_refreshmouse(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid arg 1 given to refreshmouse");
    }

    if (!windowRefreshRegions()) {
        executeProc(program, data);
    }
}

// 0x464F18
static void op_setfont(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid arg 1 given to setfont");
    }

    if (!windowSetFont(data)) {
        interpretError("Error setting font");
    }
}

// 0x464F84
static void op_settextflags(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid arg 1 given to setflags");
    }

    if (!windowSetTextFlags(data)) {
        interpretError("Error setting text flags");
    }
}

// 0x464FF0
static void op_settextcolor(Program* program)
{
    opcode_t opcode[3];
    int data[3];
    float* floats = (float*)data;

    // NOTE: Original code does not use loops.
    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    for (int arg = 0; arg < 3; arg++) {
        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_FLOAT
            && (opcode[arg] & VALUE_TYPE_MASK) == VALUE_TYPE_INT
            && data[arg] != 0) {
            interpretError("Invalid type given to settextcolor");
        }
    }

    float r = floats[2];
    float g = floats[1];
    float b = floats[0];

    if (!windowSetTextColor(r, g, b)) {
        interpretError("Error setting text color");
    }
}

// 0x465140
static void op_sayoptioncolor(Program* program)
{
    opcode_t opcode[3];
    int data[3];
    float* floats = (float*)data;

    // NOTE: Original code does not use loops.
    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    for (int arg = 0; arg < 3; arg++) {
        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_FLOAT
            && (opcode[arg] & VALUE_TYPE_MASK) == VALUE_TYPE_INT
            && data[arg] != 0) {
            interpretError("Invalid type given to sayoptioncolor");
        }
    }

    float r = floats[2];
    float g = floats[1];
    float b = floats[0];

    if (dialogSetOptionColor(r, g, b)) {
        interpretError("Error setting option color");
    }
}

// 0x465290
static void op_sayreplycolor(Program* program)
{
    opcode_t opcode[3];
    int data[3];
    float* floats = (float*)data;

    // NOTE: Original code does not use loops.
    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);
        ;

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    for (int arg = 0; arg < 3; arg++) {
        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_FLOAT
            && (opcode[arg] & VALUE_TYPE_MASK) == VALUE_TYPE_INT
            && data[arg] != 0) {
            interpretError("Invalid type given to sayreplycolor");
        }
    }

    float r = floats[2];
    float g = floats[1];
    float b = floats[0];

    if (dialogSetReplyColor(r, g, b) != 0) {
        interpretError("Error setting reply color");
    }
}

// 0x4653E0
static void op_sethighlightcolor(Program* program)
{
    opcode_t opcode[3];
    int data[3];
    float* floats = (float*)data;

    // NOTE: Original code does not use loops.
    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    for (int arg = 0; arg < 3; arg++) {
        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_FLOAT
            && (opcode[arg] & VALUE_TYPE_MASK) == VALUE_TYPE_INT
            && data[arg] != 0) {
            interpretError("Invalid type given to sethighlightcolor");
        }
    }

    float r = floats[2];
    float g = floats[1];
    float b = floats[0];

    if (!windowSetHighlightColor(r, g, b)) {
        interpretError("Error setting text highlight color");
    }
}

// 0x465530
static void op_sayreplywindow(Program* program)
{
    opcode_t opcode[5];
    int data[5];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 5; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    char* v1;
    if ((opcode[0] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        v1 = interpretGetString(program, opcode[0], data[0]);
        v1 = interpretMangleName(v1);
        v1 = mystrdup(v1, __FILE__, __LINE__); // "..\\int\\INTLIB.C", 1510
    } else if ((opcode[0] & VALUE_TYPE_MASK) == VALUE_TYPE_INT && data[0] == 0) {
        v1 = NULL;
    } else {
        interpretError("Invalid arg 5 given to sayreplywindow");
    }

    if (dialogSetReplyWindow(data[4], data[3], data[2], data[1], v1) != 0) {
        interpretError("Error setting reply window");
    }
}

// 0x465688
static void op_sayreplyflags(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid arg 1 given to sayreplyflags");
    }

    if (!dialogSetReplyFlags(data)) {
        interpretError("Error setting reply flags");
    }
}

// 0x4656F4
static void op_sayoptionflags(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid arg 1 given to sayoptionflags");
    }

    if (!dialogSetOptionFlags(data)) {
        interpretError("Error setting option flags");
    }
}

// 0x465760
static void op_sayoptionwindow(Program* program)
{
    opcode_t opcode[5];
    int data[5];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 5; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    char* v1;
    if ((opcode[0] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        v1 = interpretGetString(program, opcode[0], data[0]);
        v1 = interpretMangleName(v1);
        v1 = mystrdup(v1, __FILE__, __LINE__); // "..\\int\\INTLIB.C", 1556
    } else if ((opcode[0] & VALUE_TYPE_MASK) == VALUE_TYPE_INT && data[0] == 0) {
        v1 = NULL;
    } else {
        interpretError("Invalid arg 5 given to sayoptionwindow");
    }

    if (dialogSetOptionWindow(data[4], data[3], data[2], data[1], v1) != 0) {
        interpretError("Error setting option window");
    }
}

// 0x4658B8
static void op_sayborder(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    // NOTE: Original code does not use loops.
    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    for (int arg = 0; arg < 2; arg++) {
        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            interpretError("Invalid arg %d given to sayborder", arg + 1);
        }
    }

    if (dialogSetBorder(data[1], data[0]) != 0) {
        interpretError("Error setting dialog border");
    }
}

// 0x465978
static void op_sayscrollup(Program* program)
{
    opcode_t opcode[6];
    int data[6];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 6; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    char* v1 = NULL;
    char* v2 = NULL;
    char* v3 = NULL;
    char* v4 = NULL;
    int v5 = 0;

    if ((opcode[0] & VALUE_TYPE_MASK) == VALUE_TYPE_INT) {
        if (data[0] != -1 && data[0] != 0) {
            interpretError("Invalid arg 4 given to sayscrollup");
        }

        if (data[0] == -1) {
            v5 = 1;
        }
    } else {
        if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
            interpretError("Invalid arg 4 given to sayscrollup");
        }
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING && (opcode[1] & VALUE_TYPE_MASK) == VALUE_TYPE_INT && data[1] != 0) {
        interpretError("Invalid arg 3 given to sayscrollup");
    }

    if ((opcode[2] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING && (opcode[2] & VALUE_TYPE_MASK) == VALUE_TYPE_INT && data[2] != 0) {
        interpretError("Invalid arg 2 given to sayscrollup");
    }

    if ((opcode[3] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING && (opcode[3] & VALUE_TYPE_MASK) == VALUE_TYPE_INT && data[3] != 0) {
        interpretError("Invalid arg 1 given to sayscrollup");
    }

    if ((opcode[3] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        v1 = interpretGetString(program, opcode[3], data[3]);
        v1 = interpretMangleName(v1);
        v1 = mystrdup(v1, __FILE__, __LINE__); // "..\\int\\INTLIB.C", 1611
    }

    if ((opcode[2] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        v2 = interpretGetString(program, opcode[2], data[2]);
        v2 = interpretMangleName(v2);
        v2 = mystrdup(v2, __FILE__, __LINE__); // "..\\int\\INTLIB.C", 1613
    }

    if ((opcode[1] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        v3 = interpretGetString(program, opcode[1], data[1]);
        v3 = interpretMangleName(v3);
        v3 = mystrdup(v3, __FILE__, __LINE__); // "..\\int\\INTLIB.C", 1615
    }

    if ((opcode[0] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        v4 = interpretGetString(program, opcode[0], data[0]);
        v4 = interpretMangleName(v4);
        v4 = mystrdup(v4, __FILE__, __LINE__); // "..\\int\\INTLIB.C", 1617
    }

    if (dialogSetScrollUp(data[5], data[4], v1, v2, v3, v4, v5) != 0) {
        interpretError("Error setting scroll up");
    }
}

// 0x465CAC
static void op_sayscrolldown(Program* program)
{
    opcode_t opcode[6];
    int data[6];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 6; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    char* v1 = NULL;
    char* v2 = NULL;
    char* v3 = NULL;
    char* v4 = NULL;
    int v5 = 0;

    if ((opcode[0] & VALUE_TYPE_MASK) == VALUE_TYPE_INT) {
        if (data[0] != -1 && data[0] != 0) {
            // FIXME: Wrong function name, should be sayscrolldown.
            interpretError("Invalid arg 4 given to sayscrollup");
        }

        if (data[0] == -1) {
            v5 = 1;
        }
    } else {
        if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
            // FIXME: Wrong function name, should be sayscrolldown.
            interpretError("Invalid arg 4 given to sayscrollup");
        }
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING && (opcode[1] & VALUE_TYPE_MASK) == VALUE_TYPE_INT && data[1] != 0) {
        interpretError("Invalid arg 3 given to sayscrolldown");
    }

    if ((opcode[2] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING && (opcode[2] & VALUE_TYPE_MASK) == VALUE_TYPE_INT && data[2] != 0) {
        interpretError("Invalid arg 2 given to sayscrolldown");
    }

    if ((opcode[3] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING && (opcode[3] & VALUE_TYPE_MASK) == VALUE_TYPE_INT && data[3] != 0) {
        interpretError("Invalid arg 1 given to sayscrolldown");
    }

    if ((opcode[3] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        v1 = interpretGetString(program, opcode[3], data[3]);
        v1 = interpretMangleName(v1);
        v1 = mystrdup(v1, __FILE__, __LINE__); // "..\\int\\INTLIB.C", 1652
    }

    if ((opcode[2] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        v2 = interpretGetString(program, opcode[2], data[2]);
        v2 = interpretMangleName(v2);
        v2 = mystrdup(v2, __FILE__, __LINE__); // "..\\int\\INTLIB.C", 1654
    }

    if ((opcode[1] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        v3 = interpretGetString(program, opcode[1], data[1]);
        v3 = interpretMangleName(v3);
        v3 = mystrdup(v3, __FILE__, __LINE__); // "..\\int\\INTLIB.C", 1656
    }

    if ((opcode[0] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        v4 = interpretGetString(program, opcode[0], data[0]);
        v4 = interpretMangleName(v4);
        v4 = mystrdup(v4, __FILE__, __LINE__); // "..\\int\\INTLIB.C", 1658
    }

    if (dialogSetScrollDown(data[5], data[4], v1, v2, v3, v4, v5) != 0) {
        interpretError("Error setting scroll down");
    }
}

// 0x465FE0
static void op_saysetspacing(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid arg 1 given to saysetspacing");
    }

    if (dialogSetSpacing(data) != 0) {
        interpretError("Error setting option spacing");
    }
}

// 0x46604C
static void op_sayrestart(Program* program)
{
    if (dialogRestart() != 0) {
        interpretError("Error restarting option");
    }
}

// 0x466064
static void soundCallbackInterpret(void* userData, int a2)
{
    if (a2 == 1) {
        Sound** sound = (Sound**)userData;
        *sound = NULL;
    }
}

// 0x466070
static int soundDeleteInterpret(int value)
{
    if (value == -1) {
        return 1;
    }

    if ((value & 0xA0000000) == 0) {
        return 0;
    }

    int index = value & ~0xA0000000;
    Sound* sound = interpretSounds[index];
    if (sound == NULL) {
        return 0;
    }

    if (soundPlaying(sound)) {
        soundStop(sound);
    }

    soundDelete(sound);

    interpretSounds[index] = NULL;

    return 1;
}

// NOTE: Inlined.
//
// 0x4660E8
void soundCloseInterpret()
{
    int index;

    for (index = 0; index < INT_LIB_SOUNDS_CAPACITY; index++) {
        if (interpretSounds[index] != NULL) {
            soundDeleteInterpret(index | 0xA0000000);
        }
    }
}

// 0x466110
int soundStartInterpret(char* fileName, int mode)
{
    int v3 = 1;
    int v5 = 0;

    if (mode & 0x01) {
        // looping
        v5 |= 0x20;
    } else {
        v3 = 5;
    }

    if (mode & 0x02) {
        v5 |= 0x08;
    } else {
        v5 |= 0x10;
    }

    if (mode & 0x0100) {
        // memory
        v3 &= ~0x03;
        v3 |= 0x01;
    }

    if (mode & 0x0200) {
        // streamed
        v3 &= ~0x03;
        v3 |= 0x02;
    }

    int index;
    for (index = 0; index < INT_LIB_SOUNDS_CAPACITY; index++) {
        if (interpretSounds[index] == NULL) {
            break;
        }
    }

    if (index == INT_LIB_SOUNDS_CAPACITY) {
        return -1;
    }

    Sound* sound = interpretSounds[index] = soundAllocate(v3, v5);
    if (sound == NULL) {
        return -1;
    }

    soundSetCallback(sound, soundCallbackInterpret, &(interpretSounds[index]));

    if (mode & 0x01) {
        soundLoop(sound, 0xFFFF);
    }

    if (mode & 0x1000) {
        // mono
        soundSetChannel(sound, 2);
    }

    if (mode & 0x2000) {
        // stereo
        soundSetChannel(sound, 3);
    }

    int rc = soundLoad(sound, fileName);
    if (rc != SOUND_NO_ERROR) {
        goto err;
    }

    rc = soundPlay(sound);

    // TODO: Maybe wrong.
    switch (rc) {
    case SOUND_NO_DEVICE:
        debug_printf("soundPlay error: %s\n", "SOUND_NO_DEVICE");
        goto err;
    case SOUND_NOT_INITIALIZED:
        debug_printf("soundPlay error: %s\n", "SOUND_NOT_INITIALIZED");
        goto err;
    case SOUND_NO_SOUND:
        debug_printf("soundPlay error: %s\n", "SOUND_NO_SOUND");
        goto err;
    case SOUND_FUNCTION_NOT_SUPPORTED:
        debug_printf("soundPlay error: %s\n", "SOUND_FUNC_NOT_SUPPORTED");
        goto err;
    case SOUND_NO_BUFFERS_AVAILABLE:
        debug_printf("soundPlay error: %s\n", "SOUND_NO_BUFFERS_AVAILABLE");
        goto err;
    case SOUND_FILE_NOT_FOUND:
        debug_printf("soundPlay error: %s\n", "SOUND_FILE_NOT_FOUND");
        goto err;
    case SOUND_ALREADY_PLAYING:
        debug_printf("soundPlay error: %s\n", "SOUND_ALREADY_PLAYING");
        goto err;
    case SOUND_NOT_PLAYING:
        debug_printf("soundPlay error: %s\n", "SOUND_NOT_PLAYING");
        goto err;
    case SOUND_ALREADY_PAUSED:
        debug_printf("soundPlay error: %s\n", "SOUND_ALREADY_PAUSED");
        goto err;
    case SOUND_NOT_PAUSED:
        debug_printf("soundPlay error: %s\n", "SOUND_NOT_PAUSED");
        goto err;
    case SOUND_INVALID_HANDLE:
        debug_printf("soundPlay error: %s\n", "SOUND_INVALID_HANDLE");
        goto err;
    case SOUND_NO_MEMORY_AVAILABLE:
        debug_printf("soundPlay error: %s\n", "SOUND_NO_MEMORY");
        goto err;
    case SOUND_UNKNOWN_ERROR:
        debug_printf("soundPlay error: %s\n", "SOUND_ERROR");
        goto err;
    }

    return index | 0xA0000000;

err:

    soundDelete(sound);
    interpretSounds[index] = NULL;
    return -1;
}

// 0x46655C
static int soundPauseInterpret(int value)
{
    if (value == -1) {
        return 1;
    }

    if ((value & 0xA0000000) == 0) {
        return 0;
    }

    int index = value & ~0xA0000000;
    Sound* sound = interpretSounds[index];
    if (sound == NULL) {
        return 0;
    }

    int rc;
    if (soundType(sound, 0x01)) {
        rc = soundStop(sound);
    } else {
        rc = soundPause(sound);
    }
    return rc == SOUND_NO_ERROR;
}

// 0x4665C8
static int soundRewindInterpret(int value)
{
    if (value == -1) {
        return 1;
    }

    if ((value & 0xA0000000) == 0) {
        return 0;
    }

    int index = value & ~0xA0000000;
    Sound* sound = interpretSounds[index];
    if (sound == NULL) {
        return 0;
    }

    if (!soundPlaying(sound)) {
        return 1;
    }

    soundStop(sound);

    return soundPlay(sound) == SOUND_NO_ERROR;
}

// 0x46662C
static int soundUnpauseInterpret(int value)
{
    if (value == -1) {
        return 1;
    }

    if ((value & 0xA0000000) == 0) {
        return 0;
    }

    int index = value & ~0xA0000000;
    Sound* sound = interpretSounds[index];
    if (sound == NULL) {
        return 0;
    }

    int rc;
    if (soundType(sound, 0x01)) {
        rc = soundPlay(sound);
    } else {
        rc = soundUnpause(sound);
    }
    return rc == SOUND_NO_ERROR;
}

// 0x466698
static void op_soundplay(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = interpretPopShort(program);
        data[arg] = interpretPopLong(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid arg 2 given to soundplay");
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        interpretError("Invalid arg 1 given to soundplay");
    }

    char* fileName = interpretGetString(program, opcode[1], data[1]);
    char* mangledFileName = interpretMangleName(fileName);
    int rc = soundStartInterpret(mangledFileName, data[0]);

    interpretPushLong(program, rc);
    interpretPushShort(program, VALUE_TYPE_INT);
}

// 0x466768
static void op_soundpause(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (data == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid arg 1 given to soundpause");
    }

    soundPauseInterpret(data);
}

// 0x4667C0
static void op_soundresume(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (data == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid arg 1 given to soundresume");
    }

    soundUnpauseInterpret(data);
}

// 0x466818
static void op_soundstop(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (data == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid arg 1 given to soundstop");
    }

    soundPauseInterpret(data);
}

// 0x466870
static void op_soundrewind(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (data == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid arg 1 given to soundrewind");
    }

    soundRewindInterpret(data);
}

// 0x4668C8
static void op_sounddelete(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (data == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("Invalid arg 1 given to sounddelete");
    }

    soundDeleteInterpret(data);
}

// 0x466920
static void op_setoneoptpause(Program* program)
{
    opcode_t opcode = interpretPopShort(program);
    int data = interpretPopLong(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        interpretError("SetOneOptPause: invalid arg passed (non-integer).");
    }

    if (data) {
        if ((dialogGetMediaFlag() & 8) == 0) {
            return;
        }
    } else {
        if ((dialogGetMediaFlag() & 8) != 0) {
            return;
        }
    }

    dialogToggleMediaFlag(8);
}

// 0x466994
void updateIntLib()
{
    nevs_update();
    updateIntExtra();
}

// 0x4669A0
void intlibClose()
{
    dialogClose();
    intExtraClose();

    // NOTE: Uninline.
    soundCloseInterpret();

    nevs_close();

    if (callbacks != NULL) {
        myfree(callbacks, __FILE__, __LINE__); // "..\\int\\INTLIB.C", 1976
        callbacks = NULL;
        numCallbacks = 0;
    }
}

// 0x466A04
static bool intLibDoInput(int key)
{
    if (key < 0 || key >= INT_LIB_KEY_HANDLERS_CAPACITY) {
        return false;
    }

    if (anyKeyProg != NULL) {
        if (anyKeyOffset != 0) {
            executeProc(anyKeyProg, anyKeyOffset);
        }
        return true;
    }

    IntLibKeyHandlerEntry* entry = &(inputProc[key]);
    if (entry->program == NULL) {
        return false;
    }

    if (entry->proc != 0) {
        executeProc(entry->program, entry->proc);
    }

    return true;
}

// 0x466A70
void initIntlib()
{
    windowAddInputFunc(intLibDoInput);

    interpretAddFunc(0x806A, op_fillwin3x3);
    interpretAddFunc(0x808C, op_deletebutton);
    interpretAddFunc(0x8086, op_addbutton);
    interpretAddFunc(0x8088, op_addbuttonflag);
    interpretAddFunc(0x8087, op_addbuttontext);
    interpretAddFunc(0x8089, op_addbuttongfx);
    interpretAddFunc(0x808A, op_addbuttonproc);
    interpretAddFunc(0x808B, op_addbuttonrightproc);
    interpretAddFunc(0x8067, op_showwin);
    interpretAddFunc(0x8068, op_fillwin);
    interpretAddFunc(0x8069, op_fillrect);
    interpretAddFunc(0x8072, op_print);
    interpretAddFunc(0x8073, op_format);
    interpretAddFunc(0x8074, op_printrect);
    interpretAddFunc(0x8075, op_setfont);
    interpretAddFunc(0x8076, op_settextflags);
    interpretAddFunc(0x8077, op_settextcolor);
    interpretAddFunc(0x8078, op_sethighlightcolor);
    interpretAddFunc(0x8064, op_selectwin);
    interpretAddFunc(0x806B, op_display);
    interpretAddFunc(0x806D, op_displayraw);
    interpretAddFunc(0x806C, op_displaygfx);
    interpretAddFunc(0x806F, op_fadein);
    interpretAddFunc(0x8070, op_fadeout);
    interpretAddFunc(0x807A, op_playmovie);
    interpretAddFunc(0x807B, op_movieflags);
    interpretAddFunc(0x807C, op_playmovierect);
    interpretAddFunc(0x8079, op_stopmovie);
    interpretAddFunc(0x807F, op_addregion);
    interpretAddFunc(0x8080, op_addregionflag);
    interpretAddFunc(0x8081, op_addregionproc);
    interpretAddFunc(0x8082, op_addregionrightproc);
    interpretAddFunc(0x8083, op_deleteregion);
    interpretAddFunc(0x8084, op_activateregion);
    interpretAddFunc(0x8085, op_checkregion);
    interpretAddFunc(0x8062, op_createwin);
    interpretAddFunc(0x8063, op_deletewin);
    interpretAddFunc(0x8065, op_resizewin);
    interpretAddFunc(0x8066, op_scalewin);
    interpretAddFunc(0x804E, op_saystart);
    interpretAddFunc(0x804F, op_saystartpos);
    interpretAddFunc(0x8050, op_sayreplytitle);
    interpretAddFunc(0x8051, op_saygotoreply);
    interpretAddFunc(0x8053, op_sayreply);
    interpretAddFunc(0x8052, op_sayoption);
    interpretAddFunc(0x804D, op_sayend);
    interpretAddFunc(0x804C, op_sayquit);
    interpretAddFunc(0x8054, op_saymessage);
    interpretAddFunc(0x8055, op_sayreplywindow);
    interpretAddFunc(0x8056, op_sayoptionwindow);
    interpretAddFunc(0x805F, op_sayreplyflags);
    interpretAddFunc(0x8060, op_sayoptionflags);
    interpretAddFunc(0x8057, op_sayborder);
    interpretAddFunc(0x8058, op_sayscrollup);
    interpretAddFunc(0x8059, op_sayscrolldown);
    interpretAddFunc(0x805A, op_saysetspacing);
    interpretAddFunc(0x805B, op_sayoptioncolor);
    interpretAddFunc(0x805C, op_sayreplycolor);
    interpretAddFunc(0x805D, op_sayrestart);
    interpretAddFunc(0x805E, op_saygetlastpos);
    interpretAddFunc(0x8061, op_saymessagetimeout);
    interpretAddFunc(0x8071, op_gotoxy);
    interpretAddFunc(0x808D, op_hidemouse);
    interpretAddFunc(0x808E, op_showmouse);
    interpretAddFunc(0x8090, op_refreshmouse);
    interpretAddFunc(0x808F, op_mouseshape);
    interpretAddFunc(0x8091, op_setglobalmousefunc);
    interpretAddFunc(0x806E, op_loadpalettetable);
    interpretAddFunc(0x8092, op_addNamedEvent);
    interpretAddFunc(0x8093, op_addNamedHandler);
    interpretAddFunc(0x8094, op_clearNamed);
    interpretAddFunc(0x8095, op_signalNamed);
    interpretAddFunc(0x8096, op_addkey);
    interpretAddFunc(0x8097, op_deletekey);
    interpretAddFunc(0x8098, op_soundplay);
    interpretAddFunc(0x8099, op_soundpause);
    interpretAddFunc(0x809A, op_soundresume);
    interpretAddFunc(0x809B, op_soundstop);
    interpretAddFunc(0x809C, op_soundrewind);
    interpretAddFunc(0x809D, op_sounddelete);
    interpretAddFunc(0x809E, op_setoneoptpause);
    interpretAddFunc(0x809F, op_selectfilelist);
    interpretAddFunc(0x80A0, op_tokenize);

    nevs_initonce();
    initIntExtra();
    initDialog();
}

// 0x466F6C
void interpretRegisterProgramDeleteCallback(IntLibProgramDeleteCallback* callback)
{
    int index;
    for (index = 0; index < numCallbacks; index++) {
        if (callbacks[index] == NULL) {
            break;
        }
    }

    if (index == numCallbacks) {
        if (callbacks != NULL) {
            callbacks = (IntLibProgramDeleteCallback**)myrealloc(callbacks, sizeof(*callbacks) * (numCallbacks + 1), __FILE__, __LINE__); // ..\\int\\INTLIB.C, 2110
        } else {
            callbacks = (IntLibProgramDeleteCallback**)mymalloc(sizeof(*callbacks), __FILE__, __LINE__); // ..\\int\\INTLIB.C, 2112
        }
        numCallbacks++;
    }

    callbacks[index] = callback;
}

// 0x467040
void removeProgramReferences(Program* program)
{
    for (int index = 0; index < INT_LIB_KEY_HANDLERS_CAPACITY; index++) {
        if (program == inputProc[index].program) {
            inputProc[index].program = NULL;
        }
    }

    intExtraRemoveProgramReferences(program);

    for (int index = 0; index < numCallbacks; index++) {
        IntLibProgramDeleteCallback* callback = callbacks[index];
        if (callback != NULL) {
            callback(program);
        }
    }
}
