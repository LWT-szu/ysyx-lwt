AM_SRCS := platform/nemu/trm.c \
           platform/nemu/ioe/ioe.c \
           platform/nemu/ioe/timer.c \
           platform/nemu/ioe/input.c \
           platform/nemu/ioe/gpu.c \
           platform/nemu/ioe/audio.c \
           platform/nemu/ioe/disk.c \
           platform/nemu/mpe.c
#-fdata-sections 和 -ffunction-sections 告诉编译器把每个数据（全局变量）和函数放到单独的段（section）中，
#而不是把所有函数塞进同一个 .text 段
#这样链接器就可以识别出哪些段是未使用的，从而在链接时将它们剔除，减小最终生成的二进制文件的大小。
CFLAGS    += -fdata-sections -ffunction-sections
#-I 指定包含文件的搜索路径，这里添加了 NEMU 平台相关的头文件路径。
CFLAGS    += -I$(AM_HOME)/am/src/platform/nemu/include
LDSCRIPTS += $(AM_HOME)/scripts/linker.ld
LDFLAGS   += --defsym=_pmem_start=0x80000000 --defsym=_entry_offset=0x0
#--defsym=Symbol=Value: 在链接阶段定义一个符号。
LDFLAGS   += --gc-sections -e _start
#链接器会扫描并删除那些没有被引用的段（配合上面的 -ffunction-sections 使用）
#-e _start: 指定程序的入口符号为 _start。告诉链接器，ELF 文件的 Entry Point 地址就是 _start 标签的地址。

NEMUFLAGS += -l $(shell dirname $(IMAGE).elf)/nemu-log.txt##指定nemu-log.txt的 log 输出文件路径
NEMUFLAGS += -b #批处理
NEMUFLAGS += -e $(IMAGE).elf###指定加载的 ELF 文件,而不是bin镜像）,注意要在monitor上修改内容，让命令可以读取-e
###NEMUFLAGS 是传递给 NEMU 模拟器的命令行参数的集合

#在没有操作系统的情况下给 main 函数传参
MAINARGS_MAX_LEN = 64
MAINARGS_PLACEHOLDER = the_insert-arg_rule_in_Makefile_will_insert_mainargs_here
#定义最大长度和占位符字符串。这个占位符是一个很长且唯一的字符串，会在编译进二进制文件里
CFLAGS += -DMAINARGS_MAX_LEN=$(MAINARGS_MAX_LEN) -DMAINARGS_PLACEHOLDER=$(MAINARGS_PLACEHOLDER)

insert-arg: image
	@python $(AM_HOME)/tools/insert-arg.py $(IMAGE).bin $(MAINARGS_MAX_LEN) $(MAINARGS_PLACEHOLDER) "$(mainargs)"
# 调用 Python 脚本把 mainargs 的内容插到镜像文件里，模拟命令行参数。
# 打开 $(IMAGE).bin 文件，查找那一段占位字符串（就是 MAINARGS_PLACEHOLDER，已被编译进二进制里）。
# 用你传入的 mainargs（如 dummy）覆盖这段占位符（多余部分补零）

#image-dep是在哪定义的？abstract-machine/Makefile
#OBJDUMP 和 OBJCOPY 是在哪定义的？ 交叉编译工具链路径
#image-dep其实就是$(IMAGE).elf的目标文件，$(IMAGE).elf是依赖
image: image-dep
	@$(OBJDUMP) -d $(IMAGE).elf > $(IMAGE).txt
    # 反汇编。把 ELF 文件翻译成汇编代码，保存到 .txt 文件
	@echo + OBJCOPY "->" $(IMAGE_REL).bin
	@$(OBJCOPY) -S --set-section-flags .bss=alloc,contents -O binary $(IMAGE).elf $(IMAGE).bin
    # -S: Strip。移除所有调试符号和重定位信息（减小体积）
    # 生成二进制镜像文件 .bin
    # 这个参数alloc强制把 .bss 变成“有内容的段(contents)”，实际上就是在 .bin 文件末尾填充一堆 0。
    # 这对简单的加载器很重要，保证文件加载到内存后 BSS 区确实被清空了（或者占用了位置）
    # -O binary: Output binary。输出格式为纯二进制流
    # $(IMAGE).elf $(IMAGE).bin: 输入文件 -> 输出文件。
run: insert-arg
	$(MAKE) -C $(NEMU_HOME) ISA=$(ISA) run ARGS="$(NEMUFLAGS)" IMG=$(IMAGE).bin
#     $(MAKE) -C $(NEMU_HOME): 切换到 NEMU 的目录执行 Make。
#     ISA=$(ISA): 告诉 NEMU 编译/运行哪个架构（如 riscv32）。
#     run: 告诉 NEMU 的 Makefile 执行 run 目标。
#     ARGS="$(NEMUFLAGS)": 把上面定义的 flags（日志、批处理等）传过去。
#     IMG=$(IMAGE).bin: 把生成的二进制文件路径传给 NEMU。
gdb: insert-arg
	$(MAKE) -C $(NEMU_HOME) ISA=$(ISA) gdb ARGS="$(NEMUFLAGS)" IMG=$(IMAGE).bin

.PHONY: insert-arg
