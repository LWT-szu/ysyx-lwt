#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <stdint.h>
#include "npc.h"

#include "Vtop.h"
#include "verilated.h"
#include "verilated_vcd_c.h"

//#define NVBOARD
extern void pmem_init(const char *filename);

#ifdef NVBOARD
#include <nvboard.h>
void nvboard_bind_all_pins(Vtop *top);
#endif

//#define WAVE
int main(int argc, char** argv) {
    //pmem_init(); // 忘了加
    if (argc < 2)
    {
        printf("Usage: %s mem.hex\n", argv[0]);
        return 1;
    }
    pmem_init(argv[1]);


    VerilatedContext *contextp = new VerilatedContext; 
#ifdef WAVE
    contextp->traceEverOn(true); 
#endif
    contextp->commandArgs(argc, argv);  
    Vtop* top = new Vtop{contextp};

#ifdef NVBOARD
    nvboard_bind_all_pins(top);  
    nvboard_init();             
#endif

#ifdef WAVE
    VerilatedVcdC* m_trace = new VerilatedVcdC;  
    top->trace(m_trace, 99);                    
    m_trace->open("wave.vcd");                   
#endif
    //int count=0;
    while (!contextp->gotFinish()) {
/* ==================== NPC_State ==================== */
        if(npc_state.state == NPC_END || npc_state.state == NPC_ABORT){
            if(npc_state.state == NPC_ABORT){
                printf("\33[1;31mNPC : HIT ABORT TRAP at pc = 0x%08x\33[0m\n", npc_state.halt_pc);
            }else if(npc_state.halt_ret == 0){
                printf("\033[38;5;117ma0 = %08x\033[0m\n", top->a0);
                printf("\033[38;5;117mNPC : HIT GOOD TRAP at pc = 0x%08x\033[0m\n", npc_state.halt_pc);
            }else{
                printf("\33[1;31mError instruction : %08x\33[0m\n", top->inst_out);
                printf("\033[1;31mNPC : HIT BAD TRAP at pc = 0x%08x\033[0m\n", npc_state.halt_pc);
            }

            break;
        }

/* ==================== NPC_State ==================== */
        // 上升沿
        top->clk = 0;
        top->eval();
        //top->clk = !top->clk; // 不断翻转，产生周期性的时钟信号
    #ifdef NVBOARD
        nvboard_update();
    #endif
        contextp->timeInc(1);
    	//top->eval();
        //printf("new version!\n");
    #ifdef WAVE
        m_trace->dump(contextp->time()); // 当前仿真时间下所有信号的值
    #endif
    /*if(count >=0){
        printf("======================start==========================\n");
        printf("pc=%08x, sp=%08x,gp=%08x, tp=%08x, s0=%08x, a0=%08x, a1=%08x\n",
               top->pc, top->sp, top->gp, top->tp, top->s0, top->a0, top->a1);
        printf("count=%d,inst_out=%08x,alu_ram=0x%08x\n", count,top->inst_out, top->alu_ram);
        printf("\n");
    }
        count++;
    */

    
        // 下降沿
        top->clk = 1;
        top->eval();
        contextp->timeInc(1); // 再推进到 t+2
#ifdef WAVE
        m_trace->dump(contextp->time());
#endif
    }

    
#ifdef WAVE
    m_trace->close();    
    delete m_trace;      
#endif
    delete top;          
    delete contextp;

#ifdef NVBOARD
    nvboard_quit();
#endif

    return 0;
}
/*void pmem_init(){
    pmem[(0x00000000 >> 2)] = 0x01400513; // addi a0, zero, 20 (x10 = 20)
    pmem[(0x00000004 >> 2)] = 0x00800593; // addi a1, zero, 8 (x11 = 8)
    pmem[(0x00000008 >> 2)] = 0x00b50533; // add a0, a0, a1 (x10 = x10 + x11 = 28)
    pmem[(0x0000000c >> 2)] = 0x00a50513; // addi a0, a0, 10 (x10 = x10 + 10 = 38)
    pmem[(0x00000010 >> 2)] = 0x00100073; // ebreak
}*/
