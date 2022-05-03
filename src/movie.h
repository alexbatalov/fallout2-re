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
int sub_486C74(int a1);
int sub_486C80(int win, unsigned char* a2, int a3, int a4, int a5);
int sub_486D84(int win, int a2, int a3, int a4, int a5);
void movieSetPaletteEntriesImpl(unsigned char* palette, int start, int end);
int sub_486E08();
void movieInit();
void sub_486E98(int a1);
void movieExit();
void sub_487150();
int movieSetFlags(int a1);
void sub_48725C(MovieSetPaletteEntriesProc* proc);
void movieSetPaletteProc(MovieSetPaletteProc* proc);
void sub_4872E8();
File* movieOpen(char* filePath);
void movieLoadSubtitles(char* filePath);
void movieRenderSubtitles();
int sub_487710(int win, char* filePath, int (*a3)());
bool sub_487964();
int sub_487AC8(int win, char* filePath);
int sub_487B1C(int win, char* filePath, int a3, int a4, int a5, int a6);
int sub_487B7C();
void movieSetBuildSubtitleFilePathProc(MovieBuildSubtitleFilePathProc* proc);
void movieSetVolume(int volume);
void sub_487BEC();
int sub_487C88();

#endif /* MOVIE_H */
