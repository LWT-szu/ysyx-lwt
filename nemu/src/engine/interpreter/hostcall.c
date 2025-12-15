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
* 在NEMU中，CPU取指（ifetch）、译码、执行等过程中，如果遇到未能识别或实现的指令，
* 就会调用invalid_inst函数，报告错误并终止模拟，打印调试信息并终止仿真
***************************************************************************************/

#include <utils.h>
#include <cpu/ifetch.h>
#include <isa.h>
#include <cpu/difftest.h>

//被定义成一个宏了 在nemu/include/cpu/cpu.h
void set_nemu_state(int state, vaddr_t pc, int halt_ret) {
  // 通知参考模型跳过本次指令（通常用于差分测试，防止参考和DUT步调混乱）
  difftest_skip_ref();
  // 设置NEMU模拟器的状态
  nemu_state.state = state; // 设定模拟器当前的运行状态，比如停止、异常等
  nemu_state.halt_pc = pc;  // 记录导致模拟器终止的指令的PC（程序计数器）
  nemu_state.halt_ret = halt_ret; // 终止时返回值，通常-1表示异常,0 通常代表正常终止
}

__attribute__((noinline))
//被定义成一个宏了 在nemu/include/cpu/cpu.h
void invalid_inst(vaddr_t thispc) {
  uint32_t temp[2];
  vaddr_t pc = thispc; // 当前触发异常的指令的PC值（程序计数器，指明是哪条指令出错了）
  // 取出出错位置的两条指令数据（8字节）
  temp[0] = inst_fetch(&pc, 4);
  temp[1] = inst_fetch(&pc, 4);

  uint8_t *p = (uint8_t *)temp;
  // 打印出错指令的具体内容，便于调试
  printf("invalid opcode(PC = " FMT_WORD "):\n"
      "\t%02x %02x %02x %02x %02x %02x %02x %02x ...\n"
      "\t%08x %08x...\n",
      thispc, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], temp[0], temp[1]);

  // 打印提示信息，说明两种可能：1. 未实现指令 2. 指令实现有误
  printf("There are two cases which will trigger this unexpected exception:\n"
      "1. The instruction at PC = " FMT_WORD " is not implemented.\n"
      "2. Something is implemented incorrectly.\n", thispc);
  printf("Find this PC(" FMT_WORD ") in the disassembling result to distinguish which case it is.\n\n", thispc);
  printf(ANSI_FMT("If it is the first case, see\n%s\nfor more details.\n\n"
        "If it is the second case, remember:\n"
        "* The machine is always right!\n"
        "* Every line of untested code is always wrong!\n\n", ANSI_FG_RED), isa_logo);
  // 设置nemu的状态为异常终止
  set_nemu_state(NEMU_ABORT, thispc, -1);
}
