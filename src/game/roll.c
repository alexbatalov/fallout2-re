#include "game/roll.h"

#include <limits.h>
#include <stdlib.h>

// clang-format off
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <timeapi.h>
// clang-format on

#include "debug.h"
#include "game/scripts.h"

static int ran1(int max);
static void init_random();
static int random_seed();
static void seed_generator(int seed);
static unsigned int timer_read();
static void check_chi_squared();

// 0x51C694
static int iy = 0;

// 0x6648D0
static int iv[32];

// 0x664950
static int idum;

// 0x4A2FE0
void roll_init()
{
    // NOTE: Uninline.
    init_random();

    check_chi_squared();
}

// NOTE: Uncollapsed 0x4A2FFC.
int roll_reset()
{
    return 0;
}

// NOTE: Uncollapsed 0x4A2FFC.
int roll_exit()
{
    return 0;
}

// NOTE: Uncollapsed 0x4A2FFC.
int roll_save(File* stream)
{
    return 0;
}

// NOTE: Uncollapsed 0x4A2FFC.
int roll_load(File* stream)
{
    return 0;
}

// Rolls d% against [difficulty].
//
// 0x4A3000
int roll_check(int difficulty, int criticalSuccessModifier, int* howMuchPtr)
{
    int delta = difficulty - roll_random(1, 100);
    int result = roll_check_critical(delta, criticalSuccessModifier);

    if (howMuchPtr != NULL) {
        *howMuchPtr = delta;
    }

    return result;
}

// Translates raw d% result into [Roll] constants, possibly upgrading to
// criticals (starting from day 2).
//
// 0x4A3030
int roll_check_critical(int delta, int criticalSuccessModifier)
{
    int gameTime = gameTimeGetTime();

    int roll;
    if (delta < 0) {
        roll = ROLL_FAILURE;

        if ((gameTime / GAME_TIME_TICKS_PER_DAY) >= 1) {
            // 10% to become critical failure.
            if (roll_random(1, 100) <= -delta / 10) {
                roll = ROLL_CRITICAL_FAILURE;
            }
        }
    } else {
        roll = ROLL_SUCCESS;

        if ((gameTime / GAME_TIME_TICKS_PER_DAY) >= 1) {
            // 10% + modifier to become critical success.
            if (roll_random(1, 100) <= delta / 10 + criticalSuccessModifier) {
                roll = ROLL_CRITICAL_SUCCESS;
            }
        }
    }

    return roll;
}

// 0x4A30C0
int roll_random(int min, int max)
{
    int result;

    if (min <= max) {
        result = min + ran1(max - min + 1);
    } else {
        result = max + ran1(min - max + 1);
    }

    if (result < min || result > max) {
        debugPrint("Random number %d is not in range %d to %d", result, min, max);
        result = min;
    }

    return result;
}

// 0x4A30FC
static int ran1(int max)
{
    int v1 = 16807 * (idum % 127773) - 2836 * (idum / 127773);

    if (v1 < 0) {
        v1 += 0x7FFFFFFF;
    }

    if (v1 < 0) {
        v1 += 0x7FFFFFFF;
    }

    int v2 = iy & 0x1F;
    int v3 = iv[v2];
    iv[v2] = v1;
    iy = v3;
    idum = v1;

    return v3 % max;
}

// NOTE: Inlined.
//
// 0x4A3174
static void init_random()
{
    srand(timer_read());
    seed_generator(random_seed());
}

// 0x4A31A0
void roll_set_seed(int seed)
{
    if (seed == -1) {
        // NOTE: Uninline.
        seed = random_seed();
    }

    seed_generator(seed);
}

// 0x4A31C4
static int random_seed()
{
    int high = rand() << 16;
    int low = rand();

    return (high + low) & INT_MAX;
}

// 0x4A31E0
static void seed_generator(int seed)
{
    int num = seed;
    if (num < 1) {
        num = 1;
    }

    for (int index = 40; index > 0; index--) {
        num = 16807 * (num % 127773) - 2836 * (num / 127773);

        if (num < 0) {
            num &= INT_MAX;
        }

        if (index < 32) {
            iv[index] = num;
        }
    }

    iy = iv[0];
    idum = num;
}

// Provides seed for random number generator.
//
// 0x4A3258
static unsigned int timer_read()
{
    return timeGetTime();
}

// 0x4A3264
static void check_chi_squared()
{
    int results[25];

    for (int index = 0; index < 25; index++) {
        results[index] = 0;
    }

    for (int attempt = 0; attempt < 100000; attempt++) {
        int value = roll_random(1, 25);
        if (value - 1 < 0) {
            debugPrint("I made a negative number %d\n", value - 1);
        }

        results[value - 1]++;
    }

    double v1 = 0.0;

    for (int index = 0; index < 25; index++) {
        double v2 = ((double)results[index] - 4000.0) * ((double)results[index] - 4000.0) / 4000.0;
        v1 += v2;
    }

    debugPrint("Chi squared is %f, P = %f at 0.05\n", v1, 4000.0);

    if (v1 < 36.42) {
        debugPrint("Sequence is random, 95%% confidence.\n");
    } else {
        debugPrint("Warning! Sequence is not random, 95%% confidence.\n");
    }
}
