#ifndef _KERNEL_PAGING_H
#define _KERNEL_PAGING_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "regs.h"

/// @brief Defines structure for each page
typedef struct page
{
    uint32_t present : 1;  // if page present
    uint32_t rw : 1;       // readwrite if set
    uint32_t user : 1;     // supervisor if clear
    uint32_t accessed : 1; // if accessed since last refresh
    uint32_t dirty : 1;    // if written since last refresh
    uint32_t unused : 7;   // combination of unused and reserved bits
    uint32_t frame : 20;   // frame address (shifted right 12 bits)
} page_t;

/// @brief Defines structure for each page table
typedef struct page_table
{
    page_t pages[1024];
} page_table_t;

/// @brief Defines structure for the page directory
typedef struct page_directory
{
    page_table_t *tables[1024];
    uint32_t tablesPhysical[1024]; // pointers to page tables in physical address; loading into CR3 register
    uint32_t physicalAddr;         // pointer to tablesPhysical in physical address
} page_directory_t;

void paging_initialize(void);
void switch_page_directory(page_directory_t *);
page_t *get_page(uint32_t, bool, page_directory_t *);
void page_fault(struct regs *);
void alloc_frame(page_t *, bool, bool);
void free_frame(page_t *);
page_directory_t *clone_page_directory(page_directory_t *);

#endif