#include "widget.h"

#include "color.h"
#include "text_font.h"
#include "window.h"

// 0x50EB1C
const float flt_50EB1C = 31.0;

// 0x50EB20
const float flt_50EB20 = 31.0;

// 0x66E6A0
int dword_66E6A0[32];

// 0x4B5A64
void _showRegion(int a1)
{
    // TODO: Incomplete.
}

// 0x4B5C24
int _update_widgets()
{
    for (int index = 0; index < 32; index++) {
        if (dword_66E6A0[index]) {
            _showRegion(dword_66E6A0[index]);
        }
    }

    return 1;
}

// 0x4B6120
int widgetGetFont()
{
    return gWidgetFont;
}

// 0x4B6128
int widgetSetFont(int a1)
{
    gWidgetFont = a1;
    fontSetCurrent(a1);
    return 1;
}

// 0x4B6160
int widgetGetTextFlags()
{
    return gWidgetTextFlags;
}

// 0x4B6168
int widgetSetTextFlags(int a1)
{
    gWidgetTextFlags = a1;
    return 1;
}

// 0x4B6174
unsigned char widgetGetTextColor()
{
    return byte_6A38D0[dword_672DA4 | (dword_672DA0 << 5) | (dword_672DAC << 10)];
}

// 0x4B6198
unsigned char widgetGetHighlightColor()
{
    return byte_6A38D0[dword_672DB4 | (dword_672DB0 << 5) | (dword_672D8C << 10)];
}

// 0x4B61BC
int widgetSetTextColor(float a1, float a2, float a3)
{
    dword_672DAC = (int)(a1 * flt_50EB1C);
    dword_672DA0 = (int)(a2 * flt_50EB1C);
    dword_672DA4 = (int)(a3 * flt_50EB1C);

    return 1;
}

// 0x4B6208
int widgetSetHighlightColor(float a1, float a2, float a3)
{
    dword_672D8C = (int)(a1 * flt_50EB20);
    dword_672DB0 = (int)(a2 * flt_50EB20);
    dword_672DB4 = (int)(a3 * flt_50EB20);

    return 1;
}
