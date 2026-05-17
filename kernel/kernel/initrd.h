#ifndef KERNEL_INITRD_H
#define KERNEL_INITRD_H

#include <stdint.h>
#include "vfs.h"
#include "multiboot.h"

vfs_node_t* initrd_init(uint32_t location, uint32_t size);
vfs_node_t* initrd_init_from_multiboot(multiboot_info_t* mboot);

#endif