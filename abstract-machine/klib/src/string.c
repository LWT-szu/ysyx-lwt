// 它们不依赖系统现成的标准库（libc），而是让你自己实现这些底层常用函数，提高对底层机制的理解和掌握。
#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  size_t count = 0;
  while(*s){
    s++; // 遍历字符串的每一个字符
    count++;
  }
  return count;
}
// 将源字符串复制到目标字符串，返回目标字符串的起始地址
char *strcpy(char *dst, const char *src) {
  char* p = dst;
  while(*src != '\0'){
    *dst = *src;
    dst++;
    src++;
  }
  *dst = '\0';
  return p;
}

// 将源字符串的前 n 个字符复制到目标字符串，返回目标字符串的起始地址
char *strncpy(char *dst, const char *src, size_t n) {
  char* p = dst;
  while(n--){
    if(*src != '\0'){
      *dst = *src;
      src++;
    }
    else *dst++ = '\0';
  }
  return p;
}
// 拼接字符串：将 src 追加到 dst 的末尾，返回 dst 的起始地址
char *strcat(char *dst, const char *src) {
  char *p = dst; // 地址
  while(*dst != '\0') dst++;
  while(*src != '\0'){
    *dst = *src;
    dst++;
    src++;
  }
  *dst = '\0';
  return p;
}
// 比较两个字符串的大小，返回差值
// 比较的是每个字符的 ASCII（或 unsigned char）数值
int strcmp(const char *s1, const char *s2) {
  while (*s1 && *s2){
    if(*s1 != *s2)
      return (unsigned char)*s1 - (unsigned char)*s2;
    s1++;
    s2++;
  }
  return (unsigned char)*s1 - (unsigned char)*s2; // 如果一个提前结束或刚好相等,比较结尾字符
}

// 比较两个字符串的前 n 个字符，返回差值
int strncmp(const char *s1, const char *s2, size_t n) {
  while(n--){
    if(*s1 != *s2) return (unsigned char)*s1 - (unsigned char)*s2;
    if(*s1 == '\0') return 0;// 两边都一样且遇到\0，提前返回
    s1++;
    s2++;
  }
  return 0;
}

// 将字符 c（转换为 unsigned char）复制到字符串 s 的前 n 个字节，返回 s 的起始地址
void *memset(void *s, int c, size_t n) {
  unsigned char *p = (unsigned char *)s; 
  while(n--){
    *(p++) = (unsigned char)c; // 逐字节写入
  }
  return s;
}

// 在内存区域 src 和 dst 之间复制 n 个字节，处理重叠区域，返回 dst 的起始地址
void *memmove(void *dst, const void *src, size_t n) {
  unsigned char *ds = (unsigned char *)dst;
  unsigned char *sr = (unsigned char *)src;
  if(ds < sr || ds > sr + n){
    while (n--)
    {
      *(ds++) = *(sr++);
    }
  }
  else if(ds == sr || n==0) return dst;
  // 正向拷贝会把新写入的内容又当做源内容,导致后面的字节都被前面刚写入的内容覆盖
  // sr < ds < sr + n 表示有重叠，就是 dst 在 src 内部，需要反向拷贝
  // 内存：====src[{======需要拷贝的内容======}ds[重叠===========n========
  // 反向：====src[{==============n个字节=================}src+n[==============ds+n[===
  //                                                 sr--                 ds--
  else{// 有重叠，反向拷贝
    ds += n;
    sr += n;
    while (n--)
    {
      *(ds--) = *(sr--);
    }
  }
  return dst;
}

// 复制内存区域 src 到 dst，返回 dst 的起始地址
void *memcpy(void *out, const void *in, size_t n) {
  unsigned char *ds = (unsigned char*)out;
  unsigned char *src = (unsigned char*)in;
  while(n--){
    *(ds++) = *(src++);//先赋值后移动指针
  }
  return out;
}

// 比较内存区域 s1 和 s2 的前 n 个字节，返回差值
int memcmp(const void *s1, const void *s2, size_t n) {
  unsigned char *p1 = (unsigned char *)s1;
  unsigned char *p2 = (unsigned char *)s2;
  while(n--){
    if(*p1 != *p2) return *p1 - *p2;
    p1 ++;
    p2 ++;
  }
  return 0;
}

#endif
