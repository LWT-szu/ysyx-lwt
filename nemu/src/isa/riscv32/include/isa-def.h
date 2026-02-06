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

#ifndef __ISA_RISCV_H__
#define __ISA_RISCV_H__

#include <common.h>
typedef struct{
  word_t mtvec, mepc, mstatus, mcause;
} RISCV_CSR_STATE;

typedef struct {
  word_t gpr[MUXDEF(CONFIG_RVE, 16, 32)];
  vaddr_t pc;                                                // 程序计数器
  RISCV_CSR_STATE csr;                                   // CSR寄存器状态
} MUXDEF(CONFIG_RV64, riscv64_CPU_state, riscv32_CPU_state); // 根据宏是否定义来选择架构,判断用 16 还是 32

// decode
typedef struct {
  uint32_t inst;
} MUXDEF(CONFIG_RV64, riscv64_ISADecodeInfo, riscv32_ISADecodeInfo);

/*检查当前系统状态下对内存区间为[vaddr, vaddr + len), 类型为type的访问是否需要经过地址转换. 其中type可能为:
MEM_TYPE_IFETCH: 取指令
MEM_TYPE_READ: 读数据
MEM_TYPE_WRITE: 写数据
函数返回值可能为:

MMU_DIRECT: 该内存访问可以在物理内存上直接进行
MMU_TRANSLATE: 该内存访问需要经过地址转换
MMU_FAIL: 该内存访问失败, 需要抛出异常(如RISC架构不支持非对齐的内存访问)*/
#define isa_mmu_check(vaddr, len, type) (MMU_DIRECT)

#endif
