#ifndef KERNEL_WAV_H
#define KERNEL_WAV_H

#include <stdbool.h>
#include <stdint.h>

bool wav_play(const char* path);
bool wav_play_memory(const uint8_t* data, uint32_t len);
void wav_stop(void);
bool wav_is_playing(void);

#endif