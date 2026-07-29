// Kernels virtual addr layout

#ifndef KERNEL_VMEM_H
#define KERNEL_VMEM_H

/*  Virtual base    PD index   Region
 *  ------------    --------   ------------------------------------------
 *  0xC0000000        768      Kernel image (code, data, bss)
 *  0xD0000000        832      Kernel heap
 *  0xE0000000        896      vmm_create_address_space() scratch mapping
 *  0xE8000000        928      E1000 NIC MMIO registers (128 KB)
 *  0xF0000000        960      Linear framebuffer (3 MB at 1024x768x32)
 *  0xFFC00000       1023      Recursive page directory
 * 0xE9000000       932        NIC Descriptor
 */

#define VMEM_KERNEL_BASE 0xC0000000u
#define VMEM_HEAP_BASE 0xD0000000u
#define VMEM_VMM_SCRATCH 0xE0000000u
#define VMEM_E1000_MMIO 0xE8000000u
#define VMEM_FRAMEBUFFER 0xF0000000u
#define VMEM_RECURSIVE_PD 0xFFC00000u
#define VMEM_NET_DMA 0xE9000000u

#endif