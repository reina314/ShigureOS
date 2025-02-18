# Initialize and reload GDT
.section .text
.global gdt_flush
.extern gptr
.type gdt_flush, @function
gdt_flush:
	lgdt gptr

	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss

	ljmp $0x08, $flush2
flush2:
	ret

.size gdt_flush, . - gdt_flush