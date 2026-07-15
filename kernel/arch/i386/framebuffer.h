#ifndef ARCH_I386_FRAMEBUFFER_H
#define ARCH_I386_FRAMEBUFFER_H

#include <stdint.h>
#include "../kernel/multiboot.h"

int framebuffer_init(multiboot_info_t* mb);
void draw_pixel(uint32_t x, uint32_t y, uint32_t colour);
void fill_screen(uint32_t colour);
void draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t colour);
uint32_t fb_get_width(void);
uint32_t fb_get_height(void);

#endif