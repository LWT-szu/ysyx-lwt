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
* 这段代码就是NEMU中端口映射I/O的模拟层，它负责管理所有通过“端口号”访问的I/O设备，
* 将端口号访问请求转发给已注册的设备模拟区域，实现端口I/O的读写
* 通过注册映射、查找映射、统一读写接口，
* 把CPU对端口号的访问请求映射到设备空间，触发设备回调，完成I/O模拟。
***************************************************************************************/

#include <device/map.h>

#define PORT_IO_SPACE_MAX 65535

#define NR_MAP 16
// 保存所有注册的PIO设备的映射关系（每个条目描述一个设备的端口区间、内存空间、回调函数等）
static IOMap maps[NR_MAP] = {};
static int nr_map = 0; // 当前已经注册的设备映射数量

/* device interface */
// 注册一个端口I/O设备，比如给某个端口区间分配一段空间，指定访问该空间时要调用的回调函数
/*把映射关系存进 maps[]，方便后续查找
name：设备名，便于调试。
addr：起始端口号。
space：设备数据/寄存器的存储空间。
len：占用多少端口。
callback：设备回调函数，访问时触发模拟设备的动作。*/
void add_pio_map(const char *name, ioaddr_t addr, void *space, uint32_t len, io_callback_t callback) {
  assert(nr_map < NR_MAP);
  assert(addr + len <= PORT_IO_SPACE_MAX);
  maps[nr_map] = (IOMap){ .name = name, .low = addr, .high = addr + len - 1,
    .space = space, .callback = callback };
  Log("Add port-io map '%s' at [" FMT_PADDR ", " FMT_PADDR "]",
      maps[nr_map].name, maps[nr_map].low, maps[nr_map].high);

  nr_map ++;
}

// 目前没地方用过这个
/* CPU interface 当CPU代码通过端口号“读”时，会调用这个函数*/
uint32_t pio_read(ioaddr_t addr, int len) {
  assert(addr + len - 1 < PORT_IO_SPACE_MAX);
  // 在 maps[] 里查找这个端口号属于哪个设备映射（用 find_mapid_by_addr）
  int mapid = find_mapid_by_addr(maps, nr_map, addr);
  assert(mapid != -1);
  // 找到对应IOMap后，调用 map_read() 执行实际的设备空间访问和回调
  return map_read(addr, len, &maps[mapid]); 
}

// 目前没地方用过这个
// 当CPU代码通过端口号“写”时，可以调用这个函数
void pio_write(ioaddr_t addr, int len, uint32_t data) {
  assert(addr + len - 1 < PORT_IO_SPACE_MAX);
  int mapid = find_mapid_by_addr(maps, nr_map, addr);
  assert(mapid != -1);
  map_write(addr, len, data, &maps[mapid]);
}
