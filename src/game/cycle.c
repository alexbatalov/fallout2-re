#include "game/cycle.h"

#include "color.h"
#include "core.h"
#include "game/gconfig.h"
#include "game/palette.h"

#define COLOR_CYCLE_PERIOD_SLOW 200U
#define COLOR_CYCLE_PERIOD_MEDIUM 142U
#define COLOR_CYCLE_PERIOD_FAST 100U
#define COLOR_CYCLE_PERIOD_VERY_FAST 33U

static void cycle_colors();

// 0x51843C
static int cycle_speed_factor = 1;

// TODO: Convert colors to RGB.
// clang-format off

// Green.
//
// 0x518440
unsigned char slime[12] = {
    0, 108, 0,
    11, 115, 7,
    27, 123, 15,
    43, 131, 27,
};

// Light gray?
//
// 0x51844C
unsigned char shoreline[18] = {
    83, 63, 43,
    75, 59, 43,
    67, 55, 39,
    63, 51, 39,
    55, 47, 35,
    51, 43, 35,
};

// Orange.
//
// 0x51845E
unsigned char fire_slow[15] = {
    255, 0, 0,
    215, 0, 0,
    147, 43, 11,
    255, 119, 0,
    255, 59, 0,
};

// Red.
//
// 0x51846D
unsigned char fire_fast[15] = {
    71, 0, 0,
    123, 0, 0,
    179, 0, 0,
    123, 0, 0,
    71, 0, 0,
};

// Light blue.
//
// 0x51847C
unsigned char monitors[15] = {
    107, 107, 111,
    99, 103, 127,
    87, 107, 143,
    0, 147, 163,
    107, 187, 255,
};

// clang-format on

// 0x51848C
static bool cycle_initialized = false;

// 0x518490
static bool cycle_enabled = false;

// 0x56D7D0
static unsigned int last_cycle_fast;

// 0x56D7D4
static unsigned int last_cycle_slow;

// 0x56D7D8
static unsigned int last_cycle_medium;

// 0x56D7DC
static unsigned int last_cycle_very_fast;

// 0x42E780
void cycle_init()
{
    if (cycle_initialized) {
        return;
    }

    bool colorCycling;
    if (!configGetBool(&game_config, GAME_CONFIG_SYSTEM_KEY, GAME_CONFIG_COLOR_CYCLING_KEY, &colorCycling)) {
        colorCycling = true;
    }

    if (!colorCycling) {
        return;
    }

    for (int index = 0; index < 12; index++) {
        slime[index] >>= 2;
    }

    for (int index = 0; index < 18; index++) {
        shoreline[index] >>= 2;
    }

    for (int index = 0; index < 15; index++) {
        fire_slow[index] >>= 2;
    }

    for (int index = 0; index < 15; index++) {
        fire_fast[index] >>= 2;
    }

    for (int index = 0; index < 15; index++) {
        monitors[index] >>= 2;
    }

    tickersAdd(cycle_colors);

    cycle_initialized = true;
    cycle_enabled = true;

    int cycleSpeedFactor;
    if (!config_get_value(&game_config, GAME_CONFIG_SYSTEM_KEY, GAME_CONFIG_CYCLE_SPEED_FACTOR_KEY, &cycleSpeedFactor)) {
        cycleSpeedFactor = 1;
    }

    change_cycle_speed(cycleSpeedFactor);
}

// 0x42E8CC
void cycle_reset()
{
    if (cycle_initialized) {
        last_cycle_slow = 0;
        last_cycle_medium = 0;
        last_cycle_fast = 0;
        last_cycle_very_fast = 0;
        tickersAdd(cycle_colors);
        cycle_enabled = true;
    }
}

// 0x42E90C
void cycle_exit()
{
    if (cycle_initialized) {
        tickersRemove(cycle_colors);
        cycle_initialized = false;
        cycle_enabled = false;
    }
}

// 0x42E930
void cycle_disable()
{
    cycle_enabled = false;
}

// 0x42E93C
void cycle_enable()
{
    cycle_enabled = true;
}

// 0x42E948
bool cycle_is_enabled()
{
    return cycle_enabled;
}

// 0x42E97C
static void cycle_colors()
{
    // 0x518494
    static int slime_start = 0;

    // 0x518498
    static int shoreline_start = 0;

    // 0x51849C
    static int fire_slow_start = 0;

    // 0x5184A0
    static int fire_fast_start = 0;

    // 0x5184A4
    static int monitors_start = 0;

    // 0x5184A8
    static unsigned char bobber_red = 0;

    // 0x5184A9
    static signed char bobber_diff = -4;

    if (!cycle_enabled) {
        return;
    }

    bool changed = false;

    unsigned char* palette = getSystemPalette();
    unsigned int time = _get_time();

    if (getTicksBetween(time, last_cycle_slow) >= COLOR_CYCLE_PERIOD_SLOW * cycle_speed_factor) {
        changed = true;
        last_cycle_slow = time;

        int paletteIndex = 229 * 3;

        for (int index = slime_start; index < 12; index++) {
            palette[paletteIndex++] = slime[index];
        }

        for (int index = 0; index < slime_start; index++) {
            palette[paletteIndex++] = slime[index];
        }

        slime_start -= 3;
        if (slime_start < 0) {
            slime_start = 9;
        }

        paletteIndex = 248 * 3;

        for (int index = shoreline_start; index < 18; index++) {
            palette[paletteIndex++] = shoreline[index];
        }

        for (int index = 0; index < shoreline_start; index++) {
            palette[paletteIndex++] = shoreline[index];
        }

        shoreline_start -= 3;
        if (shoreline_start < 0) {
            shoreline_start = 15;
        }

        paletteIndex = 238 * 3;

        for (int index = fire_slow_start; index < 15; index++) {
            palette[paletteIndex++] = fire_slow[index];
        }

        for (int index = 0; index < fire_slow_start; index++) {
            palette[paletteIndex++] = fire_slow[index];
        }

        fire_slow_start -= 3;
        if (fire_slow_start < 0) {
            fire_slow_start = 12;
        }
    }

    if (getTicksBetween(time, last_cycle_medium) >= COLOR_CYCLE_PERIOD_MEDIUM * cycle_speed_factor) {
        changed = true;
        last_cycle_medium = time;

        int paletteIndex = 243 * 3;

        for (int index = fire_fast_start; index < 15; index++) {
            palette[paletteIndex++] = fire_fast[index];
        }

        for (int index = 0; index < fire_fast_start; index++) {
            palette[paletteIndex++] = fire_fast[index];
        }

        fire_fast_start -= 3;
        if (fire_fast_start < 0) {
            fire_fast_start = 12;
        }
    }

    if (getTicksBetween(time, last_cycle_fast) >= COLOR_CYCLE_PERIOD_FAST * cycle_speed_factor) {
        changed = true;
        last_cycle_fast = time;

        int paletteIndex = 233 * 3;

        for (int index = monitors_start; index < 15; index++) {
            palette[paletteIndex++] = monitors[index];
        }

        for (int index = 0; index < monitors_start; index++) {
            palette[paletteIndex++] = monitors[index];
        }

        monitors_start -= 3;

        if (monitors_start < 0) {
            monitors_start = 12;
        }
    }

    if (getTicksBetween(time, last_cycle_very_fast) >= COLOR_CYCLE_PERIOD_VERY_FAST * cycle_speed_factor) {
        changed = true;
        last_cycle_very_fast = time;

        if (bobber_red == 0 || bobber_red == 60) {
            bobber_diff = -bobber_diff;
        }

        bobber_red += bobber_diff;

        int paletteIndex = 254 * 3;
        palette[paletteIndex++] = bobber_red;
        palette[paletteIndex++] = 0;
        palette[paletteIndex++] = 0;
    }

    if (changed) {
        paletteSetEntriesInRange(palette + 229 * 3, 229, 255);
    }
}

// 0x42E950
void change_cycle_speed(int value)
{
    cycle_speed_factor = value;
    config_set_value(&game_config, GAME_CONFIG_SYSTEM_KEY, GAME_CONFIG_CYCLE_SPEED_FACTOR_KEY, value);
}

// NOTE: Unused.
//
// 0x42E974
int get_cycle_speed()
{
    return cycle_speed_factor;
}
