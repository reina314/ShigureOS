#include <stdio.h>

#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/tty.h>

void kernel_main(void)
{
	gdt_install();
	idt_install();
	terminal_initialize();
}
