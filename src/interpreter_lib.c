#include "interpreter_lib.h"

#include "color.h"
#include "core.h"
#include "datafile.h"
#include "debug.h"
#include "dialog.h"
#include "interpreter_extra.h"
#include "memory_manager.h"
#include "mouse_manager.h"
#include "nevs.h"
#include "select_file_list.h"
#include "text_font.h"
#include "widget.h"
#include "window_manager_private.h"

// 0x59D5D0
Sound* gIntLibSounds[INT_LIB_SOUNDS_CAPACITY];

// 0x59D650
unsigned char gIntLibFadePalette[256 * 3];

// 0x59D950
IntLibKeyHandlerEntry gIntLibKeyHandlerEntries[INT_LIB_KEY_HANDLERS_CAPACITY];

// 0x59E150
bool gIntLibIsPaletteFaded;

// 0x59E154
int gIntLibGenericKeyHandlerProc;

// 0x59E158
int gIntLibProgramDeleteCallbacksLength;

// 0x59E15C
Program* gIntLibGenericKeyHandlerProgram;

// 0x59E160
IntLibProgramDeleteCallback** gIntLibProgramDeleteCallbacks;

// 0x59E164
int gIntLibSayStartingPosition;

// 0x59E168
char gIntLibPlayMovieFileName[100];

// 0x59E1CC
char gIntLibPlayMovieRectFileName[100];

// fillwin3x3
// 0x461780
void opFillWin3x3(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        programFatalError("Invalid type given to fillwin3x3");
    }

    char* fileName = programGetString(program, opcode, data);
    char* mangledFileName = _interpretMangleName(fileName);

    int imageWidth;
    int imageHeight;
    unsigned char* imageData = datafileRead(mangledFileName, &imageWidth, &imageHeight);
    if (imageData == NULL) {
        programFatalError("cannot load 3x3 file '%s'", mangledFileName);
    }

    _selectWindowID(program->windowId);

    int windowHeight = _windowHeight();
    int windowWidth = _windowWidth();
    unsigned char* windowBuffer = _windowGetBuffer();
    sub_4BBFC4(imageData,
        imageWidth,
        imageHeight,
        windowBuffer,
        windowWidth,
        windowHeight);

    internal_free_safe(imageData, __FILE__, __LINE__); // "..\\int\\INTLIB.C", 94
}

// format
// 0x461850
void opFormat(Program* program)
{
    opcode_t opcode[6];
    int data[6];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 6; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            programPopString(program, opcode[arg], data[arg]);
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

// print
// 0x461A5C
void opPrint(Program* program)
{
    _selectWindowID(program->windowId);

    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode, data);
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

// selectfilelist
// 0x461B10
void opSelectFileList(Program* program)
{
    program->flags |= PROGRAM_FLAG_0x20;

    opcode_t opcode[2];
    int data[2];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            programPopString(program, opcode[arg], data[arg]);
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
            _colorTable[0x7FFF] | 0x10000);

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

// tokenize
// 0x461CA0
void opTokenize(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    opcode[0] = programStackPopInt16(program);
    data[0] = programStackPopInt32(program);

    if (opcode[0] == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode[0], data[0]);
    }

    if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Error, invalid arg 3 to tokenize.");
    }

    opcode[1] = programStackPopInt16(program);
    data[1] = programStackPopInt32(program);

    if (opcode[1] == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode[1], data[1]);
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
        programPopString(program, opcode[2], data[2]);
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

// printrect
// 0x461F1C
void opPrintRect(Program* program)
{
    _selectWindowID(program->windowId);

    opcode_t opcode[3];
    int data[3];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            programPopString(program, opcode[arg], data[arg]);
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
void opSelect(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode, data);
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

// display
// 0x46213C
void opDisplay(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        programFatalError("Invalid type given to display");
    }

    char* fileName = programGetString(program, opcode, data);

    _selectWindowID(program->windowId);

    char* mangledFileName = _interpretMangleName(fileName);
    _displayFile(mangledFileName);
}

// displayraw
// 0x4621B4
void opDisplayRaw(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode, data);
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
void sub_46222C(unsigned char* a1, unsigned char* a2, int a3, float a4, int a5)
{
    // TODO: Incomplete.
}

// fadein
// 0x462400
void opFadeIn(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid type given to fadein\n");
    }

    program->flags |= PROGRAM_FLAG_0x20;

    _setSystemPalette(gIntLibFadePalette);

    sub_46222C(gIntLibFadePalette, _cmap, 64, (float)data, 1);
    gIntLibIsPaletteFaded = true;

    program->flags &= ~PROGRAM_FLAG_0x20;
}

// fadeout
// 0x4624B4
void opFadeOut(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        // FIXME: Wrong function name, should be fadeout.
        programFatalError("Invalid type given to fadein\n");
    }

    program->flags |= PROGRAM_FLAG_0x20;

    bool cursorWasHidden = cursorIsHidden();
    mouseHideCursor();

    sub_46222C(_getSystemPalette(), gIntLibFadePalette, 64, (float)data, 1);

    if (!cursorWasHidden) {
        mouseShowCursor();
    }

    gIntLibIsPaletteFaded = false;

    program->flags &= ~PROGRAM_FLAG_0x20;
}

// 0x462570
int intLibCheckMovie(Program* program)
{
    if (_dialogGetDialogDepth() > 0) {
        return 1;
    }

    return _windowMoviePlaying();
}

// movieflags
// 0x462584
void opSetMovieFlags(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode, data);
    }

    if (!_windowSetMovieFlags(data)) {
        programFatalError("Error setting movie flags\n");
    }
}

// playmovie
// 0x4625D0
void opPlayMovie(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        programFatalError("Invalid type given to playmovie");
    }

    strcpy(gIntLibPlayMovieFileName, programGetString(program, opcode, data));

    if (strrchr(gIntLibPlayMovieFileName, '.') == NULL) {
        strcat(gIntLibPlayMovieFileName, ".mve");
    }

    _selectWindowID(program->windowId);

    program->flags |= PROGRAM_FLAG_0x10;
    program->field_7C = intLibCheckMovie;

    char* mangledFileName = _interpretMangleName(gIntLibPlayMovieFileName);
    if (!_windowPlayMovie(mangledFileName)) {
        programFatalError("Error playing movie");
    }
}

// playmovierect
// 0x4626C4
void opPlayMovieRect(Program* program)
{
    opcode_t opcode[5];
    int data[5];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 5; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            programPopString(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[4] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        // FIXME: Wrong function name, should playmovierect.
        programFatalError("Invalid arg given to playmovie");
    }

    strcpy(gIntLibPlayMovieRectFileName, programGetString(program, opcode[4], data[4]));

    if (strrchr(gIntLibPlayMovieRectFileName, '.') == NULL) {
        strcat(gIntLibPlayMovieRectFileName, ".mve");
    }

    _selectWindowID(program->windowId);

    program->field_7C = intLibCheckMovie;
    program->flags |= PROGRAM_FLAG_0x10;

    char* mangledFileName = _interpretMangleName(gIntLibPlayMovieRectFileName);
    if (!_windowPlayMovieRect(mangledFileName, data[3], data[2], data[1], data[0])) {
        programFatalError("Error playing movie");
    }
}

// stopmovie
// 0x46287C
void opStopMovie(Program* program)
{
    _windowStopMovie();
    program->flags |= PROGRAM_FLAG_0x40;
}

// deleteregion
// 0x462890
void opDeleteRegion(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode, data);
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

// activateregion
// 0x462924
void opActivateRegion(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 5; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            programPopString(program, opcode[arg], data[arg]);
        }
    }

    const char* regionName = programGetString(program, opcode[1], data[1]);
    sub_4B6DE8(regionName, data[0]);
}

// checkregion
// 0x4629A0
void opCheckRegion(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        programFatalError("Invalid arg 1 given to checkregion();\n");
    }

    const char* regionName = programGetString(program, opcode, data);

    bool regionExists = _windowCheckRegionExists(regionName);
    programStackPushInt32(program, regionExists);
    programStackPushInt16(program, VALUE_TYPE_INT);
}

// addregion
// 0x462A1C
void opAddRegion(Program* program)
{
    opcode_t opcode;
    int data;

    opcode = programStackPopInt16(program);
    data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode, data);
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
            programPopString(program, opcode, data);
        }

        if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
            programFatalError("Invalid y value given to region");
        }

        int y = data;

        opcode = programStackPopInt16(program);
        data = programStackPopInt32(program);

        if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
            programPopString(program, opcode, data);
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
            programPopString(program, opcode, data);
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

// addregionproc
// 0x462C10
void opAddRegionProc(Program* program)
{
    opcode_t opcode[5];
    int data[5];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 5; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            programPopString(program, opcode[arg], data[arg]);
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

// addregionrightproc
// 0x462DDC
void opAddRegionRightProc(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            programPopString(program, opcode[arg], data[arg]);
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

// createwin
// 0x462F08
void opCreateWin(Program* program)
{
    opcode_t opcode[5];
    int data[5];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 5; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            programPopString(program, opcode[arg], data[arg]);
        }
    }

    const char* windowName = programGetString(program, opcode[4], data[4]);
    int x = (data[3] * _windowGetXres() + 639) / 640;
    int y = (data[2] * _windowGetYres() + 479) / 480;
    int width = (data[1] * _windowGetXres() + 639) / 640;
    int height = (data[0] * _windowGetYres() + 479) / 480;;

    if (sub_4B7F3C(windowName, x, y, width, height, _colorTable[0], 0) == -1) {
        programFatalError("Couldn't create window.");
    }
}

// resizewin
// 0x46308C
void opResizeWin(Program* program)
{
    opcode_t opcode[5];
    int data[5];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 5; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            programPopString(program, opcode[arg], data[arg]);
        }
    }

    const char* windowName = programGetString(program, opcode[4], data[4]);
    int x = (data[3] * _windowGetXres() + 639) / 640;
    int y = (data[2] * _windowGetYres() + 479) / 480;
    int width = (data[1] * _windowGetXres() + 639) / 640;
    int height = (data[0] * _windowGetYres() + 479) / 480;;

    if (sub_4B7AC4(windowName, x, y, width, height) == -1) {
        programFatalError("Couldn't resize window.");
    }
}

// scalewin
// 0x463204
void opScaleWin(Program* program)
{
    opcode_t opcode[5];
    int data[5];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 5; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            programPopString(program, opcode[arg], data[arg]);
        }
    }

    const char* windowName = programGetString(program, opcode[4], data[4]);
    int x = (data[3] * _windowGetXres() + 639) / 640;
    int y = (data[2] * _windowGetYres() + 479) / 480;
    int width = (data[1] * _windowGetXres() + 639) / 640;
    int height = (data[0] * _windowGetYres() + 479) / 480;;

    if (sub_4B7E7C(windowName, x, y, width, height) == -1) {
        programFatalError("Couldn't scale window.");
    }
}

// deletewin
// 0x46337C
void opDeleteWin(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode, data);
    }

    const char* windowName = programGetString(program, opcode, data);

    if (!_deleteWindow(windowName)) {
        programFatalError("Error deleting window %s\n", windowName);
    }

    program->windowId = _popWindow();
}

// saystart
// 0x4633E4
void opSayStart(Program* program)
{
    gIntLibSayStartingPosition = 0;

    program->flags |= PROGRAM_FLAG_0x20;
    int rc = _dialogStart(program);
    program->flags &= ~PROGRAM_FLAG_0x20;

    if (rc != 0) {
        programFatalError("Error starting dialog.");
    }
}

// saystartpos
// 0x463430
void opSayStartPos(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode, data);
    }

    gIntLibSayStartingPosition = data;

    program->flags |= PROGRAM_FLAG_0x20;
    int rc = _dialogStart(program);
    program->flags &= ~PROGRAM_FLAG_0x20;

    if (rc != 0) {
        programFatalError("Error starting dialog.");
    }
}

// sayreplytitle
// 0x46349C
void opSayReplyTitle(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode, data);
    }

    char* string = NULL;
    if ((opcode & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        string = programGetString(program, opcode, data);
    }

    if (dialogSetReplyTitle(string) != 0) {
        programFatalError("Error setting title.");
    }
}

// saygotoreply
// 0x463510
void opSayGoToReply(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode, data);
    }

    char* string = NULL;
    if ((opcode & VALUE_TYPE_MASK) == VALUE_TYPE_STRING) {
        string = programGetString(program, opcode, data);
    }

    if (_dialogGotoReply(string) != 0) {
        programFatalError("Error during goto, couldn't find reply target %s", string);
    }
}

// sayreply
// 0x463584
void opSayReply(Program* program)
{
    program->flags |= PROGRAM_FLAG_0x20;

    opcode_t opcode[2];
    int data[2];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            programPopString(program, opcode[arg], data[arg]);
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
        if (_dialogOption(v1, v2) != 0) {
            program->flags &= ~PROGRAM_FLAG_0x20;
            programFatalError("Error setting option.");
        }
    } else if ((opcode[0] & VALUE_TYPE_MASK) == VALUE_TYPE_INT) {
        if (_dialogOptionProc(v1, data[0]) != 0) {
            program->flags &= ~PROGRAM_FLAG_0x20;
            programFatalError("Error setting option.");
        }
    } else {
        programFatalError("Invalid arg 2 to sayOption");
    }

    program->flags &= ~PROGRAM_FLAG_0x20;
}

// sayoption
void opSayOption(Program* program)
{
    program->flags |= PROGRAM_FLAG_0x20;

    opcode_t opcode[2];
    int data[2];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            programPopString(program, opcode[arg], data[arg]);
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

    if (_dialogReply(v1, v2) != 0) {
        program->flags &= ~PROGRAM_FLAG_0x20;
        programFatalError("Error setting option.");
    }

    program->flags &= ~PROGRAM_FLAG_0x20;
}

// 0x46378C
int intLibCheckDialog(Program* program)
{
    program->flags |= PROGRAM_FLAG_0x40;
    return _dialogGetDialogDepth() != -1;
}

// 0x4637A4
void opSayEnd(Program* program)
{
    program->flags |= PROGRAM_FLAG_0x20;
    int rc = sub_431088(gIntLibSayStartingPosition);
    program->flags &= ~PROGRAM_FLAG_0x20;

    if (rc == -2) {
        program->field_7C = intLibCheckDialog;
        program->flags |= PROGRAM_FLAG_0x10;
    }
}

// saygetlastpos
// 0x4637EC
void opSayGetLastPos(Program* program)
{
    int value = _dialogGetExitPoint();
    programStackPushInt32(program, value);
    programStackPushInt16(program, VALUE_TYPE_INT);
}

// sayquit
// 0x463810
void opSayQuit(Program* program)
{
    if (_dialogQuit() != 0) {
        programFatalError("Error quitting option.");
    }
}

// saymessagetimeout
// 0x463838
void opSayMessageTimeout(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode, data);
    }

    // TODO: What the hell is this?
    if ((opcode & VALUE_TYPE_MASK) == 0x4000) {
        programFatalError("sayMsgTimeout:  invalid var type passed.");
    }

    _TimeOut = data;
}

// saymessage
// 0x463890
void opSayMessage(Program* program)
{
    program->flags |= PROGRAM_FLAG_0x20;

    opcode_t opcode[2];
    int data[2];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            programPopString(program, opcode[arg], data[arg]);
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

    if (sub_430FD4(v1, v2, _TimeOut) != 0) {
        program->flags &= ~PROGRAM_FLAG_0x20;
        programFatalError("Error setting option.");
    }

    program->flags &= ~PROGRAM_FLAG_0x20;
}

// gotoxy
// 0x463980
void opGotoXY(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            programPopString(program, opcode[arg], data[arg]);
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

// addbuttonflag
// 0x463A38
void opAddButtonFlag(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            programPopString(program, opcode[arg], data[arg]);
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

// addregionflag
// 0x463B10
void opAddRegionFlag(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            programPopString(program, opcode[arg], data[arg]);
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

// addbutton
// 0x463BE8
void opAddButton(Program* program)
{
    opcode_t opcode[5];
    int data[5];

    opcode[0] = programStackPopInt16(program);
    data[0] = programStackPopInt32(program);

    if (opcode[0] == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode[0], data[0]);
    }

    if ((opcode[0] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid height given to addbutton");
    }

    opcode[1] = programStackPopInt16(program);
    data[1] = programStackPopInt32(program);

    if (opcode[1] == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode[1], data[1]);
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid width given to addbutton");
    }

    opcode[2] = programStackPopInt16(program);
    data[2] = programStackPopInt32(program);

    if (opcode[2] == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode[2], data[2]);
    }

    if ((opcode[2] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid y given to addbutton");
    }

    opcode[3] = programStackPopInt16(program);
    data[3] = programStackPopInt32(program);

    if (opcode[3] == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode[3], data[3]);
    }

    if ((opcode[3] & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid x given to addbutton");
    }

    opcode[4] = programStackPopInt16(program);
    data[4] = programStackPopInt32(program);

    if (opcode[4] == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode[4], data[4]);
    }

    if ((opcode[4] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        programFatalError("Invalid name given to addbutton");
    }

    _selectWindowID(program->windowId);

    int height = (data[0] * _windowGetYres() + 479) / 480;
    int width = (data[1] * _windowGetXres() + 639) / 640;
    int y = (data[2] * _windowGetYres() + 479) / 480;
    int x = (data[1] * _windowGetXres() + 639) / 640;
    const char* buttonName = programGetString(program, opcode[4], data[4]);

    _windowAddButton(buttonName, x, y, width, height, 0);
}

// addbuttontext
// 0x463DF4
void opAddButtonText(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            programPopString(program, opcode[arg], data[arg]);
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

// addbuttongfx
// 0x463EEC
void opAddButtonGfx(Program* program)
{
    opcode_t opcode[4];
    int data[4];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 4; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            programPopString(program, opcode[arg], data[arg]);
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

// addbuttonproc
// 0x4640DC
void opAddButtonProc(Program* program)
{
    opcode_t opcode[5];
    int data[5];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 5; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            programPopString(program, opcode[arg], data[arg]);
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

// addbuttonrightproc
// 0x4642A8
void opAddButtonRightProc(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            programPopString(program, opcode[arg], data[arg]);
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

// showwin
// 0x4643D4
void opShowWin(Program* program)
{
    _selectWindowID(program->windowId);
    _windowDraw();
}

// deletebutton
// 0x4643E4
void opDeleteButton(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode, data);
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

// fillwin
// 0x46449C
void opFillWin(Program* program)
{
    opcode_t opcode[3];
    int data[3];
    float* floats = (float*)data;

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            programPopString(program, opcode[arg], data[arg]);
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

// fillrect
// 0x4645FC
void opFillRect(Program* program)
{
    opcode_t opcode[7];
    int data[7];
    float* floats = (float*)data;

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 7; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            programPopString(program, opcode[arg], data[arg]);
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

// hidemouse
// 0x46489C
void opHideMouse(Program* program)
{
    mouseHideCursor();
}

// showmouse
// 0x4648A4
void opShowMouse(Program* program)
{
    mouseShowCursor();
}

// mouseshape
// 0x4648AC
void opMouseShape(Program* program)
{
    opcode_t opcode[3];
    int data[3];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            programPopString(program, opcode[arg], data[arg]);
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

    char *fileName = programGetString(program, opcode[2], data[2]);
    if (!sub_485E58(fileName, data[1], data[0])) {
        programFatalError("Error loading mouse shape.");
    }
}

// setglobalmousefunc
// 0x4649C4
void opSetGlobalMouseFunc(Program* Program)
{
    programFatalError("setglobalmousefunc not defined");
}

// displaygfx
// 0x4649D4
void opDisplayGfx(Program* program)
{
    opcode_t opcode[5];
    int data[5];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 5; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            programPopString(program, opcode[arg], data[arg]);
        }
    }

    char* fileName = programGetString(program, opcode[4], data[4]);
    char* mangledFileName = _interpretMangleName(fileName);
    _windowDisplay(mangledFileName, data[3], data[2], data[1], data[0]);
}

// loadpalettetable
// 0x464ADC
void opLoadPaletteTable(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        if ((opcode & VALUE_TYPE_MASK) == VALUE_TYPE_INT && data != -1) {
            programFatalError("Invalid type given to loadpalettetable");
        }
    }

    char* path = programGetString(program, opcode, data);
    if (!colorPaletteLoad(path)) {
        programFatalError(_colorError());
    }
}

// addnamedevent
// 0x464B54
void opAddNamedEvent(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            programPopString(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        programFatalError("Invalid type given to addnamedevent");
    }

    const char* v1 = programGetString(program, opcode[1], data[1]);
    _nevs_addevent(v1, program, data[0], NEVS_TYPE_EVENT);
}

// addnamedhandler
// 0x464BE8
void opAddNamedHandler(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            programPopString(program, opcode[arg], data[arg]);
        }
    }

    if ((opcode[1] & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        programFatalError("Invalid type given to addnamedhandler");
    }

    const char* v1 = programGetString(program, opcode[1], data[1]);
    _nevs_addevent(v1, program, data[0], NEVS_TYPE_HANDLER);
}

// clearnamed
// 0x464C80
void opClearNamed(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        programFatalError("Invalid type given to clearnamed");
    }

    char* string = programGetString(program, opcode, data);
    _nevs_clearevent(string);
}

// signalnamed
// 0x464CE4
void opSignalNamed(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_STRING) {
        programFatalError("Invalid type given to signalnamed");
    }

    char* str = programGetString(program, opcode, data);
    _nevs_signal(str);
}

// addkey
// 0x464D48
void opAddKey(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            programPopString(program, opcode[arg], data[arg]);
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
        gIntLibGenericKeyHandlerProc = proc;
        gIntLibGenericKeyHandlerProgram = program;
    } else {
        if (key > INT_LIB_KEY_HANDLERS_CAPACITY - 1) {
            programFatalError("Key out of range");
        }

        gIntLibKeyHandlerEntries[key].program = program;
        gIntLibKeyHandlerEntries[key].proc = proc;
    }
}

// deletekey
// 0x464E24
void opDeleteKey(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 1 given to deletekey");
    }

    int key = data;

    if (key == -1) {
        gIntLibGenericKeyHandlerProc = 0;
        gIntLibGenericKeyHandlerProgram = NULL;
    } else {
        if (key > INT_LIB_KEY_HANDLERS_CAPACITY - 1) {
            programFatalError("Key out of range");
        }

        gIntLibKeyHandlerEntries[key].program = NULL;
        gIntLibKeyHandlerEntries[key].proc = 0;
    }
}

// refreshmouse
// 0x464EB0
void opRefreshMouse(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 1 given to refreshmouse");
    }

    if (!sub_4B69BC()) {
        _executeProc(program, data);
    }
}

// setfont
// 0x464F18
void opSetFont(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 1 given to setfont");
    }

    if (!widgetSetFont(data)) {
        programFatalError("Error setting font");
    }
}

// setflags
// 0x464F84
void opSetTextFlags(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 1 given to setflags");
    }

    if (!widgetSetTextFlags(data)) {
        programFatalError("Error setting text flags");
    }
}

// settextcolor
// 0x464FF0
void opSetTextColor(Program* program)
{
    opcode_t opcode[3];
    int data[3];
    float* floats = (float*)data;

    // NOTE: Original code does not use loops.
    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            programPopString(program, opcode[arg], data[arg]);
        }
    }

    for (int arg = 0; arg < 3; arg++) {
        if (((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_FLOAT && (opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT)
            || floats[arg] == 0.0) {
            programFatalError("Invalid type given to settextcolor");
        }
    }

    float r = floats[2];
    float g = floats[1];
    float b = floats[0];

    if (!widgetSetTextColor(r, g, b)) {
        programFatalError("Error setting text color");
    }
}

// sayoptioncolor
// 0x465140
void opSayOptionColor(Program* program)
{
    opcode_t opcode[3];
    int data[3];
    float* floats = (float*)data;

    // NOTE: Original code does not use loops.
    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            programPopString(program, opcode[arg], data[arg]);
        }
    }

    for (int arg = 0; arg < 3; arg++) {
        if (((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_FLOAT && (opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT)
            || floats[arg] == 0.0) {
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

// sayreplycolor
// 0x465290
void opSayReplyColor(Program* program)
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
            programPopString(program, opcode[arg], data[arg]);
        }
    }

    for (int arg = 0; arg < 3; arg++) {
        if (((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_FLOAT && (opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT)
            || floats[arg] == 0.0) {
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

// sethighlightcolor
// 0x4653E0
void opSetHighlightColor(Program* program)
{
    opcode_t opcode[3];
    int data[3];
    float* floats = (float*)data;

    // NOTE: Original code does not use loops.
    for (int arg = 0; arg < 3; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            programPopString(program, opcode[arg], data[arg]);
        }
    }

    for (int arg = 0; arg < 3; arg++) {
        if (((opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_FLOAT && (opcode[arg] & VALUE_TYPE_MASK) != VALUE_TYPE_INT)
            || floats[arg] == 0.0) {
            programFatalError("Invalid type given to sethighlightcolor");
        }
    }

    float r = floats[2];
    float g = floats[1];
    float b = floats[0];

    if (!widgetSetHighlightColor(r, g, b)) {
        programFatalError("Error setting text highlight color");
    }
}

// sayreplywindow
// 0x465530
void opSayReplyWindow(Program* program)
{
    opcode_t opcode[5];
    int data[5];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 5; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            programPopString(program, opcode[arg], data[arg]);
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

// sayreplyflags
// 0x465688
void opSayReplyFlags(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 1 given to sayreplyflags");
    }

    if (!_dialogSetOptionFlags(data)) {
        programFatalError("Error setting reply flags");
    }
}

// sayoptionflags
// 0x4656F4
void opSayOptionFlags(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 1 given to sayoptionflags");
    }

    if (!_dialogSetOptionFlags(data)) {
        programFatalError("Error setting option flags");
    }
}

// sayoptionwindow
// 0x465760
void opSayOptionWindow(Program* program)
{
    opcode_t opcode[5];
    int data[5];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 5; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            programPopString(program, opcode[arg], data[arg]);
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

// sayborder
// 0x4658B8
void opSayBorder(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    // NOTE: Original code does not use loops.
    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            programPopString(program, opcode[arg], data[arg]);
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

// sayscrollup
// 0x465978
void opSayScrollUp(Program* program)
{
    opcode_t opcode[6];
    int data[6];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 6; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            programPopString(program, opcode[arg], data[arg]);
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

    if (_dialogSetScrollUp(data[5], data[4], v1, v2, v3, v4, v5) != 0) {
        programFatalError("Error setting scroll up");
    }
}

// sayscrolldown
// 0x465CAC
void opSayScrollDown(Program* program)
{
    opcode_t opcode[6];
    int data[6];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 6; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            programPopString(program, opcode[arg], data[arg]);
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

    if (_dialogSetScrollDown(data[5], data[4], v1, v2, v3, v4, v5) != 0) {
        programFatalError("Error setting scroll down");
    }
}

// saysetspacing
// 0x465FE0
void opSaySetSpacing(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 1 given to saysetspacing");
    }

    if (dialogSetOptionSpacing(data) != 0) {
        programFatalError("Error setting option spacing");
    }
}

// sayrestart
// 0x46604C
void opSayRestart(Program* program)
{
    if (_dialogRestart() != 0) {
        programFatalError("Error restarting option");
    }
}

// 0x466064
void intLibSoundCallback(void* userData, int a2)
{
    if (a2 == 1) {
        Sound** sound = (Sound**)userData;
        *sound = NULL;
    }
}

// 0x466070
int intLibSoundDelete(int value)
{
    if (value == -1) {
        return 1;
    }

    if ((value & 0xA0000000) == 0) {
        return 0;
    }

    int index = value & ~0xA0000000;
    Sound* sound = gIntLibSounds[index];
    if (sound == NULL) {
        return 0;
    }

    if (soundIsPlaying(sound)) {
        soundStop(sound);
    }

    soundDelete(sound);

    gIntLibSounds[index] = NULL;

    return 1;
}

// 0x466110
int intLibSoundPlay(char* fileName, int mode)
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
        if (gIntLibSounds[index] == NULL) {
            break;
        }
    }

    if (index == INT_LIB_SOUNDS_CAPACITY) {
        return -1;
    }

    Sound* sound = gIntLibSounds[index] = soundAllocate(v3, v5);
    if (sound == NULL) {
        return -1;
    }

    soundSetCallback(sound, intLibSoundCallback, &(gIntLibSounds[index]));

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
    gIntLibSounds[index] = NULL;
    return -1;
}

// 0x46655C
int intLibSoundPause(int value)
{
    if (value == -1) {
        return 1;
    }

    if ((value & 0xA0000000) == 0) {
        return 0;
    }

    int index = value & ~0xA0000000;
    Sound* sound = gIntLibSounds[index];
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
int intLibSoundRewind(int value)
{
    if (value == -1) {
        return 1;
    }

    if ((value & 0xA0000000) == 0) {
        return 0;
    }

    int index = value & ~0xA0000000;
    Sound* sound = gIntLibSounds[index];
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
int intLibSoundResume(int value)
{
    if (value == -1) {
        return 1;
    }

    if ((value & 0xA0000000) == 0) {
        return 0;
    }

    int index = value & ~0xA0000000;
    Sound* sound = gIntLibSounds[index];
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

// soundplay
// 0x466698
void opSoundPlay(Program* program)
{
    opcode_t opcode[2];
    int data[2];

    // NOTE: Original code does not use loop.
    for (int arg = 0; arg < 2; arg++) {
        opcode[arg] = programStackPopInt16(program);
        data[arg] = programStackPopInt32(program);

        if (opcode[arg] == VALUE_TYPE_DYNAMIC_STRING) {
            programPopString(program, opcode[arg], data[arg]);
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
    int rc = intLibSoundPlay(mangledFileName, data[0]);

    programStackPushInt32(program, rc);
    programStackPushInt16(program, VALUE_TYPE_INT);
}

// soundpause
// 0x466768
void opSoundPause(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (data == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 1 given to soundpause");
    }

    intLibSoundPause(data);
}

// soundresume
// 0x4667C0
void opSoundResume(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (data == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 1 given to soundresume");
    }

    intLibSoundResume(data);
}

// soundstop
// 0x466818
void opSoundStop(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (data == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 1 given to soundstop");
    }

    intLibSoundPause(data);
}

// soundrewind
// 0x466870
void opSoundRewind(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (data == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 1 given to soundrewind");
    }

    intLibSoundRewind(data);
}

// sounddelete
// 0x4668C8
void opSoundDelete(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (data == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 1 given to sounddelete");
    }

    intLibSoundDelete(data);
}

// SetOneOptPause
// 0x466920
void opSetOneOptPause(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode, data);
    }

    if ((opcode & VALUE_TYPE_MASK) != VALUE_TYPE_INT) {
        programFatalError("SetOneOptPause: invalid arg passed (non-integer).");
    }

    if (data) {
        if ((_dialogGetMediaFlag() & 8) == 0) {
            return;
        }
    } else {
        if ((_dialogGetMediaFlag() & 8) != 0) {
            return;
        }
    }

    _dialogToggleMediaFlag(8);
}

// 0x466994
void intLibUpdate()
{
    _nevs_update();
    intExtraUpdate();
}

// 0x4669A0
void intLibExit()
{
    _dialogClose();
    _intExtraClose_();

    for (int index = 0; index < INT_LIB_SOUNDS_CAPACITY; index++) {
        if (gIntLibSounds[index] != NULL) {
            intLibSoundDelete(index | 0xA0000000);
        }
    }

    _nevs_close();

    if (gIntLibProgramDeleteCallbacks != NULL) {
        internal_free_safe(gIntLibProgramDeleteCallbacks, __FILE__, __LINE__); // "..\\int\\INTLIB.C", 1976
        gIntLibProgramDeleteCallbacks = NULL;
        gIntLibProgramDeleteCallbacksLength = 0;
    }
}

// 0x466A04
bool intLibDoInput(int key)
{
    if (key < 0 || key >= INT_LIB_KEY_HANDLERS_CAPACITY) {
        return false;
    }

    if (gIntLibGenericKeyHandlerProgram != NULL) {
        if (gIntLibGenericKeyHandlerProc != 0) {
            _executeProc(gIntLibGenericKeyHandlerProgram, gIntLibGenericKeyHandlerProc);
        }
        return true;
    }

    IntLibKeyHandlerEntry* entry = &(gIntLibKeyHandlerEntries[key]);
    if (entry->program == NULL) {
        return false;
    }

    if (entry->proc != 0) {
        _executeProc(entry->program, entry->proc);
    }

    return true;
}

// 0x466A70
void intLibInit()
{
    sub_4B6C48(intLibDoInput);

    interpreterRegisterOpcode(0x806A, opFillWin3x3);
    interpreterRegisterOpcode(0x808C, opDeleteButton);
    interpreterRegisterOpcode(0x8086, opAddButton);
    interpreterRegisterOpcode(0x8088, opAddButtonFlag);
    interpreterRegisterOpcode(0x8087, opAddButtonText);
    interpreterRegisterOpcode(0x8089, opAddButtonGfx);
    interpreterRegisterOpcode(0x808A, opAddButtonProc);
    interpreterRegisterOpcode(0x808B, opAddButtonRightProc);
    interpreterRegisterOpcode(0x8067, opShowWin);
    interpreterRegisterOpcode(0x8068, opFillWin);
    interpreterRegisterOpcode(0x8069, opFillRect);
    interpreterRegisterOpcode(0x8072, opPrint);
    interpreterRegisterOpcode(0x8073, opFormat);
    interpreterRegisterOpcode(0x8074, opPrintRect);
    interpreterRegisterOpcode(0x8075, opSetFont);
    interpreterRegisterOpcode(0x8076, opSetTextFlags);
    interpreterRegisterOpcode(0x8077, opSetTextColor);
    interpreterRegisterOpcode(0x8078, opSetHighlightColor);
    interpreterRegisterOpcode(0x8064, opSelect);
    interpreterRegisterOpcode(0x806B, opDisplay);
    interpreterRegisterOpcode(0x806D, opDisplayRaw);
    interpreterRegisterOpcode(0x806C, opDisplayGfx);
    interpreterRegisterOpcode(0x806F, opFadeIn);
    interpreterRegisterOpcode(0x8070, opFadeOut);
    interpreterRegisterOpcode(0x807A, opPlayMovie);
    interpreterRegisterOpcode(0x807B, opSetMovieFlags);
    interpreterRegisterOpcode(0x807C, opPlayMovieRect);
    interpreterRegisterOpcode(0x8079, opStopMovie);
    interpreterRegisterOpcode(0x807F, opAddRegion);
    interpreterRegisterOpcode(0x8080, opAddRegionFlag);
    interpreterRegisterOpcode(0x8081, opAddRegionProc);
    interpreterRegisterOpcode(0x8082, opAddRegionRightProc);
    interpreterRegisterOpcode(0x8083, opDeleteRegion);
    interpreterRegisterOpcode(0x8084, opActivateRegion);
    interpreterRegisterOpcode(0x8085, opCheckRegion);
    interpreterRegisterOpcode(0x8062, opCreateWin);
    interpreterRegisterOpcode(0x8063, opDeleteWin);
    interpreterRegisterOpcode(0x8065, opResizeWin);
    interpreterRegisterOpcode(0x8066, opScaleWin);
    interpreterRegisterOpcode(0x804E, opSayStart);
    interpreterRegisterOpcode(0x804F, opSayStartPos);
    interpreterRegisterOpcode(0x8050, opSayReplyTitle);
    interpreterRegisterOpcode(0x8051, opSayGoToReply);
    interpreterRegisterOpcode(0x8053, opSayReply);
    interpreterRegisterOpcode(0x8052, opSayOption);
    interpreterRegisterOpcode(0x804D, opSayEnd);
    interpreterRegisterOpcode(0x804C, opSayQuit);
    interpreterRegisterOpcode(0x8054, opSayMessage);
    interpreterRegisterOpcode(0x8055, opSayReplyWindow);
    interpreterRegisterOpcode(0x8056, opSayOptionWindow);
    interpreterRegisterOpcode(0x805F, opSayReplyFlags);
    interpreterRegisterOpcode(0x8060, opSayOptionFlags);
    interpreterRegisterOpcode(0x8057, opSayBorder);
    interpreterRegisterOpcode(0x8058, opSayScrollUp);
    interpreterRegisterOpcode(0x8059, opSayScrollDown);
    interpreterRegisterOpcode(0x805A, opSaySetSpacing);
    interpreterRegisterOpcode(0x805B, opSayOptionColor);
    interpreterRegisterOpcode(0x805C, opSayReplyColor);
    interpreterRegisterOpcode(0x805D, opSayRestart);
    interpreterRegisterOpcode(0x805E, opSayGetLastPos);
    interpreterRegisterOpcode(0x8061, opSayMessageTimeout);
    interpreterRegisterOpcode(0x8071, opGotoXY);
    interpreterRegisterOpcode(0x808D, opHideMouse);
    interpreterRegisterOpcode(0x808E, opShowMouse);
    interpreterRegisterOpcode(0x8090, opRefreshMouse);
    interpreterRegisterOpcode(0x808F, opMouseShape);
    interpreterRegisterOpcode(0x8091, opSetGlobalMouseFunc);
    interpreterRegisterOpcode(0x806E, opLoadPaletteTable);
    interpreterRegisterOpcode(0x8092, opAddNamedEvent);
    interpreterRegisterOpcode(0x8093, opAddNamedHandler);
    interpreterRegisterOpcode(0x8094, opClearNamed);
    interpreterRegisterOpcode(0x8095, opSignalNamed);
    interpreterRegisterOpcode(0x8096, opAddKey);
    interpreterRegisterOpcode(0x8097, opDeleteKey);
    interpreterRegisterOpcode(0x8098, opSoundPlay);
    interpreterRegisterOpcode(0x8099, opSoundPause);
    interpreterRegisterOpcode(0x809A, opSoundResume);
    interpreterRegisterOpcode(0x809B, opSoundStop);
    interpreterRegisterOpcode(0x809C, opSoundRewind);
    interpreterRegisterOpcode(0x809D, opSoundDelete);
    interpreterRegisterOpcode(0x809E, opSetOneOptPause);
    interpreterRegisterOpcode(0x809F, opSelectFileList);
    interpreterRegisterOpcode(0x80A0, opTokenize);

    _nevs_initonce();
    _initIntExtra();
    dialogInit();
}

// 0x466F6C
void intLibRegisterProgramDeleteCallback(IntLibProgramDeleteCallback* callback)
{
    int index;
    for (index = 0; index < gIntLibProgramDeleteCallbacksLength; index++) {
        if (gIntLibProgramDeleteCallbacks[index] == NULL) {
            break;
        }
    }

    if (index == gIntLibProgramDeleteCallbacksLength) {
        if (gIntLibProgramDeleteCallbacks != NULL) {
            gIntLibProgramDeleteCallbacks = (IntLibProgramDeleteCallback**)internal_realloc_safe(gIntLibProgramDeleteCallbacks, sizeof(*gIntLibProgramDeleteCallbacks) * (gIntLibProgramDeleteCallbacksLength + 1), __FILE__, __LINE__); // ..\\int\\INTLIB.C, 2110
        } else {
            gIntLibProgramDeleteCallbacks = (IntLibProgramDeleteCallback**)internal_malloc_safe(sizeof(*gIntLibProgramDeleteCallbacks), __FILE__, __LINE__); // ..\\int\\INTLIB.C, 2112
        }
        gIntLibProgramDeleteCallbacksLength++;
    }

    gIntLibProgramDeleteCallbacks[index] = callback;
}

// 0x467040
void intLibRemoveProgramReferences(Program* program)
{
    for (int index = 0; index < INT_LIB_KEY_HANDLERS_CAPACITY; index++) {
        if (program == gIntLibKeyHandlerEntries[index].program) {
            gIntLibKeyHandlerEntries[index].program = NULL;
        }
    }

    intExtraRemoveProgramReferences(program);

    for (int index = 0; index < gIntLibProgramDeleteCallbacksLength; index++) {
        IntLibProgramDeleteCallback* callback = gIntLibProgramDeleteCallbacks[index];
        if (callback != NULL) {
            callback(program);
        }
    }
}
