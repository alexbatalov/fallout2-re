#ifndef COUNTER_H
#define COUNTER_H

#include <time.h>

typedef void(CounterOutputFunc)(double a1);

extern int counter_is_on;
extern unsigned char count;
extern clock_t last_time;
extern CounterOutputFunc* counter_output_func;
extern clock_t this_time;

void counter_on(CounterOutputFunc* outputFunc);
void counter_off();
void counter();

#endif /* COUNTER_H */
