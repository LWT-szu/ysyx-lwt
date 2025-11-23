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
***************************************************************************************/

#include <memory/host.h>
#include <memory/paddr.h>
#include <device/mmio.h>
#include <isa.h>
// pmem是一个全局静态数组，大小为 CONFIG_MSIZE（RAM总容
#if   defined(CONFIG_PMEM_MALLOC)
static uint8_t *pmem = NULL;
#else // CONFIG_PMEM_GARRAY
static uint8_t pmem[CONFIG_MSIZE] PG_ALIGN = {};
#endif
//仿真器会用一个数组（比如 pmem）来模拟物理内存，这个数组的起始地址就是基地址

// 两个函数本身只是做地址转换
//  将来CPU访问内存时, 我们会将CPU将要访问的内存地址paddr映射到pmem数组中的相应偏移位置->guest_to_host()函数实现
//  例如如果mips32的CPU打算访问内存地址(物理地址CONFIG_MBASE)0x80000000, 我们会让它最终访问pmem[0]
uint8_t* guest_to_host(paddr_t paddr) { return pmem + paddr - CONFIG_MBASE; }
// 把内存pmem数组的位置下标转回物理地址
paddr_t host_to_guest(uint8_t *haddr) { return haddr - pmem + CONFIG_MBASE; }

static word_t pmem_read(paddr_t addr, int len) {
  word_t ret = host_read(guest_to_host(addr), len);
  return ret;
}

static void pmem_write(paddr_t addr, int len, word_t data) {
  host_write(guest_to_host(addr), len, data);
}
// 非法访问，错误处理
// static void out_of_bound(paddr_t addr) {
//   printf("Physical address " FMT_PADDR " is out of bound\n", addr);
//   fflush(stdout);
//   panic("address = " FMT_PADDR " is out of bound of pmem [" FMT_PADDR ", " FMT_PADDR "] at pc = " FMT_WORD,
//       addr, PMEM_LEFT, PMEM_RIGHT, cpu.pc);
// }

// 这个函数是模拟器/虚拟机中初始化物理内存的核心函数，通过条件编译支持不同的内存初始化策略，同时提供了内存分配和状态日志功能。
void init_mem() {
#if   defined(CONFIG_PMEM_MALLOC)
  pmem = malloc(CONFIG_MSIZE);
  assert(pmem);
#endif
  IFDEF(CONFIG_MEM_RANDOM, memset(pmem, rand(), CONFIG_MSIZE));
  //就会用 memset 把 pmem 区域所有字节都填为一个随机值（rand() 的低8位）。
  //printf("init mem done\n");
  Log("init mem done [" FMT_PADDR ", " FMT_PADDR "]", PMEM_LEFT, PMEM_RIGHT);
  Log("physical memory area [" FMT_PADDR ", " FMT_PADDR "]", PMEM_LEFT, PMEM_RIGHT);
}

// 从物理内存中读数据。
word_t paddr_read(paddr_t addr, int len) {
  if (likely(in_pmem(addr))) return pmem_read(addr, len);//从实际的物理内存数组里读数据（通常是模拟器内存）。
  IFDEF(CONFIG_DEVICE, return mmio_read(addr, len)); // 如果是访问外设，通过特殊的外设接口读数据。
  //out_of_bound(addr);
  return 0;
}

void paddr_write(paddr_t addr, int len, word_t data) {
  if (likely(in_pmem(addr))) { pmem_write(addr, len, data); return; }
  IFDEF(CONFIG_DEVICE, mmio_write(addr, len, data); return);
  // if (addr == 0xa00003f8)
  // {
  //   printf("paddr_read: addr=0x%08x len=%d pc=0x%08x\n", addr, len, cpu.pc);
  //   return ;
  // }
  //printf("paddr_write: addr=0x%08x len=%d data=0x%08x pc=0x%08x\n", addr, len, data, cpu.pc);
  fflush(stdout);
  //out_of_bound(addr);
}
