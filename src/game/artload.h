#ifndef FALLOUT_GAME_ARTLOAD_H_
#define FALLOUT_GAME_ARTLOAD_H_

#include "game/art.h"
#include "db.h"

int load_frame(const char* path, Art** artPtr);
int load_frame_into(const char* path, unsigned char* data);
int art_writeSubFrameData(unsigned char* data, File* stream, int count);
int art_writeFrameData(Art* art, File* stream);
int save_frame(const char* path, unsigned char* data);

#endif /* FALLOUT_GAME_ARTLOAD_H_ */
