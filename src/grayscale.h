#ifndef GRAYSCALE_H
#define GRAYSCALE_H

extern unsigned char byte_596D90[256];

void grayscalePaletteUpdate(int a1, int a2);
void grayscalePaletteApply(unsigned char* surface, int width, int height, int pitch);

#endif /* GRAYSCALE_H */
