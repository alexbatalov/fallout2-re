#ifndef FALLOUT_GAME_MOVIEFX_H_
#define FALLOUT_GAME_MOVIEFX_H_

int moviefx_init();
void moviefx_reset();
void moviefx_exit();
int moviefx_start(const char* fileName);
void moviefx_stop();

#endif /* FALLOUT_GAME_MOVIEFX_H_ */
