#include <am.h>

#include <stdint.h>

#define RTC_ADDR 0xa0000048
#define MMIO_READ(addr, type) (*(volatile type *)(uintptr_t)(addr))

void __am_timer_init() {
}

void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
  //uptime->us = MMIO_READ(RTC_ADDR, uint32_t);
  uint32_t low = *(volatile uint32_t *)RTC_ADDR;
  uint32_t high = *(volatile uint32_t *)(RTC_ADDR + 4);
  uptime->us = ((uint64_t)high << 32) | low;
}

void __am_timer_rtc(AM_TIMER_RTC_T *rtc) {
  rtc->second = 0;
  rtc->minute = 0;
  rtc->hour   = 0;
  rtc->day    = 0;
  rtc->month  = 0;
  rtc->year   = 1900;
}
