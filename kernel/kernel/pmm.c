#include "pmm.h"
#include <stdio.h>
#include <string.h>

extern uint32_t _kernel_start;
extern uint32_t _kernel_end;

/*
 * Bitmap Allocator
 * Thought a bitmap would be the easiest solution to this
 * 
 * Every bit is going to represent one page frame
 * Return 1 = Used, 0 = Free
 * 
 * 128Kb bitmap will support 4GB of physical memory
 * Perfect for 32Bit OS 
 */
#define MAX_PAGES (1024 * 1024) /* 4Gb / 4Kb */
#define BITMAP_SIZE (MAX_PAGES / 8)

static uint8_t bitmap[BITMAP_SIZE];
static uint32_t total_pages;
static uint32_t used_pages;

static inline uint32_t addr_to_frame(uint32_t addr) {
    return addr / PAGE_SIZE;
}

static inline uint32_t frame_to_addr(uint32_t frame) {
    return frame * PAGE_SIZE;
}

static inline void bitmap_set(uint32_t frame) {
    bitmap[frame / 8] |= (1 << (frame % 8));
}

static inline void bitmap_clear(uint32_t frame) {
    bitmap[frame / 8] &= ~(1 << (frame % 8));
}

static inline bool bitmap_test(uint32_t frame) {
    return (bitmap[frame / 8] & (1 << (frame % 8))) != 0;
}

/* Above are helper functions */

void pmm_mark_region_free(uint32_t base, uint32_t length) {
    uint32_t start_frame = addr_to_frame(base);
    uint32_t end_frame = addr_to_frame(base + length);

    // Align start up to next frame
    if (base % PAGE_SIZE != 0) {
        start_frame++;
    }

    for (uint32_t frame = start_frame; frame < end_frame; frame++) {
        if (bitmap_test(frame)) {
            bitmap_clear(frame);
            used_pages--;
        }
    }
}

void pmm_mark_region_used(uint32_t base, uint32_t length) {
    uint32_t start_frame = addr_to_frame(base);
    uint32_t end_frame = addr_to_frame(base + length + PAGE_SIZE - 1);

    for (uint32_t frame = start_frame; frame < end_frame; frame++) {
        if (!bitmap_test(frame)) {
            bitmap_set(frame);
            used_pages++;
        }
    }
}

void pmm_init(void) {
    /* Start with everything as USED */
    memset(bitmap, 0xFF, BITMAP_SIZE);

    /* Calculate our total pages from the highest addr */
    uint64_t highest = memory_map_get_highest_address();
    if (highest > (uint64_t)MAX_PAGES * PAGE_SIZE) {
        total_pages = MAX_PAGES;
    } else {
        total_pages = (uint32_t)(highest / PAGE_SIZE);
    }
    used_pages = total_pages;

    /* Free regions that mem map says are available */
    for (uint32_t i = 0; i < g_memory_map.region_count; i++) {
        memory_region_t* r = &g_memory_map.regions[i];

        if (r->type != MEMORY_REGION_AVAILABLE) {
            continue;
        }
        /* Only use memory within the 4Gb */
        if (r->base >= 0x100000000ULL) {
            continue;
        }

        uint32_t base = (uint32_t)r->base;
        uint32_t length = (uint32_t)r->length;

        /* Attach to that 4Gb */
        if (r->base + r->length > 0x100000000ULL) {
            length = 0xFFFFFFF - base;
        }

        pmm_mark_region_free(base, length);
    }

    /*
     * Now we will mark regions that are important so they do not get overwritten
     * 
     * 1. First Page
     * 2. Kernel stuff
     * 3. This bitmap
     */

    pmm_mark_region_used(0x0, PAGE_SIZE);

    /* Kernel protection */
    uint32_t kernel_phys_start = (uint32_t)&_kernel_start;
    uint32_t kernel_phys_end = (uint32_t)&_kernel_end - 0xC0000000;
    uint32_t kernel_size = kernel_phys_end - kernel_phys_start;
    pmm_mark_region_used(kernel_phys_start, kernel_size);
    
    printf("[PMM] %d total pages, %d used, %d free (%d MB free)\n", total_pages, used_pages, total_pages - used_pages, (total_pages - used_pages) * 4 / 1024);
}

uint32_t pmm_alloc_page(void) {
    /* Scan bitmap */
    for (uint32_t i = 0; i < total_pages / 8; i++) {
        /* Skip the full bytes */
        if (bitmap[i] == 0xFF) {
            continue;
        }

        for (int bit = 0; bit < 8; bit++) {
            if (!(bitmap[i] & (1 << bit))) {
                uint32_t frame = i * 8 + bit;
                bitmap_set(frame);
                used_pages++;
                return frame_to_addr(frame);
            }
        }
    }

    /* Incase we run out of memory */
    printf("[PMM] ERROR: Out of physical memory!\n");
    return 0;
}

void pmm_free_page(uint32_t phys_addr) {
    uint32_t frame = addr_to_frame(phys_addr);
    if (!bitmap_test(frame)) {
        printf("[PMM] WARNING: Double free of frame %d (0x%x)\n", frame, phys_addr);
        return;
    }

    bitmap_clear(frame);
    used_pages--;
}

uint32_t pmm_get_free_page_count(void) {
    return total_pages - used_pages;
}

uint32_t pmm_get_total_page_count(void) {
    return total_pages;
}

void pmm_print_stats(void) {
    printf("\n=== Physical Memory Manager ===\n");
    printf("  Page size:    %d bytes\n", PAGE_SIZE);
    printf("  Total pages:  %d\n", total_pages);
    printf("  Used pages:   %d\n", used_pages);
    printf("  Free pages:   %d\n", total_pages - used_pages);
    printf("  Total memory: %d MB\n", total_pages * 4 / 1024);
    printf("  Used memory:  %d MB\n", used_pages * 4 / 1024);
    printf("  Free memory:  %d MB\n", (total_pages - used_pages) * 4 / 1024);
    printf("\n");
}