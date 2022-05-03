#ifndef MOVIE_EFFECT_H
#define MOVIE_EFFECT_H

#include <stdbool.h>

typedef enum MovieEffectType {
    MOVIE_EFFECT_TYPE_NONE = 0,
    MOVIE_EFFECT_TYPE_FADE_IN = 1,
    MOVIE_EFFECT_TYPE_FADE_OUT = 2,
} MovieEffectFadeType;

typedef struct MovieEffect {
    int startFrame;
    int endFrame;
    int steps;
    unsigned char fadeType;
    // range 0-63
    unsigned char r;
    // range 0-63
    unsigned char g;
    // range 0-63
    unsigned char b;
    struct MovieEffect* next;
} MovieEffect;

extern bool gMovieEffectsInitialized;
extern MovieEffect* gMovieEffectHead;

extern unsigned char byte_638EC4[768];
extern bool dword_6391C4;

int movieEffectsInit();
void movieEffectsReset();
void movieEffectsExit();
int movieEffectsLoad(const char* fileName);
void sub_4880F0();
void sub_488144(int frame);
void sub_4882AC(unsigned char* palette, int start, int end);
void movieEffectsClear();

#endif /* MOVIE_EFFECT_H */
