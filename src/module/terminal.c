#include "module/terminal.h"
#include "core/scheduler.h"
#include "bsp/uart.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#define TERM_LINE_MAX      96
#define TERM_ARGC_MAX      8
#define TERM_PROMPT        "\r\n> "

static char        s_line[TERM_LINE_MAX];
static uint16_t    s_len = 0;
static term_cmd_t *s_head = NULL;   /* 命令链表 */

void term_printf(const char *fmt, ...)
{
    char buf[128];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n > 0) UART_SendBytes((uint8_t *)buf, (uint16_t)n);
}

void term_register(term_cmd_t *cmd)
{
    if (!cmd || !cmd->name || !cmd->fn) return;
    cmd->next = s_head;
    s_head = cmd;
}

/* ---------- 内建命令 ---------- */

static int cmd_help(int argc, char **argv)
{
    (void)argc; (void)argv;
    term_printf("commands:\r\n");
    for (term_cmd_t *c = s_head; c; c = c->next) {
        term_printf("  %-20s %s\r\n", c->name, c->help ? c->help : "");
    }
    return 0;
}
static term_cmd_t s_cmd_help = { "help", cmd_help, "list commands", NULL };

static int cmd_task_list(int argc, char **argv)
{
    (void)argc; (void)argv;
    term_printf("idx name       period  en   runs      max_us\r\n");
    for (uint8_t i = 0; i < sched_count(); ++i) {
        const sched_task_t *t = sched_iter(i);
        term_printf(" %2u  %-10s %5lu   %c  %8lu  %8lu\r\n",
                    i, t->name,
                    (unsigned long)t->period_ms,
                    t->enabled ? 'y' : 'n',
                    (unsigned long)t->run_count,
                    (unsigned long)t->max_us);
    }
    return 0;
}
static term_cmd_t s_cmd_task_list = { "task.list", cmd_task_list, "show tasks + stats", NULL };

static int cmd_task_enable(int argc, char **argv)
{
    if (argc < 2) { term_printf("usage: task.enable <name>\r\n"); return -1; }
    return sched_set_enabled(argv[1], true) ? 0 : -1;
}
static term_cmd_t s_cmd_task_enable = { "task.enable", cmd_task_enable, "enable a task", NULL };

static int cmd_task_disable(int argc, char **argv)
{
    if (argc < 2) { term_printf("usage: task.disable <name>\r\n"); return -1; }
    return sched_set_enabled(argv[1], false) ? 0 : -1;
}
static term_cmd_t s_cmd_task_disable = { "task.disable", cmd_task_disable, "disable a task", NULL };

static int cmd_task_stats_reset(int argc, char **argv)
{
    (void)argc; (void)argv;
    sched_reset_stats();
    return 0;
}
static term_cmd_t s_cmd_task_stats_reset = { "task.stats.reset", cmd_task_stats_reset, "clear run counters", NULL };

/* ---------- 主逻辑 ---------- */

void terminal_init(void)
{
    s_len = 0;
    s_head = NULL;
    term_register(&s_cmd_help);
    term_register(&s_cmd_task_list);
    term_register(&s_cmd_task_enable);
    term_register(&s_cmd_task_disable);
    term_register(&s_cmd_task_stats_reset);
    term_printf("\r\n=== STM32 App Framework Terminal ===\r\ntype 'help'" TERM_PROMPT);
}

static void dispatch(char *line)
{
    /* 空行忽略 */
    while (*line == ' ') line++;
    if (*line == '\0') return;

    char *argv[TERM_ARGC_MAX];
    int   argc = 0;
    char *p = line;
    while (*p && argc < TERM_ARGC_MAX) {
        while (*p == ' ') *p++ = '\0';
        if (*p == '\0') break;
        argv[argc++] = p;
        while (*p && *p != ' ') p++;
    }
    if (argc == 0) return;

    for (term_cmd_t *c = s_head; c; c = c->next) {
        if (strcmp(c->name, argv[0]) == 0) {
            int rc = c->fn(argc, argv);
            if (rc != 0) term_printf("(rc=%d)\r\n", rc);
            return;
        }
    }
    term_printf("unknown: %s (try 'help')\r\n", argv[0]);
}

void terminal_task(void)
{
    uint8_t b;
    while (UART1_RxPop(&b)) {
        if (b == '\r' || b == '\n') {
            if (s_len > 0) {
                s_line[s_len] = '\0';
                UART_SendString("\r\n");
                dispatch(s_line);
                s_len = 0;
            }
            UART_SendString(TERM_PROMPT);
        } else if (b == 0x08 || b == 0x7F) {   /* Backspace / DEL */
            if (s_len > 0) {
                s_len--;
                UART_SendString("\b \b");
            }
        } else if (b >= 0x20 && b < 0x7F && s_len < TERM_LINE_MAX - 1) {
            s_line[s_len++] = (char)b;
            UART_SendByte(b);      /* 回显 */
        }
    }
}
