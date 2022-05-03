#ifndef INTERPRETER_LIB_H
#define INTERPRETER_LIB_H

#include "interpreter.h"
#include "sound.h"

#define INTERPRETER_SOUNDS_LENGTH (32)
#define INTERPRETER_KEY_HANDLER_ENTRIES_LENGTH (256)

typedef struct InterpreterKeyHandlerEntry {
    Program* program;
    int proc;
} InterpreterKeyHandlerEntry;

typedef void (*OFF_59E160)(Program*);

extern Sound* gInterpreterSounds[INTERPRETER_SOUNDS_LENGTH];
extern InterpreterKeyHandlerEntry gInterpreterKeyHandlerEntries[INTERPRETER_KEY_HANDLER_ENTRIES_LENGTH];
extern int gIntepreterAnyKeyHandlerProc;
extern int dword_59E158;
extern Program* gInterpreterAnyKeyHandlerProgram;
extern OFF_59E160* off_59E160;
extern int dword_59E164;

void opSayStart(Program* program);
void opSayStartPos(Program* program);
void opSayReplyTitle(Program* program);
void opSayGoToReply(Program* program);
void opSayGetLastPos(Program* program);
void opSayQuit(Program* program);
void opSayMessageTimeout(Program* program);
void opHideMouse(Program* program);
void opShowMouse(Program* program);
void opClearNamed(Program* program);
void opSignalNamed(Program* program);
void opAddKey(Program* program);
void opDeleteKey(Program* program);
void opSetFont(Program* program);
void opSetTextFlags(Program* program);
void opSetTextColor(Program* program);
void opSayOptionColor(Program* program);
void opSayReplyColor(Program* program);
void opSetHighlightColor(Program* program);
void opSayReplyFlags(Program* program);
void opSayOptionFlags(Program* program);
void opSayBorder(Program* program);
void opSaySetSpacing(Program* program);
void opSayRestart(Program* program);
void interpreterSoundCallback(void* userData, int a2);
int interpreterSoundDelete(int a1);
int interpreterSoundPlay(char* fileName, int mode);
int interpreterSoundPause(int value);
int interpreterSoundRewind(int value);
int interpreterSoundResume(int value);
void opSoundPlay(Program* program);
void opSoundPause(Program* program);
void opSoundResume(Program* program);
void opSoundStop(Program* program);
void opSoundRewind(Program* program);
void opSoundDelete(Program* program);
void opSetOneOptPause(Program* program);
void sub_466994();
void sub_4669A0();
bool sub_466A04(int key);
void sub_466A70();
void sub_466F6C(OFF_59E160 fn);
void sub_467040(Program* program);

#endif /* INTERPRETER_LIB_H */
