// AM 的中断/异常处理框架
#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>
#include <arch/riscv.h>
static Context* (*user_handler)(Event, Context*) = NULL;
// 在发生中断或异常时，保存 CPU 上下文，并根据中断原因（mcause）分发事件，最终恢复执行
Context* __am_irq_handle(Context *c) {
  if (user_handler) {
    Event ev = {0};
    switch (c->mcause) {
      case 11:
      case -1:
        ev.event = EVENT_YIELD;
        c->mepc += 4;
        break;
      default: ev.event = EVENT_ERROR; break;
    }

    c = user_handler(ev, c);
    assert(c != NULL);
    c->mstatus = 0x1800;
  }

  return c;
}

extern void __am_asm_trap(void);

bool cte_init(Context*(*handler)(Event, Context*)) {
  // initialize exception entry
  asm volatile("csrw mtvec, %0" : : "r"(__am_asm_trap));

  // register event handler
  user_handler = handler;

  return true;
}

Context *kcontext(Area kstack, void (*entry)(void *), void *arg) {
  Context *ctx = (Context *)((uint8_t *)kstack.end - sizeof( Context));
  memset(ctx ,0, sizeof(Context));
  ctx ->mepc = (uintptr_t)entry;
  ctx -> gpr[2] = (uintptr_t)kstack.end; // sp
  ctx -> gpr[10] = (uintptr_t)arg;
  return (Context *)ctx;
}
// 触发一个软件中断/执行 ecall 让调度逻辑运行 与 trap.S 配合：
//trap.S 只管“保存+跳转”，cte.c 完成“分类+调度+返回上下文”。
void yield() {
#ifdef __riscv_e
  asm volatile("li a5, 11; ecall");
#else
  asm volatile("li a7, -1; ecall");
#endif
}

bool ienabled() {
  return false;
}

void iset(bool enable) {
}
