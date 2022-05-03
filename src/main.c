#include "main.h"

#include "autorun.h"
#include "character_selector.h"
#include "color.h"
#include "core.h"
#include "credits.h"
#include "cycle.h"
#include "db.h"
#include "debug.h"
#include "draw.h"
#include "endgame.h"
#include "game.h"
#include "game_config.h"
#include "game_mouse.h"
#include "game_movie.h"
#include "game_sound.h"
#include "loadsave.h"
#include "map.h"
#include "object.h"
#include "options.h"
#include "palette.h"
#include "random.h"
#include "scripts.h"
#include "selfrun.h"
#include "text_font.h"
#include "version.h"
#include "window_manager.h"
#include "word_wrap.h"
#include "world_map.h"

#include <intrin.h>

// 0x5194C8
char byte_5194C8[] = "artemple.map";

// 0x5194D8
int dword_5194D8 = 0;

// 0x5194DC
char** off_5194DC = NULL;

// 0x5194E0
int dword_5194E0 = 0;

// 0x5194E4
int dword_5194E4 = 0;

// 0x5194E8
bool dword_5194E8 = false;

// 0x5194F0
int gMainMenuWindow = -1;

// 0x5194F4
unsigned char* gMainMenuWindowBuffer = NULL;

// 0x5194F8
unsigned char* gMainMenuBackgroundFrmData = NULL;

// 0x5194FC
unsigned char* gMainMenuButtonUpFrmData = NULL;

// 0x519500
unsigned char* gMainMenuButtonDownFrmData = NULL;

// 0x519504
bool dword_519504 = false;

// 0x519508
bool gMainMenuWindowInitialized = false;

// 0x51950C
unsigned int gMainMenuScreensaverDelay = 120000;

// 0x519510
const int gMainMenuButtonKeyBindings[MAIN_MENU_BUTTON_COUNT] = {
    KEY_LOWERCASE_I, // intro
    KEY_LOWERCASE_N, // new game
    KEY_LOWERCASE_L, // load game
    KEY_LOWERCASE_O, // options
    KEY_LOWERCASE_C, // credits
    KEY_LOWERCASE_E, // exit
};

// 0x519528
const int dword_519528[MAIN_MENU_BUTTON_COUNT] = {
    MAIN_MENU_INTRO,
    MAIN_MENU_NEW_GAME,
    MAIN_MENU_LOAD_GAME,
    MAIN_MENU_OPTIONS,
    MAIN_MENU_CREDITS,
    MAIN_MENU_EXIT,
};

// 0x614838
bool dword_614838;

// 0x614840
int gMainMenuButtons[MAIN_MENU_BUTTON_COUNT];

// 0x614858
bool gMainMenuWindowHidden;

// 0x61485C
CacheEntry* gMainMenuButtonUpFrmHandle;

// 0x614860
CacheEntry* gMainMenuButtonDownFrmHandle;

// 0x614864
CacheEntry* gMainMenuBackgroundFrmHandle;

// 0x48099C
int falloutMain(int argc, char** argv)
{
    if (!autorunMutexCreate()) {
        return 1;
    }

    if (!falloutInit(argc, argv)) {
        return 1;
    }

    gameMoviePlay(MOVIE_IPLOGO, GAME_MOVIE_FADE_IN);
    gameMoviePlay(MOVIE_INTRO, 0);
    gameMoviePlay(MOVIE_CREDITS, 0);

    if (mainMenuWindowInit() == 0) {
        bool done = false;
        while (!done) {
            keyboardReset();
            sub_450A08("07desert", 11);
            mainMenuWindowUnhide(1);

            mouseShowCursor();
            int mainMenuRc = mainMenuWindowHandleEvents();
            mouseHideCursor();

            switch (mainMenuRc) {
            case MAIN_MENU_INTRO:
                mainMenuWindowHide(true);
                gameMoviePlay(MOVIE_INTRO, GAME_MOVIE_PAUSE_MUSIC);
                gameMoviePlay(MOVIE_CREDITS, 0);
                break;
            case MAIN_MENU_NEW_GAME:
                mainMenuWindowHide(true);
                mainMenuWindowFree();
                if (characterSelectorOpen() == 2) {
                    gameMoviePlay(MOVIE_ELDER, GAME_MOVIE_STOP_MUSIC);
                    randomSeedPrerandom(-1);
                    sub_480D4C(byte_5194C8);
                    mainLoop();
                    paletteFadeTo(gPaletteWhite);
                    objectHide(gDude, NULL);
                    sub_482084();
                    gameReset();
                    if (dword_5194E8 != 0) {
                        showDeath();
                        dword_5194E8 = 0;
                    }
                }

                mainMenuWindowInit();

                break;
            case MAIN_MENU_LOAD_GAME:
                if (1) {
                    int win = windowCreate(0, 0, 640, 480, byte_6A38D0[0], WINDOW_FLAG_0x10 | WINDOW_FLAG_0x04);
                    mainMenuWindowHide(true);
                    mainMenuWindowFree();
                    backgroundSoundDelete();
                    dword_5186CC = 0;
                    gDude->flags &= ~0x08;
                    dword_5194E8 = 0;
                    objectShow(gDude, NULL);
                    mouseHideCursor();
                    sub_481FB4();
                    gameMouseSetCursor(MOUSE_CURSOR_NONE);
                    mouseShowCursor();
                    colorPaletteLoad("color.pal");
                    paletteFadeTo(stru_51DF34);
                    int loadGameRc = lsgLoadGame(LOAD_SAVE_MODE_FROM_MAIN_MENU);
                    if (loadGameRc == -1) {
                        debugPrint("\n ** Error running LoadGame()! **\n");
                    } else if (loadGameRc != 0) {
                        windowDestroy(win);
                        win = -1;
                        mainLoop();
                    }
                    paletteFadeTo(gPaletteWhite);
                    if (win != -1) {
                        windowDestroy(win);
                    }
                    objectHide(gDude, NULL);
                    sub_482084();
                    gameReset();
                    if (dword_5194E8 != 0) {
                        showDeath();
                        dword_5194E8 = 0;
                    }
                    mainMenuWindowInit();
                }
                break;
            case MAIN_MENU_TIMEOUT:
                debugPrint("Main menu timed-out\n");
                // FALLTHROUGH
            case MAIN_MENU_3:
                // sub_48109C();
                break;
            case MAIN_MENU_OPTIONS:
                mainMenuWindowHide(false);
                mouseShowCursor();
                showOptionsWithInitialKeyCode(112);
                gameMouseSetCursor(MOUSE_CURSOR_ARROW);
                mouseShowCursor();
                mainMenuWindowUnhide(0);
                break;
            case MAIN_MENU_CREDITS:
                mainMenuWindowHide(true);
                creditsOpen("credits.txt", -1, false);
                break;
            case MAIN_MENU_QUOTES:
                if (0 == 1) {
                    mainMenuWindowHide(true);
                    creditsOpen("quotes.txt", -1, true);
                }
                break;
            case MAIN_MENU_EXIT:
            case -1:
                done = true;
                mainMenuWindowHide(true);
                mainMenuWindowFree();
                backgroundSoundDelete();
                break;
            case MAIN_MENU_SELFRUN:
                // sub_480F64();
                break;
            }
        }
    }

    backgroundSoundDelete();
    sub_480F38();
    gameExit();

    autorunMutexClose();

    return 0;
}

// 0x480CC0
bool falloutInit(int argc, char** argv)
{
    if (gameInitWithOptions("FALLOUT II", false, 0, 0, argc, argv) == -1) {
        return false;
    }

    if (off_5194DC != NULL) {
        sub_480F38();
    }

    if (sub_4A8BE0(&off_5194DC, &dword_5194E0) == 0) {
        dword_5194E4 = 0;
    }

    return true;
}

// 0x480D4C
int sub_480D4C(char* mapFileName)
{
    dword_5186CC = 0;
    dword_5194E8 = 0;
    gDude->flags &= ~(0x08);
    objectShow(gDude, NULL);
    mouseHideCursor();

    int win = windowCreate(0, 0, 640, 480, byte_6A38D0[0], WINDOW_FLAG_0x10 | WINDOW_FLAG_0x04);
    windowRefresh(win);

    colorPaletteLoad("color.pal");
    paletteFadeTo(stru_51DF34);
    sub_481FB4();
    gameMouseSetCursor(MOUSE_CURSOR_NONE);
    mouseShowCursor();
    mapLoadByName(mapFileName);
    worldmapStartMapMusic();
    paletteFadeTo(gPaletteWhite);
    windowDestroy(win);
    colorPaletteLoad("color.pal");
    paletteFadeTo(stru_51DF34);
    return 0;
}

// 0x480E48
void mainLoop()
{
    bool cursorWasHidden = cursorIsHidden();
    if (cursorWasHidden) {
        mouseShowCursor();
    }

    dword_5194D8 = 0;

    scriptsEnable();

    while (dword_5186CC == 0) {
        int keyCode = sub_4C8B78();
        gameHandleKey(keyCode, false);

        scriptsHandleRequests();

        mapHandleTransition();

        if (dword_5194D8 != 0) {
            dword_5194D8 = 0;
        }

        if ((gDude->data.critter.combat.results & (DAM_DEAD | DAM_KNOCKED_OUT)) != 0) {
            endgameSetupDeathEnding(ENDGAME_DEATH_ENDING_REASON_DEATH);
            dword_5194E8 = 1;
            dword_5186CC = 2;
        }
    }

    scriptsDisable();

    if (cursorWasHidden) {
        mouseHideCursor();
    }
}

// 0x480F38
void sub_480F38()
{
    if (off_5194DC != NULL) {
        sub_4A8C10(&off_5194DC);
    }

    dword_5194E0 = 0;
    dword_5194E4 = 0;
    off_5194DC = NULL;
}

// 0x48118C
void showDeath()
{
    artCacheFlush();
    colorCycleDisable();
    gameMouseSetCursor(MOUSE_CURSOR_NONE);

    bool oldCursorIsHidden = cursorIsHidden();
    if (oldCursorIsHidden) {
        mouseShowCursor();
    }

    int win = windowCreate(0, 0, 640, 480, 0, WINDOW_FLAG_0x04);
    if (win != -1) {
        do {
            unsigned char* windowBuffer = windowGetBuffer(win);
            if (windowBuffer == NULL) {
                break;
            }

            // DEATH.FRM
            CacheEntry* backgroundHandle;
            int fid = buildFid(6, 309, 0, 0, 0);
            unsigned char* background = artLockFrameData(fid, 0, 0, &backgroundHandle);
            if (background == NULL) {
                break;
            }

            while (mouseGetEvent() != 0) {
                sub_4C8B78();
            }

            keyboardReset();
            inputEventQueueReset();

            blitBufferToBuffer(background, 640, 480, 640, windowBuffer, 640);
            artUnlock(backgroundHandle);

            const char* deathFileName = endgameDeathEndingGetFileName();

            int subtitles = 0;
            configGetInt(&gGameConfig, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_SUBTITLES_KEY, &subtitles);
            if (subtitles != 0) {
                char text[512];
                if (sub_4814B4(deathFileName, text) == 0) {
                    debugPrint("\n((ShowDeath)): %s\n", text);

                    short beginnings[WORD_WRAP_MAX_COUNT];
                    short count;
                    if (sub_481598(text, 560, beginnings, &count) == 0) {
                        unsigned char* p = windowBuffer + 640 * (480 - fontGetLineHeight() * count - 8);
                        bufferFill(p - 602, 564, fontGetLineHeight() * count + 2, 640, 0);
                        p += 40;
                        for (int index = 0; index < count; index++) {
                            fontDrawText(p, text + beginnings[index], 560, 640, byte_6A38D0[32767]);
                            p += 640 * fontGetLineHeight();
                        }
                    }
                }
            }

            windowRefresh(win);

            colorPaletteLoad("art\\intrface\\death.pal");
            paletteFadeTo(stru_51DF34);

            dword_614838 = false;
            speechSetEndCallback(sub_4814A8);

            unsigned int delay;
            if (speechLoad(deathFileName, 10, 14, 15) == -1) {
                delay = 3000;
            } else {
                delay = UINT_MAX;
            }

            sub_450F8C();

            unsigned int time = sub_4C9370();
            int keyCode;
            do {
                keyCode = sub_4C8B78();
            } while (keyCode == -1 && !dword_614838 && getTicksSince(time) < delay);

            speechSetEndCallback(NULL);

            speechDelete();

            while (mouseGetEvent() != 0) {
                sub_4C8B78();
            }

            if (keyCode == -1) {
                coreDelayProcessingEvents(500);
            }

            paletteFadeTo(gPaletteBlack);
            colorPaletteLoad("color.pal");
        } while (0);
        windowDestroy(win);
    }

    if (oldCursorIsHidden) {
        mouseHideCursor();
    }

    gameMouseSetCursor(MOUSE_CURSOR_ARROW);

    colorCycleEnable();
}

// 0x4814A8
void sub_4814A8()
{
    dword_614838 = true;
}

// Read endgame subtitle.
//
// 0x4814B4
int sub_4814B4(const char* fileName, char* dest)
{
    const char* p = strrchr(fileName, '\\');
    if (p == NULL) {
        return -1;
    }

    char* language = NULL;
    if (!configGetString(&gGameConfig, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_LANGUAGE_KEY, &language)) {
        debugPrint("MAIN: Error grabing language for ending. Defaulting to english.\n");
        language = byte_50B00C;
    }

    char path[MAX_PATH];
    sprintf(path, "text\\%s\\cuts\\%s%s", language, p + 1, ".TXT");

    File* stream = fileOpen(path, "rt");
    if (stream == NULL) {
        return -1;
    }

    while (true) {
        int c = fileReadChar(stream);
        if (c == -1) {
            break;
        }

        if (c == '\n') {
            c = ' ';
        }

        *dest++ = (c & 0xFF);
    }

    fileClose(stream);

    *dest = '\0';

    return 0;
}

// 0x481598
int sub_481598(char* text, int width, short* beginnings, short* count)
{
    // TODO: Probably wrong.
    while (true) {
        char* p = text;
        while (*p != ':') {
            if (*p != '\0') {
                p++;
                if (*p == ':') {
                    break;
                }
                if (*p != '\0') {
                    continue;
                }
            }
            p = NULL;
            break;
        }

        if (p == NULL) {
            break;
        }

        if (p - 1 < text) {
            break;
        }
        p[0] = ' ';
        p[-1] = ' ';
    }

    if (wordWrap(text, width, beginnings, count) == -1) {
        return -1;
    }

    // TODO: Probably wrong.
    *count -= 1;

    for (int index = 1; index < *count; index++) {
        char* p = text + beginnings[index];
        while (p >= text && *p != ' ') {
            p--;
            beginnings[index]--;
        }

        if (p != NULL) {
            *p = '\0';
            beginnings[index]++;
        }
    }

    return 0;
}

// 0x481650
int mainMenuWindowInit()
{
    int fid;
    MessageListItem msg;
    int len;

    if (gMainMenuWindowInitialized) {
        return 0;
    }

    colorPaletteLoad("color.pal");

    gMainMenuWindow = windowCreate(0, 0, 640, 480, 0, 12);
    if (gMainMenuWindow == -1) {
        mainMenuWindowFree();
        return -1;
    }

    gMainMenuWindowBuffer = windowGetBuffer(gMainMenuWindow);

    // mainmenu.frm
    int backgroundFid = buildFid(6, 140, 0, 0, 0);
    gMainMenuBackgroundFrmData = artLockFrameData(backgroundFid, 0, 0, &gMainMenuBackgroundFrmHandle);
    if (gMainMenuBackgroundFrmData == NULL) {
        mainMenuWindowFree();
        return -1;
    }

    blitBufferToBuffer(gMainMenuBackgroundFrmData, 640, 480, 640, gMainMenuWindowBuffer, 640);
    artUnlock(gMainMenuBackgroundFrmHandle);

    int oldFont = fontGetCurrent();
    fontSetCurrent(100);

    // Copyright.
    msg.num = 20;
    if (messageListGetItem(&gMiscMessageList, &msg)) {
        windowDrawText(gMainMenuWindow, msg.text, 0, 15, 460, byte_6A38D0[21091] | 0x6000000);
    }

    // Version.
    char version[VERSION_MAX];
    versionGetVersion(version);
    len = fontGetStringWidth(version);
    windowDrawText(gMainMenuWindow, version, 0, 615 - len, 460, byte_6A38D0[21091] | 0x6000000);

    // menuup.frm
    fid = buildFid(6, 299, 0, 0, 0);
    gMainMenuButtonUpFrmData = artLockFrameData(fid, 0, 0, &gMainMenuButtonUpFrmHandle);
    if (gMainMenuButtonUpFrmData == NULL) {
        mainMenuWindowFree();
        return -1;
    }

    // menudown.frm
    fid = buildFid(6, 300, 0, 0, 0);
    gMainMenuButtonDownFrmData = artLockFrameData(fid, 0, 0, &gMainMenuButtonDownFrmHandle);
    if (gMainMenuButtonDownFrmData == NULL) {
        mainMenuWindowFree();
        return -1;
    }

    __stosd((unsigned long*)gMainMenuButtons, -1, MAIN_MENU_BUTTON_COUNT);

    for (int index = 0; index < MAIN_MENU_BUTTON_COUNT; index++) {
        gMainMenuButtons[index] = buttonCreate(gMainMenuWindow, 30, 19 + index * 42 - index, 26, 26, -1, -1, 1111, gMainMenuButtonKeyBindings[index], gMainMenuButtonUpFrmData, gMainMenuButtonDownFrmData, 0, 32);
        if (gMainMenuButtons[index] == -1) {
            mainMenuWindowFree();
            return -1;
        }

        buttonSetMask(gMainMenuButtons[index], gMainMenuButtonUpFrmData);
    }

    fontSetCurrent(104);

    for (int index = 0; index < MAIN_MENU_BUTTON_COUNT; index++) {
        msg.num = 9 + index;
        if (messageListGetItem(&gMiscMessageList, &msg)) {
            len = fontGetStringWidth(msg.text);
            fontDrawText(gMainMenuWindowBuffer + 640 * (42 * index - index + 20) + 126 - (len / 2), msg.text, 640 - (126 - (len / 2)) - 1, 640, byte_6A38D0[21091]);
        }
    }

    fontSetCurrent(oldFont);

    gMainMenuWindowInitialized = true;
    gMainMenuWindowHidden = true;

    return 0;
}

// 0x481968
void mainMenuWindowFree()
{
    if (!gMainMenuWindowInitialized) {
        return;
    }

    for (int index = 0; index < MAIN_MENU_BUTTON_COUNT; index++) {
        // FIXME: Why it tries to free only invalid buttons?
        if (gMainMenuButtons[index] == -1) {
            buttonDestroy(gMainMenuButtons[index]);
        }
    }

    if (gMainMenuButtonDownFrmData) {
        artUnlock(gMainMenuButtonDownFrmHandle);
        gMainMenuButtonDownFrmHandle = NULL;
        gMainMenuButtonDownFrmData = NULL;
    }

    if (gMainMenuButtonUpFrmData) {
        artUnlock(gMainMenuButtonUpFrmHandle);
        gMainMenuButtonUpFrmHandle = NULL;
        gMainMenuButtonUpFrmData = NULL;
    }

    if (gMainMenuWindow != -1) {
        windowDestroy(gMainMenuWindow);
    }

    gMainMenuWindowInitialized = false;
}

// 0x481A00
void mainMenuWindowHide(bool animate)
{
    if (!gMainMenuWindowInitialized) {
        return;
    }

    if (gMainMenuWindowHidden) {
        return;
    }

    soundContinueAll();

    if (animate) {
        paletteFadeTo(gPaletteBlack);
        soundContinueAll();
    }

    windowHide(gMainMenuWindow);

    gMainMenuWindowHidden = true;
}

// 0x481A48
void mainMenuWindowUnhide(bool animate)
{
    if (!gMainMenuWindowInitialized) {
        return;
    }

    if (!gMainMenuWindowHidden) {
        return;
    }

    windowUnhide(gMainMenuWindow);

    if (animate) {
        colorPaletteLoad("color.pal");
        paletteFadeTo(stru_51DF34);
    }

    gMainMenuWindowHidden = false;
}

// 0x481AA8
int sub_481AA8()
{
    return 1;
}

// 0x481AEC
int mainMenuWindowHandleEvents()
{
    dword_519504 = true;

    bool oldCursorIsHidden = cursorIsHidden();
    if (oldCursorIsHidden) {
        mouseShowCursor();
    }

    unsigned int tick = sub_4C9370();

    int rc = -1;
    while (rc == -1) {
        int keyCode = sub_4C8B78();

        for (int buttonIndex = 0; buttonIndex < MAIN_MENU_BUTTON_COUNT; buttonIndex++) {
            if (keyCode == gMainMenuButtonKeyBindings[buttonIndex] || keyCode == toupper(gMainMenuButtonKeyBindings[buttonIndex])) {
                soundPlayFile("nmselec1");

                rc = dword_519528[buttonIndex];

                if (buttonIndex == MAIN_MENU_BUTTON_CREDITS && (gPressedPhysicalKeys[DIK_RSHIFT] != KEY_STATE_UP || gPressedPhysicalKeys[DIK_LSHIFT] != KEY_STATE_UP)) {
                    rc = MAIN_MENU_QUOTES;
                }

                break;
            }
        }

        if (rc == -1) {
            if (keyCode == KEY_CTRL_R) {
                rc = MAIN_MENU_SELFRUN;
                continue;
            } else if (keyCode == KEY_PLUS || keyCode == KEY_EQUAL) {
                brightnessIncrease();
            } else if (keyCode == KEY_MINUS || keyCode == KEY_UNDERSCORE) {
                brightnessDecrease();
            } else if (keyCode == KEY_UPPERCASE_D || keyCode == KEY_LOWERCASE_D) {
                rc = MAIN_MENU_3;
                continue;
            } else if (keyCode == 1111) {
                if (!(mouseGetEvent() & MOUSE_EVENT_LEFT_BUTTON_REPEAT)) {
                    soundPlayFile("nmselec0");
                }
                continue;
            }
        }

        if (keyCode == KEY_ESCAPE || dword_5186CC == 3) {
            rc = MAIN_MENU_EXIT;
            soundPlayFile("nmselec1");
            break;
        } else if (dword_5186CC == 2) {
            dword_5186CC = 0;
        } else {
            if (getTicksSince(tick) >= gMainMenuScreensaverDelay) {
                rc = MAIN_MENU_TIMEOUT;
            }
        }
    }

    if (oldCursorIsHidden) {
        mouseHideCursor();
    }

    dword_519504 = false;

    return rc;
}
