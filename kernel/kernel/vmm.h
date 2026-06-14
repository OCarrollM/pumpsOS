#ifndef KERNEL_VMM_H
#define KERNEL_VMM_H

#include <stdint.h>
#include <stdbool.h>

#define PAGE_SIZE 4096

/* Set some page flags */
#define PTE_PRESENT (1 << 0)
#define PTE_WRITABLE (1 << 1)
#define PTE_USER (1 << 2)

/* Recursive mapping (when we reach 1023 we need to remap)*/
#define RECURSIVE_ENTRY 1023
#define PAGE_DIR_VIRTUAL 0xFFFFF000
#define PAGE_TABLE_BASE 0xFFC00000

void vmm_init(void);
bool vmm_map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags);
uint32_t vmm_unmap_page(uint32_t virtual_addr);
bool vmm_is_mapped(uint32_t virtual_addr);
uint32_t vmm_get_physical(uint32_t virtual_addr);
uint32_t vmm_create_address_space(void);
void vmm_destroy_address_space(uint32_t pd_phys);
void vmm_switch_address_space(uint32_t pd_phys);
uint32_t vmm_get_current_address_space(void);
uint32_t vmm_clone_address_space(uint32_t src_pd_phys);

void vmm_print_mappings(void);

#endif