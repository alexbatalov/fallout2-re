#ifndef FALLOUT_PLIB_GNW_SVGA_H_
#define FALLOUT_PLIB_GNW_SVGA_H_

#include <stdbool.h>

#include "plib/gnw/gnw95dx.h"
#include "plib/gnw/rect.h"
#include "plib/gnw/svga_types.h"

extern LPDIRECTDRAW GNW95_DDObject;
extern LPDIRECTDRAWSURFACE GNW95_DDPrimarySurface;
extern LPDIRECTDRAWSURFACE GNW95_DDRestoreSurface;
extern LPDIRECTDRAWPALETTE GNW95_DDPrimaryPalette;
extern UpdatePaletteFunc* update_palette_func;
extern bool mmxEnabled;

extern unsigned short GNW95_Pal16[256];
extern Rect scr_size;
extern unsigned int w95rmask;
extern unsigned int w95gmask;
extern unsigned int w95bmask;
extern int w95bshift;
extern int w95rshift;
extern int w95gshift;
extern ScreenBlitFunc* scr_blit;
extern ZeroMemFunc* zero_mem;

void mmxEnable(bool enable);
int init_mode_320_200();
int init_mode_320_400();
int init_mode_640_480_16();
int init_mode_640_480();
int init_mode_640_400();
int init_mode_800_600();
int init_mode_1024_768();
int init_mode_1280_1024();
int init_vesa_mode(int mode, int width, int height, int half);
int get_start_mode();
void reset_mode();
void zero_vid_mem();
int GNW95_init_window();
int GNW95_init_DirectDraw(int width, int height, int bpp);
void GNW95_reset_mode();
void GNW95_SetPaletteEntry(int entry, unsigned char r, unsigned char g, unsigned char b);
void GNW95_SetPaletteEntries(unsigned char* a1, int a2, int a3);
void GNW95_SetPalette(unsigned char* palette);
unsigned char* GNW95_GetPalette();
void GNW95_ShowRect(unsigned char* src, int src_pitch, int a3, int src_x, int src_y, int src_width, int src_height, int dest_x, int dest_y);
void GNW95_MouseShowRect16(unsigned char* src, int srcPitch, int a3, int srcX, int srcY, int srcWidth, int srcHeight, int destX, int destY);
void GNW95_ShowRect16(unsigned char* src, int srcPitch, int a3, int srcX, int srcY, int srcWidth, int srcHeight, int destX, int destY);
void GNW95_MouseShowTransRect16(unsigned char* src, int srcPitch, int a3, int srcX, int srcY, int srcWidth, int srcHeight, int destX, int destY, unsigned char keyColor);
void GNW95_zero_vid_mem();

#endif /* FALLOUT_PLIB_GNW_SVGA_H_ */
