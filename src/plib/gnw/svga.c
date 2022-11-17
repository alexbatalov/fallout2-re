#include "plib/gnw/svga.h"

#include "mmx.h"
#include "plib/gnw/gnw.h"
#include "plib/gnw/grbuf.h"
#include "plib/gnw/mouse.h"
#include "plib/gnw/winmain.h"

static int GNW95_init_mode_ex(int width, int height, int bpp);
static int GNW95_init_mode(int width, int height);
static int ffs(int bits);

// 0x51E2B0
LPDIRECTDRAW GNW95_DDObject = NULL;

// 0x51E2B4
LPDIRECTDRAWSURFACE GNW95_DDPrimarySurface = NULL;

// 0x51E2B8
LPDIRECTDRAWSURFACE GNW95_DDRestoreSurface = NULL;

// 0x51E2BC
LPDIRECTDRAWPALETTE GNW95_DDPrimaryPalette = NULL;

// 0x51E2C4
UpdatePaletteFunc* update_palette_func = NULL;

// 0x51E2C8
bool mmxEnabled = true;

// 0x6AC7F0
unsigned short GNW95_Pal16[256];

// screen rect
Rect scr_size;

// 0x6ACA00
unsigned int w95gmask;

// 0x6ACA04
unsigned int w95rmask;

// 0x6ACA08
unsigned int w95bmask;

// 0x6ACA0C
int w95bshift;

// 0x6ACA10
int w95rshift;

// 0x6ACA14
int w95gshift;

// 0x6ACA18
ScreenBlitFunc* scr_blit = GNW95_ShowRect;

// 0x6ACA1C
ZeroMemFunc* zero_mem = NULL;

// 0x4CACD0
void mmxEnable(bool enable)
{
    // 0x51E2CC
    static bool inited = false;

    // 0x6ACA20
    static bool mmx;

    if (!inited) {
        mmx = mmxIsSupported();
        inited = true;
    }

    if (mmx) {
        mmxEnabled = enable;
    }
}

// 0x4CAD08
int init_mode_320_200()
{
    return GNW95_init_mode_ex(320, 200, 8);
}

// 0x4CAD40
int init_mode_320_400()
{
    return GNW95_init_mode_ex(320, 400, 8);
}

// 0x4CAD5C
int init_mode_640_480_16()
{
    return -1;
}

// 0x4CAD64
int init_mode_640_480()
{
    return GNW95_init_mode(640, 480);
}

// 0x4CAD94
int init_mode_640_400()
{
    return GNW95_init_mode(640, 400);
}

// 0x4CADA8
int init_mode_800_600()
{
    return GNW95_init_mode(800, 600);
}

// 0x4CADBC
int init_mode_1024_768()
{
    return GNW95_init_mode(1024, 768);
}

// 0x4CADD0
int init_mode_1280_1024()
{
    return GNW95_init_mode(1280, 1024);
}

// 0x4CADE4
int init_vesa_mode(int mode, int width, int height, int half)
{
    if (half != 0) {
        return -1;
    }

    return GNW95_init_mode_ex(width, height, 8);
}

// 0x4CADF3
int get_start_mode()
{
    return -1;
}

// 0x4CADF8
void reset_mode()
{
}

// 0x4CADFC
void zero_vid_mem()
{
    if (zero_mem) {
        zero_mem();
    }
}

// 0x4CAE1C
static int GNW95_init_mode_ex(int width, int height, int bpp)
{
    if (GNW95_init_window() == -1) {
        return -1;
    }

    if (GNW95_init_DirectDraw(width, height, bpp) == -1) {
        return -1;
    }

    scr_size.ulx = 0;
    scr_size.uly = 0;
    scr_size.lrx = width - 1;
    scr_size.lry = height - 1;

    mmxEnable(true);

    if (bpp == 8) {
        mouse_blit_trans = NULL;
        scr_blit = GNW95_ShowRect;
        zero_mem = GNW95_zero_vid_mem;
        mouse_blit = GNW95_ShowRect;
    } else {
        zero_mem = NULL;
        mouse_blit = GNW95_MouseShowRect16;
        mouse_blit_trans = GNW95_MouseShowTransRect16;
        scr_blit = GNW95_ShowRect16;
    }

    return 0;
}

// 0x4CAECC
static int GNW95_init_mode(int width, int height)
{
    return GNW95_init_mode_ex(width, height, 8);
}

// 0x4CAEDC
int GNW95_init_window()
{
    if (GNW95_hwnd == NULL) {
        int width = GetSystemMetrics(SM_CXSCREEN);
        int height = GetSystemMetrics(SM_CYSCREEN);

        GNW95_hwnd = CreateWindowExA(WS_EX_TOPMOST, "GNW95 Class", GNW95_title, WS_POPUP | WS_VISIBLE | WS_SYSMENU, 0, 0, width, height, NULL, NULL, GNW95_hInstance, NULL);
        if (GNW95_hwnd == NULL) {
            return -1;
        }

        UpdateWindow(GNW95_hwnd);
        SetFocus(GNW95_hwnd);
    }

    return 0;
}

// calculate shift for mask
// 0x4CAF50
static int ffs(int bits)
{
    int shift = 0;

    if ((bits & 0xFFFF0000) != 0) {
        shift |= 16;
        bits &= 0xFFFF0000;
    }

    if ((bits & 0xFF00FF00) != 0) {
        shift |= 8;
        bits &= 0xFF00FF00;
    }

    if ((bits & 0xF0F0F0F0) != 0) {
        shift |= 4;
        bits &= 0xF0F0F0F0;
    }

    if ((bits & 0xCCCCCCCC) != 0) {
        shift |= 2;
        bits &= 0xCCCCCCCC;
    }

    if ((bits & 0xAAAAAAAA) != 0) {
        shift |= 1;
    }

    return shift;
}

// 0x4CAF9C
int GNW95_init_DirectDraw(int width, int height, int bpp)
{
    if (GNW95_DDObject != NULL) {
        unsigned char* palette = GNW95_GetPalette();
        GNW95_reset_mode();

        if (GNW95_init_DirectDraw(width, height, bpp) == -1) {
            return -1;
        }

        GNW95_SetPalette(palette);

        return 0;
    }

    if (GNW95_DirectDrawCreate(NULL, &GNW95_DDObject, NULL) != DD_OK) {
        return -1;
    }

    if (IDirectDraw_SetCooperativeLevel(GNW95_DDObject, GNW95_hwnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN) != DD_OK) {
        return -1;
    }

    if (IDirectDraw_SetDisplayMode(GNW95_DDObject, width, height, bpp) != DD_OK) {
        return -1;
    }

    DDSURFACEDESC ddsd;
    memset(&ddsd, 0, sizeof(DDSURFACEDESC));

    ddsd.dwSize = sizeof(DDSURFACEDESC);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    if (IDirectDraw_CreateSurface(GNW95_DDObject, &ddsd, &GNW95_DDPrimarySurface, NULL) != DD_OK) {
        return -1;
    }

    GNW95_DDRestoreSurface = GNW95_DDPrimarySurface;

    if (bpp == 8) {
        PALETTEENTRY pe[256];
        for (int index = 0; index < 256; index++) {
            pe[index].peRed = index;
            pe[index].peGreen = index;
            pe[index].peBlue = index;
            pe[index].peFlags = 0;
        }

        if (IDirectDraw_CreatePalette(GNW95_DDObject, DDPCAPS_8BIT | DDPCAPS_ALLOW256, pe, &GNW95_DDPrimaryPalette, NULL) != DD_OK) {
            return -1;
        }

        if (IDirectDrawSurface_SetPalette(GNW95_DDPrimarySurface, GNW95_DDPrimaryPalette) != DD_OK) {
            return -1;
        }

        return 0;
    } else {
        DDPIXELFORMAT ddpf;
        ddpf.dwSize = sizeof(DDPIXELFORMAT);

        if (IDirectDrawSurface_GetPixelFormat(GNW95_DDPrimarySurface, &ddpf) != DD_OK) {
            return -1;
        }

        w95rmask = ddpf.dwRBitMask;
        w95gmask = ddpf.dwGBitMask;
        w95bmask = ddpf.dwBBitMask;

        w95rshift = ffs(w95rmask) - 7;
        w95gshift = ffs(w95gmask) - 7;
        w95bshift = ffs(w95bmask) - 7;

        return 0;
    }
}

// 0x4CB1B0
void GNW95_reset_mode()
{
    if (GNW95_DDObject != NULL) {
        IDirectDraw_RestoreDisplayMode(GNW95_DDObject);

        if (GNW95_DDPrimarySurface != NULL) {
            IDirectDrawSurface_Release(GNW95_DDPrimarySurface);
            GNW95_DDPrimarySurface = NULL;
            GNW95_DDRestoreSurface = NULL;
        }

        if (GNW95_DDPrimaryPalette != NULL) {
            IDirectDrawPalette_Release(GNW95_DDPrimaryPalette);
            GNW95_DDPrimaryPalette = NULL;
        }

        IDirectDraw_Release(GNW95_DDObject);
        GNW95_DDObject = NULL;
    }
}

// 0x4CB218
void GNW95_SetPaletteEntry(int entry, unsigned char r, unsigned char g, unsigned char b)
{
    PALETTEENTRY tempEntry;

    r <<= 2;
    g <<= 2;
    b <<= 2;

    if (GNW95_DDPrimaryPalette != NULL) {
        tempEntry.peRed = r;
        tempEntry.peGreen = g;
        tempEntry.peBlue = b;
        tempEntry.peFlags = PC_NOCOLLAPSE;
        IDirectDrawPalette_SetEntries(GNW95_DDPrimaryPalette, 0, entry, 1, &tempEntry);
    } else {
        GNW95_Pal16[entry] = ((w95rshift > 0 ? (r << w95rshift) : (r >> -w95rshift)) & w95rmask)
            | ((w95gshift > 0 ? (g << w95gshift) : (r >> -w95gshift)) & w95gmask)
            | ((w95bshift > 0 ? (b << w95bshift) : (r >> -w95bshift)) & w95bmask);
        win_refresh_all(&scr_size);
    }

    if (update_palette_func != NULL) {
        update_palette_func();
    }
}

// 0x4CB310
void GNW95_SetPaletteEntries(unsigned char* palette, int start, int count)
{
    if (GNW95_DDPrimaryPalette != NULL) {
        PALETTEENTRY entries[256];

        if (count != 0) {
            for (int index = 0; index < count; index++) {
                entries[index].peRed = palette[index * 3] << 2;
                entries[index].peGreen = palette[index * 3 + 1] << 2;
                entries[index].peBlue = palette[index * 3 + 2] << 2;
                entries[index].peFlags = PC_NOCOLLAPSE;
            }
        }

        IDirectDrawPalette_SetEntries(GNW95_DDPrimaryPalette, 0, start, count, entries);
    } else {
        for (int index = start; index < start + count; index++) {
            unsigned short r = palette[0] << 2;
            unsigned short g = palette[1] << 2;
            unsigned short b = palette[2] << 2;
            palette += 3;

            r = w95rshift > 0 ? (r << w95rshift) : (r >> -w95rshift);
            r &= w95rmask;

            g = w95gshift > 0 ? (g << w95gshift) : (g >> -w95gshift);
            g &= w95gmask;

            b = w95bshift > 0 ? (b << w95bshift) : (b >> -w95bshift);
            b &= w95bmask;

            unsigned short rgb = r | g | b;
            GNW95_Pal16[index] = rgb;
        }

        win_refresh_all(&scr_size);
    }

    if (update_palette_func != NULL) {
        update_palette_func();
    }
}

// 0x4CB568
void GNW95_SetPalette(unsigned char* palette)
{
    if (GNW95_DDPrimaryPalette != NULL) {
        PALETTEENTRY entries[256];

        for (int index = 0; index < 256; index++) {
            entries[index].peRed = palette[index * 3] << 2;
            entries[index].peGreen = palette[index * 3 + 1] << 2;
            entries[index].peBlue = palette[index * 3 + 2] << 2;
            entries[index].peFlags = PC_NOCOLLAPSE;
        }

        IDirectDrawPalette_SetEntries(GNW95_DDPrimaryPalette, 0, 0, 256, entries);
    } else {
        for (int index = 0; index < 256; index++) {
            unsigned short r = palette[index * 3] << 2;
            unsigned short g = palette[index * 3 + 1] << 2;
            unsigned short b = palette[index * 3 + 2] << 2;

            r = w95rshift > 0 ? (r << w95rshift) : (r >> -w95rshift);
            r &= w95rmask;

            g = w95gshift > 0 ? (g << w95gshift) : (g >> -w95gshift);
            g &= w95gmask;

            b = w95bshift > 0 ? (b << w95bshift) : (b >> -w95bshift);
            b &= w95bmask;

            unsigned short rgb = r | g | b;
            GNW95_Pal16[index] = rgb;
        }

        win_refresh_all(&scr_size);
    }

    if (update_palette_func != NULL) {
        update_palette_func();
    }
}

// 0x4CB68C
unsigned char* GNW95_GetPalette()
{
    // FIXME: This buffer was supposed to be used as temporary place to store
    // current palette while switching video modes (changing resolution). However
    // the original game does not have UI to change video mode. Even if it did this
    // buffer it too small to hold the entire palette, which require 256 * 3 bytes.
    //
    // 0x6ACA24
    static unsigned char cmap[256];

    if (GNW95_DDPrimaryPalette != NULL) {
        PALETTEENTRY paletteEntries[256];
        if (IDirectDrawPalette_GetEntries(GNW95_DDPrimaryPalette, 0, 0, 256, paletteEntries) != DD_OK) {
            return NULL;
        }

        for (int index = 0; index < 256; index++) {
            PALETTEENTRY* paletteEntry = &(paletteEntries[index]);
            cmap[index * 3] = paletteEntry->peRed >> 2;
            cmap[index * 3 + 1] = paletteEntry->peGreen >> 2;
            cmap[index * 3 + 2] = paletteEntry->peBlue >> 2;
        }

        return cmap;
    }

    int redShift = w95rshift + 2;
    int greenShift = w95gshift + 2;
    int blueShift = w95bshift + 2;

    for (int index = 0; index < 256; index++) {
        unsigned short rgb = GNW95_Pal16[index];

        unsigned short r = redShift > 0 ? ((rgb & w95rmask) >> redShift) : ((rgb & w95rmask) << -redShift);
        unsigned short g = greenShift > 0 ? ((rgb & w95gmask) >> greenShift) : ((rgb & w95gmask) << -greenShift);
        unsigned short b = blueShift > 0 ? ((rgb & w95bmask) >> blueShift) : ((rgb & w95bmask) << -blueShift);

        cmap[index * 3] = (r >> 2) & 0xFF;
        cmap[index * 3 + 1] = (g >> 2) & 0xFF;
        cmap[index * 3 + 2] = (b >> 2) & 0xFF;
    }

    return cmap;
}

// 0x4CB850
void GNW95_ShowRect(unsigned char* src, int srcPitch, int a3, int srcX, int srcY, int srcWidth, int srcHeight, int destX, int destY)
{
    DDSURFACEDESC ddsd;
    HRESULT hr;

    if (!GNW95_isActive) {
        return;
    }

    while (1) {
        ddsd.dwSize = sizeof(DDSURFACEDESC);

        hr = IDirectDrawSurface_Lock(GNW95_DDPrimarySurface, NULL, &ddsd, 1, NULL);
        if (hr == DD_OK) {
            break;
        }

        if (hr == DDERR_SURFACELOST) {
            if (IDirectDrawSurface_Restore(GNW95_DDRestoreSurface) != DD_OK) {
                return;
            }
        }
    }

    buf_to_buf(src + srcPitch * srcY + srcX, srcWidth, srcHeight, srcPitch, (unsigned char*)ddsd.lpSurface + ddsd.lPitch * destY + destX, ddsd.lPitch);

    IDirectDrawSurface_Unlock(GNW95_DDPrimarySurface, ddsd.lpSurface);
}

// 0x4CB93C
void GNW95_MouseShowRect16(unsigned char* src, int srcPitch, int a3, int srcX, int srcY, int srcWidth, int srcHeight, int destX, int destY)
{
    DDSURFACEDESC ddsd;
    HRESULT hr;

    if (!GNW95_isActive) {
        return;
    }

    while (1) {
        ddsd.dwSize = sizeof(ddsd);

        hr = IDirectDrawSurface_Lock(GNW95_DDPrimarySurface, NULL, &ddsd, 1, NULL);
        if (hr == DD_OK) {
            break;
        }

        if (hr == DDERR_SURFACELOST) {
            if (IDirectDrawSurface_Restore(GNW95_DDRestoreSurface) != DD_OK) {
                return;
            }
        }
    }

    unsigned char* dest = (unsigned char*)ddsd.lpSurface + ddsd.lPitch * destY + 2 * destX;

    src += srcPitch * srcY + srcX;

    for (int y = 0; y < srcHeight; y++) {
        unsigned short* destPtr = (unsigned short*)dest;
        unsigned char* srcPtr = src;
        for (int x = 0; x < srcWidth; x++) {
            *destPtr = GNW95_Pal16[*srcPtr];
            destPtr++;
            srcPtr++;
        }

        dest += ddsd.lPitch;
        src += srcPitch;
    }

    IDirectDrawSurface_Unlock(GNW95_DDPrimarySurface, ddsd.lpSurface);
}

// 0x4CBA44
void GNW95_ShowRect16(unsigned char* src, int srcPitch, int a3, int srcX, int srcY, int srcWidth, int srcHeight, int destX, int destY)
{
    GNW95_MouseShowRect16(src, srcPitch, a3, srcX, srcY, srcWidth, srcHeight, destX, destY);
}

// 0x4CBAB0
void GNW95_MouseShowTransRect16(unsigned char* src, int srcPitch, int a3, int srcX, int srcY, int srcWidth, int srcHeight, int destX, int destY, unsigned char keyColor)
{
    DDSURFACEDESC ddsd;
    HRESULT hr;

    if (!GNW95_isActive) {
        return;
    }

    while (1) {
        ddsd.dwSize = sizeof(ddsd);

        hr = IDirectDrawSurface_Lock(GNW95_DDPrimarySurface, NULL, &ddsd, 1, NULL);
        if (hr == DD_OK) {
            break;
        }

        if (hr == DDERR_SURFACELOST) {
            if (IDirectDrawSurface_Restore(GNW95_DDRestoreSurface) != DD_OK) {
                return;
            }
        }
    }

    unsigned char* dest = (unsigned char*)ddsd.lpSurface + ddsd.lPitch * destY + 2 * destX;

    src += srcPitch * srcY + srcX;

    for (int y = 0; y < srcHeight; y++) {
        unsigned short* destPtr = (unsigned short*)dest;
        unsigned char* srcPtr = src;
        for (int x = 0; x < srcWidth; x++) {
            if (*srcPtr != keyColor) {
                *destPtr = GNW95_Pal16[*srcPtr];
            }
            destPtr++;
            srcPtr++;
        }

        dest += ddsd.lPitch;
        src += srcPitch;
    }

    IDirectDrawSurface_Unlock(GNW95_DDPrimarySurface, ddsd.lpSurface);
}

// Clears drawing surface.
//
// 0x4CBBC8
void GNW95_zero_vid_mem()
{
    DDSURFACEDESC ddsd;
    HRESULT hr;
    unsigned char* surface;

    if (!GNW95_isActive) {
        return;
    }

    while (1) {
        ddsd.dwSize = sizeof(DDSURFACEDESC);

        hr = IDirectDrawSurface_Lock(GNW95_DDPrimarySurface, NULL, &ddsd, 1, NULL);
        if (hr == DD_OK) {
            break;
        }

        if (hr == DDERR_SURFACELOST) {
            if (IDirectDrawSurface_Restore(GNW95_DDRestoreSurface) != DD_OK) {
                return;
            }
        }
    }

    surface = (unsigned char*)ddsd.lpSurface;
    for (unsigned int y = 0; y < ddsd.dwHeight; y++) {
        memset(surface, 0, ddsd.dwWidth);
        surface += ddsd.lPitch;
    }

    IDirectDrawSurface_Unlock(GNW95_DDPrimarySurface, ddsd.lpSurface);
}
