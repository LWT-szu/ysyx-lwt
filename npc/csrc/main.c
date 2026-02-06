#include <stdio.h>
#include <stdint.h>
#include "npc.h"
void init_npc_monitor(int, char *[]);
int is_exit_status_bad();  
void exit_monitor();
void npc_engine_start();
void cpu_exec(uint64_t n);

int main(int argc, char **argv) {
    // 1. 初始化所有模块 (包括打开波形)
    init_npc_monitor(argc, argv);
    
   // 2. 运行主循环 (Auto run 或 调试命令行)
#ifdef AUTO_RUN
    cpu_exec(-1);
#else
    npc_engine_start();//sdb mainloop
#endif
    exit_monitor();
    return is_exit_status_bad();
    //这个返回值会影响AM层trm.c判断FAIL还是PASS
}

