# Makefile for AbstractMachine Kernels and Libraries
#这是整个 Abstract Machine 项目的“顶级构建规则”。
#负责环境检查，变量展开，目标编译、链接、归档、递归构建依赖库等。
#通过 -include $(AM_HOME)/scripts/$(ARCH).mk，自动加载体系结构/平台专项配置（比如 riscv32-nemu.mk）。
#定义了从 .c/.S 到 .o、从 .o 到 .elf、再到二进制镜像（image）的全套规则。
#@在“无操作系统、无 libc、无异常支持”的 AM/NEMU 裸机环境中，生成可控、可链接、可执行的程序。@
# 1️⃣ 编译 .c/.S → .o
# 2️⃣ 递归构建 AM / klib → .a
# 3️⃣ 用链接脚本把 .o + .a 链成裸机 .elf
### *Get a more readable version of this Makefile* by `make html` (requires python-markdown)
html:
	cat Makefile | sed 's/^\([^#]\)/    \1/g' | markdown_py > Makefile.html
.PHONY: html
#把 Makefile 转成 HTML 文档（用于阅读）
# sed 给非注释行加缩进
# markdown_py 把内容转成 HTML
# 生成 Makefile.html
# .PHONY 表示 html 不是文件名，而是“伪目标”

## 1. Basic Setup and Checks

### Default to create a bare-metal kernel image
#MAKECMDGOALS 是 make 内置变量，表示命令行上指定的目标列表
#去查一查 MAKECMDGOALS 的用法，等于命令行里指定的所有 target？make_point
#如果用户没有指定任何目标（run,all,clean...），就默认把 image 作为目标
ifeq ($(MAKECMDGOALS),)
  MAKECMDGOALS  = image
  .DEFAULT_GOAL = image
endif

#==================二、环境与合法性检查（非常重要）================
### Override checks when `make clean/clean-all/html`
ifeq ($(findstring $(MAKECMDGOALS),clean|clean-all|html),)
#判断当前 make 的目标是不是 clean / clean-all / html，如果不是，就执行下面的规则
#make clean 不需要 ARCH / SRCS / 编译器 等变量，所以跳过检查

### Print build info message
$(info # Building $(NAME)-$(MAKECMDGOALS) [$(ARCH)])
#$(info ...) 是 Makefile 专门用于打印信息到标准输出（构建日志） 的函数

$(info [DEBUG] Current working dir: $(shell pwd), LEV: $(MAKELEVEL), GOALS: [$(MAKECMDGOALS)])
### Check: environment variable `$AM_HOME` looks sane
# wildcard 在文件系统中查找匹配指定模式的文件 / 目录
# 确保 AM_HOME 被正确设置，并且指向一个有效的 AbstractMachine 仓库
ifeq ($(wildcard $(AM_HOME)/am/include/am.h),)
  $(error $$AM_HOME must be an AbstractMachine repo)
endif

### Check: environment variable `$ARCH` must be in the supported list
ARCHS = $(basename $(notdir $(shell ls $(AM_HOME)/scripts/*.mk)))
# filter为内置的字符串过滤 / 匹配函数
# 从第二个参数的字符串集合中，筛选出与第一个参数指定的「匹配模式」相匹配的字符串，
# 最终返回所有匹配成功的字符串（多个匹配结果以空格分隔，无匹配项则返回空字符串）
# 这里是检查用户指定的 ARCH 是否在支持的ARCHS列表里
# 和 scripts/$(ARCH).mk 强相关
ifeq ($(filter $(ARCHS), $(ARCH)), )
  $(error Expected $$ARCH in {$(ARCHS)}, Got "$(ARCH)")
endif

### Extract instruction set architecture (`ISA`) and platform from `$ARCH`. Example: `ARCH=x86_64-qemu -> ISA=x86_64; PLATFORM=qemu`
#拆 ARCH 得到 ISA / PLATFORM，架构和平台
#比如 riscv32-nemu拆为 riscv32 和 nemu
ARCH_SPLIT = $(subst -, ,$(ARCH))
ISA        = $(word 1,$(ARCH_SPLIT))
PLATFORM   = $(word 2,$(ARCH_SPLIT))

### Check if there is something to build
# flavor 查询指定变量的「定义类型 / 属性」，并返回该变量对应的属性标识字符串，
# 用于判断变量的定义状态、赋值方式等元信息
# 注意：变量名直接写，不加$( )引用符
# 核心作用：判断变量 SRCS 是否从未被定义/赋值过（检查有没有源码），
# SRCS = xxx.c yyy.c，（cpu-test的Makefile）若未定义则执行后续代码
ifeq ($(flavor SRCS), undefined)
  $(error Nothing to build)
endif

### Checks end here
endif
#==================二、环境与合法性检查（非常重要）END================

## 2. General Compilation Targets
#==================三、构建目录 & 输出文件================

### Create the destination directory (`build/$ARCH`)
# 所有 .o、.d 文件都放这里，不同 ARCH 完全隔离，就是建一个 build/riscv32-nemu 目录
WORK_DIR  = $(shell pwd)
DST_DIR   = $(WORK_DIR)/build/$(ARCH)
$(shell mkdir -p $(DST_DIR))
#-p递归创建目录

### Compilation targets (a binary image or archive)
# 构建产物（可执行文件或库文件）的输出路径和文件名，都是后面用的基础变量
# 1.IMAGE_REL 在后面用终端打印出相对路径
# 2.IMAGE 为绝对路径，作为一个基础文件名前缀，后续规则会在它后面加后缀来生成不同的文件
#  作为一个基础文件名前缀，后续规则会在它后面加后缀来生成不同的文件
#  $(IMAGE).elf：链接生成的带有调试符号的可执行文件（给 GDB 用的）。
#  $(IMAGE).bin：通过 objcopy 生成的纯二进制文件（给 NEMU 加载用的）
#  $(IMAGE).txt：反汇编文件（给人类看汇编代码用的）。
# 3.ARCHIVE 是构建的静态库文件路径，给作为库（Library）的项目用的
# 	当编译 am 时：
# 	NAME = am, ARCH = riscv32-nemu
# 	ARCHIVE 变为 .../am/build/am-riscv32-nemu.a
# 	作用：Makefile 判断如果你不是在生成 image，而是在生成 archive
# 	（第 162 行 ifeq ($(MAKECMDGOALS),archive)），
# 	它就会把所有 .o 文件打包成这个 .a 静态库文件，供其他程序链接使用
IMAGE_REL = build/$(NAME)-$(ARCH)
IMAGE     = $(abspath $(IMAGE_REL))
ARCHIVE   = $(WORK_DIR)/build/$(NAME)-$(ARCH).a

#==============四、OBJS / LIBS / LINKAGE（核心）===================
### Collect the files to be linked: object files (`.o`) and libraries (`.a`)
#它们决定了编译器需要编译哪些文件，以及链接器需要链接哪些文件
OBJS      = $(addprefix $(DST_DIR)/, $(addsuffix .o, $(basename $(SRCS))))
#	$(addsuffix .o, ...)：给上一步的结果加上 .o 后缀
#	(addprefix $(DST_DIR)/, ...)：给每个文件加上构建目录的前缀（例如 build/riscv32-nemu/）
#	OBJS 最终得到的是所有源文件对应的目标文件（.o 文件）的完整路径列表
#	比如 build/riscv32-nemu/tests/dummy.o
#作用：告诉 Make，所有的源代码都需要编译成位于 build/ 目录下对应的 .o 文件。
#	这也是后续编译规则（Line 144-160）的目标。

LIBS     := $(sort $(LIBS) am klib) # lazy evaluation ("=") causes infinite recursions
#	定义依赖库列表,确定当前程序需要链接哪些 Abstract Machine 的基础库
# 	语法解析：
# 		:=：立即赋值。这里必须用立即赋值，因为右边引用了 $(LIBS) 自身，如果用延迟赋值 = 会导致死循环。
# 		am klib：这是强制添加的两个核心库。任何 AM 程序（除了库本身）
# 		都需要 libam.a (硬件抽象层) 和 libklib.a (C 标准库的简化实现)。
# 		$(LIBS)：保留用户可能在之前的 Makefile 中手动指定的其他库。
# 		$(sort ...)：排序并去重。防止重复链接同一个库。
# 	作用：不仅方便后续生成编译选项（比如 -I 包含路径），更重要的是触发递归编译。
# 	Makefile 后面（Line 173）会遍历这个列表，去调用 am 和 klib 子目录下的 
# 	Makefile，把它们先编译成 .a 文件。


LINKAGE   = $(OBJS)  # library archives are added by LIB_TEMPLATE below
#	目的：定义最终链接生成可执行文件时，需要传给链接器 (ld) 的所有输入文件
# 	内容：
# 		首先包含 $(OBJS)：即当前项目自身的源码编译出的 .o 文件（比如 dummy.o）。
# 		注意注释：这行代码只是初始化。后面的 LIB_TEMPLATE 宏（Line 170 LINKAGE += ...）
# 		会动态地把上面计算出的 LIBS 对应的 .a 静态库文件追加到这个变量里。
# 	最终形态：当 Make 执行到链接步骤时，LINKAGE 的值会变成类似：
# 		build/.../dummy.o .../am/build/am-riscv32-nemu.a .../klib/build/klib-riscv32-nemu.a。
# 	作用：在 Line 181 $(IMAGE).elf: $(LINKAGE) ... 中作为依赖项和输入参数，
# 		确保链接器能找到所有需要的符号（函数和变量）

## 3. General Compilation Flags
#===========五、编译器与 CFLAGS / LDFLAGS 设置===================
### (Cross) compilers, e.g., mips-linux-gnu-g++
#CROSS_COMPILE 是交叉编译工具链的前缀
AS        = $(CROSS_COMPILE)gcc
CC        = $(CROSS_COMPILE)gcc
CXX       = $(CROSS_COMPILE)g++
LD        = $(CROSS_COMPILE)ld
AR        = $(CROSS_COMPILE)ar
#AR 归档工具，用于创建、修改和提取静态库文件（.a 文件）
OBJDUMP   = $(CROSS_COMPILE)objdump
# OBJDUMP 用于显示二进制文件的内容，常用于调试和分析可执行文件、目标文件和库文件。
OBJCOPY   = $(CROSS_COMPILE)objcopy
# OBJCOPY 用于在不同格式之间转换二进制文件，或者从二进制文件中提取或剥离特定的部分。
READELF   = $(CROSS_COMPILE)readelf
# READELF 用于显示 ELF 格式文件的详细信息，如节区、符号表和头信息

### Compilation flags
# 自动收集所有需要的头文件目录，并生成编译器能识别的 -I 参数列表
INC_PATH += $(WORK_DIR)/include $(addsuffix /include/, $(addprefix $(AM_HOME)/, $(LIBS)))
# $(addprefix $(AM_HOME)/, $(LIBS))：为每个库（如 am、klib）加上仓库路径前缀，
# 得到如 $(AM_HOME)/am、$(AM_HOME)/klib
# $(addsuffix /include/, ...)：再为每个库路径加上 /include/ 后缀，
# 得到如 $(AM_HOME)/am/include/、$(AM_HOME)/klib/include/
INCFLAGS += $(addprefix -I, $(INC_PATH))
# 为 INC_PATH 中每个目录加上 -I 前缀，变成编译器参数
# 例如 -I/path/to/am/include -I/path/to/klib/include
# 让编译器在这些目录里查找头文件，避免找不到声明导致编译错误

ARCH_H := arch/$(ARCH).h
#arch/riscv32-nemu.h

#-fno-asynchronous-unwind-tables 是什么意思？？？？？？？？？？？？？？？？？？？？？？？？？？？？？？？？
# -MMD 自动生成 .d 依赖文件，让你不用在 Makefile 里手动写 xxx.o: xxx.c yyy.h zzz.h
# $(INCFLAGS)指定头文件搜索路径（AM / klib / 工程自身）
# 定义字符串架构宏__ISA__ riscv32，再把 riscv32 → RISCV32 __ISA_“RISCV32__“
# 定义ARCH宏__ARCH__ riscv32-nemu，再把 riscv32-nemu → RISCV32_NEMU   __ARCH_“RISCV32_NEMU__“
# 定义平台宏__PLATFORM__ nemu，再把 nemu → NEMU __PLATFORM_“NEMU__“
# 定义ARCH_H宏 arch/riscv32-nemu.h
CFLAGS   += -O2 -MMD -Wall -Werror $(INCFLAGS) \
            -D__ISA__=\"$(ISA)\" -D__ISA_$(shell echo $(ISA) | tr a-z A-Z)__ \
            -D__ARCH__=$(ARCH) -D__ARCH_$(shell echo $(ARCH) | tr a-z A-Z | tr - _) \
            -D__PLATFORM__=$(PLATFORM) -D__PLATFORM_$(shell echo $(PLATFORM) | tr a-z A-Z | tr - _) \
            -DARCH_H=\"$(ARCH_H)\" \
            -fno-asynchronous-unwind-tables -fno-builtin -fno-stack-protector \
            -Wno-main -U_FORTIFY_SOURCE -fvisibility=hidden
#-fno-asynchronous-unwind-tables：禁用异步展开表的生成，减少代码大小，适用于不需要异常处理的内核代码。
#-fno-builtin：禁用gcc 自动用 libc 内建函数，防止编译器优化某些函数调用，确保使用自定义实现（防止偷偷调用 memcpy / printf）
#-fno-stack-protector：禁用栈保护机制，适用于内核代码，避免额外的栈检查开销。
#-Wno-main：禁止对 main 函数的警告，适用于内核代码，因为AM不使用标准的 main 函数入口点。
#-U_FORTIFY_SOURCE：取消 glibc 的安全增强，防止引入宿主库依赖，适用于内核代码。
#-fvisibility=hidden：默认隐藏符号，减少符号冲突和优化链接过程

CXXFLAGS +=  $(CFLAGS) -ffreestanding -fno-rtti -fno-exceptions
#-ffreestanding：指示编译器这是 非宿主环境，禁止假设存在 libc / OS。
#-fno-rtti：禁用运行时类型信息，减少代码大小，适用于AM内核代码。
#-fno-exceptions：禁用异常处理机制，减少代码大小，适用于AM内核代码。

ASFLAGS  += -MMD $(INCFLAGS)
#允许 .S 使用 #include AM 头文件
LDFLAGS  += -z noexecstack $(addprefix -T, $(LDSCRIPTS))
# -z noexecstack标记栈不可执行,防止某些攻击利用可执行栈注入恶意代码 
#LDSCRIPTS 来自 platform/nemu.mk
#-T 告诉链接器："不要用默认的布局，用我给的链接脚本文件来安排内存布局"
#将LDSCRIPTS = linker.ld other.ld 变成-T linker.ld -T other.ld
#指定链接脚本
#决定：段布局	入口地址	内存起始位置

## 4. Arch-Specific Configurations

### Paste in arch-specific configurations (e.g., from `scripts/x86_64-qemu.mk`)
# -MMD 对应的 包含依赖文件，这些文件定义了特定架构和平台的编译选项、源文件列表等
-include $(AM_HOME)/scripts/$(ARCH).mk
#此时会跳转读取 scripts/riscv32-nemu.mk 包含下面两个.mk 文件
#-> isa/riscv.mk -> platform/nemu.mk
#后果：编译器 (CC) 被指定为 riscv64-gcc，编译参数 (CFLAGS) 增加了 -march=rv32im，链接脚本 (linker.ld) 被加入
#把架构相关配置包含进来（比如 riscv32-nemu.mk）
# “-”找不到这个文件也不报错
# 这个文件里通常有什么：
# SRCS（源文件）
# LIBS（依赖库：am / klib / klib-macros）
# 架构相关的 CFLAGS / LDFLAGS
# 运行方式（nemu / qemu / native）

## 5. Compilation Rules 编译规则（.c/.cc/.cpp/.S → .o）

### Rule (compile): a single `.c` -> `.o` (gcc)
#把任意 xxx.c 编译成 build/riscv32-nemu/xxx.o
$(DST_DIR)/%.o: %.c
	@mkdir -p $(dir $@) && echo + CC $<
	@$(CC) -std=gnu11 $(CFLAGS) -c -o $@ $(realpath $<)
# 	-c：只编译，不链接
# 	$<：第一个依赖（源文件）
# 	$@：目标文件（.o）

### Rule (compile): a single `.cc` -> `.o` (g++)
$(DST_DIR)/%.o: %.cc
	@mkdir -p $(dir $@) && echo + CXX $<
	@$(CXX) -std=c++17 $(CXXFLAGS) -c -o $@ $(realpath $<)
#gcc++ 编译 .cc 文件的规则

### Rule (compile): a single `.cpp` -> `.o` (g++)
$(DST_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@) && echo + CXX $<
	@$(CXX) -std=c++17 $(CXXFLAGS) -c -o $@ $(realpath $<)

### Rule (compile): a single `.S` -> `.o` (gcc, which preprocesses and calls as)
$(DST_DIR)/%.o: %.S
	@mkdir -p $(dir $@) && echo + AS $<
	@$(AS) $(ASFLAGS) -c -o $@ $(realpath $<)

ifeq ($(MAKECMDGOALS),archive)
### Rule (archive): objects (`*.o`) -> `ARCHIVE.a` (ar)
$(ARCHIVE): $(OBJS)
	$(info [DEBUG?] Current working dir: $(shell pwd), LEV: $(MAKELEVEL), GOALS: [$(MAKECMDGOALS)])
	@echo + AR "->" $(shell realpath $@ --relative-to .)
	@$(AR) rcs $@ $^
else
#把一堆 .o打包成一个 .a（静态库）

# $(1): library name
#LIB_TEMPLATE 这是一个 Makefile 宏模板（函数）
# 当你用 $(call call_fixdep, foo, bar) 调用这个宏时，
# $(1) 会被替换成 foo，$(2) 会被替换成 bar
define LIB_TEMPLATE =
$$(AM_HOME)/$(1)/build/$(1)-$$(ARCH).a: force
	@$$(MAKE) -s -C $$(AM_HOME)/$(1) archive
LINKAGE += $$(AM_HOME)/$(1)/build/$(1)-$$(ARCH).a
endef
# 目标：am/build/am-riscv32-nemu.a
# klib/build/klib-riscv32-nemu.a
# 进入子目录（am / klib），目录下递归执行 make,调用它们的 Makefile 来生成静态库
# 因为 am、klib 是独立模块，
# 递归 make 可以保证它们各自使用自己的 Makefile，
# 同时复用同一套 ARCH 配置。

# 然后把生成的静态库.a路径追加到 LINKAGE 变量里，
# 确保最终链接时会把这些库也链接进去

### Rule (recursive make): build a dependent library (am, klib, ...)
#递归编译依赖库
$(foreach lib, $(LIBS), $(eval $(call LIB_TEMPLATE,$(lib))))
#$(foreach var, list, text)循环函数
# 	var: 循环变量名（这里是 lib）
# 	list: 要遍历的列表（这里是 $(LIBS)）
# 	text: 对每个元素执行的表达式
# 	临时变量名，每次迭代取 LIBS 中的一个值s
# 	比如 LIBS = lib1 lib2 lib3，那么：第一次循环lib = lib1,第二次循环lib = lib2......
#$(eval ...)：对 text 进行求值和展开，把结果作为 Makefile 规则添加进来
#$(call macro, arg1, arg2, ...)：调用宏模板，把 arg1, arg2 传给宏
# 	这里调用 LIB_TEMPLATE 宏，把当前的 lib 传给它
# 	比如第一次调用时，lib = am，那么相当于展开为：
# 	$(AM_HOME)/am/build/am-riscv32-nemu.a: force
# 		@$(MAKE) -s -C $(AM_HOME)/am archive
# 	LINKAGE += $(AM_HOME)/am/build/am-riscv32-nemu.a
# 	这样就递归地调用 am 的 Makefile 来生成 libam.a，并把它加入 LINKAGE
#$(lib) → 传给 (1)
endif

### Rule (link): objects (`*.o`) and libraries (`*.a`) -> `IMAGE.elf`, the final ELF binary to be packed into image (ld)
#这意味着：先编译程序源码 -> 发现需要库 -> 递归编译库 -> 库编译完 -> 回来链接
#最终链接（.o + .a → .elf）依赖：所有静态库 .a 链接脚本 .ld
$(IMAGE).elf: $(LINKAGE) $(LDSCRIPTS)
	@echo \# Creating image [$(ARCH)]
	@echo + LD "->" $(IMAGE_REL).elf
#native 用系统运行时环境的编译器和链接器
ifneq ($(filter $(ARCH),native),)
	@$(CXX) -o $@ -Wl,--whole-archive $(LINKAGE) -Wl,-no-whole-archive $(LDFLAGS_CXX)
else
#非 native 用交叉编译器的链接器,裸机，只能用 ld + 链接脚本
	@$(LD) $(LDFLAGS) -o $@ --start-group $(LINKAGE) --end-group
#-o $@：指定输出文件名为目标文件（这里是 IMAGE.elf）
#--start-group / --end-group：
#请在这两个标志之间列出的所有归档文件（.a 库）中反复搜索，
#直到解决所有未定义的符号
endif

### Rule (`#include` dependencies): paste in `.d` files generated by gcc on `-MMD`
-include $(addprefix $(DST_DIR)/, $(addsuffix .d, $(basename $(SRCS))))
#自动处理头文件依赖（-include build/*.d 头文件变了 → 自动重新编译）
# 1.addsuffix 函数：添加后缀
# 	$(addsuffix .d, src/main src/utils lib/helper)
# 	结果：src/main.d src/utils.d lib/helper.d
# 	添加了 .d 扩展名（依赖文件）
# 2.addprefix 函数：添加前缀（通常是目录）
# 	假设：DST_DIR = build
# 	$(addprefix build/, src/main.d src/utils.d lib/helper.d)
# 	结果：build/src/main.d build/src/utils.d build/lib/helper.d
#也就是-include build/src/main.d build/src/utils.d build/lib/helper.d

## 6. Miscellaneous

### Build order control
image: image-dep
archive: $(ARCHIVE)
image-dep: $(IMAGE).elf
.PHONY: image image-dep archive run

### Force to rebuild a rule
#用来强制触发规则执行，哪怕目标文件已经存在、时间戳也没变
#防止 make 以为“已经是最新的”而不干活
force:
.PHONY: force

### Clean a single project (remove `build/`)
clean:
	rm -rf Makefile.html $(WORK_DIR)/build/
.PHONY: clean

### Clean all sub-projects within depth 2 (and ignore errors)
CLEAN_ALL = $(dir $(shell find . -mindepth 2 -name Makefile))
#递归查找所有子目录下的 Makefile，然后取它们的目录名作为清理目标列表
clean-all: $(CLEAN_ALL) clean
$(CLEAN_ALL):
	-@$(MAKE) -s -C $@ clean
.PHONY: clean-all $(CLEAN_ALL)
