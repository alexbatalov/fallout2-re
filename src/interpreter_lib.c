#include "interpreter_lib.h"

#include "color.h"
#include "core.h"
#include "datafile.h"
#include "debug.h"
#include "dialog.h"
#include "interpreter_extra.h"
#include "memory_manager.h"
#include "nevs.h"
#include "select_file_list.h"
#include "text_font.h"
#include "widget.h"
#include "window_manager_private.h"

// 0x59D5D0
Sound* gInterpreterSounds[INTERPRETER_SOUNDS_LENGTH];

// 0x59D650
unsigned char stru_59D650[256 * 3];

// 0x59D950
InterpreterKeyHandlerEntry gInterpreterKeyHandlerEntries[INTERPRETER_KEY_HANDLER_ENTRIES_LENGTH];

// 0x59E150
int dword_59E150;

// 0x59E154
int gIntepreterAnyKeyHandlerProc;

// Number of entries in _callbacks.
//
// 0x59E158
int _numCallbacks;

// 0x59E15C
Program* gInterpreterAnyKeyHandlerProgram;

// 0x59E160
OFF_59E160* _callbacks;

// 0x59E164
int _sayStartingPosition;

// 0x59E168
char byte_59E168[100];

// 0x59E1CC
char byte_59E1CC[100];

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
    unsigned char* imageData = sub_42EFCC(mangledFileName, &imageWidth, &imageHeight);
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
    sub_4B8C68(mangledFileName);
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
    sub_4B8CA8(mangledFileName);
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

    _setSystemPalette(stru_59D650);

    sub_46222C(stru_59D650, _cmap, 64, (float)data, 1);
    dword_59E150 = 1;

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

    sub_46222C(_getSystemPalette(), stru_59D650, 64, (float)data, 1);

    if (!cursorWasHidden) {
        mouseShowCursor();
    }

    dword_59E150 = 0;

    program->flags &= ~PROGRAM_FLAG_0x20;
}

// 0x462570
int _checkMovie(Program* program)
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

    strcpy(byte_59E168, programGetString(program, opcode, data));

    if (strrchr(byte_59E168, '.') == NULL) {
        strcat(byte_59E168, ".mve");
    }

    _selectWindowID(program->windowId);

    program->flags |= PROGRAM_FLAG_0x10;
    program->field_7C = _checkMovie;

    char* mangledFileName = _interpretMangleName(byte_59E168);
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

    strcpy(byte_59E1CC, programGetString(program, opcode[4], data[4]));

    if (strrchr(byte_59E1CC, '.') == NULL) {
        strcat(byte_59E1CC, ".mve");
    }

    _selectWindowID(program->windowId);

    program->field_7C = _checkMovie;
    program->flags |= PROGRAM_FLAG_0x10;

    char* mangledFileName = _interpretMangleName(byte_59E1CC);
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
    _sayStartingPosition = 0;

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

    _sayStartingPosition = data;

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

// setglobalmousefunc
// 0x4649C4
void opSetGlobalMouseFunc(Program* Program)
{
    programFatalError("setglobalmousefunc not defined");
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
        gIntepreterAnyKeyHandlerProc = proc;
        gInterpreterAnyKeyHandlerProgram = program;
    } else {
        if (key > INTERPRETER_KEY_HANDLER_ENTRIES_LENGTH - 1) {
            programFatalError("Key out of range");
        }

        gInterpreterKeyHandlerEntries[key].program = program;
        gInterpreterKeyHandlerEntries[key].proc = proc;
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
        gIntepreterAnyKeyHandlerProc = 0;
        gInterpreterAnyKeyHandlerProgram = NULL;
    } else {
        if (key > INTERPRETER_KEY_HANDLER_ENTRIES_LENGTH - 1) {
            programFatalError("Key out of range");
        }

        gInterpreterKeyHandlerEntries[key].program = NULL;
        gInterpreterKeyHandlerEntries[key].proc = 0;
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
void interpreterSoundCallback(void* userData, int a2)
{
    if (a2 == 1) {
        Sound** sound = (Sound**)userData;
        *sound = NULL;
    }
}

// 0x466070
int interpreterSoundDelete(int value)
{
    if (value == -1) {
        return 1;
    }

    if ((value & 0xA0000000) == 0) {
        return 0;
    }

    int index = value & ~0xA0000000;
    Sound* sound = gInterpreterSounds[index];
    if (sound == NULL) {
        return 0;
    }

    if (soundIsPlaying(sound)) {
        soundStop(sound);
    }

    soundDelete(sound);

    gInterpreterSounds[index] = NULL;

    return 1;
}

// 0x466110
int interpreterSoundPlay(char* fileName, int mode)
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
    for (index = 0; index < INTERPRETER_SOUNDS_LENGTH; index++) {
        if (gInterpreterSounds[index] == NULL) {
            break;
        }
    }

    if (index == INTERPRETER_SOUNDS_LENGTH) {
        return -1;
    }

    Sound* sound = gInterpreterSounds[index] = soundAllocate(v3, v5);
    if (sound == NULL) {
        return -1;
    }

    soundSetCallback(sound, interpreterSoundCallback, &(gInterpreterSounds[index]));

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
    gInterpreterSounds[index] = NULL;
    return -1;
}

// 0x46655C
int interpreterSoundPause(int value)
{
    if (value == -1) {
        return 1;
    }

    if ((value & 0xA0000000) == 0) {
        return 0;
    }

    int index = value & ~0xA0000000;
    Sound* sound = gInterpreterSounds[index];
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
int interpreterSoundRewind(int value)
{
    if (value == -1) {
        return 1;
    }

    if ((value & 0xA0000000) == 0) {
        return 0;
    }

    int index = value & ~0xA0000000;
    Sound* sound = gInterpreterSounds[index];
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
int interpreterSoundResume(int value)
{
    if (value == -1) {
        return 1;
    }

    if ((value & 0xA0000000) == 0) {
        return 0;
    }

    int index = value & ~0xA0000000;
    Sound* sound = gInterpreterSounds[index];
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
    // TODO: Incomplete.
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

    interpreterSoundPause(data);
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

    interpreterSoundResume(data);
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

    interpreterSoundPause(data);
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

    interpreterSoundRewind(data);
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

    interpreterSoundDelete(data);
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
void _updateIntLib()
{
    _nevs_update();
    _intExtraRemoveProgramReferences_();
}

// 0x4669A0
void _intlibClose()
{
    _dialogClose();
    _intExtraClose_();

    for (int index = 0; index < INTERPRETER_SOUNDS_LENGTH; index++) {
        if (gInterpreterSounds[index] != NULL) {
            interpreterSoundDelete(index | 0xA0000000);
        }
    }

    _nevs_close();

    if (_callbacks != NULL) {
        internal_free_safe(_callbacks, __FILE__, __LINE__); // "..\\int\\INTLIB.C", 1976
        _callbacks = NULL;
        _numCallbacks = 0;
    }
}

// 0x466A04
bool _intLibDoInput(int key)
{
    if (key < 0 || key >= INTERPRETER_KEY_HANDLER_ENTRIES_LENGTH) {
        return false;
    }

    if (gInterpreterAnyKeyHandlerProgram != NULL) {
        if (gIntepreterAnyKeyHandlerProc != 0) {
            _executeProc(gInterpreterAnyKeyHandlerProgram, gIntepreterAnyKeyHandlerProc);
        }
        return true;
    }

    InterpreterKeyHandlerEntry* entry = &(gInterpreterKeyHandlerEntries[key]);
    if (entry->program == NULL) {
        return false;
    }

    if (entry->proc != 0) {
        _executeProc(entry->program, entry->proc);
    }

    return true;
}

// 0x466A70
void _initIntlib()
{
    // TODO: Incomplete.
    _nevs_initonce();
    _initIntExtra();
}

// 0x466F6C
void _interpretRegisterProgramDeleteCallback(OFF_59E160 fn)
{
    int index;
    for (index = 0; index < _numCallbacks; index++) {
        if (_callbacks[index] == NULL) {
            break;
        }
    }

    if (index == _numCallbacks) {
        if (_callbacks != NULL) {
            _callbacks = (OFF_59E160*)internal_realloc_safe(_callbacks, sizeof(*_callbacks) * (_numCallbacks + 1), __FILE__, __LINE__); // ..\\int\\INTLIB.C, 2110
        } else {
            _callbacks = (OFF_59E160*)internal_malloc_safe(sizeof(*_callbacks), __FILE__, __LINE__); // ..\\int\\INTLIB.C, 2112
        }
        _numCallbacks++;
    }

    _callbacks[index] = fn;
}

// 0x467040
void _removeProgramReferences_(Program* program)
{
    for (int index = 0; index < INTERPRETER_KEY_HANDLER_ENTRIES_LENGTH; index++) {
        if (program == gInterpreterKeyHandlerEntries[index].program) {
            gInterpreterKeyHandlerEntries[index].program = NULL;
        }
    }

    _intExtraRemoveProgramReferences_();

    for (int index = 0; index < _numCallbacks; index++) {
        OFF_59E160 fn = _callbacks[index];
        if (fn != NULL) {
            fn(program);
        }
    }
}
