#include <kernel/sh.h>
#include <kernel/fs.h>
#include <kernel/proc.h>
#include <kernel/kb.h>
#include <kernel/tty.h>

#include <stdio.h> // For printf()
#include <stdint.h>
#include <string.h>

static char command_buf[CMD_BUF_SIZE];
static int buf_location = 0;

static char history[HISTORY_SIZE][CMD_BUF_SIZE];
static int history_index = 0;
static int history_count = 0;

// External refs
extern int seconds_passed; // Uptime; defined in timer.c
extern uint32_t pm_start;
extern int pm_amount;
extern fs_node_t *version_node;

// Command table
shell_cmd_t commands[] = {
    {"clear", cmd_clear},
    {"uptime", cmd_uptime},
    // {"help", cmd_help},
    {"mem", cmd_mem},
    // {"ls", cmd_ls},
    {"version", cmd_version},
    {"getpid", cmd_getpid},
    {"logo", cmd_logo},
    {NULL, cmd_unknown} // Default handler for unknown commands
};

/// @brief Display the she;; prompt
void output_prompt(void)
{
    printf("\n$ ");
}

/// @brief Wait for user input
void wait_for_command(void)
{
    buf_location = 0;
    memset(command_buf, 0, CMD_BUF_SIZE);

    char c;
    while (1)
    {
        c = keyboard_getchar(); // Blocking input
        if (c == '\n')
        {
            command_buf[buf_location] = '\0';
            buf_location = 0;
            terminal_newline();
            break;
        }
        else if (c == '\b')
        {
            if (buf_location > 0)
            {
                buf_location--;
                terminal_backspace();
            }
        }
        else
        {
            if (buf_location < CMD_BUF_SIZE - 1)
            {
                command_buf[buf_location++] = c;
                terminal_putchar(c);
            }
        }
    }

    // Store in history
    if (buf_location > 0)
    {
        strcpy(history[history_index], command_buf);
        history_index = (history_index + 1) % HISTORY_SIZE;
        if (history_count < HISTORY_SIZE)
            history_index++;
    }
}

/// @brief Process the entered command
void process_command(void)
{
    for (int i = 0; commands[i].name != NULL; i++)
    {
        if (strcmp(commands[i].name, command_buf) == 0)
        {
            commands[i].handler();
            return;
        }
    }

    cmd_unknown();
}

/// @brief Start shell
void shell_initialize(void)
{
    while (1)
    {
        output_prompt();
        wait_for_command();
        process_command();
    }
}

void cmd_clear(void)
{
    terminal_clearscreen();
}

void cmd_uptime(void)
{
    printf("Uptime: %d sec\n", seconds_passed);
}

void cmd_mem(void)
{
    printf("Memory start address : %x\n", pm_start);
    printf("Memory avail amount  : %x\n", pm_amount);
}

void cmd_version(void)
{
    char *buffer = (char *)kmalloc(sizeof(char) * version_node->length);
    memset(buffer, 0, version_node->length + 1);
    read_fs(version_node, 0, version_node->length, (uint8_t *)buffer);
    printf("OS version : Shigure OS %s\n", buffer);
    printf("Developed by reina314\n");
    kfree(buffer);
}

void cmd_getpid(void)
{
    printf("PID : %d\n", getpid());
}

void cmd_logo(void)
{
    terminal_showlogo();
}

void cmd_unknown(void)
{
    printf("Command not found: %s\n", command_buf);
}