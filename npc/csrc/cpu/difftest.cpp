#include "npc.h"
#include <dlfcn.h>
#include "Vtop.h"
#include <capstone/capstone.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>

npc_CPU_state ref_state;
extern uint32_t pmem[];
const char *ref_reg[16] = {
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5"};

bool is_skip_ref = false;

void (*ref_difftest_memcpy)(paddr_t addr, void *buf, size_t n, bool direction) = NULL;
void (*ref_difftest_regcpy)(void *dut, bool direction) = NULL;
void (*ref_difftest_exec)(uint64_t n) = NULL;
void (*ref_difftest_raise_intr)(uint64_t NO) = NULL;

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
    // 用于在 REF(nemu) 和 DUT(npc) 之间同步内存内容
    ref_difftest_memcpy = reinterpret_cast < void (*)(paddr_t, void *, size_t, bool)> (dlsym(handle, "difftest_memcpy"));
    assert(ref_difftest_memcpy);

    // 用于在 REF(nemu) 和 DUT(npc) 之间同步寄存器内容
    ref_difftest_regcpy = reinterpret_cast<void (*)(void *, bool)>(dlsym(handle, "difftest_regcpy"));
    assert(ref_difftest_regcpy);

    // ref_difftest_exec：指向参考模型的单步执行函数
    // 让 REF(nemu) 执行指定条数的指令
    ref_difftest_exec = reinterpret_cast<void (*)(uint64_t)>(dlsym(handle, "difftest_exec"));
    assert(ref_difftest_exec);

    // 中断函数指针
    ref_difftest_raise_intr = reinterpret_cast<void (*)(uint64_t)>(dlsym(handle, "difftest_raise_intr"));
    assert(ref_difftest_raise_intr);

    // 获取初始化参考模型的函数指针
    void (*ref_difftest_init)(int) = reinterpret_cast<void (*)(int)>(dlsym(handle, "difftest_init"));
    assert(ref_difftest_init);

    // 初始化 REF
    ref_difftest_init(port);

    // -------------------------------------同步初始状态---------------------------------------
    // 内存同步：把 DUT(npc) 的内存内容同步到 REF(nemu)
    ref_difftest_memcpy(PMEM_BASE, (uint8_t *)pmem, img_size, 1); // 1: DIFFTEST_TO_REF

    // 寄存器同步：把 DUT(npc) 的寄存器同步到 REF(nemu)
    npc_CPU_state cpu_snap;
    reg_curr_state(&cpu_snap);
    ref_difftest_regcpy(&cpu_snap,1);
    //printf("init_cpu cpu.pc = 0x%08x\n", cpu_snap.pc);
}
// 跳过某些不需要对比的指令
// 例如：访问串口、时钟等外设的指令

void difftest_step(){
    npc_CPU_state dut_state;
    reg_curr_state(&dut_state);
    //printf("init_DUT: dut.pc = 0x%08x\n", dut_state.pc);
    //printf("NPC: sizeof(npc_CPU_state) = %ld\n", sizeof(npc_CPU_state));

    static uint32_t last_skip_pc = 0;// 上一次跳过的PC地址

    if(is_skip_ref){
         // Current instruction is MMIO. Sync and Mark skipping.
        ref_difftest_regcpy(&dut_state, 1); // 1: DIFFTEST_TO_REF
        last_skip_pc = dut_state.pc;
        is_skip_ref = false; // Reset flag
        return;
    }
    
    if (last_skip_pc != 0) {
        if (dut_state.pc == last_skip_pc) {
             // Still at the same PC (stall?). Keep skipping.
             ref_difftest_regcpy(&dut_state, 1); 
             return;
        }
        // We moved to a new PC!
        // Sync Ref to this new state (skipping the execution of the old PC).
        ref_difftest_regcpy(&dut_state, 1);
        last_skip_pc = 0; 
        return;
    }

    // 1. 让 REF 执行一条指令
    //printf("Difftest: exec at pc=0x%08x\n", dut_state.pc);
    ref_difftest_exec(1);

    // 2. 从 REF 拿回寄存器
    // npc_CPU_state ref_state;
    ref_difftest_regcpy(&ref_state, 0); // 0: DIFFTEST_TO_DUT
    
    // 3. 比较：所有寄存器和PC
    for(int i = 0;i<16;i++){
        if(dut_state.gpr[i] != ref_state.gpr[i]){
            printf("Difftest Fail: %s NPC=0x%08x REF=0x%08x pc=0x%08x\n", ref_reg[i], dut_state.gpr[i], ref_state.gpr[i], dut_state.pc);
            //printf("NPC : inst=0x%08x pc = 0x%08x\n", top->inst_out, top->pc);
            npc_set_state(NPC_ABORT,dut_state.pc,1);
            return;
        }
    }

    if(dut_state.pc != ref_state.pc){
        printf("Difftest fail:PC mismatch NPC=0x%08x REF=0x%08x\n",dut_state.pc,ref_state.pc);
        npc_set_state(NPC_ABORT,dut_state.pc,1);
        return;
    }
}