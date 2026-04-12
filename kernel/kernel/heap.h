#ifndef KERNEL_HEAP_H
#define KERNEL_HEAP_H

#include <stdint.h>
#include <stddef.h>

/* Heap V-Addr Range */
#define KHEAP_START 0xD0000000
#define KHEAP_INITIAL_SIZE 0x10000
#define KHEAP_MAX_SIZE 0x01000000

void heap_init(void);
void* kmalloc(size_t size);
void kfree(void* ptr);
void heap_print_stats(void);

#endif