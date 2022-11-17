#include "game/counter.h"

#include <time.h>

#include "plib/gnw/input.h"
#include "plib/gnw/debug.h"

static void counter();

// 0x518324
static int counter_is_on = 0;

// 0x518328
static unsigned char count = 0;

// 0x51832C
static clock_t last_time = 0;

// 0x518330
static CounterOutputFunc* counter_output_func;

// 0x42C790
void counter_on(CounterOutputFunc* outputFunc)
{
    if (!counter_is_on) {
        debug_printf("Turning on counter...\n");
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
static void counter()
{
    // 0x56D730
    static clock_t this_time;

    count++;
    if (count == 0) {
        this_time = clock();
        if (counter_output_func != NULL) {
            counter_output_func(256.0 / (this_time - last_time) / 100.0);
        }
        last_time = this_time;
    }
}
