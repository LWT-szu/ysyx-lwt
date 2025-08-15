#include <am.h>
// 为虚拟内存实验提供页表初始化、打开/关闭地址转换、建立映射等
bool vme_init(void* (*pgalloc_f)(int), void (*pgfree_f)(void*)) {
  return false;
}
// 创建受保护的地址空间（Context 里带 satp）
void protect(AddrSpace *as) {
}

void unprotect(AddrSpace *as) {
}
// 填写页表项 在 NPC 上如果暂时不开启虚拟内存，这些函数可能返回占位/直接使用物理地址
void map(AddrSpace *as, void *va, void *pa, int prot) {
}

Context *ucontext(AddrSpace *as, Area kstack, void *entry) {
  return NULL;
}
