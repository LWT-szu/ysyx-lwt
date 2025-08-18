#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>

#include "Vtop.h"
#include "verilated.h"
#include "verilated_vcd_c.h"

//#define NVBOARD
//void pmem_init();
extern void pmem_init(const char *filename);

#ifdef NVBOARD
#include <nvboard.h>
void nvboard_bind_all_pins(Vtop *top);
#endif

#define WAVE

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
    
    top->rst = 1;    
    top->clk =0;top->eval();contextp->timeInc(1); 
    // 推进仿真时间（通常每执行一次，仿真时间加1）

    int rst_cycles = 2; //1
    while (rst_cycles-- > 0)
    {

        top->clk = 1;top->eval();contextp->timeInc(1);
        top->clk = 0;top->eval();contextp->timeInc(1);
    }
    top->rst = 0;

    top->clk = 1;top->eval();contextp->timeInc(1);
    top->clk = 0;top->eval();contextp->timeInc(1);

    //int print_cycle = 30;
    //int cycle = 0;
    while (!contextp->gotFinish()) {
        //printf("check pc whether entry\n"); 
        // 上升沿
        top->clk = 1;
        top->eval();
        //top->clk = !top->clk; // 不断翻转，产生周期性的时钟信号

    #ifdef NVBOARD
        nvboard_update();
    #endif
        contextp->timeInc(1);
    	//top->eval();
        //printf("new version!\n");
        /*
        if (cycle <= print_cycle)
        {
            printf("cycle=%d, pc=%08x, x0=%08x, x1=%08x, x10=%08x, x11=%08x\n",
                   cycle, top->pc, top->rf0, top->rf1, top->rf10, top->rf11);
        }
            cycle++;*/
    #ifdef WAVE
        m_trace->dump(contextp->time()); // 当前仿真时间下所有信号的值
    #endif
        // 下降沿
        top->clk = 0;
        top->eval();
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
