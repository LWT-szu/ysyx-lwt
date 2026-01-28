#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "npc.h"

// itrace 日志功能：记录每条指令的地址、指令机器码、反汇编结果，访存操作的地址和数据
// 单步执行功能：支持用户通过命令行界面单步执行指令
// info r命令：打印当前寄存器状态
// 扫描内存命令x N addr：打印从addr开始的N个字的内存内容
// 连续执行命令c：让程序继续运行直到结束或遇到断点

const char *reg[16] = {
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5"};
char *npc_readline(const char *prompt)
{
    char *line = readline(prompt);
    if (line && *line)
        add_history(line);
    return line;
}

// 程序等待用户输入指令
// char cmd[32];
// printf("(npc) ");
// fgets(cmd, sizeof(cmd), stdin)
// Makefile需要链接时加 -lreadline

void npc_sdb_mainloop()
{
     while(1){
        // 程序等待用户输入指令
        //char cmd[32];
        //printf("(npc) ");
        //fgets(cmd, sizeof(cmd), stdin)
        //Makefile需要链接时加 -lreadline
        char *cmd = npc_readline("(npc) ");
        if(cmd[0] == 'q'){
            npc_state.state = NPC_QUIT;
            free(cmd);
            break;
        }
        // 打印寄存器info r
        else if(strncmp(cmd,"info r",6) == 0){
            isa_reg_display();
        }
        //单步执行si N
        else if(strncmp(cmd,"si",2) == 0){
            int step = 1;
            print_inst = 1;
            sscanf(cmd + 2, "%d",&step);
            if(step <= 0) step = 1;
            // if(npc_state.state == NPC_END) printf("\033[1;33m程序已运行结束,请重新编译!\033[0m\n");
            // else if(npc_state.state == NPC_RUNNING)
            cpu_exec(step);
        }
        //扫描内存x N addr
        else if(strncmp(cmd,"x",1) == 0){
            int len=1;
            uint32_t addr = 0x80000000;
            int scan = sscanf(cmd +2, "%d %x", &len,&addr);
            uint32_t data;
            int j = 0;
            if(scan == 2){
                printf("address         data(16,0x)\n");
                for(j;j<len;j++){
                    data = pmem_read(addr + j*4, 0x80000000, 0, 0);
                    printf("0x%08x:   %08x\n", addr + j*4 , data);
                }
                
            }
        }
        else if (cmd[0] == 'c'){
            // if(npc_state.state == NPC_END) printf("\033[1;33m程序已运行结束,请重新编译!\033[0m\n");
            // else if(npc_state.state == NPC_RUNNING) 
            print_inst = 0;
            cpu_exec(-1); // 连续跑
        }
        else printf("未知命令！支持 si [N], c, q,info r\n");
        free(cmd);
    }
}