/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
对外提供表达式解析功能的接口。在 NEMU 调试器中，当用户输入涉及数值计算的命令
（例如设置断点时指定地址、查看某个内存地址的值等），就需要通过 expr 函数来解析这些表达式并计算结果。
实际的解析逻辑会在对应的 .c 文件（如 sdb.c）中实现。
简单来说，这是调试器中表达式解析模块的 “接口声明”，让其他代码可以调用这个功能而无需关心具体实现细节。
***************************************************************************************/

#ifndef __SDB_H__
#define __SDB_H__

#include <common.h>

word_t expr(char *e, bool *success);

#endif

