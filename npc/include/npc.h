#include <stdint.h>
#include <stdio.h>
#include "Vtop.h" 
#include "verilated.h"
#include "verilated_vcd_c.h"
#ifndef __NPC_H__
#define __NPC_H__

#define WAVE
// #define NVBOARD
#define ANSI_FG_SKYBLUE "\033[38;5;117m"
#define ANSI_FG_RED "\33[1;31m"
#define ANSI_NONE "\33[0m"
//#define CONFIG_NPC_MTRACE
#define DISASSEMBLE //反汇编
// #define LOG/ /ITRACE日志
#define RAM_MODE_BIN 1
#define SI_TRACE
#define PMEM_BASE 0x80000000u
#define SERIAL_PORT 0xa00003f8 // 串口（UART）0x10000000
#define RTC_ADDR 0xa0000048
#define RTC_ADDR_END 0xa0000050
#define CONFIG_DIFFTEST
// 注意如果使用外设（串口、时钟等需要关闭difftest）
#define AUTO_RUN
#ifdef __cplusplus
extern "C" {
#endif
    typedef uint32_t paddr_t; //32位物理地址
    // 声明外部变量
    extern Vtop *top;
    extern VerilatedContext *contextp;
    extern VerilatedVcdC *m_trace;
    extern FILE *itrace_fp;
    extern int print_inst;
    extern unsigned char isa_logo[];
    extern char str[128];//反汇编字符串缓冲区
    //extern uint8_t pmem[];


    enum { NPC_RUNNING,NPC_STOP,NPC_END,NPC_ABORT,NPC_QUIT};
    typedef struct
    {
        int state;
        uint32_t halt_pc;
        uint32_t halt_ret;
    }NPCState;

    extern NPCState npc_state;

    typedef struct
    {
        uint32_t gpr[16];
        uint32_t pc;
        uint32_t mtvec, mepc, mstatus, mcause;

    }npc_CPU_state;

    // 函数声明
    void cpu_exec(uint64_t n);
    void execute(uint64_t n);
    void npc_sdb_mainloop();
    //void init_npc_monitor(int argc, char **argv);
    //void exit_monitor(); // 声明新函数
    void reg_curr_state(npc_CPU_state *dst);
    void isa_reg_display();
    void display_logo();
    void init_itrace(char **argv);
    void trace_inst_log(uint32_t pc, uint32_t inst);
    //int is_exit_status_bad();
    //void npc_engine_start();

    void npc_set_state(int state, uint32_t pc, uint32_t halt_pc);
    void halt(uint32_t pc,uint32_t halt_ret);
    void init_disassemble();
    void npc_disassemble(char *str, int size, uint32_t pc, uint32_t inst);
    int pmem_read(int raddr, int pc, int valid, int wen_ram);
    char *npc_readline(const char *prompt);
    void init_difftest(long img_size,int port);
    void difftest_step();
    void init_timer();
    uint64_t get_time_in_us();

    static bool is_skip_ref = false;
    // static inline void is_skip_difftest(){
    //     is_skip_ref = true;
    //     //printf("Difftest: skip diff test for this instruction\n");
    // }

#ifdef __cplusplus
}
#endif

#endif