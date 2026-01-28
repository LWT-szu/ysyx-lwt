#include "npc.h"
#include <stdio.h>

// 寄存器名字，仅在本文件可见
const char *regs[] = {
    "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
    "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};

// 获取当前NPC寄存器状态
void reg_curr_state(npc_CPU_state *dst)
{
    assert(dst != NULL);
    for (int i = 0; i < 16; i++)
    { //???????????
        dst->gpr[i] = top->rf[i];
    }
    dst->pc = top->pc;
}

// 打印寄存器 (供 sdb 使用)
void isa_reg_display()
{
    npc_CPU_state ref_state;
    reg_curr_state(&ref_state);
    printf("pc  = 0x%08x\n", ref_state.pc);
    for (int i = 0; i < 16; i++)
    { 
        printf("%-3s = 0x%08x\t", regs[i], ref_state.gpr[i]);
        if ((i + 1) % 4 == 0)
            printf("\n");
    }
}

void display_logo(){
puts("\033[38;5;117m"
     "  _____  _____  _____  _______      ______ ___  ______      _   _ _____   _____ \n"
     " |  __ \\|_   _|/ ____|/ ____\\ \\    / /___ \\__ \\|  ____|    | \\ | |  __ \\ / ____|\n"
     " | |__) | | | | (___ | |     \\ \\  / /  __) | ) | |__ ______|  \\| | |__) | |     \n"
     " |  _  /  | |  \\___ \\| |      \\ \\/ /  |__ < / /|  __|______| . ` |  ___/| |     \n"
     " | | \\ \\ _| |_ ____) | |____   \\  /   ___) / /_| |____     | |\\  | |    | |____ \n"
     " |_|  \\_\\_____|_____/ \\_____|   \\/   |____/____|______|    |_| \\_|_|     \\_____|\n"
     "                                                                                \n"
     "\33[0m");                                                                              
}