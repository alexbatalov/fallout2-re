#include "game/counter.h"

#include "core.h"
#include "debug.h"

// 0x518324
int counter_is_on = 0;

// 0x518328
unsigned char count = 0;

// 0x51832C
clock_t last_time = 0;

// 0x518330
CounterOutputFunc* counter_output_func;

// 0x56D730
clock_t this_time;

// 0x42C790
void counter_on(CounterOutputFunc* outputFunc)
{
    if (!counter_is_on) {
        debugPrint("Turning on counter...\n");
        tickersAdd(counter);
        counter_output_func = outputFunc;
        counter_is_on = 1;
        last_time = clock();
    }
}

// 0x42C7D4
void counter_off()
{
    if (counter_is_on) {
        tickersRemove(counter);
        counter_is_on = 0;
    }
}

// 0x42C7F4
void counter()
{
    count++;
    if (count == 0) {
        this_time = clock();
        if (counter_output_func != NULL) {
            counter_output_func(256.0 / (this_time - last_time) / 100.0);
        }
        last_time = this_time;
    }
}
