#include <kernel/mm.h>
#include <kernel/multiboot.h>
#include <stdbool.h> // for bool type
#include <stdlib.h>  // for round_up_to_multiple
#include <stddef.h>  // for size_t
#include <stdio.h>   // for printf

// Passed from linker.ld
extern char end[]; // End of the kernel
extern char code[];

/// @brief Pointer to the beginning of physical memory
uint32_t memStartLocation;
/// @brief Pointer to the end of physical memory (memStartLocation + memAmont)
uint32_t memEndLocation;
/// @brief Temporary variable to be passed to printf
uint32_t ramStartLocation;
/// @brief Address the kheap is initialized; should be declared in kheap.c?
uint32_t placement_address;
int memAmount;
int memUsable;

/// @brief The beginning of initial RAM disk; defined in kernel.c
uint32_t initrd_start;
/// @brief The end of initial RAM disk
uint32_t initrd_end;
/// @brief Size of initial RAM disk
uint32_t initrd_size;

/// @brief Linkedlist header for mem allocation
typedef struct mm_header
{
    void *ptr;
    size_t size;
    bool used;
    void *next;
} mm_header_t;

/// @brief Prototype
/// @param
/// @return
void *palloc(unsigned int);

/// @brief Set up memory location; To be called in kernel main
/// @param initrd_start
void mm_initialize(uint32_t initrd_start)
{
    // multiboot_info passed to kernel main
    extern multiboot_info_t *mbt;
    initrd_start = initrd_start;
    multiboot_memory_map_t *mmap = (multiboot_memory_map_t *)mbt->mmap_addr;

    // Loop through mmap for usable (1) part larger than 0x1000000 bytes
    while ((uint32_t)mmap < (mbt->mmap_addr + mbt->mmap_length))
    {
        if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE && mmap->len > 0x1000000)
        {
            memStartLocation = mmap->addr;
            memAmount = mmap->len;
        }
        mmap = (multiboot_memory_map_t *)((unsigned int)mmap + mmap->size + sizeof(mmap->size));
    }

    // Calculate initrd size
    initrd_end = *(uint32_t *)(mbt->mods_addr + 4);
    initrd_size = initrd_end - initrd_start;

    ramStartLocation = memStartLocation;

    // Calculate a pointer to the end of physical memory
    memEndLocation = memStartLocation + memAmount;

    // Add the size of the kernel with 4KB boundary to memory location
    memStartLocation += (uint32_t)round_up_to_multiple(end - code, 4096);
    // Add the size of the initrd with 4KB boundary to memory location
    memStartLocation += (uint32_t)round_up_to_multiple(initrd_size, 4096);

    // Calculate the amount of memory usable for heap alloc after kernel
    memUsable = memEndLocation - memStartLocation;

    printf("\nKernel start addr : %x", (uint32_t)code);
    printf("\nKernel end addr   : %x", (uint32_t)end);
    printf("\nKernel size       : %x\n", (uint32_t)(end - code));

    printf("\nMemory start      : %x", (uint32_t)ramStartLocation);
    printf("\nMemory end        : %x", (uint32_t)memEndLocation);
    printf("\nMemory amount     : %x\n", memAmount);

    printf("\nInitrd start      : %x", initrd_start);
    printf("\nInitrd end        : %x", initrd_end);
    printf("\nInitrd size       : %x\n", initrd_size);

    printf("\nUsable mem start  : %x", (uint32_t)memStartLocation);
    printf("\nUsable mem amount : %x\n", memUsable);

    placement_address = (uint32_t)memStartLocation;
}
