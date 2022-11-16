#include "game/palette.h"

#include <string.h>

#include "plib/color/color.h"
#include "core.h"
#include "game/cycle.h"
#include "plib/gnw/debug.h"
#include "game/gsound.h"

// 0x6639D0
static unsigned char current_palette[256 * 3];

// 0x663CD0
unsigned char white_palette[256 * 3];

// 0x663FD0
unsigned char black_palette[256 * 3];

// 0x6642D0
static int fade_steps;

// 0x493A00
void palette_init()
{
    memset(black_palette, 0, 256 * 3);
    memset(white_palette, 63, 256 * 3);
    memcpy(current_palette, cmap, 256 * 3);

    unsigned int tick = _get_time();
    if (gsound_background_is_enabled() || gsound_speech_is_enabled()) {
        colorSetFadeBkFunc(soundContinueAll);
    }

    fadeSystemPalette(current_palette, current_palette, 60);

    colorSetFadeBkFunc(NULL);

    unsigned int diff = getTicksSince(tick);

    // NOTE: Modern CPUs are super fast, so it's possible that less than 10ms
    // (the resolution of underlying GetTicks) is needed to fade between two
    // palettes, which leads to zero diff, which in turn leads to unpredictable
    // number of fade steps. To fix that the fallback value is used (46). This
    // value is commonly seen when running the game in 1 core VM.
    if (diff == 0) {
        diff = 46;
    }

    fade_steps = (int)(60.0 / (diff * (1.0 / 700.0)));

    debug_printf("\nFade time is %u\nFade steps are %d\n", diff, fade_steps);
}

// NOTE: Uncollapsed 0x493AD0.
void palette_reset()
{
}

// NOTE: Uncollapsed 0x493AD0.
void palette_exit()
{
}

// 0x493AD4
void palette_fade_to(unsigned char* palette)
{
    bool colorCycleWasEnabled = cycle_is_enabled();
    cycle_disable();

    if (gsound_background_is_enabled() || gsound_speech_is_enabled()) {
        colorSetFadeBkFunc(soundContinueAll);
    }

    fadeSystemPalette(current_palette, palette, fade_steps);
    colorSetFadeBkFunc(NULL);

    memcpy(current_palette, palette, 768);

    if (colorCycleWasEnabled) {
        cycle_enable();
    }
}

// 0x493B48
void palette_set_to(unsigned char* palette)
{
    memcpy(current_palette, palette, sizeof(current_palette));
    setSystemPalette(palette);
}

// 0x493B78
void palette_set_entries(unsigned char* palette, int start, int end)
{
    memcpy(current_palette + 3 * start, palette, 3 * (end - start + 1));
    setSystemPaletteEntries(palette, start, end);
}
