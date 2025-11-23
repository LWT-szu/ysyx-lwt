/*这个文件建立了“trap/异常分发”的底层框架
  发生 trap/异常时，CPU会保存context，进入 __am_asm_trap(汇编) -> __am_irq_handle(C) -> 用户自定义 handler。
  cte_init 完成注册和 trap 向量表的初始化
  yield 可以用来主动抛出异常/让出CPU
  Context/Event 是“异常响应机制”流程的核心数据结构*/

#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>
// 定义了一个函数指针 user_handler ，就是后面注册的“用户自定义异常响应函数”,用于回调用户自己的异常处理函数
// Context 表示“CPU上下文结构体”，Event 表示异常/中断原因
static Context* (*user_handler)(Event, Context*) = NULL;

// 底层异常分发入口，处理 Context 和 Event
// 每当发生异常/中断后，AM汇编层trap跳转到的C函数，负责“到底发生了什么”，决定调用哪个 handler。
Context* __am_irq_handle(Context *c) {
  if (user_handler) {
    Event ev = {0};
    switch (c->mcause) {
      default: ev.event = EVENT_ERROR; break;
    }

    c = user_handler(ev, c);
    assert(c != NULL);
  }

  return c;
}

/*具体流程：
1.如果 user_handler 已经注册了，就准备一个 Event 类型变量 ev。
2.检查 c->mcause（mcause 是 RISC-V 架构中的“异常/中断原因寄存器”），
  根据不同mcause种类决定到底是什么“事件”（比如定时中断、系统调用等）。
  不过你现在的代码是默认都设成 EVENT_ERROR（占位，实际应该根据 mcause 填细分的事件）。
3.调用户handler函数 user_handler(ev, c);，让“用户态”决定怎么处理当前事件。
handler 也必须返回一个新的 Context（比如 context切换或者更新后）。
4.返回处理后的 Context 给 trap 返回。*/

extern void __am_asm_trap(void);

// 初始化异常入口和注册 handler
bool cte_init(Context*(*handler)(Event, Context*)) {
  // initialize exception entry
  // 设置机器异常入口 mtvec ，让 trap 时跳转到 __am_asm_trap
  asm volatile("csrw mtvec, %0" : : "r"(__am_asm_trap));

  // register event handler
  // 把用户传入的 handler 函数指针保存到全局变量 user_handler
  // user_handler 是你上层 nanos-lite/nemu 传进来的
  user_handler = handler;

  return true;
}

/*作用：初始化异常和中断响应机制，注册你的自定义handler
  mtvec 是 RISC-V 的 trap/exception 向量表入口，
  只要发生异常/中断就会跳到 __am_asm_trap 这个汇编入口。
  handler 注册给 user_handler，后续发生异常就用这个 callback。*/

// 内核态上下文创建（PA 里可以先不关注）
// 用于构建内核线程/协程上下文（context），这里只留了个空壳
// 实际上 nanos-lite 里系统调用/上下文切换会用到
Context *kcontext(Area kstack, void (*entry)(void *), void *arg) {
  return NULL;
}

// 主动触发异常切换（软件模拟“让出CPU”）
void yield() {
#ifdef __riscv_e
  asm volatile("li a5, -1; ecall");
#else
  asm volatile("li a7, -1; ecall");
#endif
}

/*用于主动请求切换（比如多核协程里让自己停下来，切到别人）。
  本质是执行一条 ecall 指令并设置特定寄存器（a5或a7为-1），
  表示“我想让出CPU”，立即抛出软中断/异常*/

//与中断开关有关的接口
bool ienabled() {
  return false;
}

void iset(bool enable) {
}

/*ienabled 通常是“返回当前是否允许中断”
  iset(bool) 通常是“设置当前是否允许中断”*/