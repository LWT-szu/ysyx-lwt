/*这个文件建立了“trap/异常分发”的底层框架
  发生 trap/异常时，CPU会保存context，
  进入 __am_asm_trap(汇编) -> __am_irq_handle(C) -> 用户自定义 handler。
  cte_init 完成注册和 trap 向量表的初始化
  yield 可以用来主动抛出异常/让出CPU
  handler include Context/Event 是“异常响应机制”流程的核心数据结构
  Event：事件类型
  Context*：当前上下文指针
  具体流程：
    1. 发生 trap/异常
    2. 跳转到 __am_asm_trap 汇编入口
    3. 汇编保存现场，调用 __am_irq_handle 传入 Context 指针
    4. __am_irq_handle 根据 mcause 构造 Event 结构体
    5. 调用用户注册的 handler 函数，传入 Event 和 Context 指针
    6. handler 处理事件，返回新的 Context 指针
    7. __am_irq_handle 返回新的 Context 指针给汇编
    8. 汇编恢复现场，返回到用户程序继续执行
  注册 handler 的流程：
    你写函数 handler
            ↓
    cte_init(handler)      ← 注册
            ↓
    user_handler = handler ← 存地址
            ↓
    trap 发生
            ↓
    user_handler(ev, c)       ← 回调
    普通调用：你决定什么时候调用
    回调调用：执行流决定什么时候调用
  */

#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>
#include <arch/riscv.h>
// 定义了一个函数指针 user_handler ，就是后面注册的“用户自定义异常响应函数”,用于回调异常处理函数
// Context 表示“CPU上下文结构体”，Event 表示异常/中断原因
// “句柄”本质上就是“指针”或者“索引”，是用来间接引用某个对象或资源的东西

// 用户异常处理"函数指针",只是一个变量，用来存放某个函数的地址
static Context *(*user_handler)(Event, Context *) = NULL;

// 底层异常分发入口，处理 Context 和 Event
// 每当发生异常/中断后，AM汇编层trap跳转到的C函数，负责“到底发生了什么”，决定调用哪个 handler。
// c 是“当前异常发生时 CPU 状态的描述指针sp”,只包含上下文，不包含事件
Context* __am_irq_handle(Context *c) {
  //printf("__am_irq_handle called, mcause = %lu\n", c->mcause);
  if (user_handler) {//如果 user_handler 已经注册了
    Event ev = {0};
    // 分配事件
    switch (c->mcause)
    {
      //case 8:// 软中断 ecall from U-mode
      //case 9:// 软中断 ecall from S-mode
      //case 10:// 软中断 ecall from M-mode
      case 11:
      case-1: ev.event = EVENT_YIELD; c->mepc += 4;break;

    default:
      ev.event = EVENT_ERROR;
      break;
    }
    c = user_handler(ev, c); // 回调函数simple_trap
    // 如果回调函数返回的上下文被更改了，具体看回调函数的实现，
    // 说明上下文被切换，在后面a0的返回值会改变，则栈顶sp也需要跟着改变，
    // 实现上下文的切换.这里simple_trap没有更改上下文，故没有切换上下文
    assert(c != NULL);
    // ===========================================
    c->mstatus = 0x1800;// 0x1800 是 MIE 和 MPIE 位，表示进入 M-mode 后允许中断
  }

  return c; // 返回给a0去更新sp(a0为返回值寄存器)
}

/*具体流程：
1.如果 user_handler 已经注册了，就准备一个 Event 类型变量 ev。
2.检查 c->mcause（mcause 是 RISC-V 架构中的“异常/中断原因寄存器”），
  根据不同mcause种类决定到底是什么“事件”（比如定时中断、系统调用等）。
  不过你现在的代码是默认都设成 EVENT_ERROR（占位，实际应该根据 mcause 填细分的事件）。
3.调用户handler函数 user_handler(ev, c);，让“用户态”决定怎么处理当前事件。
handler 也必须返回一个新的 Context（比如 context切换或者更新后）。
4.返回处理后的 Context 给 trap 返回。*/

// __am_asm_trap是一个地址,extern意思是在trap.s中找到这个符号
extern void __am_asm_trap(void); 

// 初始化异常入口和注册 handler , Context*(*handler)(Event, Context*) -> ctx
// 硬件异常无法预测何时发生，所以必须用回调！
// 传给 cte_init 的 handler 是“被注册的函数” simple_trap
bool cte_init(Context*(*handler)(Event, Context*)) {
  // initialize exception entry
  asm volatile("csrw mtvec, %0" : : "r"(__am_asm_trap));
  //printf("__am_asm_trap = 0x%x\n", __am_asm_trap);
  // 把符号 __am_asm_trap 对应的地址写入 CSR mtvec，
  // 作为 trap/异常入口地址。

  // register event handler
  // 把用户传入的 handler 函数指针保存到全局变量 user_handler(随时回调)
  user_handler = handler; // “以后发生异常时，用user_handler函数指针处理”

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
  // 1. 在栈顶创建上下文结构体
  Context *ctx = (Context *)((uint8_t *)kstack.end - sizeof(Context)); // 栈顶
   // 2. 清空上下文,避免脏数据
  memset(ctx, 0, sizeof(Context));
  ctx -> mepc = (uintptr_t)entry;
  // printf("kstack.end        = %p\n", kstack.end);
  // printf("kstack start      = %p\n", kstack.start);
  // printf("arg value        = %p\n", arg);
  // printf("&arg (on stack)  = %p\n", &arg);

  // printf("ctx->gpr[12] val = 0x%x\n", ctx->gpr[12]);
  // printf("&ctx->gpr[12]    = %p\n", &ctx->gpr[12]);

  ctx->gpr[2] = (uintptr_t)kstack.end; // sp (x2) 设置上下文的入口地址
  // printf("ctx -> gpr[10] = 0x%x\n", ctx->gpr[10]);
  ctx -> gpr[10] = (uintptr_t)arg; // a0 (x10) 传递参数
  //ctx->mstatus = 0x1800;

  return (Context *)ctx;
}

void yield()
{
  //ev.event = EVENT_YIELD;
  // printf("yield called %d\n", ev.event);
#ifdef __riscv_e
  asm volatile("li a5, -1; ecall");
#else
  //for(int j=0;j< 100000;j++);
  //printf("remove the ecall\n");
  //printf("yield called\n");
  asm volatile("li a7, -1; ecall"); // nemu 中 mcause = -1

#endif
}

//与中断开关有关的接口
bool ienabled() {
  return false;
}

// enable or disable interrupt
void iset(bool enable) {
}
