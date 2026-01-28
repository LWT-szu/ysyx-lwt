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
#include <dlfcn.h>
#include <isa.h>
#include <cpu/difftest.h>
#include "../local-include/reg.h"
#include <dlfcn.h> 
// 把 cpu 和 ref_r 一一对比  NEMU作为DUT时使用
bool isa_difftest_checkregs(CPU_state *ref_r, vaddr_t pc) {
  bool match = true;
  
  // 比较pc
  if (ref_r->pc != cpu.pc) {
    printf("PC不一致: ref=0x%08x, dut=0x%08x\n", ref_r->pc, cpu.pc);
    match = false;
  }
  
  // 比较通用寄存器
  for (int i = 0; i < 32; i++) {
    if (ref_r->gpr[i] != cpu.gpr[i]) {
      printf("x%d不一致: ref=0x%08x, dut=0x%08x\n", 
             i, ref_r->gpr[i], cpu.gpr[i]);
      match = false;
    }
  }
  return match;
  //return !memcmp(ref_r, &cpu, sizeof(CPU_state));
}

void isa_difftest_attach() {
}
