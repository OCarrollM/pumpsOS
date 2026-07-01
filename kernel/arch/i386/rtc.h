// real time clock wiki.osdev.org/RTC
#ifndef ARCH_I386_RTC_H
#define ARCH_I386_RTC_H

#include <stdint.h>

typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint32_t year;
} rtc_time_t;

void rtc_read(rtc_time_t* out);

#endif