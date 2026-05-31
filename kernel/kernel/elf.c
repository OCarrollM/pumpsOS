// Currently only a validator
#include "elf.h"
#include "vmm.h"
#include "pmm.h"
#include <stdio.h>
#include <string.h>

#define KERNEL_BASE 0xC0000000

// Page flooring (round down)
#define PAGE_FLOOR(x) ((x) & ~(PAGE_SIZE - 1))

// Page ceiling (round up)
#define PAGE_CEIL(x) (((x) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))

// Lots of checks incase anything fails in the process
// and we will be informed about it
int elf_validate(const Elf32_Ehdr* hdr) {
    // File must have E L or F
    if (hdr->e_ident[EI_MAG0] != ELFMAG0 ||
        hdr->e_ident[EI_MAG1] != ELFMAG1 ||
        hdr->e_ident[EI_MAG2] != ELFMAG2 ||
        hdr->e_ident[EI_MAG3] != ELFMAG3) {
            printf("Bad magic\n");
            return 0;
    }

    // Check we are 32 bit
    if (hdr->e_ident[EI_CLASS] != ELFCLASS32) {
        printf("Not a 32-bit object\n");
        return 0;
    }
    if (hdr->e_ident[EI_DATA] != ELFDATA2LSB) {
        printf("Not little endian\n");
        return 0;
    }

    // Must be a exec
    if (hdr->e_type != ET_EXEC) {
        printf("Not an executable (e_type=%d)\n", hdr->e_type);
        return 0;
    }

    // Must target i386
    if (hdr->e_machine != EM_386) {
        printf("Wrong architecture (e_machine=%d)\n", hdr->e_machine);
        return 0;
    }

    // Need at least one program header
    if (hdr->e_phnum == 0) {
        printf("No program headers\n");
        return 0;
    }
    if (hdr->e_phentsize != sizeof(Elf32_Phdr)) {
        printf("Unexpected program header size (%d)\n", hdr->e_phentsize);
        return 0;
    }

    // Entry point must be in the user half (ring 3)
    if (hdr->e_entry >= KERNEL_BASE) {
        printf("Entry point 0x%x is in kernel space\n", hdr->e_entry);
        return 0;
    }

    return 1;
}


// Switch address spaces so the kernel code stack and globals are always 
// reachable
uint32_t elf_load(vfs_node_t* node, uint32_t pd_phys) {
    Elf32_Ehdr ehdr;
    if (node->length < sizeof(ehdr)) {
        printf("File too small for header (%d bytes)\n", node->length);
        return 0;
    }
    if (vfs_read(node, 0, sizeof(ehdr), (uint8_t*)&ehdr) != sizeof(ehdr)) {
        printf("Failed to read header\n");
        return 0;
    }
    if (!elf_validate(&ehdr)) {
        return 0;
    }

    // Bounds check
    uint32_t pht_bytes = (uint32_t)ehdr.e_phnum * sizeof(Elf32_Phdr);
    if (ehdr.e_phoff + pht_bytes > node->length) {
        printf("Program header table out of bounds\n");
        return 0;
    }

    // Switch into the target addr space for duration of load time
    uint32_t saved_pd = vmm_get_current_address_space();
    vmm_switch_address_space(pd_phys);

    // Walk program header
    for (uint16_t i = 0; i < ehdr.e_phnum; i++) {
        Elf32_Phdr phdr;
        uint32_t phoff = ehdr.e_phoff + i * sizeof(Elf32_Phdr);

        if (vfs_read(node, phoff, sizeof(phdr), (uint8_t*)&phdr) != sizeof(phdr)) {
            printf("Failed to read program header %d\n", i);
            goto fail;
        }

        // Ignore the non-loadable entries
        if (phdr.p_type != PT_LOAD) {
            continue;
        }

        if (phdr.p_memsz == 0) {
            continue;
        }

        // Safety features
        if (phdr.p_vaddr >= KERNEL_BASE || phdr.p_vaddr + phdr.p_memsz > KERNEL_BASE ||
            phdr.p_filesz > phdr.p_memsz) {
            printf("[ELF] segment %d has bad layout (vaddr=0x%x memsz=%d filesz=%d)\n",
                   i, phdr.p_vaddr, phdr.p_memsz, phdr.p_filesz);
            goto fail;
        }

        // File bounds check
        if (phdr.p_offset + phdr.p_filesz > node->length) {
            printf("Segment %d file data out of bounds\n", i);
            goto fail;
        }

        // Now we map every page in the half open range with PAGE_FLOOR/CEIL
        
        uint32_t start = PAGE_FLOOR(phdr.p_vaddr);
        uint32_t end = PAGE_CEIL(phdr.p_vaddr + phdr.p_memsz);

        for (uint32_t v = start; v < end; v+= PAGE_SIZE) {
            if (vmm_is_mapped(v)) {
                continue; // Skipping if already mapped
            }

            uint32_t phys = pmm_alloc_page();
            if (phys == 0) {
                printf("Out of physical memory at 0x%x\n", v);
                goto fail;
            }

            if (!vmm_map_page(v, phys, PTE_PRESENT | PTE_WRITABLE | PTE_USER)) {
                pmm_free_page(phys);
                printf("Failed to map 0x%x\n", v);
                goto fail;
            }

            // Wipe the fresh page
            memset((void*)v, 0, PAGE_SIZE);
        }

        // Now copy the actual file bytes to exactly vaddr if they aren't 0. Any other bytes are already 0
        if (phdr.p_filesz > 0) {
            if (vfs_read(node, phdr.p_offset, phdr.p_filesz, (uint8_t*)phdr.p_vaddr) != phdr.p_filesz) {
                printf("Failed to read segment %d data\n", i);
                goto fail;
            }
        }

        printf("[ELF] segment %d: vaddr=0x%x filesz=%d memsz=%d flags=%c%c%c\n",
               i, phdr.p_vaddr, phdr.p_filesz, phdr.p_memsz,
               (phdr.p_flags & PF_R) ? 'r' : '-',
               (phdr.p_flags & PF_W) ? 'w' : '-',
               (phdr.p_flags & PF_X) ? 'x' : '-'); // Like if you do ls -lh (-la) on terminal
    }

    vmm_switch_address_space(saved_pd);
    return ehdr.e_entry;

fail:
    vmm_switch_address_space(saved_pd);
    return 0;
}