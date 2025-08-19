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
* 模拟了串口的功能
***************************************************************************************/

#include <utils.h>
#include <device/map.h>

/* http://en.wikibooks.org/wiki/Serial_Programming/8250_UART_Programming */
// NOTE: this is compatible to 16550

#define CH_OFFSET 0

// 串口寄存器的虚拟内存区域，模拟8字节的串口寄存器组
static uint8_t *serial_base = NULL;


static void serial_putc(char ch) {
  MUXDEF(CONFIG_TARGET_AM, putch(ch), putc(ch, stderr));
  /*如果是 AM 平台（模拟器自带的抽象机），用 putch（AM的输出函数）。
  否则直接 putc(ch, stderr)，即输出到主机标准错误流，标准错误	stderr	屏幕（错误输出）
  你在命令行看到的字符就是这样来的
  具体是c的标准库干的事了，不用了解
  功能：向 指定的文件流（__stream） 写入一个字符。
  参数：__c：要写入的字符（以 int 形式传递，实际会转换为 unsigned char）。
  __stream：目标文件流（如 stdout、文件指针等）。*/
}

/*写数据寄存器（offset==0）时，把刚写入的那个字节通过 serial_putc 输出到主机终端。
读操作直接报错（NEMU只支持写串口）。*/
static void serial_io_handler(uint32_t offset, int len, bool is_write) {
  assert(len == 1);
  switch (offset) {// offset=0，对应串口数据寄存器
    /* We bind the serial port with the host stderr in NEMU. */
    case CH_OFFSET:
      if (is_write) serial_putc(serial_base[0]);// 主机输出
      else panic("do not support read");
      break;
    default: panic("do not support offset = %d", offset);
  }
}

void init_serial() {
  serial_base = new_space(8); // 分配8字节的串口寄存器空间
#ifdef CONFIG_HAS_PORT_IO
  add_pio_map ("serial", CONFIG_SERIAL_PORT, serial_base, 8, serial_io_handler);
#else
  add_mmio_map("serial", CONFIG_SERIAL_MMIO, serial_base, 8, serial_io_handler);
#endif
  /*把串口的物理端口号（如 0x3F8）或 MMIO 地址（如 0xa00003F8）映射到 serial_base，
  并注册serial_io_handler作为回调——以后只要有程序对这些端口/MMIO空间读写，就会调用 handler。*/
}
