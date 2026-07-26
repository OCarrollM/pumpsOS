#ifndef ARCH_I386_WINDOW_H
#define ARCH_I386_WINDOW_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_WINDOWS 8
#define WIN_BORDER 2 // Thickness
#define WIN_TITLEBAR 20 // Height

struct window;

typedef struct window {
    int32_t x, y;
    int32_t w, h;
    char title[32];
    uint32_t bg;
    bool used;

    void (*draw_content)(struct window* win);
    void* content;
} window_t;

window_t* window_create(int32_t x, int32_t y, int32_t w, int32_t h, const char* title, uint32_t bg);
void wm_redraw(void);
void wm_handle_mouse(void);
int32_t window_client_x(const window_t* win);
int32_t window_client_y(const window_t* win);
int32_t window_client_h(const window_t* win);
int32_t window_client_w(const window_t* win);

#endif