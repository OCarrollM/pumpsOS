// Intel AC97 Audio driver

#ifndef ARCH_I386_AC97_H
#define ARCH_I386_AC97_H

#include <stdint.h>
#include <stdbool.h>

#define AC97_SAMPLE_RATE 48000
#define AC97_CHANNELS 2

typedef uint32_t (*ac97_fill_t)(int16_t* buf, uint32_t frames);

bool ac97_init(void);
bool ac97_play(ac97_fill_t fill);
void ac97_stop(void);
void ac97_poll(void);
bool ac97_is_playing(void);

#endif