// text console rendered onto frame buffer
// will replace the current VGA text mode
#ifndef ARCH_I386_FBCON_H
#define ARCH_I386_FBCON_H

#include <stddef.h>
#include <stdint.h>

// initialize framebuffer
void fbcon_init(void);

// write chars at cursor location
void fbcon_write(const char* data, size_t len);

// write single char
void fbcon_putchar(char c);

// clear screen
void fbcon_clear(void);

// set fg and bg
void fbcon_set_colour(uint32_t fb, uint32_t bg);

void fbcon_draw_string_at(uint32_t px, uint32_t py, const char* s, uint32_t fg, uint32_t bg);
void fbcon_draw_char_at(uint32_t px, uint32_t py, char c, uint32_t fg, uint32_t bg);

#endif