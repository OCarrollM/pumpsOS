// Real time clock
#include "rtc.h"
#include "ports.h"

#define CMOS_ADDRESS 0x70
#define CMOS_DATA 0x71

// Read one reg
static uint8_t cmos_read(uint8_t reg) {
    outb(CMOS_ADDRESS, reg);
    return inb(CMOS_DATA);
}

// check if a clock update is in progress at 0x0A
static int rtc_update_in_progress(void) {
    outb(CMOS_ADDRESS, 0x0A);
    return inb(CMOS_DATA) & 0x80;
}

// Read all time fields
static void rtc_read_raw(rtc_time_t* t) {
    while(rtc_update_in_progress()) { } // Just waiting for a moment
    t->second = cmos_read(0x00);
    t->minute = cmos_read(0x02);
    t->hour = cmos_read(0x04);
    t->day = cmos_read(0x07);
    t->month = cmos_read(0x08);
    t->year = cmos_read(0x09);
}

void rtc_read(rtc_time_t* out) {
    rtc_time_t a, b;

    // Read clock twice, but only accept if the two times are the same for accuracy
    rtc_read_raw(&a);
    do {
        b = a;
        rtc_read_raw(&a);
    } while (a.second != b.second || a.minute != b.minute || a.hour != b.hour || a.day != b.day || a.month != b.month || a.year != b.year);

    uint8_t regB = cmos_read(0x0B);
    int is_bcd = !(regB & 0x04);

    if (is_bcd) {
        #define BCD2BIN(v) (((v) & 0x0F) + (((v) >> 4) * 10))
        a.second = BCD2BIN(a.second);
        a.minute = BCD2BIN(a.minute);
        a.hour = BCD2BIN(a.hour & 0x7F) | (a.hour & 0x80); // for AM and PM
        a.day = BCD2BIN(a.day);
        a.month = BCD2BIN(a.month);
        a.year = BCD2BIN(a.year);
        #undef BCD2BIN
    }

    // 12 hour mode
    if (!(regB & 0x02) && (a.hour & 0x80)) {
        a.hour = ((a.hour & 0x7F) + 12) % 24;
    }

    // Year reg
    a.year += 2000; // 2000 AD that is

    *out = a;
}