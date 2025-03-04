#ifndef _KERNEL_SHELL_H
#define _KERNEL_SHELL_H

#define CMD_BUF_SIZE 256
#define HISTORY_SIZE 8

#include <stddef.h>

/* Function prototypes */
void shell_initialize(void);
void output_prompt(void);
void wait_for_command(void);
void process_command(void);

/* Command structure */
typedef struct shell_cmd
{
    const char *name;
    void (*handler)(void);
} shell_cmd_t;

/* Function prototypes for commands */
void cmd_clear(void);
void cmd_uptime(void);
// void cmd_help(void);
void cmd_mem(void);
// void cmd_ls(void);
void cmd_version(void);
void cmd_getpid(void);
void cmd_logo(void);
void cmd_unknown(void);

#endif