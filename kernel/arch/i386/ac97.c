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

static uint16_t nam_base = 0; // Mixer port base
static uint16_t nabm_base = 0; // bus master port base
static volatile bdl_entry_t* bdl = 0;
static uint32_t bdl_phys = 0;
static uint16_t* audio_buf = 0;
static uint32_t audio_buf_phys = 0;
static uint32_t audio_dma_virt = VMEM_AUDIO_DMA;

static const int16_t sine_table[64] = {
         0,    980,   1950,   2902,   3826,   4713,   5555,   6343,
      7071,   7730,   8314,   8819,   9238,   9569,   9807,   9951,
     10000,   9951,   9807,   9569,   9238,   8819,   8314,   7730,
      7071,   6343,   5555,   4713,   3826,   2902,   1950,    980,
         0,   -980,  -1950,  -2902,  -3826,  -4713,  -5555,  -6343,
     -7071,  -7730,  -8314,  -8819,  -9238,  -9569,  -9807,  -9951,
    -10000,  -9951,  -9807,  -9569,  -9238,  -8819,  -8314,  -7730,
     -7071,  -6343,  -5555,  -4713,  -3826,  -2902,  -1950,   -980,
};

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
    memset((void*)virt, 0, 0x1000);
    audio_dma_virt += 0x1000;
    *virt_out = virt;
    return phys;
}

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

    audio_buf_phys = alloc_audio_page(&virt);
    if (!audio_buf_phys) return false;
    audio_buf = (int16_t*)virt;

    printf("AC97 BDL at phys 0x%x, buffer 0x%x\n", bdl_phys, audio_buf_phys);
    printf("AC97 codec ready!\n");
    return true;
}

bool ac97_play_tone(uint32_t freq_hz, uint32_t ms) {
    if (!bdl || !audio_buf) return false;
    (void)ms; // abuot 21ms

    const uint32_t total_samples = 4096 / sizeof(int16_t);
    const uint32_t frames = total_samples / CHANNELS;

    // steps through a table at a fast speed to produce a frequency
    uint32_t phase = 0;
    uint32_t step = (freq_hz * 64 * 65536) / SAMPLE_RATE;

    for (uint32_t i = 0; i < frames; i++) {
        int16_t s = sine_table[(phase >> 16) & 63];
        audio_buf[i * 2] = s;
        audio_buf[i * 2 + 1] = s;
        phase += step;
    }

    // stop and reset channel
    outb(nabm_base + NABM_PO_CR, 0);
    ac97_delay();
    outb(nabm_base + NABM_PO_CR, PO_CR_RESET);
    while (inb(nabm_base + NABM_PO_CR) & PO_CR_RESET) { }

    bdl[0].addr = audio_buf_phys;
    bdl[0].samples = (uint16_t)total_samples;
    bdl[0].control = BDL_IOC;

    outl(nabm_base + NABM_PO_BDBAR, bdl_phys);
    outb(nabm_base + NABM_PO_LVI, 0);

    printf("AC97 playing %d Hz\n", freq_hz);
    outb(nabm_base + NABM_PO_CR, PO_CR_RUN);
    return true;
}