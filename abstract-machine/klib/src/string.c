// 它们不依赖系统现成的标准库（libc），而是让你自己实现这些底层常用函数，提高对底层机制的理解和掌握。
#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  size_t count = 0;
  while(*s){
    s++;
    count++;
  }
  return count;
}
//
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

char *strncpy(char *dst, const char *src, size_t n) {
  char* p = dst;
  while(n--){
    if(*src != '\0'){
      *dst = *src++;
    }
    else *dst++ = '\0';
  }
  return p;
}

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
//
int strcmp(const char *s1, const char *s2) {
  while (*s1 && *s1){
    if(*s1 != *s2) return *s1 - *s2;
    s1++;
    s2++;
  }
  return *s1 - *s2; // 如果一个提前结束,比较结尾字符
}

int strncmp(const char *s1, const char *s2, size_t n) {
  while(n--){
    if(*s1 != *s2) return (unsigned char)*s1 - (unsigned char)*s2;
    if(*s1 == '\0') return 0;// 两边都一样且遇到\0，提前返回
  }
  return 0;
}

void *memset(void *s, int c, size_t n) {
  unsigned char *p = (unsigned char *)s; // 逐字节写入
  while(n--){
    *p++ = (unsigned char)c;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  panic("Not implemented");
}

void *memcpy(void *out, const void *in, size_t n) {
  unsigned char *ds = (unsigned char*)out;
  unsigned char *src = (unsigned char*)in;
  while(n--){
    *ds++ = *src++;
  }
  return out;
}
//
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
