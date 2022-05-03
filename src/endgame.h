#ifndef ENDGAME_H
#define ENDGAME_H

// Provides [MAX_PATH].
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdbool.h>

typedef enum EndgameDeathEndingReason {
    // Dude died.
    ENDGAME_DEATH_ENDING_REASON_DEATH = 0,

    // 13 years passed.
    ENDGAME_DEATH_ENDING_REASON_TIMEOUT = 2,
} EndgameDeathEndingReason;

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

extern char byte_50B00C[];

extern int gEndgameEndingSubtitlesLength;
extern int gEndgameEndingSubtitlesCharactersCount;
extern int gEndgameEndingSubtitlesCurrentLine;
extern int dword_518674;
extern EndgameDeathEnding* gEndgameDeathEndings;
extern int gEndgameDeathEndingsLength;

extern char gEndgameDeathEndingFileName[40];
extern bool gEndgameEndingVoiceOverSpeechLoaded;
extern char gEndgameEndingSubtitlesLocalizedPath[MAX_PATH];
extern bool gEndgameEndingSpeechEnded;
extern EndgameEnding* gEndgameEndings;
extern char** gEndgameEndingSubtitles;
extern bool gEndgameEndingSubtitlesEnabled;
extern bool gEndgameEndingSubtitlesEnded;
extern bool dword_570BD4;
extern bool dword_570BD8;
extern int gEndgameEndingsLength;
extern bool gEndgameEndingVoiceOverSubtitlesLoaded;
extern unsigned int gEndgameEndingSubtitlesReferenceTime;
extern unsigned int* gEndgameEndingSubtitlesTimings;
extern int gEndgameEndingSlideshowOldFont;
extern unsigned char* gEndgameEndingSlideshowWindowBuffer;
extern int gEndgameEndingSlideshowWindow;

void endgamePlaySlideshow();
void endgamePlayMovie();
void endgameEndingRenderPanningScene(int direction, const char* narratorFileName);
void endgameEndingRenderStaticScene(int fid, const char* narratorFileName);
int endgameEndingHandleContinuePlaying();
int endgameEndingSlideshowWindowInit();
void endgameEndingSlideshowWindowFree();
void endgameEndingVoiceOverInit(const char* fname);
void endgameEndingVoiceOverReset();
void endgameEndingVoiceOverFree();
void endgameEndingLoadPalette(int type, int id);
void sub_4403F0();
int endgameEndingSubtitlesLoad(const char* filePath);
void endgameEndingRefreshSubtitles();
void endgameEndingSubtitlesFree();
void sub_440728();
void sub_440734();
int endgameEndingInit();
void endgameEndingFree();
int endgameDeathEndingInit();
int endgameDeathEndingExit();
void endgameSetupDeathEnding(int reason);
int endgameDeathEndingValidate(int* percentage);
char* endgameDeathEndingGetFileName();

#endif /* ENDGAME_H */
