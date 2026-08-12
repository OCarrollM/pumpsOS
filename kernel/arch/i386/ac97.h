// Intel AC97 Audio driver

#ifndef ARCH_I386_AC97_H
#define ARCH_I386_AC97_H

#include <stdint.h>
#include <stdbool.h>

bool ac97_init(void);
bool ac97_play_tone(uint32_t freq_hz, uint32_t ms);

#endif