/* Physical mem map implementation */

#include "memory_map.h"
#include <stdio.h>

#define PHYS_TO_VIRT(addr) ((addr) + 0xC0000000)

memory_map_t g_memory_map;

/* Convert memory regiontpe to human string */
const char* memory_region_type_string(uint32_t type) {
    switch (type) {
        case MEMORY_REGION_AVAILABLE: return "Available";
        case MEMORY_REGION_RESERVED: return "Reserved";
        case MEMORY_REGION_ACPI_RECL: return "ACPI Reclaimable";
        case MEMORY_REGION_ACPI_NVS: return "ACPI NVS";
        case MEMORY_REGION_BAD: return "Bad Memory";
        default: return "Unknown";
    }
}

/* Parse the memory map */
bool memory_map_init(multiboot_info_t* mboot) {
    g_memory_map.region_count = 0;
    g_memory_map.total_memory = 0;
    g_memory_map.usable_memory = 0;
    g_memory_map.highest_address = 0;
    g_memory_map.mem_lower_kb = 0;
    g_memory_map.mem_upper_kb = 0;

    /* check if basic info is available */
    if(mboot->flags & MULTIBBOOT_INFO_MEMORY) {
        g_memory_map.mem_lower_kb = mboot->mem_lower;
        g_memory_map.mem_upper_kb = mboot->mem_upper;

        printf("Memory: lower=%dKB, upper=%dKB\n", mboot->mem_lower, mboot->mem_upper);
    }

    /* Check detailed mem map */
    if(!(mboot->flags & MULTIBBOOT_INFO_MEM_MAP)) {
        printf("ERROR: No memory map provided by bootloader!\n");
        return false;
    }

    printf("Memory map at 0x%x, length=%d bytes\n", mboot->mmap_addr, mboot->mmap_length);

    /* Parse the map */
    uint32_t mmap_virt = PHYS_TO_VIRT(mboot->mmap_addr);
    uint32_t mmap_end = mboot->mmap_addr + mboot->mmap_length;
    multiboot_mmap_entry_t* mmap = (multiboot_mmap_entry_t*) mmap_virt;

    while ((uint32_t)mmap < mmap_end && g_memory_map.region_count < MAX_MEMORY_REGIONS) {
        memory_region_t* region = &g_memory_map.regions[g_memory_map.region_count];
        region->base = mmap->addr;
        region->length = mmap->len;
        region->type = mmap->type;
        g_memory_map.region_count++;

        g_memory_map.total_memory += mmap->len;

        if(mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
            g_memory_map.usable_memory += mmap->len;

            uint64_t region_end = mmap->addr + mmap->len;
            if(region_end > g_memory_map.highest_address) {
                g_memory_map.highest_address = region_end;
            }
        }

        // Advance to next entry
        mmap = (multiboot_mmap_entry_t*) ((uint32_t)mmap + mmap->size + sizeof(mmap->size));
    }

    printf("Found %d memory regions\n", g_memory_map.region_count);
    printf("Total usable memory: %d MB\n", (uint32_t)(g_memory_map.usable_memory / (1024 * 1024)));

    return true;
}

void memory_map_print(void) {
    printf("\n=== Physical Memory Map ===\n");
    printf("Region  Base Address      Length            Type\n");
    printf("------  ----------------  ----------------  ----------------\n");
    
    for (uint32_t i = 0; i < g_memory_map.region_count; i++) {
        memory_region_t* r = &g_memory_map.regions[i];
        
        /*
         * Since we don't have full 64-bit printf support, we'll print
         * the high and low 32-bit parts separately for addresses > 4GB.
         * For most 32-bit systems, the high part will be 0.
         */
        uint32_t base_high = (uint32_t)(r->base >> 32);
        uint32_t base_low = (uint32_t)(r->base & 0xFFFFFFFF);
        uint32_t len_high = (uint32_t)(r->length >> 32);
        uint32_t len_low = (uint32_t)(r->length & 0xFFFFFFFF);
        
        if (base_high == 0 && len_high == 0) {
            /* Simple case: addresses fit in 32 bits */
            printf("  %d     0x%x      0x%x      %s\n",
                   i, base_low, len_low, memory_region_type_string(r->type));
        } else {
            /* Full 64-bit addresses */
            printf("  %d     0x%x%x  0x%x%x  %s\n",
                   i, base_high, base_low, len_high, len_low,
                   memory_region_type_string(r->type));
        }
    }
    
    printf("\nSummary:\n");
    printf("  Total memory:   %d MB\n", (uint32_t)(g_memory_map.total_memory / (1024 * 1024)));
    printf("  Usable memory:  %d MB\n", (uint32_t)(g_memory_map.usable_memory / (1024 * 1024)));
    printf("  Memory regions: %d\n", g_memory_map.region_count);
    printf("\n");
}

/*
 * memory_map_get_usable - Get total usable memory in bytes
 */
uint64_t memory_map_get_usable(void) {
    return g_memory_map.usable_memory;
}

/*
 * memory_map_get_highest_address - Get highest usable physical address
 */
uint64_t memory_map_get_highest_address(void) {
    return g_memory_map.highest_address;
}

/*
 * memory_map_is_usable - Check if a physical address range is usable
 * 
 * This is useful for checking if it's safe to use a particular
 * memory region (e.g., when loading modules or allocating buffers).
 */
bool memory_map_is_usable(uint64_t addr, uint64_t length) {
    uint64_t end = addr + length;
    
    for (uint32_t i = 0; i < g_memory_map.region_count; i++) {
        memory_region_t* r = &g_memory_map.regions[i];
        
        /* Skip non-available regions */
        if (r->type != MEMORY_REGION_AVAILABLE) {
            continue;
        }
        
        uint64_t region_end = r->base + r->length;
        
        /* Check if our range is entirely within this usable region */
        if (addr >= r->base && end <= region_end) {
            return true;
        }
    }
    
    return false;
}