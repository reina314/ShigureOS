#include <kernel/idt.h>
#include <kernel/irq.h>
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
/// @param index
/// @param handler
void register_request_handler(int index, irqhandler handler)
{
    irq_routines[index] = handler;
}

void unregister_request_handler(int index)
{
    irq_routines[index] = 0;
}

/// @brief Remap IRQ 0-7 to IDT entry 32-47
/// @param
void irq_remap(void)
{
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x0);
    outb(0xA1, 0x0);
}

/// @brief To be called in kernel main
void irq_install()
{
    irq_remap();

    for (uint8_t vector = 0; vector < 16; vector++)
    {
        idt_set_descriptor(vector + 32, (unsigned)irq_stub_table[vector], 0x08, 0x8E);
    }
}

/// @brief Send EOI(0x20) to two chips of IRQ controllers (located at 0x20 and 0xA0) conditionally
/// @param r
void irq_handler(struct regs *r)
{
    void (*handler)(struct regs *r);

    handler = irq_routines[r->int_no - 32];

    // If IDT entry is greater than 40 (meaning IRQ 8-15), send EOI to the second (0xA0)
    if (r->int_no >= 40)
    {
        outb(0xA0, 0x20);
    }

    // Either case, send EOI to the first (0x20)
    outb(0x20, 0x20);

    if (handler)
    {
        handler(r);
    }
}