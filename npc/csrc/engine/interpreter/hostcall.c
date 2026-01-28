#include "npc.h"

void npc_set_state(int state, uint32_t pc, uint32_t halt_ret)
{
    npc_state.state = state;
    npc_state.halt_pc = pc;
    npc_state.halt_ret = halt_ret;
}
// 对应NEMU的 void invalid_inst(vaddr_t thispc)
// 就是无效指令报错提示，打印logo
extern "C" void invalid_inst_npc(uint32_t thispc, uint32_t inst)
{
    npc_state.state = NPC_ABORT;
    npc_state.halt_pc = thispc;
    npc_state.halt_ret = -1;
    printf("\33[1;31m[NPC]Invalid instruction : 0x%08x\33[0m\n", inst);
    printf("\33[1;31m There are two cases which will trigger this unexpected exception:\n"
           "1. The instruction at PC =  0x%08x  is not implemented.\n"
           "2. Something is implemented incorrectly.\n"
           "可能是跳转到了非法地址,指令为0,请打开基础设施,gtkwave调试\n"
           "指令未实现或实现错误,赶紧看看哪漏了\n"
           "\33[0m\n",thispc);
}
// 统计仿真信息并输出
// static void statistic()
