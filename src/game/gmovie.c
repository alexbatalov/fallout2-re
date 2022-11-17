#include "game/gmovie.h"

#include <stdio.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "int/window.h"
#include "plib/color/color.h"
#include "plib/gnw/input.h"
#include "game/cycle.h"
#include "plib/gnw/debug.h"
#include "game/game.h"
#include "game/gconfig.h"
#include "game/gmouse.h"
#include "game/gsound.h"
#include "int/movie.h"
#include "game/moviefx.h"
#include "game/palette.h"
#include "plib/gnw/text.h"
#include "plib/gnw/gnw.h"
#include "plib/gnw/svga.h"

#define GAME_MOVIE_WINDOW_WIDTH 640
#define GAME_MOVIE_WINDOW_HEIGHT 480

static char* gmovie_subtitle_func(char* movieFilePath);

// 0x518DA0
static const char* movie_list[MOVIE_COUNT] = {
    "iplogo.mve",
    "intro.mve",
    "elder.mve",
    "vsuit.mve",
    "afailed.mve",
    "adestroy.mve",
    "car.mve",
    "cartucci.mve",
    "timeout.mve",
    "tanker.mve",
    "enclave.mve",
    "derrick.mve",
    "artimer1.mve",
    "artimer2.mve",
    "artimer3.mve",
    "artimer4.mve",
    "credits.mve",
};

// 0x518DE4
static const char* subtitlePalList[MOVIE_COUNT] = {
    NULL,
    "art\\cuts\\introsub.pal",
    "art\\cuts\\eldersub.pal",
    NULL,
    "art\\cuts\\artmrsub.pal",
    NULL,
    NULL,
    NULL,
    "art\\cuts\\artmrsub.pal",
    NULL,
    NULL,
    NULL,
    "art\\cuts\\artmrsub.pal",
    "art\\cuts\\artmrsub.pal",
    "art\\cuts\\artmrsub.pal",
    "art\\cuts\\artmrsub.pal",
    "art\\cuts\\crdtssub.pal",
};

// 0x518E28
static bool gmMovieIsPlaying = false;

// 0x518E2C
static bool gmPaletteWasFaded = false;

// 0x596C78
static unsigned char gmovie_played_list[MOVIE_COUNT];

// gmovie_init
// 0x44E5C0
int gmovie_init()
{
    int v1 = 0;
    if (gsound_background_is_enabled()) {
        v1 = gsound_background_volume_get();
    }

    movieSetVolume(v1);

    movieSetSubtitleFunc(gmovie_subtitle_func);

    memset(gmovie_played_list, 0, sizeof(gmovie_played_list));

    gmMovieIsPlaying = false;
    gmPaletteWasFaded = false;

    return 0;
}

// 0x44E60C
void gmovie_reset()
{
    memset(gmovie_played_list, 0, sizeof(gmovie_played_list));

    gmMovieIsPlaying = false;
    gmPaletteWasFaded = false;
}

// 0x44E638
int gmovie_load(File* stream)
{
    if (db_fread(gmovie_played_list, sizeof(*gmovie_played_list), MOVIE_COUNT, stream) != MOVIE_COUNT) {
        return -1;
    }

    return 0;
}

// 0x44E664
int gmovie_save(File* stream)
{
    if (db_fwrite(gmovie_played_list, sizeof(*gmovie_played_list), MOVIE_COUNT, stream) != MOVIE_COUNT) {
        return -1;
    }

    return 0;
}

// gmovie_play
// 0x44E690
int gmovie_play(int movie, int flags)
{
    gmMovieIsPlaying = true;

    const char* movieFileName = movie_list[movie];
    debug_printf("\nPlaying movie: %s\n", movieFileName);

    char* language;
    if (!config_get_string(&game_config, GAME_CONFIG_SYSTEM_KEY, GAME_CONFIG_LANGUAGE_KEY, &language)) {
        debug_printf("\ngmovie_play() - Error: Unable to determine language!\n");
        gmMovieIsPlaying = false;
        return -1;
    }

    char movieFilePath[MAX_PATH];
    int movieFileSize;
    bool movieFound = false;

    if (stricmp(language, ENGLISH) != 0) {
        sprintf(movieFilePath, "art\\%s\\cuts\\%s", language, movie_list[movie]);
        movieFound = db_dir_entry(movieFilePath, &movieFileSize) == 0;
    }

    if (!movieFound) {
        sprintf(movieFilePath, "art\\cuts\\%s", movie_list[movie]);
        movieFound = db_dir_entry(movieFilePath, &movieFileSize) == 0;
    }

    if (!movieFound) {
        debug_printf("\ngmovie_play() - Error: Unable to open %s\n", movie_list[movie]);
        gmMovieIsPlaying = false;
        return -1;
    }

    if ((flags & GAME_MOVIE_FADE_IN) != 0) {
        palette_fade_to(black_palette);
        gmPaletteWasFaded = true;
    }

    int gameMovieWindowX = 0;
    int gameMovieWindowY = 0;
    int win = win_add(gameMovieWindowX,
        gameMovieWindowY,
        GAME_MOVIE_WINDOW_WIDTH,
        GAME_MOVIE_WINDOW_HEIGHT,
        0,
        WINDOW_FLAG_0x10);
    if (win == -1) {
        gmMovieIsPlaying = false;
        return -1;
    }

    if ((flags & GAME_MOVIE_STOP_MUSIC) != 0) {
        gsound_background_stop();
    } else if ((flags & GAME_MOVIE_PAUSE_MUSIC) != 0) {
        gsound_background_pause();
    }

    win_draw(win);

    bool subtitlesEnabled = false;
    int v1 = 4;
    configGetBool(&game_config, GAME_CONFIG_PREFERENCES_KEY, GAME_CONFIG_SUBTITLES_KEY, &subtitlesEnabled);
    if (subtitlesEnabled) {
        char* subtitlesFilePath = gmovie_subtitle_func(movieFilePath);

        int subtitlesFileSize;
        if (db_dir_entry(subtitlesFilePath, &subtitlesFileSize) == 0) {
            v1 = 12;
        } else {
            subtitlesEnabled = false;
        }
    }

    movieSetFlags(v1);

    int oldTextColor;
    int oldFont;
    if (subtitlesEnabled) {
        const char* subtitlesPaletteFilePath;
        if (subtitlePalList[movie] != NULL) {
            subtitlesPaletteFilePath = subtitlePalList[movie];
        } else {
            subtitlesPaletteFilePath = "art\\cuts\\subtitle.pal";
        }

        loadColorTable(subtitlesPaletteFilePath);

        oldTextColor = windowGetTextColor();
        windowSetTextColor(1.0, 1.0, 1.0);

        oldFont = text_curr();
        windowSetFont(101);
    }

    bool cursorWasHidden = mouse_hidden();
    if (cursorWasHidden) {
        gmouse_set_cursor(MOUSE_CURSOR_NONE);
        mouse_show();
    }

    while (mouse_get_buttons() != 0) {
        mouse_info();
    }

    mouse_hide();
    cycle_disable();

    moviefx_start(movieFilePath);

    zero_vid_mem();
    movieRun(win, movieFilePath);

    int v11 = 0;
    int buttons;
    do {
        if (!moviePlaying() || game_user_wants_to_quit || get_input() != -1) {
            break;
        }

        int x;
        int y;
        mouse_get_raw_state(&x, &y, &buttons);

        v11 |= buttons;
    } while (((v11 & 1) == 0 && (v11 & 2) == 0) || (buttons & 1) != 0 || (buttons & 2) != 0);

    movieStop();
    moviefx_stop();
    movieUpdate();
    palette_set_to(black_palette);

    gmovie_played_list[movie] = 1;

    cycle_enable();

    gmouse_set_cursor(MOUSE_CURSOR_ARROW);

    if (!cursorWasHidden) {
        mouse_show();
    }

    if (subtitlesEnabled) {
        loadColorTable("color.pal");

        windowSetFont(oldFont);

        float r = (float)((Color2RGB(oldTextColor) & 0x7C00) >> 10) / 31.0f;
        float g = (float)((Color2RGB(oldTextColor) & 0x3E0) >> 5) / 31.0f;
        float b = (float)(Color2RGB(oldTextColor) & 0x1F) / 31.0f;
        windowSetTextColor(r, g, b);
    }

    win_delete(win);

    if ((flags & GAME_MOVIE_PAUSE_MUSIC) != 0) {
        gsound_background_unpause();
    }

    if ((flags & GAME_MOVIE_FADE_OUT) != 0) {
        if (!subtitlesEnabled) {
            loadColorTable("color.pal");
        }

        palette_fade_to(cmap);
        gmPaletteWasFaded = false;
    }

    gmMovieIsPlaying = false;
    return 0;
}

// 0x44EAE4
void gmPaletteFinish()
{
    if (gmPaletteWasFaded) {
        palette_fade_to(cmap);
        gmPaletteWasFaded = false;
    }
}

// 0x44EB04
bool gmovie_has_been_played(int movie)
{
    return gmovie_played_list[movie] == 1;
}

// 0x44EB14
bool gmovieIsPlaying()
{
    return gmMovieIsPlaying;
}

// 0x44EB1C
static char* gmovie_subtitle_func(char* movieFilePath)
{
    // 0x596C89
    static char full_path[MAX_PATH];

    char* language;
    config_get_string(&game_config, GAME_CONFIG_SYSTEM_KEY, GAME_CONFIG_LANGUAGE_KEY, &language);

    char* path = movieFilePath;

    char* separator = strrchr(path, '\\');
    if (separator != NULL) {
        path = separator + 1;
    }

    sprintf(full_path, "text\\%s\\cuts\\%s", language, path);

    char* pch = strrchr(full_path, '.');
    if (*pch != '\0') {
        *pch = '\0';
    }

    strcpy(full_path + strlen(full_path), ".SVE");

    return full_path;
}
