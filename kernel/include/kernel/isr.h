#ifndef _KERNEL_ISR_H
#define _KERNEL_ISR_H

#include <regs.h>

void isr_install(void);
typedef void (*handler)(struct regs *r);
void register_interrupt_handler(int vector, handler callback);

#endif 