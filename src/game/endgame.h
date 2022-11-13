#ifndef FALLOUT_GAME_ENDGAME_H_
#define FALLOUT_GAME_ENDGAME_H_

typedef enum EndgameDeathEndingReason {
    // Dude died.
    ENDGAME_DEATH_ENDING_REASON_DEATH = 0,

    // 13 years passed.
    ENDGAME_DEATH_ENDING_REASON_TIMEOUT = 2,
} EndgameDeathEndingReason;

extern char _aEnglish_2[];

void endgame_slideshow();
void endgame_movie();
int endgameEndingHandleContinuePlaying();
int endgameDeathEndingInit();
int endgameDeathEndingExit();
void endgameSetupDeathEnding(int reason);
char* endgameGetDeathEndingFileName();

#endif /* FALLOUT_GAME_ENDGAME_H_ */
