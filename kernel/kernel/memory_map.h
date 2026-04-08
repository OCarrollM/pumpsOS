#ifndef KERNEL_MEMORY_MAP_H
#define KERNEL_MEMORY_MAP_H

#include <stdint.h>
#include <stdbool.h>
#include "multiboot.h"

#define MAX_MEMORY_REGIONS 32

/* Memory regions (matches the multiboot) */
typedef enum {
    MEMORY_REGION_AVAILABLE = 1,
    MEMORY_REGION_RESERVED = 2,
    MEMORY_REGION_ACPI_RECL = 3,
    MEMORY_REGION_ACPI_NVS = 4,
    MEMORY_REGION_BAD = 5,
} memoery_region_type_t;

/* single memory region */
typedef struct {
    uint64_t base;
    uint64_t length;
    uint32_t type;
} memory_region_t;

/* Global mem map info */
typedef struct {
    memory_region_t regions[MAX_MEMORY_REGIONS];
    uint32_t region_count;

    uint64_t total_memory;
    uint64_t usable_memory;
    uint64_t highest_address;

    uint32_t mem_lower_kb;
    uint32_t mem_upper_kb;
} memory_map_t;

/* Global mem map */
extern memory_map_t g_memory_map;

/* Functions */
bool memory_map_init(multiboot_info_t* mboot);
void memory_map_print(void);
uint64_t memory_map_get_usable(void);
uint64_t memory_map_get_highest_address(void);
bool memory_map_is_usable(uint64_t addr, uint64_t length);
const char* memory_region_type_string(uint32_t type);

#endif