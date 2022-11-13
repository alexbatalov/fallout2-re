#ifndef FALLOUT_GAME_GRAPHLIB_H_
#define FALLOUT_GAME_GRAPHLIB_H_

extern int* dad;
extern int match_length;
extern int textsize;
extern int* rson;
extern int* lson;
extern unsigned char* text_buf;
extern int codesize;
extern int match_position;

int HighRGB(int a1);
int CompLZS(unsigned char* a1, unsigned char* a2, int a3);
int DecodeLZS(unsigned char* a1, unsigned char* a2, int a3);

#endif /* FALLOUT_GAME_GRAPHLIB_H_ */
