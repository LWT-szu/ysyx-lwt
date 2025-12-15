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
//抛出一个号码为NO的异常, 其中epc为触发异常的指令PC, 返回异常处理的出口地址.
word_t isa_raise_intr(word_t NO, vaddr_t epc) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * Then return the address of the interrupt/exception vector.
   返回一个异常入口地址
   mtvec 的内容，实际就是定义“异常发生后，CPU该跳到哪个具体代码入口”。
   */
  cpu.csr.mcause = NO;
  cpu.csr.mepc   = epc;
  //printf("isa_raise_intr: mcause = %u, mepc = %u\n", cpu.csr.mcause, cpu.csr.mepc);
  //printf("cpu.gpr[17] = %u\n", cpu.gpr[17]); // a7
  return cpu.csr.mtvec;
}

// 查询当前是否有未处理的中断, 若有则返回中断号码, 否则返回INTR_EMPTY
word_t isa_query_intr() {
  return INTR_EMPTY;
}
