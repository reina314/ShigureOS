#ifndef _KERNEL_PROC_H
#define _KERNEL_PROC_H

#include <stdlib.h>
#include <kernel/vm.h>
#include "regs.h"

typedef struct proc
{
    int id;               // Process ID
    uint32_t esp, ebp;    // Starck and Base pointer
    uint32_t eip;         // Instruction pointer
    page_directory_t *pd; // Page directory
    struct proc *next;    // Next process (linked list)
} proc_t;

void proc_initialize(void);
void switch_proc(struct regs *r);
int fork(void);
void move_stack(void *new_stack_start, size_t size);
int getpid(void);

#endif