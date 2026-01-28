#include "npc.h"

char str[128];

static void exec_once()
{
    int count =0;//记录执行了多少条指令
        // 上升沿
        top->clk = 1;top->eval();contextp->timeInc(1);
        memset(str, 0, sizeof(str)); // 清空字符缓冲区
        npc_disassemble(str, sizeof(str), top->pc, top->inst_out);
        __uint128_t asm_val1 = 0;
        for (int i = 0; i < 16; ++i)
        {
            asm_val1 |= (__uint128_t)((uint8_t)str[i]) << (8 * (15 - i));
        }
        // 拆分为 4 个 32 位，赋值给 VlWide<4>
        for (int i = 0; i < 4; ++i)
        {
            top->asm_out1[i] = (uint32_t)((asm_val1 >> (32 * i)) & 0xFFFFFFFF);
        }
        // 为什么 m_trace->dump(contextp->timecpu-exec.cpp()); 放到 asm_out 更新之后，波形里 asm_out 不是当前指令的内容？
#ifdef WAVE
        m_trace->dump(contextp->time()); // 记录当前仿真时间下所有信号的值
#endif
        //printf("[DEBUG] while pc = 0x%08x\n",top->pc);
        

        //top->clk = !top->clk; // 不断翻转，产生周期性的时钟信号
    #ifdef NVBOARD
        nvboard_update();
    #endif
        
        //top->eval();
        // 下降沿
        top->clk = 0;top->eval();contextp->timeInc(1); // 再推进到 t+2
    #ifdef WAVE
        m_trace->dump(contextp->time());
    #endif

    #ifdef CONFIG_DIFFTEST
            difftest_step();//把difftest_step()从打印/日志逻辑里拿到主循环每一轮的末尾,你一开始把他放主循环外面了
    #endif
        count++;
    }

void execute(uint64_t n){
    for(uint64_t i = 0; i < n && !contextp->gotFinish(); i++){
        exec_once();
        if (npc_state.state != NPC_RUNNING) break;

        /* ==================== NPC_ITRAC ==================== */
        // 只在单步/si时打印
        if (print_inst)
        {
            // printf("          0x%08x \n",top->inst_out);
            npc_disassemble(str, sizeof(str), top->pc, top->inst_out);
            uint8_t inst_byte[4]; // 两个字节分开输出
            inst_byte[0] = top->inst_out & 0xFF;
            inst_byte[1] = (top->inst_out >> 8) & 0xFF;
            inst_byte[2] = (top->inst_out >> 16) & 0xFF;
            inst_byte[3] = (top->inst_out >> 24) & 0xFF;
            printf("0x%08x: %02x %02x %02x %02x %s\n", top->pc, inst_byte[3], inst_byte[2], inst_byte[1], inst_byte[0], str);
        }
#ifdef LOG
        trace_inst_log(top->pc, top->inst_out);
#endif
    }
        /* ==================== NPC_ITRAC ==================== */

}

//sdb mainloop里调用cpu_exec
//execute n条指令
void cpu_exec(uint64_t n)
{
    // printf("npc_state.state = %d\n", npc_state.state);
    switch (npc_state.state) {
    case NPC_END: case NPC_ABORT: case NPC_QUIT:
        printf("\033[1;33m程序已运行结束,请重新编译!\033[0m\n");
        printf("\033[1;33mProgram execution has ended. To restart the program, exit NEMU and run again.\033[0m\n");
        return;
    default: npc_state.state = NPC_RUNNING;// 将初始的STOP状态设置状态为运行中
    }
  
//   printf("开始执行CPU指令...\n");
//   printf("npc_state.state = %d\n", npc_state.state);
    execute(n);
    switch(npc_state.state) {
        // 执行完一批指令后，自动把状态从 RUNNING 切回 STOP，以便下次可以继续单步或批量执行。
        case NPC_RUNNING: npc_state.state = NPC_STOP; break;
        case NPC_ABORT:
#ifdef LOG
            npc_disassemble(str, sizeof(str), top->pc, top->inst_out);
            fprintf(itrace_fp, "0x%08x:      %08x      %s\n", top->pc, top->inst_out, str);
#endif
            printf("\33[1;31mNPC : HIT ABORT TRAP at pc = 0x%08x\33[0m\n", npc_state.halt_pc);
        case NPC_END:
            if(npc_state.halt_ret == 0){
#ifdef LOG
                npc_disassemble(str, sizeof(str), top->pc, top->inst_out);
                fprintf(itrace_fp, "0x%08x:      %08x      %s\n", top->pc, top->inst_out, str);//log ebreak
                fprintf(itrace_fp, "\033[38;5;117mNPC : HIT GOOD TRAP at pc = 0x%08x\033[0m\n", npc_state.halt_pc);
#endif
                printf("\033[38;5;117ma0 = %08x\033[0m\n", top->a0);
                printf("\033[38;5;117mNPC : HIT GOOD TRAP at pc = 0x%08x\033[0m\n", npc_state.halt_pc);
                // end = 1;
            }else{
#ifdef LOG
            npc_disassemble(str, sizeof(str), top->pc, top->inst_out);
            fprintf(itrace_fp, "0x%08x:      %08x      %s\n", top->pc, top->inst_out, str);
#endif
            printf("\33[1;31mError instruction : %08x\33[0m\n", top->inst_out);
            printf("\033[1;31mNPC : HIT BAD TRAP at pc = 0x%08x\033[0m\n", npc_state.halt_pc);
            }
        case NPC_QUIT: printf("退出程序\n");
    }

/* ==================== NPC_State ==================== */
    
}