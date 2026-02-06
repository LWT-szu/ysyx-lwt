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
#include <cpu/decode.h>
#include <string.h>

extern bool key_read;
word_t key_ret = 0; // [NEW] 用于传递读取到的值给 cpu-exec.c 展示

#define IO_SPACE_MAX (32 * 1024 * 1024)

// 静态分配一块I/O空间，用于映射所有外设的寄存器空间（无论是端口映射还是内存映射）
static uint8_t *io_space = NULL; // 物理存储：实际保存所有设备寄存器的数据
static uint8_t *p_space = NULL;  // 指向 io_space 中 当前可分配区域的起始地址，用于动态分配设备所需的寄存器空间

// 分配一段对齐到页的I/O空间，返回首地址用于设备映射
uint8_t* new_space(int size) {
  uint8_t *p = p_space;//接着上个设备分配的空间继续分配
  // 向上对齐到页边界（保证分配的空间是页对齐的，便于后续管理和性能优化）
  // page aligned;
  printf("size before align = %d\n", size);
  size = (size + (PAGE_SIZE - 1)) & ~PAGE_MASK; // 就是十六进制对齐，比如size=8，把整数8，变成4096，对应0x1000
  printf("size after align = %d\n", size);
  p_space += size; // 移动p_space指针，分配完当前设备后，指向下一个可分配空间的起始地址
  printf("p_space = %p\n", p_space);
  assert(p_space - io_space < IO_SPACE_MAX);
  return p;
}
// PAGE_SIZE = 4096，PAGE_MASK = 4095（即0xFFF）
// 只要 size 小于等于 4096，对齐后都会变成4096
// size = 8
// size + (PAGE_SIZE - 1) = 8 + 4095 = 4103
// ~PAGE_MASK = ~0xFFF = 0xFFFFF000
// 4103 & 0xFFFFF000 = 4096

// size = 4
// size + (PAGE_SIZE - 1) = 4 + 4095 = 4099
// 4099 & 0xFFFFF000 = 4096

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
  io_space = malloc(IO_SPACE_MAX);// 分配32MB的I/O空间
  assert(io_space);
  p_space = io_space;// 初始化可分配空间指针
}

// 统一的I/O读接口：将addr地址映射到map指示的目标空间，并读取数据
/*“映射”，就是把CPU读写的物理地址映射到设备内部的存储空间（通常是个内存buffer），
并通过回调函数与真实设备状态同步*/
//键盘
word_t map_read(paddr_t addr, int len, IOMap *map) {
  assert(len >= 1 && len <= 8);
  check_bound(map, addr); // 1. 检查访问地址是否合法
  paddr_t offset = addr - map->low;
  // 计算addr在设备内部空间的偏移,比如map->low=0xa0000048、addr=0xa000004c，则offset=4
  invoke_callback(map->callback, offset, len, false); // prepare data to read 2. 调用回调函数（如有），用于同步设备状态
  word_t ret = host_read(map->space + offset, len);   // 3. 从从map->space读出us值.读取数据
  
  // [NEW] 状态机观测逻辑 (Before State & Action)
  // 增加限制: && ret != 0 (只有在真的读到按键数据时才打印，屏蔽掉轮询的无效日志)
      // 这里的 val 读取可能不准确，因为不知道编译器用了哪个寄存器，为了避免误导，我们先不打印具体寄存器值
      // bool success;
      // word_t val = isa_reg_str2val("a0", &success);
      key_ret = ret; // 保存返回值用于 After 阶段展示
      if(strcmp(map->name, "keyboard") == 0 && ret != 0){
        printf("\n\033[1;33m[Microscopic View: State Machine Transition]\033[0m\n");
        // 去掉多余的 0x 前缀，假定 FMT_WORD 不带 0x
        printf("\033[1;36mBefore: PC=" FMT_WORD " a4 = 0x" FMT_WORD "\033[0m\n", cpu.pc, cpu.gpr[14]);
        printf("\033[1;35mAction: Executing Load Instruction on KBD_ADDR (0xa0000060)\033[0m\n");
        printf("\033[1;35m        (Hardware Queue -> MMIO -> CPU)\033[0m\n");
        printf("\033[1;35m        (Return value from key_queue: 0x%x)\033[0m\n", ret);
        key_read = true;
      }
  //测试键盘
  if(strcmp(map->name, "keyboard") == 0 && ret != 0)
     printf("[key_board]map->space= %p,offset = %d data=0x%x\n", map->space, offset,ret);
#ifdef CONFIG_DTRACE
    //printf("[DTrace] PC=0x%x READ: Device=%s addr=%08x data=0x%x len=%d\n", cpu.pc, map->name, addr, ret, len);

#endif
  return ret;
}

// 统一的I/O写接口：将addr地址映射到map指示的目标空间，并写入数据,串口
void map_write(paddr_t addr, int len, word_t data, IOMap *map) {
  assert(len >= 1 && len <= 8);
  check_bound(map, addr);
  paddr_t offset = addr - map->low;// 计算addr在设备内部空间的偏移
  host_write(map->space + offset, len, data);
  //if (strcmp(map->name, "serial") == 0)
    //printf("map->space= %p,offset = %d\n", map->space, offset);
  invoke_callback(map->callback, offset, len, true);//其实只有偏移量为0时才有意义
#ifdef CONFIG_DTRACE
  //printf("[DTrace] PC=0x%x WRITE: Device=%s addr=%08x data=0x%x len=%d\n", cpu.pc, map->name, addr, data, len);
#endif
}
