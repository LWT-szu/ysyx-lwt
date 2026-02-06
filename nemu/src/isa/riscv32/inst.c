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
以 addi 指令为例：一条指令的生命周期
  1.匹配：INSTPAT 将字符串转换为掩码。如果 s->isa.inst 符合该模式。
  2.准备环境：进入 INSTPAT_MATCH。
  3.解码：调用 decode_operand，传入类型 TYPE_I。
    从指令中解析 rd, rs1。
    执行 src1R() 读取 rs1 的值到 src1。
    执行 immI() 提取立即数到 imm。
  4.执行：执行宏的最后一个参数代码：R(rd) = src1 + imm;。
  5.结束：goto 跳出，完成解码。
***************************************************************************************/

#include "local-include/reg.h"
#include <cpu/cpu.h>
#include <cpu/ifetch.h>
#include <cpu/decode.h>
#include <limits.h> // for INT32_MIN
#include "ftrace.h"

#define R(i) gpr(i)
#define Mr vaddr_read
#define Mw vaddr_write
extern word_t isa_raise_intr(word_t NO, vaddr_t epc);
extern word_t isa_reg_str2val(const char *s, bool *success);
enum
{
  TYPE_I,
  TYPE_SHAMTI,
  TYPE_U,
  TYPE_S,
  TYPE_N,
  TYPE_J,
  TYPE_r, // none
  TYPE_B,
};


word_t *csr(word_t addr)
{
  switch(addr){
    case 0x300: return &(cpu.csr.mstatus);
    case 0x305: return &(cpu.csr.mtvec);
    case 0x341: return &(cpu.csr.mepc);
    case 0x342: return &(cpu.csr.mcause);
    default: panic("unsupported csr addr = 0x%x", addr);
  }
}
// 从寄存器堆（R数组）中读取指定寄存器的值，并记录到解码阶段的操作数变量中,供指令执行时使用
#define src1R() do { *src1 = R(rs1); } while (0)
#define src2R() do { *src2 = R(rs2); } while (0)

// 辅助宏从指令中抽取出立即数,BITS宏:位抽取,SEXT宏:符号扩展
#define immI() do { *imm = SEXT(BITS(i, 31, 20), 12); } while(0)
#define immU() do { *imm = SEXT(BITS(i, 31, 12), 20) << 12; } while(0)
#define immS() do { *imm = (SEXT(BITS(i, 31, 25), 7) << 5) | BITS(i, 11, 7); } while(0)//把符号扩展后的高 7 位左移 5 位
#define immJ() do { *imm =  SEXT((BITS(i,31 , 31) << 20| BITS(i, 19, 12) << 12 | BITS(i, 20, 20) << 11 | BITS(i,30,21) << 1),21);} while(0)
#define immB() do { *imm = SEXT( ( BITS(i, 31, 31)<< 12| BITS(i, 7, 7) << 11 | BITS(i,30,25)  << 5  | BITS(i, 11, 8) << 1 ) , 13) ; } while(0)
#define shamtI() do { *imm = BITS(i, 25, 20); } while(0)
#define CSR(imm) *csr(imm)
#define ECALL(dnpc) dnpc = (isa_raise_intr(11, s->pc));//32E -> a5

// 指令类型解析出寄存器和立即数
static void decode_operand(Decode *s, int *rd, word_t *src1, word_t *src2, word_t *imm, int type) {
  uint32_t i = s->isa.inst;
  // 解码
  int rs1 = BITS(i, 19, 15); 
  int rs2 = BITS(i, 24, 20);
  *rd     = BITS(i, 11, 7);//对目标操作数进行寄存器操作数的译码
  switch (type) {
    case TYPE_I: src1R();          immI(); break;
    case TYPE_U:                   immU(); break;
    case TYPE_S: src1R(); src2R(); immS(); break;
    case TYPE_J:                   immJ(); break;
    case TYPE_r: src1R(); src2R();         break;
    case TYPE_B: src1R(); src2R(); immB(); break;
    case TYPE_SHAMTI: src1R();    shamtI(); break;
    case TYPE_N: break;
    default: panic("unsupported type = %d", type);
  }
}
// 指令解码和执行
static int decode_exec(Decode *s) {
  s->dnpc = s->snpc;
  //保证默认顺序执行，后续遇到需要跳转的指令再覆盖 dnpc

#define INSTPAT_INST(s) ((s)->isa.inst)
/*这个宏就像一个微型的“函数环境”。当指令匹配成功时，它自动声明变量 rd, src1, src2, imm，
并自动调用 decode_operand 把值填好，最后执行你写在 INSTPAT 最后面的具体逻辑。*/
//... 会匹配从该位置开始的所有剩余参数
/*该宏有四个参数：
s - CPU状态，name - 指令名称，type - 指令类型
... - 可变参数，匹配所有剩余的参数
则__VA_ARGS__对应reg[rd] = src1 + src2（举例）*/
#define INSTPAT_MATCH(s, name, type, ... /* execute body */ ) { \
  int rd = 0; \
  word_t src1 = 0, src2 = 0, imm = 0; \
  decode_operand(s, &rd, &src1, &src2, &imm, concat(TYPE_, type)); \
  __VA_ARGS__ ; \
}
  // 两个宏用来包裹一组指令解码模板,通过标签地址跳转机制,
  //只要有一个指令模式匹配成功，就立即跳出解码流程，不会去尝试后面的模板
  INSTPAT_START();
  // INSTPAT(模式字符串, 指令名称, 指令类型, 指令执行操作)
  INSTPAT("??????? ????? ????? ??? ????? 00101 11", auipc  , U, R(rd) = s->pc + imm);// ? 表示任意位，用 0/1 表示必须匹配的位
  INSTPAT("??????? ????? ????? ??? ????? 01101 11", lui    , U, R(rd) = imm);

  INSTPAT("??????? ????? ????? ??? ????? 11011 11", jal    , J, R(rd) = s->pc + 4,s->dnpc = s->pc + imm);   // jal

  INSTPAT("??????? ????? ????? 000 ????? 00100 11", addi   , I, R(rd) = src1 + imm);//addi
  INSTPAT("??????? ????? ????? 000 ????? 11001 11", jalr   , I, R(rd) = s->pc + 4,s->dnpc = (src1 + imm) & ~1); // jalr,最低位清零,跳转目标必须是偶数
  INSTPAT("??????? ????? ????? 100 ????? 00000 11", lbu    , I, R(rd) = Mr(src1 + imm, 1) & 0xFF);//保证高位清零
  INSTPAT("??????? ????? ????? 000 ????? 00000 11", lb     , I, R(rd) = SEXT( Mr(src1 + imm, 1),8));    // 高位做符号扩展
  INSTPAT("??????? ????? ????? 010 ????? 00000 11", lw     , I, R(rd) = Mr(src1 + imm, 4));
  INSTPAT("??????? ????? ????? 011 ????? 00100 11", sltiu  , I, R(rd) = ((uint32_t)src1 < (uint32_t)imm) ? 1 : 0);//无符号比较
  INSTPAT("??????? ????? ????? 010 ????? 00100 11", slti   , I, R(rd) = ( (int32_t)src1 < (int32_t)imm) ? 1 : 0);  // 有符号比较
  INSTPAT("0100000 ????? ????? 101 ????? 00100 11", srai   , I, if( (imm & 0x20 ) == 0) R(rd) =  (int32_t)src1 >> imm);//仅当 shamt[5]=0 时合法    0??????????
  INSTPAT("??????? ????? ????? 111 ????? 00100 11", andi   , I, R(rd) =  src1 & imm); //andi
  INSTPAT("??????? ????? ????? 100 ????? 00100 11", xori   , I, R(rd) =  src1 ^ imm); 
  INSTPAT("0000000 ????? ????? 101 ????? 00100 11", srli   , I, if( (imm & 0x20 ) == 0) R(rd) =  (uint32_t)src1 >> (imm & 0x1F)); //仅当 shamt[5]=0 时合法
  INSTPAT("000000? ????? ????? 001 ????? 00100 11", slli   , I, if ((imm & 0x20) == 0) R(rd) = (uint32_t)src1 << (imm & 0x1F));      // 仅当 shamt[5]=0 时合法,该过，你试试跑一下bit
  INSTPAT("??????? ????? ????? 001 ????? 00000 11", lh     , I, R(rd) = SEXT(Mr(src1 + imm, 2),16) );
  INSTPAT("??????? ????? ????? 101 ????? 00000 11", lhu    , I, R(rd) = Mr(src1 + imm, 2) & 0xFFFF);//无符号扩展16位
  INSTPAT("??????? ????? ????? 110 ????? 00100 11", ori    , I, R(rd) = src1 | imm);

  INSTPAT("??????? ????? ????? 010 ????? 11100 11", csrrs  , I,  R(rd) = CSR(imm) ; if(src1 != 0) CSR(imm) = CSR(imm) | src1 ); // 读写 CSR 寄存器
  INSTPAT("??????? ????? ????? 001 ????? 11100 11", csrrw  , I,  R(rd) = CSR(imm) ; CSR(imm) = src1);            // 读写 CSR 寄存器

  INSTPAT("??????? ????? ????? 000 ????? 11000 11", beq    , B, if(src1 == src2) s->dnpc = s->pc + imm); // beq条件判断跳转
  INSTPAT("??????? ????? ????? 001 ????? 11000 11", bne    , B, if (src1 != src2) s->dnpc = s->pc + imm); // bne
  INSTPAT("??????? ????? ????? 111 ????? 11000 11", bgeu   , B, if ((uint32_t)src1 >= (uint32_t)src2) s->dnpc = s->pc + imm);//无符号比较
  INSTPAT("??????? ????? ????? 101 ????? 11000 11", bge    , B, if((int32_t)src1 >= (int32_t)src2) s->dnpc = s->pc + imm);//补码比较(有符号整数类型)
  INSTPAT("??????? ????? ????? 101 ????? 11000 11", blez   , B, if (0 >= (int32_t)src2) s->dnpc = s->pc + imm);//伪指令
  INSTPAT("??????? ????? ????? 100 ????? 11000 11", blt    , B, if((int32_t)src1 < (int32_t)src2) s->dnpc = s->pc + imm);//补码比较(有符号整数类型)
  INSTPAT("??????? ????? ????? 110 ????? 11000 11", bltu   , B, if((uint32_t)src1 < (uint32_t)src2) s->dnpc = s->pc + imm);


  INSTPAT("0000000 ????? ????? 000 ????? 01100 11", add    , r, R(rd) = src1 + src2);
  INSTPAT("0100000 ????? ????? 000 ????? 01100 11", sub    , r, R(rd) = src1 - src2);
  INSTPAT("0000000 ????? ????? 001 ????? 01100 11", sll    , r, R(rd) = (uint32_t)src1 << (src2& 0x1F));//空位补零     ??????
  INSTPAT("0000000 ????? ????? 111 ????? 01100 11", and    , r, R(rd) = src1 & src2);
  INSTPAT("0000000 ????? ????? 011 ????? 01100 11", sltu   , r, R(rd) = ((uint32_t)src1 < (uint32_t)src2) ? 1 : 0);//sltu,snez
  INSTPAT("0000000 ????? ????? 110 ????? 01100 11", or     , r, R(rd) = src1 | src2);
  
  INSTPAT("0000001 ????? ????? 000 ????? 01100 11", mul    , r, R(rd) = (int32_t)src1 * (int32_t)src2);//M

  INSTPAT("0000001 ????? ????? 100 ????? 01100 11", Div    , r, R(rd) =(int32_t)src1 / (int32_t)src2;); // M

  INSTPAT("0000000 ????? ????? 100 ????? 01100 11", xor    , r, R(rd) = src1 ^ src2);

  INSTPAT("0000001 ????? ????? 110 ????? 01100 11", rem    , r, R(rd) = (int32_t)src1 % (int32_t)src2;);

  INSTPAT("0000000 ????? ????? 010 ????? 01100 11", slt    , r, R(rd) = ((int32_t)src1 < (int32_t)src2) ? 1 : 0 );//补码比较(有符号整数类型)
  INSTPAT("0000001 ????? ????? 001 ????? 01100 11", mulh   , r, R(rd) = ((int64_t)(int32_t) src1 * (int64_t)(int32_t) src2) >> 32);//M  有符号
  INSTPAT("0000001 ????? ????? 011 ????? 01100 11", mulhu  , r, R(rd) = ((uint64_t)(uint32_t)src1 * (uint64_t)(uint32_t)src2) >> 32);//无符号

  INSTPAT("0000001 ????? ????? 111 ????? 01100 11", remu   , r, R(rd) = (uint32_t)src1 % (uint32_t)src2);//M
  INSTPAT("0000001 ????? ????? 101 ????? 01100 11", Divu   , r, R(rd) = (uint32_t)src1 / (uint32_t)src2);// M

  INSTPAT("0100000 ????? ????? 101 ????? 01100 11", sra    , r, R(rd) = (int32_t)src1 >> ((int32_t)src2 & 0x1F));//用原数最高位（符号位）填充,就是补码的算术右移,未考虑第五位为位移数
  INSTPAT("0000000 ????? ????? 101 ????? 01100 11", srl    , r, R(rd) = (uint32_t)src1 >> ((int32_t)src2 & 0x1F));//空位补零,未考虑第五位为位移数

  INSTPAT("??????? ????? ????? 010 ????? 01000 11", sw     , S, Mw(src1 + imm, 4, src2));
  INSTPAT("??????? ????? ????? 000 ????? 01000 11", sb     , S, Mw(src1 + imm, 1, src2));
  INSTPAT("??????? ????? ????? 001 ????? 01000 11", sh     , S, Mw(src1 + imm, 2, src2));


  //add more instruction in here(maybe)
  //ecall,csrrw,csrrs,mret
  INSTPAT("0011000 00010 00000 000 00000 11100 11", mret   , r, { cpu.csr.mstatus = (cpu.csr.mstatus & ~0x8) | ((cpu.csr.mstatus & 0x80) >> 4); 
                                                                  s->dnpc = cpu.csr.mepc; 
                                                                  #ifdef CONFIG_ETRACE
                                                                  printf("\n[Etrace] Mret  ->  pc = 0x%x\n", s->dnpc);
                                                                  #endif
                                                                });//将 mstatus 的 MIE 位设置为 MPIE 的值，然后将 MPIE 清零，最后跳转到 mepc 指定的地址继续执行程序
  INSTPAT("0000000 00000 00000 000 00000 11100 11", ecall  , I,  
                                                                #ifdef CONFIG_ETRACE
                                                                printf("\n[Etrace] Ecall pc = 0x%x  a7 = %d \n", s->pc, R(17)); 
                                                                #endif
                                                                ECALL(s->dnpc)); // ecall 是 “系统调用专用指令”，核心解决 “用户态无法访问内核资源” 的问题；
  INSTPAT("0000000 00001 00000 000 00000 11100 11", ebreak, N, NEMUTRAP(s->pc, R(10))); // R(10) is $a0  ebreak 是 “调试断点专用指令”，核心解决 “程序调试时暂停执行” 的问题。
  // 二者均触发同步异常，但通过异常原因码、软件约定和硬件设计明确区分
  INSTPAT("??????? ????? ????? ??? ????? ????? ??", inv    , N, INV(s->pc));//inv前面所有的模式匹配规则都无法成功匹配, 则将该指令视为非法指令
  INSTPAT_END();

  R(0) = 0; // reset $zero to 0

  return 0;
}

int isa_exec_once(Decode *s) {
  s->isa.inst = inst_fetch(&s->snpc, 4);//s->snpc每次加4

  // 1. 关键修改：先执行解码和指令执行
  // 这样做的原因是计算 s->dnpc (下一条指令地址/跳转目标) 的逻辑是在 decode_exec 中完成的
  int ret_decode = decode_exec(s);

  // 2. 执行完后再进行 Trace，此时 s->dnpc 才是有效的跳转目标地址
  #ifdef CONFIG_FTRACE
  uint32_t inst = s->isa.inst;
  if((inst & 0x7F) == 0x6F){
    // jal 指令
    // 此时 s->dnpc 只有在 decode_exec 执行后才会被更新为 (pc + imm)
    printf(FMT_WORD ":%*scall [%s@0x%08x]\n", s->pc, ftrace_depth * 2, "", ftrace_funcname(s->dnpc), s->dnpc);
    ftrace_depth++;
  }
  else if ((inst & 0x7F) == 0x67)
  {
    // jalr 指令
    int rd = (inst >> 7) & 0x1f;
    int rs1 = (inst >> 15) & 0x1f;
    int imm = (int32_t)inst >> 20;
    if (rd == 0 && rs1 == 1 && imm == 0)
    {
      // ret 指令 (jalr x0, x1, 0)
      ftrace_depth--;
      if (ftrace_depth < 0) ftrace_depth = 0;
      printf(FMT_WORD ":%*sret  [%s]\n", s->pc, ftrace_depth * 2, "", ftrace_funcname(s->pc));
    }
    else
    {
      // 普通的 jalr 函数调用
      // 此时 s->dnpc 已经被更新为 (src1 + imm) & ~1
      printf(FMT_WORD ":%*scall [%s@0x%08x]\n", s->dnpc, ftrace_depth * 2, "", ftrace_funcname(s->dnpc), s->dnpc);
      ftrace_depth++;
    }
  }
#endif

  // 3. 返回 decode_exec 的结果
  return ret_decode;
}
