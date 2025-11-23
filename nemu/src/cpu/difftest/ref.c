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

#include <isa.h>
#include <cpu/cpu.h>
#include <difftest-def.h>
#include <memory/paddr.h>
#include <string.h>
// 在DUT host memory的`buf`和REF guest memory的`addr`之间拷贝`n`字节,
// `direction`指定拷贝的方向, `DIFFTEST_TO_DUT`表示往DUT拷贝, `DIFFTEST_TO_REF`表示往REF拷贝
__EXPORT void difftest_memcpy(paddr_t addr, void *buf, size_t n, bool direction) {
  if(direction == DIFFTEST_TO_DUT){
    memcpy(buf, guest_to_host(addr), n);
  }
  else{
    memcpy(guest_to_host(addr), buf, n);
  }
}

// `direction`为`DIFFTEST_TO_DUT`时, 获取REF的寄存器状态到`dut`;
// `direction`为`DIFFTEST_TO_REF`时, 设置REF的寄存器状态为`dut`;
// NEMU作为REF时使用
__EXPORT void difftest_regcpy(void *dut, bool direction) {
  //printf("dut=%p, direction=%d\n", dut, direction);
  //printf("NEMU: sizeof(cpu) = %ld\n", sizeof(cpu));
  if(direction == DIFFTEST_TO_DUT){
    memcpy(dut, &cpu, sizeof(cpu)); //???????
  }
  else{
    memcpy(&cpu, dut, sizeof(cpu));
  }
}
// 让REF执行`n`条指令
__EXPORT void difftest_exec(uint64_t n) {
  cpu_exec(n);
}

// 触发REF产生异常中断，NO为中断号
__EXPORT void difftest_raise_intr(word_t NO) {
  assert(0);
}
// 初始化REF的DiffTest功能
__EXPORT void difftest_init(int port) {
  void init_mem();
  init_mem();
  /* Perform ISA dependent initialization. */
  init_isa();
}
