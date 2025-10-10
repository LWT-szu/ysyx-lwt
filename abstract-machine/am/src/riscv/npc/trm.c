#include <am.h>
#include <klib-macros.h>
#include <riscv/riscv.h>

// #define DEVICE_BASE 0xa0000000
// #define SERIAL_PORT (DEVICE_BASE + 0x00003f8)

extern char _heap_start;
int main(const char *args);

extern char _pmem_start;
#define PMEM_SIZE (128 * 1024 * 1024)
#define PMEM_END ((uintptr_t)&_pmem_start + PMEM_SIZE)

Area heap = RANGE(&_heap_start, PMEM_END);
static const char mainargs[MAINARGS_MAX_LEN] = TOSTRING(MAINARGS); // defined in CFLAGS

#define UART_BASE 0x10000000
#define THR_ADDR (UART_BASE + 0x0)
#define LER_ADDR (UART_BASE + 0x1)
#define IIR_ADDR (UART_BASE + 0x2)
#define FCR_ADDR (UART_BASE + 0x2)
#define LCR_ADDR (UART_BASE + 0x3)
#define MCR_ADDR (UART_BASE + 0x4)
#define LSR_ADDR (UART_BASE + 0x5)
#define MSR_ADDR (UART_BASE + 0x6)
#define LSB_ADDR (UART_BASE + 0x0)
#define MSB_ADDR (UART_BASE + 0x1)

void putch(char ch)
{
  while ((inb(LSR_ADDR) & 0x20) == 0)
  {
    // 空循环，等待LSR[5] (THRE) 位为1
  }
  outb(THR_ADDR, ch);
}
// void putch(char ch) {
//   outb(SERIAL_PORT, ch);
// }

void halt(int code)
{
  register long a0 asm("a0") = code;
  asm volatile("ebreak" : : "r"(a0));
  while (1)
    ;
}

void _trm_init()
{
  // 配置除数寄存器
  outb(LCR_ADDR, 0x80); // LCR
  outb(LSB_ADDR, 0x36); // LSB
  outb(MSB_ADDR, 0x00); // MSB

  outb(LCR_ADDR, 0x03); // LCR
  outb(FCR_ADDR, 0x07);
  outb(MCR_ADDR, 0x03);
  outb(LER_ADDR, 0x00);
  uint32_t ysyx_ascii,stuid;
  uint32_t dummy = 0;
  asm volatile("csrrw %0,mvendorid,%1" : "=r"(ysyx_ascii) : "r"(dummy));
  asm volatile("csrrw %0,marchid,%1" : "=r"(stuid) : "r"(dummy));
  //printf("mainargs: %s\n", mainargs);
  int ret = main(mainargs);
  halt(ret);
}
// #include <am.h>
// #include <klib-macros.h>
// #include <klib.h>
// #define UART_BASE 0x10000000 // 0xa00003f8

// #define UART_LCR (UART_BASE + 0X03) // 线路控制寄存器 - 设置通信格式（数据位长度、停止位数量、奇偶校验类型）
// #define UART_IER (UART_BASE + 0x01) // 中断使能寄存器 - 控制各类中断的使能状态
// #define UART_IIR (UART_BASE + 0x02) // 中断标识寄存器 - 标识当前发生的中断类型
// #define UART_LSR (UART_BASE + 0x05) // 线路状态寄存器 - 表示UART的状态信息
// #define UART_DLL (UART_BASE + 0x00) // 除数锁存器低字节 - 设置波特率的低8位
// #define UART_DLM (UART_BASE + 0x01) // 除数锁存器高字节 - 设置波特率的高8位
// #define UART_THR (UART_BASE + 0x00) // 发送保持寄存器 - 存放要发送的数据
// #define UART_MCR (UART_BASE + 0x04) // 调制解调器控制寄存器 - 控制调制解调器功能
// #define UART_MSR (UART_BASE + 0x06) // 调制解调器状态寄存器 - 表示调制解调器的状态信息

// #define UART_FREQ 115200000 // UART工作频率为115.2 MHz
// #define BAUD_DIVISOR (UART_FREQ / 115200) // 波特率分频值
// extern char _heap_start;
// int main(const char *args);

// extern char _pmem_start;
// #define PMEM_SIZE (128 * 1024 * 1024)
// #define PMEM_END  ((uintptr_t)&_pmem_start + PMEM_SIZE)

// Area heap = RANGE(&_heap_start, PMEM_END);
// //static const char mainargs[MAINARGS_MAX_LEN] = TOSTRING(MAINARGS_PLACEHOLDER); // defined in CFLAGS
// static const char mainargs[MAINARGS_MAX_LEN] = TOSTRING(MAINARGS); // defined in CFLAGS

// void init_uart() {
//   // 设置波特率
//   uint8_t lcr_value = 0x03; // 8位数据，无奇偶校验，1个停止位
//   outb(UART_LCR, 0x80); // 设置DLAB位以访问DLL和DLM寄存器
  
//   outb(UART_DLL, BAUD_DIVISOR & 0xFF); // 设置DLL寄存器（波特率分频值的低8位）
//   outb(UART_DLM, (BAUD_DIVISOR >> 8) & 0xFF); // 设置DLM寄存器（波特率分频值的高8位）
//   outb(UART_LCR, lcr_value); // 恢复LCR寄存器，清除DLAB位
//   outb(UART_MCR, 0x03); // 设置MCR寄存器，启用发射器和接收器
//   outb(UART_MSR, 0x00); // 清除MSR寄存器
//   outb(UART_IER, 0x00); // 禁用所有中断
// }



// void putch(char ch) {
//   //*(volatile uint8_t *)SERIAL_PORT = ch;
//   outb(UART_THR, ch);                  // 将字符写入THR寄存器
//   while (!(inb(UART_LSR) & 0x20)); // 等待直到THR寄存器为空
// }

// void halt(int code) {
//   asm volatile("mv a0, %0; ebreak" : :"r"(code));//ebreak
//   while (1);
// } 

// void _trm_init() {
//   uint32_t ysyx_ascii,stuid;
//   uint32_t dummy = 0;
//   // ysyx_ascii = 'ysyx' = 0x79737978
//   asm volatile("csrrw %0,mvendorid,%1" : "=r"(ysyx_ascii) : "r"(dummy));//
//   asm volatile("csrrw %0,marchid,%1" : "=r"(stuid) : "r"(dummy));
//   //printf("ysyx_ascii = 0x%x, stuid = %x\n", ysyx_ascii, stuid);
//   //printf("Welcome to -NPC!\n");
//   init_uart();
//   printf("mainargs: %s\n", mainargs);
//   int ret = main(mainargs);
//   halt(ret);
// }
