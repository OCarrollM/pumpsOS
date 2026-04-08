#ifndef KERNEL_PMM_H
#define KERNEL_PMM_H

#include <stdint.h>
#include <stdbool.h>
#include "memory_map.h"

#define PAGE_SIZE 4096

void pmm_init(void);                                        // Initialize with parsed mem map
uint32_t pmm_alloc_page(void);                              // Allocate a frame (4KB) return phys addr or fail
void pmm_free_page(uint32_t phys_addr);                     // Free a single frame
void pmm_mark_region_used(uint32_t base, uint32_t length);  // Mark range of phys memory used
void pmm_mark_region_free(uint32_t base, uint32_t length);  // Mark range of phys memory free
uint32_t pmm_get_free_page_count(void);                     // Get number of free pages
uint32_t pmm_get_total_page_count(void);                    // Get total number of pages
void pmm_print_stats(void);                                 // Debugger

#endif

