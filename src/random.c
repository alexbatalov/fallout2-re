#include "random.h"

#include "debug.h"
#include "scripts.h"

#include <stdlib.h>

// clang-format off
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <timeapi.h>
// clang-format on

// 0x50D4BA
const double dbl_50D4BA = 36.42;

// 0x50D4C2
const double dbl_50D4C2 = 4000;

// 0x51C694
int dword_51C694 = 0;

// 0x6648D0
int dword_6648D0[32];

// 0x664950
int dword_664950;

// 0x4A2FE0
void randomInit()
{
    unsigned int randomSeed = randomGetSeed();
    srand(randomSeed);

    int pseudorandomSeed = randomInt32();
    randomSeedPrerandomInternal(pseudorandomSeed);

    randomValidatePrerandom();
}

// Note: Collapsed.
//
// 0x4A2FFC
int sub_4A2FFC()
{
    return 0;
}

// NOTE: Uncollapsed 0x4A2FFC.
void randomReset()
{
    sub_4A2FFC();
}

// NOTE: Uncollapsed 0x4A2FFC.
void randomExit()
{
    sub_4A2FFC();
}

// NOTE: Uncollapsed 0x4A2FFC.
int randomSave(File* stream)
{
    return sub_4A2FFC();
}

// NOTE: Uncollapsed 0x4A2FFC.
int randomLoad(File* stream)
{
    return sub_4A2FFC();
}

// Rolls d% against [difficulty].
//
// 0x4A3000
int randomRoll(int difficulty, int criticalSuccessModifier, int* howMuchPtr)
{
    int delta = difficulty - randomBetween(1, 100);
    int result = randomTranslateRoll(delta, criticalSuccessModifier);

    if (howMuchPtr != NULL) {
        *howMuchPtr = delta;
    }

    return result;
}

// Translates raw d% result into [Roll] constants, possibly upgrading to
// criticals (starting from day 2).
//
// 0x4A3030
int randomTranslateRoll(int delta, int criticalSuccessModifier)
{
    int gameTime = gameTimeGetTime();

    int roll;
    if (delta < 0) {
        roll = ROLL_FAILURE;

        if ((gameTime / GAME_TIME_TICKS_PER_DAY) >= 1) {
            // 10% to become critical failure.
            if (randomBetween(1, 100) <= -delta / 10) {
                roll = ROLL_CRITICAL_FAILURE;
            }
        }
    } else {
        roll = ROLL_SUCCESS;

        if ((gameTime / GAME_TIME_TICKS_PER_DAY) >= 1) {
            // 10% + modifier to become critical success.
            if (randomBetween(1, 100) <= delta / 10 + criticalSuccessModifier) {
                roll = ROLL_CRITICAL_SUCCESS;
            }
        }
    }

    return roll;
}

// 0x4A30C0
int randomBetween(int min, int max)
{
    int result;

    if (min <= max) {
        result = min + random(max - min + 1);
    } else {
        result = max + random(min - max + 1);
    }

    if (result < min || result > max) {
        debugPrint("Random number %d is not in range %d to %d", result, min, max);
        result = min;
    }

    return result;
}

// 0x4A30FC
int random(int max)
{
    int v1 = 16807 * (dword_664950 % 127773) - 2836 * (dword_664950 / 127773);

    if (v1 < 0) {
        v1 += 0x7FFFFFFF;
    }

    if (v1 < 0) {
        v1 += 0x7FFFFFFF;
    }

    int v2 = dword_51C694 & 0x1F;
    int v3 = dword_6648D0[v2];
    dword_6648D0[v2] = v1;
    dword_51C694 = v3;
    dword_664950 = v1;

    return v3 % max;
}

// 0x4A31A0
void randomSeedPrerandom(int seed)
{
    if (seed == -1) {
        // NOTE: Uninline.
        seed = randomInt32();
    }

    randomSeedPrerandomInternal(seed);
}

// 0x4A31C4
int randomInt32()
{
    int high = rand() << 16;
    int low = rand();

    return (high + low) & INT_MAX;
}

// 0x4A31E0
void randomSeedPrerandomInternal(int seed)
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
            dword_6648D0[index] = num;
        }
    }

    dword_51C694 = dword_6648D0[0];
    dword_664950 = num;
}

// Provides seed for random number generator.
//
// 0x4A3258
unsigned int randomGetSeed()
{
    return timeGetTime();
}

// 0x4A3264
void randomValidatePrerandom()
{
    int results[25];

    // NOTE: Original code uses [__stosd].
    for (int index = 0; index < 25; index++) {
        results[index] = 0;
    }

    for (int attempt = 0; attempt < 100000; attempt++) {
        int value = randomBetween(1, 25);
        if (value - 1 < 0) {
            debugPrint("I made a negative number %d\n", value - 1);
        }

        results[value - 1]++;
    }

    double v1 = 0.0;

    for (int index = 0; index < 25; index++) {
        double v2 = ((double)results[index] - dbl_50D4C2) * ((double)results[index] - dbl_50D4C2) / dbl_50D4C2;
        v1 += v2;
    }

    debugPrint("Chi squared is %f, P = %f at 0.05\n", v1, dbl_50D4C2);

    if (v1 < dbl_50D4BA) {
        debugPrint("Sequence is random, 95%% confidence.\n");
    } else {
        debugPrint("Warning! Sequence is not random, 95%% confidence.\n");
    }
}
