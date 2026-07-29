// Intels driver
#include "e1000.h"
#include "pci.h"
#include "../kernel/vmm.h"
#include "../kernel/pmm.h"
#include "../kernel/vmem.h"
#include <stdio.h>
#include <string.h>

#define E1000_VENDOR 0x8086
#define E1000_DEVICE 0x100E

// Register the offsets
#define E1000_CTRL 0x0000
#define E1000_STATUS 0x0008
#define E1000_EERD 0x0014
#define E1000_TCTL 0x0400
#define E1000_TIPG 0x0410
#define E1000_RAL 0x5400
#define E1000_RAH 0x5404
#define E1000_TDBAL 0x3800
#define E1000_TDBAH 0x3804
#define E1000_TDLEN 0x3808
#define E1000_TDH 0x3810
#define E1000_TDT 0x3818

// EERD Bits
#define EERD_START (1 << 0)
#define EERD_DONE (1 << 4)
#define EERD_ADDR_SHIFT 8
#define EERD_DATA_SHIFT 16

// TCTL bits
#define TCTL_EN (1 << 1)
#define TCTL_PSP (1 << 3)
#define TCTL_CT_SHIFT 4
#define TCTL_COLD_SHIFT 12

// tx descriptor bits
#define TXD_CMD_EOP (1 << 0)
#define TXD_CMD_IFCS (1 << 1)
#define TXD_CMD_RS (1 << 3)
#define TXD_STAT_DD (1 << 0)

// cards register space
#define E1000_MMIO_SIZE 0x20000

#define TX_DESC_COUNT 8
#define TX_BUF_SIZE 2048
#define PAGE_SZ 0x1000

// transmit descriptor
typedef struct {
    uint64_t addr;
    uint16_t length;
    uint8_t cso;
    uint8_t cmd;
    uint8_t status;
    uint8_t css;
    uint16_t special;
} __attribute__((packed)) tx_desc_t;

static uint32_t mmio = 0;
static uint8_t mac[6];
static bool eeprom_present = false;
static volatile tx_desc_t* tx_ring = 0;
static uint32_t tx_ring_phys = 0;
static uint8_t* tx_buf[TX_DESC_COUNT];

// register access
static inline uint32_t e1000_read(uint32_t reg) {
    return *(volatile uint32_t*)(mmio + reg);
}
static inline void e1000_write(uint32_t reg, uint32_t value) {
    *(volatile uint32_t*)(mmio + reg) = value;
}

static void e1000_detect_eeprom(void) {
    e1000_write(E1000_EERD, EERD_START);
    for (int i = 0; i < 1000; i++) {
        if (e1000_read(E1000_EERD) & EERD_DONE) {
            eeprom_present = true;
            return;
        }
    }
    eeprom_present = false;
}

static uint16_t eeprom_read(uint8_t addr) {
    e1000_write(E1000_EERD, ((uint32_t)addr << EERD_ADDR_SHIFT) | EERD_START);
    uint32_t val;
    int timeout = 100000;
    do {
        val = e1000_read(E1000_EERD);
    } while (!(val & EERD_DONE) && --timeout);
    return (uint16_t)(val >> EERD_DATA_SHIFT);
}
 // Read mac
static void e1000_read_mac(void) {
    if (eeprom_present) {
        uint16_t w;
        w = eeprom_read(0); mac[0] = w & 0xFF; mac[1] = w >> 8;
        w = eeprom_read(1); mac[2] = w & 0xFF; mac[3] = w >> 8;
        w = eeprom_read(2); mac[4] = w & 0xFF; mac[5] = w >> 8;
    } else {
        uint32_t ral = e1000_read(E1000_RAL);
        uint32_t rah = e1000_read(E1000_RAH);
        mac[0] = (ral) & 0xFF;
        mac[1] = (ral >> 8) & 0xFF;
        mac[2] = (ral >> 16) & 0xFF;
        mac[3] = (ral >> 24) & 0xFF;
        mac[4] = (rah) & 0xFF;
        mac[5] = (rah >> 8) & 0xFF;
    }
}

const uint8_t* e1000_mac(void) { return mac; }

// Allocoate one phys page and map it at virt
static uint32_t alloc_dma_page(uint32_t virt) {
    uint32_t phys = pmm_alloc_page();
    if (!phys) return 0;
    if (!vmm_map_page(virt, phys, PTE_PRESENT | PTE_WRITABLE)) {
        pmm_free_page(phys);
        return 0;
    }
    memset((void*)virt, 0, PAGE_SZ);
    return phys;
}

// Build the transmit ring
static bool e1000_setup_tx(void) {
    uint32_t virt = VMEM_NET_DMA;

    tx_ring_phys = alloc_dma_page(virt);
    if (!tx_ring_phys) return false;
    tx_ring = (tx_desc_t*)virt;
    virt += PAGE_SZ;

    for (int i = 0; i < TX_DESC_COUNT; i += 2) {
        uint32_t phys = alloc_dma_page(virt);
        if (!phys) return false;

        tx_buf[i] = (uint8_t*)virt;
        tx_ring[i].addr = phys;

        tx_buf[i+1] = (uint8_t*)(virt + TX_BUF_SIZE);
        tx_ring[i+1].addr = phys + TX_BUF_SIZE;

        virt += PAGE_SZ;
    }

    for (int i = 0; i < TX_DESC_COUNT; i++) {
        tx_ring[i].cmd = 0;
        tx_ring[i].status = TXD_STAT_DD;
    }

    e1000_write(E1000_TDBAL, tx_ring_phys);
    e1000_write(E1000_TDBAH, 0);
    e1000_write(E1000_TDLEN, TX_DESC_COUNT * sizeof(tx_desc_t));
    e1000_write(E1000_TDH, 0);
    e1000_write(E1000_TDT, 0);
    e1000_write(E1000_TCTL, TCTL_EN | TCTL_PSP | (15 << TCTL_CT_SHIFT) | (64 << TCTL_COLD_SHIFT));
    e1000_write(E1000_TIPG, 10 | (8 << 10) | (6 << 20));

    printf("E1000 TX ring at phys 0x%x (%d descriptors)\n", tx_ring_phys, TX_DESC_COUNT);
    return true;
}

bool e1000_send(const void* data, uint16_t len) {
    if (!tx_ring || len > TX_BUF_SIZE) return false;

    uint32_t tail = e1000_read(E1000_TDT);

    memcpy(tx_buf[tail], data, len);

    tx_ring[tail].length = len;
    tx_ring[tail].cso = 0;
    tx_ring[tail].css = 0;
    tx_ring[tail].special = 0;
    tx_ring[tail].status = 0;
    tx_ring[tail].cmd = TXD_CMD_EOP | TXD_CMD_IFCS | TXD_CMD_RS;

    e1000_write(E1000_TDT, (tail + 1) % TX_DESC_COUNT);

    int timeout = 1000000;
    while (!(tx_ring[tail].status & TXD_STAT_DD) && --timeout) { }
    if (!timeout) {
        printf("E1000 trasmit timed out\n");
        return false;
    }
    return true;
}

bool e1000_init(void) {
    pci_device_t dev;
    if (!pci_find_by_id(E1000_VENDOR, E1000_DEVICE, &dev)) {
        printf("No E1000 card found\n");
        return false;
    }

    printf("E1000 card found at %d:%d.%d irq=%d\n", dev.bus, dev.device, dev.function, dev.irq_line);

    // let card do I/O and respond to mem reads
    pci_enable_device(&dev);

    uint32_t bar0 = dev.bar[0] & 0xFFFFFFF0u;
    printf("E1000 BAR0 phys = 0x%x\n", bar0);

    // map register space into kernel half
    uint32_t pages = E1000_MMIO_SIZE / 0x1000;
    for (uint32_t i = 0; i < pages; i++) {
        if (!vmm_map_page(VMEM_E1000_MMIO + i * 0x1000, bar0 + i * 0x1000, PTE_PRESENT | PTE_WRITABLE)) {
            printf("E1000 failed to map registers\n");
            return false;
        }
    }
    mmio = VMEM_E1000_MMIO;

    // for sanity AHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH
    uint32_t status = e1000_read(E1000_STATUS);
    printf("E1000 Status = 0x%x\n", status);

    e1000_detect_eeprom();
    printf("E1000 eeprom %s\n", eeprom_present ? "present" : "absent");

    e1000_read_mac();
    printf("E1000 mac = %02x:%02x:%02x:%02x:%02x:%02x\n",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    if (!e1000_setup_tx()) {
        printf("E1000 TX setup failed\n");
    }
    return true;
}