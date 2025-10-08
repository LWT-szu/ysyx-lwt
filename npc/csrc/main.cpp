#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include "npc.h"

#include "VysyxSoCFull.h"
#include "verilated.h"
#include "verilated_vcd_c.h"

#define FLASH_SIZE (16 * 1024 * 1024) // 16MB, 
uint32_t flash_mem[FLASH_SIZE];


extern size_t pmem_init(const char *filename);
VerilatedContext *contextp;
VysyxSoCFull *ysyx_25080201;
VerilatedVcdC *m_trace;
int end; // 单步执行用的end
FILE *itrace_fp = NULL;// 日志文件指针
char str[128];// 反汇编字符串

extern "C" void flash_init(const char *bin_path) {
    FILE *fp = fopen(bin_path, "rb");
    if (fp == NULL) {
        perror("Cannot open flash image");
        exit(1);
    }else{
        printf("[flash_init] Opened flash image: %s\n", bin_path);
    }
    size_t size = fread(flash_mem, 1, FLASH_SIZE * sizeof(uint32_t), fp);
    fclose(fp);

    printf("[flash_init] Loaded %zu bytes into flash from %s\n", size, bin_path);
    assert(size > 0);
}
extern "C" void flash_read(int32_t addr, int32_t *data) { assert(0); }
/*
extern "C" void flash_read(int32_t addr, int32_t *data) {
    uint32_t val = 0;
    for(int i=0;i<4;i++){

    }
    // 小端方式组装4字节
    val |= (uint32_t)flash_mem[addr + 0];
    val |= (uint32_t)flash_mem[addr + 1] << 8;
    val |= (uint32_t)flash_mem[addr + 2] << 16;
    val |= (uint32_t)flash_mem[addr + 3] << 24;
    *data = val;
    assert(addr >= 0 && addr + 4 <= FLASH_SIZE);
    // printf("[flash_read] addr = 0x%08x, data = 0x%08x\n", addr, *data);
}
*/

#ifdef NVBOARD
#include <nvboard.h>
    void
    nvboard_bind_all_pins(VysyxSoCFull *ysyx_25080201);
#endif

const char *reg[32] = {
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
    "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"};

// n = -1表示无限执行
// print_inst = 1表示打印指令
void cpu_exec(int n,int print_inst){
    int count =0;
    while (!contextp->gotFinish() && (n == -1 || count < n) ) {
        // 上升沿
        ysyx_25080201->clock = 0;ysyx_25080201->eval();contextp->timeInc(1);
#ifdef WAVE
        m_trace->dump(contextp->time()); // 当前仿真时间下所有信号的值
#endif
        //printf("[DEBUG] while pc = 0x%08x\n",ysyx_25080201->pc);
/* ==================== NPC_State ==================== */
        if(npc_state.state == NPC_END || npc_state.state == NPC_ABORT){
            if(npc_state.state == NPC_ABORT){
                #ifdef LOG
                npc_disassemble(str, sizeof(str), ysyx_25080201->pc, ysyx_25080201->inst_out);
                fprintf(itrace_fp, "0x%08x:      %08x      %s\n", ysyx_25080201->pc, ysyx_25080201->inst_out, str);
                #endif
                printf("\33[1;31mNPC : HIT ABORT TRAP at pc = 0x%08x\33[0m\n", npc_state.halt_pc);

            }else if(npc_state.halt_ret == 0){
#ifdef LOG
                npc_disassemble(str, sizeof(str), ysyx_25080201->pc, ysyx_25080201->inst_out);
                fprintf(itrace_fp, "0x%08x:      %08x      %s\n", ysyx_25080201->pc, ysyx_25080201->inst_out, str);//log ebreak
                fprintf(itrace_fp, "\033[38;5;117mNPC : HIT GOOD TRAP at pc = 0x%08x\033[0m\n", npc_state.halt_pc);
#endif
                //printf("\033[38;5;117ma0 = %08x\033[0m\n", ysyx_25080201->a0);
                printf("\033[38;5;117mNPC : HIT GOOD TRAP at pc = 0x%08x\033[0m\n", npc_state.halt_pc);
                end = 1;
            }else{
#ifdef LOG
                npc_disassemble(str, sizeof(str), ysyx_25080201->pc, ysyx_25080201->inst_out);
                fprintf(itrace_fp, "0x%08x:      %08x      %s\n", ysyx_25080201->pc, ysyx_25080201->inst_out, str);
#endif
                //printf("\033[1;31mError instruction(main_debug) : %08x\33[0m\n", ysyx_25080201->inst_out);
                printf("\033[1;31mNPC : HIT BAD TRAP at pc = 0x%08x\033[0m\n", npc_state.halt_pc);
            }
            break;
        }
/* ==================== NPC_State ==================== */

/* ==================== NPC_ITRAC ==================== */
#ifdef LOG
        // 实现命令c的日志加载rdata_ram
        if(n==-1 && itrace_fp){
            npc_disassemble(str, sizeof(str), ysyx_25080201->pc, ysyx_25080201->inst_out);
            fprintf(itrace_fp,"0x%08x:      %08x      %s",ysyx_25080201->pc,ysyx_25080201->io_ifu_rdata,str);
            /* ==================== 记录访存操作的地址和数据 ==================== */
            if(!ysyx_25080201->is_load_type && !ysyx_25080201->w_ram) fprintf(itrace_fp,"\n");
            if (ysyx_25080201->is_load_type)  fprintf(itrace_fp, "        [addr:%08x] [read_ram:%08x]\n", ysyx_25080201->alu_ram, ysyx_25080201->rdata_ram);
            if (ysyx_25080201->w_ram)         fprintf(itrace_fp, "        [addr:%08x] [write_ram:%08x]\n", ysyx_25080201->alu_ram, ysyx_25080201->rs2_data);

            /* ==================== 记录访存操作的地址和数据 ==================== */
            fflush(itrace_fp);
        }
#endif
        // 只在单步/si时打印
        if (print_inst){
            //printf("          0x%08x \n",ysyx_25080201->inst_out);
            //npc_disassemble(str,sizeof(str), ysyx_25080201->pc, ysyx_25080201->inst_out);
#ifdef LOG
            if(itrace_fp){
                fprintf(itrace_fp, "0x%08x:      %08x      %s", ysyx_25080201->pc, ysyx_25080201->inst_out, str);
                /* ==================== 记录访存操作的地址和数据 ==================== */
                if (!ysyx_25080201->is_load_type && !ysyx_25080201->w_ram) fprintf(itrace_fp,"\n");
                if (ysyx_25080201->is_load_type)  fprintf(itrace_fp, "        [addr:%08x] [read_ram:%08x]\n", ysyx_25080201->alu_ram, ysyx_25080201->rdata_ram);
                if (ysyx_25080201->w_ram)         fprintf(itrace_fp, "        [addr:%08x] [write_ram:%08x]\n", ysyx_25080201->alu_ram,ysyx_25080201->rs2_data);
                /* ==================== 记录访存操作的地址和数据 ==================== */
                fflush(itrace_fp);
            }
#endif
            // uint8_t inst_byte[4]; // 单步si打印,两个字节分开输出
            // inst_byte[0] = ysyx_25080201->inst_out & 0xFF;
            // inst_byte[1] = (ysyx_25080201->inst_out >> 8 ) & 0xFF;
            // inst_byte[2] = (ysyx_25080201->inst_out >> 16 ) & 0xFF;
            // inst_byte[3] = (ysyx_25080201->inst_out >> 24 ) & 0xFF;
            // printf("0x%08x: %02x %02x %02x %02x %s\n", ysyx_25080201->pc, inst_byte[3], inst_byte[2], inst_byte[1], inst_byte[0],str);
        }
/* ==================== NPC_ITRAC ==================== */
        
    #ifdef NVBOARD
        nvboard_update();
    #endif
        
        // 下降沿
        ysyx_25080201->clock = 1;ysyx_25080201->eval();contextp->timeInc(1); // 再推进到 t+2
    #ifdef WAVE
        m_trace->dump(contextp->time());
    #endif

    #ifdef CONFIG_DIFFTEST
            difftest_step();//把difftest_step()从打印/日志逻辑里拿到主循环每一轮的末尾,你一开始把他放主循环外面了
    #endif
        count++;// 执行指令数+1
    }
}



int main(int argc, char** argv) {
    if (argc < 2)
    {
        printf("Usage: %s mem.hex\n", argv[0]);
        return 1;
    }

    // 初始化Verilator仿真环境
    contextp = new VerilatedContext; // VerilatedContext *contextp 
#ifdef WAVE
    contextp->traceEverOn(true);
#endif
    contextp->commandArgs(argc, argv);
    ysyx_25080201 = new VysyxSoCFull{contextp}; // VysyxSoCFull* ysyx_25080201

#ifdef WAVE
    m_trace = new VerilatedVcdC; // VerilatedVcdC* m_trace
    ysyx_25080201->trace(m_trace, 99);
    m_trace->open("wave.vcd");
#endif

#ifdef CONFIG_DIFFTEST
    printf("\033[38;5;117mCONFIG_difftest_npc:ON\033[0m\n");
    size_t imge_size = pmem_init(argv[1]);
    // difftest debug上电后拉高复位
    ysyx_25080201->reset = 1;
    ysyx_25080201->clock = 0;ysyx_25080201->eval();
    ysyx_25080201->clock = 1;ysyx_25080201->eval();
    // 拉低复位，进入正常运行
    ysyx_25080201->reset = 0;

    //printf("[DEBUG] pc = 0x%08x\n",ysyx_25080201->pc);
    init_difftest(imge_size, 0);
#else
//     ysyx_25080201->reset = 1;
//     ysyx_25080201->clock = 0;ysyx_25080201->eval();contextp->timeInc(1);
// #ifdef WAVE
//     m_trace->dump(contextp->time());
// #endif
//     ysyx_25080201->clock = 1;ysyx_25080201->eval();contextp->timeInc(1);
//     // 拉低复位，进入正常运行
//     ysyx_25080201->reset = 0;
// #ifdef WAVE
//     m_trace->dump(contextp->time());
// #endif
    flash_init(argv[1]);
    printf("\033[38;5;117mLoad_File:%s\033[0m\n", argv[1]);
#endif


#ifdef CONFIG_NPC_ITRACE
    init_disassemble();
#endif

#ifdef LOG
    printf("\033[38;5;117m加载NPC日志\033[0m\n");
    itrace_fp = fopen("npc-itrace-log.txt","w");
    if(!itrace_fp){
        perror("npc-itrace-log.txt 创建失败");
        assert(0);
    }
    fprintf(itrace_fp, "Load image file : %s\n", argv[1]);
#endif

#ifdef NVBOARD
    nvboard_bind_all_pins(ysyx_25080201);  
    nvboard_init();             
#endif
/*
#ifdef WAVE
    m_trace = new VerilatedVcdC; // VerilatedVcdC* m_trace
    ysyx_25080201->trace(m_trace, 99);                    
    m_trace->open("wave.vcd");            
#endif
*/
    end = 0;// 单步执行用的end

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
                printf("%3s = 0x%08x\t",reg[i],ysyx_25080201->rf[i]);
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
    delete ysyx_25080201;          
    delete contextp;
    if(itrace_fp) fclose(itrace_fp);
#ifdef NVBOARD
    nvboard_quit();
#endif
    return 0;
}

