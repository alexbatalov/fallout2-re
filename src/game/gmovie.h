#ifndef FALLOUT_GAME_GMOVIE_H_
#define FALLOUT_GAME_GMOVIE_H_

#include <stdbool.h>

#include "db.h"

typedef enum GameMovieFlags {
    GAME_MOVIE_FADE_IN = 0x01,
    GAME_MOVIE_FADE_OUT = 0x02,
    GAME_MOVIE_STOP_MUSIC = 0x04,
    GAME_MOVIE_PAUSE_MUSIC = 0x08,
} GameMovieFlags;

typedef enum GameMovie {
    MOVIE_IPLOGO,
    MOVIE_INTRO,
    MOVIE_ELDER,
    MOVIE_VSUIT,
    MOVIE_AFAILED,
    MOVIE_ADESTROY,
    MOVIE_CAR,
    MOVIE_CARTUCCI,
    MOVIE_TIMEOUT,
    MOVIE_TANKER,
    MOVIE_ENCLAVE,
    MOVIE_DERRICK,
    MOVIE_ARTIMER1,
    MOVIE_ARTIMER2,
    MOVIE_ARTIMER3,
    MOVIE_ARTIMER4,
    MOVIE_CREDITS,
    MOVIE_COUNT,
} GameMovie;

int gmovie_init();
void gmovie_reset();
int gmovie_load(File* stream);
int gmovie_save(File* stream);
int gmovie_play(int movie, int flags);
void gmPaletteFinish();
bool gmovie_has_been_played(int movie);
bool gmovieIsPlaying();

#endif /* FALLOUT_GAME_GMOVIE_H_ */
