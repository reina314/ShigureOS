#include <stdio.h>

#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/isr.h>
#include <kernel/irq.h>
#include <kernel/timer.h>
#include <kernel/serial.h>
#include <kernel/tty.h>
#include <kernel/kb.h>

int kernel_main(void)
{
	terminal_initialize();
	gdt_install();
	idt_install();
	isr_install();
	irq_install();
	serial_initialize(SERIAL_COM1_BASE);
	timer_install();
	keyboard_install();

	asm volatile("sti");
	__asm__ volatile("sti"); // Enable interrupts

	for (;;)
		;

	return 0;
}
