#ifndef __MODULE_TERMINAL_H
#define __MODULE_TERMINAL_H

#include <stdint.h>

/*
 * 串口命令行（USART1，115200，PA9/PA10）
 *
 * 用法：
 *   1. 主初始化里调用 terminal_init()
 *   2. 各模块自己 term_register(&cmd) 注册命令
 *   3. 主循环里通过调度器周期性跑 terminal_task()
 *
 * 命令风格：点号命名，如
 *     help
 *     task.list
 *     task.disable ui
 *     fft.thd
 *     adc.raw 0
 */

typedef int (*term_cmd_fn_t)(int argc, char **argv);

typedef struct term_cmd_s {
    const char        *name;       /* 命令字，例如 "task.list"    */
    term_cmd_fn_t      fn;
    const char        *help;       /* 一行帮助，用于 help 输出   */
    struct term_cmd_s *next;       /* 内部链表串联               */
} term_cmd_t;

void terminal_init(void);
void terminal_task(void);          /* 注册到调度器 */
void term_register(term_cmd_t *cmd);
void term_printf(const char *fmt, ...);

#endif /* __MODULE_TERMINAL_H */
