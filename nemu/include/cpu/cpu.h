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
 *这个头文件定义了 NEMU 模拟器中控制 CPU 运行、管理执行状态的核心接口，是连接调试命令（如 c、si）与实际指令执行逻辑的桥梁。
 例如，当你输入 si 命令时，cmd_si 函数会调用 cpu_exec(1)，而 cpu_exec 内部会执行一条指令，
 然后通过 set_nemu_state 控制模拟器进入暂停状态，等待下一条命令。
 ***************************************************************************************/

#ifndef __CPU_CPU_H__
#define __CPU_CPU_H__

#include <common.h>

void cpu_exec(uint64_t n); // src/cpu/difftest/ref.c

void set_nemu_state(int state, vaddr_t pc, int halt_ret);
void invalid_inst(vaddr_t thispc);//nemu/src/engine/interpreter/hostcall.c

#define NEMUTRAP(thispc, code) set_nemu_state(NEMU_END, thispc, code)
#define INV(thispc) invalid_inst(thispc) // nemu/src/isa/riscv32/inst.c

#endif
