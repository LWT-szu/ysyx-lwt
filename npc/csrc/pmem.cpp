#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "npc.h"

#define MEM_SIZE (1 << 24) // 16MB 
uint32_t pmem[(MEM_SIZE)];

// 可以用PC和ALU计算的结果来来访问lw,lbu
extern "C" int pmem_read(int raddr, int pc, int valid, int wen_ram)
{
    //if (raddr == 0) return ; 
    //printf("pmem_read: raddr=0x%08x pc=0x%08x\n", raddr, pc);
    if (raddr < PMEM_BASE)
    {
        printf("Error: blow base raddr=0x%08x base=0x%08x pc=0x%08x\n", raddr, PMEM_BASE, pc);
        npc_set_state(NPC_ABORT,pc,1);
        printf("\33[1;31mNPC : HIT ABORT TRAP at pc = 0x%08x\33[0m\n", npc_state.halt_pc);
        return 0;
    }

    if (raddr == RTC_ADDR)
    {
        uint64_t now = get_time_in_us();
        return (uint32_t)(now & 0xFFFFFFFF); // 低32位
    }
    if (raddr == RTC_ADDR + 4)
    {
        uint64_t now = get_time_in_us();
        return (uint32_t)(now >> 32); // 高32位
    }

    uint32_t off = raddr - PMEM_BASE;
    uint32_t idx = (off & ~0x3u) >> 2;
    if (idx >= MEM_SIZE)
    {
        printf("Error: pmem_read access out of range! raddr=0x%x idx=%d\n", raddr, idx);
        npc_set_state(NPC_ABORT, pc, 1);
        printf("\33[1;31mNPC : HIT ABORT TRAP at pc = 0x%08x\33[0m\n", npc_state.halt_pc);
        return 0;
    }
    /* ==================== mtrace ==================== */
#ifdef CONFIG_NPC_MTRACE
    if (valid && !wen_ram)
    {
        // printf("off = 0x%08x | idx = 0x%08x\n", raddr, idx);
        printf("pc=0x%08x pmem_read[%u][%08x] = 0x%08x\n", pc, idx, idx, pmem[idx]);
        printf("\n");
    }
#endif
    /* ==================== mtrace ==================== */
    //printf("pc=0x%08x pmem_read[%u][%08x] = 0x%08x\n", pc, idx, idx, pmem[idx]);
    return pmem[(off & ~0x3u) >> 2]; // 4 字节对齐的地址
}

/*waddr：写入内存的地址（字节地址，可能不是4字节对齐）
wdata：写入的数据（32位整数）
wmask：写掩码（4位，每位对应一个字节，1表示需要写，0表示不写）*/

// ALU算出的有效地址
extern "C" void pmem_write( int waddr,  int wdata, char wmask,int pc)
{
    if (waddr < PMEM_BASE)
    {
        printf("Error: blow base raddr=0x%08x base=0x%08x\n", waddr, PMEM_BASE);
        npc_set_state(NPC_ABORT, pc, 1);
        printf("\33[1;31mNPC : HIT ABORT TRAP at pc = 0x%08x\33[0m\n", npc_state.halt_pc);
    }

    if (waddr == SERIAL_PORT)
    {
        //printf("[SERIAL] write: waddr=0x%08x, wdata=0x%08x, wmask=0x%02x\n", waddr, wdata, wmask);
        putchar(wdata & 0xFF);
        fflush(stdout);               // 确保输出及时刷新：
        // printf("hello!");
        return;
    }

    uint32_t addr = (uint32_t)waddr - PMEM_BASE;
    uint32_t idx = (addr & ~0x3u) >> 2; // 地址变成4字节对齐
    if (idx >= MEM_SIZE)
    {
        printf("Error: pmem_write access out of range! waddr=0x%x idx=%d\n", waddr, idx);
        npc_set_state(NPC_ABORT, pc, 1);
        printf("\33[1;31mNPC : HIT ABORT TRAP at pc = 0x%08x\33[0m\n", npc_state.halt_pc);
    }
    for (int i = 0; i < 4; i++)     // 遍历4个字节.字节写使能
    {
        if (wmask & (1 << i))       // 每一位代表当前字节是否需要写,若 wmask 的第 i 位是1，则写入该字节，否则保留原值。
        {
            ((uint8_t *)&pmem[idx])[i] = (wdata >> (i * 8)) & 0xFF;
        }
    }
    /* ==================== mtrace start==================== */
#ifdef CONFIG_NPC_MTRACE
    printf("======== wdata=0x%08x==========\n", wdata);
    if (wmask == 0x0F) printf("pmem_sw[%u][0x%08x] = %08x\n", idx,idx, pmem[idx]);
    else printf("pmem_sb[%u][0x%08x] = %08x\n", idx,idx, pmem[idx]);
    printf("==============end==============\n");
#endif
    /* ==================== mtrace end==================== */
}
/*(1 << i)得到一个只有第 i 位是 1 的二进制数
(uint8_t *)&pmem[idx]：把 pmem[idx] 的地址强转为字节指针，可以按字节访问一个32位整数的内容
&pmem[idx] 得到的是这个元素在内存中的地址（指向一个32位整型）
[i]：表示第 i 个字节。

把这个地址（本来是 uint32_t* 类型）强制转换为 uint8_t* 类型。
意思就是把它当成一个“字节指针”，指向这个元素的第一个字节。
这样就可以用 [0]、[1]、[2]、[3] 来分别访问这个32位整数的每一个字节

(wdata >> (i * 8)) & 0xFF：把要写的数据右移 i*8 位，提取第 i 个字节，然后与0xFF做位掩码只保留低8位
作用：将 wdata 的第 i 个字节写到 pmem[idx] 的第 i 个字节（如果 wmask 允许）*/
#ifdef RAM_MODE_BIN
    size_t pmem_init(const char *filename)
    {
        FILE *fp = fopen(filename, "rb");
        if (!fp)
        {
            perror("open bin file");
            exit(1);
        }
        // 把文件内容读到 pmem
        size_t n = fread(pmem, 1, MEM_SIZE * sizeof(uint32_t), fp);
        printf("Loaded %zu bytes from %s\n", n, filename);
        fclose(fp);
        return n;
    }
#endif

#ifndef RAM_MODE_BIN
    //十六进制.hex
    void pmem_init(const char *filename)
    {
        FILE *fp = fopen(filename, "r");
        if (!fp)
        {
            perror("open mem.hex");
            exit(1);
        }
        int idx = 0;
        uint32_t inst;
        while (fscanf(fp, "%x", &inst) == 1)
        {
            if (idx >= MEM_SIZE)
            {
                printf("Error: mem.hex too large! idx=%d\n", idx);
                exit(1);
            }
            pmem[idx++] = inst;
        }
        printf("Loaded %d instructions from %s\n", idx, filename);
        fclose(fp);
    }
#endif
