#include "npc.h"
#include <dlfcn.h>
#include "Vtop.h"
#include <capstone/capstone.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>

#ifdef NVBOARD
#include <nvboard.h>
    void nvboard_bind_all_pins(Vtop *top);
#endif

extern Vtop *top;


// 全局变量定义（确保其他文件 extern 引用的是这些）
extern size_t pmem_init(const char *filename);
VerilatedContext *contextp = NULL;
Vtop *top = NULL;
VerilatedVcdC *m_trace = NULL;
int end = 0; // 单步执行用的end
int print_inst = 0; // 是否打印指令

void init_npc_monitor(int argc, char **argv)
{
    contextp = new VerilatedContext;
#ifdef WAVE
    contextp->traceEverOn(true);
#endif
    contextp->commandArgs(argc, argv);
    top = new Vtop{contextp};
    init_timer();
    display_logo();
// ---------------------------------------
// 2. 内存、Difftest、反汇编初始化
// ---------------------------------------
#ifdef CONFIG_DIFFTEST
    printf("\033[38;5;117mCONFIG_difftest_npc:ON\033[0m\n");
    size_t imge_size = pmem_init(argv[1]);
    // 复位序列
    top->rst = 1;
    for (int i = 0; i < 10; i++)
    {
        top->clk = 0;
        top->eval();
        top->clk = 1;
        top->eval();
    }
    top->rst = 0;
    init_difftest(imge_size, 0);
#else
    pmem_init(argv[1]);
#endif
    printf("load file %s\n", argv[1]);
#ifdef DISASSEMBLE
    init_disassemble();
#endif

#ifdef LOG
    init_itrace(argv);
#endif

#ifdef NVBOARD
    nvboard_bind_all_pins(top);
    nvboard_init();
#endif

#ifdef WAVE
    m_trace = new VerilatedVcdC;
    top->trace(m_trace, 99);
    m_trace->open("wave.vcd");
#endif

    end = 0;
}

// 新增清理函数：专门负责关波形、删对象、关文件
void exit_monitor()
{
    int ret_code = 0;

#ifdef WAVE
    m_trace->close();
    delete m_trace;
#endif

#ifdef NVBOARD
    nvboard_quit();
#endif

    if (itrace_fp)
        fclose(itrace_fp);
    delete top;
    delete contextp;
}

/*void pmem_init(){
    pmem[(0x00000000 >> 2)] = 0x01400513; // addi a0, zero, 20 (x10 = 20)
    pmem[(0x00000004 >> 2)] = 0x00800593; // addi a1, zero, 8 (x11 = 8)
    pmem[(0x00000008 >> 2)] = 0x00b50533; // add a0, a0, a1 (x10 = x10 + x11 = 28)
    pmem[(0x0000000c >> 2)] = 0x00a50513; // addi a0, a0, 10 (x10 = x10 + 10 = 38)
    pmem[(0x00000010 >> 2)] = 0x00100073; // ebreak
}*/