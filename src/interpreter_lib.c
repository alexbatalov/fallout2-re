#include "interpreter_lib.h"

#include "core.h"
#include "debug.h"
#include "dialog.h"
#include "interpreter_extra.h"
#include "memory_manager.h"
#include "nevs.h"
#include "widget.h"

// 0x59D5D0
Sound* gInterpreterSounds[INTERPRETER_SOUNDS_LENGTH];

// 0x59D950
InterpreterKeyHandlerEntry gInterpreterKeyHandlerEntries[INTERPRETER_KEY_HANDLER_ENTRIES_LENGTH];

// 0x59E154
int gIntepreterAnyKeyHandlerProc;

// Number of entries in off_59E160.
//
// 0x59E158
int dword_59E158;

// 0x59E15C
Program* gInterpreterAnyKeyHandlerProgram;

// 0x59E160
OFF_59E160* off_59E160;

// 0x59E164
int dword_59E164;

// movieflags
// 0x462584
void opSetMovieFlags(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode, data);
    }

    if (!sub_4BB23C(data)) {
        programFatalError("Error setting movie flags\n");
    }
}

// saystart
// 0x4633E4
void opSayStart(Program* program)
{
    dword_59E164 = 0;

    program->flags |= PROGRAM_FLAG_0x20;
    int rc = sub_430D40(program);
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

    dword_59E164 = data;

    program->flags |= PROGRAM_FLAG_0x20;
    int rc = sub_430D40(program);
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
    if ((opcode & 0xF7FF) == VALUE_TYPE_STRING) {
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
    if ((opcode & 0xF7FF) == VALUE_TYPE_STRING) {
        string = programGetString(program, opcode, data);
    }

    if (sub_430DE4(string) != 0) {
        programFatalError("Error during goto, couldn't find reply target %s", string);
    }
}

// saygetlastpos
// 0x4637EC
void opSayGetLastPos(Program* program)
{
    int value = sub_431184();
    programStackPushInt32(program, value);
    programStackPushInt16(program, VALUE_TYPE_INT);
}

// sayquit
// 0x463810
void opSayQuit(Program* program)
{
    if (sub_431198() != 0) {
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
    if ((opcode & 0xF7FF) == 0x4000) {
        programFatalError("sayMsgTimeout:  invalid var type passed.");
    }

    dword_519038 = data;
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

    if ((opcode[0] & 0xF7FF) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 2 given to addregionflag");
    }

    if ((opcode[1] & 0xF7FF) != VALUE_TYPE_STRING) {
        programFatalError("Invalid arg 1 given to addregionflag");
    }

    const char* regionName = programGetString(program, opcode[1], data[1]);
    if (!sub_4BAF2C(regionName, data[0])) {
        // NOTE: Original code calls programGetString one more time with the
        // same params.
        programFatalError("Error setting flag on region %s", regionName);
    }
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

// clearnamed
// 0x464C80
void opClearNamed(Program* program)
{
    opcode_t opcode = programStackPopInt16(program);
    int data = programStackPopInt32(program);

    if (opcode == VALUE_TYPE_DYNAMIC_STRING) {
        programPopString(program, opcode, data);
    }

    if ((opcode & 0xF7FF) != VALUE_TYPE_STRING) {
        programFatalError("Invalid type given to clearnamed");
    }

    char* string = programGetString(program, opcode, data);
    sub_48859C(string);
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

    if ((opcode & 0xF7FF) != VALUE_TYPE_STRING) {
        programFatalError("Invalid type given to signalnamed");
    }

    char* str = programGetString(program, opcode, data);
    sub_48862C(str);
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
        if ((opcode[arg] & 0xF7FF) != VALUE_TYPE_INT) {
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

    if ((opcode & 0xF7FF) != VALUE_TYPE_INT) {
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

    if ((opcode & 0xF7FF) != VALUE_TYPE_INT) {
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

    if ((opcode & 0xF7FF) != VALUE_TYPE_INT) {
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
        if (((opcode[arg] & 0xF7FF) != VALUE_TYPE_FLOAT && (opcode[arg] & 0xF7FF) != VALUE_TYPE_INT)
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
        if (((opcode[arg] & 0xF7FF) != VALUE_TYPE_FLOAT && (opcode[arg] & 0xF7FF) != VALUE_TYPE_INT)
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
        if (((opcode[arg] & 0xF7FF) != VALUE_TYPE_FLOAT && (opcode[arg] & 0xF7FF) != VALUE_TYPE_INT)
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
        if (((opcode[arg] & 0xF7FF) != VALUE_TYPE_FLOAT && (opcode[arg] & 0xF7FF) != VALUE_TYPE_INT)
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

    if ((opcode & 0xF7FF) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 1 given to sayreplyflags");
    }

    if (!sub_431420(data)) {
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

    if ((opcode & 0xF7FF) != VALUE_TYPE_INT) {
        programFatalError("Invalid arg 1 given to sayoptionflags");
    }

    if (!sub_431420(data)) {
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
        if ((opcode[arg] & 0xF7FF) != VALUE_TYPE_INT) {
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

    if ((opcode & 0xF7FF) != VALUE_TYPE_INT) {
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
    if (sub_430DB8() != 0) {
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
    if (sub_4ADBC4(sound, 0x01)) {
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
    if (sub_4ADBC4(sound, 0x01)) {
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

    if ((opcode & 0xF7FF) != VALUE_TYPE_INT) {
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

    if ((opcode & 0xF7FF) != VALUE_TYPE_INT) {
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

    if ((opcode & 0xF7FF) != VALUE_TYPE_INT) {
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

    if ((opcode & 0xF7FF) != VALUE_TYPE_INT) {
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

    if ((opcode & 0xF7FF) != VALUE_TYPE_INT) {
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

    if ((opcode & 0xF7FF) != VALUE_TYPE_INT) {
        programFatalError("SetOneOptPause: invalid arg passed (non-integer).");
    }

    if (data) {
        if ((sub_431554() & 8) == 0) {
            return;
        }
    } else {
        if ((sub_431554() & 8) != 0) {
            return;
        }
    }

    sub_431530(8);
}

// 0x466994
void sub_466994()
{
    sub_4886AC();
    sub_45D878();
}

// 0x4669A0
void sub_4669A0()
{
    sub_431434();
    sub_45CDD4();

    for (int index = 0; index < INTERPRETER_SOUNDS_LENGTH; index++) {
        if (gInterpreterSounds[index] != NULL) {
            interpreterSoundDelete(index | 0xA0000000);
        }
    }

    sub_4883AC();

    if (off_59E160 != NULL) {
        internal_free_safe(off_59E160, __FILE__, __LINE__); // "..\\int\\INTLIB.C", 1976
        off_59E160 = NULL;
        dword_59E158 = 0;
    }
}

// 0x466A04
bool sub_466A04(int key)
{
    if (key < 0 || key >= INTERPRETER_KEY_HANDLER_ENTRIES_LENGTH) {
        return false;
    }

    if (gInterpreterAnyKeyHandlerProgram != NULL) {
        if (gIntepreterAnyKeyHandlerProc != 0) {
            sub_46DB58(gInterpreterAnyKeyHandlerProgram, gIntepreterAnyKeyHandlerProc);
        }
        return true;
    }

    InterpreterKeyHandlerEntry* entry = &(gInterpreterKeyHandlerEntries[key]);
    if (entry->program == NULL) {
        return false;
    }

    if (entry->proc != 0) {
        sub_46DB58(entry->program, entry->proc);
    }

    return true;
}

// 0x466A70
void sub_466A70()
{
    // TODO: Incomplete.
    sub_488418();
    sub_45CDD8();
}

// 0x466F6C
void sub_466F6C(OFF_59E160 fn)
{
    int index;
    for (index = 0; index < dword_59E158; index++) {
        if (off_59E160[index] == NULL) {
            break;
        }
    }

    if (index == dword_59E158) {
        if (off_59E160 != NULL) {
            off_59E160 = internal_realloc_safe(off_59E160, sizeof(*off_59E160) * (dword_59E158 + 1), __FILE__, __LINE__); // ..\\int\\INTLIB.C, 2110
        } else {
            off_59E160 = internal_malloc_safe(sizeof(*off_59E160), __FILE__, __LINE__); // ..\\int\\INTLIB.C, 2112
        }
        dword_59E158++;
    }

    off_59E160[index] = fn;
}

// 0x467040
void sub_467040(Program* program)
{
    for (int index = 0; index < INTERPRETER_KEY_HANDLER_ENTRIES_LENGTH; index++) {
        if (program == gInterpreterKeyHandlerEntries[index].program) {
            gInterpreterKeyHandlerEntries[index].program = NULL;
        }
    }

    sub_45D878();

    for (int index = 0; index < dword_59E158; index++) {
        OFF_59E160 fn = off_59E160[index];
        if (fn != NULL) {
            fn(program);
        }
    }
}
