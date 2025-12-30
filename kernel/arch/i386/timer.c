#include "timer.h"
#include "ports.h"
#include "pic.h"
#include "isr.h"
#include <stdio.h>

static volatile uint64_t ticks = 0;
static uint32_t timer_freq = 0;

static void timer_handler(struct registers* regs) {
    (void)regs; // not used
    ticks++;

    pic_send_eoi(IRQ_TIMER);
}

void timer_init(uint32_t frequency) {
    uint32_t divisor = PIT_FREQUENCY / frequency;

    if(divisor > 65535) {
        divisor = 65535;
    }
    if(divisor < 1) {
        divisor = 1;
    }

    timer_freq = PIT_FREQUENCY / divisor;

    isr_register_handler(32, timer_handler);

    outb(PIT_COMMAND, 0x36);

    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));

    pic_clear_mask(IRQ_TIMER);

    printf("Timer set at %d Hz\n", timer_freq);
}

uint64_t timer_get_ticks(void) {
    return ticks;
}

void timer_sleep(uint32_t ms) {
    uint64_t target = ticks + (ms * timer_freq) / 1000;

    while(ticks < target) {
        asm volatile("hlt");
    }
}