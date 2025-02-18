#include <stdio.h>

#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/isr.h>
#include <kernel/irq.h>
#include <kernel/tty.h>

void kernel_main(void)
{
	gdt_install();
	idt_install();
	isr_install();
	irq_install();
	terminal_initialize();
}
