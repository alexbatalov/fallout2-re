#ifndef GAME_H
#define GAME_H

#include <stdbool.h>

#include "game_vars.h"
#include "message.h"

typedef enum GameState {
    GAME_STATE_0,
    GAME_STATE_1,
    GAME_STATE_2,
    GAME_STATE_3,
    GAME_STATE_4,
    GAME_STATE_5,
} GameState;

extern char _aGame_0[];

extern bool gGameUiDisabled;
extern int _game_state_cur;
extern bool gIsMapper;
extern int* gGameGlobalVars;
extern int gGameGlobalVarsLength;
extern const char* asc_5186C8;
extern int _game_user_wants_to_quit;

extern MessageList gMiscMessageList;
extern int _master_db_handle;
extern int _critter_db_handle;

int gameInitWithOptions(const char* windowTitle, bool isMapper, int a3, int a4, int argc, char** argv);
void gameReset();
void gameExit();
int gameHandleKey(int eventCode, bool isInCombatMode);
void gameUiDisable(int a1);
void gameUiEnable();
bool gameUiIsDisabled();
int gameGetGlobalVar(int var);
int gameSetGlobalVar(int var, int value);
int gameLoadGlobalVars();
int globalVarsRead(const char* path, const char* section, int* variablesListLengthPtr, int** variablesListPtr);
int _game_state();
int _game_state_request(int a1);
void _game_state_update();
void game_display_counter(double value);
int gameTakeScreenshot(int width, int height, unsigned char* buffer, unsigned char* palette);
void gameFreeGlobalVars();
void showHelp();
int showQuitConfirmationDialog();
int gameDbInit();
void showSplash();

#endif /* GAME_H */
