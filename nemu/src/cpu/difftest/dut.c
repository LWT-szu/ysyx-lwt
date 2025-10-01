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
* 是 NEMU 的 DiffTest 框架的主控代码，实现了与参考模型(REF, 通常为 QEMU 或 Spike)的动态交互。
* 主要负责动态加载参考模型的 so 文件，初始化接口，拷贝内存和寄存器，步进执行，并和 DUT (NEMU自身) 做指令级、寄存器级对比。
* 支持特殊情况的跳过机制，如 QEMU 指令打包和无法比对的特殊指令。
* 是 NEMU 实现“差分测试”的桥梁和核心
***************************************************************************************/

#include <dlfcn.h> // 动态链接库操作头文件，支持运行时加载参考模型（Reference Model）的 so 文件

#include <isa.h>
#include <cpu/cpu.h>
#include <memory/paddr.h>
#include <utils.h>
#include <difftest-def.h>

// 下面这四个是指向参考模型(REF)的函数指针，后续通过 dlsym 加载
void (*ref_difftest_memcpy)(paddr_t addr, void *buf, size_t n, bool direction) = NULL; // REF的内存同步
void (*ref_difftest_regcpy)(void *dut, bool direction) = NULL; // REF的寄存器同步
void (*ref_difftest_exec)(uint64_t n) = NULL;        // REF的单步执行
void (*ref_difftest_raise_intr)(uint64_t NO) = NULL; // REF的异常注入

#ifdef CONFIG_DIFFTEST

static bool is_skip_ref = false; // 指示是否跳过 REF 的一次检查
static int skip_dut_nr_inst = 0; // 要跳过的 DUT 指令数

// this is used to let ref skip instructions which
// can not produce consistent behavior with NEMU
// 让 REF 跳过一次 DiffTest 检查（通常用于特殊指令、外设访问等）
void difftest_skip_ref() {
  is_skip_ref = true;
  // If such an instruction is one of the instruction packing in QEMU
  // (see below), we end the process of catching up with QEMU's pc to
  // keep the consistent behavior in our best.
  // Note that this is still not perfect: if the packed instructions
  // already write some memory, and the incoming instruction in NEMU
  // will load that memory, we will encounter false negative. But such
  // situation is infrequent.
  skip_dut_nr_inst = 0;// 取消追赶 QEMU PC 的过程
}

// this is used to deal with instruction packing in QEMU.
// Sometimes letting QEMU step once will execute multiple instructions.
// We should skip checking until NEMU's pc catches up with QEMU's pc.
// The semantic is
//   Let REF run `nr_ref` instructions first.
//   We expect that DUT will catch up with REF within `nr_dut` instructions.
// 当 QEMU 一次执行多条指令（instruction packing）时，
// 让 REF 先执行 nr_ref 条指令，DUT 最多跳 nr_dut 条指令来追赶 REF 的 PC
void difftest_skip_dut(int nr_ref, int nr_dut) {
  skip_dut_nr_inst += nr_dut;

  while (nr_ref -- > 0) {
    ref_difftest_exec(1);
  }
}
// 初始化 DiffTest，加载参考模型动态库，获取相关函数指针，初始化参考模型状态
// 打开传入的动态库文件ref_so_file
/*通过动态链接对动态库中的上述API符号进行符号解析和重定位, 返回它们的地址.
  对REF的DIffTest功能进行初始化, 具体行为因REF而异.
  将DUT的guest memory拷贝到REF中.
  将DUT的寄存器状态拷贝到REF中.
*/
void init_difftest(char *ref_so_file, long img_size, int port) {
  assert(ref_so_file != NULL);

  // 加载参考模型的动态库
  // 声明动态库句柄：用于存储 dlopen 返回值
  void *handle;
  handle = dlopen(ref_so_file, RTLD_LAZY);
  assert(handle);

  // 获取 REF 端的各项函数指针
  ref_difftest_memcpy = dlsym(handle, "difftest_memcpy");
  assert(ref_difftest_memcpy);

  ref_difftest_regcpy = dlsym(handle, "difftest_regcpy");
  assert(ref_difftest_regcpy);

  ref_difftest_exec = dlsym(handle, "difftest_exec");
  assert(ref_difftest_exec);

  ref_difftest_raise_intr = dlsym(handle, "difftest_raise_intr");
  assert(ref_difftest_raise_intr);

  // 调用 REF 的初始化函数
  void (*ref_difftest_init)(int) = dlsym(handle, "difftest_init");
  assert(ref_difftest_init);

  Log("Differential testing: %s", ANSI_FMT("ON", ANSI_FG_GREEN));
  Log("The result of every instruction will be compared with %s. "
      "This will help you a lot for debugging, but also significantly reduce the performance. "
      "If it is not necessary, you can turn it off in menuconfig.", ref_so_file);

  ref_difftest_init(port); // 初始化参考模型
  /*
  // 用于将 DUT 的镜像内容（通常是指令和数据）拷贝到 REF
  // RESET_VECTOR: 宿主物理内存起始地址
  // guest_to_host(RESET_VECTOR): DUT 侧镜像的起始虚拟地址
  // img_size: 镜像文件大小
  // DIFFTEST_TO_REF: 方向标志，表示把内存内容从 DUT 拷贝到 REF*/
  ref_difftest_memcpy(RESET_VECTOR, guest_to_host(RESET_VECTOR), img_size, DIFFTEST_TO_REF); // 拷贝镜像到 REF

  // &cpu: 指向 DUT 当前寄存器状态的指针
  // DIFFTEST_TO_REF: 方向标志，表示把寄存器内容从 DUT 拷贝到 REF
  ref_difftest_regcpy(&cpu, DIFFTEST_TO_REF);// 拷贝初始寄存器状态到 REF!!!!!!
}

// 检查参考模型和 DUT 的寄存器是否一致，如果不一致则触发异常
static void checkregs(CPU_state *ref, vaddr_t pc) {
  if (!isa_difftest_checkregs(ref, pc)) {
    nemu_state.state = NEMU_ABORT;
    nemu_state.halt_pc = pc;
    isa_reg_display();
  }
}

// 核心的 DiffTest 步进函数，每执行一条指令都调用一次
void difftest_step(vaddr_t pc, vaddr_t npc) {
  CPU_state ref_r; // REF 当前寄存器状态

  // 如果还需要跳过 DUT 的比对（比如 QEMU 指令打包时）
  if (skip_dut_nr_inst > 0) {
    ref_difftest_regcpy(&ref_r, DIFFTEST_TO_DUT);
    if (ref_r.pc == npc) {
      skip_dut_nr_inst = 0;
      checkregs(&ref_r, npc);
      return;
    }
    skip_dut_nr_inst --;
    if (skip_dut_nr_inst == 0)
      panic("can not catch up with ref.pc = " FMT_WORD " at pc = " FMT_WORD, ref_r.pc, pc);
    return;
  }
  // 处理需要跳过参考模型执行的情况
  if (is_skip_ref) {
    // to skip the checking of an instruction, just copy the reg state to reference design
    ref_difftest_regcpy(&cpu, DIFFTEST_TO_REF);
    is_skip_ref = false;
    return;
  }
  // 正常的差分测试流程：
  ref_difftest_exec(1); // 让参考模型执行一条指令
  ref_difftest_regcpy(&ref_r, DIFFTEST_TO_DUT); // 获取参考模型的寄存器状态

  checkregs(&ref_r, pc); // 检查寄存器是否一致
}
#else // 如果没有打开 DIFFTEST，提供空函数防止链接错误
void init_difftest(char *ref_so_file, long img_size, int port) { }
#endif
