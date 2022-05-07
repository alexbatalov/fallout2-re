#include "movie_effect.h"

#include "config.h"
#include "debug.h"
#include "memory.h"
#include "movie.h"
#include "palette.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 0x5195F0
bool gMovieEffectsInitialized = false;

// 0x5195F4
MovieEffect* gMovieEffectHead = NULL;

// 0x638EC4
unsigned char byte_638EC4[768];

// 0x6391C4
bool dword_6391C4;

// 0x487CC0
int movieEffectsInit()
{
    if (gMovieEffectsInitialized) {
        return -1;
    }

    memset(byte_638EC4, 0, sizeof(byte_638EC4));

    gMovieEffectsInitialized = true;

    return 0;
}

// 0x487CF4
void movieEffectsReset()
{
    if (!gMovieEffectsInitialized) {
        return;
    }

    movieSetPaletteProc(NULL);
    _movieSetPaletteFunc(NULL);
    movieEffectsClear();

    dword_6391C4 = false;

    memset(byte_638EC4, 0, sizeof(byte_638EC4));
}

// 0x487D30
void movieEffectsExit()
{
    if (!gMovieEffectsInitialized) {
        return;
    }

    movieSetPaletteProc(NULL);
    _movieSetPaletteFunc(NULL);
    movieEffectsClear();

    dword_6391C4 = false;

    memset(byte_638EC4, 0, sizeof(byte_638EC4));
}

// 0x487D7C
int movieEffectsLoad(const char* filePath)
{
    if (!gMovieEffectsInitialized) {
        return -1;
    }

    movieSetPaletteProc(NULL);
    _movieSetPaletteFunc(NULL);
    movieEffectsClear();
    dword_6391C4 = false;
    memset(byte_638EC4, 0, sizeof(byte_638EC4));

    if (filePath == NULL) {
        return -1;
    }

    Config config;
    if (!configInit(&config)) {
        return -1;
    }

    int rc = -1;

    char path[FILENAME_MAX];
    strcpy(path, filePath);

    char* pch = strrchr(path, '.');
    if (pch != NULL) {
        *pch = '\0';
    }

    strcpy(path + strlen(path), ".cfg");

    if (!configRead(&config, path, true)) {
        goto out;
    }

    int movieEffectsLength;
    if (!configGetInt(&config, "info", "total_effects", &movieEffectsLength)) {
        goto out;
    }

    int* movieEffectFrameList = internal_malloc(sizeof(*movieEffectFrameList) * movieEffectsLength);
    if (movieEffectFrameList == NULL) {
        goto out;
    }

    bool frameListRead;
    if (movieEffectsLength >= 2) {
        frameListRead = configGetIntList(&config, "info", "effect_frames", movieEffectFrameList, movieEffectsLength);
    } else {
        frameListRead = configGetInt(&config, "info", "effect_frames", &(movieEffectFrameList[0]));
    }

    if (frameListRead) {
        int movieEffectsCreated = 0;
        for (int index = 0; index < movieEffectsLength; index++) {
            char section[20];
            itoa(movieEffectFrameList[index], section, 10);

            char* fadeTypeString;
            if (!configGetString(&config, section, "fade_type", &fadeTypeString)) {
                continue;
            }

            int fadeType = MOVIE_EFFECT_TYPE_NONE;
            if (stricmp(fadeTypeString, "in") == 0) {
                fadeType = MOVIE_EFFECT_TYPE_FADE_IN;
            } else if (stricmp(fadeTypeString, "out") == 0) {
                fadeType = MOVIE_EFFECT_TYPE_FADE_OUT;
            }

            if (fadeType == MOVIE_EFFECT_TYPE_NONE) {
                continue;
            }

            int fadeColor[3];
            if (!configGetIntList(&config, section, "fade_color", fadeColor, 3)) {
                continue;
            }

            int steps;
            if (!configGetInt(&config, section, "fade_steps", &steps)) {
                continue;
            }

            MovieEffect* movieEffect = internal_malloc(sizeof(*movieEffect));
            if (movieEffect == NULL) {
                continue;
            }

            memset(movieEffect, 0, sizeof(*movieEffect));
            movieEffect->startFrame = movieEffectFrameList[index];
            movieEffect->endFrame = movieEffect->startFrame + steps - 1;
            movieEffect->steps = steps;
            movieEffect->fadeType = fadeType & 0xFF;
            movieEffect->r = fadeColor[0] & 0xFF;
            movieEffect->g = fadeColor[1] & 0xFF;
            movieEffect->b = fadeColor[2] & 0xFF;

            if (movieEffect->startFrame <= 1) {
                dword_6391C4 = true;
            }

            movieEffect->next = gMovieEffectHead;
            gMovieEffectHead = movieEffect;

            movieEffectsCreated++;
        }

        if (movieEffectsCreated != 0) {
            movieSetPaletteProc(_moviefx_callback_func);
            _movieSetPaletteFunc(_moviefx_palette_func);
            rc = 0;
        }
    }

    internal_free(movieEffectFrameList);

out:

    configFree(&config);

    return rc;
}

// 0x4880F0
void _moviefx_stop()
{
    if (!gMovieEffectsInitialized) {
        return;
    }

    movieSetPaletteProc(NULL);
    _movieSetPaletteFunc(NULL);

    movieEffectsClear();

    dword_6391C4 = false;
    memset(byte_638EC4, 0, sizeof(byte_638EC4));
}

// 0x488144
void _moviefx_callback_func(int frame)
{
    MovieEffect* movieEffect = gMovieEffectHead;
    while (movieEffect != NULL) {
        if (frame >= movieEffect->startFrame && frame <= movieEffect->endFrame) {
            break;
        }
        movieEffect = movieEffect->next;
    }

    if (movieEffect != NULL) {
        unsigned char palette[768];
        int step = frame - movieEffect->startFrame + 1;

        if (movieEffect->fadeType == MOVIE_EFFECT_TYPE_FADE_IN) {
            for (int index = 0; index < 256; index++) {
                palette[index * 3] = movieEffect->r - (step * (movieEffect->r - byte_638EC4[index * 3]) / movieEffect->steps);
                palette[index * 3 + 1] = movieEffect->g - (step * (movieEffect->g - byte_638EC4[index * 3 + 1]) / movieEffect->steps);
                palette[index * 3 + 2] = movieEffect->b - (step * (movieEffect->b - byte_638EC4[index * 3 + 2]) / movieEffect->steps);
            }
        } else {
            for (int index = 0; index < 256; index++) {
                palette[index * 3] = byte_638EC4[index * 3] - (step * (byte_638EC4[index * 3] - movieEffect->r) / movieEffect->steps);
                palette[index * 3 + 1] = byte_638EC4[index * 3 + 1] - (step * (byte_638EC4[index * 3 + 1] - movieEffect->g) / movieEffect->steps);
                palette[index * 3 + 2] = byte_638EC4[index * 3 + 2] - (step * (byte_638EC4[index * 3 + 2] - movieEffect->b) / movieEffect->steps);
            }
        }

        paletteSetEntries(palette);
    }

    dword_6391C4 = movieEffect != NULL;
}

// 0x4882AC
void _moviefx_palette_func(unsigned char* palette, int start, int end)
{
    memcpy(byte_638EC4 + 3 * start, palette, 3 * (end - start + 1));

    if (!dword_6391C4) {
        paletteSetEntriesInRange(palette, start, end);
    }
}

// 0x488310
void movieEffectsClear()
{
    MovieEffect* movieEffect = gMovieEffectHead;
    while (movieEffect != NULL) {
        MovieEffect* next = movieEffect->next;
        internal_free(movieEffect);
        movieEffect = next;
    }

    gMovieEffectHead = NULL;
}
