/* ShigureOS - By reina314 */

#include <kernel/multiboot.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/isr.h>
#include <kernel/irq.h>
#include <kernel/timer.h>
#include <kernel/serial.h>
#include <kernel/tty.h>
#include <kernel/kb.h>
#include <kernel/pm.h>
#include <kernel/vm.h>

#include <stdio.h>
#include <stdint.h>

// for debug
// #define CHECK_FLAG(flags, bit) ((flags) & (1 << (bit)))

multiboot_info_t *mbt; // Used in mm.c
uint32_t initrd_location;
unsigned int initial_esp;

int kernel_main(multiboot_info_t *mbtt, unsigned int magic, unsigned int initial_stack)
{
	/* Make sure the magic number matches for memory mapping*/
	if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
	{
		PANIC("Invalid magic number\n");
		return 1;
	}

	mbt = mbtt; // Accessed in mm.c
	initrd_location = *((uint32_t *)mbt->mods_addr);
	initial_esp = initial_stack;

	/* Check bit 6 to see if we have a valid memory map */
	if (!(mbt->flags >> 6 & 0x1))
	{
		PANIC("Invalid memory map given by GRUB bootloader");
		return 1;
	}

	gdt_install();
	idt_install();
	isr_install();
	irq_install();
	serial_initialize(SERIAL_COM1_BASE);
	terminal_initialize();
	timer_install();
	keyboard_install();
	pm_initialize(initrd_location);
	vm_initialize();

	// Refer to https://www.gnu.org/software/grub/manual/multiboot/multiboot.html#Example-boot-loader-code about multiboot info

	printf("\nMultiboot magic   : %x\n", magic);
	printf("Multiboot mods    : %d\n", mbt->mods_count); // Must be after terminal_init
	printf("Initial stack     : %x\n", initial_esp);
	printf("Initrd start      : %x\n", (unsigned int)initrd_location); // Currently broken! Fix this!

	printf("\n##### OS successfully booted! #####\n");

	asm volatile("sti");
	__asm__ volatile("sti"); // Enable interrupts

	for (;;)
		;

	return 0;
}
