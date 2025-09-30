// ftrace.h
#ifndef __FTRACE_H__
#define __FTRACE_H__

#include <stdint.h>

#define MAX_FUNC 4096
extern int ftrace_depth;

typedef struct
{
    uint32_t st_value; // 函数起始地址
    uint32_t st_size;  // 函数大小
    uint32_t st_name;  // 名字在strtab偏移
} FtraceFunc;

void ftrace_init(const char *elf_path);
const char *ftrace_funcname(uint32_t addr);

#endif