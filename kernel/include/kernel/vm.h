#ifndef _KERNEL_VM_H
#define _KERNEL_VM_H

// Paging
#define PAGE_SIZE 0x1000 // Page size; should be the same with frame size
#define PD_ENTRIES 0x400 // Number of entries in a page directory
#define PT_ENTRIES 0x400 // Number of entries in a page table

// PTE flags
#define PAGE_PRESENT 0x1    // 1 = mapped, 0 = not mapped
#define PAGE_RW 0x2         // 1 = writable, 0 = read-only
#define PAGE_USER 0x4       // 1 = user, 0 = kernel
#define PAGE_WRITETHRU 0x8  // 1 = write-through caching
#define PAGE_CACHE_DIS 0x10 // 1 = disable caching
#define PAGE_ACCESSED 0x20  // Set by CPU when accessed
#define PAGE_DIRTY 0x40     // Set by CPU when written
#define PAGE_SIZE_4MB 0x80  // 1 = 4MB page, 0 = 4KB page

// PDE flags
#define TABLE_PRESENT 0x1    // 1 = mapped, 0 = not mapped
#define TABLE_RW 0x2         // 1 = writable, 0 = read-only
#define TABLE_USER 0x4       // 1 = user, 0 = kernel
#define TABLE_WRITETHRU 0x8  // 1 = write-through caching
#define TABLE_CACHE_DIS 0x10 // 1 = disable caching
#define TABLE_ACCESSED 0x20  // Set by CPU when accessed
#define TABLE_DIRTY 0x40     // Set by CPU when written
#define TABLE_SIZE_4MB 0x80  // 1 = 4MB page, 0 = 4KB page

// Heap (all virtual address)
#define KHEAP_START 0xC0000000
#define KHEAP_INITIAL_SIZE 0x100000
#define KHEAP_MAX_SIZE 0xCFFFF000
#define HEAP_MAGIC 0x5AFE205E
#define HEAP_MIN_SIZE 0x70000
// #define HEAP_HEADER_ALIGNMENT 8 // Amount of Alignment (bytes) for heap header

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h> // for several types
#include "regs.h"

/// @brief Alias for page directory entry
typedef uint32_t pde_t;
/// @brief Alias for page table entry (=page)
typedef uint32_t pte_t;

/// @brief Structure for page table
typedef struct page_table
{
    pte_t entries[PT_ENTRIES]; // [31:12] paddr of page; [11:0] flags
} page_table_t;

/// @brief Structure for page directory
typedef struct page_directory
{
    pde_t entries[PD_ENTRIES];        // [31:12] paddr of PT; [11:0] flags
    page_table_t *tables[PD_ENTRIES]; // Pointers to each page table
    uint32_t paddr_pde;               // Physical address of entries[]
} page_directory_t;

/// @brief Structure for each AVL node
typedef struct avl_node
{
    uint32_t base; // Base virtual address
    size_t size;   // Size of the allocated block
    int height;    // Level of the node
    struct avl_node *left, *right;
} avl_node_t;

/// @brief Structure for heap block header
typedef struct header
{
    uint32_t magic; // Used for error checking and identification
    size_t size;    // Size of the allocated block excluding header
    // Add any options if necessary
} header_t;

/// @brief Structure for heap area
typedef struct heap
{
    avl_node_t *root;    // Root of the AVL tree
    uint32_t start_addr; // Start of the heap
    uint32_t end_addr;   // Current end of the heap (may expand)
    uint32_t max_size;   // Maximum heap size
    bool supervisor;     // If the heap is priviledged or not
    bool readonly;       // If the heap is read protected or not
} heap_t;

void vm_initialize(void);
heap_t *heap_init(uint32_t start_addr, uint32_t end_addr, uint32_t max_size, bool supervisor, bool readonly);
void *imalloc(uint32_t size, bool page_align, heap_t *heap);
uint32_t ikmalloc(uint32_t size, bool page_align, uint32_t *paddr);
void *kmalloc(size_t size);
void ifree(heap_t *heap, void *ptr);
void kfree(void *ptr);
void switch_page_directory(page_directory_t *pd);
page_table_t *clone_page_table(page_table_t *src_pt, uint32_t *paddr);
page_directory_t *clone_page_directory(page_directory_t *src_pd);
pte_t *get_page(uint32_t vaddr, page_directory_t *pd, bool create);
void page_fault(struct regs *regs);
void alloc_frame(pte_t *page, bool kernel, bool writable);
void free_frame(pte_t *page);
avl_node_t *avl_insert(avl_node_t *, uint32_t, size_t);
avl_node_t *avl_delete(avl_node_t *, uint32_t);
avl_node_t *find_best_fit(avl_node_t *node, size_t size);
void free_avl_tree(avl_node_t *node);

#endif