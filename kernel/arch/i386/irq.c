#include <kernel/idt.h>
#include <kernel/irq.h>
#include <stdio.h> // for debug
#include <stdint.h>
#include <stdlib.h> // for outb
#include <regs.h>

/// @brief An array of function pointers; for custom IRQ handlers for a given IRQ
void *irq_routines[16] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0};

/// @brief A table containing the address for each IRQ entry
extern void *irq_stub_table[];

/// @brief Install custom IRQ handler for the given IRQ
/// @param vector
/// @param handler
void register_request_handler(int vector, irqhandler handler)
{
    printf("Registering handler for IRQ %d\n", vector);
    irq_routines[vector] = handler;
}

void unregister_request_handler(int vector)
{
    irq_routines[vector] = 0;
}

/// @brief Remap IRQ 0-7 to IDT entry 32-47
/// @param
void irq_remap(void)
{
    printf("Remapping IRQs\n");
    outb(0x20, 0x11); // Start PIC1 initialization
    outb(0xA0, 0x11); // Start PIC2 initialization
    outb(0x21, 0x20); // Remap PIC1 to 0x20-0x27 (32-39)
    outb(0xA1, 0x28); // Remap PIC2 to 0x28-0x2F (40-47)
    outb(0x21, 0x04); // Tell PIC1 that PIC2 is at IRQ2 (0000 0100)
    outb(0xA1, 0x02); // Tell PIC2 its cascade identity (0000 0010)
    outb(0x21, 0x01); // Set PIC1 to 8086 mode
    outb(0xA1, 0x01); // Set PIC2 to 8086 mode
    outb(0x21, 0x0);  // Enable all IRQs on PIC1
    outb(0xA1, 0x0);  // Enable all IRQs on PIC2
}

/// @brief To be called in kernel main
void irq_install()
{
    printf("Installing IRQs\n");
    irq_remap();

    printf("IRQ 1 should be at IDT entry 33, set to address %p\n", irq_stub_table[1]);

    for (uint8_t vector = 0; vector < 16; vector++)
    {
        printf("Setting IDT descriptor for IRQ %d\n", vector); // Debug print
        idt_set_descriptor(vector + 32, (unsigned)irq_stub_table[vector], 0x08, 0x8E);
    }
}

/// @brief Send EOI(0x20) to two chips of IRQ controllers (located at 0x20 and 0xA0) conditionally
/// @param r
void irq_handler(struct regs *r)
{
    void (*handler)(struct regs *r);

    handler = irq_routines[r->int_no - 32];
    printf("IRQ %d triggered, mapped index %d\n", r->int_no, r->int_no - 32);

    // If IDT entry is greater than 40 (meaning IRQ 8-15), send EOI to the second (0xA0)
    if (r->int_no >= 40)
    {
        outb(0xA0, 0x20);
    }

    // Either case, send EOI to the first (0x20)
    outb(0x20, 0x20);

    if (handler)
    {
        printf("Calling handler for IRQ %d\n", r->int_no - 32);
        handler(r);
    }
    else
    {
        printf("No handler registered for IRQ %d\n", r->int_no - 32);
    }
}