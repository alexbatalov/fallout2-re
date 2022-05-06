#ifndef LOAD_SAVE_GAME_H
#define LOAD_SAVE_GAME_H

#include "art.h"
#include "db.h"
#include "geometry.h"
#include "message.h"

#include <stdbool.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define LOAD_SAVE_DESCRIPTION_LENGTH (30)
#define LOAD_SAVE_HANDLER_COUNT (27)

typedef enum LoadSaveMode {
    // Special case - loading game from main menu.
    LOAD_SAVE_MODE_FROM_MAIN_MENU,

    // Normal (full-screen) save/load screen.
    LOAD_SAVE_MODE_NORMAL,

    // Quick load/save.
    LOAD_SAVE_MODE_QUICK,
} LoadSaveMode;

typedef enum LoadSaveWindowType {
    LOAD_SAVE_WINDOW_TYPE_SAVE_GAME,
    LOAD_SAVE_WINDOW_TYPE_PICK_QUICK_SAVE_SLOT,
    LOAD_SAVE_WINDOW_TYPE_LOAD_GAME,
    LOAD_SAVE_WINDOW_TYPE_LOAD_GAME_FROM_MAIN_MENU,
    LOAD_SAVE_WINDOW_TYPE_PICK_QUICK_LOAD_SLOT,
} LoadSaveWindowType;

typedef enum LoadSaveSlotState {
    SLOT_STATE_EMPTY,
    SLOT_STATE_OCCUPIED,
    SLOT_STATE_ERROR,
    SLOT_STATE_UNSUPPORTED_VERSION,
} LoadSaveSlotState;

typedef int LoadGameHandler(File* stream);
typedef int SaveGameHandler(File* stream);

#define LSGAME_MSG_NAME ("LSGAME.MSG")

typedef struct STRUCT_613D30 {
    char field_0[24];
    short field_18;
    short field_1A;
    // TODO: The type is probably char, but it's read with the same function as
    // reading unsigned chars, which in turn probably result of collapsing
    // reading functions.
    unsigned char field_1C;
    char character_name[32];
    char description[LOAD_SAVE_DESCRIPTION_LENGTH];
    short field_5C;
    short field_5E;
    short field_60;
    int field_64;
    short field_68;
    short field_6A;
    short field_6C;
    int field_70;
    short field_74;
    short field_76;
    char file_name[16];
} STRUCT_613D30;

typedef enum LoadSaveFrm {
    LOAD_SAVE_FRM_BACKGROUND,
    LOAD_SAVE_FRM_BOX,
    LOAD_SAVE_FRM_PREVIEW_COVER,
    LOAD_SAVE_FRM_RED_BUTTON_PRESSED,
    LOAD_SAVE_FRM_RED_BUTTON_NORMAL,
    LOAD_SAVE_FRM_ARROW_DOWN_NORMAL,
    LOAD_SAVE_FRM_ARROW_DOWN_PRESSED,
    LOAD_SAVE_FRM_ARROW_UP_NORMAL,
    LOAD_SAVE_FRM_ARROW_UP_PRESSED,
    LOAD_SAVE_FRM_COUNT,
} LoadSaveFrm;

extern const int gLoadSaveFrmIds[LOAD_SAVE_FRM_COUNT];

extern int dword_5193B8;
extern bool dword_5193BC;
extern bool gLoadSaveWindowIsoWasEnabled;
extern int dword_5193C4;
extern int dword_5193C8;
extern char* off_5193CC;
extern char byte_5193D0[];
extern SaveGameHandler* off_5193EC[LOAD_SAVE_HANDLER_COUNT];
extern LoadGameHandler* off_519458[LOAD_SAVE_HANDLER_COUNT];
extern int dword_5194C4;

extern Size gLoadSaveFrmSizes[LOAD_SAVE_FRM_COUNT];
extern MessageList gLoadSaveMessageList;
extern STRUCT_613D30 stru_613D30[10];
extern int dword_614280[10];
extern unsigned char* off_6142A8;
extern unsigned char* off_6142AC;
extern MessageListItem gLoadSaveMessageListItem;
extern int dword_6142C0;
extern int gLoadSaveWindow;
extern unsigned char* gLoadSaveFrmData[LOAD_SAVE_FRM_COUNT];
extern unsigned char* off_6142EC;
extern char byte_6142F0[MAX_PATH];
extern char byte_6143F4[MAX_PATH];
extern char byte_6144F8[MAX_PATH];
extern char byte_6145FC[MAX_PATH];
extern unsigned char* gLoadSaveWindowBuffer;
extern char byte_614704[MAX_PATH];
extern File* off_614808;
extern int dword_61480C;
extern int gLoadSaveWindowOldFont;
extern CacheEntry* gLoadSaveFrmHandles[LOAD_SAVE_FRM_COUNT];

void InitLoadSave();
void ResetLoadSave();
int lsgSaveGame(int mode);
int QuickSnapShot();
int lsgLoadGame(int mode);
int lsgWindowInit(int windowType);
int lsgWindowFree(int windowType);
int lsgPerformSaveGame();
int isLoadingGame();
int lsgLoadGameInSlot(int slot);
int lsgSaveHeaderInSlot(int slot);
int lsgLoadHeaderInSlot(int slot);
int GetSlotList();
void ShowSlotList(int a1);
void DrawInfoBox(int a1);
int LoadTumbSlot(int a1);
int GetComment(int a1);
int get_input_str2(int win, int doneKeyCode, int cancelKeyCode, char* description, int maxLength, int x, int y, int textColor, int backgroundColor, int flags);
int DummyFunc(File* stream);
int PrepLoad(File* stream);
int EndLoad(File* stream);
int GameMap2Slot(File* stream);
int SlotMap2Game(File* stream);
int mygets(char* dest, File* stream);
int copy_file(const char* a1, const char* a2);
void lsgInit();
int MapDirErase(const char* path, const char* a2);
int sub_4800C8(const char* a1, const char* a2);
int SaveBackup();
int RestoreSave();
int LoadObjDudeCid(File* stream);
int SaveObjDudeCid(File* stream);
int EraseSave();

#endif /* LOAD_SAVE_GAME_H */
