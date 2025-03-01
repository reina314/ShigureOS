#include <kernel/gdt.h>
#include <stdint.h>

/// @brief Entry format in GDT
struct gdt_entry
{
    unsigned short limit_low;
    unsigned short base_low;
    unsigned char base_middle;
    unsigned char access;
    unsigned char granularity;
    unsigned char base_high;
} __attribute__((packed));
// packed for avoiding compiler's optimization

struct gdt_ptr
{
    unsigned short limit;
    unsigned int base;
} __attribute__((packed));

struct gdt_entry gdt[3];
struct gdt_ptr gdtp;

/// @brief Reload new segment registers; defined in gdt.S
extern void gdt_flush();

void gdt_set_descriptor(int index, unsigned long base, unsigned long limit, unsigned char access, unsigned char granularity)
{
    gdt[index].base_low = (base & 0xFFFF);
    gdt[index].base_middle = (base >> 16) & 0xFF;
    gdt[index].base_high = (base >> 24) & 0xFF;

    gdt[index].limit_low = (limit & 0xFFFF);
    gdt[index].granularity = ((limit >> 16) & 0x0F);

    gdt[index].granularity |= (granularity & 0xF0);
    gdt[index].access = access;
}

/// @brief To be called from kernel main
/// @param
void gdt_install(void)
{
    gdtp.limit = (sizeof(struct gdt_entry) * 3) - 1;
    gdtp.base = (uint32_t)&gdt;

    // Null descriptor
    gdt_set_descriptor(0, 0, 0, 0, 0);
    // Code sengment
    gdt_set_descriptor(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    // Data segment
    gdt_set_descriptor(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    gdt_flush();
}