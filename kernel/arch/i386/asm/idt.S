.section .text
.global idt_load
.extern idtp
.type idt_load, @function
idt_load:
    lidt idtp   # Load new IDT
    sti         # Set the interrupt flag
    ret