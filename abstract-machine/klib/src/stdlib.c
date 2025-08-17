#include <am.h>
#include <klib.h>
#include <klib-macros.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
static unsigned long int next = 1;

int rand(void) {
  // RAND_MAX assumed to be 32767
  next = next * 1103515245 + 12345;
  return (unsigned int)(next/65536) % 32768;
}

void srand(unsigned int seed) {
  next = seed;
}

int abs(int x) {
  return (x < 0 ? -x : x);
}

int atoi(const char* nptr) {
  int x = 0;
  while (*nptr == ' ') { nptr ++; }
  while (*nptr >= '0' && *nptr <= '9') {
    x = x * 10 + *nptr - '0';
    nptr ++;
  }
  return x;
}

extern Area heap; // 表示可用堆区的地址范围

static uintptr_t curr = 0; // 当前分配指针（uintptr_t: 无符号指针整数表示）
static uintptr_t end = 0;  // 结束地址缓存（减少取结构体开销）

void *malloc(size_t size) {
  // On native, malloc() will be called during initializaion of C runtime.
  // Therefore do not call panic() here, else it will yield a dead recursion:
  //   panic() -> putchar() -> (glibc) -> malloc() -> panic()
#if !(defined(__ISA_NATIVE__) && defined(__NATIVE_USE_KLIB__))
  if(size == 0) return NULL;//第一次调用 malloc() 时初始化 curr/end

  if(curr == 0){
    curr = (uintptr_t)heap.start;
    end = (uintptr_t)heap.end;
  }

  size = (size + 7) & ~((size_t)7);
  void *ret = (void *)curr;
  curr += size;
  return ret;
#endif
  return NULL;
}

void free(void *ptr) {
  (void)ptr;
}

#endif
