// frame buffer time
#include "framebuffer.h"
#include "../kernel/vmm.h"

#define FB_VIRT_BASE 0xE0000000u // free virtual region to map into

static uint32_t fb_virt = 0; // v base after mapping
static uint32_t fb_width = 0;
static uint32_t fb_height = 0;
static uint32_t fb_pitch = 0;
static uint32_t fb_bpp = 0;


int framebuffer_init(multiboot_info_t* mb) {
    // multiboot flag bit 12
    if (!(mb->flags & (1u << 12))) {
        // return nothing
        return 0;
    }

    uint32_t fb_phys = (uint32_t)mb->framebuffer_addr;
    fb_width = mb->framebuffer_width;
    fb_height = mb->framebuffer_height;
    fb_pitch = mb->framebuffer_pitch;
    fb_bpp = mb->framebuffer_bpp;

    if (fb_bpp != 32) {
        return 0;
    }

    // map the framebuffers physical pages into vm
    uint32_t fb_size = fb_pitch * fb_height;
    uint32_t pages = (fb_size + 0xFFF) / 0x1000;

    for (uint32_t i = 0; i < pages; i++) {
        if(!vmm_map_page(FB_VIRT_BASE + i * 0x1000, fb_phys + i * 0x1000, PTE_PRESENT | PTE_WRITABLE)) {
            return 0;
        }
    }
    fb_virt = FB_VIRT_BASE;
    return 1;
}

void draw_pixel(uint32_t x, uint32_t y, uint32_t colour) {
    if (!fb_virt || x >= fb_width || y >= fb_height) return;
    uint32_t* pixel = (uint32_t*)(fb_virt + y * fb_pitch + x * 4);
    *pixel = colour;
}

void fill_screen(uint32_t colour) {
    if (!fb_virt) return;
    for (uint32_t y = 0; y < fb_height; y++) {
        uint32_t* row = (uint32_t*)(fb_virt + y * fb_pitch);
        for (uint32_t x = 0; x < fb_width; x++) {
            row[x] = colour;
        }
    }
}

void draw_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t colour) {
    for (uint32_t dy = 0; dy < h; dy++) {
        for (uint32_t dx = 0; dx < w; dx++) {
            draw_pixel(x + dx, y + dy, colour);
        }
    }
}

uint32_t fb_get_width(void) { return fb_width; }
uint32_t fb_get_height(void) { return fb_height; }