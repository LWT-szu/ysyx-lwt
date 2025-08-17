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
* 定义了统一的I/O映射抽象和接口
* NEMU通过IOMap和map_read/map_write，把PIO和MMIO都统一成“映射区间+回调+数据空间”的通用机制，
* 实现了两类I/O的模拟，外部用不同的add_xxx_map注册即可，访问时都是同一套接口。
***************************************************************************************/

#ifndef __DEVICE_MAP_H__
#define __DEVICE_MAP_H__

#include <cpu/difftest.h>

typedef void(*io_callback_t)(uint32_t, int, bool);
// 分配一段I/O空间用于设备映射（空间由map.c统一管理）
uint8_t* new_space(int size);

/*IOMap结构体用于描述一个I/O映射关系（包括端口映射和内存映射）
  通过统一的映射结构体，NEMU可以方便地支持不同的I/O编址方式*/
typedef struct {
  const char *name;
  // we treat ioaddr_t as paddr_t here
  paddr_t low;         // 内存映射的起始物理地址（或端口号）
  paddr_t high;        // 内存映射的结束物理地址（或端口号）
  void *space;         // 指向设备寄存器/内存的实际存储空间
  io_callback_t callback; // 设备的回调函数，用于处理I/O访问
} IOMap;

// 检查addr是否在映射区间内
static inline bool map_inside(IOMap *map, paddr_t addr) {
  return (addr >= map->low && addr <= map->high);
}

// 根据地址查找对应的IOMap映射，便于I/O访问时定位设备
static inline int find_mapid_by_addr(IOMap *maps, int size, paddr_t addr) {
  int i;
  for (i = 0; i < size; i ++) {
    if (map_inside(maps + i, addr)) {
      difftest_skip_ref();
      return i;
    }
  }
  return 0;
}

// 为端口映射I/O和内存映射I/O分别添加映射关系
void add_pio_map(const char *name, ioaddr_t addr,
        void *space, uint32_t len, io_callback_t callback);
void add_mmio_map(const char *name, paddr_t addr,
        void *space, uint32_t len, io_callback_t callback);

// 统一的I/O读/写接口（由map.c实现）
word_t map_read(paddr_t addr, int len, IOMap *map);
void map_write(paddr_t addr, int len, word_t data, IOMap *map);

#endif
