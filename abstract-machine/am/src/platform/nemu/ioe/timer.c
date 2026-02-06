#include <am.h>
#include <nemu.h>

void __am_timer_init() {
  outl(RTC_ADDR,0); // 初始化时将低32位置0
  outl(RTC_ADDR +4,0); // 初始化时将高32位置0
}

void __am_timer_uptime(AM_TIMER_UPTIME_T *uptime) {
  // 坑点位置：读取顺序 根据NEMU中只在读高32位的时候更新时间
  //必须 先读高位，再读低位
  uint32_t high = inl(RTC_ADDR + 4);
  uint32_t low = inl(RTC_ADDR);
  uptime->us = ((uint64_t)high <<32) | low ;
  //由于是 32 位架构，它必须分两次读取 RTC_ADDR 处的 64 位微秒计数器的低 32 位和高 32 位，
  //然后将它们合并成一个完整的 64 位值返回
}

void __am_timer_rtc(AM_TIMER_RTC_T *rtc) {
  rtc->second = 0;
  rtc->minute = 0;
  rtc->hour   = 0;
  rtc->day    = 0;
  rtc->month  = 0;
  rtc->year   = 1900;
}
