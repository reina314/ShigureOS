#include <kernel/idt.h>
#include <string.h>
#include <stdint.h>

/// @brief Entry format for IDT
struct idt_entry
{
    unsigned short base_low;
    unsigned short selector;
    unsigned char reserved; // always should be 0
    unsigned char flags;
    unsigned short base_high;
} __attribute__((packed));
// Avoid compiler's optimization

struct idt_ptr
{
    unsigned short limit;
    unsigned int base;
} __attribute__((packed));

__attribute__((aligned(0x10))) // aligned for performance
/// @brief Declare IDT of 256 entries
struct idt_entry idt[256];
struct idt_ptr idtp;

/// @brief Defined in idt.S
extern void idt_load();

void idt_set_descriptor(unsigned char index, unsigned long base, unsigned short selector, unsigned char flags)
{
    idt[index].base_low = (base & 0xFFFF);
    idt[index].base_high = (base >> 16) & 0xFFFF;

    idt[index].selector = selector;
    idt[index].reserved = 0;
    idt[index].flags = flags;
}

/// @brief To be called from kernel main; do ISR staffs too
void idt_install()
{
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base = (uint32_t)&idt;

    // Initialize entire idt to 0
    memset(&idt, 0, sizeof(struct idt_entry) * 256);

    // You can add new ISRs to the IDT here
    //
    idt_load();
}