#ifndef _KERNEL_IRQ_H
#define _KERNEL_IRQ_H

#include <regs.h>

void irq_install(void);
typedef void (*irqhandler)(struct regs *r);
void register_request_handler(int vector, irqhandler callback);

#endif