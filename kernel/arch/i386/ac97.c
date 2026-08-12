// code for Intels driver

#include "ac97.h"
#include "pci.h"
#include "ports.h"
#include "../kernel/vmm.h"
#include "../kernel/pmm.h"
#include "../kernel/vmem.h"
#include <stdio.h>
#include <string.h>

#define AC97_VENDOR 0x8086
#define AC97_DEVICE 0x2415

// Mixer Registers
#define NAM_RESET 0x00
#define NAM_MASTER_VOLUME 0x02
#define NAM_PCM_OUT_VOLUME 0x18

// Bus master registers
#define NABM_PO_BDBAR 0x10
#define NABM_PO_CIV 0x14
#define NABM_PO_LVI 0x15
#define NABM_PO_SR 0x16
#define NABM_PO_PICB 0x18
#define NABM_PO_CR 0x1B
#define NABM_GLOBAL_CONTROL 0x2C
#define NABM_GLOBAL_STATUS 0x30

// Global control bits
#define GLOB_CNT_COLD_RESET (1 << 1)

// PCM out control bits
#define PO_CR_RUN (1 << 0)
#define PO_CR_RESET (1 << 1)
#define PO_CR_LVBIE (1 << 2)
#define PO_CR_IOCE (1 << 4)

// PCM out status bits
#define PO_SR_DCH (1 << 0)

#define SAMPLE_RATE 48000
#define CHANNELS 2
#define BDL_ENTRIES 32

typedef struct {
    uint32_t addr;
    uint16_t samples;
    uint16_t control;
}__attribute__((packed)) bdl_entry_t;

#define BDL_IOC (1 << 15)
#define BDL_BUP (1 << 14)

#define NUM_BUFFERS 4
#define BUF_BYTES 0x1000
#define BUF_SAMPLES (BUF_BYTES / sizeof(int16_t))
#define BUF_FRAMES (BUF_SAMPLES / AC97_CHANNELS)

static uint16_t nam_base = 0; // Mixer port base
static uint16_t nabm_base = 0; // bus master port base
static volatile bdl_entry_t* bdl = 0;
static uint32_t bdl_phys = 0;
static int16_t* buf_virt[NUM_BUFFERS];
static uint32_t buf_phys[NUM_BUFFERS];

static ac97_fill_t fill_cb = 0;
static bool playing = false;
static uint8_t next_buf = 0;

static uint32_t audio_dma_virt = VMEM_AUDIO_DMA;

static void ac97_delay(void) {
    for (volatile int i = 0; i < 100000; i++) { }
}

static uint32_t alloc_audio_page(uint32_t* virt_out) {
    uint32_t virt = audio_dma_virt;
    uint32_t phys = pmm_alloc_page();
    if (!phys) return 0;
    if (!vmm_map_page(virt, phys, PTE_PRESENT | PTE_WRITABLE)) {
        pmm_free_page(phys);
        return 0;
    }
    memset((void*)virt, 0, BUF_BYTES);
    audio_dma_virt += BUF_BYTES;
    *virt_out = virt;
    return phys;
}

bool ac97_is_playing(void) { return playing; }

bool ac97_init(void) {
    pci_device_t dev;
    if (!pci_find_by_id(AC97_VENDOR, AC97_DEVICE, &dev)) {
        printf("AC97 no card found\n");
        return false;
    }
    printf("AC97 found at %d:%d.%d irq=%d\n", dev.bus, dev.device, dev.function, dev.irq_line);

    pci_enable_device(&dev);

    nam_base = (uint16_t)(dev.bar[0] & 0xFFFFFFFC);
    nabm_base = (uint16_t)(dev.bar[1] & 0xFFFFFFFC);
    printf("AC97 Mixer ports at 0x%x, bus master at 0x%x\n", nam_base, nabm_base);

    // Bring controller out of cold reset
    outl(nabm_base + NABM_GLOBAL_CONTROL, GLOB_CNT_COLD_RESET);
    ac97_delay();

    outw(nam_base + NAM_RESET, 0);
    ac97_delay();

    printf("AC97 global status = 0x%x\n", inl(nabm_base + NABM_GLOBAL_STATUS));

    // 0 is loudest cause of attenuation
    outw(nam_base + NAM_MASTER_VOLUME, 0x0000);
    outw(nam_base + NAM_PCM_OUT_VOLUME, 0x0000);

    if (inw(nam_base + NAM_MASTER_VOLUME) == 0xFFFF) {
        printf("AC97 codec not responding\n");
        return false;
    }

    uint32_t virt;
    bdl_phys = alloc_audio_page(&virt);
    if (!bdl_phys) return false;
    bdl = (volatile bdl_entry_t*)virt;

    for (int i = 0; i < NUM_BUFFERS; i++) {
        buf_phys[i] = alloc_audio_page(&virt);
        if (!buf_phys[i]) return false;
        buf_virt[i] = (int16_t*)virt;
    }

    printf("AC97 %d buffers of %d frames (~%dms each)\n", NUM_BUFFERS, (int)BUF_FRAMES, (int)(BUF_FRAMES * 1000 / AC97_SAMPLE_RATE));
    printf("AC97 codec ready\n");
    return true;
}

static bool refill(uint8_t i) {
    uint32_t got = fill_cb ? fill_cb(buf_virt[i], BUF_FRAMES) : 0;
    if (got == 0) return false;

    if (got < BUF_FRAMES) {
        memset(buf_virt[i] + got * AC97_CHANNELS, 0, (BUF_FRAMES - got) * AC97_CHANNELS * sizeof(int16_t));
    }

    bdl[i].addr = buf_phys[i];
    bdl[i].samples = (uint16_t)BUF_SAMPLES;
    bdl[i].control = BDL_IOC;
    return true;
}

bool ac97_play(ac97_fill_t fill) {
    if (!bdl) return false;

    fill_cb = fill;

    // stop and reset channel
    outb(nabm_base + NABM_PO_CR, 0);
    ac97_delay();
    outb(nabm_base + NABM_PO_CR, PO_CR_RESET);
    while (inb(nabm_base + NABM_PO_CR) & PO_CR_RESET) { }

    int filled = 0;
    for (int i = 0; i < NUM_BUFFERS; i++) {
        if (!refill((uint8_t)i)) break;
        filled++;
    }
    if (filled == 0) return false;

    next_buf = 0;
    playing = true;

    outl(nabm_base + NABM_PO_BDBAR, bdl_phys);
    outb(nabm_base + NABM_PO_LVI, (uint8_t)(filled - 1));
    outb(nabm_base + NABM_PO_CR, PO_CR_RUN);

    return true;
}

void ac97_stop(void) {
    outb(nabm_base + NABM_PO_CR, 0);
    playing = false;
    fill_cb = 0;
}

void ac97_poll(void) {
    if (!playing) return;

    uint8_t civ = inb(nabm_base + NABM_PO_CIV);
    uint8_t lvi = inb(nabm_base + NABM_PO_LVI);

    while (lvi != (uint8_t)((civ + NUM_BUFFERS - 1) % NUM_BUFFERS)) {
        uint8_t slot = (uint8_t)((lvi + 1) % NUM_BUFFERS);
        if (!refill(slot)) {
            if (inb(nabm_base + NABM_PO_SR) & PO_SR_DCH) {
                ac97_stop();
                printf("AC97 playback finished\n");
            }
            return;
        }
        lvi = slot;
        outb(nabm_base + NABM_PO_LVI, lvi);
    }

    if (inb(nabm_base + NABM_PO_SR) & PO_SR_DCH) {
        ac97_stop();
        printf("AC97 playback finished\n");
    }
}