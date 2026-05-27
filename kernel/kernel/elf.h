#ifndef KERNEL_ELF_H
#define KERNEL_ELF_H

#include <stdint.h>
#include "vfs.h"

// This file acts as an ELF executable which will help
// for program loading

// ELF Header
// 16 byte id array 'E', 'L', 'F'

#define EI_NIDENT 16
#define EI_MAG0 0
#define EI_MAG1 1
#define EI_MAG2 2
#define EI_MAG3 3
#define EI_CLASS 4
#define EI_DATA 5
#define ELFMAG0 0x7F
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'
#define ELFCLASS32 1
#define ELFDATA2LSB 1
#define ET_NONE 0
#define ET_REL 1
#define ET_EXEC 2
#define ET_DYN 3
#define EM_386 3

typedef struct {
    uint8_t  e_ident[EI_NIDENT]; // magic + class + encoding + padding 
    uint16_t e_type;             // object file type (expect ET_EXEC) 
    uint16_t e_machine;          // architecture (expect EM_386) 
    uint32_t e_version;          // object file version 
    uint32_t e_entry;            // virtual address to start execution 
    uint32_t e_phoff;            // program header table file offset 
    uint32_t e_shoff;            // section header table file offset 
    uint32_t e_flags;            // processor-specific flags 
    uint16_t e_ehsize;           // ELF header size in bytes 
    uint16_t e_phentsize;        // size of one program header entry 
    uint16_t e_phnum;            // number of program header entries 
    uint16_t e_shentsize;        // size of one section header entry
    uint16_t e_shnum;            // number of section header entries 
    uint16_t e_shstrndx;         // section header string table index 
} __attribute__((packed)) Elf32_Ehdr;

// Program header

// One per second

#define PT_NULL 0
#define PT_LOAD 1
#define PT_DYNAMIC 2
#define PT_INTERP 3
#define PT_NOTE 4
#define PT_PHDR 6

// flags
#define PF_X 0x1 // Exec
#define PF_W 0x2 // Write
#define PF_R 0x4 // Read

typedef struct {
    uint32_t p_type;      // segment type (PT_LOAD etc.) 
    uint32_t p_offset;    // file offset of segment's first byte 
    uint32_t p_vaddr;     // virtual address to place the segment at 
    uint32_t p_paddr;     // physical address (ignored; we have paging) 
    uint32_t p_filesz;    // bytes of the segment present in the file 
    uint32_t p_memsz;     // bytes the segment occupies in memory 
    uint32_t p_flags;     // PF_R / PF_W / PF_X 
    uint32_t p_align;     // alignment constraint 
} __attribute__((packed)) Elf32_Phdr; 

// We load an ELF exec from a VFS node into the addr space
// If a success, we return the entry point virtual address
// Fail returns 0
uint32_t elf_load(vfs_node_t* node, uint32_t pd_phys);
int elf_validate(const Elf32_Ehdr* hdr);

#endif