#include "movie.h"

#include "color.h"
#include "core.h"
#include "debug.h"
#include "draw.h"
#include "game_config.h"
#include "memory_manager.h"
#include "movie_effect.h"
#include "movie_lib.h"
#include "sound.h"
#include "text_font.h"
#include "window_manager.h"

// 0x5195B8
int gMovieWindow = -1;

// 0x5195BC
int gMovieSubtitlesFont = -1;

// 0x5195E0
MovieSetPaletteEntriesProc* gMovieSetPaletteEntriesProc = setSystemPaletteEntries;

// 0x5195E4
int gMovieSubtitlesColorR = 31;

// 0x5195E8
int gMovieSubtitlesColorG = 31;

// 0x5195EC
int gMovieSubtitlesColorB = 31;

// 0x638E10
Rect gMovieWindowRect;

// 0x638E20
Rect stru_638E20;

// 0x638E30
void (*off_638E30)();

// 0x638E38
MovieSetPaletteProc* gMoviePaletteProc;

// NOTE: Some kind of callback which was intended to change movie file path
// in place during opening movie file to find subsitutions. This callback is
// never set.
//
// 0x638E3C
int (*off_638E3C)(char* filePath);

// 0x638E40
MovieBuildSubtitleFilePathProc* gMovieBuildSubtitleFilePathProc;

// 0x638E48
int dword_638E48;

// 0x638E4C
int dword_638E4C;

// 0x638E50
int dword_638E50;

// 0x638E54
int dword_638E54;

// 0x638E58
int dword_638E58;

// 0x638E5C
int dword_638E5C;

// 0x638E64
int dword_638E64;

// 0x638E68
int dword_638E68;

// 0x638E6C
int dword_638E6C;

// 0x638E70
int dword_638E70;

// 0x638E74
MovieSubtitleListNode* gMovieSubtitleHead;

// 0x638E78
MovieExtendedFlags gMovieFlags;

// 0x638E7C
int dword_638E7C;

// 0x638E80
bool dword_638E80;

// 0x638E84
int dword_638E84;

// 0x638E88
int dword_638E88;

// 0x638E8C
void (*off_638E8C)(void*, int, int, int, int, int);

// 0x638E90
unsigned char* off_638E90;

// 0x638E94
int dword_638E94;

// 0x638E98
void (*off_638E98)();

// 0x638E9C
LPDIRECTDRAWSURFACE gMovieDirectDrawSurface;

// 0x638EA0
int dword_638EA0;

// 0x638EA4
int dword_638EA4;

// 0x638EA8
File* gMovieFileStream;

// 0x638EAC
unsigned char* off_638EAC;

// 0x638EB0
int dword_638EB0;

// 0x638EB4
int dword_638EB4;

// 0x638EB8
bool gMovieDirectSoundInitialized;

// 0x638EBC
File* off_638EBC;

// 0x638EC0
unsigned char* off_638EC0;

// 0x4865FC
void* movieMallocImpl(size_t size)
{
    return internal_malloc_safe(size, __FILE__, __LINE__); // "..\\int\\MOVIE.C", 209
}

// 0x486614
void movieFreeImpl(void* ptr)
{
    internal_free_safe(ptr, __FILE__, __LINE__); // "..\\int\\MOVIE.C", 213
}

// 0x48662C
bool movieReadImpl(int fileHandle, void* buf, int count)
{
    return fileRead(buf, 1, count, (File*)fileHandle) == count;
}

// 0x486654
void movieDirectImpl(LPDIRECTDRAWSURFACE a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9)
{
    int v14;
    int v15;

    DDSURFACEDESC ddsd;
    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(DDSURFACEDESC);

    RECT srcRect;
    srcRect.left = a4;
    srcRect.top = a5;
    srcRect.right = a2 + a4;
    srcRect.bottom = a3 + a5;

    v14 = gMovieWindowRect.right - gMovieWindowRect.left;
    v15 = gMovieWindowRect.right - gMovieWindowRect.left + 1;

    RECT destRect;

    if (dword_638E5C) {
        if ((gMovieFlags & MOVIE_EXTENDED_FLAG_0x08) != 0) {
            destRect.top = (gMovieWindowRect.bottom - gMovieWindowRect.top + 1 - a7) / 2;
            destRect.left = (v15 - 4 * a2 / 3) / 2;
        } else {
            destRect.top = dword_638EB4 + gMovieWindowRect.top;
            destRect.left = gMovieWindowRect.left + dword_638EB0;
        }

        destRect.right = 4 * a2 / 3 + destRect.left;
        destRect.bottom = a7 + destRect.top;
    } else {
        if ((gMovieFlags & MOVIE_EXTENDED_FLAG_0x08) != 0) {
            destRect.top = (gMovieWindowRect.bottom - gMovieWindowRect.top + 1 - a7) / 2;
            destRect.left = (v15 - a6) / 2;
        } else {
            destRect.top = dword_638EB4 + gMovieWindowRect.top;
            destRect.left = gMovieWindowRect.left + dword_638EB0;
        }
        destRect.right = a6 + destRect.left;
        destRect.bottom = a7 + destRect.top;
    }

    dword_638E54 = a4;
    dword_638E58 = a5;
    dword_638E6C = destRect.left;
    dword_638E70 = destRect.top;
    dword_638E4C = a3;
    dword_638E68 = destRect.right - destRect.left;
    gMovieDirectDrawSurface = a1;
    dword_638E50 = a2;
    dword_638E64 = destRect.bottom - destRect.top;

    HRESULT hr;
    do {
        if (off_638E8C != NULL) {
            if (IDirectDrawSurface_Lock(a1, NULL, &ddsd, 1, NULL) == DD_OK) {
                off_638E8C(ddsd.lpSurface, a2, destRect.left, destRect.top, destRect.right - destRect.left, destRect.bottom - destRect.top);
                IDirectDrawSurface_Unlock(a1, ddsd.lpSurface);
            }
        }

        hr = IDirectDrawSurface_Blt(gDirectDrawSurface1, &destRect, a1, &srcRect, 0, NULL);
    } while (hr != DD_OK && hr != DDERR_SURFACELOST && hr == DDERR_WASSTILLDRAWING);
}

// 0x486900
void movieBufferedImpl(LPDIRECTDRAWSURFACE a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9)
{
    int v13;

    if (gMovieWindow == -1) {
        return;
    }

    dword_638E50 = a2;
    gMovieDirectDrawSurface = a1;
    dword_638E4C = a2;
    dword_638E68 = a6;
    dword_638E64 = a7;
    dword_638E6C = a4;
    dword_638E70 = a5;
    dword_638E54 = a4;
    dword_638E58 = a5;

    DDSURFACEDESC ddsd;
    ddsd.dwSize = sizeof(DDSURFACEDESC);

    if (IDirectDrawSurface_Lock(a1, NULL, &ddsd, 1, NULL) != DD_OK) {
        return;
    }

    if (off_638E8C != NULL) {
        // TODO: Ignore, off_638E8C is never set.
        // off_638E8C()
    }

    if (off_638E98 != NULL) {
        // TODO: Ignore, off_638E98 is never set.
        // off_638E98();
    } else {
        v13 = 4 * dword_638E80 + 8 * dword_638E5C + 16 * dword_638E7C;
        // TODO: Incomplete.
    }

    IDirectDrawSurface_Unlock(a1, ddsd.lpSurface);
}

// 0x486C74
int movieScaleSubRectAlpha(int a1)
{
    gMovieFlags |= 1;
    return 0;
}

// 0x486C80
int blitAlpha(int win, unsigned char* a2, int a3, int a4, int a5)
{
    unsigned char* buf;
    int offset;

    offset = windowGetWidth(win) * dword_638EB4 + dword_638EB0;
    buf = windowGetBuffer(win);

    // TODO: Incomplete.
    // alphaBltBuf(a2, a3, a4, a5, off_638EAC, off_638EC0, buf + offset, windowGetWidth(win));

    return 1;
}

// 0x486D84
int blitNormal(int win, int a2, int a3, int a4, int a5)
{
    unsigned char* buf;
    int offset;

    offset = windowGetWidth(win) * dword_638EB4 + dword_638EB0;
    buf = windowGetBuffer(win);

    // TODO: Incomplete.
    // drawScaled(buf + offset, dword_638E94, dword_638E84, windowGetWidth(win), a2, a3, a4, a5);

    return 1;
}

// 0x486DDC
void movieSetPaletteEntriesImpl(unsigned char* palette, int start, int end)
{
    if (end != 0) {
        gMovieSetPaletteEntriesProc(palette + start * 3, start, end + start - 1);
    }
}

// 0x486E08
int noop()
{
    return 0;
}

// initMovie
// 0x486E0C
void movieInit()
{
    movieLibSetMemoryProcs(movieMallocImpl, movieFreeImpl);
    movieLibSetDirectSound(gDirectSound);
    gMovieDirectSoundInitialized = (gDirectSound != NULL);
    movieLibSetDirectDraw(gDirectDraw);
    movieLibSetPaletteEntriesProc(movieSetPaletteEntriesImpl);
    MVE_sfSVGA(640, 480, 480, 0, 0, 0, 0, 0, 0);
    movieLibSetReadProc(movieReadImpl);
}

// 0x486E98
void cleanupMovie(int a1)
{
    if (!dword_638EA4) {
        return;
    }

    // TODO: Probably can be ignored.
    // if (dword_638E34) {
    //     dword_638E34(dword_638E94, dword_638EB0, dword_638E84);
    // }

    int frame;
    int dropped;
    MVE_rmFrameCounts(&frame, &dropped);
    debugPrint("Frames %d, dropped %d\n", frame, dropped);

    if (off_638E90 != NULL) {
        internal_free_safe(off_638E90, __FILE__, __LINE__); // "..\\int\\MOVIE.C", 787
        off_638E90 = NULL;
    }

    if (gMovieDirectDrawSurface != NULL) {
        DDSURFACEDESC ddsd;
        ddsd.dwSize = sizeof(DDSURFACEDESC);
        if (IDirectDrawSurface_Lock(gMovieDirectDrawSurface, 0, &ddsd, 1, NULL) == DD_OK) {
            off_638E90 = internal_malloc_safe(dword_638E4C * dword_638E50, __FILE__, __LINE__); // "..\\int\\MOVIE.C", 802
            blitBufferToBuffer((unsigned char*)ddsd.lpSurface + ddsd.lPitch * dword_638E54 + dword_638E58, dword_638E50, dword_638E4C, ddsd.lPitch, off_638E90, dword_638E50);
            IDirectDrawSurface_Unlock(gMovieDirectDrawSurface, ddsd.lpSurface);
        } else {
            debugPrint("Couldn't lock movie surface\n");
        }

        gMovieDirectDrawSurface = NULL;
    }

    if (a1) {
        MVE_rmEndMovie();
    }

    MVE_ReleaseMem();

    fileClose(gMovieFileStream);

    if (off_638EAC != NULL) {
        blitBufferToBuffer(off_638EAC, dword_638E94, dword_638E84, dword_638E94, windowGetBuffer(gMovieWindow) + dword_638EB4 * windowGetWidth(gMovieWindow) + dword_638EB0, windowGetWidth(gMovieWindow));
        windowRefreshRect(gMovieWindow, &stru_638E20);
    }

    if (off_638EBC != NULL) {
        fileClose(off_638EBC);
        off_638EBC = NULL;
    }

    if (off_638EC0 != NULL) {
        internal_free_safe(off_638EC0, __FILE__, __LINE__); // "..\\int\\MOVIE.C", 840
        off_638EC0 = NULL;
    }

    if (off_638EAC != NULL) {
        internal_free_safe(off_638EAC, __FILE__, __LINE__); // "..\\int\\MOVIE.C", 845
        off_638EAC = NULL;
    }

    while (gMovieSubtitleHead != NULL) {
        MovieSubtitleListNode* next = gMovieSubtitleHead->next;
        internal_free_safe(gMovieSubtitleHead->text, __FILE__, __LINE__); // "..\\int\\MOVIE.C", 851
        internal_free_safe(gMovieSubtitleHead, __FILE__, __LINE__); // "..\\int\\MOVIE.C", 852
        gMovieSubtitleHead = next;
    }

    dword_638EA4 = 0;
    dword_638E80 = 0;
    dword_638E5C = 0;
    dword_638E7C = 0;
    gMovieFlags = 0;
    gMovieWindow = -1;
}

// 0x48711C
void movieExit()
{
    cleanupMovie(1);

    if (off_638E90) {
        internal_free_safe(off_638E90, __FILE__, __LINE__); // "..\\int\\MOVIE.C", 869
        off_638E90 = NULL;
    }
}

// 0x487150
void movieStop()
{
    if (dword_638EA4) {
        gMovieFlags |= MOVIE_EXTENDED_FLAG_0x02;
    }
}

// 0x487164
int movieSetFlags(int flags)
{
    if ((flags & MOVIE_FLAG_0x04) != 0) {
        gMovieFlags |= MOVIE_EXTENDED_FLAG_0x04 | MOVIE_EXTENDED_FLAG_0x08;
    } else {
        gMovieFlags &= ~MOVIE_EXTENDED_FLAG_0x08;
        if ((flags & MOVIE_FLAG_0x02) != 0) {
            gMovieFlags |= MOVIE_EXTENDED_FLAG_0x04;
        } else {
            gMovieFlags &= ~MOVIE_EXTENDED_FLAG_0x04;
        }
    }

    if ((flags & MOVIE_FLAG_0x01) != 0) {
        dword_638E5C = 1;

        if ((gMovieFlags & MOVIE_EXTENDED_FLAG_0x04) != 0) {
            sub_4F4BB(3);
        }
    } else {
        dword_638E5C = 0;

        if ((gMovieFlags & MOVIE_EXTENDED_FLAG_0x04) != 0) {
            sub_4F4BB(4);
        } else {
            gMovieFlags &= ~MOVIE_EXTENDED_FLAG_0x08;
        }
    }

    if ((flags & MOVIE_FLAG_0x08) != 0) {
        gMovieFlags |= MOVIE_EXTENDED_FLAG_0x10;
    } else {
        gMovieFlags &= ~MOVIE_EXTENDED_FLAG_0x10;
    }

    return 0;
}

// 0x48725C
void movieSetPaletteFunc(MovieSetPaletteEntriesProc* proc)
{
    gMovieSetPaletteEntriesProc = proc != NULL ? proc : setSystemPaletteEntries;
}

// 0x487274
void movieSetPaletteProc(MovieSetPaletteProc* proc)
{
    gMoviePaletteProc = proc;
}

// 0x4872E8
void cleanupLast()
{
    if (off_638E90 != NULL) {
        internal_free_safe(off_638E90, __FILE__, __LINE__); // "..\\int\\MOVIE.C", 981
        off_638E90 = NULL;
    }

    gMovieDirectDrawSurface = NULL;
}

// 0x48731C
File* movieOpen(char* filePath)
{
    gMovieFileStream = fileOpen(filePath, "rb");
    if (gMovieFileStream == NULL) {
        if (off_638E3C == NULL) {
            debugPrint("Couldn't find movie file %s\n", filePath);
            return 0;
        }

        while (gMovieFileStream == NULL && off_638E3C(filePath) != 0) {
            gMovieFileStream = fileOpen(filePath, "rb");
        }
    }
    return gMovieFileStream;
}

// 0x487380
void movieLoadSubtitles(char* filePath)
{
    dword_638E48 = windowGetXres();
    dword_638EA0 = fontGetLineHeight() + 4;

    if (gMovieBuildSubtitleFilePathProc != NULL) {
        filePath = gMovieBuildSubtitleFilePathProc(filePath);
    }

    char path[MAX_PATH];
    strcpy(path, filePath);

    debugPrint("Opening subtitle file %s\n", path);
    File* stream = fileOpen(path, "r");
    if (stream == NULL) {
        debugPrint("Couldn't open subtitle file %s\n", path);
        gMovieFlags &= ~MOVIE_EXTENDED_FLAG_0x10;
        return;
    }

    MovieSubtitleListNode* prev = NULL;
    int subtitleCount = 0;
    while (!fileEof(stream)) {
        char string[260];
        string[0] = '\0';
        fileReadString(string, 259, stream);
        if (*string == '\0') {
            break;
        }

        MovieSubtitleListNode* subtitle = internal_malloc_safe(sizeof(*subtitle), __FILE__, __LINE__); // "..\\int\\MOVIE.C", 1050
        subtitle->next = NULL;

        subtitleCount++;

        char* pch;

        pch = string;
        while (*pch != '\0' && *pch != '\n') {
            pch++;
        }

        if (*pch != '\0') {
            *pch = '\0';
        }

        pch = string;
        while (*pch != '\0' && *pch != '\r') {
            pch++;
        }

        if (*pch != '\0') {
            *pch = '\0';
        }

        pch = string;
        while (*pch != '\0' && *pch != ':') {
            pch++;
        }

        if (*pch != '\0') {
            *pch = '\0';
            subtitle->num = atoi(string);
            subtitle->text = strdup_safe(pch + 1, __FILE__, __LINE__); // "..\\int\\MOVIE.C", 1058

            if (prev != NULL) {
                prev->next = subtitle;
            } else {
                gMovieSubtitleHead = subtitle;
            }

            prev = subtitle;
        } else {
            debugPrint("subtitle: couldn't parse %s\n", string);
        }
    }

    fileClose(stream);

    debugPrint("Read %d subtitles\n", subtitleCount);
}

// 0x48755C
void movieRenderSubtitles()
{
    if (gMovieSubtitleHead == NULL) {
        return;
    }

    if ((gMovieFlags & MOVIE_EXTENDED_FLAG_0x10) == 0) {
        return;
    }

    int v1 = fontGetLineHeight();
    int v2 = (480 - dword_638E64 - dword_638E70 - v1) / 2 + dword_638E64 + dword_638E70;

    if (dword_638EA0 + v2 > windowGetYres()) {
        dword_638EA0 = windowGetYres() - v2;
    }

    int frame;
    int dropped;
    MVE_rmFrameCounts(&frame, &dropped);

    while (gMovieSubtitleHead != NULL) {
        if (frame < gMovieSubtitleHead->num) {
            break;
        }

        MovieSubtitleListNode* next = gMovieSubtitleHead->next;

        windowFill(gMovieWindow, 0, v2, dword_638E48, dword_638EA0, 0);

        int oldFont;
        if (gMovieSubtitlesFont != -1) {
            oldFont = fontGetCurrent();
            fontSetCurrent(gMovieSubtitlesFont);
        }

        int colorIndex = (gMovieSubtitlesColorR << 10) | (gMovieSubtitlesColorG << 5) | gMovieSubtitlesColorB;
        windowWrapLine(gMovieWindow, gMovieSubtitleHead->text, dword_638E48, dword_638EA0, 0, v2, byte_6A38D0[colorIndex] | 0x2000000, TEXT_ALIGNMENT_CENTER);

        Rect rect;
        rect.right = dword_638E48;
        rect.top = v2;
        rect.bottom = v2 + dword_638EA0;
        rect.left = 0;
        windowRefreshRect(gMovieWindow, &rect);

        internal_free_safe(gMovieSubtitleHead->text, __FILE__, __LINE__); // "..\\int\\MOVIE.C", 1108
        internal_free_safe(gMovieSubtitleHead, __FILE__, __LINE__); // "..\\int\\MOVIE.C", 1109

        gMovieSubtitleHead = next;

        if (gMovieSubtitlesFont != -1) {
            fontSetCurrent(oldFont);
        }
    }
}

// 0x487710
int movieStart(int win, char* filePath, int (*a3)())
{
    int v15;
    int v16;
    int v17;

    if (dword_638EA4) {
        return 1;
    }

    cleanupLast();

    gMovieFileStream = movieOpen(filePath);
    if (gMovieFileStream == NULL) {
        return 1;
    }

    gMovieWindow = win;
    dword_638EA4 = 1;
    gMovieFlags &= ~MOVIE_EXTENDED_FLAG_0x01;

    if ((gMovieFlags & MOVIE_EXTENDED_FLAG_0x10) != 0) {
        movieLoadSubtitles(filePath);
    }

    if ((gMovieFlags & MOVIE_EXTENDED_FLAG_0x04) != 0) {
        debugPrint("Direct ");
        windowGetRect(gMovieWindow, &gMovieWindowRect);
        debugPrint("Playing at (%d, %d)  ", dword_638EB0 + gMovieWindowRect.left, dword_638EB4 + gMovieWindowRect.top);
        MVE_rmCallbacks(a3);
        MVE_sfCallbacks(movieDirectImpl);

        v17 = 0;
        v16 = dword_638EB4 + gMovieWindowRect.top;
        v15 = dword_638EB0 + gMovieWindowRect.left;
    } else {
        debugPrint("Buffered ");
        MVE_rmCallbacks(a3);
        MVE_sfCallbacks(movieBufferedImpl);
        v17 = 0;
        v16 = 0;
        v15 = 0;
    }

    MVE_rmPrepMovie((int)gMovieFileStream, v15, v16, v17);

    if (dword_638E5C) {
        debugPrint("scaled\n");
    } else {
        debugPrint("not scaled\n");
    }

    // TODO: Probably can be ignored, never set.
    // if (dword_638E44) {
    //     dword_638E44();
    // }

    if (off_638EBC != NULL) {
        // TODO: Probably can be ignored, never set.
        abort();
    }

    stru_638E20.left = dword_638EB0;
    stru_638E20.top = dword_638EB4;
    stru_638E20.right = dword_638E94 + dword_638EB0;
    stru_638E20.bottom = dword_638E84 + dword_638EB4;

    return 0;
}

// 0x487964
bool localMovieCallback()
{
    movieRenderSubtitles();

    if (off_638E30 != NULL) {
        off_638E30();
    }

    return get_input() != -1;
}

// 0x487AC8
int movieRun(int win, char* filePath)
{
    if (dword_638EA4) {
        return 1;
    }

    dword_638EB0 = 0;
    dword_638EB4 = 0;
    dword_638E88 = 0;
    dword_638E94 = windowGetWidth(win);
    dword_638E84 = windowGetHeight(win);
    dword_638E80 = 0;
    return movieStart(win, filePath, noop);
}

// 0x487B1C
int movieRunRect(int win, char* filePath, int a3, int a4, int a5, int a6)
{
    if (dword_638EA4) {
        return 1;
    }

    dword_638EB0 = a3;
    dword_638EB4 = a4;
    dword_638E88 = a3 + a4 * windowGetWidth(win);
    dword_638E94 = a5;
    dword_638E84 = a6;
    dword_638E80 = 1;

    return movieStart(win, filePath, noop);
}

// 0x487B7C
int stepMovie()
{
    if (off_638EBC != NULL) {
        int size;
        fileReadInt32(off_638EBC, &size);
        fileRead(off_638EC0, 1, size, off_638EBC);
    }

    int v1 = MVE_rmStepMovie();
    if (v1 != -1) {
        movieRenderSubtitles();
    }

    return v1;
}

// 0x487BC8
void movieSetBuildSubtitleFilePathProc(MovieBuildSubtitleFilePathProc* proc)
{
    gMovieBuildSubtitleFilePathProc = proc;
}

// 0x487BD0
void movieSetVolume(int volume)
{
    if (gMovieDirectSoundInitialized) {
        int normalizedVolume = soundVolumeHMItoDirectSound(volume);
        movieLibSetVolume(normalizedVolume);
    }
}

// 0x487BEC
void movieUpdate()
{
    if (!dword_638EA4) {
        return;
    }

    if ((gMovieFlags & MOVIE_EXTENDED_FLAG_0x02) != 0) {
        debugPrint("Movie aborted\n");
        cleanupMovie(1);
        return;
    }

    if ((gMovieFlags & MOVIE_EXTENDED_FLAG_0x01) != 0) {
        debugPrint("Movie error\n");
        cleanupMovie(1);
        return;
    }

    if (stepMovie() == -1) {
        cleanupMovie(1);
        return;
    }

    if (gMoviePaletteProc != NULL) {
        int frame;
        int dropped;
        MVE_rmFrameCounts(&frame, &dropped);
        gMoviePaletteProc(frame);
    }
}

// 0x487C88
int moviePlaying()
{
    return dword_638EA4;
}
