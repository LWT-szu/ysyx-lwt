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
#include "local-include/reg.h"
#include <common.h>
const char *regs[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

void isa_reg_display(char *args) {
  int i=0;
  printf("pc = 0x%08x \n", cpu.pc);
  for(i=0;i<16;i++)
  {
    printf("%s = 0x%08x\n", reg_name(i), (uint32_t)gpr(i)); // 打印8位十六进制
  }
  
}
//实现字符串到寄存器编号的映射，并返回对应的值。
word_t isa_reg_str2val(const char *s, bool *success) {
  int i;
  // 依次遍历regs数组，查找和s完全相同的名字
  
  if (strcmp(s, "pc") == 0)
  {
    *success = true;
    return cpu.pc;
  }
  
  for (i = 0; i < 32; i++)
  {
    if(strcmp(s,regs[i])== 0) {  // 逐个和regs数组对比，完全匹配才返回
      if(success) *success = true;
      return gpr(i); // 返回第i号通用寄存器的值
    }
  }
  
  
  if (success) *success = false;
  return 0;
}
