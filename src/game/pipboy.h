#ifndef FALLOUT_GAME_PIPBOY_H_
#define FALLOUT_GAME_PIPBOY_H_

#include <stdbool.h>

#include "game/art.h"
#include "plib/db/db.h"
#include "plib/gnw/rect.h"
#include "game/message.h"

typedef enum PipboyOpenIntent {
    PIPBOY_OPEN_INTENT_UNSPECIFIED = 0,
    PIPBOY_OPEN_INTENT_REST = 1,
} PipboyOpenIntent;

typedef void(PipboyRenderProc)(int a1);

PipboyRenderProc* PipFnctn[5];

int pipboy(int intent);
void pip_init();
int save_pipboy(File* stream);
int load_pipboy(File* stream);

#endif /* FALLOUT_GAME_PIPBOY_H_ */
