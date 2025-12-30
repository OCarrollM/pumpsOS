#ifndef ARCH_I386_TIMER_H
#define ARCH_I386_TIMER_H

#include <stdint.h>

#define PIT_CHANNEL0 0x40
#define PIT_CHANNEL1 0x41
#define PIT_CHANNEL2 0x42
#define PIT_COMMAND 0x43

#define PIT_FREQUENCY 1193182

void timer_init(uint32_t frequency);

uint64_t timer_get_ticks(void);

void timer_sleep(uint32_t ms);

#endif