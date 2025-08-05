#VSRC = $(wildcard ./vsrc/top.v)
#CSRC = $(wildcard ./csrc/main.cpp)
#all:
#	@echo "Write this Makefile by your self."
#sim:
#	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
#	verilator -Wno-fatal $(VSRC) $(CSRC) --top-module top --cc --trace --exe
#	make -C obj_dir -f Vtop.mk Vtop
#	./obj_dir/Vtop
#	#gtkwave wave.vcd &
#
# .PHONY:clean
#clean:
#	rm -rf obj_dir wave.vcd
#include ../Makefile


# 基本配置
TOPNAME = top
NXDC_FILES = constr/top.nxdc
INC_PATH ?=

# 项目路径
BUILD_DIR = ./build
OBJ_DIR = $(BUILD_DIR)/obj_dir
BIN = $(BUILD_DIR)/$(TOPNAME)

# Verilator配置
VERILATOR = verilator
VERILATOR_CFLAGS += -MMD --build --trace -cc -O3 --x-assign fast --x-initial fast --noassert

# 文件路径
VSRCS = $(shell find $(abspath ./vsrc) -name "*.v")
CSRCS = $(shell find $(abspath ./csrc) -name "*.c" -or -name "*.cc" -or -name "*.cpp")
SRC_AUTO_BIND = $(abspath $(BUILD_DIR)/auto_bind.cpp)

# NVBoard配置
include $(NVBOARD_HOME)/scripts/nvboard.mk

# 生成自动绑定文件
$(SRC_AUTO_BIND): $(NXDC_FILES)
	python3 $(NVBOARD_HOME)/scripts/auto_pin_bind.py $^ $@

# 编译规则
CSRCS += $(SRC_AUTO_BIND)
INCFLAGS = $(addprefix -I, $(INC_PATH))
CXXFLAGS += $(INCFLAGS) -DTOP_NAME="\"V$(TOPNAME)\""
LDFLAGS += -lSDL2 -lSDL2_image

# 主要构建目标
$(shell mkdir -p $(BUILD_DIR))

$(BIN): $(VSRCS) $(CSRCS) $(NVBOARD_ARCHIVE)
	@rm -rf $(OBJ_DIR) wave.vcd
	$(VERILATOR) $(VERILATOR_CFLAGS) \
		--top-module $(TOPNAME) $^ \
		$(addprefix -CFLAGS , $(CXXFLAGS)) $(addprefix -LDFLAGS , $(LDFLAGS)) \
		--Mdir $(OBJ_DIR) --exe -o $(abspath $(BIN))

# 兼容旧Makefile的目标
sim: $(BIN)
	$(call git_commit, "sim RTL")
	./$(BIN) +trace

# 其他目标
all: $(BIN)

run: $(BIN)
	./$(BIN) +trace

clean:
	rm -rf $(BUILD_DIR) wave.vcd obj_dir

.PHONY: all clean sim run
