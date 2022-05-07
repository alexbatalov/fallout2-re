#ifndef GAME_DIALOG_H
#define GAME_DIALOG_H

#include "art.h"
#include "geometry.h"
#include "interpreter.h"
#include "lips.h"
#include "message.h"
#include "obj_types.h"

#include <stdbool.h>

#define DIALOG_REVIEW_ENTRIES_CAPACITY 80

#define DIALOG_OPTION_ENTRIES_CAPACITY 30

typedef enum GameDialogReviewWindowButton {
    GAME_DIALOG_REVIEW_WINDOW_BUTTON_SCROLL_UP,
    GAME_DIALOG_REVIEW_WINDOW_BUTTON_SCROLL_DOWN,
    GAME_DIALOG_REVIEW_WINDOW_BUTTON_DONE,
    GAME_DIALOG_REVIEW_WINDOW_BUTTON_COUNT,
} GameDialogReviewWindowButton;

typedef enum GameDialogReviewWindowButtonFrm {
    GAME_DIALOG_REVIEW_WINDOW_BUTTON_FRM_ARROW_UP_NORMAL,
    GAME_DIALOG_REVIEW_WINDOW_BUTTON_FRM_ARROW_UP_PRESSED,
    GAME_DIALOG_REVIEW_WINDOW_BUTTON_FRM_ARROW_DOWN_NORMAL,
    GAME_DIALOG_REVIEW_WINDOW_BUTTON_FRM_ARROW_DOWN_PRESSED,
    GAME_DIALOG_REVIEW_WINDOW_BUTTON_FRM_DONE_NORMAL,
    GAME_DIALOG_REVIEW_WINDOW_BUTTON_FRM_DONE_PRESSED,
    GAME_DIALOG_REVIEW_WINDOW_BUTTON_FRM_COUNT,
} GameDialogReviewWindowButtonFrm;

typedef enum GameDialogReaction {
    GAME_DIALOG_REACTION_GOOD = 49,
    GAME_DIALOG_REACTION_NEUTRAL = 50,
    GAME_DIALOG_REACTION_BAD = 51,
} GameDialogReaction;

typedef struct GameDialogReviewEntry {
    int replyMessageListId;
    int replyMessageId;
    // Can be NULL.
    char* replyText;
    int optionMessageListId;
    int optionMessageId;
    char* optionText;
} GameDialogReviewEntry;

typedef struct GameDialogOptionEntry {
    int messageListId;
    int messageId;
    int reaction;
    int proc;
    int btn;
    int field_14;
    char text[900];
    int field_39C;
} GameDialogOptionEntry;

// Provides button configuration for party member combat control and
// customization interface.
typedef struct GameDialogButtonData {
    int x;
    int y;
    int upFrmId;
    int downFrmId;
    int disabledFrmId;
    CacheEntry* upFrmHandle;
    CacheEntry* downFrmHandle;
    CacheEntry* disabledFrmHandle;
    int keyCode;
    int value;
} GameDialogButtonData;

typedef struct STRUCT_5189E4 {
    int messageId;
    int value;
} STRUCT_5189E4;

typedef enum PartyMemberCustomizationOption {
    PARTY_MEMBER_CUSTOMIZATION_OPTION_AREA_ATTACK_MODE,
    PARTY_MEMBER_CUSTOMIZATION_OPTION_RUN_AWAY_MODE,
    PARTY_MEMBER_CUSTOMIZATION_OPTION_BEST_WEAPON,
    PARTY_MEMBER_CUSTOMIZATION_OPTION_DISTANCE,
    PARTY_MEMBER_CUSTOMIZATION_OPTION_ATTACK_WHO,
    PARTY_MEMBER_CUSTOMIZATION_OPTION_CHEM_USE,
    PARTY_MEMBER_CUSTOMIZATION_OPTION_COUNT,
} PartyMemberCustomizationOption;

extern int dword_444D10[3];

extern int dword_5186D4;
extern int gGameDialogOptionEntriesLength;
extern int gGameDialogReviewEntriesLength;
extern unsigned char* gGameDialogDisplayBuffer;
extern int gGameDialogReplyWindow;
extern int gGameDialogOptionsWindow;
extern bool dword_5186EC;
extern int dword_5186F0;
extern int gGameDialogFidgetFid;
extern CacheEntry* gGameDialogFidgetFrmHandle;
extern Art* gGameDialogFidgetFrm;
extern int gGameDialogBackground;
extern int dword_518704;
extern int dword_518704;
extern Art* off_51870C;
extern bool gGameDialogLipSyncStarted;
extern int dword_518714;
extern int dword_518718;
extern int dword_51871C;
extern bool dword_518720;
extern bool dword_518724;
extern int dword_518728;
extern bool dword_51872C;
extern Object* off_518730;
extern Object* off_518734;
extern Object* off_518738;
extern int gGameDialogBarterModifier;
extern int gGameDialogBackgroundWindow;
extern int gGameDialogWindow;
extern Rect stru_518748[8];
extern int dword_5187C8;
extern bool dword_5187CC;
extern int dword_5187D0;
extern int dword_5187D4;
extern int gGameDialogOldMusicVolume;
extern int gGameDialogOldCenterTile;
extern int gGameDialogOldDudeTile;
extern unsigned char* off_5187E4;
extern unsigned char* off_5187E8;
extern int dword_5187EC;
extern int dword_5187F0;
extern CacheEntry* gGameDialogReviewWindowButtonFrmHandles[GAME_DIALOG_REVIEW_WINDOW_BUTTON_FRM_COUNT];
extern CacheEntry* off_51880C;
extern CacheEntry* gGameDialogReviewWindowBackgroundFrmHandle;
extern unsigned char* gGameDialogReviewWindowBackgroundFrmData;
extern const int gGameDialogReviewWindowButtonWidths[GAME_DIALOG_REVIEW_WINDOW_BUTTON_COUNT];
extern const int gGameDialogReviewWindowButtonHeights[GAME_DIALOG_REVIEW_WINDOW_BUTTON_COUNT];
extern int gGameDialogReviewWindowButtonFrmIds[GAME_DIALOG_REVIEW_WINDOW_BUTTON_FRM_COUNT];
extern Object* gGameDialogSpeaker;
extern bool gGameDialogSpeakerIsPartyMember;
extern int gGameDialogHeadFid;
extern int gGameDialogSid;
extern int dword_518858[PHONEME_COUNT];
extern int dword_518900;
extern int dword_518904;
extern unsigned int dword_518908;
extern const char* off_51890C[3];
extern int dword_518918;
extern GameDialogButtonData gGameDialogDispositionButtonsData[5];
extern STRUCT_5189E4 stru_5189E4[PARTY_MEMBER_CUSTOMIZATION_OPTION_COUNT][6];
extern GameDialogButtonData stru_518B04[PARTY_MEMBER_CUSTOMIZATION_OPTION_COUNT];
extern int dword_518BF4;

extern int dword_58EA80[PARTY_MEMBER_CUSTOMIZATION_OPTION_COUNT];
extern MessageList gCustomMessageList;
extern unsigned char byte_58EAA0[256];
extern unsigned char byte_58EBA0[256];
extern unsigned char* off_58ECA0[8];
extern Rect stru_58ECC0;
extern Rect stru_58ECD0;
extern GameDialogReviewEntry gDialogReviewEntries[DIALOG_REVIEW_ENTRIES_CAPACITY];
extern int dword_58F460;
extern int dword_58F464;
extern int gGameDialogReviewWindowOldFont;
extern CacheEntry* gGameDialogRedButtonUpFrmHandle;
extern int dword_58F470[9];
extern CacheEntry* gGameDialogUpperHighlightFrmHandle;
extern CacheEntry* gGameDialogReviewButtonUpFrmHandle;
extern int gGameDialogLowerHighlightFrmHeight;
extern CacheEntry* gGameDialogReviewButtonDownFrmHandle;
extern unsigned char* gGameDialogRedButtonDownFrmData;
extern int gGameDialogLowerHighlightFrmWidth;
extern unsigned char* gGameDialogRedButtonUpFrmData;
extern int gGameDialogUpperHighlightFrmWidth;
extern Art* gGameDialogLowerHighlightFrm;
extern int gGameDialogUpperHighlightFrmHeight;
extern CacheEntry* gGameDialogRedButtonDownFrmHandle;
extern CacheEntry* gGameDialogLowerHighlightFrmHandle;
extern Art* gGameDialogUpperHighlightFrm;
extern int dword_58F4C8;
extern unsigned int gGameDialogFidgetLastUpdateTimestamp;
extern int gGameDialogFidgetReaction;
extern Program* gDialogReplyProgram;
extern int gDialogReplyMessageListId;
extern int gDialogReplyMessageId;
extern int dword_58F4E0;
extern char gDialogReplyText[900];
extern GameDialogOptionEntry gDialogOptionEntries[DIALOG_OPTION_ENTRIES_CAPACITY];
extern int dword_596C30;
extern unsigned int gGameDialogFidgetUpdateDelay;
extern int gGameDialogFidgetFrmCurrentFrame;

int gameDialogInit();
int _gdialogReset();
int gameDialogReset();
int gameDialogExit();
bool _gdialogActive();
void gameDialogEnter(Object* a1, int a2);
void _gdialogSystemEnter();
void gameDialogStartLips(const char* a1);
void gameDialogEndLips();
int gameDialogEnable();
int gameDialogDisable();
int _gdialogInitFromScript(int headFid, int reaction);
int _gdialogExitFromScript();
void gameDialogSetBackground(int a1);
void gameDialogRenderSupplementaryMessage(char* msg);
int _gdialogStart();
int _gdialogSayMessage();
int gameDialogAddMessageOptionWithProcIdentifier(int messageListId, int messageId, const char* a3, int reaction);
int gameDialogAddTextOptionWithProcIdentifier(int messageListId, const char* text, const char* a3, int reaction);
int gameDialogAddMessageOptionWithProc(int messageListId, int messageId, int proc, int reaction);
int gameDialogAddTextOptionWithProc(int messageListId, const char* text, int proc, int reaction);
int gameDialogSetMessageReply(Program* a1, int a2, int a3);
int gameDialogSetTextReply(Program* a1, int a2, const char* a3);
int _gdialogGo();
void _gdialogUpdatePartyStatus();
int gameDialogAddMessageOption(int a1, int a2, int a3);
int gameDialogAddTextOption(int a1, const char* a2, int a3);
int gameDialogReviewWindowInit(int* win);
int gameDialogReviewWindowFree(int* win);
int gameDialogShowReview();
void gameDialogReviewButtonOnMouseUp(int btn, int keyCode);
void gameDialogReviewWindowUpdate(int win, int origin);
void dialogReviewEntriesClear();
int gameDialogAddReviewMessage(int messageListId, int messageId);
int gameDialogAddReviewText(const char* text);
int gameDialogSetReviewOptionMessage(int messageListId, int messageId);
int gameDialogSetReviewOptionText(const char* text);
int _gdProcessInit();
void _gdProcessCleanup();
int _gdProcessExit();
void gameDialogRenderCaps();
int _gdProcess();
int _gdProcessChoice(int a1);
void gameDialogOptionOnMouseEnter(int a1);
void gameDialogOptionOnMouseExit(int a1);
void gameDialogRenderReply();
void _gdProcessUpdate();
int _gdCreateHeadWindow();
void _gdDestroyHeadWindow();
void _gdSetupFidget(int headFid, int reaction);
void gameDialogWaitForFidgetToComplete();
void _gdPlayTransition(int a1);
void _reply_arrow_up(int btn, int a2);
void _reply_arrow_down(int btn, int a2);
void _reply_arrow_restore(int btn, int a2);
void _demo_copy_title(int win);
void _demo_copy_options(int win);
void _gDialogRefreshOptionsRect(int win, Rect* drawRect);
void gameDialogTicker();
void _talk_to_critter_reacts(int a1);
void _gdialog_scroll_subwin(int a1, int a2, unsigned char* a3, unsigned char* a4, unsigned char* a5, int a6, int a7);
int _text_num_lines(const char* a1, int a2);
int gameDialogDrawText(unsigned char* buffer, Rect* rect, char* string, int* a4, int height, int pitch, int color, int a7);
void gameDialogSetBarterModifier(int modifier);
int gameDialogBarter(int modifier);
void _barter_end_to_talk_to();
int _gdialog_barter_create_win();
void _gdialog_barter_destroy_win();
void _gdialog_barter_cleanup_tables();
int partyMemberControlWindowInit();
void partyMemberControlWindowFree();
void partyMemberControlWindowUpdate();
void gameDialogCombatControlButtonOnMouseUp(int a1, int a2);
int _gdPickAIUpdateMsg(Object* obj);
int _gdCanBarter();
void partyMemberControlWindowHandleEvents();
int partyMemberCustomizationWindowInit();
void partyMemberCustomizationWindowFree();
void partyMemberCustomizationWindowHandleEvents();
void partyMemberCustomizationWindowUpdate();
void _gdCustomSelectRedraw(unsigned char* dest, int pitch, int type, int selectedIndex);
int _gdCustomSelect(int a1);
void _gdCustomUpdateSetting(int option, int value);
void gameDialogBarterButtonUpMouseUp(int btn, int a2);
int _gdialog_window_create();
void _gdialog_window_destroy();
int gameDialogWindowRenderBackground();
int _talkToRefreshDialogWindowRect(Rect* rect);
void gameDialogRenderHighlight(unsigned char* src, int srcWidth, int srcHeight, int srcPitch, unsigned char* dest, int x, int y, int destPitch, unsigned char* a9, unsigned char* a10);
void gameDialogRenderTalkingHead(Art* art, int frame);
void gameDialogPrepareHighlights();

#endif /* GAME_DIALOG_H */
