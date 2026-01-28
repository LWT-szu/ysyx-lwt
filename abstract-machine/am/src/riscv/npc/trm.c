#include <am.h>
#include <klib-macros.h>
#include <stdio.h>
#define SERIAL_PORT 0xa00003f8

extern char _heap_start;
int main(const char *args);

extern char _pmem_start;
#define PMEM_SIZE (128 * 1024 * 1024)
#define PMEM_END  ((uintptr_t)&_pmem_start + PMEM_SIZE)

Area heap = RANGE(&_heap_start, PMEM_END);
static const char mainargs[MAINARGS_MAX_LEN] = TOSTRING(MAINARGS_PLACEHOLDER); // defined in CFLAGS

void putch(char ch) {
  *(volatile uint8_t *)SERIAL_PORT = ch;
}

void halt(int code) {
  asm volatile("mv a0, %0; ebreak" : :"r"(code));//ebreak 将退出码code放在a0寄存器 
  while (1);
} 

void _trm_init() {
  // uint32_t ysyx, id;
  // asm volatile(
  //     "csrr %0, mvendorid\n"
  //     "csrr %1, marchid\n"
  //     : "=r"(ysyx), "=r"(id));
  // printf("mvendorid = %x\n", ysyx);
  // printf("marchid   = %d\n", id);
  
  int ret = main(mainargs);
  //asm volatile("li a5, -1; ecall");
  //asm volatile(".word 0x00000000\n");//检查无效指令
  halt(ret);
  // halt(0); // 正常退出
  // halt(1); // 错误退出 注意halt函数的具体实现！！
}
