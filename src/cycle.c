#include "cycle.h"

#include "color.h"
#include "core.h"
#include "game_config.h"
#include "palette.h"

// 0x51843C
int gColorCycleSpeedFactor = 1;

// TODO: Convert colors to RGB.
// clang-format off

// Green.
//
// 0x518440
unsigned char byte_518440[12] = {
    0, 108, 0,
    11, 115, 7,
    27, 123, 15,
    43, 131, 27,
};

// Light gray?
//
// 0x51844C
unsigned char byte_51844C[18] = {
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
unsigned char byte_51845E[15] = {
    255, 0, 0,
    215, 0, 0,
    147, 43, 11,
    255, 119, 0,
    255, 59, 0,
};

// Red.
//
// 0x51846D
unsigned char byte_51846D[15] = {
    71, 0, 0,
    123, 0, 0,
    179, 0, 0,
    123, 0, 0,
    71, 0, 0,
};

// Light blue.
//
// 0x51847C
unsigned char byte_51847C[15] = {
    107, 107, 111,
    99, 103, 127,
    87, 107, 143,
    0, 147, 163,
    107, 187, 255,
};

// clang-format on

// 0x51848C
bool gColorCycleInitialized = false;

// 0x518490
bool gColorCycleEnabled = false;

// 0x518494
int dword_518494 = 0;

// 0x518498
int dword_518498 = 0;

// 0x51849C
int dword_51849C = 0;

// 0x5184A0
int dword_5184A0 = 0;

// 0x5184A4
int dword_5184A4 = 0;

// 0x5184A8
unsigned char byte_5184A8 = 0;

// 0x5184A9
signed char byte_5184A9 = -4;

// 0x56D7D0
unsigned int gColorCycleTimestamp3;

// 0x56D7D4
unsigned int gColorCycleTimestamp1;

// 0x56D7D8
unsigned int gColorCycleTimestamp2;

// 0x56D7DC
unsigned int gColorCycleTimestamp4;

// 0x42E780
void colorCycleInit()
{
    if (gColorCycleInitialized) {
        return;
    }

    bool colorCycling;
    if (!configGetBool(&gGameConfig, GAME_CONFIG_SYSTEM_KEY, GAME_CONFIG_COLOR_CYCLING_KEY, &colorCycling)) {
        colorCycling = true;
    }

    if (!colorCycling) {
        return;
    }

    for (int index = 0; index < 12; index++) {
        byte_518440[index] >>= 2;
    }

    for (int index = 0; index < 18; index++) {
        byte_51844C[index] >>= 2;
    }

    for (int index = 0; index < 15; index++) {
        byte_51845E[index] >>= 2;
    }

    for (int index = 0; index < 15; index++) {
        byte_51846D[index] >>= 2;
    }

    for (int index = 0; index < 15; index++) {
        byte_51847C[index] >>= 2;
    }

    tickersAdd(colorCycleTicker);

    gColorCycleInitialized = true;
    gColorCycleEnabled = true;

    int cycleSpeedFactor;
    if (!configGetInt(&gGameConfig, GAME_CONFIG_SYSTEM_KEY, GAME_CONFIG_CYCLE_SPEED_FACTOR_KEY, &cycleSpeedFactor)) {
        cycleSpeedFactor = 1;
    }

    cycleSetSpeedFactor(cycleSpeedFactor);
}

// 0x42E8CC
void colorCycleReset()
{
    if (gColorCycleInitialized) {
        gColorCycleTimestamp1 = 0;
        gColorCycleTimestamp2 = 0;
        gColorCycleTimestamp3 = 0;
        gColorCycleTimestamp4 = 0;
        tickersAdd(colorCycleTicker);
        gColorCycleEnabled = true;
    }
}

// 0x42E90C
void colorCycleFree()
{
    if (gColorCycleInitialized) {
        tickersRemove(colorCycleTicker);
        gColorCycleInitialized = false;
        gColorCycleEnabled = false;
    }
}

// 0x42E930
void colorCycleDisable()
{
    gColorCycleEnabled = false;
}

// 0x42E93C
void colorCycleEnable()
{
    gColorCycleEnabled = true;
}

// 0x42E948
bool colorCycleEnabled()
{
    return gColorCycleEnabled;
}

// 0x42E950
void cycleSetSpeedFactor(int value)
{
    gColorCycleSpeedFactor = value;
    configSetInt(&gGameConfig, GAME_CONFIG_SYSTEM_KEY, GAME_CONFIG_CYCLE_SPEED_FACTOR_KEY, value);
}

// 0x42E97C
void colorCycleTicker()
{
    if (!gColorCycleEnabled) {
        return;
    }

    bool changed = false;

    unsigned char* palette = _getSystemPalette();
    unsigned int time = _get_time();

    if (getTicksBetween(time, gColorCycleTimestamp1) >= COLOR_CYCLE_PERIOD_1 * gColorCycleSpeedFactor) {
        changed = true;
        gColorCycleTimestamp1 = time;

        int paletteIndex = 229 * 3;

        for (int index = dword_518494; index < 12; index++) {
            palette[paletteIndex++] = byte_518440[index];
        }

        for (int index = 0; index < dword_518494; index++) {
            palette[paletteIndex++] = byte_518440[index];
        }

        dword_518494 -= 3;
        if (dword_518494 < 0) {
            dword_518494 = 9;
        }

        paletteIndex = 248 * 3;

        for (int index = dword_518498; index < 18; index++) {
            palette[paletteIndex++] = byte_51844C[index];
        }

        for (int index = 0; index < dword_518498; index++) {
            palette[paletteIndex++] = byte_51844C[index];
        }

        dword_518498 -= 3;
        if (dword_518498 < 0) {
            dword_518498 = 15;
        }

        paletteIndex = 238 * 3;

        for (int index = dword_51849C; index < 15; index++) {
            palette[paletteIndex++] = byte_51845E[index];
        }

        for (int index = 0; index < dword_51849C; index++) {
            palette[paletteIndex++] = byte_51845E[index];
        }

        dword_51849C -= 3;
        if (dword_51849C < 0) {
            dword_51849C = 12;
        }
    }

    if (getTicksBetween(time, gColorCycleTimestamp2) >= COLOR_CYCLE_PERIOD_2 * gColorCycleSpeedFactor) {
        changed = true;
        gColorCycleTimestamp2 = time;

        int paletteIndex = 243 * 3;

        for (int index = dword_5184A0; index < 15; index++) {
            palette[paletteIndex++] = byte_51846D[index];
        }

        for (int index = 0; index < dword_5184A0; index++) {
            palette[paletteIndex++] = byte_51846D[index];
        }

        dword_5184A0 -= 3;
        if (dword_5184A0 < 0) {
            dword_5184A0 = 12;
        }
    }

    if (getTicksBetween(time, gColorCycleTimestamp3) >= COLOR_CYCLE_PERIOD_3 * gColorCycleSpeedFactor) {
        changed = true;
        gColorCycleTimestamp3 = time;

        int paletteIndex = 233 * 3;

        for (int index = dword_5184A4; index < 15; index++) {
            palette[paletteIndex++] = byte_51847C[index];
        }

        for (int index = 0; index < dword_5184A4; index++) {
            palette[paletteIndex++] = byte_51847C[index];
        }

        dword_5184A4 -= 3;

        if (dword_5184A4 < 0) {
            dword_5184A4 = 12;
        }
    }

    if (getTicksBetween(time, gColorCycleTimestamp4) >= COLOR_CYCLE_PERIOD_4 * gColorCycleSpeedFactor) {
        changed = true;
        gColorCycleTimestamp4 = time;

        if (byte_5184A8 == 0 || byte_5184A8 == 60) {
            byte_5184A9 = -byte_5184A9;
        }

        byte_5184A8 += byte_5184A9;

        int paletteIndex = 254 * 3;
        palette[paletteIndex++] = byte_5184A8;
        palette[paletteIndex++] = 0;
        palette[paletteIndex++] = 0;
    }

    if (changed) {
        paletteSetEntriesInRange(palette + 229 * 3, 229, 255);
    }
}
