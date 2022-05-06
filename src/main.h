#ifndef MAIN_H
#define MAIN_H

#include "art.h"

#include <stdbool.h>

typedef enum MainMenuButton {
    MAIN_MENU_BUTTON_INTRO,
    MAIN_MENU_BUTTON_NEW_GAME,
    MAIN_MENU_BUTTON_LOAD_GAME,
    MAIN_MENU_BUTTON_OPTIONS,
    MAIN_MENU_BUTTON_CREDITS,
    MAIN_MENU_BUTTON_EXIT,
    MAIN_MENU_BUTTON_COUNT,
} MainMenuButton;

typedef enum MainMenuOption {
    MAIN_MENU_INTRO,
    MAIN_MENU_NEW_GAME,
    MAIN_MENU_LOAD_GAME,
    MAIN_MENU_3,
    MAIN_MENU_TIMEOUT,
    MAIN_MENU_CREDITS,
    MAIN_MENU_QUOTES,
    MAIN_MENU_EXIT,
    MAIN_MENU_SELFRUN,
    MAIN_MENU_OPTIONS,
} MainMenuOption;

extern char byte_5194C8[];
extern int dword_5194D8;
extern char** off_5194DC;
extern int dword_5194E0;
extern int dword_5194E4;
extern bool dword_5194E8;
extern int mainMenuWindowHandle;
extern unsigned char* mainMenuWindowBuf;
extern unsigned char* gMainMenuBackgroundFrmData;
extern unsigned char* gMainMenuButtonUpFrmData;
extern unsigned char* gMainMenuButtonDownFrmData;
extern bool dword_519504;
extern bool gMainMenuWindowInitialized;
extern unsigned int gMainMenuScreensaverDelay;
extern const int gMainMenuButtonKeyBindings[MAIN_MENU_BUTTON_COUNT];
extern const int dword_519528[MAIN_MENU_BUTTON_COUNT];

extern bool dword_614838;
extern int gMainMenuButtons[MAIN_MENU_BUTTON_COUNT];
extern bool gMainMenuWindowHidden;
extern CacheEntry* gMainMenuButtonUpFrmHandle;
extern CacheEntry* gMainMenuButtonDownFrmHandle;
extern CacheEntry* gMainMenuBackgroundFrmHandle;

int falloutMain(int argc, char** argv);
bool falloutInit(int argc, char** argv);
int main_load_new(char* fname);
void mainLoop();
void main_selfrun_exit();
void showDeath();
void main_death_voiceover_callback();
int mainDeathGrabTextFile(const char* fileName, char* dest);
int mainDeathWordWrap(char* text, int width, short* beginnings, short* count);
int mainMenuWindowInit();
void mainMenuWindowFree();
void mainMenuWindowHide(bool animate);
void mainMenuWindowUnhide(bool animate);
int main_menu_is_enabled();
int mainMenuWindowHandleEvents();

#endif /* MAIN_H */
