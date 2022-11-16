#ifndef FALLOUT_GAME_WORDWRAP_H_
#define FALLOUT_GAME_WORDWRAP_H_

#define WORD_WRAP_MAX_COUNT 64

int word_wrap(const char* string, int width, short* breakpoints, short* breakpointsLengthPtr);

#endif /* FALLOUT_GAME_WORDWRAP_H_ */
