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

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_EQ,
  TK_NUM,
  TK_HEX,
  TK_REG,
  TK_NEG,
  TK_DEREF,

  /* TODO: Add more token types */

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"==", TK_EQ},        // equal
  {"\\-", '-'},
  {"\\*", '*'},
  {"\\/", '/'},
  {"\\(", '('},
  {"\\)", ')'},
  {"0x[0-9A-Fa-f]+", TK_HEX}, //Hex integer
  {"[0-9]+", TK_NUM}, //Decimal integer
  {"\\$[0-9a-z]+", TK_REG},

};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
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

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        tokens[nr_token].type = rules[i].token_type;
        switch (rules[i].token_type) {
          case TK_NUM:
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
            break;
          case TK_HEX:
            strncpy(tokens[nr_token].str, substr_start+2, substr_len-2);
            tokens[nr_token].str[substr_len-2] = '\0';
            break;
          case TK_REG:
            strncpy(tokens[nr_token].str, substr_start+1, substr_len-1);
            tokens[nr_token].str[substr_len-1] = '\0';
            break;
        }
        nr_token++;
        break;
      }
    }

    for(int j = 0; j <nr_token; j++) {
      if(tokens[j].type == '-' && ( (j == 0) || (tokens[j-1].type !=')' && tokens[j-1].type != TK_NUM && tokens[j-1].type != TK_HEX && tokens[j-1].type != TK_REG))) {
        tokens[j].type = TK_NEG;
      }
      if(tokens[j].type == '-' && ( (j == 0) || (tokens[j-1].type !=')' && tokens[j-1].type != TK_NUM && tokens[j-1].type != TK_HEX && tokens[j-1].type != TK_REG))) {
        tokens[j].type = TK_DEREF;
      }

    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

bool check_parentheses( int p, int q ) {
  if (tokens[p].type != '(' || tokens[q].type != ')')
    return false;
  int i, cnt = 0;
  for (i = p; i <= q; i++) {
      if (tokens[i].type == '(') {
      cnt++;
      } else if (tokens[i].type == ')') {
      cnt--;
      }
      if (cnt == 0 && i < q) {
      return false;
      }
      if (cnt < 0) {
        return false;
      }
  }
    return cnt == 0;
}


int get_op( int p, int q ) {
  int par_cnt = 0;
  int op = -1;
  int lowest_pre = 10;
  for( int i = p; i <= q; i++) {
    if(tokens[i].type == '(') {
      par_cnt++;
      continue;
    } else if (tokens[i].type == ')') {
      par_cnt--;
      continue;
    } else if (par_cnt == 0 && (tokens[i].type == '+' || tokens[i].type == '-')) {
      if(lowest_pre >= 1) {
        lowest_pre = 1;
        op = i;
      }
    } else if (par_cnt == 0 && (tokens[i].type == '*' || tokens[i].type == '/')) {
      if(lowest_pre >= 2) {
        lowest_pre = 2;
        op = i;
      }
    }
  }
  return op;
}
word_t eval( int p, int q ) {
  if (p > q) {
    printf("Bad expression\n");
    assert(0);
  } else if (p == q) {
    return atoi(tokens[p].str);
  } else if (check_parentheses(p,q)) {
    return eval(p + 1, q - 1);
  } else {
    int op = get_op(p,q); //todo
    word_t val1 = eval(p, op - 1);
    word_t val2 = eval(op + 1, q);
    switch (tokens[op].type) {
      case '+' : return val1 + val2;
      case '-' : return val1 - val2;
      case '*' : return val1 * val2;
      case '/' : return val1 / val2;
      default: assert(0);
    }
  }
}


word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }
  /* TODO: Insert codes to evaluate the expression. */
  return eval (0,nr_token - 1);

}
