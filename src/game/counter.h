#ifndef FALLOUT_GAME_COUNTER_H_
#define FALLOUT_GAME_COUNTER_H_

typedef void(CounterOutputFunc)(double a1);

void counter_on(CounterOutputFunc* outputFunc);
void counter_off();

#endif /* FALLOUT_GAME_COUNTER_H_ */
