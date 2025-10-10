#VSRC = $(wildcard ./vsrc/ysyx_25080201.v)
#CSRC = $(wildcard ./csrc/main.cpp)
#all:
#	@echo "Write this Makefile by your self."
#sim:
#	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
#	verilator -Wno-fatal $(VSRC) $(CSRC) --ysyx_25080201-module ysyx_25080201 --cc --trace --exe
#	make -C obj_dir -f Vtop.mk Vtop
#	./obj_dir/Vtop
#	#gtkwave wave.vcd &
#
# .PHONY:clean
#clean:
#	rm -rf obj_dir wave.vcd
#include ../Makefile


# 基本配置
#TOPNAME = ysyx_25080201
TOPNAME = ysyxSoCFull
NXDC_FILES = constr/ysyx_25080201.nxdc
INC_PATH ?=

# 项目路径
BUILD_DIR = ./build
OBJ_DIR = $(BUILD_DIR)/obj_dir
BIN = $(BUILD_DIR)/$(TOPNAME)

# Verilator配置
VERILATOR = verilator
VERILATOR_CFLAGS += -MMD --build --trace -cc -O3 --x-assign fast --x-initial fast --noassert
VERILATOR_CFLAGS += -I$(abspath ../ysyxSoC/perip/uart16550/rtl) -I$(abspath ../ysyxSoC/perip/spi/rtl)
VERILATOR_CFLAGS += --timescale "1ns/1ns" --no-timing
# 文件路径
VSRCS = $(shell find $(abspath ./vsrc) -name "*.v") \
		$(shell find $(abspath ../ysyxSoC/perip) -name "*.v") \
		$(abspath ../ysyxSoC/ready-to-run/D-stage/ysyxSoCFull.v)
CSRCS = $(shell find $(abspath ./csrc) -name "*.c" -or -name "*.cc" -or -name "*.cpp")

#SRC_AUTO_BIND = $(abspath $(BUILD_DIR)/auto_bind.cpp)

# NVBoard配置
#include $(NVBOARD_HOME)/scripts/nvboard.mk

# 生成自动绑定文件
#$(SRC_AUTO_BIND): $(NXDC_FILES)
#	python3 $(NVBOARD_HOME)/scripts/auto_pin_bind.py $^ $@

# 编译规则
#CSRCS += $(SRC_AUTO_BIND)
INCFLAGS = $(addprefix -I, $(INC_PATH))
CXXFLAGS += $(INCFLAGS) -DTOP_NAME="\"V$(TOPNAME)\""
CXXFLAGS += -fsanitize=address
CFLAGS += -fsanitize=address
# 加载和显示图像的库
LDFLAGS += -lSDL2 -lSDL2_image -lreadline -fsanitize=address

# ---------------- Capstone 自动识别与依赖配置 ----------------
# 检查 csrc/npc.h 是否定义了 CONFIG_NPC_ITRACE 宏（表示启用指令反汇编 itrace）
# 如果启用，则自动添加 Capstone 头文件和动态库依赖
ifeq ($(findstring CONFIG_NPC_ITRACE, $(shell grep -o CONFIG_NPC_ITRACE csrc/npc.h)),CONFIG_NPC_ITRACE)
CAPSTONE_PATH = $(abspath ../nemu/tools/capstone/repo)
LIBCAPSTONE = $(CAPSTONE_PATH)/libcapstone.so.5
CXXFLAGS += -I$(CAPSTONE_PATH)/include
LDFLAGS += -L$(CAPSTONE_PATH) -lcapstone
CAPSTONE_DEP = $(LIBCAPSTONE)
else 
CAPSTONE_DEP = 
endif
# --------------------------------------------------------------------------

# 主要构建目标
$(shell mkdir -p $(BUILD_DIR))
#$^代表所有依赖,给 verilator 传参数
$(BIN): $(VSRCS) $(CSRCS) $(NVBOARD_ARCHIVE) $(CAPSTONE_DEP) 
	@rm -rf $(OBJ_DIR) wave.vcd
	$(VERILATOR) $(VERILATOR_CFLAGS) \
		--top-module $(TOPNAME) $(VSRCS) $(CSRCS) $(NVBOARD_ARCHIVE) \
		$(addprefix -CFLAGS , $(CXXFLAGS)) $(addprefix -LDFLAGS , $(LDFLAGS)) \
		--Mdir $(OBJ_DIR) --exe -o $(abspath $(BIN))

# capstone 库依赖自动构建
$(LIBCAPSTONE):
	$(MAKE)	-C	../nemu/tools/capstone

sim: $(BIN)
	$(call git_commit, "sim RTL")
	./$(BIN) $(IMAGE) +trace

# 其他目标
all: $(BIN)

run: $(BIN)
	$(BIN) /home/lwt/ysyx-workbench/ysyxSoC/ready-to-run/D-stage/new.bin +trace
	$(call git_commit, "sim RTL")
#	./$(BIN) $(IMAGE) +trace  
# 运行指定的镜像

clean:
	rm -rf $(BUILD_DIR) wave.vcd obj_dir

.PHONY: all clean sim run
