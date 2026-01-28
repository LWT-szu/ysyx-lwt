#include <capstone/capstone.h>
#include "npc.h"
#include <string.h>
#include <stdio.h>
#include <dlfcn.h>
#include <assert.h>

static size_t (*cs_disasm_dl)(csh handl, const uint8_t *code,
                              size_t code_size, uint32_t address, size_t count, cs_insn **insn);
static void (*cs_free_dl)(cs_insn *insn, size_t count);
static csh handle;

void init_disassemble()
{
    void *dl_handle;
    dl_handle = dlopen("/home/lwt/ysyx-workbench/nemu/tools/capstone/repo/libcapstone.so.5", RTLD_LAZY);
    if (!dl_handle)
    {
        fprintf(stderr, "dlopen failed: %s\n", dlerror());
        exit(1);
    }
    assert(dl_handle);

    cs_err (*cs_open_dl)(cs_arch arch, cs_mode mode, csh *handle) = NULL;
    cs_open_dl = (cs_err (*)(cs_arch, cs_mode, csh *))dlsym(dl_handle, "cs_open");
    assert(cs_open_dl);

    cs_disasm_dl = (size_t (*)(csh, const uint8_t *, size_t, uint32_t, size_t, cs_insn **))dlsym(dl_handle, "cs_disasm");
    assert(cs_disasm_dl);

    cs_free_dl = (void (*)(cs_insn *, size_t))dlsym(dl_handle, "cs_free");
    assert(cs_free_dl);

    cs_arch arch = CS_ARCH_RISCV;
    cs_mode mode = CS_MODE_RISCV32;
    int ret = cs_open_dl(arch, mode, &handle);
    assert(ret == CS_ERR_OK);
}

void npc_disassemble(char *str, int size, uint32_t pc, uint32_t inst)
{
    cs_insn *insn;
    uint8_t bytes[4];
    bytes[0] = inst & 0xff;
    bytes[1] = (inst >> 8) & 0xff;
    bytes[2] = (inst >> 16) & 0xff;
    bytes[3] = (inst >> 24) & 0xff;
    // 小端字节流
    //(uint8_t*)&inst
    size_t count = cs_disasm_dl(handle, bytes, 4, pc, 1, &insn);
    if (count != 1)
    {
        snprintf(str, size, "invalid");
        printf("npc_disassemble: 反汇编失败! count = %ld, pc=0x%08x inst=0x%08x\n", count, pc, inst);
    }
    int ret = snprintf(str, size, "%s", insn->mnemonic);
    if (insn->op_str[0] != '\0')
    {
        snprintf(str + ret, size - ret, "\t%s", insn->op_str);
    }
    cs_free_dl(insn, count);
}