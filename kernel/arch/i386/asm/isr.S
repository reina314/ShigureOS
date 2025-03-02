.section .text

.macro ISR_NO_ERR num
.global isr_stub\num
.type isr_stub\num, @function
isr_stub\num:
    cli
    pushl $0
    pushl $\num
    jmp isr_common_stub
.endm

.macro ISR_ERR num
.global isr_stub\num
.type isr_stub\num, @function
isr_stub\num:
    cli
    pushl $0
    pushl $\num
    jmp isr_common_stub
.endm

.extern exception_handler

ISR_NO_ERR 0
ISR_NO_ERR 1
ISR_NO_ERR 2
ISR_NO_ERR 3
ISR_NO_ERR 4
ISR_NO_ERR 5
ISR_NO_ERR 6
ISR_NO_ERR 7
ISR_ERR    8
ISR_NO_ERR 9
ISR_ERR    10
ISR_ERR    11
ISR_ERR    12
ISR_ERR    13
ISR_ERR    14
ISR_NO_ERR 15
ISR_NO_ERR 16
ISR_ERR    17
ISR_NO_ERR 18
ISR_NO_ERR 19
ISR_NO_ERR 20
ISR_NO_ERR 21
ISR_NO_ERR 22
ISR_NO_ERR 23
ISR_NO_ERR 24
ISR_NO_ERR 25
ISR_NO_ERR 26
ISR_NO_ERR 27
ISR_NO_ERR 28
ISR_NO_ERR 29
ISR_ERR    30
ISR_NO_ERR 31

.global isr_stub_table
.type isr_stub_table, @object
isr_stub_table:
    .long isr_stub0, isr_stub1, isr_stub2, isr_stub3
    .long isr_stub4, isr_stub5, isr_stub6, isr_stub7
    .long isr_stub8, isr_stub9, isr_stub10, isr_stub11
    .long isr_stub12, isr_stub13, isr_stub14, isr_stub15
    .long isr_stub16, isr_stub17, isr_stub18, isr_stub19
    .long isr_stub20, isr_stub21, isr_stub22, isr_stub23
    .long isr_stub24, isr_stub25, isr_stub26, isr_stub27
    .long isr_stub28, isr_stub29, isr_stub30, isr_stub31

.type isr_common_stub, @function
isr_common_stub:
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
    movl $exception_handler, %eax
    call *%eax
    popl %eax

    popl %gs
    popl %fs
    popl %es
    popl %ds
    popa
    addl $8, %esp
    iret