#include <stdint.h>
#include <stdio.h>
#include "npc.h"
NPCState npc_state = {.state = NPC_STOP};

void halt(uint32_t pc, uint32_t halt_ret)
{
    npc_set_state(NPC_END, pc, halt_ret);
    // 设置状态为结束 halt_ret = 1 表示异常结束, halt_ret = 0 表示正常结束
}

// 判断退出状态是否“异常”**的函数，给操作系统返回退出码用。
int is_exit_status_bad()
{
    int good = (npc_state.state == NPC_END && npc_state.halt_ret == 0) ||
               (npc_state.state == NPC_QUIT);
    printf("npc_state.halt_ret=%d\n", npc_state.halt_ret);
    printf("is_exit_status_bad: good=%d ,npc_state.state=%d\n", good, npc_state.state);
    return !good;
}
