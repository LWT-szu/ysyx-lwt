// ftrace.c
#include "ftrace.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef uint32_t Elf32_Addr;
typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Off;
typedef int32_t  Elf32_Sword;
typedef uint32_t Elf32_Word;

int ftrace_depth = 0;
#define EI_NIDENT 16

typedef struct {
  unsigned char e_ident[EI_NIDENT];
  Elf32_Half    e_type;
  Elf32_Half    e_machine;
  Elf32_Word    e_version;
  Elf32_Addr    e_entry;
  Elf32_Off     e_phoff;
  Elf32_Off     e_shoff;
  Elf32_Word    e_flags;
  Elf32_Half    e_ehsize;
  Elf32_Half    e_phentsize;
  Elf32_Half    e_phnum;
  Elf32_Half    e_shentsize;
  Elf32_Half    e_shnum;
  Elf32_Half    e_shstrndx;
} Elf32_Ehdr;

typedef struct {
  Elf32_Word sh_name;
  Elf32_Word sh_type;
  Elf32_Word sh_flags;
  Elf32_Addr sh_addr;
  Elf32_Off  sh_offset;
  Elf32_Word sh_size;
  Elf32_Word sh_link;
  Elf32_Word sh_info;
  Elf32_Word sh_addralign;
  Elf32_Word sh_entsize;
} Elf32_Shdr;

typedef struct {
  Elf32_Word st_name;
  Elf32_Addr st_value;
  Elf32_Word st_size;
  unsigned char st_info;
  unsigned char st_other;
  Elf32_Half st_shndx;
} Elf32_Sym;

// ELF section type
#define SHT_SYMTAB  2
#define SHT_STRTAB  3

// ELF symbol type
#define STT_FUNC    2
#define ELF32_ST_TYPE(i) ((i)&0xf)

FtraceFunc func_table[MAX_FUNC];
int func_cnt = 0;
char *strtab = NULL;

//解析ELF文件
void ftrace_init(const char *elf_path) {
  //printf("[ftrace Debug]ftrace_init called with elf_path = %s\n",elf_path);
  /* ========================= 解析ELF文件 ========================= */
  FILE *fp = fopen(elf_path, "rb");
  assert(fp);

  Elf32_Ehdr ehdr;
  //fread(&ehdr, sizeof(ehdr), 1, fp);
  size_t read_cnt;
  read_cnt = fread(&ehdr, sizeof(ehdr), 1, fp);
  (void)read_cnt;

  // 1. 找到 Section Header Table
  fseek(fp, ehdr.e_shoff, SEEK_SET);
  int shnum = ehdr.e_shnum;
  Elf32_Shdr *shdrs = malloc(sizeof(Elf32_Shdr) * shnum);
  //fread(shdrs, sizeof(Elf32_Shdr), shnum, fp);
  read_cnt = fread(shdrs, sizeof(Elf32_Shdr), shnum, fp);
  (void)read_cnt;

  // 2. 找到 .symtab 和 .strtab
  Elf32_Shdr *symtab_sh = NULL, *strtab_sh = NULL;
  for (int i = 0; i < shnum; i++) {
    if (shdrs[i].sh_type == SHT_SYMTAB) symtab_sh = &shdrs[i];
    // 只找第一个strtab（通常是符号表用的）
    if (shdrs[i].sh_type == SHT_STRTAB && !strtab_sh) strtab_sh = &shdrs[i];
  }
  assert(symtab_sh && strtab_sh);

  // 3. 读取 strtab
  strtab = malloc(strtab_sh->sh_size);
  fseek(fp, strtab_sh->sh_offset, SEEK_SET);
  //fread(strtab, strtab_sh->sh_size, 1, fp);
  read_cnt = fread(strtab, strtab_sh->sh_size, 1, fp);
  (void)read_cnt;

  // 4. 读取 symtab
  int symnum = symtab_sh->sh_size / symtab_sh->sh_entsize;
  printf("[ftrace debug] symtab offset: 0x%x, strtab offset: 0x%x\n", symtab_sh->sh_offset, strtab_sh->sh_offset);
  Elf32_Sym *syms = malloc(symtab_sh->sh_size);
  fseek(fp, symtab_sh->sh_offset, SEEK_SET);
  //fread(syms, symtab_sh->sh_entsize, symnum, fp);
  read_cnt = fread(syms, symtab_sh->sh_entsize, symnum, fp);
  (void)read_cnt;
  /* ========================= 解析ELF文件 ========================= */

  // 5. 提取 FUNC 类型符号
  func_cnt = 0;
  for (int i = 0; i < symnum; i++) {
    if (ELF32_ST_TYPE(syms[i].st_info) == STT_FUNC && syms[i].st_size > 0) {
      if (func_cnt < MAX_FUNC) {
        func_table[func_cnt].st_value = syms[i].st_value;
        func_table[func_cnt].st_size  = syms[i].st_size;
        func_table[func_cnt].st_name  = syms[i].st_name;
        func_cnt++;
      }
    }
  }
  free(shdrs);
  free(syms);
  fclose(fp);

}

// 地址转函数名
const char *ftrace_funcname(uint32_t addr) {
  for (int i = 0; i < func_cnt; i++) {
    // 检查地址是否在这个函数的范围内
    if (addr >= func_table[i].st_value && addr < func_table[i].st_value + func_table[i].st_size){
       // printf("[ftrace] addr=0x%08x match function %s at 0x%08x\n", addr, strtab + func_table[i].st_name, func_table[i].st_value);
        return strtab + func_table[i].st_name;
    }
  }
  //printf("[ftrace] addr=0x%08x not matched any func\n", addr);
  return "???";
}