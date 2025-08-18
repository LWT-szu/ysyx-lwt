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
 * 提供了统一的空间分配和实际读写逻辑，
 * 实现了映射的管理, 包括I/O空间的分配及其映射, 还有映射的访问接口.
 * 无论端口映射I/O还是内存映射I/O，都被抽象成“地址区间+访问回调+数据空间”的统一映射，
 * 访问时只需查找映射并操作即可，区别仅是映射的地址区间不同
 ***************************************************************************************/

#include <isa.h>
#include <memory/host.h>
#include <memory/vaddr.h>
#include <device/map.h>

#define IO_SPACE_MAX (32 * 1024 * 1024)

// 静态分配一块I/O空间，用于映射所有外设的寄存器空间（无论是端口映射还是内存映射）
static uint8_t *io_space = NULL; // 物理存储：实际保存所有设备寄存器的数据
static uint8_t *p_space = NULL;  // 指向 io_space 中 当前可分配区域的起始地址，用于动态分配设备所需的寄存器空间

// 分配一段对齐到页的I/O空间，返回首地址用于设备映射
uint8_t* new_space(int size) {
  uint8_t *p = p_space;
  // page aligned;
  size = (size + (PAGE_SIZE - 1)) & ~PAGE_MASK;
  p_space += size;
  assert(p_space - io_space < IO_SPACE_MAX);
  return p;
}

// 检查地址是否在IOMap描述的映射范围内
static void check_bound(IOMap *map, paddr_t addr) {
  if (map == NULL) {
    Assert(map != NULL, "address (" FMT_PADDR ") is out of bound at pc = " FMT_WORD, addr, cpu.pc);
  } else {
    Assert(addr <= map->high && addr >= map->low,
        "address (" FMT_PADDR ") is out of bound {%s} [" FMT_PADDR ", " FMT_PADDR "] at pc = " FMT_WORD,
        addr, map->name, map->low, map->high, cpu.pc);
  }
}

// 调用设备的回调函数（如有），每次进行I/O读写的时候, 才会调用设备提供的回调函数
static void invoke_callback(io_callback_t c, paddr_t offset, int len, bool is_write) {
  if (c != NULL) { c(offset, len, is_write); }
}

// 初始化I/O映射空间
void init_map() {
  io_space = malloc(IO_SPACE_MAX);
  assert(io_space);
  p_space = io_space;
}

// 统一的I/O读接口：将addr地址映射到map指示的目标空间，并读取数据
/*“映射”，就是把CPU读写的物理地址映射到设备内部的存储空间（通常是个内存buffer），
并通过回调函数与真实设备状态同步*/
word_t map_read(paddr_t addr, int len, IOMap *map) {
  assert(len >= 1 && len <= 8);
  check_bound(map, addr); // 1. 检查访问地址是否合法
  paddr_t offset = addr - map->low;
  // 计算addr在设备内部空间的偏移,比如map->low=0xa0000048、addr=0xa000004c，则offset=4
  invoke_callback(map->callback, offset, len, false); // prepare data to read 2. 调用回调函数（如有），用于同步设备状态
  word_t ret = host_read(map->space + offset, len);   // 3. 从从map->space读出us值.读取数据
  //printf("[DTrace] PC=0x%x WRITE %s.%08x data=0x%x\n",pc,map->name,addr,ret);
  return ret;
}

// 统一的I/O写接口：将addr地址映射到map指示的目标空间，并写入数据
void map_write(paddr_t addr, int len, word_t data, IOMap *map) {
  assert(len >= 1 && len <= 8);
  check_bound(map, addr);
  paddr_t offset = addr - map->low;
  host_write(map->space + offset, len, data);
  invoke_callback(map->callback, offset, len, true);
}
