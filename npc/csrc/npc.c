#include "npc.h"
#include <dlfcn.h>
#include "Vtop.h"
#include <capstone/capstone.h>
#include <stdio.h>
#include <assert.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdbool.h>
#include <sys/time.h>
extern Vtop *top;
extern uint32_t pmem[];

NPCState npc_state = { .state = NPC_RUNNING};

static size_t(*cs_disasm_dl)(csh handl,const uint8_t *code,
    size_t code_size, uint32_t address, size_t count,cs_insn **insn);
static void (*cs_free_dl)(cs_insn *insn, size_t count);

void (*ref_difftest_memcpy)(paddr_t addr,void *buf,size_t n,bool direction) = NULL;
void (*ref_difftest_regcpy)(void *dut,bool direction)= NULL;
void (*ref_difftest_exec)(uint64_t n) = NULL;
void (*ref_difftest_raise_intr)(uint64_t NO)=NULL;
static csh handle;

void npc_set_state(int state,uint32_t pc,uint32_t halt_ret){
    npc_state.state = state;
    npc_state.halt_pc = pc;
    npc_state.halt_ret = halt_ret;
}

void halt(uint32_t pc,uint32_t halt_ret){
    npc_set_state(NPC_END,pc,halt_ret);
}

void init_disassemble(){
    void *dl_handle;
    dl_handle = dlopen("/home/lwt/ysyx-workbench/nemu/tools/capstone/repo/libcapstone.so.5", RTLD_LAZY);
    if (!dl_handle)
    {
        fprintf(stderr, "dlopen failed: %s\n", dlerror());
        exit(1);
    }
    assert(dl_handle);
    
    cs_err(*cs_open_dl)(cs_arch arch,cs_mode mode,csh *handle) = NULL;
    cs_open_dl = (cs_err (*)(cs_arch, cs_mode, csh *))dlsym(dl_handle, "cs_open");
    assert(cs_open_dl);

    cs_disasm_dl = (size_t (*)(csh, const uint8_t *, size_t, uint32_t, size_t, cs_insn **))dlsym(dl_handle, "cs_disasm");
    assert(cs_disasm_dl);

    cs_free_dl = (void (*)(cs_insn *, size_t))dlsym(dl_handle, "cs_free");
    assert(cs_free_dl);

    cs_arch arch = CS_ARCH_RISCV;
    cs_mode mode = CS_MODE_RISCV32;
    int ret = cs_open_dl(arch,mode,&handle);
    assert(ret == CS_ERR_OK);
}

void npc_disassemble(char *str,int size,uint32_t pc,uint32_t inst){
    if (inst == 0x00000000)
    {
        snprintf(str, size, "invalid");
        return;
    }
    cs_insn *insn;
    uint8_t bytes[4];
    bytes[0] = inst & 0xff;
    bytes[1] = (inst >> 8) & 0xff;
    bytes[2] = (inst >> 16) & 0xff;
    bytes[3] = (inst >> 24) & 0xff;
    // 小端字节流
    //(uint8_t*)&inst
    size_t count = cs_disasm_dl(handle, bytes, 4, pc, 1, &insn);
    if (count != 1) {
        snprintf(str, size, "invalid");
        printf("npc_disassemble: 反汇编失败! count = %ld, pc=0x%08x inst=0x%08x\n", count, pc, inst);
    }
    int ret = snprintf(str,size,"%s",insn->mnemonic);
    if(insn->op_str[0] != '\0'){
        snprintf(str + ret, size - ret , "\t%s",insn->op_str);
    }
    cs_free_dl(insn,count);
}

char* npc_readline(const char *prompt){
    char *line = readline(prompt);
    if(line && *line) add_history(line);
    return line;
}

// 获取当前NPC寄存器状态
void reg_curr_state(npc_CPU_state *dst){
    assert(dst != NULL);
    for(int i = 0;i<32;i++){//???????????
        dst->gpr[i] = top->rf[i];
    }
    dst->pc = top->pc;
}

// 加载参考模型动态库，获取相关函数指针，初始化参考模型状态
void init_difftest(long img_size,int port){
    
    // 声明动态库句柄：用于存储 dlopen 返回值
    void *handle;
    // // 加载参考模型(NEMU REF)的动态库，RTLD_LAZY表示“用到哪个符号再解析哪个符号”，更快
    handle = dlopen("/home/lwt/ysyx-workbench/nemu/build/riscv32-nemu-interpreter-so", RTLD_LAZY);
    if (!handle)
    {
        fprintf(stderr, "dlopen failed: %s\n", dlerror());
        exit(1);
    }
    assert(handle);

    // --------- 下面是获取各个 DiffTest 接口的函数指针 ---------
    // verilator编译的变量类型声明与nemu不一致
    // 用于在 REF 和 DUT 之间同步内存内容
    ref_difftest_memcpy = reinterpret_cast < void (*)(paddr_t, void *, size_t, bool)> (dlsym(handle, "difftest_memcpy"));
    assert(ref_difftest_memcpy);

    // 用于在 REF 和 DUT 之间同步寄存器内容
    ref_difftest_regcpy = reinterpret_cast<void (*)(void *, bool)>(dlsym(handle, "difftest_regcpy"));
    assert(ref_difftest_regcpy);

    // ref_difftest_exec：指向参考模型的单步执行函数
    // 让 REF 执行指定条数的指令
    ref_difftest_exec = reinterpret_cast<void (*)(uint64_t)>(dlsym(handle, "difftest_exec"));
    assert(ref_difftest_exec);

    ref_difftest_raise_intr = reinterpret_cast<void (*)(uint64_t)>(dlsym(handle, "difftest_raise_intr"));
    assert(ref_difftest_raise_intr);

    void (*ref_difftest_init)(int) = reinterpret_cast<void (*)(int)>(dlsym(handle, "difftest_init"));
    assert(ref_difftest_init);

    // 初始化 REF
    ref_difftest_init(port);// 初始化参考模型
    // 用于将 DUT 的镜像内容（通常是指令和数据）拷贝到 REF
    ref_difftest_memcpy(PMEM_BASE, (uint8_t *)pmem, img_size, 1); // 1: DIFFTEST_TO_REF

    // 寄存器同步：把 DUT 的寄存器同步到 REF
    npc_CPU_state cpu_snap;
    reg_curr_state(&cpu_snap);
    ref_difftest_regcpy(&cpu_snap, 1); // 拷贝初始寄存器状态到 REF!!!!!!
}
// 跳过某些不需要对比的指令
// 例如：访问串口、时钟等外设的指令
static inline bool is_skip_difftest_inst(uint32_t addr){
    return addr == SERIAL_PORT || addr == RTC_ADDR || addr == RTC_ADDR_END;
}
void difftest_step(){
    npc_CPU_state dut_state;// DUT 当前寄存器状态
    reg_curr_state(&dut_state);
    // 如果当前指令不需要进行 DiffTest 比对，直接把 DUT 的状态同步到 REF
    // 这样 REF 就“跳过”了这条指令
    if(is_skip_difftest_inst(dut_state.pc)){
        ref_difftest_regcpy(&dut_state, 1); // 1: DIFFTEST_TO_REF
        return;
    }
    // 1. 让 REF 执行一条指令
    ref_difftest_exec(1);

    // 2. 从 REF 拿回寄存器
    npc_CPU_state ref_state;
    ref_difftest_regcpy(&ref_state, 0); // 0: DIFFTEST_TO_DUT

    // 3. 取 DUT 自己当前寄存器

    // 反汇编当前指令
    uint32_t inst = top->ifu_rdata;
    char disasm_str[64];
    npc_disassemble(disasm_str, sizeof(disasm_str), dut_state.pc, inst);

    // 4. 比较：所有寄存器和PC
    for(int i = 0;i<32;i++){
        if(dut_state.gpr[i] != ref_state.gpr[i]){
            printf("\033[1;33m[DIFFTEST]: reg[%d] NPC=0x%08x REF=0x%08x\033[0m\n", i, dut_state.gpr[i], ref_state.gpr[i]);
            printf("\033[1;33m[DIFFTEST] At LAST PC=0x%08x, INST=0x%08x, %s\033[0m\n", dut_state.pc, inst, disasm_str);
            npc_set_state(NPC_ABORT,dut_state.pc,1);
            return;
        }
    }

    if(dut_state.pc != ref_state.pc){
        printf("\033[1;33m[DIFFTEST]:PC mismatch NPC=0x%08x REF=0x%08x\033[0m\n", dut_state.pc, ref_state.pc);
        printf("\033[1;33m[DIFFTEST] At PC=0x%08x, INST=0x%08x, %s\033[0m\n", dut_state.pc, inst, disasm_str);
        npc_set_state(NPC_ABORT,dut_state.pc,1);
        return;
    }
}
// 获取当前时间，单位微秒 like nemu
static uint64_t boot_time = 0;
uint64_t get_time_in_us()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t now = (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;// 秒转微秒再加上微秒
    if (boot_time == 0) boot_time = now;// 记录启动时间
    return now - boot_time;
}