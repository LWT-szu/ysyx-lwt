// #include <am.h>
// #define RTC_ADDR 0xa0000048

// void __am_timer_init() {
// }

// void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
//   //uptime->us = MMIO_READ(RTC_ADDR, uint32_t);
//   uint32_t low = *(volatile uint32_t *)RTC_ADDR;
//   uint32_t high = *(volatile uint32_t *)(RTC_ADDR + 4);
//   uptime->us = ((uint64_t)high << 32) | low;
// }

// void __am_timer_rtc(AM_TIMER_RTC_T *rtc) {
//   rtc->second = 0;
//   rtc->minute = 0;
//   rtc->hour   = 0;
//   rtc->day    = 0;
//   rtc->month  = 0;
//   rtc->year   = 1900;
// }

#include <riscv/riscv.h>
#include <am.h>

#define CSR_MCYCLE 0xB00
#define CSR_MCYCLEH 0xB80
#define DEVICE_BASE 0xa0000000
// #define MMIO_BASE = 0xa0000000
// 0x10000000
#define SERIAL_PORT (DEVICE_BASE + 0x00003f8)
// #define KBD_ADDR            (DEVICE_BASE + 0x0000060)
#define RTC_ADDR (DEVICE_BASE + 0x0000048)
// #define VGACTL_ADDR         (DEVICE_BASE + 0x0000100)
// #define AUDIO_ADDR          (DEVICE_BASE + 0x0000200)
// #define DISK_ADDR           (DEVICE_BASE + 0x0000300)
// #define FB_ADDR             (MMIO_BASE   + 0x1000000)
// #define AUDIO_SUBF_ADDR     (MMIO_BASE   + 0x1200000)

static uint64_t base_time = 0;
static uint64_t base_rtc = 0;
static uint64_t get_time()
{
  uint32_t high_1, high_2, low;
  do
  {
    __asm__ __volatile__("csrr %0, mcycleh" : "=r"(high_1));
    __asm__ __volatile__("csrr %0, mcycle" : "=r"(low));
    __asm__ __volatile__("csrr %0, mcycleh" : "=r"(high_2));

  } while (high_1 != high_2);

  return ((uint64_t)high_1 << 32) | low;
}

void __am_timer_init()
{
  base_time = get_time(); // 当前系统微秒计数（作为参考点）
  base_rtc = 1758672000;  // 2025年10月11日 00:00:00 UTC
}

void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime)
{
  uint64_t now = get_time();
  uptime->us = now - base_time; // (μs)
}

static void time_convert(uint64_t seconds, AM_TIMER_RTC_T *rtc)
{
  uint64_t secsond = seconds;
  rtc->second = secsond % 60;
  secsond /= 60;
  rtc->minute = secsond % 60;
  secsond /= 60;
  rtc->hour = secsond % 24;
  secsond /= 24;
  // 计算年份（从1970年开始）
  rtc->year = 1970 + secsond / 365;
  secsond %= 365;

  rtc->month = secsond / 30 + 1;
  secsond %= 30;
  rtc->day = secsond + 1;
}

void __am_timer_rtc(AM_TIMER_RTC_T *rtc)
{
  uint64_t seconds_runtime = (get_time() - base_time) / 1000000; // 把单位从微秒（μs）换成秒（s）
  uint64_t seconds_now = base_rtc + seconds_runtime;

  time_convert(seconds_now, rtc);
}
// void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
//   uptime->us = *(volatile uint32_t *)(RTC_ADDR + 4);
//   uptime->us <<=32;
//   uptime->us += *(volatile uint32_t *)(RTC_ADDR);
// }

// void __am_timer_rtc(AM_TIMER_RTC_T *rtc) {
//   rtc->second = 0;
//   rtc->minute = 0;
//   rtc->hour   = 0;
//   rtc->day    = 0;
//   rtc->month  = 0;
//   rtc->year   = 1900;
// }