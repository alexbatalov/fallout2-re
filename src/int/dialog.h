#ifndef FALLOUT_INT_DIALOG_H_
#define FALLOUT_INT_DIALOG_H_

#include "interpreter.h"

typedef void DialogFunc1(int win);
typedef void DialogFunc2(int win);

extern DialogFunc1* replyWinDrawCallback;
extern DialogFunc2* optionsWinDrawCallback;

int dialogStart(Program* a1);
int dialogRestart();
int dialogGotoReply(const char* a1);
int dialogTitle(const char* a1);
int dialogReply(const char* a1, const char* a2);
int dialogOption(const char* a1, const char* a2);
int dialogOptionProc(const char* a1, int a2);
int dialogMessage(const char* a1, const char* a2, int timeout);
int dialogGo(int a1);
int dialogGetExitPoint();
int dialogQuit();
int dialogSetOptionWindow(int x, int y, int width, int height, char* backgroundFileName);
int dialogSetReplyWindow(int x, int y, int width, int height, char* backgroundFileName);
int dialogSetBorder(int a1, int a2);
int dialogSetScrollUp(int a1, int a2, char* a3, char* a4, char* a5, char* a6, int a7);
int dialogSetScrollDown(int a1, int a2, char* a3, char* a4, char* a5, char* a6, int a7);
int dialogSetSpacing(int value);
int dialogSetOptionColor(float a1, float a2, float a3);
int dialogSetReplyColor(float a1, float a2, float a3);
int dialogSetOptionFlags(short flags);
int dialogSetReplyFlags(short flags);
void initDialog();
void dialogClose();
int dialogGetDialogDepth();
void dialogRegisterWinDrawCallbacks(DialogFunc1* a1, DialogFunc2* a2);
int dialogToggleMediaFlag(int a1);
int dialogGetMediaFlag();

#endif /* FALLOUT_INT_DIALOG_H_ */
