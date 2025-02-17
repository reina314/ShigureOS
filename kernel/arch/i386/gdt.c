#include <kernel/gdt.h>
#include <stdint.h>
#include <stdio.h>

// Entry format in GDT
struct gdt_entry {
    unsigned short limit_low;
    unsigned short base_low;
    unsigned char base_middle;
    unsigned char flag;
    unsigned char granularity;
    unsigned char base_high;
} __attribute__((packed));
// packed for avoiding compiler's optimization

struct gdt_ptr {
    unsigned short limit;
    unsigned int base;
} __attribute__((packed));

struct gdt_entry gdt[3];
struct gdt_ptr gptr;

// Reload new segment registers; defined in boot.S
extern void gdt_flush();

void gdt_encode_entry(int index, unsigned long base, unsigned long limit, unsigned char flag, unsigned char granularity) {
    gdt[index].base_low = (base & 0xFFFF);
    gdt[index].base_middle = (base >> 16) & 0xFF;
    gdt[index].base_high = (base >> 24) & 0xFF;
    
    gdt[index].limit_low = (limit & 0xFFFF);
    gdt[index].granularity = ((limit >> 16) & 0x0F);

    gdt[index].granularity |= (granularity & 0xF0);
    gdt[index].flag = flag;
}

// To be called from kernel main
void gdt_install(void) {
    gptr.limit = (sizeof(struct gdt_entry) * 3) - 1;
    gptr.base = (uint32_t) &gdt;

    // Null descriptor
    gdt_encode_entry(0, 0, 0, 0, 0);
    // Code sengment
    gdt_encode_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    // Data segment
    gdt_encode_entry(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

    printf("Flushing old GDT cache...\n");
    gdt_flush();

    printf("New GDT has been enabled.\n");
}