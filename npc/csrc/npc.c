#include "npc.h"
NPCState npc_state = { .state = NPC_RUNNING};

void npc_set_state(int state,uint32_t pc,uint32_t halt_ret){
    npc_state.state = state;
    npc_state.halt_pc = pc;
    npc_state.halt_ret = halt_ret;
}

void halt(uint32_t pc,uint32_t halt_ret){
    npc_set_state(NPC_END,pc,halt_ret);
}