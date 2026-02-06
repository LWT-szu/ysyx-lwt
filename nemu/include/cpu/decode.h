/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#ifndef __CPU_DECODE_H__
#define __CPU_DECODE_H__

#include <isa.h>
typedef struct Decode {
  vaddr_t pc;                       
  vaddr_t snpc; // static next pc
  vaddr_t dnpc; // dynamic next pc
  ISADecodeInfo isa;
  /*如果定义了CONFIG_ITRACE，每个Decode对象就有一个
  char logbuf[128]的数组用于存放一条指令的日志信息
 （PC、机器码、反汇编结果等）*/
  IFDEF(CONFIG_ITRACE, char logbuf[128]);
} Decode;

// --- pattern matching mechanism ---
__attribute__((always_inline))
// 模板字符串转换成 key、mask(掩码决定哪些位要参与指令匹配，哪些位随意)、shift(对齐)，方便高效地做二进制模式匹配
static inline void pattern_decode(const char *str, int len,
  uint64_t *key, uint64_t *mask, uint64_t *shift){
  // __key 输出，去除右边连续的'?'后模板中必须匹配的位的值
  // __mask 输出，哪些位需要参与匹配（1为需要（对应1或0），0为忽略（对应?））
  //  __shift 输出，从最右边开始的连续'?'的数量 遇到'0'或'1'就重置为0
  uint64_t __key = 0, __mask = 0, __shift = 0;
// 处理模板字符串的第 i 个字符
//'1,0,?'分别表示该位必须为1、必须为0、可以忽略
// 从左到右处理模板字符串的每个字符，构建 __key、__mask 和 __shift
// 从 i=0 开始，递增到 len-1。
// 如果当前处理的位置 i 已经超过了模板字符串的长度 len，
// 就跳转到 finish 标签，结束处理。
#define macro(i) \
  if ((i) >= len) goto finish; \
  else { \
    char c = str[i]; \
    if (c != ' ') { /*跳过空格*/\
      Assert(c == '0' || c == '1' || c == '?', \
          "invalid character '%c' in pattern string", c); \
      __key  = (__key  << 1) | (c == '1' ? 1 : 0); \
      __mask = (__mask << 1) | (c == '?' ? 0 : 1); \
      __shift = (c == '?' ? __shift + 1 : 0); \
    } \
  }
// 64位模式匹配，支持最长64位的指令模板字符串
#define macro2(i)  macro(i);   macro((i) + 1)
#define macro4(i)  macro2(i);  macro2((i) + 2)
#define macro8(i)  macro4(i);  macro4((i) + 4)
#define macro16(i) macro8(i);  macro8((i) + 8)
#define macro32(i) macro16(i); macro16((i) + 16)
#define macro64(i) macro32(i); macro32((i) + 32)
  macro64(0);
  panic("pattern too long");
#undef macro

finish:
  *key = __key >> __shift;
  *mask = __mask >> __shift;
  *shift = __shift;
}
/*遍历模板字符串的每一位（跳过空格）。
  对于每一位：
  如果是 '1' 或 '0'，__mask 该位为1，__key 该位为模板值。
  如果是 '?'，__mask 该位为0，__key 该位为0，__shift 加1。
  统计末尾连续 ? 的数量（__shift），用于后续对齐。
  最后将 __key 和 __mask 右移 __shift 位，得到最终的 key 和 mask。*/
//=======================

__attribute__((always_inline))
static inline void pattern_decode_hex(const char *str, int len,
    uint64_t *key, uint64_t *mask, uint64_t *shift) {
  uint64_t __key = 0, __mask = 0, __shift = 0;
#define macro(i) \
  if ((i) >= len) goto finish; \
  else { \
    char c = str[i]; \
    if (c != ' ') { \
      Assert((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || c == '?', \
          "invalid character '%c' in pattern string", c); \
      __key  = (__key  << 4) | (c == '?' ? 0 : (c >= '0' && c <= '9') ? c - '0' : c - 'a' + 10); \
      __mask = (__mask << 4) | (c == '?' ? 0 : 0xf); \
      __shift = (c == '?' ? __shift + 4 : 0); \
    } \
  }

  macro16(0);
  panic("pattern too long");
#undef macro
finish:
  *key = __key >> __shift;
  *mask = __mask >> __shift;
  *shift = __shift;
}

/*1.pattern_decode
  处理格式：普通的二进制模板字符串，如 "??????? ????? ????? 000 ????? 00100 11"
  每次处理一位：每个字符代表一位（bit），'1'、'0'、'?'。
  用途：适用于逐位描述的指令格式（如 RISC-V 指令模板）。
  宏处理：macro(i) 每次处理一位，左移一位。
2. pattern_decode_hex
  处理格式：十六进制模板字符串，如 "???????3f"
  每次处理四位：每个字符代表四位（一个 nibble），'0'-'9'、'a'-'f'、'?'。
  用途：适用于用十六进制描述的指令模板（更紧凑）。
  宏处理：macro(i) 每次处理四位，左移四位。
*/

// --- pattern matching wrappers for decode ---
//用字符串模板（类似汇编格式）来描述一条指令的机器码格式，并自动解析和执行对应的代码
//模式匹配规则 INSTPAT(模式字符串, 指令名称, 指令类型, 指令执行操作);
//... 会匹配从该位置开始的所有剩余参数
#define INSTPAT(pattern, ...) do { \
  uint64_t key, mask, shift; \
  pattern_decode(pattern, STRLEN(pattern), &key, &mask, &shift); \
  if ((((uint64_t)INSTPAT_INST(s) >> shift) & mask) == key) { \
    INSTPAT_MATCH(s, ##__VA_ARGS__); /*## 是预处理器的粘贴操作符当可变参数为空时，去掉前面的逗号*/\
    goto *(__instpat_end); /* 匹配成功后，跳出整个解码流程 */ \
  } \
} while (0)
//   INSTPAT_INST(s) 返回当前解码的指令机器码
//  if ((((uint64_t)INSTPAT_INST(s) >> shift) & mask) == key)用掩码和key判断当前指令机器码是否和指令模板匹配
//   INSTPAT_MATCH匹配成功后，解析操作数并执行你定义的操作

/*goto *(__instpat_end); 就是跳转到 INSTPAT_END(name) 宏定义的位置，
也就是 concat(__instpat_end_, name): ; 这个标签处。*/

// 定义了一个跳转标签指针 __instpat_end，用于在匹配成功后跳出解码流程
#define INSTPAT_START(name) { const void * __instpat_end = &&concat(__instpat_end_, name);
// 定义了 goto 要跳转到的目的地标签
#define INSTPAT_END(name)   concat(__instpat_end_, name): ; }
// name 是用来区分不同解码流程的标签名，防止多个解码流程的跳转标签冲突。
#endif
