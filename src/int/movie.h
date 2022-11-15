#ifndef MOVIE_H
#define MOVIE_H

#include "db.h"
#include "geometry.h"
#include "win32.h"

typedef enum MovieFlags {
    MOVIE_FLAG_0x01 = 0x01,
    MOVIE_FLAG_0x02 = 0x02,
    MOVIE_FLAG_0x04 = 0x04,
    MOVIE_FLAG_0x08 = 0x08,
} MovieFlags;

typedef enum MovieExtendedFlags {
    MOVIE_EXTENDED_FLAG_0x01 = 0x01,
    MOVIE_EXTENDED_FLAG_0x02 = 0x02,
    MOVIE_EXTENDED_FLAG_0x04 = 0x04,
    MOVIE_EXTENDED_FLAG_0x08 = 0x08,
    MOVIE_EXTENDED_FLAG_0x10 = 0x10,
} MovieExtendedFlags;

typedef struct MovieSubtitleListNode {
    int num;
    char* text;
    struct MovieSubtitleListNode* next;
} MovieSubtitleListNode;

typedef char* MovieBuildSubtitleFilePathProc(char* movieFilePath);
typedef void MovieSetPaletteEntriesProc(unsigned char* palette, int start, int end);
typedef void MovieSetPaletteProc(int frame);
typedef void(MovieFrameGrabProc)(unsigned char* data, int width, int height, int pitch);
typedef void(MovieCaptureFrameProc)(unsigned char* data, int width, int height, int pitch, int movieX, int movieY, int movieWidth, int movieHeight);
typedef int(MovieBlitFunc)(int win, unsigned char* data, int width, int height, int pitch);
typedef void(MoviePreDrawFunc)(int win, Rect* rect);
typedef void(MovieStartFunc)(int win);
typedef void(MovieEndFunc)(int win, int x, int y, int width, int height);
typedef int(MovieFailedOpenFunc)(char* path);

extern int gMovieWindow;
extern int gMovieSubtitlesFont;
extern MovieBlitFunc* gMovieBlitFuncs[2][2][2];
extern MovieSetPaletteEntriesProc* gMovieSetPaletteEntriesProc;
extern int gMovieSubtitlesColorR;
extern int gMovieSubtitlesColorG;
extern int gMovieSubtitlesColorB;

extern Rect gMovieWindowRect;
extern Rect _movieRect;
extern void (*_movieCallback)();
extern MovieEndFunc* _endMovieFunc;
extern MovieSetPaletteProc* gMoviePaletteProc;
extern MovieFailedOpenFunc* _failedOpenFunc;
extern MovieBuildSubtitleFilePathProc* gMovieBuildSubtitleFilePathProc;
extern MovieStartFunc* _startMovieFunc;
extern int _subtitleW;
extern int _lastMovieBH;
extern int _lastMovieBW;
extern int _lastMovieSX;
extern int _lastMovieSY;
extern int _movieScaleFlag;
extern MoviePreDrawFunc* _moviePreDrawFunc;
extern int _lastMovieH;
extern int _lastMovieW;
extern int _lastMovieX;
extern int _lastMovieY;
extern MovieSubtitleListNode* gMovieSubtitleHead;
extern unsigned int gMovieFlags;
extern int _movieAlphaFlag;
extern bool _movieSubRectFlag;
extern int _movieH;
extern int _movieOffset;
extern MovieCaptureFrameProc* _movieCaptureFrameFunc;
extern unsigned char* _lastMovieBuffer;
extern int _movieW;
extern MovieFrameGrabProc* _movieFrameGrabFunc;
extern LPDIRECTDRAWSURFACE gMovieDirectDrawSurface;
extern int _subtitleH;
extern int _running;
extern File* gMovieFileStream;
extern unsigned char* _alphaWindowBuf;
extern int _movieX;
extern int _movieY;
extern bool gMovieDirectSoundInitialized;
extern File* _alphaHandle;
extern unsigned char* _alphaBuf;

void _movieSetPreDrawFunc(MoviePreDrawFunc* preDrawFunc);
void _movieSetFailedOpenFunc(MovieFailedOpenFunc* failedOpenFunc);
void _movieSetFunc(MovieStartFunc* startFunc, MovieEndFunc* endFunc);
void* movieMallocImpl(size_t size);
void movieFreeImpl(void* ptr);
bool movieReadImpl(int fileHandle, void* buf, int count);
void movieDirectImpl(LPDIRECTDRAWSURFACE a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9);
void movieBufferedImpl(LPDIRECTDRAWSURFACE a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9);
void _movieSetFrameGrabFunc(MovieFrameGrabProc* func);
void _movieSetCaptureFrameFunc(MovieCaptureFrameProc* func);
int _movieScaleSubRect(int win, unsigned char* data, int width, int height, int pitch);
int _movieScaleSubRectAlpha(int win, unsigned char* data, int width, int height, int pitch);
int _movieScaleWindowAlpha(int win, unsigned char* data, int width, int height, int pitch);
int _blitAlpha(int win, unsigned char* data, int width, int height, int pitch);
int _movieScaleWindow(int win, unsigned char* data, int width, int height, int pitch);
int _blitNormal(int win, unsigned char* data, int width, int height, int pitch);
void movieSetPaletteEntriesImpl(unsigned char* palette, int start, int end);
int _noop();
void movieInit();
void _cleanupMovie(int a1);
void movieExit();
void _movieStop();
int movieSetFlags(int a1);
void _movieSetPaletteFunc(MovieSetPaletteEntriesProc* proc);
void movieSetPaletteProc(MovieSetPaletteProc* proc);
void _cleanupLast();
File* movieOpen(char* filePath);
void movieLoadSubtitles(char* filePath);
void movieRenderSubtitles();
int _movieStart(int win, char* filePath, int (*a3)());
bool _localMovieCallback();
int _movieRun(int win, char* filePath);
int _movieRunRect(int win, char* filePath, int a3, int a4, int a5, int a6);
int _stepMovie();
void movieSetBuildSubtitleFilePathProc(MovieBuildSubtitleFilePathProc* proc);
void movieSetVolume(int volume);
void _movieUpdate();
int _moviePlaying();

#endif /* MOVIE_H */
