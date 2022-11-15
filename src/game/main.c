#include "main.h"

// NOTE: Actual file name is unknown. Functions in this module do not present
// in debug symbols from `mapper2.exe`. In OS X binary these functions appear
// very far from the ones found in `mainmenu.c`, implying they are in separate
// compilation unit. In Windows binary these functions appear between
// `loadsave.c` and `mainmenu.c`. Based on the order it's file name should be
// between these two, so `main.c` is a perfect candidate, but again, it's just a
// guess.
//
// Function names and visibility scope are from in OS X binary.

#include <stdbool.h>
#include <stddef.h>

#include "game/amutex.h"
#include "game/art.h"
#include "game/select.h"
#include "color.h"
#include "core.h"
#include "game/credits.h"
#include "game/cycle.h"
#include "debug.h"
#include "draw.h"
#include "game/endgame.h"
#include "game/game.h"
#include "game/gconfig.h"
#include "game/gmouse.h"
#include "game/gmovie.h"
#include "game/gsound.h"
#include "game/loadsave.h"
#include "game/mainmenu.h"
#include "game/map.h"
#include "game/object.h"
#include "game/options.h"
#include "game/palette.h"
#include "game/proto.h"
#include "game/roll.h"
#include "game/scripts.h"
#include "game/selfrun.h"
#include "text_font.h"
#include "window_manager.h"
#include "window_manager_private.h"
#include "word_wrap.h"
#include "worldmap.h"

#define DEATH_WINDOW_WIDTH 640
#define DEATH_WINDOW_HEIGHT 480

static bool main_init_system(int argc, char** argv);
static int main_reset_system();
static void main_exit_system();
static int main_load_new(char* fname);
static int main_loadgame_new();
static void main_unload_new();
static void main_game_loop();
static bool main_selfrun_init();
static void main_selfrun_exit();
static void main_selfrun_record();
static void main_selfrun_play();
static void main_death_scene();
static void main_death_voiceover_callback();
static int mainDeathGrabTextFile(const char* fileName, char* dest);
static int mainDeathWordWrap(char* text, int width, short* beginnings, short* count);

// 0x5194C8
static char mainMap[] = "artemple.map";

// 0x5194D8
int main_game_paused = 0;

// 0x5194DC
static char** main_selfrun_list = NULL;

// 0x5194E0
static int main_selfrun_count = 0;

// 0x5194E4
static int main_selfrun_index = 0;

// 0x5194E8
static bool main_show_death_scene = false;

// 0x614838
static bool main_death_voiceover_done;

// 0x48099C
int RealMain(int argc, char** argv)
{
    if (!autorun_mutex_create()) {
        return 1;
    }

    if (!main_init_system(argc, argv)) {
        return 1;
    }

    gmovie_play(MOVIE_IPLOGO, GAME_MOVIE_FADE_IN);
    gmovie_play(MOVIE_INTRO, 0);
    gmovie_play(MOVIE_CREDITS, 0);

    if (main_menu_create() == 0) {
        bool done = false;
        while (!done) {
            kb_clear();
            gsound_background_play_level_music("07desert", 11);
            main_menu_show(1);

            mouse_show();
            int mainMenuRc = main_menu_loop();
            mouse_hide();

            switch (mainMenuRc) {
            case MAIN_MENU_INTRO:
                main_menu_hide(true);
                gmovie_play(MOVIE_INTRO, GAME_MOVIE_PAUSE_MUSIC);
                gmovie_play(MOVIE_CREDITS, 0);
                break;
            case MAIN_MENU_NEW_GAME:
                main_menu_hide(true);
                main_menu_destroy();
                if (select_character() == 2) {
                    gmovie_play(MOVIE_ELDER, GAME_MOVIE_STOP_MUSIC);
                    roll_set_seed(-1);
                    main_load_new(mainMap);
                    main_game_loop();
                    palette_fade_to(white_palette);

                    // NOTE: Uninline.
                    main_unload_new();

                    // NOTE: Uninline.
                    main_reset_system();

                    if (main_show_death_scene != 0) {
                        main_death_scene();
                        main_show_death_scene = 0;
                    }
                }

                main_menu_create();

                break;
            case MAIN_MENU_LOAD_GAME:
                if (1) {
                    int win = windowCreate(0, 0, 640, 480, colorTable[0], WINDOW_FLAG_0x10 | WINDOW_FLAG_0x04);
                    main_menu_hide(true);
                    main_menu_destroy();
                    gsound_background_stop();

                    // NOTE: Uninline.
                    main_loadgame_new();

                    loadColorTable("color.pal");
                    palette_fade_to(cmap);
                    int loadGameRc = LoadGame(LOAD_SAVE_MODE_FROM_MAIN_MENU);
                    if (loadGameRc == -1) {
                        debugPrint("\n ** Error running LoadGame()! **\n");
                    } else if (loadGameRc != 0) {
                        windowDestroy(win);
                        win = -1;
                        main_game_loop();
                    }
                    palette_fade_to(white_palette);
                    if (win != -1) {
                        windowDestroy(win);
                    }

                    // NOTE: Uninline.
                    main_unload_new();

                    // NOTE: Uninline.
                    main_reset_system();

                    if (main_show_death_scene != 0) {
                        main_death_scene();
                        main_show_death_scene = 0;
                    }
                    main_menu_create();
                }
                break;
            case MAIN_MENU_TIMEOUT:
                debugPrint("Main menu timed-out\n");
                // FALLTHROUGH
            case MAIN_MENU_SCREENSAVER:
                main_selfrun_play();
                break;
            case MAIN_MENU_OPTIONS:
                main_menu_hide(false);
                mouse_show();
                do_optionsFunc(112);
                gmouse_set_cursor(MOUSE_CURSOR_ARROW);
                mouse_show();
                main_menu_show(0);
                break;
            case MAIN_MENU_CREDITS:
                main_menu_hide(true);
                credits("credits.txt", -1, false);
                break;
            case MAIN_MENU_QUOTES:
                // NOTE: There is a strange cmp at 0x480C50. Both operands are
                // zero, set before the loop and do not modify afterwards. For
                // clarity this condition is omitted.
                main_menu_hide(true);
                credits("quotes.txt", -1, true);
                break;
            case MAIN_MENU_EXIT:
            case -1:
                done = true;
                main_menu_hide(true);
                main_menu_destroy();
                gsound_background_stop();
                break;
            case MAIN_MENU_SELFRUN:
                main_selfrun_record();
                break;
            }
        }
    }

    // NOTE: Uninline.
    main_exit_system();

    autorun_mutex_destroy();

    return 0;
}

// 0x480CC0
static bool main_init_system(int argc, char** argv)
{
    if (game_init("FALLOUT II", false, 0, 0, argc, argv) == -1) {
        return false;
    }

    // NOTE: Uninline.
    main_selfrun_init();

    return true;
}

// NOTE: Inlined.
//
// 0x480D0C
static int main_reset_system()
{
    game_reset();

    return 1;
}

// NOTE: Inlined.
//
// 0x480D18
static void main_exit_system()
{
    gsound_background_stop();

    // NOTE: Uninline.
    main_selfrun_exit();

    game_exit();
}

// 0x480D4C
static int main_load_new(char* mapFileName)
{
    game_user_wants_to_quit = 0;
    main_show_death_scene = 0;
    obj_dude->flags &= ~OBJECT_FLAT;
    obj_turn_on(obj_dude, NULL);
    mouse_hide();

    int win = windowCreate(0, 0, 640, 480, colorTable[0], WINDOW_FLAG_0x10 | WINDOW_FLAG_0x04);
    win_draw(win);

    loadColorTable("color.pal");
    palette_fade_to(cmap);
    map_init();
    gmouse_set_cursor(MOUSE_CURSOR_NONE);
    mouse_show();
    map_load(mapFileName);
    wmMapMusicStart();
    palette_fade_to(white_palette);
    windowDestroy(win);
    loadColorTable("color.pal");
    palette_fade_to(cmap);
    return 0;
}

// NOTE: Inlined.
//
// 0x480DF8
static int main_loadgame_new()
{
    game_user_wants_to_quit = 0;
    main_show_death_scene = 0;

    obj_dude->flags &= ~OBJECT_FLAT;

    obj_turn_on(obj_dude, NULL);
    mouse_hide();

    map_init();

    gmouse_set_cursor(MOUSE_CURSOR_NONE);
    mouse_show();

    return 0;
}

// 0x480E34
static void main_unload_new()
{
    obj_turn_off(obj_dude, NULL);
    map_exit();
}

// 0x480E48
static void main_game_loop()
{
    bool cursorWasHidden = mouse_hidden();
    if (cursorWasHidden) {
        mouse_show();
    }

    main_game_paused = 0;

    scr_enable();

    while (game_user_wants_to_quit == 0) {
        int keyCode = _get_input();
        game_handle_input(keyCode, false);

        scripts_check_state();

        map_check_state();

        if (main_game_paused != 0) {
            main_game_paused = 0;
        }

        if ((obj_dude->data.critter.combat.results & (DAM_DEAD | DAM_KNOCKED_OUT)) != 0) {
            endgameSetupDeathEnding(ENDGAME_DEATH_ENDING_REASON_DEATH);
            main_show_death_scene = 1;
            game_user_wants_to_quit = 2;
        }
    }

    scr_disable();

    if (cursorWasHidden) {
        mouse_hide();
    }
}

// NOTE: Inlined.
//
// 0x480EE4
static bool main_selfrun_init()
{
    if (main_selfrun_list != NULL) {
        // NOTE: Uninline.
        main_selfrun_exit();
    }

    if (selfrunInitFileList(&main_selfrun_list, &main_selfrun_count) != 0) {
        return false;
    }

    main_selfrun_index = 0;

    return true;
}

// 0x480F38
static void main_selfrun_exit()
{
    if (main_selfrun_list != NULL) {
        selfrunFreeFileList(&main_selfrun_list);
    }

    main_selfrun_count = 0;
    main_selfrun_index = 0;
    main_selfrun_list = NULL;
}

// 0x480F64
static void main_selfrun_record()
{
    SelfrunData selfrunData;
    bool ready = false;

    char** fileList;
    int fileListLength = fileNameListInit("maps\\*.map", &fileList, 0, 0);
    if (fileListLength != 0) {
        int selectedFileIndex = _win_list_select("Select Map", fileList, fileListLength, 0, 80, 80, 0x10000 | 0x100 | 4);
        if (selectedFileIndex != -1) {
            // NOTE: It's size is likely 13 chars (on par with SelfrunData
            // fields), but due to the padding it takes 16 chars on stack.
            char recordingName[SELFRUN_RECORDING_FILE_NAME_LENGTH];
            recordingName[0] = '\0';
            if (_win_get_str(recordingName, sizeof(recordingName) - 2, "Enter name for recording (8 characters max, no extension):", 100, 100) == 0) {
                memset(&selfrunData, 0, sizeof(selfrunData));
                if (selfrunPrepareRecording(recordingName, fileList[selectedFileIndex], &selfrunData) == 0) {
                    ready = true;
                }
            }
        }
        fileNameListFree(&fileList, 0);
    }

    if (ready) {
        main_menu_hide(true);
        main_menu_destroy();
        gsound_background_stop();
        roll_set_seed(0xBEEFFEED);

        // NOTE: Uninline.
        main_reset_system();

        proto_dude_init("premade\\combat.gcd");
        main_load_new(selfrunData.mapFileName);
        selfrunRecordingLoop(&selfrunData);
        palette_fade_to(white_palette);

        // NOTE: Uninline.
        main_unload_new();

        // NOTE: Uninline.
        main_reset_system();

        main_menu_create();

        // NOTE: Uninline.
        main_selfrun_init();
    }
}

// 0x48109C
static void main_selfrun_play()
{
    // A switch to pick selfrun vs. intro video for screensaver:
    // - `false` - will play next selfrun recording
    // - `true` - will play intro video
    //
    // This value will alternate on every attempt, even if there are no selfrun
    // recordings.
    //
    // 0x5194EC
    static bool toggle = false;

    if (!toggle && main_selfrun_count > 0) {
        SelfrunData selfrunData;
        if (selfrunPreparePlayback(main_selfrun_list[main_selfrun_index], &selfrunData) == 0) {
            main_menu_hide(true);
            main_menu_destroy();
            gsound_background_stop();
            roll_set_seed(0xBEEFFEED);

            // NOTE: Uninline.
            main_reset_system();

            proto_dude_init("premade\\combat.gcd");
            main_load_new(selfrunData.mapFileName);
            selfrunPlaybackLoop(&selfrunData);
            palette_fade_to(white_palette);

            // NOTE: Uninline.
            main_unload_new();

            // NOTE: Uninline.
            main_reset_system();

            main_menu_create();
        }

        main_selfrun_index++;
        if (main_selfrun_index >= main_selfrun_count) {
            main_selfrun_index = 0;
        }
    } else {
        main_menu_hide(true);
        gmovie_play(MOVIE_INTRO, GAME_MOVIE_PAUSE_MUSIC);
    }

    toggle = 1 - toggle;
}

// 0x48118C
static void main_death_scene()
{
    art_flush();
    cycle_disable();
    gmouse_set_cursor(MOUSE_CURSOR_NONE);

    bool oldCursorIsHidden = mouse_hidden();
    if (oldCursorIsHidden) {
        mouse_show();
    }

    int deathWindowX = 0;
    int deathWindowY = 0;
    int win = windowCreate(deathWindowX,
        deathWindowY,
        DEATH_WINDOW_WIDTH,
        DEATH_WINDOW_HEIGHT,
        0,
        WINDOW_FLAG_0x04);
    if (win != -1) {
        do {
            unsigned char* windowBuffer = windowGetBuffer(win);
            if (windowBuffer == NULL) {
                break;
            }

            // DEATH.FRM
            CacheEntry* backgroundHandle;
            int fid = art_id(OBJ_TYPE_INTERFACE, 309, 0, 0, 0);
            unsigned char* background = art_ptr_lock_data(fid, 0, 0, &backgroundHandle);
            if (background == NULL) {
                break;
            }

            while (mouse_get_buttons() != 0) {
                _get_input();
            }

            kb_clear();
            inputEventQueueReset();

            blitBufferToBuffer(background, 640, 480, 640, windowBuffer, 640);
            art_ptr_unlock(backgroundHandle);

            const char* deathFileName = endgameGetDeathEndingFileName();

            int subtitles = 0;
            config_get_value(&game_config, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_SUBTITLES_KEY, &subtitles);
            if (subtitles != 0) {
                char text[512];
                if (mainDeathGrabTextFile(deathFileName, text) == 0) {
                    debugPrint("\n((ShowDeath)): %s\n", text);

                    short beginnings[WORD_WRAP_MAX_COUNT];
                    short count;
                    if (mainDeathWordWrap(text, 560, beginnings, &count) == 0) {
                        unsigned char* p = windowBuffer + 640 * (480 - fontGetLineHeight() * count - 8);
                        bufferFill(p - 602, 564, fontGetLineHeight() * count + 2, 640, 0);
                        p += 40;
                        for (int index = 0; index < count; index++) {
                            fontDrawText(p, text + beginnings[index], 560, 640, colorTable[32767]);
                            p += 640 * fontGetLineHeight();
                        }
                    }
                }
            }

            win_draw(win);

            loadColorTable("art\\intrface\\death.pal");
            palette_fade_to(cmap);

            main_death_voiceover_done = false;
            gsound_speech_callback_set(main_death_voiceover_callback);

            unsigned int delay;
            if (gsound_speech_play(deathFileName, 10, 14, 15) == -1) {
                delay = 3000;
            } else {
                delay = UINT_MAX;
            }

            gsound_speech_play_preloaded();

            unsigned int time = _get_time();
            int keyCode;
            do {
                keyCode = _get_input();
            } while (keyCode == -1 && !main_death_voiceover_done && getTicksSince(time) < delay);

            gsound_speech_callback_set(NULL);

            gsound_speech_stop();

            while (mouse_get_buttons() != 0) {
                _get_input();
            }

            if (keyCode == -1) {
                coreDelayProcessingEvents(500);
            }

            palette_fade_to(black_palette);
            loadColorTable("color.pal");
        } while (0);
        windowDestroy(win);
    }

    if (oldCursorIsHidden) {
        mouse_hide();
    }

    gmouse_set_cursor(MOUSE_CURSOR_ARROW);

    cycle_enable();
}

// 0x4814A8
static void main_death_voiceover_callback()
{
    main_death_voiceover_done = true;
}

// Read endgame subtitle.
//
// 0x4814B4
static int mainDeathGrabTextFile(const char* fileName, char* dest)
{
    const char* p = strrchr(fileName, '\\');
    if (p == NULL) {
        return -1;
    }

    char* language = NULL;
    if (!config_get_string(&game_config, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_LANGUAGE_KEY, &language)) {
        debugPrint("MAIN: Error grabing language for ending. Defaulting to english.\n");
        language = _aEnglish_2;
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
static int mainDeathWordWrap(char* text, int width, short* beginnings, short* count)
{
    while (true) {
        char* sep = strchr(text, ':');
        if (sep == NULL) {
            break;
        }

        if (sep - 1 < text) {
            break;
        }
        sep[0] = ' ';
        sep[-1] = ' ';
    }

    if (wordWrap(text, width, beginnings, count) == -1) {
        return -1;
    }

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
