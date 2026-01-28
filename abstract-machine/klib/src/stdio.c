#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  int count = 0;

  for (const char *p = fmt; *p; ++p)
  {
    if (*p != '%')
    {
      putch(*p);
      count++;
      continue;
    }
    // 处理格式符
    ++p;
    if (*p == 's')
    {
      const char *str = va_arg(args, const char *);
      if (!str)
        str = "(null)";
      while (*str)
      {
        putch(*str++);
        count++;
      }
    }
    else if (*p == 'u')
    {
      unsigned int num = va_arg(args, unsigned int);
      char buf[16];
      int i = 0;
      if (num == 0)
        buf[i++] = '0';
      while (num)
      {
        buf[i++] = '0' + (num % 10);
        num /= 10;
      }
      while (i--)
      {
        putch(buf[i]);
        count++;
      }
    }
    else if (*p == 'd')
    {
      int num = va_arg(args, int);
      // 处理负数
      unsigned int u;
      if (num < 0)
      {
        putch('-');
        count++;
        u = (unsigned int)(-num);
      }
      else
      {
        u = (unsigned int)num;
      }
      // 转字符串逆序暂存
      char buf[16];
      int i = 0;
      if (u == 0)
        buf[i++] = '0';
      while (u)
      {
        buf[i++] = '0' + (u % 10);
        u /= 10;
      }
      // 倒序输出
      while (i--)
      {
        putch(buf[i]);
        count++;
      }
    }
    else if (*p == 'c')
    {
      char ch = (char)va_arg(args, int);
      putch(ch);
      count++;
    }
    else if (*p == 'x')
    {
      unsigned int num = va_arg(args, unsigned int);
      char buf[16];
      int i = 0;
      if (num == 0)
        buf[i++] = '0';
      while (num)
      {
        int d = num & 0xf;
        buf[i++] = (d < 10) ? ('0' + d) : ('a' + d - 10);
        num >>= 4;
      }
      while (i--)
      {
        putch(buf[i]);
        count++;
      }
    }

    else if (*p == 'p')
    {
      uintptr_t num = (uintptr_t)va_arg(args, void *);
      putch('0');
      putch('x');
      count += 2;

      char buf[2 * sizeof(uintptr_t)];
      int i = 0;
      if (num == 0)
        buf[i++] = '0';
      while (num)
      {
        int d = num & 0xf;
        buf[i++] = (d < 10) ? ('0' + d) : ('a' + d - 10);
        num >>= 4;
      }
      while (i--)
      {
        putch(buf[i]);
        count++;
      }
    }

    else
    {
      // 不支持的格式，原样输出
      //putch('%');
      putch(*p);
      count += 2;
    }
  }

  va_end(args);
  return count;
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

      else if (*p == 'c') {
        char ch = (char)va_arg(ap, int);
        *q++ = ch;
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
  va_list ap;
  va_start(ap, fmt);
  int ret = vsnprintf(out, n, fmt, ap);
  va_end(ap);
  return ret;
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  char *q = out;
  const char *p = fmt;
  size_t written = 0;
  if (n == 0) return 0;
  while (*p && written < n - 1) {
    if (*p == '%') {
      p++;
      if (*p == 's') {
        char *str = va_arg(ap, char *);
        if (!str) str = "(null)";
        while (*str && written < n - 1)
          *q++ = *str++, written++;
      } else if (*p == 'd') {
        int num = va_arg(ap, int);
        if (num < 0 && written < n - 1) {
          *q++ = '-', written++;
          num = -num;
        }
        char buf[16], *r = buf;
        if (num == 0) *r++ = '0';
        else {
          while (num) {
            *r++ = '0' + num % 10;
            num /= 10;
          }
        }
        while (r > buf && written < n - 1) *q++ = *--r, written++;
      } else if (*p == 'c') {
        char ch = (char)va_arg(ap, int);
        if (written < n - 1) *q++ = ch, written++;
      } else {
        if (written < n - 1) *q++ = '%', written++;
        if (written < n - 1) *q++ = *p, written++;
      }
      p++;
    } else {
      *q++ = *p++;
      written++;
    }
  }
  *q = '\0';
  return written;
}

#endif
