// Currently only a validator
#include "elf.h"
#include <stdio.h>

#define KERNEL_BASE 0xC0000000

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
        printfO("Wrong architecture (e_machine=%d)\n", hdr->e_machine);
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

uint32_t elf_load(vfs_node_t* node, uint32_t pd_phys) {
    (void)node; (void)pd_phys;
    return 0;
}