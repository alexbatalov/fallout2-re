#ifndef CYCLE_H
#define CYCLE_H

#include <stdbool.h>

#define COLOR_CYCLE_PERIOD_1 (200U)
#define COLOR_CYCLE_PERIOD_2 (142U)
#define COLOR_CYCLE_PERIOD_3 (100U)
#define COLOR_CYCLE_PERIOD_4 (33U)

extern int gColorCycleSpeedFactor;
extern unsigned char byte_518440[12];
extern unsigned char byte_51844C[18];
extern unsigned char byte_51845E[15];
extern unsigned char byte_51846D[15];
extern unsigned char byte_51847C[15];
extern bool gColorCycleEnabled;
extern bool gColorCycleInitialized;

extern unsigned int gColorCycleTimestamp3;
extern unsigned int gColorCycleTimestamp1;
extern unsigned int gColorCycleTimestamp2;
extern unsigned int gColorCycleTimestamp4;

void colorCycleInit();
void colorCycleReset();
void colorCycleFree();
void colorCycleDisable();
void colorCycleEnable();
bool colorCycleEnabled();
void cycleSetSpeedFactor(int value);
void colorCycleTicker();

#endif /* CYCLE_H */
