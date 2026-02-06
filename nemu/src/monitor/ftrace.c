// ftrace.c
/**
 * @file ftrace.c
 * @brief 函数追踪（Function Trace）模块实现
 * 
 * 本文件主要负责解析 ELF 可执行文件，提取符号表（Symbol Table）和字符串表（String Table），
 * 从而在运行时能够根据指令地址（PC）查找对应的函数名称。通常用于调试或性能分析。
 * 
 * 主要流程：
 * 1. ftrace_init: 读取 ELF 文件头 -> 读取节区头表 -> 找到 .symtab 和 .strtab -> 提取所有 FUNC 类型符号并存储。
 * 2. ftrace_funcname: 遍历存储的函数表，检查给定地址是否落在某个函数的地址范围内。
 */

#include "ftrace.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ================== ELF32 数据类型定义 ================== */
// 参考标准的 elf.h 定义，确保与 32 位 ELF 文件格式兼容
typedef uint32_t Elf32_Addr; // 程序地址
typedef uint16_t Elf32_Half; // 中等大小整数 (16-bit)
typedef uint32_t Elf32_Off;  // 文件偏移量
typedef int32_t  Elf32_Sword; // 有符号大整数
typedef uint32_t Elf32_Word; // 无符号大整数 (32-bit)

int ftrace_depth = 0; // 当前函数调用栈的深度（用于缩进输出日志等）

#define EI_NIDENT 16 // ELF 标识数组的大小

/**
 * @brief ELF 文件头 (ELF Header)
 * 位于文件的最开始，包含了描述文件与目标体系结构的信息
 */
typedef struct {
  unsigned char e_ident[EI_NIDENT]; // 魔数和其他标识信息
  Elf32_Half    e_type;      // 文件类型 (REL, EXEC, DYN 等)
  Elf32_Half    e_machine;   // 目标体系结构 (如 EM_MIPS, EM_RISCV)
  Elf32_Word    e_version;   // 文件版本
  Elf32_Addr    e_entry;     // 程序入口虚拟地址
  Elf32_Off     e_phoff;     // 程序头表 (Program Header Table) 的文件偏移
  Elf32_Off     e_shoff;     // 节区头表 (Section Header Table) 的文件偏移
  Elf32_Word    e_flags;     // 处理器特定标志
  Elf32_Half    e_ehsize;    // ELF 头的大小
  Elf32_Half    e_phentsize; // 程序头表中每个表项的大小
  Elf32_Half    e_phnum;     // 程序头表项的个数
  Elf32_Half    e_shentsize; // 节区头表中每个表项的大小
  Elf32_Half    e_shnum;     // 节区头表项的个数
  Elf32_Half    e_shstrndx;  // 包含节区名称的字符串表在节区头表中的索引
} Elf32_Ehdr;

/**
 * @brief 节区头 (Section Header)
 * 描述文件中的一个节区（Section），如 .text, .data, .symtab 等
 */
typedef struct {
  Elf32_Word sh_name;      // 节区名称（在 .shstrtab 中的索引）
  Elf32_Word sh_type;      // 节区类型 (如 SHT_PROGBITS, SHT_SYMTAB)
  Elf32_Word sh_flags;     // 节区标志 (如 SHF_WRITE, SHF_ALLOC)
  Elf32_Addr sh_addr;      // 如果节区出现在内存映像中，此为起始虚拟地址
  Elf32_Off  sh_offset;    // 此节区在文件中的偏移
  Elf32_Word sh_size;      // 此节区的大小（字节）
  Elf32_Word sh_link;      // 链接到其他节区的索引
  Elf32_Word sh_info;      // 附加信息
  Elf32_Word sh_addralign; // 地址对齐约束
  Elf32_Word sh_entsize;   // 如果节区包含固定大小的表项，此为表项大小
} Elf32_Shdr;

/**
 * @brief 符号表项 (Symbol Table Entry)
 * 描述一个符号（变量、函数等）
 */
typedef struct {
  Elf32_Word st_name;      // 符号名称（在 .strtab 中的索引）
  Elf32_Addr st_value;     // 符号的值（通常是地址）
  Elf32_Word st_size;      // 符号的大小（字节），如函数长度
  unsigned char st_info;   // 符号类型和绑定属性
  unsigned char st_other;  // 符号可见性
  Elf32_Half st_shndx;     // 符号所在的节区索引
} Elf32_Sym;

// ELF Section Types (节区类型常量)
#define SHT_SYMTAB  2 // 符号表
#define SHT_STRTAB  3 // 字符串表

// ELF Symbol Types (符号类型常量)
#define STT_FUNC    2 // 函数符号
// 从 st_info 字段中提取低 4 位作为符号类型
#define ELF32_ST_TYPE(i) ((i)&0xf)

// 全局存储解析出的函数信息
FtraceFunc func_table[MAX_FUNC]; // 函数表数组，存储函数名索引、地址范围
int func_cnt = 0;                // 当前已解析的函数数量
char *strtab = NULL;             // 字符串表内容的缓存指针

/**
 * @brief 初始化 ftrace 功能：解析 ELF 文件
 * 
 * @param elf_path ELF 文件的路径
 */
void ftrace_init(const char *elf_path) {
  //printf("[ftrace Debug]ftrace_init called with elf_path = %s\n",elf_path);
  /* ========================= 解析ELF文件 ========================= */
  
  if (elf_path == NULL) {
      return; // 如果没有指定 ELF 文件，直接返回
  }

  FILE *fp = fopen(elf_path, "rb");
  assert(fp);

  // 1. 读取 ELF 文件头 (Execute Header)
  // 用于获取节区头表的位置 (e_shoff) 和数量 (e_shnum)
  Elf32_Ehdr ehdr;
  size_t read_cnt;
  read_cnt = fread(&ehdr, sizeof(ehdr), 1, fp);
  assert(read_cnt == 1); // 确保读取成功
  (void)read_cnt;        // 防止编译器警告未使用变量

  // 2. 读取节区头表 (Section Header Table)
  // 将文件指针移动到节区头表的开始位置
  fseek(fp, ehdr.e_shoff, SEEK_SET);
  
  int shnum = ehdr.e_shnum;
  Elf32_Shdr *shdrs = malloc(sizeof(Elf32_Shdr) * shnum);
  // 一次性读取所有的节区头
  read_cnt = fread(shdrs, sizeof(Elf32_Shdr), shnum, fp);
  assert(read_cnt == shnum);
  (void)read_cnt;

  // 3. 遍历节区头表，查找符号表 (.symtab) 和 字符串表 (.strtab)
  // .symtab 包含符号定义，.strtab 包含符号名称的字符串
  Elf32_Shdr *symtab_sh = NULL, *strtab_sh = NULL;
  for (int i = 0; i < shnum; i++) {
    if (shdrs[i].sh_type == SHT_SYMTAB) symtab_sh = &shdrs[i];
    // 只找第一个 strtab。ELF 中可能包含 shstrtab（节区名字符串表），通常符号表用的 strtab 会先出现或有特定索引
    // 注意：严谨的做法是检查 shdrs[i].sh_name 在 shstrtab 中的名字是否为 ".strtab"
    // 或者检查 symtab_sh->sh_link 是否指向当前的 strtab 节区
    if (shdrs[i].sh_type == SHT_STRTAB && !strtab_sh) strtab_sh = &shdrs[i];
  }
  
  // 如果找不到这两个关键节区，无法进行符号解析，直接断言失败
  assert(symtab_sh && strtab_sh);

  // 4. 读取字符串表内容 (.strtab) 到内存
  strtab = malloc(strtab_sh->sh_size);
  fseek(fp, strtab_sh->sh_offset, SEEK_SET);
  read_cnt = fread(strtab, strtab_sh->sh_size, 1, fp);
  assert(read_cnt == 1);
  (void)read_cnt;

  // 5. 读取符号表内容 (.symtab) 到临时内存
  int symnum = symtab_sh->sh_size / symtab_sh->sh_entsize; // 计算符号总数
  printf("[ftrace debug] symtab offset: 0x%x, strtab offset: 0x%x\n", symtab_sh->sh_offset, strtab_sh->sh_offset);
  
  Elf32_Sym *syms = malloc(symtab_sh->sh_size);
  fseek(fp, symtab_sh->sh_offset, SEEK_SET);
  read_cnt = fread(syms, symtab_sh->sh_entsize, symnum, fp);
  assert(read_cnt == symnum);
  (void)read_cnt;
  /* ========================= 解析ELF文件结束 ========================= */

  // 6. 筛选并提取类型为 FUNC (函数) 的符号
  func_cnt = 0;
  for (int i = 0; i < symnum; i++) {
    // 过滤条件：符号类型是函数 (STT_FUNC) 且 大小大于 0
    if (ELF32_ST_TYPE(syms[i].st_info) == STT_FUNC && syms[i].st_size > 0) {
      if (func_cnt < MAX_FUNC) {
        // 保存函数的起始地址
        func_table[func_cnt].st_value = syms[i].st_value;
        // 保存函数的大小（用于判断地址是否落在此函数内）
        func_table[func_cnt].st_size  = syms[i].st_size;
        // 保存函数名在 strtab 中的偏移量
        func_table[func_cnt].st_name  = syms[i].st_name;
        func_cnt++;
      } else {
        printf("[ftrace warning] Too many functions, MAX_FUNC limit reached.\n");
        break;
      }
    }
  }
  
  // 释放不再需要的临时内存
  free(shdrs);
  free(syms);
  fclose(fp);
}

/**
 * @brief 根据内存地址查找对应的函数名称
 * 
 * @param addr 当前指令的地址 (PC)
 * @return const char* 对应的函数名字符串，如果未找到则返回 "???"
 */
const char *ftrace_funcname(uint32_t addr) {
  for (int i = 0; i < func_cnt; i++) {
    // 检查地址是否在这个函数的闭开区间 [start, start + size) 内
    if (addr >= func_table[i].st_value && addr < func_table[i].st_value + func_table[i].st_size){
       // printf("[ftrace] addr=0x%08x match function %s at 0x%08x\n", addr, strtab + func_table[i].st_name, func_table[i].st_value);
        // 通过 strtab 基地址 + 名字偏移量 得到函数名字符串
        return strtab + func_table[i].st_name;
    }
  }
  //printf("[ftrace] addr=0x%08x not matched any func\n", addr);
  return "???";
}