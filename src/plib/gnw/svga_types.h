#ifndef FALLOUT_PLIB_GNW_SVGA_TYPES_H_
#define FALLOUT_PLIB_GNW_SVGA_TYPES_H_

// NOTE: These typedefs always appear in this order in every implementation file
// with extended debug info. However `mouse.c` does not have DirectX types
// implying it does not include `svga.h` which does so to expose primary
// DirectDraw objects.

typedef void(UpdatePaletteFunc)();
typedef void(ZeroMemFunc)();
typedef void(ResetModeFunc)();
typedef int(SetModeFunc)();
typedef void(ScreenTransBlitFunc)(unsigned char* buf, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, unsigned char a10);
typedef void(ScreenBlitFunc)(unsigned char* src, int src_pitch, int a3, int src_x, int src_y, int src_width, int src_height, int dest_x, int dest_y);

#endif /* FALLOUT_PLIB_GNW_SVGA_TYPES_H_ */
