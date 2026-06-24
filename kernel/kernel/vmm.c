#include "vmm.h"
#include "pmm.h"
#include <stdio.h>
#include <string.h>

/*
 * Recursive paging trick:
 * 
 * Page directory entry 1023 points back to the page directory itself.
 * This means the CPU treats the page directory as both a directory AND
 * a page table for addresses starting at 0xFFC00000.
 *
 * To access the page directory:
 *   Virtual 0xFFFFF000 -> PD entry 1023 -> PD (as page table) entry 1023 -> PD physical
 *   So 0xFFFFF000 lets us read/write the page directory entries.
 *
 * To access page table N:
 *   Virtual 0xFFC00000 + N*4096 -> PD entry 1023 -> PD (as page table) entry N -> PT N physical
 *   So 0xFFC00000 + N*0x1000 lets us read/write the entries of page table N.
 */

/* Pointer to page directory */
static inline uint32_t* vmm_get_page_directory(void) {
    return (uint32_t*)PAGE_DIR_VIRTUAL;
}

/* Pointer to page table */
static inline uint32_t* vmm_get_page_table(uint32_t pd_index) {
    return (uint32_t*)(PAGE_TABLE_BASE + pd_index * PAGE_SIZE);
}

/* Extract page dir from virtual addr (22 - 31) */
static inline uint32_t pd_index(uint32_t vaddr) {
    return vaddr >> 22;
}

/* Extract page table index from virtual addr (12 - 21)*/
static inline uint32_t pt_index(uint32_t vaddr) {
    return (vaddr >> 12) & 0x3FF;
}

/* Flush single entry */
static inline void vmm_flush_tlb(uint32_t vaddr) {
    asm volatile("invlpg (%0)" : : "r"(vaddr) : "memory");
}

void vmm_init(void) {
    /**
     * Set up recursive mapping
     * 
     * We need physcial address of page dir. Originally defined in boot.s
     */
    extern uint32_t boot_page_directory[];
    uint32_t pd_phys = (uint32_t)boot_page_directory - 0xC0000000;

    /* We now need to access the page dir through its own virtual address, mapped to higher
    half*/
    uint32_t* pd = boot_page_directory;
    pd[RECURSIVE_ENTRY] = pd_phys | PTE_PRESENT | PTE_WRITABLE;

    /* Flush TLB so mapping is active */
    asm volatile (
        "movl %%cr3, %%eax\n"
        "movl %%eax, %%cr3\n"
        ::: "eax", "memory"
    );

    printf("Recursive mapping set up at PD entry: %d\n", RECURSIVE_ENTRY);
    printf("Page Directory at addr: 0x%x\n", pd_phys);
}

bool vmm_map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags) {
    uint32_t pdi = pd_index(virtual_addr);
    uint32_t pti = pt_index(virtual_addr);
    uint32_t* pd = vmm_get_page_directory();

    /* Check if the page table exists */
    if (!(pd[pdi] & PTE_PRESENT)) {
        uint32_t pt_phys = pmm_alloc_page();
        if (pt_phys == 0) {
            printf("ERROR: Cannot allocate a page table\n");
            return false;
        }

        /* Install new page */
        pd[pdi] = pt_phys | PTE_PRESENT | PTE_WRITABLE | (flags & PTE_USER);

        vmm_flush_tlb((uint32_t)vmm_get_page_table(pdi));
        memset(vmm_get_page_table(pdi), 0, PAGE_SIZE);
    }

    /* Now set the table entry */
    uint32_t* pt = vmm_get_page_table(pdi);

    if (pt[pti] & PTE_PRESENT) {
        printf("WARNING: Remapping already mapped page 0x%x\n", virtual_addr);
    }

    pt[pti] = (physical_addr & 0xFFFFF000) | (flags & 0xFFF) | PTE_PRESENT;
    vmm_flush_tlb(virtual_addr);

    return true;
}

uint32_t vmm_unmap_page(uint32_t virtual_addr) {
    uint32_t pdi = pd_index(virtual_addr);
    uint32_t pti = pt_index(virtual_addr);
    uint32_t* pd = vmm_get_page_directory();

    if (!(pd[pdi] & PTE_PRESENT)) {
        return 0;
    }

    uint32_t* pt = vmm_get_page_table(pdi);

    if (!(pt[pti] & PTE_PRESENT)) {
        return 0;
    }

    uint32_t phys = pt[pti] & 0xFFFFF000;
    pt[pti] = 0;
    vmm_flush_tlb(virtual_addr);

    return phys;
}

bool vmm_is_mapped(uint32_t virtual_addr) {
    uint32_t pdi = pd_index(virtual_addr);
    uint32_t pti = pt_index(virtual_addr);
    uint32_t* pd = vmm_get_page_directory();

    if(!(pd[pdi] & PTE_PRESENT)) {
        return false;
    }

    uint32_t* pt = vmm_get_page_table(pdi);
    return (pt[pti] & PTE_PRESENT) != 0;
}

uint32_t vmm_get_physical(uint32_t virtual_addr) {
    uint32_t pdi = pd_index(virtual_addr);
    uint32_t pti = pt_index(virtual_addr);
    uint32_t* pd = vmm_get_page_directory();

    if (!(pd[pdi] & PTE_PRESENT)) {
        return 0;
    }

    uint32_t* pt = vmm_get_page_table(pdi);

    if (!(pt[pti] & PTE_PRESENT)) {
        return 0;
    }

    return (pt[pti] & 0xFFFFF000) | (virtual_addr & 0xFFF);
}

uint32_t vmm_create_address_space(void) {
    uint32_t new_pd_phys = pmm_alloc_page();
    if (new_pd_phys == 0) {
        printf("ERROR: Could not allocate page directory\n");
        return 0;
    }

    uint32_t scratch_vaddr = 0xE0000000;

    if (!vmm_map_page(scratch_vaddr, new_pd_phys, PTE_PRESENT | PTE_WRITABLE)) {
        pmm_free_page(new_pd_phys);
        return 0;
    }

    uint32_t* new_pd = (uint32_t*)scratch_vaddr;
    uint32_t* current_pd = vmm_get_page_directory();

    for (int i = 0; i < 768; i++) {
        new_pd[i] = 0; // 0 the lower half
    }

    for (int i = 768; i < 1023; i++) {
        new_pd[i] = current_pd[i];
    }

    new_pd[1023] = new_pd_phys | PTE_PRESENT | PTE_WRITABLE;

    vmm_unmap_page(scratch_vaddr);

    return new_pd_phys;
}

void vmm_switch_address_space(uint32_t pd_phys) {
    asm volatile("movl %0, %%cr3" : : "r"(pd_phys) : "memory");
}

void vmm_destroy_address_space(uint32_t pd_phys) {
    pmm_free_page(pd_phys);
}

uint32_t vmm_get_current_address_space(void) {
    uint32_t pd_phys;
    asm volatile("movl %%cr3, %0" : "=r"(pd_phys));
    return pd_phys;
}

void vmm_print_mappings(void) {
    uint32_t* pd = vmm_get_page_directory();
    int mapped_count = 0;

    printf("\n=== Virtual Memory Mappings ===\n");
    printf("Active page directory mappings\n");

    for (int i = 0; i < 1024; i++) {
        if (pd[i] & PTE_PRESENT) {
            uint32_t virt_start = (uint32_t)i << 22;
            printf("    PD[%d]: 0x%x -> PT at phys 0x%x", i, virt_start, pd[i] & 0xFFFFF000);

            if(pd[i] & PTE_WRITABLE) printf(" W");
            if(pd[i] & PTE_USER) printf(" U");
            printf("\n");

            /* Count mapped pages */
            if (i != RECURSIVE_ENTRY) {
                uint32_t* pt = vmm_get_page_table(i);
                for (int j = 0; j < 1024; j++) {
                    if (pt[j] & PTE_PRESENT) {
                        mapped_count++;
                    }
                }
            }
        }
    }
    printf("Total mapped pages: %d (%d KB)\n\n", mapped_count, mapped_count * 4);
}

// Clone a addr space and return physical addr of new page
uint32_t vmm_clone_address_space(uint32_t src_pd_phys) {
    uint32_t child_pd = vmm_create_address_space();
    if (child_pd == 0) return 0;

    uint8_t* page_buffer = (uint8_t*)kmalloc(PAGE_SIZE);
    if (!page_buffer) {
        vmm_destroy_address_space(child_pd);
        return 0;
    }

    uint32_t saved_pd = vmm_get_current_address_space();
    if (saved_pd != src_pd_phys) {
        vmm_switch_address_space(src_pd_phys);
    }

    uint32_t* src_pd_virt = (uint32_t*)PAGE_DIR_VIRTUAL;
    /* NOTE: page_buffer is the kmalloc'd heap pointer above.
     * Do NOT declare a stack array here -- a 4 KB stack local
     * overflows the kernel stack. */

    for (uint32_t pdi = 0; pdi < 768; pdi++) {
        if (!(src_pd_virt[pdi] & PTE_PRESENT)) {
            continue;
        }

        uint32_t* src_pt = (uint32_t*)(PAGE_TABLE_BASE + pdi * PAGE_SIZE);

        for (uint32_t pti = 0; pti < 1024; pti++) {
            if (!(src_pt[pti] & PTE_PRESENT)) {
                continue;
            }

            uint32_t vaddr = (pdi << 22) | (pti << 12);
            uint32_t src_flags = src_pt[pti] & 0xFFF;

            /* Read source page (we're in source AS). */
            memcpy(page_buffer, (void*)vaddr, PAGE_SIZE);

            /* Switch to child to install the copy. */
            vmm_switch_address_space(child_pd);

            uint32_t child_phys = pmm_alloc_page();
            if (child_phys == 0) {
                kfree(page_buffer);
                vmm_switch_address_space(saved_pd);
                vmm_destroy_address_space(child_pd);
                return 0;
            }

            if (!vmm_map_page(vaddr, child_phys, src_flags)) {
                pmm_free_page(child_phys);
                kfree(page_buffer);
                vmm_switch_address_space(saved_pd);
                vmm_destroy_address_space(child_pd);
                return 0;
            }

            memcpy((void*)vaddr, page_buffer, PAGE_SIZE);

            /* Back to source for the next iteration. */
            vmm_switch_address_space(src_pd_phys);
        }
    }

    kfree(page_buffer);
    vmm_switch_address_space(saved_pd);
    return child_pd;
}