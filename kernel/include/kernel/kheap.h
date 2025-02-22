#ifndef _KERNEL_KHEAP_H
#define _KERNEL_KHEAP_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

// Set the virtual address of the start of the kernel heap
#define KHEAP_START 0xC0000000
#define KHEAP_INITIAL_SIZE 0x100000

#define HEAP_INDEX_SIZE 0x20000
#define HEAP_MAGIC 0xDEADBABE
#define HEAP_MIN_SIZE 0x70000

/// @brief Header for a hole or block in the heap
typedef struct header
{
    uint32_t magic;  // Magic number; used for error checking and identification
    uint8_t is_hole; // 1 if a hole; 0 if a block
    uint32_t size;   // Size of the block including the end footer
} header_t;

/// @brief Footer for a hole or block in the heap
typedef struct footer
{
    uint32_t magic;   // Magic same as header_t
    header_t *header; // Pointer to block header
} footer_t;

/// @brief Defines heap format
typedef struct heap
{
    ordered_array_t index;
    uint32_t start_address; // Start of allocated space
    uint32_t end_address;   // End of allocated space; can be max_address
    uint32_t max_address;   // Max address the heap expands to
    uint8_t supervisor;     // Should extra pages mapped as supervisor only
    uint8_t readonly;       // Should extra pages mapped as read only
} heap_t;

heap_t *create_heap(uint32_t, uint32_t, uint32_t, bool, bool);
void *alloc(uint32_t, bool, heap_t *);
void free_kheap(void *, heap_t *);
// uint32_t kmalloc_internal(uint32_t, bool, uint32_t *);
uint32_t kmalloc_aligned(uint32_t);
uint32_t kmalloc_physical(uint32_t, uint32_t *);
uint32_t kmalloc_aligned_physical(uint32_t, uint32_t *);
uint32_t kmalloc(uint32_t);
void kfree(void *);

#endif