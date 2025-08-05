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
这段代码是 NEMU 调试器（sdb）中表达式解析功能的实现框架，主要用于解析和计算用户输入的表达式
（如内存地址、寄存器值、算术表达式等）。
负责把用户输入的表达式（比如 0x80000000 + $a0 * 2）拆成有意义的小单元（token），并计算表达式的结果。
它是调试器处理用户输入（如内存地址、算术计算）的基础。
***************************************************************************************/

#include <isa.h>
#include <limits.h>
/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
/* enum 枚举（token 类型）：*/
#include <regex.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "memory/vaddr.h"
typedef uint32_t word_t;
enum
{
  TK_NOTYPE = 256,
  TK_EQ,
  TK_NEQ, // !=
  TK_AND, // &&
  TK_ADD,
  TK_SUB,
  TK_START,
  TK_NUM,
  TK_REG,
  TK_Q1,
  TK_Q2,
  TK_DIV,
  TK_DEREF, // 指针解引用
  TK_HEX,
  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */
  {" +", TK_NOTYPE},    //  spaces              
  {"\\+", TK_ADD},         // plus
  {"==", TK_EQ},        // equal
  {"!=", TK_NEQ},      
  {"&&", TK_AND}, 
  {"-",TK_SUB},
  {"\\*",TK_START},
  { "0[xX][0-9a-fA-F]+", TK_HEX },
  {"[0-9]+",TK_NUM},
  {"\\$[a-zA-Z0-9_]+",TK_REG},
  {"\\(",TK_Q1},
  {"\\)",TK_Q2},
  {"\\/",TK_DIV},

};

#define NR_REGEX ARRLEN(rules)

bool check_parentheses(int p, int q);
int find_main(int p, int q);
int32_t eval(int p, int q);

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
/*init_regex() 函数：编译正则规则
作用：程序启动时，把 rules 数组里的正则表达式 “编译” 成计算机能快速识别的格式（类似 “预编译”），
避免每次解析表达式时重复编译，提高效率。
细节：如果正则表达式写错了（比如语法错误），会报错并提示错误原因。*/
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
      
    }
  }
}
/*存储单个 token 的信息：type 是 token 类型，
str 是 token 的原始字符串（如数字 "123"、寄存器 "$a0"）。*/
typedef struct token {
  int type;
  char str[4096];
} Token;

static Token tokens[4096] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;


bool make_token(char *e)
{
  int position = 0;
  int i=0;
  regmatch_t pmatch;

  nr_token = 0; // 记录当前已识别的 token 数量

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;
        // // 日志输出：记录匹配信息（规则索引、正则表达式、位置、长度、匹配内容
        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        if(nr_token< 4095 ){
          switch (rules[i].token_type) {
            case TK_NEQ:
                    tokens[nr_token].type = TK_NEQ;
                    strcpy(tokens[nr_token].str,"!=");
                    nr_token++;
                    break;
            case TK_AND:
                    tokens[nr_token].type = TK_AND;
                    strcpy(tokens[nr_token].str,"&&");
                    nr_token++;
                    break;
            case TK_REG:                        //实现寄存器值处理
                    tokens[nr_token].type = TK_REG;
                    int safe_len_REG = (substr_len > 4095) ? 4095 : substr_len;
                    strncpy(tokens[nr_token].str,substr_start,safe_len_REG);
                    tokens[nr_token].str[safe_len_REG] = '\0';
                    nr_token++;
                    break;
            case TK_ADD: 
                    tokens[nr_token].type = TK_ADD;
                    strcpy(tokens[nr_token].str,"+");
                    nr_token++;
                    break;
            case TK_NOTYPE: 
                    break;
            case TK_EQ: 
                    tokens[nr_token].type = TK_EQ;
                    strcpy(tokens[nr_token].str,"==");
                    nr_token++;
                    break;
            case TK_SUB:
                    tokens[nr_token].type = TK_SUB;
                    strcpy(tokens[nr_token].str,"-");
                    nr_token++;
                    break;
            case TK_NUM:
                    tokens[nr_token].type = TK_NUM;
                    int safe_len = (substr_len > 4095) ? 4095: substr_len;
                    strncpy(tokens[nr_token].str, substr_start, safe_len); // 防止溢出
                    tokens[nr_token].str[safe_len] = '\0'; // 确保字符串终止
                    if (substr_len > 255)
                    {
                      Log("Warning: Number truncated to 255 chars");
                    }
                    nr_token++;
                    break;
            case TK_HEX:
                    tokens[nr_token].type = TK_HEX;
                    int safe_len_hex = (substr_len > 4095) ? 4095 : substr_len;
                    strncpy(tokens[nr_token].str, substr_start,substr_len);
                    tokens[nr_token].str[safe_len_hex] = '\0';
                    nr_token++;
                    break;

            case TK_START:
                    tokens[nr_token].type = TK_START;
                    strcpy(tokens[nr_token].str,"*");
                    nr_token++;
                    break;
            case TK_DIV:
                    tokens[nr_token].type = TK_DIV;
                    strcpy(tokens[nr_token].str,"/");
                    nr_token++;
                    break;
            case TK_Q1:
                    tokens[nr_token].type = TK_Q1;
                    strcpy(tokens[nr_token].str,"(");
                    nr_token++;
                    break;
            case TK_Q2:
                    tokens[nr_token].type = TK_Q2;
                    strcpy(tokens[nr_token].str, ")");
                    nr_token++;
                    break;

            default: 
                    Log("[ERRPR]");
                    return false;
          }
          break;
        }
        else{
          printf("Error: Unknown token type %d\n", rules[i].token_type);
          return false; // 遇到未知token类型直接返回错误
        }
      }
      
    }
  
    
  }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  

  return true;
}

//作用：调用 make_token() 拆分表达式得到 token 后，根据 token 计算最终结果（比如算术运算、地址解析）。
//word_t“机器字长的数据”
word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    printf("Tokenize failed\n");
    *success = false; // tokens 是静态变量必须在 expr() 中完成计算.tokens 的内存仍然存在，但 其他文件无法直接访问它
    return UINT32_MAX; // 只在真正的错误（如除0、溢出、语法错误等）时返回UINT32_MAX。!!!!!!!!!!!
  }
  
  /* TODO: Insert codes to evaluate the expression. */
  // 如果 * 前面没有token，或前一个token是左括号或运算符（如 + - * / && == !=），那么这个 * 就是指针解引用，不是乘法。
  for(int i = 0; i<nr_token;i++){
    if (tokens[i].type == TK_START &&
        (i == 0 ||
         tokens[i - 1].type == TK_ADD ||
         tokens[i - 1].type == TK_SUB ||
         tokens[i - 1].type == TK_START ||
         tokens[i - 1].type == TK_DIV ||
         tokens[i - 1].type == TK_AND ||
         tokens[i - 1].type == TK_EQ ||
         tokens[i - 1].type == TK_NEQ ||
         tokens[i - 1].type == TK_Q1)){
         tokens[i].type = TK_DEREF;
        }
        
  }
  int32_t result = eval(0, nr_token - 1);
  *success = true;        // 标记成功
  
  return result;
 
}


int32_t eval(int p, int q){
  if(p > q){
    printf("please write appoiate expression!\n");
    return 0;
  }

  if(p==q){
    // 十进制
    if(tokens[p].type == TK_NUM)
    {
      unsigned long val = strtoul(tokens[p].str, NULL, 10); // 将字符串 tokens[p].str 按十进制转换为无符号长整型数。
      if (val > UINT32_MAX)
      {
        printf("Number overflow! [%s]\n", tokens[p].str);
        return 0; 
      }
      return (int32_t)val;
    }
    // 十六进制
    else if(tokens[p].type == TK_HEX){
      unsigned long val = strtoul(tokens[p].str,NULL,16);
      if (val > UINT32_MAX)
      {
        printf("Number overflow! [%s]\n", tokens[p].str);
        return 0; 
      }
      return (int32_t)val;
    }
    // 寄存器
    else if(tokens[p].type == TK_REG){
      bool succ = false;
      word_t val = isa_reg_str2val(tokens[p].str + 1 ,&succ);
      if(!succ){
        printf("NO Such Reg:%s\n",tokens[p].str);
        return 0;
      }
      return (int32_t)val;
    }
    else {
      printf("Please add number to your expression!\n");
      return 0;
    }
  }

  if(check_parentheses(p,q)){
    return eval(p+1,q-1);
  }
  

  // 处理解引用
  if(tokens[p].type == TK_DEREF){
    uint32_t addr = eval(p + 1, q); // 先算出 * 后面表达式的值
    return vaddr_read(addr, 4);     // 从该地址读取 4 字节数据
  }

  int op = find_main(p, q);
  if (op == -1)
  {
    return 0;
  }
  int32_t val1 = eval(p,op -1);//把表达式拆成两半，分别递归求值
  int32_t val2 = eval(op + 1, q);
 

  switch(tokens[op].type){
    case TK_EQ:
      return val1 == val2;
    case TK_AND:
      return val1 && val2;
    case TK_NEQ:
      return val1 != val2;
    case TK_ADD :
      return val1 + val2;
    case TK_SUB :
      return val1 - val2;
    case TK_START :
      return val1 * val2;
    case TK_DIV : 
          if(val2 == 0)
          {
            printf("The denominator cannot be zero!\n");
            return UINT32_MAX;
          }
          return val1 / val2;
    default:
      printf(" Expression error : unknown operator type % d at[% d]\n ", tokens[op].type, __LINE__);
      return 0;
  }

}
//判断从 p 到 q 的表达式是否被一对匹配的括号完整包裹”
bool check_parentheses(int p, int q){
  if(tokens[p].type != TK_Q1 || tokens[q].type != TK_Q2) return false;
  int balance = 0,i=0;
  for(i=p;i<=q;i++)
  {
    if(tokens[i].type == TK_Q1) balance ++;
    else if(tokens[i].type == TK_Q2)
    {
      balance--;
      if(balance == 0 && i<q) return false;
      if(balance < 0) return false;
    }
  }
  return (balance == 0);
}

// 扫描所有运算符，定位主运算符
int find_main(int p, int q) {
    int i, count = 0, prio_min = 4; // prio_min 初始值设为最大（比所有运算符优先级高）
    int op = -1;
    //优先级最低且最右的那个运算符的下标
    for (i = p; i <= q; i++) {
        if (tokens[i].type == TK_Q1) { count++; continue; }
        if (tokens[i].type == TK_Q2) { count--; continue; }
        if (count != 0) continue; // 跳过括号内的 token

        // 识别运算符并计算优先级
        int prio = 4; // 优先级最低
        if (tokens[i].type == TK_AND) prio = 0;
        else if (tokens[i].type == TK_EQ || tokens[i].type == TK_NEQ) prio = 1;
        else if (tokens[i].type == TK_ADD || tokens[i].type == TK_SUB) prio = 2;
        else if (tokens[i].type == TK_START || tokens[i].type == TK_DIV) prio = 3;

        // 更新主运算符：优先级最低且最右
        // 同优先级但更靠右（后面覆盖前面）
        if (prio <= prio_min) {
            prio_min = prio;
            op = i;
        }
    }
    if (op == -1){
      printf("Please write appoiate expression!!!\n");
      return -1;
    } // 无运算符（比如纯数字或括号不匹配）
    return op;
}