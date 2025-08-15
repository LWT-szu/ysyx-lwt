#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

void putch(char c);
int printf(const char *fmt, ...) {
  /*
  va_list args;
  va_start(args,fmt);
  int count = 0;
  for(const char*p = fmt;*p;p++){
    if(*p != '%'){
      putch(*p);
      count++;
    }else{
      p++;
      if(*p == 'd'){
        int val = va_arg(args,int);

      }
    }
  }
  va_end(args);
  return count;
  */
  panic("Not implemented");
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  panic("Not implemented");
}

int sprintf(char *out, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt); // 可变参数列表
  const char *p = fmt; // 指向格式字符串的指针
  char *q = out; // 指向输出缓冲区的指针
  while (*p) {// 遍历格式字符串
    if (*p == '%') {
      p++;

      if (*p == 's') {// 处理 %s：字符串
        char *str = va_arg(ap, char *);
        while (*str) *q++ = *str++;  // 把字符串逐字符复制到输出
      } 

      else if (*p == 'd') {
        int num = va_arg(ap, int);

        if (num < 0) {// 处理负数
          *q++ = '-';
          num = -num;
        }

        char buf[20], *r = buf;    // 临时缓冲区，倒序存储数字字符
        if (num == 0) *r++ = '0';// 特殊处理0
        else {
          while (num) {
            *r++ = '0' + num % 10; // 存个位，十位，百位...（倒序）字符 '0'是把个位数字根据ASCII变成对应的字符
            num /= 10;
          }
        }
        while (r > buf) *q++ = *--r;// 倒序输出到结果字符串
      } 

      else {                // 其它格式直接输出
        *q++ = '%';
        *q++ = *p;
      }
      p++;
    } 

    else {
      *q++ = *p++; // 普通字符直接输出
    }
  }
  *q = '\0';
  va_end(ap);
  return q - out; // 返回输出字符个数
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
