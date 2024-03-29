/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
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

typedef struct watchpoint {
  int NO; //number of watchpoint
  struct watchpoint *next;
  char expr[100];
  word_t value;

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
  if (free_== NULL) {
    assert(0);
  }
  WP* new = free_;
  free_ = free_ -> next;
  new -> next = head;
  head = new;
  return new;
}

void free_wp(WP *wp) {
  WP *h = head;
  if (h == wp) {
    head = NULL;
  } else {
    while (h && h -> next != wp) {
      h = h -> next;
    }
    if (h==NULL) {
      printf("Delete Error: watchpoint not found\n");
      assert(0);
    }
    h -> next = wp -> next;
  }
  wp -> next = free_;
  free_ = wp;
} 
/* TODO: Implement the functionality of watchpoint */
void wp_watch(char *expr, word_t ans) {
  WP* wp = new_wp();
  strcpy(wp->expr, expr);
  wp->value = ans;
  printf("Watchpoint %d: %s\n",wp->NO, expr);
}

void wp_delete(int no) {
  assert(no < NR_WP);
  WP* wp = &wp_pool[no];
  free_wp(wp);
  printf("Delete watchpoint %d: %s\n",wp->NO, wp->expr);
}

void sdb_watchpoint_display() {
  WP* h = head;
  if(h == NULL) {
    printf("No watchpoints.\n");
    return;
  }
  printf("%-8s%-8s\n","NUM","What");
  while(h) {
    printf("%-8d%-8s\n", h->NO,h->expr);
    h = h -> next;
  }
}

int wp_checkdiff() {
  int flag = 0;
  WP* tmp = head;
  while(tmp) {
    if(tmp -> value != expr(tmp->expr, NULL)) {
      flag = 1;
      word_t new_value = expr(tmp->expr, NULL);
      printf("Watchpoint NO.%d: %s old_value:%u new_value:%u\n",tmp->NO,tmp->expr,tmp->value,new_value);
      tmp -> value = new_value;
    }
  }
  return flag;
}