#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

/*
 * 支持的格式:
 *   %s
 *   %d  (有符号十进制)
 *   %u  (无符号十进制)
 *   %x  (无符号十六进制, 小写)
 *   %0[width]x  例如 %02x/%04x/%08x  -> 零填充到指定宽度
 *   %[width]x   例如 %4x -> 宽度最小为 width, 左侧空格填充
 * 不支持: 其它 flag、长整型、%p、%c 等（可再扩展）
 */

static inline void output_dec_unsigned(unsigned int u, int *count)
{
  if (u == 0)
  {
    putch('0');
    (*count)++;
    return;
  }
  char buf[16];
  int i = 0;
  while (u)
  {
    buf[i++] = '0' + (u % 10);
    u /= 10;
  }
  while (i--)
  {
    putch(buf[i]);
    (*count)++;
  }
}

int printf(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  int count = 0;

  for (const char *p = fmt; *p;)
  {
    if (*p != '%')
    {
      putch(*p++);
      count++;
      continue;
    }

    p++; // 跳过 '%'
    if (!*p)
      break; // 末尾单独一个 %

    // 解析可选的 flag 和 width（只处理 '0' 填充 和 数字宽度）
    int zero_pad = 0;
    int width = 0;
    if (*p == '0')
    { // 零填充
      zero_pad = 1;
      p++;
    }
    // 宽度数字
    while (*p >= '0' && *p <= '9')
    {
      width = width * 10 + (*p - '0');
      p++;
    }

    char spec = *p;
    if (!spec)
      break;
    p++; // 消耗格式符

    if (spec == 's')
    {
      char *str = va_arg(args, char *);
      if (!str)
        str = "(null)";
      int len = 0;
      for (char *t = str; *t; t++)
        len++;
      // 这里未实现右对齐逻辑，若需要可扩展
      for (int i = 0; i < len; i++)
      {
        putch(str[i]);
        count++;
      }
    }
    else if (spec == 'd')
    {
      int num = va_arg(args, int);
      unsigned int u;
      if (num < 0)
      {
        putch('-');
        count++;
        u = (unsigned int)(-(long long)num);
      }
      else
      {
        u = (unsigned int)num;
      }
      // 先把数转成倒序缓冲
      char buf[16];
      int i = 0;
      if (u == 0)
        buf[i++] = '0';
      else
      {
        while (u)
        {
          buf[i++] = '0' + (u % 10);
          u /= 10;
        }
      }
      int digits = i;
      int pad = (width > digits) ? (width - digits) : 0;
      // %0[width]d 的语义此处未实现，只使用空格（需要可同 hex 一样加 zero_pad 判断）
      while (pad--)
      {
        putch(' ');
        count++;
      }
      while (i--)
      {
        putch(buf[i]);
        count++;
      }
    }
    else if (spec == 'u')
    {
      unsigned int u = va_arg(args, unsigned int);
      char buf[16];
      int i = 0;
      if (u == 0)
        buf[i++] = '0';
      else
      {
        while (u)
        {
          buf[i++] = '0' + (u % 10);
          u /= 10;
        }
      }
      int digits = i;
      int pad = (width > digits) ? (width - digits) : 0;
      while (pad--)
      {
        putch(zero_pad ? '0' : ' ');
        count++;
      }
      while (i--)
      {
        putch(buf[i]);
        count++;
      }
    }
    else if (spec == 'x')
    {
      unsigned int u = va_arg(args, unsigned int);
      char buf[16];
      int i = 0;
      if (u == 0)
        buf[i++] = '0';
      else
      {
        while (u)
        {
          unsigned d = u & 0xf;
          buf[i++] = (d < 10) ? ('0' + d) : ('a' + (d - 10));
          u >>= 4;
        }
      }
      int digits = i;
      // 宽度含义：最小占位位数
      int pad = (width > digits) ? (width - digits) : 0;
      while (pad--)
      {
        putch(zero_pad ? '0' : ' ');
        count++;
      }
      while (i--)
      {
        putch(buf[i]);
        count++;
      }
    }
    else
    {
      // 未支持的格式，按字面输出
      putch('%');
      putch(spec);
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
