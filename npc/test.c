#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
extern "C" void flash_init(const char *bin_path)
{
    FILE *fp = fopen(bin_path, "rb");
    if (fp == NULL)
    {
        perror("Cannot open flash image");
        exit(1);
    }

    size_t size = fread(flash_mem, 1, FLASH_SIZE, fp);
    fclose(fp);

    printf("[flash_init] Loaded %zu bytes into flash from %s\n", size, bin_path);
    assert(size > 0);
}
extern "C" void flash_read(int32_t addr, int32_t *data)
{
    // 小端方式组装 4 字节
    uint32_t val = 0;
    val |= (uint32_t)flash_mem[addr + 0];
    val |= (uint32_t)flash_mem[addr + 1] << 8;
    val |= (uint32_t)flash_mem[addr + 2] << 16;
    val |= (uint32_t)flash_mem[addr + 3] << 24;

    *data = val;
    assert(addr + 4 <= FLASH_SIZE);

    // rintf("[flash_read] addr = 0x%08x, data = 0x%08x\n", addr, *data);
}