#define concat_temp(x, y) x ## y
#define concat(x, y) concat_temp(x, y)
#define MAP(c, f) c(f)

#if __riscv_xlen == 32
#define LOAD  lw
#define STORE sw
#define XLEN  4
#else
#define LOAD  ld
#define STORE sd
#define XLEN  8
#endif

#这里为什么没有f(0)和f(2)???????他就不应该有，加了会报错
#define REGS_LO16(f) \
      f( 1)       f( 3) f( 4) f( 5) f( 6) f( 7) f( 8) f( 9) \
f(10) f(11) f(12) f(13) f(14) f(15)
#ifndef __riscv_e
#define REGS_HI16(f) \
                                    f(16) f(17) f(18) f(19) \
f(20) f(21) f(22) f(23) f(24) f(25) f(26) f(27) f(28) f(29) \
f(30) f(31)
#define NR_REGS 32
#else
#define REGS_HI16(f)
#define NR_REGS 16
#endif

#define REGS(f) REGS_LO16(f) REGS_HI16(f)

#define PUSH(n) STORE concat(x, n), (n * XLEN)(sp);
#define POP(n)  LOAD  concat(x, n), (n * XLEN)(sp);

#CONTEXT_SIZE 表示一次 trap 需要在栈上保存分配的“完整上下文”大小
#它是 32 个通用寄存器 + mcause + mstatus + mepc
#在栈上占用的总字节数。

#NR_REGS= 32
#define CONTEXT_SIZE  ((NR_REGS + 3) * XLEN)
#define OFFSET_SP     ( 2 * XLEN)
#define OFFSET_CAUSE  ((NR_REGS + 0) * XLEN)
#define OFFSET_STATUS ((NR_REGS + 1) * XLEN)
#define OFFSET_EPC    ((NR_REGS + 2) * XLEN)

.align 3
#让 __am_asm_trap 的地址按 2^3 = 8 字节对齐

.globl __am_asm_trap
#把这个符号导出给链接器,让它对 C 可见

__am_asm_trap:
  addi sp, sp, -CONTEXT_SIZE
#“压栈”就是把当前CPU的寄存器等关键信息保存到栈上
#把 sp 往“低地址”移动 CONTEXT_SIZE 字节
#即一次性在栈上为异常处理“开辟” CONTEXT_SIZE(280字节) 大小的空间

#__am_asm_trap 是一个地址,在汇编文件中定义的异常入口标签，
#通过 .globl 导出，供 C 代码写入 mtvec 作为 trap 的入口地址

  MAP(REGS, PUSH)
#宏把所有通用寄存器（除了x0和sp）依次保存到刚才分配的栈空间里
#sw x1, 4(sp)  sw x3, 12(sp) ...
  csrr t0, mcause
  csrr t1, mstatus
  csrr t2, mepc

  STORE t0, OFFSET_CAUSE(sp)
  STORE t1, OFFSET_STATUS(sp)
  STORE t2, OFFSET_EPC(sp)
  #把异常号、机器状态、异常PC等特殊寄存器也保存到内存中的栈上，跟ecall不一样，ecall是保存到硬件寄存器
  #在新开的栈空间，保存异常发生时的 CPU 上下文（寄存器现场）
  #trap 汇编保存完现场后，把“现场所在地址”用mv交给后文的 __am_irq_handle

  # set mstatus.MPRV to pass difftest
  li a0, (1 << 17)
  or t1, t1, a0
  csrw mstatus, t1

# 在 RISC-V 调用约定里：函数的第 1 个参数放在 a0（x10）
  mv a0, sp
# 则把 sp 的值，作为第一个参数，传给 __am_irq_handle(sp) 函数
  call __am_irq_handle
  mv sp, a0 

  LOAD t1, OFFSET_STATUS(sp)
  LOAD t2, OFFSET_EPC(sp)
  csrw mstatus, t1
  csrw mepc, t2

  MAP(REGS, POP)#通用寄存器赋值,宏展开

  addi sp, sp, CONTEXT_SIZE
  #恢复（弹栈）,从栈上恢复到寄存器
  mret
  #执行 mret 返回异常前的代码处

#由于 RISC-V 栈向低地址增长，因此通过减小 sp 来完成栈帧建立。
#  高地址
#│
#│   ← 旧 sp
#│
#│   （栈空间）
#│
#│   ← 新 sp
#│
#  低地址

#sp 指向的不是“普通栈”，而是一个完整保存了寄存器现场的 Context 结构
#sp ──▶ Context {
#         gpr[]
#         mcause
#         mstatus
#         mepc
#       }

