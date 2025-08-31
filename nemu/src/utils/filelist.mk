#***************************************************************************************
# Copyright (c) 2014-2024 Zihao Yu, Nanjing University
#
# NEMU is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#          http://license.coscl.org.cn/MulanPSL2
#
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
# EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
# MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
#
# See the Mulan PSL v2 for more details.
#**************************************************************************************/
#检查是否启用了 itrace/iqueue
ifeq ($(CONFIG_ITRACE)$(CONFIG_IQUEUE),)
#如果没有启用,就把 src/utils/disasm.c 排除出编译(加入黑名单)
SRCS-BLACKLIST-y += src/utils/disasm.c

#如果启用了 , 则配置 Capstone 依赖
else
#capstone 编译生成的动态库文件
LIBCAPSTONE = tools/capstone/repo/libcapstone.so.5

#编译时加上 capstone 的头文件路径
CFLAGS += -I tools/capstone/repo/include

#声明：src/utils/disasm.c 依赖于 libcapstone.so.5，即 capstone 库必须先生成
src/utils/disasm.c: $(LIBCAPSTONE)

#如果 capstone 库不存在，就先进入 tools/capstone 目录，自动调用 make，编译 capstone 库
$(LIBCAPSTONE):
	$(MAKE) -C tools/capstone
endif
