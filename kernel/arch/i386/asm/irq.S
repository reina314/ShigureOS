.section .text

.macro IRQ num
.global irq\num
.type irq\num, @function
irq\num:
    cli
    pushl $0
    .set irq_vector, \num + 32
    pushl $irq_vector
    jmp irq_common_stub
.endm

.extern irq_handler

IRQ 0
IRQ 1
IRQ 2
IRQ 3
IRQ 4
IRQ 5
IRQ 6
IRQ 7
IRQ 8
IRQ 9
IRQ 10
IRQ 11
IRQ 12
IRQ 13
IRQ 14
IRQ 15

.global irq_stub_table
.type irq_stub_table, @object
irq_stub_table:
    .long irq0, irq1, irq2, irq3
    .long irq4, irq5, irq6, irq7
    .long irq8, irq9, irq10, irq11
    .long irq12, irq13, irq14, irq15

.type irq_common_stub, @function
irq_common_stub:
    pusha
    pushl %ds
    pushl %es
    pushl %fs
    pushl %gs

    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs

    movl %esp, %eax
    pushl %eax
    movl $irq_handler, %eax
    call *%eax
    popl %eax

    popl %gs
    popl %fs
    popl %es
    popl %ds
    popa
    addl $8, %esp
    iret
    