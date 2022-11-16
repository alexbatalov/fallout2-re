#include "game/endgame.h"

#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "color.h"
#include "core.h"
#include "game/credits.h"
#include "game/cycle.h"
#include "db.h"
#include "game/bmpdlog.h"
#include "debug.h"
#include "draw.h"
#include "game/game.h"
#include "game/gconfig.h"
#include "game/gmouse.h"
#include "game/gmovie.h"
#include "game/gsound.h"
#include "game/map.h"
#include "memory.h"
#include "game/object.h"
#include "game/palette.h"
#include "game/pipboy.h"
#include "game/roll.h"
#include "game/stat.h"
#include "text_font.h"
#include "window_manager.h"
#include "word_wrap.h"
#include "worldmap.h"

// The maximum number of subtitle lines per slide.
#define ENDGAME_ENDING_MAX_SUBTITLES 50

#define ENDGAME_ENDING_WINDOW_WIDTH 640
#define ENDGAME_ENDING_WINDOW_HEIGHT 480

typedef struct EndgameDeathEnding {
    int gvar;
    int value;
    int worldAreaKnown;
    int worldAreaNotKnown;
    int min_level;
    int percentage;
    char voiceOverBaseName[16];

    // This flag denotes that the conditions for this ending is met and it was
    // selected as a candidate for final random selection.
    bool enabled;
} EndgameDeathEnding;

typedef struct EndgameEnding {
    int gvar;
    int value;
    int art_num;
    char voiceOverBaseName[12];
    int direction;
} EndgameEnding;

static void endgame_pan_desert(int direction, const char* narratorFileName);
static void endgame_display_image(int fid, const char* narratorFileName);
static int endgame_init();
static void endgame_exit();
static void endgame_load_voiceover(const char* fname);
static void endgame_play_voiceover();
static void endgame_stop_voiceover();
static void endgame_load_palette(int type, int id);
static void endgame_voiceover_callback();
static int endgame_load_subtitles(const char* filePath);
static void endgame_show_subtitles();
static void endgame_clear_subtitles();
static void endgame_movie_callback();
static void endgame_movie_bk_process();
static int endgame_load_slide_info();
static void endgame_unload_slide_info();
static int endgameSetupInit(int* percentage);

// TODO: Remove.
// 0x50B00C
char _aEnglish_2[] = ENGLISH;

// The number of lines in current subtitles file.
//
// It's used as a length for two arrays:
// - [endgame_subtitle_text]
// - [endgame_subtitle_times]
//
// This value does not exceed [ENDGAME_ENDING_SUBTITLES_CAPACITY].
//
// 0x518668
static int endgame_subtitle_count = 0;

// The number of characters in current subtitles file.
//
// This value is used to determine
//
// 0x51866C
static int endgame_subtitle_characters = 0;

// 0x518670
static int endgame_current_subtitle = 0;

// 0x518674
static int endgame_maybe_done = 0;

// enddeath.txt
//
// 0x518678
static EndgameDeathEnding* endDeathInfoList = NULL;

// The number of death endings in [endDeathInfoList] array.
//
// 0x51867C
static int maxEndDeathInfo = 0;

// Base file name for death ending.
//
// This value does not include extension.
//
// 0x570A90
static char endDeathSndChoice[40];

// This flag denotes whether speech sound was successfully loaded for
// the current slide.
//
// 0x570AB8
static bool endgame_voiceover_loaded;

// 0x570ABC
static char endgame_subtitle_path[MAX_PATH];

// The flag used to denote voice over speech for current slide has ended.
//
// 0x570BC0
static bool endgame_voiceover_done;

// endgame.txt
//
// 0x570BC4
static EndgameEnding* slides;

// The array of text lines in current subtitles file.
//
// The length is specified in [endgame_subtitle_count]. It's capacity
// is [ENDGAME_ENDING_SUBTITLES_CAPACITY].
//
// 0x570BC8
static char** endgame_subtitle_text;

// 0x570BCC
static bool endgame_do_subtitles;

// The flag used to denote voice over subtitles for current slide has ended.
//
// 0x570BD0
static bool endgame_subtitle_done;

// 0x570BD4
static bool endgame_map_enabled;

// 0x570BD8
static bool endgame_mouse_state;

// The number of endings in [slides] array.
//
// 0x570BDC
static int num_slides = 0;

// This flag denotes whether subtitles was successfully loaded for
// the current slide.
//
// 0x570BE0
static bool endgame_subtitle_loaded;

// Reference time is a timestamp when subtitle is first displayed.
//
// This value is used together with [endgame_subtitle_times] array to
// determine when next line needs to be displayed.
//
// 0x570BE4
static unsigned int endgame_subtitle_start_time;

// The array of timings for each line in current subtitles file.
//
// The length is specified in [endgame_subtitle_count]. It's capacity
// is [ENDGAME_ENDING_SUBTITLES_CAPACITY].
//
// 0x570BE8
static unsigned int* endgame_subtitle_times;

// Font that was current before endgame slideshow window was created.
//
// 0x570BEC
static int endgame_old_font;

// 0x570BF0
static unsigned char* endgame_window_buffer;

// 0x570BF4
static int endgame_window;

// 0x43F788
void endgame_slideshow()
{
    if (endgame_init() == -1) {
        return;
    }

    for (int index = 0; index < num_slides; index++) {
        EndgameEnding* ending = &(slides[index]);
        int value = game_get_global_var(ending->gvar);
        if (value == ending->value) {
            if (ending->art_num == 327) {
                endgame_pan_desert(ending->direction, ending->voiceOverBaseName);
            } else {
                int fid = art_id(OBJ_TYPE_INTERFACE, ending->art_num, 0, 0, 0);
                endgame_display_image(fid, ending->voiceOverBaseName);
            }
        }
    }

    endgame_exit();
}

// 0x43F810
void endgame_movie()
{
    gsound_background_stop();
    map_disable_bk_processes();
    palette_fade_to(black_palette);
    endgame_maybe_done = 0;
    tickersAdd(endgame_movie_bk_process);
    gsound_background_callback_set(endgame_movie_callback);
    gsound_background_play("akiss", 12, 14, 15);
    coreDelayProcessingEvents(3000);

    // NOTE: Result is ignored. I guess there was some kind of switch for male
    // vs. female ending, but it was not implemented.
    critterGetStat(obj_dude, STAT_GENDER);

    credits("credits.txt", -1, false);
    gsound_background_stop();
    gsound_background_callback_set(NULL);
    tickersRemove(endgame_movie_bk_process);
    gsound_background_stop();
    loadColorTable("color.pal");
    palette_fade_to(cmap);
    map_enable_bk_processes();
    endgameEndingHandleContinuePlaying();
}

// 0x43F8C4
int endgameEndingHandleContinuePlaying()
{
    bool isoWasEnabled = map_disable_bk_processes();

    bool gameMouseWasVisible;
    if (isoWasEnabled) {
        gameMouseWasVisible = gmouse_3d_is_on();
    } else {
        gameMouseWasVisible = false;
    }

    if (gameMouseWasVisible) {
        gmouse_3d_off();
    }

    bool oldCursorIsHidden = mouse_hidden();
    if (oldCursorIsHidden) {
        mouse_show();
    }

    int oldCursor = gmouse_get_cursor();
    gmouse_set_cursor(MOUSE_CURSOR_ARROW);

    int rc;

    MessageListItem messageListItem;
    messageListItem.num = 30;
    if (message_search(&misc_message_file, &messageListItem)) {
        rc = dialog_out(messageListItem.text, NULL, 0, 169, 117, colorTable[32328], NULL, colorTable[32328], DIALOG_BOX_YES_NO);
        if (rc == 0) {
            game_user_wants_to_quit = 2;
        }
    } else {
        rc = -1;
    }

    gmouse_set_cursor(oldCursor);
    if (oldCursorIsHidden) {
        mouse_hide();
    }

    if (gameMouseWasVisible) {
        gmouse_3d_on();
    }

    if (isoWasEnabled) {
        map_enable_bk_processes();
    }

    return rc;
}

// 0x43FBDC
static void endgame_pan_desert(int direction, const char* narratorFileName)
{
    int fid = art_id(OBJ_TYPE_INTERFACE, 327, 0, 0, 0);

    CacheEntry* backgroundHandle;
    Art* background = art_ptr_lock(fid, &backgroundHandle);
    if (background != NULL) {
        int width = art_frame_width(background, 0, 0);
        int height = art_frame_length(background, 0, 0);
        unsigned char* backgroundData = art_frame_data(background, 0, 0);
        bufferFill(endgame_window_buffer, ENDGAME_ENDING_WINDOW_WIDTH, ENDGAME_ENDING_WINDOW_HEIGHT, ENDGAME_ENDING_WINDOW_WIDTH, colorTable[0]);
        endgame_load_palette(6, 327);

        unsigned char palette[768];
        memcpy(palette, cmap, 768);

        palette_set_to(black_palette);
        endgame_load_voiceover(narratorFileName);

        // TODO: Unclear math.
        int v8 = width - 640;
        int v32 = v8 / 4;
        unsigned int v9 = 16 * v8 / v8;
        unsigned int v9_ = 16 * v8;

        if (endgame_voiceover_loaded) {
            unsigned int v10 = 1000 * gsound_speech_length_get();
            if (v10 > v9_ / 2) {
                v9 = (v10 + v9 * (v8 / 2)) / v8;
            }
        }

        int start;
        int end;
        if (direction == -1) {
            start = width - 640;
            end = 0;
        } else {
            start = 0;
            end = width - 640;
        }

        tickersDisable();

        bool subtitlesLoaded = false;

        unsigned int since = 0;
        while (start != end) {
            int v12 = 640 - v32;

            // TODO: Complex math, setup scene in debugger.
            if (getTicksSince(since) >= v9) {
                blitBufferToBuffer(backgroundData + start, ENDGAME_ENDING_WINDOW_WIDTH, ENDGAME_ENDING_WINDOW_HEIGHT, width, endgame_window_buffer, ENDGAME_ENDING_WINDOW_WIDTH);

                if (subtitlesLoaded) {
                    endgame_show_subtitles();
                }

                win_draw(endgame_window);

                since = _get_time();

                bool v14;
                double v31;
                if (start > v32) {
                    if (v12 > start) {
                        v14 = false;
                    } else {
                        int v28 = v32 - (start - v12);
                        v31 = (double)v28 / (double)v32;
                        v14 = true;
                    }
                } else {
                    v14 = true;
                    v31 = (double)start / (double)v32;
                }

                if (v14) {
                    unsigned char darkenedPalette[768];
                    for (int index = 0; index < 768; index++) {
                        darkenedPalette[index] = (unsigned char)trunc(palette[index] * v31);
                    }
                    palette_set_to(darkenedPalette);
                }

                start += direction;

                if (direction == 1 && (start == v32)) {
                    // NOTE: Uninline.
                    endgame_play_voiceover();
                    subtitlesLoaded = true;
                } else if (direction == -1 && (start == v12)) {
                    // NOTE: Uninline.
                    endgame_play_voiceover();
                    subtitlesLoaded = true;
                }
            }

            soundContinueAll();

            if (_get_input() != -1) {
                // NOTE: Uninline.
                endgame_stop_voiceover();
                break;
            }
        }

        tickersEnable();
        art_ptr_unlock(backgroundHandle);

        palette_fade_to(black_palette);
        bufferFill(endgame_window_buffer, ENDGAME_ENDING_WINDOW_WIDTH, ENDGAME_ENDING_WINDOW_HEIGHT, ENDGAME_ENDING_WINDOW_WIDTH, colorTable[0]);
        win_draw(endgame_window);
    }

    while (mouse_get_buttons() != 0) {
        _get_input();
    }
}

// 0x440004
static void endgame_display_image(int fid, const char* narratorFileName)
{
    CacheEntry* backgroundHandle;
    Art* background = art_ptr_lock(fid, &backgroundHandle);
    if (background == NULL) {
        return;
    }

    unsigned char* backgroundData = art_frame_data(background, 0, 0);
    if (backgroundData != NULL) {
        blitBufferToBuffer(backgroundData, ENDGAME_ENDING_WINDOW_WIDTH, ENDGAME_ENDING_WINDOW_HEIGHT, ENDGAME_ENDING_WINDOW_WIDTH, endgame_window_buffer, ENDGAME_ENDING_WINDOW_WIDTH);
        win_draw(endgame_window);

        endgame_load_palette(FID_TYPE(fid), fid & 0xFFF);

        endgame_load_voiceover(narratorFileName);

        unsigned int delay;
        if (endgame_subtitle_loaded || endgame_voiceover_loaded) {
            delay = UINT_MAX;
        } else {
            delay = 3000;
        }

        palette_fade_to(cmap);

        coreDelayProcessingEvents(500);

        // NOTE: Uninline.
        endgame_play_voiceover();

        unsigned int referenceTime = _get_time();
        tickersDisable();

        int keyCode;
        while (true) {
            keyCode = _get_input();
            if (keyCode != -1) {
                break;
            }

            if (endgame_voiceover_done) {
                break;
            }

            if (endgame_subtitle_done) {
                break;
            }

            if (getTicksSince(referenceTime) > delay) {
                break;
            }

            blitBufferToBuffer(backgroundData, ENDGAME_ENDING_WINDOW_WIDTH, ENDGAME_ENDING_WINDOW_HEIGHT, ENDGAME_ENDING_WINDOW_WIDTH, endgame_window_buffer, ENDGAME_ENDING_WINDOW_WIDTH);
            endgame_show_subtitles();
            win_draw(endgame_window);
            soundContinueAll();
        }

        tickersEnable();
        gsound_speech_stop();
        endgame_clear_subtitles();

        endgame_voiceover_loaded = false;
        endgame_subtitle_loaded = false;

        if (keyCode == -1) {
            coreDelayProcessingEvents(500);
        }

        palette_fade_to(black_palette);

        while (mouse_get_buttons() != 0) {
            _get_input();
        }
    }

    art_ptr_unlock(backgroundHandle);
}

// 0x43F99C
static int endgame_init()
{
    if (endgame_load_slide_info() != 0) {
        return -1;
    }

    gsound_background_stop();

    endgame_map_enabled = map_disable_bk_processes();

    cycle_disable();
    gmouse_set_cursor(MOUSE_CURSOR_NONE);

    bool oldCursorIsHidden = mouse_hidden();
    endgame_mouse_state = oldCursorIsHidden == 0;

    if (oldCursorIsHidden) {
        mouse_show();
    }

    endgame_old_font = fontGetCurrent();
    fontSetCurrent(101);

    palette_fade_to(black_palette);

    int windowEndgameEndingX = 0;
    int windowEndgameEndingY = 0;
    endgame_window = windowCreate(windowEndgameEndingX,
        windowEndgameEndingY,
        ENDGAME_ENDING_WINDOW_WIDTH,
        ENDGAME_ENDING_WINDOW_HEIGHT,
        colorTable[0],
        WINDOW_FLAG_0x04);
    if (endgame_window == -1) {
        return -1;
    }

    endgame_window_buffer = windowGetBuffer(endgame_window);
    if (endgame_window_buffer == NULL) {
        return -1;
    }

    cycle_disable();

    gsound_speech_callback_set(endgame_voiceover_callback);

    endgame_do_subtitles = false;
    configGetBool(&game_config, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_SUBTITLES_KEY, &endgame_do_subtitles);
    if (!endgame_do_subtitles) {
        return 0;
    }

    char* language;
    if (!config_get_string(&game_config, GAME_CONFIG_SYSTEM_KEY, GAME_CONFIG_LANGUAGE_KEY, &language)) {
        endgame_do_subtitles = false;
        return 0;
    }

    sprintf(endgame_subtitle_path, "text\\%s\\cuts\\", language);

    endgame_subtitle_text = (char**)internal_malloc(sizeof(*endgame_subtitle_text) * ENDGAME_ENDING_MAX_SUBTITLES);
    if (endgame_subtitle_text == NULL) {
        endgame_do_subtitles = false;
        return 0;
    }

    for (int index = 0; index < ENDGAME_ENDING_MAX_SUBTITLES; index++) {
        endgame_subtitle_text[index] = NULL;
    }

    endgame_subtitle_times = (unsigned int*)internal_malloc(sizeof(*endgame_subtitle_times) * ENDGAME_ENDING_MAX_SUBTITLES);
    if (endgame_subtitle_times == NULL) {
        internal_free(endgame_subtitle_text);
        endgame_do_subtitles = false;
        return 0;
    }

    return 0;
}

// 0x43FB28
static void endgame_exit()
{
    if (endgame_do_subtitles) {
        endgame_clear_subtitles();

        internal_free(endgame_subtitle_times);
        internal_free(endgame_subtitle_text);

        endgame_subtitle_text = NULL;
        endgame_do_subtitles = false;
    }

    // NOTE: Uninline.
    endgame_unload_slide_info();

    fontSetCurrent(endgame_old_font);

    gsound_speech_callback_set(NULL);
    windowDestroy(endgame_window);

    if (!endgame_mouse_state) {
        mouse_hide();
    }

    gmouse_set_cursor(MOUSE_CURSOR_ARROW);

    loadColorTable("color.pal");
    palette_fade_to(cmap);

    cycle_enable();

    if (endgame_map_enabled) {
        map_enable_bk_processes();
    }
}

// 0x4401A0
static void endgame_load_voiceover(const char* fileBaseName)
{
    char path[MAX_PATH];

    // NOTE: Uninline.
    endgame_stop_voiceover();

    endgame_voiceover_loaded = false;
    endgame_subtitle_loaded = false;

    // Build speech file path.
    sprintf(path, "%s%s", "narrator\\", fileBaseName);

    if (gsound_speech_play(path, 10, 14, 15) != -1) {
        endgame_voiceover_loaded = true;
    }

    if (endgame_do_subtitles) {
        // Build subtitles file path.
        sprintf(path, "%s%s.txt", endgame_subtitle_path, fileBaseName);

        if (endgame_load_subtitles(path) != 0) {
            return;
        }

        double durationPerCharacter;
        if (endgame_voiceover_loaded) {
            durationPerCharacter = (double)gsound_speech_length_get() / (double)endgame_subtitle_characters;
        } else {
            durationPerCharacter = 0.08;
        }

        unsigned int timing = 0;
        for (int index = 0; index < endgame_subtitle_count; index++) {
            double charactersCount = strlen(endgame_subtitle_text[index]);
            // NOTE: There is floating point math at 0x4402E6 used to add
            // timing.
            timing += (unsigned int)trunc(charactersCount * durationPerCharacter * 1000.0);
            endgame_subtitle_times[index] = timing;
        }

        endgame_subtitle_loaded = true;
    }
}

// NOTE: This function was inlined at every call site.
//
// 0x440324
static void endgame_play_voiceover()
{
    endgame_subtitle_done = false;
    endgame_voiceover_done = false;

    if (endgame_voiceover_loaded) {
        gsound_speech_play_preloaded();
    }

    if (endgame_subtitle_loaded) {
        endgame_subtitle_start_time = _get_time();
    }
}

// NOTE: This function was inlined at every call site.
//
// 0x44035C
static void endgame_stop_voiceover()
{
    gsound_speech_stop();
    endgame_clear_subtitles();
    endgame_voiceover_loaded = false;
    endgame_subtitle_loaded = false;
}

// 0x440378
static void endgame_load_palette(int type, int id)
{
    char fileName[13];
    if (art_get_base_name(type, id, fileName) != 0) {
        return;
    }

    // Remove extension from file name.
    char* pch = strrchr(fileName, '.');
    if (pch != NULL) {
        *pch = '\0';
    }

    if (strlen(fileName) <= 8) {
        char path[MAX_PATH];
        sprintf(path, "%s\\%s.pal", "art\\intrface", fileName);
        loadColorTable(path);
    }
}

// 0x4403F0
static void endgame_voiceover_callback()
{
    endgame_voiceover_done = true;
}

// Loads subtitles file.
//
// 0x4403FC
static int endgame_load_subtitles(const char* filePath)
{
    endgame_clear_subtitles();

    File* stream = fileOpen(filePath, "rt");
    if (stream == NULL) {
        return -1;
    }

    // FIXME: There is at least one subtitle for Arroyo ending (nar_ar1) that
    // does not fit into this buffer.
    char string[256];
    while (fileReadString(string, sizeof(string), stream)) {
        char* pch;

        // Find and clamp string at EOL.
        pch = strchr(string, '\n');
        if (pch != NULL) {
            *pch = '\0';
        }

        // Find separator. The value before separator is ignored (as opposed to
        // movie subtitles, where the value before separator is a timing).
        pch = strchr(string, ':');
        if (pch != NULL) {
            if (endgame_subtitle_count < ENDGAME_ENDING_MAX_SUBTITLES) {
                endgame_subtitle_text[endgame_subtitle_count] = internal_strdup(pch + 1);
                endgame_subtitle_count++;
                endgame_subtitle_characters += strlen(pch + 1);
            }
        }
    }

    fileClose(stream);

    return 0;
}

// Refreshes subtitles.
//
// 0x4404EC
static void endgame_show_subtitles()
{
    if (endgame_subtitle_count <= endgame_current_subtitle) {
        if (endgame_subtitle_loaded) {
            endgame_subtitle_done = true;
        }
        return;
    }

    if (getTicksSince(endgame_subtitle_start_time) > endgame_subtitle_times[endgame_current_subtitle]) {
        endgame_current_subtitle++;
        return;
    }

    char* text = endgame_subtitle_text[endgame_current_subtitle];
    if (text == NULL) {
        return;
    }

    short beginnings[WORD_WRAP_MAX_COUNT];
    short count;
    if (wordWrap(text, 540, beginnings, &count) != 0) {
        return;
    }

    int height = fontGetLineHeight();
    int y = 480 - height * count;

    for (int index = 0; index < count - 1; index++) {
        char* beginning = text + beginnings[index];
        char* ending = text + beginnings[index + 1];

        if (ending[-1] == ' ') {
            ending--;
        }

        char c = *ending;
        *ending = '\0';

        int width = fontGetStringWidth(beginning);
        int x = (640 - width) / 2;
        bufferFill(endgame_window_buffer + 640 * y + x, width, height, 640, colorTable[0]);
        fontDrawText(endgame_window_buffer + 640 * y + x, beginning, width, 640, colorTable[32767]);

        *ending = c;

        y += height;
    }
}

// 0x4406CC
static void endgame_clear_subtitles()
{
    for (int index = 0; index < endgame_subtitle_count; index++) {
        if (endgame_subtitle_text[index] != NULL) {
            internal_free(endgame_subtitle_text[index]);
            endgame_subtitle_text[index] = NULL;
        }
    }

    endgame_current_subtitle = 0;
    endgame_subtitle_characters = 0;
    endgame_subtitle_count = 0;
}

// 0x440728
static void endgame_movie_callback()
{
    endgame_maybe_done = 1;
}

// 0x440734
static void endgame_movie_bk_process()
{
    if (endgame_maybe_done) {
        gsound_background_play("10labone", 11, 14, 16);
        gsound_background_callback_set(NULL);
        tickersRemove(endgame_movie_bk_process);
    }
}

// 0x440770
static int endgame_load_slide_info()
{
    File* stream;
    char str[256];
    char *ch, *tok;
    const char* delim = " \t,";
    EndgameEnding entry;
    EndgameEnding* entries;
    int narrator_file_len;

    if (slides != NULL) {
        internal_free(slides);
        slides = NULL;
    }

    num_slides = 0;

    stream = fileOpen("data\\endgame.txt", "rt");
    if (stream == NULL) {
        return -1;
    }

    while (fileReadString(str, sizeof(str), stream)) {
        ch = str;
        while (isspace(*ch)) {
            ch++;
        }

        if (*ch == '#') {
            continue;
        }

        tok = strtok(ch, delim);
        if (tok == NULL) {
            continue;
        }

        entry.gvar = atoi(tok);

        tok = strtok(NULL, delim);
        if (tok == NULL) {
            continue;
        }

        entry.value = atoi(tok);

        tok = strtok(NULL, delim);
        if (tok == NULL) {
            continue;
        }

        entry.art_num = atoi(tok);

        tok = strtok(NULL, delim);
        if (tok == NULL) {
            continue;
        }

        strcpy(entry.voiceOverBaseName, tok);

        narrator_file_len = strlen(entry.voiceOverBaseName);
        if (isspace(entry.voiceOverBaseName[narrator_file_len - 1])) {
            entry.voiceOverBaseName[narrator_file_len - 1] = '\0';
        }

        tok = strtok(NULL, delim);
        if (tok != NULL) {
            entry.direction = atoi(tok);
        } else {
            entry.direction = 1;
        }

        entries = (EndgameEnding*)internal_realloc(slides, sizeof(*entries) * (num_slides + 1));
        if (entries == NULL) {
            goto err;
        }

        memcpy(&(entries[num_slides]), &entry, sizeof(entry));

        slides = entries;
        num_slides++;
    }

    fileClose(stream);

    return 0;

err:

    fileClose(stream);

    return -1;
}

// NOTE: There are no references to this function. It was inlined.
//
// 0x44095C
static void endgame_unload_slide_info()
{
    if (slides != NULL) {
        internal_free(slides);
        slides = NULL;
    }

    num_slides = 0;
}

// endgameDeathEndingInit
// 0x440984
int endgameDeathEndingInit()
{
    File* stream;
    char str[256];
    char* ch;
    const char* delim = " \t,";
    char* tok;
    EndgameDeathEnding entry;
    EndgameDeathEnding* entries;
    int narrator_file_len;

    strcpy(endDeathSndChoice, "narrator\\nar_5");

    stream = fileOpen("data\\enddeath.txt", "rt");
    if (stream == NULL) {
        return -1;
    }

    while (fileReadString(str, 256, stream)) {
        ch = str;
        while (isspace(*ch)) {
            ch++;
        }

        if (*ch == '#') {
            continue;
        }

        tok = strtok(ch, delim);
        if (tok == NULL) {
            continue;
        }

        entry.gvar = atoi(tok);

        tok = strtok(NULL, delim);
        if (tok == NULL) {
            continue;
        }

        entry.value = atoi(tok);

        tok = strtok(NULL, delim);
        if (tok == NULL) {
            continue;
        }

        entry.worldAreaKnown = atoi(tok);

        tok = strtok(NULL, delim);
        if (tok == NULL) {
            continue;
        }

        entry.worldAreaNotKnown = atoi(tok);

        tok = strtok(NULL, delim);
        if (tok == NULL) {
            continue;
        }

        entry.min_level = atoi(tok);

        tok = strtok(NULL, delim);
        if (tok == NULL) {
            continue;
        }

        entry.percentage = atoi(tok);

        tok = strtok(NULL, delim);
        if (tok == NULL) {
            continue;
        }

        // this code is slightly different from the original, but does the same thing
        narrator_file_len = strlen(tok);
        strncpy(entry.voiceOverBaseName, tok, narrator_file_len);

        entry.enabled = false;

        if (isspace(entry.voiceOverBaseName[narrator_file_len - 1])) {
            entry.voiceOverBaseName[narrator_file_len - 1] = '\0';
        }

        entries = (EndgameDeathEnding*)internal_realloc(endDeathInfoList, sizeof(*entries) * (maxEndDeathInfo + 1));
        if (entries == NULL) {
            goto err;
        }

        memcpy(&(entries[maxEndDeathInfo]), &entry, sizeof(entry));

        endDeathInfoList = entries;
        maxEndDeathInfo++;
    }

    fileClose(stream);

    return 0;

err:

    fileClose(stream);

    return -1;
}

// 0x440BA8
int endgameDeathEndingExit()
{
    if (endDeathInfoList != NULL) {
        internal_free(endDeathInfoList);
        endDeathInfoList = NULL;

        maxEndDeathInfo = 0;
    }

    return 0;
}

// endgameSetupDeathEnding
// 0x440BD0
void endgameSetupDeathEnding(int reason)
{
    if (!maxEndDeathInfo) {
        debugPrint("\nError: endgameSetupDeathEnding: No endgame death info!");
        return;
    }

    // Build death ending file path.
    strcpy(endDeathSndChoice, "narrator\\");

    int percentage = 0;
    endgameSetupInit(&percentage);

    int selectedEnding = 0;
    bool specialEndingSelected = false;

    switch (reason) {
    case ENDGAME_DEATH_ENDING_REASON_DEATH:
        if (game_get_global_var(GVAR_MODOC_SHITTY_DEATH) != 0) {
            selectedEnding = 12;
            specialEndingSelected = true;
        }
        break;
    case ENDGAME_DEATH_ENDING_REASON_TIMEOUT:
        gmovie_play(MOVIE_TIMEOUT, GAME_MOVIE_FADE_IN | GAME_MOVIE_FADE_OUT | GAME_MOVIE_PAUSE_MUSIC);
        break;
    }

    if (!specialEndingSelected) {
        int chance = roll_random(0, percentage);

        int accum = 0;
        for (int index = 0; index < maxEndDeathInfo; index++) {
            EndgameDeathEnding* deathEnding = &(endDeathInfoList[index]);

            if (deathEnding->enabled) {
                accum += deathEnding->percentage;
                if (accum >= chance) {
                    break;
                }
                selectedEnding++;
            }
        }
    }

    EndgameDeathEnding* deathEnding = &(endDeathInfoList[selectedEnding]);

    strcat(endDeathSndChoice, deathEnding->voiceOverBaseName);

    debugPrint("\nendgameSetupDeathEnding: Death Filename Picked: %s", endDeathSndChoice);
}

// Validates conditions imposed by death endings.
//
// Upon return [percentage] is set as a sum of all valid endings' percentages.
// Always returns 0.
//
// 0x440CF4
static int endgameSetupInit(int* percentage)
{
    *percentage = 0;

    for (int index = 0; index < maxEndDeathInfo; index++) {
        EndgameDeathEnding* deathEnding = &(endDeathInfoList[index]);

        deathEnding->enabled = false;

        if (deathEnding->gvar != -1) {
            if (game_get_global_var(deathEnding->gvar) >= deathEnding->value) {
                continue;
            }
        }

        if (deathEnding->worldAreaKnown != -1) {
            if (!wmAreaIsKnown(deathEnding->worldAreaKnown)) {
                continue;
            }
        }

        if (deathEnding->worldAreaNotKnown != -1) {
            if (wmAreaIsKnown(deathEnding->worldAreaNotKnown)) {
                continue;
            }
        }

        if (pcGetStat(PC_STAT_LEVEL) < deathEnding->min_level) {
            continue;
        }

        deathEnding->enabled = true;

        *percentage += deathEnding->percentage;
    }

    return 0;
}

// Returns file name for voice over for death ending.
//
// This path does not include extension.
//
// 0x440D8C
char* endgameGetDeathEndingFileName()
{
    if (maxEndDeathInfo == 0) {
        debugPrint("\nError: endgameSetupDeathEnding: No endgame death info!");
        strcpy(endDeathSndChoice, "narrator\\nar_4");
    }

    debugPrint("\nendgameSetupDeathEnding: Death Filename: %s", endDeathSndChoice);

    return endDeathSndChoice;
}
