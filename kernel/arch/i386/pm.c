#include <kernel/pm.h>
#include <kernel/vm.h>
#include <kernel/multiboot.h>
#include <stdbool.h> // for bool type
#include <stdlib.h>  // for align()
#include <stddef.h>  // for size_t
#include <string.h>  // for memset
#include <stdio.h>   // for printf

// Passed from linker.ld
extern char stext[];   // Start of the text section
extern char ekernel[]; // End of the kernel

/// @brief Pointer to the beginning of physical memory
uint32_t pm_start;
/// @brief Pointer to the end of physical memory (pm_start + pm_amount)
uint32_t pm_end;
/// @brief Temporary variable to be passed to printf
uint32_t pm_start_temp;
/// @brief Address currently available after the end of the kernel
uint32_t placement_address;
int pm_amount;
/// @brief Amount of memory usable for allocation after the kernel
int pm_usable;

/// @brief The beginning of initial RAM disk; defined in kernel.c
uint32_t initrd_start;
/// @brief The end of initial RAM disk
uint32_t initrd_end;
/// @brief Size of initial RAM disk
uint32_t initrd_size;

/// @brief Bitmap of frames; used or free
uint32_t *frames;
/// @brief Total number of frames available in RAM
uint32_t nframes;

// PFN-PHYADDR
#define PFN_TO_ADDR(pfn) ((pfn) * FRAME_SIZE)
#define ADDR_TO_PFN(addr) ((uint32_t)(addr) / FRAME_SIZE)
// PFN bitmap manipulation
#define INDEX_FROM_PFN(pfn) (pfn / 8)
#define OFFSET_FROM_PFN(pfn) (pfn % 8)

/// @brief Buddy bitmap that contains status of every frame; used or free
buddy_bitmap_t buddy_bitmaps[MAX_ORDER + 1];

/// @brief Set up physical memory location; To be called in kernel main
/// @param initrd_start
void pm_initialize(uint32_t initrd_start)
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
            pm_start = mmap->addr;
            pm_amount = mmap->len;
        }
        mmap = (multiboot_memory_map_t *)((unsigned int)mmap + mmap->size + sizeof(mmap->size));
    }

    // Calculate initrd size
    initrd_end = *(uint32_t *)(mbt->mods_addr + 4);
    initrd_size = initrd_end - initrd_start;

    pm_start_temp = pm_start;

    // Calculate a pointer to the end of physical memory
    pm_end = pm_start + pm_amount;

    // Add the size of the kernel with 4KB boundary to memory location
    pm_start += (uint32_t)ALIGN(ekernel - stext, FRAME_SIZE);
    // Add the size of the initrd with 4KB boundary to memory location
    pm_start += (uint32_t)ALIGN(initrd_size, FRAME_SIZE);

    // Calculate the amount of memory usable for heap alloc after kernel
    pm_usable = pm_end - pm_start;

    // printf("\nKernel start addr : %x", (uint32_t)stext);
    // printf("\nKernel end addr   : %x", (uint32_t)ekernel);
    // printf("\nKernel size       : %x\n", (uint32_t)(ekernel - stext));

    // printf("\nMemory start      : %x", (uint32_t)pm_start_temp);
    // printf("\nMemory end        : %x", (uint32_t)pm_end);
    // printf("\nMemory amount     : %x\n", pm_amount);

    // printf("\nInitrd start      : %x", initrd_start);
    // printf("\nInitrd end        : %x", initrd_end);
    // printf("\nInitrd size       : %x\n", initrd_size);

    // printf("\nUsable mem start  : %x", (uint32_t)pm_start);
    // printf("\nUsable mem amount : %x\n", pm_usable);

    placement_address = (uint32_t)pm_start;
}

void frame_initialize(void)
{
    nframes = pm_usable / FRAME_SIZE;

    // Initialize buddy bitmaps
    for (int i = 0; i <= MAX_ORDER; i++)
    {
        buddy_bitmaps[i].num_bits = nframes / (1 << i);
        int bytes = (buddy_bitmaps[i].num_bits + 7) / 8;                   // Round up to nearest byte
        buddy_bitmaps[i].bitmap = (uint8_t *)ikmalloc(bytes, (i == 0), 0); // Align only the first time
        memset(buddy_bitmaps[i].bitmap, 0xFF, bytes);                      // Initialize all bits to 1 (free)
    }
}

/// @brief Set a bit as free (=1)
/// @param bmp Bitmap array of specific order
/// @param index Index the bit to change locates in the array
static void set_bit(buddy_bitmap_t *bmp, int index)
{
    bmp->bitmap[INDEX_FROM_PFN(index)] |= (1 << OFFSET_FROM_PFN(index));
}

/// @brief Clear a bit as used (=0)
/// @param bmp Bitmap array of specific order
/// @param index Index the bit to change locates in the array
static void clear_bit(buddy_bitmap_t *bmp, int index)
{
    bmp->bitmap[INDEX_FROM_PFN(index)] &= ~(1 << OFFSET_FROM_PFN(index));
}

/// @brief Check if a bit is set as free (=1)
/// @param bmp Bitmap array of specific order
/// @param index Index the bit to change locates in the array
/// @return
static bool is_bit_set(buddy_bitmap_t *bmp, int index)
{
    return bmp->bitmap[INDEX_FROM_PFN(index)] & (1 << OFFSET_FROM_PFN(index));
}

/// @brief Get order for a given size
/// @param size
/// @return Minimum O such that the number of required frame < 2^O
static int get_order(size_t size)
{
    int order = 0;
    size = (size + FRAME_SIZE - 1) / FRAME_SIZE; // Convert to block size units 1<=size
    while ((1U << order) < size)
        order++;
    return order;
}

/// @brief Allocate physical memory; using buddy system
/// @param size In bytes
/// @return -1 if failed; otherwise PFN (Physical Frame Number)
int32_t pmalloc(size_t size)
{
    int order = get_order(size);

    // Find the first available free block in the order needed
    buddy_bitmap_t *bmp = &buddy_bitmaps[order];

    for (int i = 0; i < bmp->num_bits; i++)
    {
        if (!is_bit_set(bmp, i)) // Check if the block is used (=0)
            continue;

        // Mark all related blocks as used
        for (int j = 0; j <= MAX_ORDER; j++)
        {
            uint32_t start_index = (order >= j) ? (i << (order - j)) : (i >> (j - order));
            if (!is_bit_set(&buddy_bitmaps[j], (int)start_index)) // Check if already marked as used
                break;

            for (int k = 0; k < (((order - j) > 0) ? (1 << (order - j)) : 1); k++)
                clear_bit(&buddy_bitmaps[j], (start_index + (uint32_t)k));
        }

        return (int32_t)(i * (1 << order)); // Return starting PFN
    }

    return -1; // No memory available
}

/// @brief Free physical memory; using buddy system
/// @param pfn Starting PFN (Physical frame number) to free
/// @param size In bytes
void pfree(uint32_t pfn, size_t size)
{
    int order = get_order(size);

    // Mark all related blocks as free
    for (int j = 0; j <= MAX_ORDER; j++)
    {
        uint32_t start_index = pfn >> j;

        for (int k = 0; k < (((order - j) > 0) ? (1 << (order - j)) : 1); k++)
            set_bit(&buddy_bitmaps[j], (start_index + (uint32_t)k));

        if (!is_bit_set(&buddy_bitmaps[order], (start_index ^ 0b1))) // start_index ^ 0b1 is buddy index
            // Buddy is not free; stop merging
            break;
    }
}

/// @brief Clean up buddy bitmaps
/// @param
void frame_cleanup(void)
{
    for (int i = 0; i <= MAX_ORDER; i++)
    {
        // kfree(buddy_bitmaps[i].bitmap);
        buddy_bitmaps[i].bitmap = NULL;
    }
}
