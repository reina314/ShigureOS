#ifndef _KERNEL_PM_H
#define _KERNEL_PM_H

#define FRAME_SIZE 0x1000 // Frame size (bytes)
#define MAX_ORDER 7       // Max order (size = 2^MAX_ORDER frames); Up to 512K blocks

#include <stddef.h>
#include <stdint.h>

/// @brief Structure of buddy frame allocator system; 1=free, 0=used
typedef struct buddy_bitmap
{
    uint8_t *bitmap; // Pointer to the bitmap array
    int num_bits;    // Number of bits in this order
} buddy_bitmap_t;

void pm_initialize(uint32_t);
void frame_initialize(void);
int32_t pmalloc(size_t size);
void pfree(uint32_t pfn, size_t size);

#endif