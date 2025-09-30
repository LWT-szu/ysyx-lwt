#include <stdint.h>
#ifndef __NPC_H__
#define __NPC_H__

#define ANSI_FG_SKYBLUE "\033[38;5;117m"
#define ANSI_FG_RED "\33[1;31m"
#define ANSI_NONE "\33[0m"
#define PMEM_BASE 0x80000000u
#define SERIAL_PORT 0xa00003f8 // 串口（UART）
#define RTC_ADDR 0xa0000048
#ifdef __cplusplus
extern "C"
{
#endif

    enum { NPC_RUNNING,NPC_STOP,NPC_END,NPC_ABORT,NPC_QUIT};
    typedef struct
    {
        int state;
        uint32_t halt_pc;
        uint32_t halt_ret;
    }NPCState;

    extern NPCState npc_state;

    void npc_set_state(int state,uint32_t pc,uint32_t halt_pc);
    void halt(uint32_t pc,uint32_t halt_ret);
    uint64_t get_time_in_us();
#ifdef __cplusplus
}
#endif

#endif