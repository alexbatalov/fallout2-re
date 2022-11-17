#include "int/movie.h"

#include <string.h>

#include "plib/color/color.h"
#include "core.h"
#include "db.h"
#include "plib/gnw/debug.h"
#include "plib/gnw/grbuf.h"
#include "game/gconfig.h"
#include "int/memdbg.h"
#include "game/moviefx.h"
#include "movie_lib.h"
#include "int/sound.h"
#include "plib/gnw/text.h"
#include "plib/gnw/gnw.h"
#include "plib/gnw/winmain.h"

typedef void(MovieCallback)();
typedef int(MovieBlitFunc)(int win, unsigned char* data, int width, int height, int pitch);

typedef struct MovieSubtitleListNode {
    int num;
    char* text;
    struct MovieSubtitleListNode* next;
} MovieSubtitleListNode;

static void* movieMalloc(size_t size);
static void movieFree(void* ptr);
static bool movieRead(int fileHandle, void* buf, int count);
static void movie_MVE_ShowFrame(LPDIRECTDRAWSURFACE a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9);
static void movieShowFrame(LPDIRECTDRAWSURFACE a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9);
static int movieScaleSubRect(int win, unsigned char* data, int width, int height, int pitch);
static int movieScaleWindowAlpha(int win, unsigned char* data, int width, int height, int pitch);
static int movieScaleSubRectAlpha(int win, unsigned char* data, int width, int height, int pitch);
static int blitAlpha(int win, unsigned char* data, int width, int height, int pitch);
static int movieScaleWindow(int win, unsigned char* data, int width, int height, int pitch);
static int blitNormal(int win, unsigned char* data, int width, int height, int pitch);
static void movieSetPalette(unsigned char* palette, int start, int end);
static int noop();
static void cleanupMovie(int a1);
static void cleanupLast();
static File* openFile(char* filePath);
static void openSubtitle(char* filePath);
static void doSubtitle();
static int movieStart(int win, char* filePath, int (*a3)());
static bool localMovieCallback();
static int stepMovie();

// 0x5195B8
static int GNWWin = -1;

// 0x5195BC
static int subtitleFont = -1;

// 0x5195C0
static MovieBlitFunc* showFrameFuncs[2][2][2] = {
    {
        {
            blitNormal,
            blitNormal,
        },
        {
            movieScaleWindow,
            movieScaleSubRect,
        },
    },
    {
        {
            blitAlpha,
            blitAlpha,
        },
        {
            movieScaleSubRectAlpha,
            movieScaleWindowAlpha,
        },
    },
};

// 0x5195E0
static MoviePaletteFunc* paletteFunc = setSystemPaletteEntries;

// 0x5195E4
static int subtitleR = 31;

// 0x5195E8
static int subtitleG = 31;

// 0x5195EC
static int subtitleB = 31;

// 0x638E10
static Rect winRect;

// 0x638E20
static Rect movieRect;

// 0x638E30
static MovieCallback* movieCallback;

// 0x638E34
static MovieEndFunc* endMovieFunc;

// 0x638E38
static MovieUpdateCallbackProc* updateCallbackFunc;

// 0x638E3C
static MovieFailedOpenFunc* failedOpenFunc;

// 0x638E40
static MovieSubtitleFunc* subtitleFilenameFunc;

// 0x638E44
static MovieStartFunc* startMovieFunc;

// 0x638E48
static int subtitleW;

// 0x638E4C
static int lastMovieBH;

// 0x638E50
static int lastMovieBW;

// 0x638E54
static int lastMovieSX;

// 0x638E58
static int lastMovieSY;

// 0x638E5C
static int movieScaleFlag;

// 0x638E60
static MoviePreDrawFunc* moviePreDrawFunc;

// 0x638E64
static int lastMovieH;

// 0x638E68
static int lastMovieW;

// 0x638E6C
static int lastMovieX;

// 0x638E70
static int lastMovieY;

// 0x638E74
static MovieSubtitleListNode* subtitleList;

// 0x638E78
static unsigned int movieFlags;

// 0x638E7C
static int movieAlphaFlag;

// 0x638E80
static bool movieSubRectFlag;

// 0x638E84
static int movieH;

// 0x638E88
static int movieOffset;

// 0x638E8C
static MovieCaptureFrameProc* movieCaptureFrameFunc;

// 0x638E90
static unsigned char* lastMovieBuffer;

// 0x638E94
static int movieW;

// 0x638E98
static MovieFrameGrabProc* movieFrameGrabFunc;

// 0x638E9C
static LPDIRECTDRAWSURFACE MVE_lastBuffer;

// 0x638EA0
static int subtitleH;

// 0x638EA4
static int running;

// 0x638EA8
static File* handle;

// 0x638EAC
static unsigned char* alphaWindowBuf;

// 0x638EB0
static int movieX;

// 0x638EB4
static int movieY;

// 0x638EB8
static bool soundEnabled;

// 0x638EBC
static File* alphaHandle;

// 0x638EC0
static unsigned char* alphaBuf;

// NOTE: Unused.
//
// 0x4865E0
void movieSetPreDrawFunc(MoviePreDrawFunc* func)
{
    moviePreDrawFunc = func;
}

// NOTE: Unused.
//
// 0x4865E8
void movieSetFailedOpenFunc(MovieFailedOpenFunc* func)
{
    failedOpenFunc = func;
}

// NOTE: Unused.
//
// 0x4865F0
void movieSetFunc(MovieStartFunc* startFunc, MovieEndFunc* endFunc)
{
    startMovieFunc = startFunc;
    endMovieFunc = endFunc;
}

// 0x4865FC
static void* movieMalloc(size_t size)
{
    return mymalloc(size, __FILE__, __LINE__); // "..\\int\\MOVIE.C", 209
}

// 0x486614
static void movieFree(void* ptr)
{
    myfree(ptr, __FILE__, __LINE__); // "..\\int\\MOVIE.C", 213
}

// 0x48662C
static bool movieRead(int fileHandle, void* buf, int count)
{
    return fileRead(buf, 1, count, (File*)fileHandle) == count;
}

// 0x486654
static void movie_MVE_ShowFrame(LPDIRECTDRAWSURFACE surface, int srcWidth, int srcHeight, int srcX, int srcY, int destWidth, int destHeight, int a8, int a9)
{
    int v14;
    int v15;

    DDSURFACEDESC ddsd;
    memset(&ddsd, 0, sizeof(DDSURFACEDESC));
    ddsd.dwSize = sizeof(DDSURFACEDESC);

    RECT srcRect;
    srcRect.left = srcX;
    srcRect.top = srcY;
    srcRect.right = srcWidth + srcX;
    srcRect.bottom = srcHeight + srcY;

    v14 = winRect.lrx - winRect.ulx;
    v15 = winRect.lrx - winRect.ulx + 1;

    RECT destRect;

    if (movieScaleFlag) {
        if ((movieFlags & MOVIE_EXTENDED_FLAG_0x08) != 0) {
            destRect.top = (winRect.lry - winRect.uly + 1 - destHeight) / 2;
            destRect.left = (v15 - 4 * srcWidth / 3) / 2;
        } else {
            destRect.top = movieY + winRect.uly;
            destRect.left = winRect.ulx + movieX;
        }

        destRect.right = 4 * srcWidth / 3 + destRect.left;
        destRect.bottom = destHeight + destRect.top;
    } else {
        if ((movieFlags & MOVIE_EXTENDED_FLAG_0x08) != 0) {
            destRect.top = (winRect.lry - winRect.uly + 1 - destHeight) / 2;
            destRect.left = (v15 - destWidth) / 2;
        } else {
            destRect.top = movieY + winRect.uly;
            destRect.left = winRect.ulx + movieX;
        }
        destRect.right = destWidth + destRect.left;
        destRect.bottom = destHeight + destRect.top;
    }

    lastMovieSX = srcX;
    lastMovieSY = srcY;
    lastMovieX = destRect.left;
    lastMovieY = destRect.top;
    lastMovieBH = srcHeight;
    lastMovieW = destRect.right - destRect.left;
    MVE_lastBuffer = surface;
    lastMovieBW = srcWidth;
    lastMovieH = destRect.bottom - destRect.top;

    HRESULT hr;
    do {
        if (movieCaptureFrameFunc != NULL) {
            if (IDirectDrawSurface_Lock(surface, NULL, &ddsd, 1, NULL) == DD_OK) {
                unsigned char* data = (unsigned char*)ddsd.lpSurface + ddsd.lPitch * srcY + srcX;
                movieCaptureFrameFunc(data,
                    srcWidth,
                    srcHeight,
                    ddsd.lPitch,
                    destRect.left,
                    destRect.top,
                    destRect.right - destRect.left,
                    destRect.bottom - destRect.top);
                IDirectDrawSurface_Unlock(surface, ddsd.lpSurface);
            }
        }

        hr = IDirectDrawSurface_Blt(gDirectDrawSurface1, &destRect, surface, &srcRect, 0, NULL);
    } while (hr != DD_OK && hr != DDERR_SURFACELOST && hr == DDERR_WASSTILLDRAWING);
}

// 0x486900
static void movieShowFrame(LPDIRECTDRAWSURFACE a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9)
{
    if (GNWWin == -1) {
        return;
    }

    lastMovieBW = a2;
    MVE_lastBuffer = a1;
    lastMovieBH = a2;
    lastMovieW = a6;
    lastMovieH = a7;
    lastMovieX = a4;
    lastMovieY = a5;
    lastMovieSX = a4;
    lastMovieSY = a5;

    DDSURFACEDESC ddsd;
    ddsd.dwSize = sizeof(DDSURFACEDESC);

    if (IDirectDrawSurface_Lock(a1, NULL, &ddsd, 1, NULL) != DD_OK) {
        return;
    }

    unsigned char* data = (unsigned char*)ddsd.lpSurface + ddsd.lPitch * a5 + a4;

    if (movieCaptureFrameFunc != NULL) {
        // FIXME: Looks wrong as it ignores lPitch (as seen in movie_MVE_ShowFrame).
        movieCaptureFrameFunc(data, a2, a3, a2, movieRect.ulx, movieRect.uly, a6, a7);
    }

    if (movieFrameGrabFunc != NULL) {
        movieFrameGrabFunc(data, a2, a3, ddsd.lPitch);
    } else {
        MovieBlitFunc* func = showFrameFuncs[movieAlphaFlag][movieScaleFlag][movieSubRectFlag];
        if (func(GNWWin, data, a2, a3, ddsd.lPitch) != 0) {
            if (moviePreDrawFunc != NULL) {
                moviePreDrawFunc(GNWWin, &movieRect);
            }

            win_draw_rect(GNWWin, &movieRect);
        }
    }

    IDirectDrawSurface_Unlock(a1, ddsd.lpSurface);
}

// NOTE: Unused.
//
// 0x486A98
void movieSetFrameGrabFunc(MovieFrameGrabProc* func)
{
    movieFrameGrabFunc = func;
}

// NOTE: Unused.
//
// 0x486AA0
void movieSetCaptureFrameFunc(MovieCaptureFrameProc* func)
{
    movieCaptureFrameFunc = func;
}

// 0x486B68
static int movieScaleSubRect(int win, unsigned char* data, int width, int height, int pitch)
{
    int windowWidth = win_width(win);
    unsigned char* windowBuffer = win_get_buf(win) + windowWidth * movieY + movieX;
    if (width * 4 / 3 > movieW) {
        movieFlags |= 0x01;
        return 0;
    }

    int v1 = width / 3;
    for (int y = 0; y < height; y++) {
        int x;
        for (x = 0; x < v1; x++) {
            unsigned int value = data[0];
            value |= data[1] << 8;
            value |= data[2] << 16;
            value |= data[2] << 24;

            *(unsigned int*)windowBuffer = value;

            windowBuffer += 4;
            data += 3;
        }

        for (x = x * 3; x < width; x++) {
            *windowBuffer++ = *data++;
        }

        data += pitch - width;
        windowBuffer += windowWidth - movieW;
    }

    return 1;
}

// 0x486C74
static int movieScaleWindowAlpha(int win, unsigned char* data, int width, int height, int pitch)
{
    movieFlags |= 1;
    return 0;
}

// NOTE: Uncollapsed 0x486C74.
static int movieScaleSubRectAlpha(int win, unsigned char* data, int width, int height, int pitch)
{
    movieFlags |= 1;
    return 0;
}

// 0x486C80
static int blitAlpha(int win, unsigned char* data, int width, int height, int pitch)
{
    int windowWidth = win_width(win);
    unsigned char* windowBuffer = win_get_buf(win);
    _alphaBltBuf(data, width, height, pitch, alphaWindowBuf, alphaBuf, windowBuffer + windowWidth * movieY + movieX, windowWidth);
    return 1;
}

// 0x486CD4
static int movieScaleWindow(int win, unsigned char* data, int width, int height, int pitch)
{
    int windowWidth = win_width(win);
    if (width != 3 * windowWidth / 4) {
        movieFlags |= 1;
        return 0;
    }

    unsigned char* windowBuffer = win_get_buf(win);
    for (int y = 0; y < height; y++) {
        int scaledWidth = width / 3;
        for (int x = 0; x < scaledWidth; x++) {
            unsigned int value = data[0];
            value |= data[1] << 8;
            value |= data[2] << 16;
            value |= data[3] << 24;

            *(unsigned int*)windowBuffer = value;

            windowBuffer += 4;
            data += 3;
        }
        data += pitch - width;
    }

    return 1;
}

// 0x486D84
static int blitNormal(int win, unsigned char* data, int width, int height, int pitch)
{
    int windowWidth = win_width(win);
    unsigned char* windowBuffer = win_get_buf(win);
    _drawScaled(windowBuffer + windowWidth * movieY + movieX, movieW, movieH, windowWidth, data, width, height, pitch);
    return 1;
}

// 0x486DDC
static void movieSetPalette(unsigned char* palette, int start, int end)
{
    if (end != 0) {
        paletteFunc(palette + start * 3, start, end + start - 1);
    }
}

// 0x486E08
static int noop()
{
    return 0;
}

// initMovie
// 0x486E0C
void initMovie()
{
    movieLibSetMemoryProcs(movieMalloc, movieFree);
    movieLibSetDirectSound(soundDSObject);
    soundEnabled = (soundDSObject != NULL);
    movieLibSetDirectDraw(gDirectDraw);
    movieLibSetPaletteEntriesProc(movieSetPalette);
    _MVE_sfSVGA(640, 480, 480, 0, 0, 0, 0, 0, 0);
    movieLibSetReadProc(movieRead);
}

// 0x486E98
static void cleanupMovie(int a1)
{
    if (!running) {
        return;
    }

    if (endMovieFunc != NULL) {
        endMovieFunc(GNWWin, movieX, movieY, movieW, movieH);
    }

    int frame;
    int dropped;
    _MVE_rmFrameCounts(&frame, &dropped);
    debug_printf("Frames %d, dropped %d\n", frame, dropped);

    if (lastMovieBuffer != NULL) {
        myfree(lastMovieBuffer, __FILE__, __LINE__); // "..\\int\\MOVIE.C", 787
        lastMovieBuffer = NULL;
    }

    if (MVE_lastBuffer != NULL) {
        DDSURFACEDESC ddsd;
        ddsd.dwSize = sizeof(DDSURFACEDESC);
        if (IDirectDrawSurface_Lock(MVE_lastBuffer, 0, &ddsd, 1, NULL) == DD_OK) {
            lastMovieBuffer = (unsigned char*)mymalloc(lastMovieBH * lastMovieBW, __FILE__, __LINE__); // "..\\int\\MOVIE.C", 802
            blitBufferToBuffer((unsigned char*)ddsd.lpSurface + ddsd.lPitch * lastMovieSX + lastMovieSY, lastMovieBW, lastMovieBH, ddsd.lPitch, lastMovieBuffer, lastMovieBW);
            IDirectDrawSurface_Unlock(MVE_lastBuffer, ddsd.lpSurface);
        } else {
            debug_printf("Couldn't lock movie surface\n");
        }

        MVE_lastBuffer = NULL;
    }

    if (a1) {
        _MVE_rmEndMovie();
    }

    _MVE_ReleaseMem();

    fileClose(handle);

    if (alphaWindowBuf != NULL) {
        blitBufferToBuffer(alphaWindowBuf, movieW, movieH, movieW, win_get_buf(GNWWin) + movieY * win_width(GNWWin) + movieX, win_width(GNWWin));
        win_draw_rect(GNWWin, &movieRect);
    }

    if (alphaHandle != NULL) {
        fileClose(alphaHandle);
        alphaHandle = NULL;
    }

    if (alphaBuf != NULL) {
        myfree(alphaBuf, __FILE__, __LINE__); // "..\\int\\MOVIE.C", 840
        alphaBuf = NULL;
    }

    if (alphaWindowBuf != NULL) {
        myfree(alphaWindowBuf, __FILE__, __LINE__); // "..\\int\\MOVIE.C", 845
        alphaWindowBuf = NULL;
    }

    while (subtitleList != NULL) {
        MovieSubtitleListNode* next = subtitleList->next;
        myfree(subtitleList->text, __FILE__, __LINE__); // "..\\int\\MOVIE.C", 851
        myfree(subtitleList, __FILE__, __LINE__); // "..\\int\\MOVIE.C", 852
        subtitleList = next;
    }

    running = 0;
    movieSubRectFlag = 0;
    movieScaleFlag = 0;
    movieAlphaFlag = 0;
    movieFlags = 0;
    GNWWin = -1;
}

// 0x48711C
void movieClose()
{
    cleanupMovie(1);

    if (lastMovieBuffer) {
        myfree(lastMovieBuffer, __FILE__, __LINE__); // "..\\int\\MOVIE.C", 869
        lastMovieBuffer = NULL;
    }
}

// 0x487150
void movieStop()
{
    if (running) {
        movieFlags |= MOVIE_EXTENDED_FLAG_0x02;
    }
}

// 0x487164
int movieSetFlags(int flags)
{
    if ((flags & MOVIE_FLAG_0x04) != 0) {
        movieFlags |= MOVIE_EXTENDED_FLAG_0x04 | MOVIE_EXTENDED_FLAG_0x08;
    } else {
        movieFlags &= ~MOVIE_EXTENDED_FLAG_0x08;
        if ((flags & MOVIE_FLAG_0x02) != 0) {
            movieFlags |= MOVIE_EXTENDED_FLAG_0x04;
        } else {
            movieFlags &= ~MOVIE_EXTENDED_FLAG_0x04;
        }
    }

    if ((flags & MOVIE_FLAG_0x01) != 0) {
        movieScaleFlag = 1;

        if ((movieFlags & MOVIE_EXTENDED_FLAG_0x04) != 0) {
            _sub_4F4BB(3);
        }
    } else {
        movieScaleFlag = 0;

        if ((movieFlags & MOVIE_EXTENDED_FLAG_0x04) != 0) {
            _sub_4F4BB(4);
        } else {
            movieFlags &= ~MOVIE_EXTENDED_FLAG_0x08;
        }
    }

    if ((flags & MOVIE_FLAG_0x08) != 0) {
        movieFlags |= MOVIE_EXTENDED_FLAG_0x10;
    } else {
        movieFlags &= ~MOVIE_EXTENDED_FLAG_0x10;
    }

    return 0;
}

// NOTE: Unused.
//
// 0x48720C
void movieSetSubtitleFont(int font)
{
    subtitleFont = font;
}

// NOTE: Unused.
//
// 0x487214
void movieSetSubtitleColor(float r, float g, float b)
{
    subtitleR = (int)(r * 31.0f);
    subtitleG = (int)(g * 31.0f);
    subtitleB = (int)(b * 31.0f);
}

// 0x48725C
void movieSetPaletteFunc(MoviePaletteFunc* func)
{
    paletteFunc = func != NULL ? func : setSystemPaletteEntries;
}

// 0x487274
void movieSetCallback(MovieUpdateCallbackProc* func)
{
    updateCallbackFunc = func;
}

// 0x4872E8
static void cleanupLast()
{
    if (lastMovieBuffer != NULL) {
        myfree(lastMovieBuffer, __FILE__, __LINE__); // "..\\int\\MOVIE.C", 981
        lastMovieBuffer = NULL;
    }

    MVE_lastBuffer = NULL;
}

// 0x48731C
static File* openFile(char* filePath)
{
    handle = fileOpen(filePath, "rb");
    if (handle == NULL) {
        if (failedOpenFunc == NULL) {
            debug_printf("Couldn't find movie file %s\n", filePath);
            return 0;
        }

        while (handle == NULL && failedOpenFunc(filePath) != 0) {
            handle = fileOpen(filePath, "rb");
        }
    }
    return handle;
}

// 0x487380
static void openSubtitle(char* filePath)
{
    subtitleW = _windowGetXres();
    subtitleH = text_height() + 4;

    if (subtitleFilenameFunc != NULL) {
        filePath = subtitleFilenameFunc(filePath);
    }

    char path[MAX_PATH];
    strcpy(path, filePath);

    debug_printf("Opening subtitle file %s\n", path);
    File* stream = fileOpen(path, "r");
    if (stream == NULL) {
        debug_printf("Couldn't open subtitle file %s\n", path);
        movieFlags &= ~MOVIE_EXTENDED_FLAG_0x10;
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

        MovieSubtitleListNode* subtitle = (MovieSubtitleListNode*)mymalloc(sizeof(*subtitle), __FILE__, __LINE__); // "..\\int\\MOVIE.C", 1050
        subtitle->next = NULL;

        subtitleCount++;

        char* pch;

        pch = strchr(string, '\n');
        if (pch != NULL) {
            *pch = '\0';
        }

        pch = strchr(string, '\r');
        if (pch != NULL) {
            *pch = '\0';
        }

        pch = strchr(string, ':');
        if (pch != NULL) {
            *pch = '\0';
            subtitle->num = atoi(string);
            subtitle->text = mystrdup(pch + 1, __FILE__, __LINE__); // "..\\int\\MOVIE.C", 1058

            if (prev != NULL) {
                prev->next = subtitle;
            } else {
                subtitleList = subtitle;
            }

            prev = subtitle;
        } else {
            debug_printf("subtitle: couldn't parse %s\n", string);
        }
    }

    fileClose(stream);

    debug_printf("Read %d subtitles\n", subtitleCount);
}

// 0x48755C
static void doSubtitle()
{
    if (subtitleList == NULL) {
        return;
    }

    if ((movieFlags & MOVIE_EXTENDED_FLAG_0x10) == 0) {
        return;
    }

    int v1 = text_height();
    int v2 = (480 - lastMovieH - lastMovieY - v1) / 2 + lastMovieH + lastMovieY;

    if (subtitleH + v2 > _windowGetYres()) {
        subtitleH = _windowGetYres() - v2;
    }

    int frame;
    int dropped;
    _MVE_rmFrameCounts(&frame, &dropped);

    while (subtitleList != NULL) {
        if (frame < subtitleList->num) {
            break;
        }

        MovieSubtitleListNode* next = subtitleList->next;

        win_fill(GNWWin, 0, v2, subtitleW, subtitleH, 0);

        int oldFont;
        if (subtitleFont != -1) {
            oldFont = text_curr();
            text_font(subtitleFont);
        }

        int colorIndex = (subtitleR << 10) | (subtitleG << 5) | subtitleB;
        windowWrapLine(GNWWin, subtitleList->text, subtitleW, subtitleH, 0, v2, colorTable[colorIndex] | 0x2000000, TEXT_ALIGNMENT_CENTER);

        Rect rect;
        rect.lrx = subtitleW;
        rect.uly = v2;
        rect.lry = v2 + subtitleH;
        rect.ulx = 0;
        win_draw_rect(GNWWin, &rect);

        myfree(subtitleList->text, __FILE__, __LINE__); // "..\\int\\MOVIE.C", 1108
        myfree(subtitleList, __FILE__, __LINE__); // "..\\int\\MOVIE.C", 1109

        subtitleList = next;

        if (subtitleFont != -1) {
            text_font(oldFont);
        }
    }
}

// 0x487710
static int movieStart(int win, char* filePath, int (*a3)())
{
    int v15;
    int v16;
    int v17;

    if (running) {
        return 1;
    }

    cleanupLast();

    handle = openFile(filePath);
    if (handle == NULL) {
        return 1;
    }

    GNWWin = win;
    running = 1;
    movieFlags &= ~MOVIE_EXTENDED_FLAG_0x01;

    if ((movieFlags & MOVIE_EXTENDED_FLAG_0x10) != 0) {
        openSubtitle(filePath);
    }

    if ((movieFlags & MOVIE_EXTENDED_FLAG_0x04) != 0) {
        debug_printf("Direct ");
        win_get_rect(GNWWin, &winRect);
        debug_printf("Playing at (%d, %d)  ", movieX + winRect.ulx, movieY + winRect.uly);
        _MVE_rmCallbacks(a3);
        _MVE_sfCallbacks(movie_MVE_ShowFrame);

        v17 = 0;
        v16 = movieY + winRect.uly;
        v15 = movieX + winRect.ulx;
    } else {
        debug_printf("Buffered ");
        _MVE_rmCallbacks(a3);
        _MVE_sfCallbacks(movieShowFrame);
        v17 = 0;
        v16 = 0;
        v15 = 0;
    }

    _MVE_rmPrepMovie((int)handle, v15, v16, v17);

    if (movieScaleFlag) {
        debug_printf("scaled\n");
    } else {
        debug_printf("not scaled\n");
    }

    if (startMovieFunc != NULL) {
        startMovieFunc(GNWWin);
    }

    if (alphaHandle != NULL) {
        int size;
        fileReadInt32(alphaHandle, &size);

        short tmp;
        fileReadInt16(alphaHandle, &tmp);
        fileReadInt16(alphaHandle, &tmp);

        alphaBuf = (unsigned char*)mymalloc(size, __FILE__, __LINE__); // "..\\int\\MOVIE.C", 1178
        alphaWindowBuf = (unsigned char*)mymalloc(movieH * movieW, __FILE__, __LINE__); // "..\\int\\MOVIE.C", 1179

        unsigned char* windowBuffer = win_get_buf(GNWWin);
        blitBufferToBuffer(windowBuffer + win_width(GNWWin) * movieY + movieX,
            movieW,
            movieH,
            win_width(GNWWin),
            alphaWindowBuf,
            movieW);
    }

    movieRect.ulx = movieX;
    movieRect.uly = movieY;
    movieRect.lrx = movieW + movieX;
    movieRect.lry = movieH + movieY;

    return 0;
}

// 0x487964
static bool localMovieCallback()
{
    doSubtitle();

    if (movieCallback != NULL) {
        movieCallback();
    }

    return _get_input() != -1;
}

// 0x487AC8
int movieRun(int win, char* filePath)
{
    if (running) {
        return 1;
    }

    movieX = 0;
    movieY = 0;
    movieOffset = 0;
    movieW = win_width(win);
    movieH = win_height(win);
    movieSubRectFlag = 0;
    return movieStart(win, filePath, noop);
}

// 0x487B1C
int movieRunRect(int win, char* filePath, int a3, int a4, int a5, int a6)
{
    if (running) {
        return 1;
    }

    movieX = a3;
    movieY = a4;
    movieOffset = a3 + a4 * win_width(win);
    movieW = a5;
    movieH = a6;
    movieSubRectFlag = 1;

    return movieStart(win, filePath, noop);
}

// 0x487B7C
static int stepMovie()
{
    if (alphaHandle != NULL) {
        int size;
        fileReadInt32(alphaHandle, &size);
        fileRead(alphaBuf, 1, size, alphaHandle);
    }

    int v1 = _MVE_rmStepMovie();
    if (v1 != -1) {
        doSubtitle();
    }

    return v1;
}

// 0x487BC8
void movieSetSubtitleFunc(MovieSubtitleFunc* func)
{
    subtitleFilenameFunc = func;
}

// 0x487BD0
void movieSetVolume(int volume)
{
    if (soundEnabled) {
        int normalizedVolume = soundVolumeHMItoDirectSound(volume);
        movieLibSetVolume(normalizedVolume);
    }
}

// 0x487BEC
void movieUpdate()
{
    if (!running) {
        return;
    }

    if ((movieFlags & MOVIE_EXTENDED_FLAG_0x02) != 0) {
        debug_printf("Movie aborted\n");
        cleanupMovie(1);
        return;
    }

    if ((movieFlags & MOVIE_EXTENDED_FLAG_0x01) != 0) {
        debug_printf("Movie error\n");
        cleanupMovie(1);
        return;
    }

    if (stepMovie() == -1) {
        cleanupMovie(1);
        return;
    }

    if (updateCallbackFunc != NULL) {
        int frame;
        int dropped;
        _MVE_rmFrameCounts(&frame, &dropped);
        updateCallbackFunc(frame);
    }
}

// 0x487C88
int moviePlaying()
{
    return running;
}
