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
这实现了 NEMU 调试器的基础框架，提供了命令输入、解析和执行的核心功能，让用户可以通过命令与模拟器交互，控制程序的运行和调试。
后续可以通过扩展cmd_table添加更多调试功能，如设置断点、查看寄存器 / 内存、单步执行等。
这部分代码负责把用户输入的表达式 "翻译" 成计算机能理解的形式并计算结果，是调试器处理数值输入的核心模块。
***************************************************************************************/

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include <stdlib.h>
#include "memory/paddr.h"
#include <regex.h>
// 注意引入读取寄存器数值的文件paddr.h
static int is_batch_mode = false;
extern bool make_token(char *e);
extern int set_watchpoint(const char *expr_str);
extern bool delete_watchpoint(int NO);
extern void info_watchpoints(void);

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  nemu_state.state = NEMU_QUIT;
  return -2;
}

static int cmd_si(char *args)
{
  int step = 1; // 默认执行1步
  if (args != NULL && args[0] != '\0')
  {
    step = atoi(args);
  }
  cpu_exec(step);
  return 0;
}

static int cmd_info_reg(char *args)
{
  if (strcmp(args, "r") == 0)
  {
    isa_reg_display(); // 打印寄存器
  }
  else if (strcmp(args, "w") == 0)
  {
    info_watchpoints();
  }
  else
  {
    printf("Unknown info type: %s\n", args);
  }
  return 0;
}



//这个实现的是，从客户程序读入 的内存中，读取地址和内存并输出
static int cmd_x(char *args){
  // 使用strtok拆分命令参数
  char *len_str = strtok (args," ");
  char *addr_str = strtok (NULL," ");
  // 长度转 int，地址转 paddr_t,
  int len = atoi(len_str);
  paddr_t start_addr = strtoul(addr_str, NULL, 0); 
  // paddr_t 是物理地址类型.strtoul()将字符串转换为无符号长整数
  printf("address\t\tdata(16,0x)\n");
  int i;
  for(i=0;i<len;i++)
  {
    paddr_t curr_addr = start_addr + i*sizeof(word_t);
    // 从当前物理地址读取一个word长度的数据
    word_t data = paddr_read(curr_addr,sizeof(word_t));
    // 格式化输出：地址以8位十六进制显示，数据以8位十六进制显示
    printf("0x%08x:\t0x%08x\n",curr_addr,data);
  }
  return 0;
}


static int cmd_p(char *args){
  bool success;
  uint32_t result = expr(args, &success);
  if (success)
  {
    printf("%u\n", result); 
  }
  return 0;
}

static int main_expr(char *args){
  //Add your file"input" to compare with your result in the expr.c
  FILE *fp = fopen("input", "r");
    if (!fp) {
        printf("Cannot open input file!\n");
        return -1;
    }
    char line[65536];
    int line_num = 0, pass_cnt = 0, fail_cnt = 0;
    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        // 跳过空行
        if (line[0] == '\n' || line[0] == '\0') continue;
        uint32_t expected = 0;
        char expr_buf[65536];
        int n = sscanf(line, "%u %[^\n]", &expected, expr_buf);
        if (n != 2) {
            printf("Line %d format error: %s", line_num, line);
            fail_cnt++;
            continue;
        }
        bool success = false;
        uint32_t actual = expr(expr_buf, &success);
        if (!success) {
            printf("Line %d EXPR ERROR: %s\n", line_num, expr_buf);
            fail_cnt++;
            continue;
        }
        if ((uint32_t)actual == expected)
        {
          // printf("Line %d PASSED: %s = %llu\n", line_num, expr_buf, expected);
          pass_cnt++;
        }
        else
        {
          printf("Line %d WRONG: %s\n  expect: %u\n  actual: %u\n",
                 line_num, expr_buf, expected, actual);
          fail_cnt++;
        }
    }
    fclose(fp);
    printf("Test finished. PASS: %d, FAIL: %d\n", pass_cnt, fail_cnt);
    return 0;
}

// 设置监视点
static int cmd_w(char *args){
  if((args == NULL || args[0] == '\0')){
    printf("Usage:w EXRP\n");
    return 0;
  }
  set_watchpoint(args);
  return 0;
}

// 删除监视点
static int cmd_d(char *args){
  if ((args == NULL || args[0] == '\0'))
  {
    printf("Usage:d N\n");
    return 0;
  }
  int NO = atoi(args);
  delete_watchpoint(NO);
  return 0;
}

static int cmd_help(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);//// 要求：参数 char*，返回 int
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "si","Step execute N instructions (default 1 step)",cmd_si},
  { "info","print reg to displat content", cmd_info_reg},
  { "x","Scan memory: x [长度] [起始地址]",cmd_x},
  { "p","Evaluate expression",cmd_p},
  { "expr", "Compare expr.c with input file", main_expr },
  { "w","Set a watchpoint: w EXPR",cmd_w},
  { "d","Delete a watchpoint: w EXPR",cmd_d},
  { "q", "Exit NEMU", cmd_q },
  /* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

// 接收并拆分命令
void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }
  // 主循环：读取输入并处理
  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    /*strtok：在 sdb_mainloop 里，
    把输入的命令拆成两部分：cmd（命令名，如 x ）和 args（参数，如 10 0x80000000 ）*/
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();
  

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
