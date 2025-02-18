#ifndef _KERNEL_IDT_H
#define _KERNEL_IDT_H

void idt_install(void);
void idt_set_descriptor(unsigned char index, unsigned long base, unsigned short selector, unsigned char flags);

#endif