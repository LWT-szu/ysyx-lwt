#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include "npc.h"

#include "Vtop.h"
#include "verilated.h"
#include "verilated_vcd_c.h"

extern size_t pmem_init(const char *filename);
VerilatedContext *contextp;
Vtop *top;
VerilatedVcdC *m_trace;
int end; // 单步执行用的end
FILE *itrace_fp = NULL;
char str[128];

#ifdef NVBOARD
#include <nvboard.h>
    void
    nvboard_bind_all_pins(Vtop *top);
#endif

const char *reg[32] = {
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
    "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};

void cpu_exec(int n,int print_inst){
    int count =0;
    
    while (!contextp->gotFinish() && (n == -1 || count < n) ) {
        // 上升沿
        top->clk = 0;top->eval();contextp->timeInc(1);
#ifdef WAVE
        m_trace->dump(contextp->time()); // 当前仿真时间下所有信号的值
#endif
        //printf("[DEBUG] while pc = 0x%08x\n",top->pc);
/* ==================== NPC_State ==================== */
        if(npc_state.state == NPC_END || npc_state.state == NPC_ABORT){
            if(npc_state.state == NPC_ABORT){
                #ifdef LOG
                npc_disassemble(str, sizeof(str), top->pc, top->inst_out);
                fprintf(itrace_fp, "0x%08x:      %08x      %s\n", top->pc, top->inst_out, str);
                #endif

                printf("\33[1;31mNPC : HIT ABORT TRAP at pc = 0x%08x\33[0m\n", npc_state.halt_pc);

            }else if(npc_state.halt_ret == 0){
#ifdef LOG
                npc_disassemble(str, sizeof(str), top->pc, top->inst_out);
                fprintf(itrace_fp, "0x%08x:      %08x      %s\n", top->pc, top->inst_out, str);//log ebreak
                fprintf(itrace_fp, "\033[38;5;117mNPC : HIT GOOD TRAP at pc = 0x%08x\033[0m\n", npc_state.halt_pc);
#endif
                printf("\033[38;5;117ma0 = %08x\033[0m\n", top->a0);
                printf("\033[38;5;117mNPC : HIT GOOD TRAP at pc = 0x%08x\033[0m\n", npc_state.halt_pc);
                end = 1;
            }else{
#ifdef LOG
                npc_disassemble(str, sizeof(str), top->pc, top->inst_out);
                fprintf(itrace_fp, "0x%08x:      %08x      %s\n", top->pc, top->inst_out, str);
#endif
                printf("\33[1;31mError instruction : %08x\33[0m\n", top->inst_out);
                printf("\033[1;31mNPC : HIT BAD TRAP at pc = 0x%08x\033[0m\n", npc_state.halt_pc);
            }
            break;
        }
/* ==================== NPC_State ==================== */

/* ==================== NPC_ITRAC ==================== */
#ifdef LOG
        // 实现命令c的日志加载rdata_ram
        if(n==-1 && itrace_fp){
            npc_disassemble(str, sizeof(str), top->pc, top->inst_out);
            fprintf(itrace_fp,"0x%08x:      %08x      %s",top->pc,top->inst_out,str);
            /* ==================== 记录访存操作的地址和数据 ==================== */
            if(!top->is_load_type && !top->w_ram) fprintf(itrace_fp,"\n");

            if (top->is_load_type)
            {
                fprintf(itrace_fp, "        [addr:%08x] [read_ram:%08x]\n", top->alu_ram, top->rdata_ram);
            }
            if (top->w_ram)
            {
                fprintf(itrace_fp, "        [addr:%08x] [write_ram:%08x]\n", top->alu_ram, top->rs2_data);
            }
            /* ==================== 记录访存操作的地址和数据 ==================== */
            fflush(itrace_fp);
        }
#endif
        // 只在单步/si时打印
        if (print_inst){
            //printf("          0x%08x \n",top->inst_out);
            npc_disassemble(str,sizeof(str), top->pc, top->inst_out);
#ifdef LOG
            if(itrace_fp){
                fprintf(itrace_fp, "0x%08x:      %08x      %s", top->pc, top->inst_out, str);
                /* ==================== 记录访存操作的地址和数据 ==================== */
                if (!top->is_load_type && !top->w_ram) fprintf(itrace_fp,"\n");

                if (top->is_load_type) fprintf(itrace_fp, "        [addr:%08x] [read_ram:%08x]\n", top->alu_ram, top->rdata_ram);
                if (top->w_ram) fprintf(itrace_fp, "        [addr:%08x] [write_ram:%08x]\n", top->alu_ram,top->rs2_data);
                /* ==================== 记录访存操作的地址和数据 ==================== */
                fflush(itrace_fp);
            }
#endif
            uint8_t inst_byte[4]; // 两个字节分开输出
            inst_byte[0] = top->inst_out & 0xFF;
            inst_byte[1] = (top->inst_out >> 8 ) & 0xFF;
            inst_byte[2] = (top->inst_out >> 16 ) & 0xFF;
            inst_byte[3] = (top->inst_out >> 24 ) & 0xFF;
            printf("0x%08x: %02x %02x %02x %02x %s\n", top->pc, inst_byte[3], inst_byte[2], inst_byte[1], inst_byte[0],str);
        }
/* ==================== NPC_ITRAC ==================== */
        
        //top->clk = !top->clk; // 不断翻转，产生周期性的时钟信号
    #ifdef NVBOARD
        nvboard_update();
    #endif
        
        //top->eval();
        //printf("new version!\n");
        // 下降沿
        top->clk = 1;top->eval();contextp->timeInc(1); // 再推进到 t+2
    #ifdef WAVE
        m_trace->dump(contextp->time());
    #endif

    #ifdef CONFIG_DIFFTEST
            difftest_step();//把difftest_step()从打印/日志逻辑里拿到主循环每一轮的末尾,你一开始把他放主循环外面了
    #endif
        count++;
    }
}



int main(int argc, char** argv) {
    if (argc < 2)
    {
        printf("Usage: %s mem.hex\n", argv[0]);
        return 1;
    }


    contextp = new VerilatedContext; // VerilatedContext *contextp
#ifdef WAVE
    contextp->traceEverOn(true);
#endif
    contextp->commandArgs(argc, argv);
    top = new Vtop{contextp}; // Vtop* top

#ifdef CONFIG_DIFFTEST
    size_t imge_size = pmem_init(argv[1]);
    // difftest debug上电后拉高复位
    top->rst = 1;
    top->clk = 0;top->eval();
    top->clk = 1;top->eval();
    // 拉低复位，进入正常运行
    top->rst = 0;

    //printf("[DEBUG] pc = 0x%08x\n",top->pc);
    init_difftest(imge_size, 0);
#else
    pmem_init(argv[1]);
#endif

    printf("load file%s\n",argv[1]);
    

#ifdef CONFIG_NPC_ITRACE
    init_disassemble();
#endif

#ifdef LOG
    itrace_fp = fopen("npc-itrace-log.txt","w");
    fprintf(itrace_fp, "Load image file : %s\n", argv[1]);
    if(!itrace_fp){
        perror("npc-itrace-log.txt 创建失败");
        exit(1);
    } 
#endif

#ifdef NVBOARD
    nvboard_bind_all_pins(top);  
    nvboard_init();             
#endif

#ifdef WAVE
    m_trace = new VerilatedVcdC; // VerilatedVcdC* m_trace
    top->trace(m_trace, 99);                    
    m_trace->open("wave.vcd");            
#endif
    end = 0;

#ifdef AUTO_RUN
    cpu_exec(-1, 0);
#elif
    while(1){
        // 程序等待用户输入指令
        //char cmd[32];
        //printf("(npc) ");
        //fgets(cmd, sizeof(cmd), stdin)
        //Makefile需要链接时加 -lreadline
        char *cmd = npc_readline("(npc) ");
        if(cmd[0] == 'q'){
            free(cmd);
            break;
        }
        // 打印寄存器info r
        else if(strncmp(cmd,"info r",6) == 0){
            int i =0;
            for(i;i<32;i++){
                printf("%3s = 0x%08x\t",reg[i],top->rf[i]);
                if ((i+1)%4==0) puts("");
            }
        }
        //单步执行si N
        else if(strncmp(cmd,"si",2) == 0){
            int step = 1;
            sscanf(cmd + 2, "%d",&step);
            if(step <= 0) step = 1;
            if(end) printf("\033[1;33m程序已运行结束,请重新编译!\033[0m\n");
            else if(!end)cpu_exec(step,1);
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
        else if(cmd[0] == 'q') break;
        else if (cmd[0] == 'c'){
            if(end) printf("\033[1;33m程序已运行结束,请重新编译!\033[0m\n");
            else if(!end) cpu_exec(-1,0); // 连续跑
        }
        else printf("未知命令！支持 si [N], c, q,info r\n");
        free(cmd);
    }
#endif

#ifdef WAVE
    m_trace->close();    
    delete m_trace;      
#endif
    delete top;          
    delete contextp;
    if(itrace_fp) fclose(itrace_fp);
#ifdef NVBOARD
    nvboard_quit();
#endif
    return 0;
}

/*if(count >=0){
    printf("======================start==========================\n");
    printf("pc=%08x, sp=%08x,gp=%08x, tp=%08x, s0=%08x, a0=%08x, a1=%08x\n",
           top->pc, top->sp, top->gp, top->tp, top->s0, top->a0, top->a1);
    printf("count=%d,inst_out=%08x,alu_ram=0x%08x\n", count,top->inst_out, top->alu_ram);
    printf("\n");
}
    count++;
*/

/*void pmem_init(){
    pmem[(0x00000000 >> 2)] = 0x01400513; // addi a0, zero, 20 (x10 = 20)
    pmem[(0x00000004 >> 2)] = 0x00800593; // addi a1, zero, 8 (x11 = 8)
    pmem[(0x00000008 >> 2)] = 0x00b50533; // add a0, a0, a1 (x10 = x10 + x11 = 28)
    pmem[(0x0000000c >> 2)] = 0x00a50513; // addi a0, a0, 10 (x10 = x10 + 10 = 38)
    pmem[(0x00000010 >> 2)] = 0x00100073; // ebreak
}*/
