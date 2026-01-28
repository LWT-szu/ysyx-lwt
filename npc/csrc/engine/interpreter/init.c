#include "npc.h"

void npc_engine_start()
{
    /* Receive commands from user. */
    npc_sdb_mainloop();
}