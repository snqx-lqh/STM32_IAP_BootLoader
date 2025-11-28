/**
  ******************************************************************************
  * @file    MiniShell.h
  * @author  少年潜行(snqx-lgh)
  * @version V 1.0
  * @date    2025/11/15
  * @brief   
  ******************************************************************************
  * @attention
  *
  *
  * <h2><center>&copy; Copyright {Year} LQH,China</center></h2>
  ******************************************************************************
  */

#ifndef _MINI_SHELL_H
#define _MINI_SHELL_H

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define USER_NAME        "@STM32:"
#define SHELL_BUF_SIZE    64
#define HIST_MAX          10              // 最大历史数据条数


typedef struct
{
    const char *name;
    void (*func)(int argc, char **argv);
    const char *help;
} shell_cmd_t;

void mini_cli_loop(void);
 

#endif
