/***************************************************************************************
* Copyright (c) 2020-2021 Institute of Computing Technology, Chinese Academy of Sciences
* Copyright (c) 2020-2021 Peng Cheng Laboratory
*
* XiangShan is licensed under Mulan PSL v2.
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

#include "common.h"
#include "stdlib.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <termios.h>

#define UART_INTERACTIVE

#define QUEUE_SIZE 1024
static char queue[QUEUE_SIZE] = {};
static int f = 0, r = 0;

static void uart_enqueue(char ch) {
  int next = (r + 1) % QUEUE_SIZE;
  if (next != f) {
    // not full
    queue[r] = ch;
    r = next;
  } else {
    printf("UART queue full, dropping character: %ch\n", ch);
  }
}

static uint8_t uart_dequeue(void) {
  uint8_t k = 0xff;
  if (f != r) {
    k = queue[f];
    f = (f + 1) % QUEUE_SIZE;
  }
  return k;
}

#ifdef UART_INTERACTIVE
struct termios saved_attributes;

void *read_input(void *arg) {
  char c;
  while (1) {
    // set terminal attributes to non-blocking mode
    struct termios new_attributes = saved_attributes;
    new_attributes.c_cc[VMIN] = 0;
    new_attributes.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_attributes);

    // read a character from stdin
    if (read(STDIN_FILENO, &c, 1) > 0) {
      uart_enqueue(c);
    }

    // restore terminal attributes
    tcsetattr(STDIN_FILENO, TCSANOW, &saved_attributes);
  }
}
#endif

uint8_t uart_getc() {
  uint8_t ch = uart_dequeue();
  // printf("uart_getc %c (%d)\n", ch, ch);
  return ch;
}

static void preset_input() {
  char rtthread_cmd[128] = "memtrace\n";
  char init_cmd[128] = "2" // choose PAL
    "jjjjjjjkkkkkk" // walk to enemy
    ;
  char busybox_cmd[128] =
    "ls\n"
    "echo 123\n"
    "cd /root/benchmark\n"
    "ls\n"
    "./stream\n"
    "ls\n"
    "cd /root/redis\n"
    "ls\n"
    "ifconfig -a\n"
    "./redis-server\n";
  char debian_cmd[128] = "root\n";
  char *buf = debian_cmd;
  int i;
  for (i = 0; i < strlen(buf); i ++) {
    uart_enqueue(buf[i]);
  }
}

void init_uart(void) {
#ifdef UART_INTERACTIVE
  pthread_t thread;

  // save current terminal attributes
  tcgetattr(STDIN_FILENO, &saved_attributes);

  // create thread to read input
  pthread_create(&thread, NULL, read_input, NULL);
#else
  preset_input();
#endif
}
