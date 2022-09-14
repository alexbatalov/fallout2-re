#ifndef INTERPRETER_LIB_H
#define INTERPRETER_LIB_H

#include <stdbool.h>

#include "interpreter.h"
#include "sound.h"

#define INT_LIB_SOUNDS_CAPACITY 32
#define INT_LIB_KEY_HANDLERS_CAPACITY 256

typedef struct IntLibKeyHandlerEntry {
    Program* program;
    int proc;
} IntLibKeyHandlerEntry;

typedef void(IntLibProgramDeleteCallback)(Program*);

extern Sound* gIntLibSounds[INT_LIB_SOUNDS_CAPACITY];
extern unsigned char gIntLibFadePalette[256 * 3];
extern IntLibKeyHandlerEntry gIntLibKeyHandlerEntries[INT_LIB_KEY_HANDLERS_CAPACITY];
extern bool gIntLibIsPaletteFaded;
extern int gIntLibGenericKeyHandlerProc;
extern int gIntLibProgramDeleteCallbacksLength;
extern Program* gIntLibGenericKeyHandlerProgram;
extern IntLibProgramDeleteCallback** gIntLibProgramDeleteCallbacks;
extern int gIntLibSayStartingPosition;
extern char gIntLibPlayMovieFileName[100];
extern char gIntLibPlayMovieRectFileName[100];

void opFillWin3x3(Program* program);
void opFormat(Program* program);
void opPrint(Program* program);
void opSelectFileList(Program* program);
void opTokenize(Program* program);
void opPrintRect(Program* program);
void opSelect(Program* program);
void opDisplay(Program* program);
void opDisplayRaw(Program* program);
void _interpretFadePaletteBK(unsigned char* oldPalette, unsigned char* newPalette, int a3, float duration, int shouldProcessBk);
void interpretFadePalette(unsigned char* oldPalette, unsigned char* newPalette, int a3, float duration);
int intlibGetFadeIn();
void interpretFadeOut(float duration);
void interpretFadeIn(float duration);
void interpretFadeOutNoBK(float duration);
void interpretFadeInNoBK(float duration);
void opFadeIn(Program* program);
void opFadeOut(Program* program);
int intLibCheckMovie(Program* program);
void opSetMovieFlags(Program* program);
void opPlayMovie(Program* program);
void opStopMovie(Program* program);
void opAddRegionProc(Program* program);
void opAddRegionRightProc(Program* program);
void opCreateWin(Program* program);
void opResizeWin(Program* program);
void opScaleWin(Program* program);
void opDeleteWin(Program* program);
void opSayStart(Program* program);
void opDeleteRegion(Program* program);
void opActivateRegion(Program* program);
void opCheckRegion(Program* program);
void opAddRegion(Program* program);
void opSayStartPos(Program* program);
void opSayReplyTitle(Program* program);
void opSayGoToReply(Program* program);
void opSayReply(Program* program);
void opSayOption(Program* program);
int intLibCheckDialog(Program* program);
void opSayEnd(Program* program);
void opSayGetLastPos(Program* program);
void opSayQuit(Program* program);
int getTimeOut();
void setTimeOut(int value);
void opSayMessageTimeout(Program* program);
void opSayMessage(Program* program);
void opGotoXY(Program* program);
void opAddButtonFlag(Program* program);
void opAddRegionFlag(Program* program);
void opAddButton(Program* program);
void opAddButtonText(Program* program);
void opAddButtonGfx(Program* program);
void opAddButtonProc(Program* program);
void opAddButtonRightProc(Program* program);
void opShowWin(Program* program);
void opDeleteButton(Program* program);
void opFillWin(Program* program);
void opFillRect(Program* program);
void opHideMouse(Program* program);
void opShowMouse(Program* program);
void opMouseShape(Program* program);
void opSetGlobalMouseFunc(Program* Program);
void opDisplayGfx(Program* program);
void opLoadPaletteTable(Program* program);
void opAddNamedEvent(Program* program);
void opAddNamedHandler(Program* program);
void opClearNamed(Program* program);
void opSignalNamed(Program* program);
void opAddKey(Program* program);
void opDeleteKey(Program* program);
void opRefreshMouse(Program* program);
void opSetFont(Program* program);
void opSetTextFlags(Program* program);
void opSetTextColor(Program* program);
void opSayOptionColor(Program* program);
void opSayReplyColor(Program* program);
void opSetHighlightColor(Program* program);
void opSayReplyWindow(Program* program);
void opSayReplyFlags(Program* program);
void opSayOptionFlags(Program* program);
void opSayOptionWindow(Program* program);
void opSayBorder(Program* program);
void opSayScrollUp(Program* program);
void opSayScrollDown(Program* program);
void opSaySetSpacing(Program* program);
void opSayRestart(Program* program);
void intLibSoundCallback(void* userData, int a2);
int intLibSoundDelete(int value);
int intLibSoundPlay(char* fileName, int mode);
int intLibSoundPause(int value);
int intLibSoundRewind(int value);
int intLibSoundResume(int value);
void opSoundPlay(Program* program);
void opSoundPause(Program* program);
void opSoundResume(Program* program);
void opSoundStop(Program* program);
void opSoundRewind(Program* program);
void opSoundDelete(Program* program);
void opSetOneOptPause(Program* program);
void intLibUpdate();
void intLibExit();
bool intLibDoInput(int key);
void intLibInit();
void intLibRegisterProgramDeleteCallback(IntLibProgramDeleteCallback* callback);
void intLibRemoveProgramReferences(Program* program);

#endif /* INTERPRETER_LIB_H */
