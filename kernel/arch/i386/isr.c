#include <kernel/isr.h>
#include <kernel/idt.h>
#include <stdio.h>
#include "regs.h"

/// @brief List of registered interrupt handlers
void *interrupt_handlers[256];

/// @brief A table containing the address for each ISR entry
extern void *isr_stub_table[];

/// @brief Map ISRs to each IDT entries
/// @param
void isr_install(void)
{
    for (uint8_t vector = 0; vector < 32; vector++)
        idt_set_descriptor(vector, (unsigned)isr_stub_table[vector], 0x08, 0x8E);
}

char *exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault 007",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"};

/// @brief
/// @param vector
/// @param handler
void register_interrupt_handler(int vector, handler handler)
{
    interrupt_handlers[vector] = handler;
}

/// @brief Currently all ISRs point to this function and just halt the system
/// @param r
void exception_handler(struct regs *r)
{
    void (*handler)(struct regs *r);

    // If interrupt_handler defined for the interrupt then call it
    if (interrupt_handlers[r->int_no] != 0)
    {
        handler = interrupt_handlers[r->int_no];
        handler(r);
    }

    if (r->int_no < 32)
    {
        puts("\nException. System halted.");
        puts(exception_messages[r->int_no]);

        for (;;)
            ;
    }
}
