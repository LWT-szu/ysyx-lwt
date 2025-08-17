/***************************************************************************************
 * Copyright (c) 2014-2024 Zihao Yu, Nanjing University
 *
 * NEMU is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 *添加了一个自定义的时钟. i8253计时器初始化时会分别注册0x48处长度为8个字节的端口,
 *以及0xa0000048处长度为8字节的MMIO空间, 它们都会映射到两个32位的RTC寄存器.
 *CPU可以访问这两个寄存器来获得用64位表示的当前时间.
 ***************************************************************************************/

#include <device/map.h>
#include <device/alarm.h>
#include <utils.h>

// 指向RTC寄存器空间的指针（共8字节，两个32位寄存器，模拟64位时间戳）
static uint32_t *rtc_port_base = NULL;

// RTC设备的I/O处理函数
// offset: 访问偏移（0或4），len: 访问字节数，is_write: 读/写标志
// 读取offset=4（高32位）时，将当前64位us时间分别写入两个32位寄存器（低、高）
static void rtc_io_handler(uint32_t offset, int len, bool is_write) {
  assert(offset == 0 || offset == 4); // 只允许访问0和4两个偏移
  if (!is_write && offset == 4) {     // 仅在读高32位时更新时间
    uint64_t us = get_time();        // 获取当前us级时间戳
    rtc_port_base[0] = (uint32_t)us; // 低32位
    rtc_port_base[1] = us >> 32;
  }
}

#ifndef CONFIG_TARGET_AM
// 时钟中断处理函数，仅在NEMU仿真状态下有效
static void timer_intr() {
  if (nemu_state.state == NEMU_RUNNING) {
    extern void dev_raise_intr();
    dev_raise_intr();
  }
}
#endif
// RTC/定时器初始化函数
void init_timer() {
  rtc_port_base = (uint32_t *)new_space(8); // // 分配8字节空间用于模拟RTC寄存器
#ifdef CONFIG_HAS_PORT_IO
  add_pio_map ("rtc", CONFIG_RTC_PORT, rtc_port_base, 8, rtc_io_handler);
#else
  // 注册端口映射（如PC机I/O端口0x48），与rtc_io_handler关联
  // CPU访问rtc_port_base获取时间，相关MMIO空间CONFIG_RTC_MMIO (0xa0000048)会被映射到rtc_port_base
  add_mmio_map("rtc", CONFIG_RTC_MMIO, rtc_port_base, 8, rtc_io_handler);
#endif
  IFNDEF(CONFIG_TARGET_AM, add_alarm_handle(timer_intr));
}
