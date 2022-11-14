#include "int/intlib.h"

#include "color.h"
#include "core.h"
#include "int/datafile.h"
#include "debug.h"
#include "int/dialog.h"
#include "int/support/intextra.h"
#include "memory_manager.h"
#include "mouse_manager.h"
#include "nevs.h"
#include "select_file_list.h"
#include "sound.h"
#include "text_font.h"
#include "window_manager_private.h"

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
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        programFatalError("Invalid type given to fillwin3x3");
    }

    char* fileName = programGetString(program, opcode, data);
    char* mangledFileName = _interpretMangleName(fileName);

    int imageWidth;
    int imageHeight;
    unsigned char* imageData = loadDataFile(mangledFileName, &imageWidth, &imageHeight);
    if (imageData == NULL) {
        programFatalError("cannot load 3x3 file '%s'", mangledFileName);
    }

    _selectWindowID(program->windowId);

    int windowHeight = _windowHeight();
    int windowWidth = _windowWidth();
    unsigned char* windowBuffer = _windowGetBuffer();
    _fillBuf3x3(imageData,
        imageWidth,
        imageHeight,
        windowBuffer,
        windowWidth,
        windowHeight);

    internal_free_safe(imageData, __FILE__, __LINE__); // "..\\int\\INTLIB.C", 94
}

// 0x461850
static void op_format(Program* program)
{
    opcode_t opcode[6];
    int data[6];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 6; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 6 given to format\n");
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 5 given to format\n");
    }

    if ((opcode[2] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 4 given to format\n");
    }

    if ((opcode[3] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 3 given to format\n");
    }

    if ((opcode[4] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 2 given to format\n");
    }

    if ((opcode[5] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        programFatalError("Invalid arg 1 given to format\n");
    }

    char* string = programGetString(program, opcode[5], data[5]);
    int x = data[4];
    int y = data[3];
    int width = data[2];
    int height = data[1];
    int textAlignment = data[0];

    if (!_windowFormatMessage(string, x, y, width, height, textAlignment)) {
        programFatalError("Error formatting message\n");
    }
}

// 0x461A5C
static void op_print(Program* program)
{
    _selectWindowID(program->windowId);

    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode, data);
    }

    switch (opcode & VALUE_TYPE_MASK) {
    case VALUE_TYPE_STRING:
        _interpretOutput("%s", programGetString(program, opcode, data));
        break;
    case VALUE_TYPE_FLOAT:
        _interpretOutput("%.5f", *((float*)&data));
        break;
    case VALUE_TYPE_INT:
        _interpretOutput("%d", data);
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
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        programFatalError("Error, invalid arg 2 given to selectfilelist");
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        programFatalError("Error, invalid arg 1 given to selectfilelist");
    }

    char* pattern = programGetString(program, opcode[0], data[0]);
    char* title = programGetString(program, opcode[1], data[1]);

    int fileListLength;
    char** fileList = _getFileList(_interpretMangleName(pattern), &fileListLength);
    if (fileList != NULL && fileListLength != 0) {
        int selectedIndex = _win_list_select(title,
            fileList,
            fileListLength,
            NULL,
            320 - fontGetStringWidth(title) / 2,
            200,
            colorTable[0x7FFF] | 0x10000);

        if (selectedIndex != -1) {
            programStackPushInt32(program, programPushString(program, fileList[selectedIndex]));
            programStackPushInt16(program, VALUE_TYPE_DYNAMIC_STRING);
        } else {
            programStackPushInt32(program, 0);
            programStackPushInt16(program, VALUE_TYPE_INT);
        }

        _freeFileList(fileList);
    } else {
        programStackPushInt32(program, 0);
        programStackPushInt16(program, VALUE_TYPE_INT);
    }

    program->flags &= ~PROGRAM_FLAG_0x20;
}

// 0x461CA0
static void op_tokenize(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    opcode[0] = programStackPopInt16(program);
    data[0] = programStackPopInt32(program);

    if (opcode[0] == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode[0], data[0]);
    }

    if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Error, invalid arg 3 to tokenize.");
    }

    opcode[1] = programStackPopInt16(program);
    data[1] = programStackPopInt32(program);

    if (opcode[1] == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode[1], data[1]);
    }

    char* prev = NULL;
    if ((opcode[1] & VALUE_TYPE_MASK) == VALUE_TYPE_INT) {
        if (data[1] != 0) {
            programFatalError("Error, invalid arg 2 to tokenize. (only accept 0 for int value)");
        }
    } else if ((opcode[1] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        prev = programGetString(program, opcode[1], data[1]);
    } else {
        programFatalError("Error, invalid arg 2 to tokenize. (string)");
    }

    opcode[2] = programStackPopInt16(program);
    data[2] = programStackPopInt32(program);

    if (opcode[2] == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode[2], data[2]);
    }

    if ((opcode[2] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        programFatalError("Error, invalid arg 1 to tokenize.");
    }

    char* string = programGetString(program, opcode[2], data[2]);
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

            temp = (char*)internal_calloc_safe(1, length + 1, __FILE__, __LINE__); // "..\\int\\INTLIB.C, 230
            strncpy(temp, start, length);
            programStackPushInt32(program, programPushString(program, temp));
            programStackPushInt16(program, VALUE_TYPE_DYNAMIC_STRING);
        } else {
            programStackPushInt32(program, 0);
            programStackPushInt16(program, VALUE_TYPE_INT);
        }
    } else {
        int length = 0;
        char* end = string;
        while (*end != data[0] && *end != '\0') {
            end++;
            length++;
        }

        if (string != NULL) {
            temp = (char*)internal_calloc_safe(1, length + 1, __FILE__, __LINE__); // "..\\int\\INTLIB.C", 248
            strncpy(temp, string, length);
            programStackPushInt32(program, programPushString(program, temp));
            programStackPushInt16(program, VALUE_TYPE_DYNAMIC_STRING);
        } else {
            programStackPushInt32(program, 0);
            programStackPushInt16(program, VALUE_TYPE_INT);
        }
    }

    if (temp != NULL) {
        internal_free_safe(temp, __FILE__, __LINE__); // "..\\int\\INTLIB.C" , 260
    }
}

// 0x461F1C
static void op_printrect(Program* program)
{
    _selectWindowID(program->windowId);

    opcode_t opcode[3];
    int data[3];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_INT || data[0] > 2) {
        programFatalError("Invalid arg 3 given to printrect, expecting int");
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 2 given to printrect, expecting int");
    }

    char string[80];
    switch (opcode[2] & VALUE_TYPE_MASK) {
    case VALUE_TYPE_STRING:
        sprintf(string, "%s", programGetString(program, opcode[2], data[2]));
        break;
    case VALUE_TYPE_FLOAT:
        sprintf(string, "%.5f", *((float*)&data[2]));
        break;
    case VALUE_TYPE_INT:
        sprintf(string, "%d", data[2]);
        break;
    }

    if (!_windowPrintRect(string, data[1], data[0])) {
        programFatalError("Error in printrect");
    }
}

// 0x46209C
static void op_selectwin(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        programFatalError("Invalid type given to select");
    }

    const char* windowName = programGetString(program, opcode, data);
    int win = _pushWindow(windowName);
    if (win == -1) {
        programFatalError("Error selecing window %s\n", programGetString(program, opcode, data));
    }

    program->windowId = win;

    _interpretOutputFunc(_windowOutput);
}

// 0x46213C
static void op_display(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        programFatalError("Invalid type given to display");
    }

    char* fileName = programGetString(program, opcode, data);

    _selectWindowID(program->windowId);

    char* mangledFileName = _interpretMangleName(fileName);
    _displayFile(mangledFileName);
}

// 0x4621B4
static void op_displayraw(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        programFatalError("Invalid type given to displayraw");
    }

    char* fileName = programGetString(program, opcode, data);

    _selectWindowID(program->windowId);

    char* mangledFileName = _interpretMangleName(fileName);
    _displayFileRaw(mangledFileName);
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

    time = _get_time();
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
                _process_bk();
            }

            time = _get_time();
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
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid type given to fadein\n");
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
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        // FIXME: Wrong function name, should be fadeout.
        programFatalError("Invalid type given to fadein\n");
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

    return _windowMoviePlaying();
}

// 0x462584
static void op_movieflags(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode, data);
    }

    if (!_windowSetMovieFlags(data)) {
        programFatalError("Error setting movie flags\n");
    }
}

// 0x4625D0
static void op_playmovie(Program* program)
{
    // 0x59E168
    static char name[100];

    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        programFatalError("Invalid type given to playmovie");
    }

    strcpy(name, programGetString(program, opcode, data));

    if (strrchr(name, '.') == NULL) {
        strcat(name, ".mve");
    }

    _selectWindowID(program->windowId);

    program->flags |= PROGRAM_IS_WAITING;
    program->checkWaitFunc = checkMovie;

    char* mangledFileName = _interpretMangleName(name);
    if (!_windowPlayMovie(mangledFileName)) {
        programFatalError("Error playing movie");
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
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[4] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        // FIXME: Wrong function name, should playmovierect.
        programFatalError("Invalid arg given to playmovie");
    }

    strcpy(name, programGetString(program, opcode[4], data[4]));

    if (strrchr(name, '.') == NULL) {
        strcat(name, ".mve");
    }

    _selectWindowID(program->windowId);

    program->checkWaitFunc = checkMovie;
    program->flags |= PROGRAM_IS_WAITING;

    char* mangledFileName = _interpretMangleName(name);
    if (!_windowPlayMovieRect(mangledFileName, data[3], data[2], data[1], data[0])) {
        programFatalError("Error playing movie");
    }
}

// 0x46287C
static void op_stopmovie(Program* program)
{
    _windowStopMovie();
    program->flags |= PROGRAM_FLAG_0x40;
}

// 0x462890
static void op_deleteregion(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        if ((opcode & VALUE_TYPE_MASK) == VALUE_TYPE_INT && data != -1) {
            programFatalError("Invalid type given to deleteregion");
        }
    }

    _selectWindowID(program->windowId);

    const char* regionName = data != -1 ? programGetString(program, opcode, data) : NULL;
    _windowDeleteRegion(regionName);
}

// 0x462924
static void op_activateregion(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    const char* regionName = programGetString(program, opcode[1], data[1]);
    _windowActivateRegion(regionName, data[0]);
}

// 0x4629A0
static void op_checkregion(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        programFatalError("Invalid arg 1 given to checkregion();\n");
    }

    const char* regionName = programGetString(program, opcode, data);

    bool regionExists = _windowCheckRegionExists(regionName);
    programStackPushInt32(program, regionExists);
    programStackPushInt16(program, VALUE_TYPE_INT);
}

// 0x462A1C
static void op_addregion(Program* program)
{
    opcode_t opcode;
    int data;

    opcode = programStackPopInt16(program);
    data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid number of elements given to region");
    }

    int args = data;

    if (args < 2) {
        programFatalError("addregion call without enough points!");
    }

    _selectWindowID(program->windowId);

    _windowStartRegion(args / 2);

    while (args >= 2) {
        opcode = programStackPopInt16(program);
        data = programStackPopInt32(program);

        if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode, data);
        }

        if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            programFatalError("Invalid y value given to region");
        }

        int y = data;

        opcode = programStackPopInt16(program);
        data = programStackPopInt32(program);

        if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode, data);
        }

        if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            programFatalError("Invalid x value given to region");
        }

        int x = data;

        y = (y * _windowGetYres() + 479) / 480;
        x = (x * _windowGetXres() + 639) / 640;
        args -= 2;

        _windowAddRegionPoint(x, y, true);
    }

    if (args == 0) {
        programFatalError("Unnamed regions not allowed\n");
        _windowEndRegion();
    } else {
        opcode = programStackPopInt16(program);
        data = programStackPopInt32(program);

        if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode, data);
        }

        if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_STRING && opcode == VALUE_TYPE_INT) {
            if (data != 0) {
                programFatalError("Invalid name given to region");
            }
        }

        const char* regionName = programGetString(program, opcode, data);
        _windowAddRegionName(regionName);
        _windowEndRegion();
    }
}

// 0x462C10
static void op_addregionproc(Program* program)
{
    opcode_t opcode[5];
    int data[5];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 5; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid procedure 4 name given to addregionproc");
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid procedure 3 name given to addregionproc");
    }

    if ((opcode[2] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid procedure 2 name given to addregionproc");
    }

    if ((opcode[3] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid procedure 1 name given to addregionproc");
    }

    if ((opcode[4] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        programFatalError("Invalid name given to addregionproc");
    }

    const char* regionName = programGetString(program, opcode[4], data[4]);
    _selectWindowID(program->windowId);

    if (!_windowAddRegionProc(regionName, program, data[3], data[2], data[1], data[0])) {
        programFatalError("Error setting procedures to region %s\n", regionName);
    }
}

// 0x462DDC
static void op_addregionrightproc(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid procedure 2 name given to addregionrightproc");
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid procedure 1 name given to addregionrightproc");
    }

    if ((opcode[2] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        programFatalError("Invalid name given to addregionrightproc");
    }

    const char* regionName = programGetString(program, opcode[2], data[2]);
    _selectWindowID(program->windowId);

    if (!_windowAddRegionRightProc(regionName, program, data[1], data[0])) {
        programFatalError("ErrorError setting right button procedures to region %s\n", regionName);
    }
}

// 0x462F08
static void op_createwin(Program* program)
{
    opcode_t opcode[5];
    int data[5];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 5; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    const char* windowName = programGetString(program, opcode[4], data[4]);
    int x = (data[3] * _windowGetXres() + 639) / 640;
    int y = (data[2] * _windowGetYres() + 479) / 480;
    int width = (data[1] * _windowGetXres() + 639) / 640;
    int height = (data[0] * _windowGetYres() + 479) / 480;

    if (_createWindow(windowName, x, y, width, height, colorTable[0], 0) == -1) {
        programFatalError("Couldn't create window.");
    }
}

// 0x46308C
static void op_resizewin(Program* program)
{
    opcode_t opcode[5];
    int data[5];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 5; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    const char* windowName = programGetString(program, opcode[4], data[4]);
    int x = (data[3] * _windowGetXres() + 639) / 640;
    int y = (data[2] * _windowGetYres() + 479) / 480;
    int width = (data[1] * _windowGetXres() + 639) / 640;
    int height = (data[0] * _windowGetYres() + 479) / 480;

    if (sub_4B7AC4(windowName, x, y, width, height) == -1) {
        programFatalError("Couldn't resize window.");
    }
}

// 0x463204
static void op_scalewin(Program* program)
{
    opcode_t opcode[5];
    int data[5];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 5; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    const char* windowName = programGetString(program, opcode[4], data[4]);
    int x = (data[3] * _windowGetXres() + 639) / 640;
    int y = (data[2] * _windowGetYres() + 479) / 480;
    int width = (data[1] * _windowGetXres() + 639) / 640;
    int height = (data[0] * _windowGetYres() + 479) / 480;

    if (sub_4B7E7C(windowName, x, y, width, height) == -1) {
        programFatalError("Couldn't scale window.");
    }
}

// 0x46337C
static void op_deletewin(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode, data);
    }

    const char* windowName = programGetString(program, opcode, data);

    if (!_deleteWindow(windowName)) {
        programFatalError("Error deleting window %s\n", windowName);
    }

    program->windowId = _popWindow();
}

// 0x4633E4
static void op_saystart(Program* program)
{
    sayStartingPosition = 0;

    program->flags |= PROGRAM_FLAG_0x20;
    int rc = dialogStart(program);
    program->flags &= ~PROGRAM_FLAG_0x20;

    if (rc != 0) {
        programFatalError("Error starting dialog.");
    }
}

// 0x463430
static void op_saystartpos(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode, data);
    }

    sayStartingPosition = data;

    program->flags |= PROGRAM_FLAG_0x20;
    int rc = dialogStart(program);
    program->flags &= ~PROGRAM_FLAG_0x20;

    if (rc != 0) {
        programFatalError("Error starting dialog.");
    }
}

// 0x46349C
static void op_sayreplytitle(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode, data);
    }

    char* string = NULL;
    if ((opcode & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        string = programGetString(program, opcode, data);
    }

    if (dialogTitle(string) != 0) {
        programFatalError("Error setting title.");
    }
}

// 0x463510
static void op_saygotoreply(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode, data);
    }

    char* string = NULL;
    if ((opcode & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        string = programGetString(program, opcode, data);
    }

    if (dialogGotoReply(string) != 0) {
        programFatalError("Error during goto, couldn't find reply target %s", string);
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
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    const char* v1;
    if ((opcode[1] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        v1 = programGetString(program, opcode[1], data[1]);
    } else {
        v1 = NULL;
    }

    if ((opcode[0] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        const char* v2 = programGetString(program, opcode[0], data[0]);
        if (dialogOption(v1, v2) != 0) {
            program->flags &= ~PROGRAM_FLAG_0x20;
            programFatalError("Error setting option.");
        }
    } else if ((opcode[0] & VALUE_TYPE_MASK) == VALUE_TYPE_INT) {
        if (dialogOptionProc(v1, data[0]) != 0) {
            program->flags &= ~PROGRAM_FLAG_0x20;
            programFatalError("Error setting option.");
        }
    } else {
        programFatalError("Invalid arg 2 to sayOption");
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
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    const char* v1;
    if ((opcode[1] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        v1 = programGetString(program, opcode[1], data[1]);
    } else {
        v1 = NULL;
    }

    const char* v2;
    if ((opcode[0] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        v2 = programGetString(program, opcode[0], data[0]);
    } else {
        v2 = NULL;
    }

    if (dialogReply(v1, v2) != 0) {
        program->flags &= ~PROGRAM_FLAG_0x20;
        programFatalError("Error setting option.");
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
    programStackPushInt32(program, value);
    programStackPushInt16(program, VALUE_TYPE_INT);
}

// 0x463810
static void op_sayquit(Program* program)
{
    if (dialogQuit() != 0) {
        programFatalError("Error quitting option.");
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
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode, data);
    }

    // TODO: What the hell is this?
    if ((opcode & VALUE_TYPE_MASK) == 0x4000) {
        programFatalError("sayMsgTimeout:  invalid var type passed.");
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
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    const char* v1;
    if ((opcode[1] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        v1 = programGetString(program, opcode[1], data[1]);
    } else {
        v1 = NULL;
    }

    const char* v2;
    if ((opcode[0] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        v2 = programGetString(program, opcode[0], data[0]);
    } else {
        v2 = NULL;
    }

    if (dialogMessage(v1, v2, TimeOut) != 0) {
        program->flags &= ~PROGRAM_FLAG_0x20;
        programFatalError("Error setting option.");
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
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_INT
        || (opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid operand given to gotoxy");
    }

    _selectWindowID(program->windowId);

    int x = data[1];
    int y = data[0];
    _windowGotoXY(x, y);
}

// 0x463A38
static void op_addbuttonflag(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 2 given to addbuttonflag");
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        programFatalError("Invalid arg 1 given to addbuttonflag");
    }

    const char* buttonName = programGetString(program, opcode[1], data[1]);
    if (!_windowSetButtonFlag(buttonName, data[0])) {
        // NOTE: Original code calls programGetString one more time with the
        // same params.
        programFatalError("Error setting flag on button %s", buttonName);
    }
}

// 0x463B10
static void op_addregionflag(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 2 given to addregionflag");
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        programFatalError("Invalid arg 1 given to addregionflag");
    }

    const char* regionName = programGetString(program, opcode[1], data[1]);
    if (!_windowSetRegionFlag(regionName, data[0])) {
        // NOTE: Original code calls programGetString one more time with the
        // same params.
        programFatalError("Error setting flag on region %s", regionName);
    }
}

// 0x463BE8
static void op_addbutton(Program* program)
{
    opcode_t opcode[5];
    int data[5];

    opcode[0] = programStackPopInt16(program);
    data[0] = programStackPopInt32(program);

    if (opcode[0] == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode[0], data[0]);
    }

    if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid height given to addbutton");
    }

    opcode[1] = programStackPopInt16(program);
    data[1] = programStackPopInt32(program);

    if (opcode[1] == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode[1], data[1]);
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid width given to addbutton");
    }

    opcode[2] = programStackPopInt16(program);
    data[2] = programStackPopInt32(program);

    if (opcode[2] == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode[2], data[2]);
    }

    if ((opcode[2] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid y given to addbutton");
    }

    opcode[3] = programStackPopInt16(program);
    data[3] = programStackPopInt32(program);

    if (opcode[3] == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode[3], data[3]);
    }

    if ((opcode[3] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid x given to addbutton");
    }

    opcode[4] = programStackPopInt16(program);
    data[4] = programStackPopInt32(program);

    if (opcode[4] == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode[4], data[4]);
    }

    if ((opcode[4] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        programFatalError("Invalid name given to addbutton");
    }

    _selectWindowID(program->windowId);

    int height = (data[0] * _windowGetYres() + 479) / 480;
    int width = (data[1] * _windowGetXres() + 639) / 640;
    int y = (data[2] * _windowGetYres() + 479) / 480;
    int x = (data[3] * _windowGetXres() + 639) / 640;
    const char* buttonName = programGetString(program, opcode[4], data[4]);

    _windowAddButton(buttonName, x, y, width, height, 0);
}

// 0x463DF4
static void op_addbuttontext(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        programFatalError("Invalid text string given to addbuttontext");
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        programFatalError("Invalid name string given to addbuttontext");
    }

    const char* text = programGetString(program, opcode[0], data[0]);
    const char* buttonName = programGetString(program, opcode[1], data[1]);

    if (!_windowAddButtonText(buttonName, text)) {
        programFatalError("Error setting text to button %s\n",
            programGetString(program, opcode[1], data[1]));
    }
}

// 0x463EEC
static void op_addbuttongfx(Program* program)
{
    opcode_t opcode[4];
    int data[4];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 4; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if (((opcode[2] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING || ((opcode[2] & VALUE_TYPE_MASK) == VALUE_TYPE_INT && data[2] == 0))
        || ((opcode[1] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING || ((opcode[1] & VALUE_TYPE_MASK) == VALUE_TYPE_INT && data[1] == 0))
        || ((opcode[0] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING || ((opcode[0] & VALUE_TYPE_MASK) == VALUE_TYPE_INT && data[0] == 0))) {

        if ((opcode[3] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
            // FIXME: Wrong function name, should be addbuttongfx.
            programFatalError("Invalid name given to addbuttontext");
        }

        const char* buttonName = programGetString(program, opcode[3], data[3]);
        char* pressedFileName = _interpretMangleName(programGetString(program, opcode[2], data[2]));
        char* normalFileName = _interpretMangleName(programGetString(program, opcode[1], data[1]));
        char* hoverFileName = _interpretMangleName(programGetString(program, opcode[0], data[0]));

        _selectWindowID(program->windowId);

        if (!_windowAddButtonGfx(buttonName, pressedFileName, normalFileName, hoverFileName)) {
            programFatalError("Error setting graphics to button %s\n", buttonName);
        }
    } else {
        programFatalError("Invalid filename given to addbuttongfx");
    }
}

// 0x4640DC
static void op_addbuttonproc(Program* program)
{
    opcode_t opcode[5];
    int data[5];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 5; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid procedure 4 name given to addbuttonproc");
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid procedure 3 name given to addbuttonproc");
    }

    if ((opcode[2] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid procedure 2 name given to addbuttonproc");
    }

    if ((opcode[3] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid procedure 1 name given to addbuttonproc");
    }

    if ((opcode[4] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        programFatalError("Invalid name given to addbuttonproc");
    }

    const char* buttonName = programGetString(program, opcode[4], data[4]);
    _selectWindowID(program->windowId);

    if (!_windowAddButtonProc(buttonName, program, data[3], data[2], data[1], data[0])) {
        programFatalError("Error setting procedures to button %s\n", buttonName);
    }
}

// 0x4642A8
static void op_addbuttonrightproc(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid procedure 2 name given to addbuttonrightproc");
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid procedure 1 name given to addbuttonrightproc");
    }

    if ((opcode[2] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        programFatalError("Invalid name given to addbuttonrightproc");
    }

    const char* regionName = programGetString(program, opcode[2], data[2]);
    _selectWindowID(program->windowId);

    if (!_windowAddRegionRightProc(regionName, program, data[1], data[0])) {
        programFatalError("Error setting right button procedures to button %s\n", regionName);
    }
}

// 0x4643D4
static void op_showwin(Program* program)
{
    _selectWindowID(program->windowId);
    windowDraw();
}

// 0x4643E4
static void op_deletebutton(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        if ((opcode & VALUE_TYPE_MASK) == VALUE_TYPE_INT && data != -1) {
            programFatalError("Invalid type given to delete button");
        }
    }

    _selectWindowID(program->windowId);

    if ((opcode & VALUE_TYPE_MASK) == VALUE_TYPE_INT) {
        if (_windowDeleteButton(NULL)) {
            return;
        }
    } else {
        const char* buttonName = programGetString(program, opcode, data);
        if (_windowDeleteButton(buttonName)) {
            return;
        }
    }

    programFatalError("Error deleting button");
}

// 0x46449C
static void op_fillwin(Program* program)
{
    opcode_t opcode[3];
    int data[3];
    float* floats = (float*)data;

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[2] & VALUE_TYPE_MASK) != VALUE_TYPE_FLOAT) {
        if ((opcode[2] & VALUE_TYPE_MASK) == VALUE_TYPE_INT) {
            if (data[2] == 1) {
                floats[2] = 1.0;
            } else if (data[2] != 0) {
                programFatalError("Invalid red value given to fillwin");
            }
        }
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_FLOAT) {
        if ((opcode[1] & VALUE_TYPE_MASK) == VALUE_TYPE_INT) {
            if (data[1] == 1) {
                floats[1] = 1.0;
            } else if (data[1] != 0) {
                programFatalError("Invalid green value given to fillwin");
            }
        }
    }

    if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_FLOAT) {
        if ((opcode[0] & VALUE_TYPE_MASK) == VALUE_TYPE_INT) {
            if (data[0] == 1) {
                floats[0] = 1.0;
            } else if (data[0] != 0) {
                programFatalError("Invalid blue value given to fillwin");
            }
        }
    }

    _selectWindowID(program->windowId);

    _windowFill(floats[2], floats[1], floats[0]);
}

// 0x4645FC
static void op_fillrect(Program* program)
{
    opcode_t opcode[7];
    int data[7];
    float* floats = (float*)data;

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 7; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[2] & VALUE_TYPE_MASK) != VALUE_TYPE_FLOAT) {
        if ((opcode[2] & VALUE_TYPE_MASK) == VALUE_TYPE_INT) {
            if (data[2] == 1) {
                floats[2] = 1.0;
            } else if (data[2] != 0) {
                programFatalError("Invalid red value given to fillrect");
            }
        }
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_FLOAT) {
        if ((opcode[1] & VALUE_TYPE_MASK) == VALUE_TYPE_INT) {
            if (data[1] == 1) {
                floats[1] = 1.0;
            } else if (data[1] != 0) {
                programFatalError("Invalid green value given to fillrect");
            }
        }
    }

    if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_FLOAT) {
        if ((opcode[0] & VALUE_TYPE_MASK) == VALUE_TYPE_INT) {
            if (data[0] == 1) {
                floats[0] = 1.0;
            } else if (data[0] != 0) {
                programFatalError("Invalid blue value given to fillrect");
            }
        }
    }

    if ((opcode[6] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 1 given to fillrect");
    }

    if ((opcode[5] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 2 given to fillrect");
    }

    if ((opcode[4] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 3 given to fillrect");
    }
    if ((opcode[3] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 4 given to fillrect");
    }

    _selectWindowID(program->windowId);

    _windowFillRect(data[6], data[5], data[4], data[3], floats[2], floats[1], floats[0]);
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
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 3 given to mouseshape");
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 2 given to mouseshape");
    }

    if ((opcode[2] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        programFatalError("Invalid filename given to mouseshape");
    }

    char* fileName = programGetString(program, opcode[2], data[2]);
    if (!mouseManagerSetMouseShape(fileName, data[1], data[0])) {
        programFatalError("Error loading mouse shape.");
    }
}

// 0x4649C4
static void op_setglobalmousefunc(Program* Program)
{
    programFatalError("setglobalmousefunc not defined");
}

// 0x4649D4
static void op_displaygfx(Program* program)
{
    opcode_t opcode[5];
    int data[5];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 5; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    char* fileName = programGetString(program, opcode[4], data[4]);
    char* mangledFileName = _interpretMangleName(fileName);
    _windowDisplay(mangledFileName, data[3], data[2], data[1], data[0]);
}

// 0x464ADC
static void op_loadpalettetable(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        if ((opcode & VALUE_TYPE_MASK) == VALUE_TYPE_INT && data != -1) {
            programFatalError("Invalid type given to loadpalettetable");
        }
    }

    char* path = programGetString(program, opcode, data);
    if (!loadColorTable(path)) {
        programFatalError(colorError());
    }
}

// 0x464B54
static void op_addNamedEvent(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        programFatalError("Invalid type given to addnamedevent");
    }

    const char* v1 = programGetString(program, opcode[1], data[1]);
    _nevs_addevent(v1, program, data[0], NEVS_TYPE_EVENT);
}

// 0x464BE8
static void op_addNamedHandler(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        programFatalError("Invalid type given to addnamedhandler");
    }

    const char* v1 = programGetString(program, opcode[1], data[1]);
    _nevs_addevent(v1, program, data[0], NEVS_TYPE_HANDLER);
}

// 0x464C80
static void op_clearNamed(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        programFatalError("Invalid type given to clearnamed");
    }

    char* string = programGetString(program, opcode, data);
    _nevs_clearevent(string);
}

// 0x464CE4
static void op_signalNamed(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        programFatalError("Invalid type given to signalnamed");
    }

    char* str = programGetString(program, opcode, data);
    _nevs_signal(str);
}

// 0x464D48
static void op_addkey(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    for (int arg = 0; arg < 2; arg++) {
        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            programFatalError("Invalid arg %d given to addkey", arg + 1);
        }
    }

    int key = data[1];
    int proc = data[0];

    if (key == -1) {
        anyKeyOffset = proc;
        anyKeyProg = program;
    } else {
        if (key > INT_LIB_KEY_HANDLERS_CAPACITY - 1) {
            programFatalError("Key out of range");
        }

        inputProc[key].program = program;
        inputProc[key].proc = proc;
    }
}

// 0x464E24
static void op_deletekey(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 1 given to deletekey");
    }

    int key = data;

    if (key == -1) {
        anyKeyOffset = 0;
        anyKeyProg = NULL;
    } else {
        if (key > INT_LIB_KEY_HANDLERS_CAPACITY - 1) {
            programFatalError("Key out of range");
        }

        inputProc[key].program = NULL;
        inputProc[key].proc = 0;
    }
}

// 0x464EB0
static void op_refreshmouse(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 1 given to refreshmouse");
    }

    if (!_windowRefreshRegions()) {
        _executeProc(program, data);
    }
}

// 0x464F18
static void op_setfont(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 1 given to setfont");
    }

    if (!windowSetFont(data)) {
        programFatalError("Error setting font");
    }
}

// 0x464F84
static void op_settextflags(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 1 given to setflags");
    }

    if (!windowSetTextFlags(data)) {
        programFatalError("Error setting text flags");
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
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    for (int arg = 0; arg < 3; arg++) {
        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_FLOAT
            && (opcode[arg] & VALUE_TYPE_MASK) == VALUE_TYPE_INT
            && data[arg] != 0) {
            programFatalError("Invalid type given to settextcolor");
        }
    }

    float r = floats[2];
    float g = floats[1];
    float b = floats[0];

    if (!windowSetTextColor(r, g, b)) {
        programFatalError("Error setting text color");
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
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    for (int arg = 0; arg < 3; arg++) {
        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_FLOAT
            && (opcode[arg] & VALUE_TYPE_MASK) == VALUE_TYPE_INT
            && data[arg] != 0) {
            programFatalError("Invalid type given to sayoptioncolor");
        }
    }

    float r = floats[2];
    float g = floats[1];
    float b = floats[0];

    if (dialogSetOptionColor(r, g, b)) {
        programFatalError("Error setting option color");
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
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);
        ;

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    for (int arg = 0; arg < 3; arg++) {
        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_FLOAT
            && (opcode[arg] & VALUE_TYPE_MASK) == VALUE_TYPE_INT
            && data[arg] != 0) {
            programFatalError("Invalid type given to sayreplycolor");
        }
    }

    float r = floats[2];
    float g = floats[1];
    float b = floats[0];

    if (dialogSetReplyColor(r, g, b) != 0) {
        programFatalError("Error setting reply color");
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
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    for (int arg = 0; arg < 3; arg++) {
        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_FLOAT
            && (opcode[arg] & VALUE_TYPE_MASK) == VALUE_TYPE_INT
            && data[arg] != 0) {
            programFatalError("Invalid type given to sethighlightcolor");
        }
    }

    float r = floats[2];
    float g = floats[1];
    float b = floats[0];

    if (!windowSetHighlightColor(r, g, b)) {
        programFatalError("Error setting text highlight color");
    }
}

// 0x465530
static void op_sayreplywindow(Program* program)
{
    opcode_t opcode[5];
    int data[5];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 5; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    char* v1;
    if ((opcode[0] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        v1 = programGetString(program, opcode[0], data[0]);
        v1 = _interpretMangleName(v1);
        v1 = strdup_safe(v1, __FILE__, __LINE__); // "..\\int\\INTLIB.C", 1510
    } else if ((opcode[0] & VALUE_TYPE_MASK) == VALUE_TYPE_INT && data[0] == 0) {
        v1 = NULL;
    } else {
        programFatalError("Invalid arg 5 given to sayreplywindow");
    }

    if (dialogSetReplyWindow(data[4], data[3], data[2], data[1], v1) != 0) {
        programFatalError("Error setting reply window");
    }
}

// 0x465688
static void op_sayreplyflags(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 1 given to sayreplyflags");
    }

    if (!dialogSetReplyFlags(data)) {
        programFatalError("Error setting reply flags");
    }
}

// 0x4656F4
static void op_sayoptionflags(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 1 given to sayoptionflags");
    }

    if (!dialogSetOptionFlags(data)) {
        programFatalError("Error setting option flags");
    }
}

// 0x465760
static void op_sayoptionwindow(Program* program)
{
    opcode_t opcode[5];
    int data[5];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 5; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    char* v1;
    if ((opcode[0] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        v1 = programGetString(program, opcode[0], data[0]);
        v1 = _interpretMangleName(v1);
        v1 = strdup_safe(v1, __FILE__, __LINE__); // "..\\int\\INTLIB.C", 1556
    } else if ((opcode[0] & VALUE_TYPE_MASK) == VALUE_TYPE_INT && data[0] == 0) {
        v1 = NULL;
    } else {
        programFatalError("Invalid arg 5 given to sayoptionwindow");
    }

    if (dialogSetOptionWindow(data[4], data[3], data[2], data[1], v1) != 0) {
        programFatalError("Error setting option window");
    }
}

// 0x4658B8
static void op_sayborder(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    // NOTE: Original code does not use loops.
    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    for (int arg = 0; arg < 2; arg++) {
        if ((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            programFatalError("Invalid arg %d given to sayborder", arg + 1);
        }
    }

    if (dialogSetBorder(data[1], data[0]) != 0) {
        programFatalError("Error setting dialog border");
    }
}

// 0x465978
static void op_sayscrollup(Program* program)
{
    opcode_t opcode[6];
    int data[6];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 6; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    char* v1 = NULL;
    char* v2 = NULL;
    char* v3 = NULL;
    char* v4 = NULL;
    int v5 = 0;

    if ((opcode[0] & VALUE_TYPE_MASK) == VALUE_TYPE_INT) {
        if (data[0] != -1 && data[0] != 0) {
            programFatalError("Invalid arg 4 given to sayscrollup");
        }

        if (data[0] == -1) {
            v5 = 1;
        }
    } else {
        if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
            programFatalError("Invalid arg 4 given to sayscrollup");
        }
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING && (opcode[1] & VALUE_TYPE_MASK) == VALUE_TYPE_INT && data[1] != 0) {
        programFatalError("Invalid arg 3 given to sayscrollup");
    }

    if ((opcode[2] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING && (opcode[2] & VALUE_TYPE_MASK) == VALUE_TYPE_INT && data[2] != 0) {
        programFatalError("Invalid arg 2 given to sayscrollup");
    }

    if ((opcode[3] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING && (opcode[3] & VALUE_TYPE_MASK) == VALUE_TYPE_INT && data[3] != 0) {
        programFatalError("Invalid arg 1 given to sayscrollup");
    }

    if ((opcode[3] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        v1 = programGetString(program, opcode[3], data[3]);
        v1 = _interpretMangleName(v1);
        v1 = strdup_safe(v1, __FILE__, __LINE__); // "..\\int\\INTLIB.C", 1611
    }

    if ((opcode[2] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        v2 = programGetString(program, opcode[2], data[2]);
        v2 = _interpretMangleName(v2);
        v2 = strdup_safe(v2, __FILE__, __LINE__); // "..\\int\\INTLIB.C", 1613
    }

    if ((opcode[1] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        v3 = programGetString(program, opcode[1], data[1]);
        v3 = _interpretMangleName(v3);
        v3 = strdup_safe(v3, __FILE__, __LINE__); // "..\\int\\INTLIB.C", 1615
    }

    if ((opcode[0] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        v4 = programGetString(program, opcode[0], data[0]);
        v4 = _interpretMangleName(v4);
        v4 = strdup_safe(v4, __FILE__, __LINE__); // "..\\int\\INTLIB.C", 1617
    }

    if (dialogSetScrollUp(data[5], data[4], v1, v2, v3, v4, v5) != 0) {
        programFatalError("Error setting scroll up");
    }
}

// 0x465CAC
static void op_sayscrolldown(Program* program)
{
    opcode_t opcode[6];
    int data[6];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 6; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode[arg], data[arg]);
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
            programFatalError("Invalid arg 4 given to sayscrollup");
        }

        if (data[0] == -1) {
            v5 = 1;
        }
    } else {
        if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
            // FIXME: Wrong function name, should be sayscrolldown.
            programFatalError("Invalid arg 4 given to sayscrollup");
        }
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING && (opcode[1] & VALUE_TYPE_MASK) == VALUE_TYPE_INT && data[1] != 0) {
        programFatalError("Invalid arg 3 given to sayscrolldown");
    }

    if ((opcode[2] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING && (opcode[2] & VALUE_TYPE_MASK) == VALUE_TYPE_INT && data[2] != 0) {
        programFatalError("Invalid arg 2 given to sayscrolldown");
    }

    if ((opcode[3] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING && (opcode[3] & VALUE_TYPE_MASK) == VALUE_TYPE_INT && data[3] != 0) {
        programFatalError("Invalid arg 1 given to sayscrolldown");
    }

    if ((opcode[3] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        v1 = programGetString(program, opcode[3], data[3]);
        v1 = _interpretMangleName(v1);
        v1 = strdup_safe(v1, __FILE__, __LINE__); // "..\\int\\INTLIB.C", 1652
    }

    if ((opcode[2] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        v2 = programGetString(program, opcode[2], data[2]);
        v2 = _interpretMangleName(v2);
        v2 = strdup_safe(v2, __FILE__, __LINE__); // "..\\int\\INTLIB.C", 1654
    }

    if ((opcode[1] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        v3 = programGetString(program, opcode[1], data[1]);
        v3 = _interpretMangleName(v3);
        v3 = strdup_safe(v3, __FILE__, __LINE__); // "..\\int\\INTLIB.C", 1656
    }

    if ((opcode[0] & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        v4 = programGetString(program, opcode[0], data[0]);
        v4 = _interpretMangleName(v4);
        v4 = strdup_safe(v4, __FILE__, __LINE__); // "..\\int\\INTLIB.C", 1658
    }

    if (dialogSetScrollDown(data[5], data[4], v1, v2, v3, v4, v5) != 0) {
        programFatalError("Error setting scroll down");
    }
}

// 0x465FE0
static void op_saysetspacing(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 1 given to saysetspacing");
    }

    if (dialogSetSpacing(data) != 0) {
        programFatalError("Error setting option spacing");
    }
}

// 0x46604C
static void op_sayrestart(Program* program)
{
    if (dialogRestart() != 0) {
        programFatalError("Error restarting option");
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

    if (soundIsPlaying(sound)) {
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
        soundSetLooping(sound, 0xFFFF);
    }

    if (mode & 0x1000) {
        // mono
        soundSetChannels(sound, 2);
    }

    if (mode & 0x2000) {
        // stereo
        soundSetChannels(sound, 3);
    }

    int rc = soundLoad(sound, fileName);
    if (rc != SOUND_NO_ERROR) {
        goto err;
    }

    rc = soundPlay(sound);

    // TODO: Maybe wrong.
    switch (rc) {
    case SOUND_NO_DEVICE:
        debugPrint("soundPlay error: %s\n", "SOUND_NO_DEVICE");
        goto err;
    case SOUND_NOT_INITIALIZED:
        debugPrint("soundPlay error: %s\n", "SOUND_NOT_INITIALIZED");
        goto err;
    case SOUND_NO_SOUND:
        debugPrint("soundPlay error: %s\n", "SOUND_NO_SOUND");
        goto err;
    case SOUND_FUNCTION_NOT_SUPPORTED:
        debugPrint("soundPlay error: %s\n", "SOUND_FUNC_NOT_SUPPORTED");
        goto err;
    case SOUND_NO_BUFFERS_AVAILABLE:
        debugPrint("soundPlay error: %s\n", "SOUND_NO_BUFFERS_AVAILABLE");
        goto err;
    case SOUND_FILE_NOT_FOUND:
        debugPrint("soundPlay error: %s\n", "SOUND_FILE_NOT_FOUND");
        goto err;
    case SOUND_ALREADY_PLAYING:
        debugPrint("soundPlay error: %s\n", "SOUND_ALREADY_PLAYING");
        goto err;
    case SOUND_NOT_PLAYING:
        debugPrint("soundPlay error: %s\n", "SOUND_NOT_PLAYING");
        goto err;
    case SOUND_ALREADY_PAUSED:
        debugPrint("soundPlay error: %s\n", "SOUND_ALREADY_PAUSED");
        goto err;
    case SOUND_NOT_PAUSED:
        debugPrint("soundPlay error: %s\n", "SOUND_NOT_PAUSED");
        goto err;
    case SOUND_INVALID_HANDLE:
        debugPrint("soundPlay error: %s\n", "SOUND_INVALID_HANDLE");
        goto err;
    case SOUND_NO_MEMORY_AVAILABLE:
        debugPrint("soundPlay error: %s\n", "SOUND_NO_MEMORY");
        goto err;
    case SOUND_UNKNOWN_ERROR:
        debugPrint("soundPlay error: %s\n", "SOUND_ERROR");
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
    if (_soundType(sound, 0x01)) {
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

    if (!soundIsPlaying(sound)) {
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
    if (_soundType(sound, 0x01)) {
        rc = soundPlay(sound);
    } else {
        rc = soundResume(sound);
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
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            _interpretDecStringRef(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 2 given to soundplay");
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        programFatalError("Invalid arg 1 given to soundplay");
    }

    char* fileName = programGetString(program, opcode[1], data[1]);
    char* mangledFileName = _interpretMangleName(fileName);
    int rc = soundStartInterpret(mangledFileName, data[0]);

    programStackPushInt32(program, rc);
    programStackPushInt16(program, VALUE_TYPE_INT);
}

// 0x466768
static void op_soundpause(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (data == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 1 given to soundpause");
    }

    soundPauseInterpret(data);
}

// 0x4667C0
static void op_soundresume(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (data == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 1 given to soundresume");
    }

    soundUnpauseInterpret(data);
}

// 0x466818
static void op_soundstop(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (data == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 1 given to soundstop");
    }

    soundPauseInterpret(data);
}

// 0x466870
static void op_soundrewind(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (data == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 1 given to soundrewind");
    }

    soundRewindInterpret(data);
}

// 0x4668C8
static void op_sounddelete(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (data == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 1 given to sounddelete");
    }

    soundDeleteInterpret(data);
}

// 0x466920
static void op_setoneoptpause(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        _interpretDecStringRef(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("SetOneOptPause: invalid arg passed (non-integer).");
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
    _nevs_update();
    updateIntExtra();
}

// 0x4669A0
void intlibClose()
{
    dialogClose();
    intExtraClose();

    // NOTE: Uninline.
    soundCloseInterpret();

    _nevs_close();

    if (callbacks != NULL) {
        internal_free_safe(callbacks, __FILE__, __LINE__); // "..\\int\\INTLIB.C", 1976
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
            _executeProc(anyKeyProg, anyKeyOffset);
        }
        return true;
    }

    IntLibKeyHandlerEntry* entry = &(inputProc[key]);
    if (entry->program == NULL) {
        return false;
    }

    if (entry->proc != 0) {
        _executeProc(entry->program, entry->proc);
    }

    return true;
}

// 0x466A70
void initIntlib()
{
    _windowAddInputFunc(intLibDoInput);

    interpreterRegisterOpcode(0x806A, op_fillwin3x3);
    interpreterRegisterOpcode(0x808C, op_deletebutton);
    interpreterRegisterOpcode(0x8086, op_addbutton);
    interpreterRegisterOpcode(0x8088, op_addbuttonflag);
    interpreterRegisterOpcode(0x8087, op_addbuttontext);
    interpreterRegisterOpcode(0x8089, op_addbuttongfx);
    interpreterRegisterOpcode(0x808A, op_addbuttonproc);
    interpreterRegisterOpcode(0x808B, op_addbuttonrightproc);
    interpreterRegisterOpcode(0x8067, op_showwin);
    interpreterRegisterOpcode(0x8068, op_fillwin);
    interpreterRegisterOpcode(0x8069, op_fillrect);
    interpreterRegisterOpcode(0x8072, op_print);
    interpreterRegisterOpcode(0x8073, op_format);
    interpreterRegisterOpcode(0x8074, op_printrect);
    interpreterRegisterOpcode(0x8075, op_setfont);
    interpreterRegisterOpcode(0x8076, op_settextflags);
    interpreterRegisterOpcode(0x8077, op_settextcolor);
    interpreterRegisterOpcode(0x8078, op_sethighlightcolor);
    interpreterRegisterOpcode(0x8064, op_selectwin);
    interpreterRegisterOpcode(0x806B, op_display);
    interpreterRegisterOpcode(0x806D, op_displayraw);
    interpreterRegisterOpcode(0x806C, op_displaygfx);
    interpreterRegisterOpcode(0x806F, op_fadein);
    interpreterRegisterOpcode(0x8070, op_fadeout);
    interpreterRegisterOpcode(0x807A, op_playmovie);
    interpreterRegisterOpcode(0x807B, op_movieflags);
    interpreterRegisterOpcode(0x807C, op_playmovierect);
    interpreterRegisterOpcode(0x8079, op_stopmovie);
    interpreterRegisterOpcode(0x807F, op_addregion);
    interpreterRegisterOpcode(0x8080, op_addregionflag);
    interpreterRegisterOpcode(0x8081, op_addregionproc);
    interpreterRegisterOpcode(0x8082, op_addregionrightproc);
    interpreterRegisterOpcode(0x8083, op_deleteregion);
    interpreterRegisterOpcode(0x8084, op_activateregion);
    interpreterRegisterOpcode(0x8085, op_checkregion);
    interpreterRegisterOpcode(0x8062, op_createwin);
    interpreterRegisterOpcode(0x8063, op_deletewin);
    interpreterRegisterOpcode(0x8065, op_resizewin);
    interpreterRegisterOpcode(0x8066, op_scalewin);
    interpreterRegisterOpcode(0x804E, op_saystart);
    interpreterRegisterOpcode(0x804F, op_saystartpos);
    interpreterRegisterOpcode(0x8050, op_sayreplytitle);
    interpreterRegisterOpcode(0x8051, op_saygotoreply);
    interpreterRegisterOpcode(0x8053, op_sayreply);
    interpreterRegisterOpcode(0x8052, op_sayoption);
    interpreterRegisterOpcode(0x804D, op_sayend);
    interpreterRegisterOpcode(0x804C, op_sayquit);
    interpreterRegisterOpcode(0x8054, op_saymessage);
    interpreterRegisterOpcode(0x8055, op_sayreplywindow);
    interpreterRegisterOpcode(0x8056, op_sayoptionwindow);
    interpreterRegisterOpcode(0x805F, op_sayreplyflags);
    interpreterRegisterOpcode(0x8060, op_sayoptionflags);
    interpreterRegisterOpcode(0x8057, op_sayborder);
    interpreterRegisterOpcode(0x8058, op_sayscrollup);
    interpreterRegisterOpcode(0x8059, op_sayscrolldown);
    interpreterRegisterOpcode(0x805A, op_saysetspacing);
    interpreterRegisterOpcode(0x805B, op_sayoptioncolor);
    interpreterRegisterOpcode(0x805C, op_sayreplycolor);
    interpreterRegisterOpcode(0x805D, op_sayrestart);
    interpreterRegisterOpcode(0x805E, op_saygetlastpos);
    interpreterRegisterOpcode(0x8061, op_saymessagetimeout);
    interpreterRegisterOpcode(0x8071, op_gotoxy);
    interpreterRegisterOpcode(0x808D, op_hidemouse);
    interpreterRegisterOpcode(0x808E, op_showmouse);
    interpreterRegisterOpcode(0x8090, op_refreshmouse);
    interpreterRegisterOpcode(0x808F, op_mouseshape);
    interpreterRegisterOpcode(0x8091, op_setglobalmousefunc);
    interpreterRegisterOpcode(0x806E, op_loadpalettetable);
    interpreterRegisterOpcode(0x8092, op_addNamedEvent);
    interpreterRegisterOpcode(0x8093, op_addNamedHandler);
    interpreterRegisterOpcode(0x8094, op_clearNamed);
    interpreterRegisterOpcode(0x8095, op_signalNamed);
    interpreterRegisterOpcode(0x8096, op_addkey);
    interpreterRegisterOpcode(0x8097, op_deletekey);
    interpreterRegisterOpcode(0x8098, op_soundplay);
    interpreterRegisterOpcode(0x8099, op_soundpause);
    interpreterRegisterOpcode(0x809A, op_soundresume);
    interpreterRegisterOpcode(0x809B, op_soundstop);
    interpreterRegisterOpcode(0x809C, op_soundrewind);
    interpreterRegisterOpcode(0x809D, op_sounddelete);
    interpreterRegisterOpcode(0x809E, op_setoneoptpause);
    interpreterRegisterOpcode(0x809F, op_selectfilelist);
    interpreterRegisterOpcode(0x80A0, op_tokenize);

    _nevs_initonce();
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
            callbacks = (IntLibProgramDeleteCallback**)internal_realloc_safe(callbacks, sizeof(*callbacks) * (numCallbacks + 1), __FILE__, __LINE__); // ..\\int\\INTLIB.C, 2110
        } else {
            callbacks = (IntLibProgramDeleteCallback**)internal_malloc_safe(sizeof(*callbacks), __FILE__, __LINE__); // ..\\int\\INTLIB.C, 2112
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
