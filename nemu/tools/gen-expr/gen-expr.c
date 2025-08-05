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
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the Mulan PSL v2 for more details.
 ***************************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>


static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
    "#include <stdio.h>\n"
    "int main() { "
    "  unsigned result = %s; "
    "  printf(\"%%u\", result); "
    "  return 0; "
    "}";

// 生成 [0, n) 之间的随机数
uint32_t choose(uint32_t n)
{
  return rand() % n;
}

// 向buf加一个字符
static void gen(char c)
{
  int len = snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%c", c);
  assert(len > 0);
}

// 生成随机运算符（+ - * /），运算符前后有概率插入空格
static void gen_rand_op()
{
  char ops[4] = {'+', '-', '*', '/'};
  int op = choose(4);
  if (choose(2))
  {
    gen(' ');
  }
  int len = snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%c", ops[op]);
  assert(len > 0);
  if (choose(2))
  {
    gen(' ');
  }
}

// 生成随机数字（0~99），数字前后有概率插入空格
static void gen_num()
{
  uint32_t num = choose(100);
  if (choose(2))
  {
    gen(' ');
  }
  int len = snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%u", num);
  assert(len > 0);
  if (choose(2))
  {
    gen(' ');
  }
}


static void gen_rand_expr()
{
  if (strlen(buf) > 65530) // 防止溢出
  {
    gen_num();
    return;
  }
  switch (choose(3))
  {
  case 0:
    gen_num();
    break;
  case 1:
    gen('(');
    gen_rand_expr();
    gen(')');
    break;
  default:
    gen_rand_expr();
    gen_rand_op();
    gen_rand_expr();
    break;
  }
}


int main(int argc, char *argv[])
{
  memset(buf, 0, sizeof(buf));           // 清零表达式缓冲区
  memset(code_buf, 0, sizeof(code_buf)); // 清零代码缓冲区

  // 初始化随机种子
  int seed = time(0);
  srand(seed);

  // 控制生成表达式的数量（默认为1，可通过命令行参数指定）
  int loop = 1;
  if (argc > 1)
  {
    sscanf(argv[1], "%d", &loop);
  }

  int i;
  for (i = 0; i < loop; i++)
  {
    // 生成表达式，若包含除0则重新生成
    do
    {
      memset(buf, 0, sizeof(buf));
      gen_rand_expr();
    } while (strstr(buf, "/0") != NULL); 

    // 将生成的表达式插入到 C 代码模板
    sprintf(code_buf, code_format, buf);

    // 写入临时 C 源码文件
    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    // 编译生成的 C 源文件
    int ret = system("gcc -Wall -Werror /tmp/.code.c -o /tmp/.expr ");
    if (ret != 0)
      continue;

    // 执行生成的表达式程序，并读取输出结果
    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);
    int result;
    ret = fscanf(fp, "%d", &result);
    pclose(fp);

    // 输出结果和表达式
    printf("%d %s\n", result, buf);
  }
  return 0;
}