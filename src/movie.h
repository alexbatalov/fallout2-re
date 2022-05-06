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

extern int gMovieWindow;
extern int gMovieSubtitlesFont;
extern MovieSetPaletteEntriesProc* gMovieSetPaletteEntriesProc;
extern int gMovieSubtitlesColorR;
extern int gMovieSubtitlesColorG;
extern int gMovieSubtitlesColorB;

extern Rect gMovieWindowRect;
extern Rect stru_638E20;
extern void (*off_638E30)();
extern MovieSetPaletteProc* gMoviePaletteProc;
extern int (*off_638E3C)(char* filePath);
extern MovieBuildSubtitleFilePathProc* gMovieBuildSubtitleFilePathProc;
extern int dword_638E48;
extern int dword_638E4C;
extern int dword_638E50;
extern int dword_638E54;
extern int dword_638E58;
extern int dword_638E5C;
extern int dword_638E64;
extern int dword_638E68;
extern int dword_638E6C;
extern int dword_638E70;
extern MovieSubtitleListNode* gMovieSubtitleHead;
extern MovieExtendedFlags gMovieFlags;
extern int dword_638E7C;
extern bool dword_638E80;
extern int dword_638E84;
extern int dword_638E88;
extern void (*off_638E8C)(void*, int, int, int, int, int);
extern unsigned char* off_638E90;
extern int dword_638E94;
extern void (*off_638E98)();
extern LPDIRECTDRAWSURFACE gMovieDirectDrawSurface;
extern int dword_638EA0;
extern int dword_638EA4;
extern File* gMovieFileStream;
extern unsigned char* off_638EAC;
extern int dword_638EB0;
extern int dword_638EB4;
extern bool gMovieDirectSoundInitialized;
extern File* off_638EBC;
extern unsigned char* off_638EC0;

void* movieMallocImpl(size_t size);
void movieFreeImpl(void* ptr);
bool movieReadImpl(int fileHandle, void* buf, int count);
void movieDirectImpl(LPDIRECTDRAWSURFACE a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9);
void movieBufferedImpl(LPDIRECTDRAWSURFACE a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9);
int movieScaleSubRectAlpha(int a1);
int blitAlpha(int win, unsigned char* a2, int a3, int a4, int a5);
int blitNormal(int win, int a2, int a3, int a4, int a5);
void movieSetPaletteEntriesImpl(unsigned char* palette, int start, int end);
int noop();
void movieInit();
void cleanupMovie(int a1);
void movieExit();
void movieStop();
int movieSetFlags(int a1);
void movieSetPaletteFunc(MovieSetPaletteEntriesProc* proc);
void movieSetPaletteProc(MovieSetPaletteProc* proc);
void cleanupLast();
File* movieOpen(char* filePath);
void movieLoadSubtitles(char* filePath);
void movieRenderSubtitles();
int movieStart(int win, char* filePath, int (*a3)());
bool localMovieCallback();
int movieRun(int win, char* filePath);
int movieRunRect(int win, char* filePath, int a3, int a4, int a5, int a6);
int stepMovie();
void movieSetBuildSubtitleFilePathProc(MovieBuildSubtitleFilePathProc* proc);
void movieSetVolume(int volume);
void movieUpdate();
int moviePlaying();

#endif /* MOVIE_H */
