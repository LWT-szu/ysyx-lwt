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
***************************************************************************************/

#include "sdb.h"

#define NR_WP 32
// 监视点结构体
typedef struct watchpoint {
  int NO;
  struct watchpoint *next;          // 指向下一个监视点的指针
  char expr[4096];                  // 要监视的表达式字符串
  uint32_t last_val;

  /* TODO: Add more members if necessary */
  
} WP;


static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);
    
  }

  head = NULL;     
  free_ = wp_pool; 
}

WP* new_wp() {
  if(free_ == NULL){
    assert(0 && "No free watchpoint availabe!");
  }
  WP *wp = free_;      // wp指针指向空闲链表的第一个元素
  free_ = free_->next; // free_指向下一个空闲监视点

  wp->next = head; 
  head = wp;       

  return wp; 
}


void free_wp(WP *wp){
  if(head == wp){
    head = wp->next;
  }
  else {
    WP *prev = head;
    // 遍历链表，直到prev->next等于wp,prev 不为 NULL
    while (prev && prev->next != wp)
    { 
      prev = prev->next;
    }
    if(prev)
    {
      // 让前驱节点的next指针跳过wp，指向wp的下一个节点!!!
      prev->next = wp->next;
    }
  }
  // 将wp插入到空闲链表free_的头部
  wp->next = free_; 
  free_ = wp;       
}

/* TODO: Implement the functionality of watchpoint */
//set watchpoint,return NO
int set_watchpoint(const char *expr_str)
{
  WP *wp = new_wp(); 

  // 将表达式字符串拷贝到监视点结构体中，确保不会越界
  strncpy(wp->expr, expr_str, sizeof(wp->expr) - 1);
  wp->expr[sizeof(wp->expr) - 1] = '\0';

  bool success = true;
  //比对
  uint32_t val = expr((char *)expr_str, &success);
  wp->last_val = val;
  printf("Set watchpoint %d: %s (init value: 0x%x)\n", wp->NO, wp->expr, val);
  return wp->NO;
}

// 删除监视点
bool delete_watchpoint(int NO)
{
  WP *wp = head;
  
  while (wp != NULL)
  {
    if (wp->NO == NO)
    {
      free_wp(wp);
      printf("Watchpoint %d deleted.\n", NO);
      return true;
    }
    wp = wp->next;
  }
  printf("No such watchpoint: %d\n", NO);
  return false;
}


// 打印所有监视点
void info_watchpoints()
{
  WP *wp = head;
  printf("Num\tWhat\t\tValue\n");
  while (wp != NULL)
  {
    printf("%d\t%s\t0x%x\n", wp->NO, wp->expr, wp->last_val);
    wp = wp->next;
  }
}

void check_watchpoints()
{
  WP *wp = head;
  bool first = true;//一次循环内只设置一次NEMU_STOP
  while (wp != NULL)
  {
    bool success = true;
    uint32_t new_val = expr(wp->expr, &success);
    /*printf("[DEBUG] wp expr=%s last_val=0x%x new_val=0x%x success=%d\n",
           wp->expr, wp->last_val, new_val, success);*/
    if (!success)
    {
      printf("Failed to evaluate expression for watchpoint %d\n", wp->NO);
      wp = wp->next;
      continue;
    }
    if (new_val != wp->last_val)
    {
      if(first){
        nemu_state.state = NEMU_STOP;
        first = false;
      }
      
      printf("\nWatchpoint %d triggered: %s\n", wp->NO, wp->expr);
      printf("Old value = 0x%x\n", wp->last_val);
      printf("New value = 0x%x\n", new_val);
      wp->last_val = new_val;
      //break; // 只触发第一个
    }
    wp = wp->next;
  }
}











// 让外部能访问head
WP *get_wp_head() { return head; }
