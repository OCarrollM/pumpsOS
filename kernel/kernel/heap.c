#include "heap.h"
#include "vmm.h"
#include "pmm.h"
#include <stdio.h>
#include <string.h>

/**
 * A simple linked list heap allocator where memoery is divided into blocks
 * 
 * [header][  usable memory  ][header][  usable memory  ]...
 *
 *  The header stores the block size, whether it's free, and
 * pointers to the next and previous blocks. When kmalloc is
 * called, we walk the list looking for a free block big enough.
 * When kfree is called, we mark the block free and merge with
 * neighbors to reduce fragmentation.
 */

typedef struct block_header {
    uint32_t magic;
    uint32_t size;
    bool free;
    struct block_header* next;
    struct block_header* prev;
} block_header_t;

#define HEAP_MAGIC 0xDEADBEEF
#define HEADER_SIZE sizeof(block_header_t)
#define MIN_BLOCK_SIZE 16
#define ALIGNMENT 8
#define ALIGN_UP(x) (((x) + ALIGNMENT - 1) & ~(ALIGNMENT - 1))

static uint32_t heap_start;
static uint32_t heap_end;
static uint32_t heap_max;
static block_header_t* first_block;
static uint32_t total_allocs;
static uint32_t total_frees;

/**
 * We expand the heap by mapping new pages and return True on a success 
 */

static bool heap_expand(uint32_t new_end) {
    if (new_end > heap_max) {
        printf("ERROR: Heap would exceed max size\n");
        return false;
    }

    /* Align to the page boundary */
    new_end = (new_end + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    /* Map the new pages from current end to new end from above */
    for (uint32_t addr = heap_end; addr < new_end; addr += PAGE_SIZE) {
        uint32_t phys = pmm_alloc_page();
        if (phys == 0) {
            printf("ERROR: Out of physical memory during the expansion\n");
            return false;
        }

        if (!vmm_map_page(addr, phys, PTE_PRESENT | PTE_WRITABLE)) {
            printf("ERROR: Failed to map page at 0x%x\n", addr);
            return false;
        }
    }
    heap_end = new_end;
    return true;
}

void heap_init(void) {
    heap_start = KHEAP_START;
    heap_end = KHEAP_START;
    heap_max = KHEAP_START + KHEAP_MAX_SIZE;
    total_allocs = 0;
    total_frees = 0;

    /* Map initial pages */
    if (!heap_expand(KHEAP_START + KHEAP_INITIAL_SIZE)) {
        printf("FATAL ERROR: Could not allocate initial heap\n");
        return;
    }

    /* Create our first free block spanning the whole heap */
    first_block = (block_header_t*)heap_start;
    first_block->magic = HEAP_MAGIC;
    first_block->size = KHEAP_INITIAL_SIZE - HEADER_SIZE;
    first_block->free = true;
    first_block->next = NULL;
    first_block->prev = NULL;

    printf("Initialized at 0x%x, size=%d KB\n", heap_start, KHEAP_INITIAL_SIZE / 1024);
}

/**
 * Split a block if it is too big + a new header and min block size
 */
static void split_block(block_header_t* block, uint32_t size) {
    uint32_t remaining = block->size - size - HEADER_SIZE;

    if (remaining < MIN_BLOCK_SIZE) {
        return;
        // Uses whole block
    }

    /* Create new block */
    block_header_t* new_block = (block_header_t*)((uint8_t*)block + HEADER_SIZE + size);
    new_block->magic = HEAP_MAGIC;
    new_block->size = remaining;
    new_block->free = true;
    new_block->next = block->next;
    new_block->prev = block;

    if (block->next) {
        block->next->prev = new_block;
    }

    block->next = new_block;
    block->size = size;
}

/* Merge a free block with adjacent free blocks */
static void merge_free_blocks(block_header_t* block) {
    if (block->next && block->next->free) {
        block->size += HEADER_SIZE + block->next->size;
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }

    if (block->prev && block->prev->free) {
        block->prev->size += HEADER_SIZE + block->size;
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
    }
}

void* kmalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }

    size = ALIGN_UP(size);
    block_header_t* current = first_block;

    while (current) {
        if (current->magic != HEAP_MAGIC) {
            printf("FATAL ERROR: Heap corruption detected at 0x%x\n", (uint32_t)current);
            return NULL;
        }

        if (current->free && current->size >= size) {
            split_block(current, size); // found a block and split if possible
            current->free = false;
            total_allocs++;
            return (void*)((uint8_t*)current + HEADER_SIZE);
        }

        /* Expand to heap if last block*/
        if (current->next == NULL) {
            uint32_t needed = size + HEADER_SIZE;
            uint32_t new_end = heap_end + needed;

            if (!heap_expand(new_end)) {
                printf("ERROR: Cannot expand heap for %d bytes\n", size);
                return NULL;
            }

            /* Extend last block if free, else create a new one */
            if (current->free) {
                current->size += needed;
            } else {
                block_header_t* new_block = (block_header_t*)((uint8_t*)current + HEADER_SIZE + current->size);
                new_block->magic = HEAP_MAGIC;
                new_block->size = needed - HEADER_SIZE;
                new_block->free = true;
                new_block->next = NULL;
                new_block->prev = current;
                current->next = new_block;
            }

            /* Loop will find the space on next pass over*/
            continue;
        }   
        current = current->next;
    }
    return NULL;
}

void kfree(void* ptr) {
    if (ptr == NULL) {
        return;
    }

    block_header_t* block = (block_header_t*)((uint8_t*)ptr - HEADER_SIZE);

    if (block->magic != HEAP_MAGIC) {
        printf("FATAL ERROR: kfree corruptioin - bad magic at 0x%x\n", (uint32_t)block);
        return;
    }

    if (block->free) {
        printf("WARNING: Double free at 0x%x\n", (uint32_t)ptr);
        return;
    }

    block->free = true;
    total_frees++;

    merge_free_blocks(block);
}

void heap_print_stats(void) {
    uint32_t total_blocks = 0;
    uint32_t free_blocks = 0;
    uint32_t used_blocks = 0;
    uint32_t free_bytes = 0;
    uint32_t used_bytes = 0;

    block_header_t* current = first_block;
    while (current) {
        total_blocks++;
        if (current->free) {
            free_blocks++;
            free_bytes += current->size;
        } else {
            used_blocks++;
            used_bytes += current->size;
        }
        current = current->next;
    }

        printf("\n=== Kernel Heap ===\n");
    printf("  Range: 0x%x - 0x%x (%d KB mapped)\n",
           heap_start, heap_end, (heap_end - heap_start) / 1024);
    printf("  Blocks: %d total, %d used, %d free\n",
           total_blocks, used_blocks, free_blocks);
    printf("  Used:  %d bytes\n", used_bytes);
    printf("  Free:  %d bytes\n", free_bytes);
    printf("  Overhead: %d bytes (%d headers)\n",
           total_blocks * HEADER_SIZE, total_blocks);
    printf("  Allocs: %d, Frees: %d\n", total_allocs, total_frees);
    printf("\n");
}
