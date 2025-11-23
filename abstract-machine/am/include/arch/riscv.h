#ifndef ARCH_H__
#define ARCH_H__

#ifdef __riscv_e
#define NR_REGS 16
#else
#define NR_REGS 32
#endif

/*就是你 C 语言中记录trap现场的一个“快照”。
  必须保证 顺序/布局/意义 与 trap.s 里 STORE t0, OFFSET_CAUSE(sp) 等保存位置一致。
  比如你的 struct Context 现在写的是：mepc, mcause, gpr[], mstatus
  那么 trap.s 也必须第一项保存 mepc，然后是 mcause，然后按顺序是寄存器，
  然后是 mstatus，才能用 memcpy/sp指针互通。*/
struct Context {
  // TODO: fix the order of these members to match trap.S
  // 让Context结构体和trap.S保存/恢复顺序完全一致
  uintptr_t mepc, mcause, gpr[NR_REGS], mstatus;
  void *pdir;
};

#ifdef __riscv_e
#define GPR1 gpr[15] // a5
#else
#define GPR1 gpr[17] // a7
#endif

#define GPR2 gpr[0]
#define GPR3 gpr[0]
#define GPR4 gpr[0]
#define GPRx gpr[0]

#endif
