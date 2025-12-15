/*这个文件建立了“trap/异常分发”的底层框架
  发生 trap/异常时，CPU会保存context，进入 __am_asm_trap(汇编) -> __am_irq_handle(C) -> 用户自定义 handler。
  cte_init 完成注册和 trap 向量表的初始化
  yield 可以用来主动抛出异常/让出CPU
  Context/Event 是“异常响应机制”流程的核心数据结构
  Event：事件类型
  Context*：当前上下文指针
  */

#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>
#include <arch/riscv.h>
// 定义了一个函数指针 user_handler ，就是后面注册的“用户自定义异常响应函数”,用于回调用户自己的异常处理函数
// Context 表示“CPU上下文结构体”，Event 表示异常/中断原因
// “句柄”本质上就是“指针”或者“索引”，是用来间接引用某个对象或资源的东西
static Context* (*user_handler)(Event, Context*) = NULL;

// 底层异常分发入口，处理 Context 和 Event
// 每当发生异常/中断后，AM汇编层trap跳转到的C函数，负责“到底发生了什么”，决定调用哪个 handler。
Context* __am_irq_handle(Context *c) {
  //printf("__am_irq_handle called, mcause = %lu\n", c->mcause);
  if (user_handler) {
    Event ev = {0};
    //printf("AM:mcause = %u\n", c->mcause);
    switch (c->mcause) {
      
      case-1: ev.event = EVENT_YIELD; c->mepc += 4;break;
      default: ev.event = EVENT_ERROR; break;
    }
    //printf("irq event: %d\n", ev.event);
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

// 初始化异常入口和注册 handler , Context*(*handler)(Event, Context*) -> ctx
// 硬件异常你无法预测何时发生，所以必须用回调！
bool cte_init(Context*(*handler)(Event, Context*)) {
  // initialize exception entry
  // 设置机器异常入口 mtvec ，让 trap 时跳转到 __am_asm_trap
  asm volatile("csrw mtvec, %0" : : "r"(__am_asm_trap));
  //mtvec 是 RISC-V 架构中的“机器模式异常向量寄存器”
  //它存储了当 nemu 进入机器模式时异常处理程序的地址

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

// 内核态上下文创建
// 其中kstack是栈的范围, entry是内核线程的入口, arg则是内核线程的参数.
// 此外, kcontext() 要求内核线程不能从entry返回, 否则其行为是未定义的.
// 你需要在kstack的底部创建一个以entry为入口的上下文结构(目前你可以先忽略arg参数), PA 4.1
// 然后返回这一结构的指针 
Context *kcontext(Area kstack, void (*entry)(void *), void *arg)
{
  uintptr_t stack_end = (uintptr_t )kstack.end;
  Context *ctx = (Context *)(stack_end - sizeof(Context));
  memset(ctx, 0, sizeof(Context));

  ctx->mepc = (uintptr_t)entry;
  ctx->gpr[2] = stack_end;       // sp (x2)
  ctx->gpr[10] = (uintptr_t)arg; // a0 (x10) 传递参数

  return ctx;
}

// 主动触发异常切换（软件模拟“让出CPU”）
void yield()
{
  //ev.event = EVENT_YIELD;
  // printf("yield called %d\n", ev.event);
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