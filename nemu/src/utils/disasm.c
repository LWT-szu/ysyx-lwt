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
#include <capstone/capstone.h>
#include <common.h>

static size_t (*cs_disasm_dl)(csh handle, const uint8_t *code,
    size_t code_size, uint64_t address, size_t count, cs_insn **insn);
static void (*cs_free_dl)(cs_insn *insn, size_t count);

static csh handle;
// 初始化 capstone，设置好句柄、反汇编模式等,程序启动时只需调用一次
void init_disasm() {
  void *dl_handle;
  dl_handle = dlopen("tools/capstone/repo/libcapstone.so.5", RTLD_LAZY);
  assert(dl_handle);

  cs_err (*cs_open_dl)(cs_arch arch, cs_mode mode, csh *handle) = NULL;
  cs_open_dl = dlsym(dl_handle, "cs_open");
  assert(cs_open_dl);

  cs_disasm_dl = dlsym(dl_handle, "cs_disasm");
  assert(cs_disasm_dl);

  cs_free_dl = dlsym(dl_handle, "cs_free");
  assert(cs_free_dl);
  // 宏判断当前 NEMU 架构,选择正确的 capstone 配置
  cs_arch arch = MUXDEF(CONFIG_ISA_x86,      CS_ARCH_X86,
                   MUXDEF(CONFIG_ISA_mips32, CS_ARCH_MIPS,
                   MUXDEF(CONFIG_ISA_riscv,  CS_ARCH_RISCV,
                   MUXDEF(CONFIG_ISA_loongarch32r,  CS_ARCH_LOONGARCH, -1))));
  cs_mode mode = MUXDEF(CONFIG_ISA_x86,      CS_MODE_32,
                   MUXDEF(CONFIG_ISA_mips32, CS_MODE_MIPS32,
                   MUXDEF(CONFIG_ISA_riscv,  MUXDEF(CONFIG_ISA64, CS_MODE_RISCV64, CS_MODE_RISCV32) | CS_MODE_RISCVC,
                   MUXDEF(CONFIG_ISA_loongarch32r,  CS_MODE_LOONGARCH32, -1))));
	int ret = cs_open_dl(arch, mode, &handle);
  assert(ret == CS_ERR_OK);

#ifdef CONFIG_ISA_x86
  cs_err (*cs_option_dl)(csh handle, cs_opt_type type, size_t value) = NULL;
  cs_option_dl = dlsym(dl_handle, "cs_option");
  assert(cs_option_dl);

  ret = cs_option_dl(handle, CS_OPT_SYNTAX, CS_OPT_SYNTAX_ATT);
  assert(ret == CS_ERR_OK);
#endif
}
// 反汇编函数,输入：目标字符串、最大长度、PC、指令二进制码、字节数
void disassemble(char *str, int size, uint64_t pc, uint8_t *code, int nbyte) {
  // 声明 capstone 反汇编结构体指针 cs_insn *insn;
  // 调用 capstone 的反汇编函数，把机器码转成汇编字符串
  // 参数说明：
  // - handle: capstone 句柄
  // - code: 指令的二进制数据,注意：指令需要以字节流形式传入（uint8_t*），长度4
  // - nbyte: 指令长度（字节数，RISC-V 通常是 4）
  // - pc: 指令的地址
  // - 0: count=0，表示不限条数，但这里每次只反汇编一条
  // - &insn: 输出的反汇编结果（capstone 的 insn 结构体数组）
  cs_insn *insn;
  size_t count = cs_disasm_dl(handle, code, nbyte, pc, 0, &insn); // 解码
  // 断言反汇编结果必须正好为 1 条
  assert(count == 1);
  // 先把反汇编出来的操作码（如 "add", "lw"）写到 str
  int ret = snprintf(str, size, "%s", insn->mnemonic);

  // 如果该指令有操作数（即 op_str 不为空），继续拼接到 str 后面
  if (insn->op_str[0] != '\0') {
    // 拼接 "\t操作数" 到 str 后面
    snprintf(str + ret, size - ret, "\t%s", insn->op_str);
  }
  // 释放 capstone 分配的 insn 内存
  cs_free_dl(insn, count);
}
