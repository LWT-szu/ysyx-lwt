#include <stdio.h>
#include "npc.h"
FILE *itrace_fp = NULL;
void init_itrace(char **argv){
    printf("\033[38;5;117m日志打开\033[0m\n");
    itrace_fp = fopen("npc-itrace-log.txt", "w");
    if (itrace_fp)
        fprintf(itrace_fp, "Load image file : %s\n", argv[1]);
    else
    {
        perror("npc-itrace-log.txt 创建失败");
        exit(1);
    }
}

void trace_inst_log(uint32_t pc, uint32_t inst){
    if (itrace_fp)
        npc_disassemble(str, sizeof(str), top->pc, top->inst_out);
    fprintf(itrace_fp, "0x%08x:      %08x      %s", top->pc, top->inst_out, str);
    /* ==================== 记录访存操作的地址和数据 ==================== */
    if (!top->is_load_type && !top->w_ram)
        fprintf(itrace_fp, "\n");

    if (top->is_load_type)
    {
        fprintf(itrace_fp, "        [addr:%08x] [read_ram:%08x]\n", top->alu_ram, top->rdata_ram);
    }
    if (top->w_ram)
    {
        fprintf(itrace_fp, "        [addr:%08x] [write_ram:%08x]\n", top->alu_ram, top->rs2_data);
    }
    /* ==================== 记录访存操作的地址和数据 ==================== */
    fflush(itrace_fp); // 刷新缓冲区，确保日志及时写入文件
}