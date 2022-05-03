#ifndef RANDOM_H
#define RANDOM_H

#include "db.h"

typedef enum Roll {
    ROLL_CRITICAL_FAILURE,
    ROLL_FAILURE,
    ROLL_SUCCESS,
    ROLL_CRITICAL_SUCCESS,
} Roll;

extern const double dbl_50D4BA;
extern const double dbl_50D4C2;

extern int dword_51C694;

extern int dword_6648D0[32];
extern int dword_664950;

void randomInit();
int sub_4A2FFC();
void randomReset();
void randomExit();
int randomSave(File* stream);
int randomLoad(File* stream);
int randomRoll(int difficulty, int criticalSuccessModifier, int* howMuchPtr);
int randomTranslateRoll(int delta, int criticalSuccessModifier);
int randomBetween(int min, int max);
int random(int max);
void randomSeedPrerandom(int seed);
int randomInt32();
void randomSeedPrerandomInternal(int seed);
unsigned int randomGetSeed();
void randomValidatePrerandom();

#endif /* RANDOM_H */
