#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <nvboard.h>
#include "Vtop.h"
#include "verilated.h"
#include "verilated_vcd_c.h"


void nvboard_bind_all_pins(Vtop* top);

//#define WAVE

int main(int argc, char** argv) {

    VerilatedContext *contextp = new VerilatedContext; 
#ifdef WAVE
    contextp->traceEverOn(true); 
#endif
    contextp->commandArgs(argc, argv);  

 
    Vtop* top = new Vtop{contextp};

    nvboard_bind_all_pins(top);  
    nvboard_init();             
  
#ifdef WAVE
    VerilatedVcdC* m_trace = new VerilatedVcdC;  
    top->trace(m_trace, 99);                    
    m_trace->open("wave.vcd");                   
#endif
    
    top->resetn = 0;    
    top->clk =0;

    int reset_cycles = 10; 
    while (reset_cycles-- > 0)
    {
        
        top->clk = !top->clk;
        top->eval(); 
        contextp->timeInc(1);
    }
    top->resetn = 1; 
    
    while (!contextp->gotFinish()) {
        top->clk = !top->clk;
        nvboard_update();
        contextp->timeInc(1);
    	top->eval();
#ifdef WAVE
       
        m_trace->dump(contextp->time()); // 当前仿真时间下所有信号的值
#endif	
    }

    
#ifdef WAVE
    m_trace->close();    
    delete m_trace;      
#endif
    delete top;          
    delete contextp;    
    nvboard_quit();      

    return 0;
}
    
