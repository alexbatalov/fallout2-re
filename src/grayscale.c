#include "grayscale.h"

#include "color.h"

// 0x596D90
unsigned char byte_596D90[256];

// 0x44FA78
void grayscalePaletteUpdate(int a1, int a2)
{
    if (a1 >= 0 && a2 <= 255) {
        for (int index = a1; index <= a2; index++) {
            // NOTE: The only way to explain so much calls to [sub_4C72E0] with
            // the same repeated pattern is by the use of min/max macros.

            int v1 = max((sub_4C72E0(index) & 0x7C00) >> 10, max((sub_4C72E0(index) & 0x3E0) >> 5, sub_4C72E0(index) & 0x1F));
            int v2 = min((sub_4C72E0(index) & 0x7C00) >> 10, min((sub_4C72E0(index) & 0x3E0) >> 5, sub_4C72E0(index) & 0x1F));
            int v3 = v1 + v2;
            int v4 = (int)((double)v3 * 240.0 / 510.0);

            int paletteIndex = ((v4 & 0xFF) << 10) | ((v4 & 0xFF) << 5) | (v4 & 0xFF);
            byte_596D90[index] = byte_6A38D0[paletteIndex];
        }
    }
}

// 0x44FC40
void grayscalePaletteApply(unsigned char* buffer, int width, int height, int pitch)
{
    unsigned char* ptr = buffer;
    int skip = pitch - width;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            unsigned char c = *ptr;
            *ptr++ = byte_596D90[c];
        }
        ptr += skip;
    }
}
