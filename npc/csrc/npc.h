#include <stdint.h>
#ifndef __NPC_H__
#define __NPC_H__

#define WAVE
// #define NVBOARD

#define ANSI_FG_SKYBLUE "\033[38;5;117m"
#define ANSI_FG_RED "\33[1;31m"
#define ANSI_NONE "\33[0m"
//#define CONFIG_NPC_MTRACE
#define CONFIG_NPC_ITRACE
#define RAM_MODE_BIN 1
#define SI_TRACE

#ifdef __cplusplus
extern "C" {
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
    void init_disassemble();
    void npc_disassemble(char *str, int size, uint32_t pc, uint32_t inst);
    int pmem_read(int raddr, int pc, int valid, int wen_ram);

#ifdef __cplusplus
}
#endif

#endif