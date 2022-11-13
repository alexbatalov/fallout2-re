#ifndef FALLOUT_GAME_GDIALOG_H_
#define FALLOUT_GAME_GDIALOG_H_

#include <stdbool.h>

#include "game/art.h"
#include "interpreter.h"
#include "obj_types.h"

extern unsigned char* light_BlendTable;
extern unsigned char* dark_BlendTable;
extern Object* dialog_target;
extern bool dialog_target_is_party;
extern int dialogue_head;
extern int dialogue_scr_id;

extern unsigned char light_GrayTable[256];
extern unsigned char dark_GrayTable[256];

int gdialogInit();
int gdialogReset();
int gdialogExit();
bool gdialogActive();
void gdialogEnter(Object* a1, int a2);
void gdialogSystemEnter();
void gdialogSetupSpeech(const char* a1);
void gdialogFreeSpeech();
int gdialogEnableBK();
int gdialogDisableBK();
int gdialogInitFromScript(int headFid, int reaction);
int gdialogExitFromScript();
void gdialogSetBackground(int a1);
void gdialogDisplayMsg(char* msg);
int gdialogStart();
int gdialogSayMessage();
int gdialogOption(int messageListId, int messageId, const char* a3, int reaction);
int gdialogOptionStr(int messageListId, const char* text, const char* a3, int reaction);
int gdialogOptionProc(int messageListId, int messageId, int proc, int reaction);
int gdialogOptionProcStr(int messageListId, const char* text, int proc, int reaction);
int gdialogReply(Program* a1, int a2, int a3);
int gdialogReplyStr(Program* a1, int a2, const char* a3);
int gdialogGo();
void gdialogUpdatePartyStatus();
void talk_to_critter_reacts(int a1);
void gdialogSetBarterMod(int modifier);
int gdActivateBarter(int modifier);
void barter_end_to_talk_to();

#endif /* FALLOUT_GAME_GDIALOG_H_ */
