#include <am.h>
#include <klib-macros.h>
#include <klib.h>
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
  asm volatile("mv a0, %0; ebreak" : :"r"(code));//ebreak
  while (1);
} 

void _trm_init() {
  uint32_t ysyx_ascii,stuid;
  uint32_t dummy = 0;
  // ysyx_ascii = 'ysyx' = 0x79737978
  asm volatile("csrrw %0,mvendorid,%1" : "=r"(ysyx_ascii) : "r"(dummy));//
  asm volatile("csrrw %0,marchid,%1" : "=r"(stuid) : "r"(dummy));
  //printf("ysyx_ascii = 0x%x, stuid = 0d%d\n", ysyx_ascii, stuid);
  //printf("Welcome to -NPC!\n");
  int ret = main(mainargs);
  halt(ret);
}
