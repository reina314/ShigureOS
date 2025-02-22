#include <kernel/paging.h>
#include <kernel/kheap.h>
#include <kernel/isr.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>  // for debug
#include <string.h> // for memset

#include "regs.h"

/// @brief Kernel's page directory
page_directory_t *kernel_directory = 0;
/// @brief Current page directory
page_directory_t *current_directory = 0;

/// @brief Bitset of frames; used or free
uint32_t *frames;
/// @brief Total number of frames available in RAM
uint32_t nframes;

extern uint32_t placement_address; // Defined in mm.c
extern heap_t *kheap;              // Defined in kheap.c
extern int memUsable;              // Defined in mm.c

// Macros used in the bitset algorithm
#define INDEX_FROM_BIT(a) (a / (8 * 4))
#define OFFSET_FROM_BIT(a) (a % (8 * 4))

/// @brief To be called only once in a runtime
/// @param page_directory
void enable_paging(page_directory_t *page_directory)
{
    uint32_t cr0;
    current_directory = page_directory;

    printf("11\n");
    asm volatile("mov %0, %%cr3" ::"r"(&page_directory->physicalAddr));
    printf("12\n");
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; // Enable paging
    printf("13\n");
    asm volatile("mov %0, %%cr0" ::"r"(cr0)); // <- stack here! placement_address? malloc alignment?
    printf("14\n");
}

/// @brief Initializes the paging system
/// @param
void paging_initialize(void)
{
    // Calculate the nframes available based on memUsable
    nframes = memUsable / 0x1000;
    frames = (uint32_t *)kmalloc_aligned(INDEX_FROM_BIT(nframes)); // allocate bitset
    memset(frames, 0, INDEX_FROM_BIT(nframes));                    // initialize with 0

    printf("1\n");

    // Create the top level page directory
    kernel_directory = (page_directory_t *)kmalloc_aligned(sizeof(page_directory_t));
    memset(kernel_directory, 0, sizeof(page_directory_t));                       // initialize with 0
    kernel_directory->physicalAddr = (uint32_t)kernel_directory->tablesPhysical; // for multitasking

    printf("2\n");

    // Create pages for the kernel heap area
    unsigned int address = 0;
    for (address = KHEAP_START; address < KHEAP_START + KHEAP_INITIAL_SIZE; address += 0x1000)
    {
        if (address == KHEAP_START)
        {
            page_t *firstPageAddress = get_page(address, true, kernel_directory);
            printf("\nFirst page address : %x\n", firstPageAddress);
            printf("First page content : %x\n", *firstPageAddress);
        }
        get_page(address, true, kernel_directory);
    }

    printf("3\n");

    // Create pages for kernel things (except kheap's) and map them to frames identically (phys addr == virt addr)
    // so we can access the RAM easily from the kernel
    address = 0;
    while (address < placement_address + 0x1000)
    {
        alloc_frame(get_page(address, true, kernel_directory), false, false); // Kernel code is readable but not writable from user space
        address += 0x1000;
    }

    printf("4\n");

    // Map kheap pages to frames
    for (address = KHEAP_START; address < KHEAP_START + KHEAP_INITIAL_SIZE; address += 0x1000)
    {
        alloc_frame(get_page(address, true, kernel_directory), false, false);
    }
    printf("5\n");

    // Register page fault handler before enabling paging
    register_interrupt_handler(14, page_fault);

    printf("6\n");

    // Enable paging
    enable_paging(kernel_directory);

    printf("7\n");

    // Initialize the kernel heap
    kheap = create_heap(KHEAP_START, KHEAP_START + KHEAP_INITIAL_SIZE, 0xCFFFF000, false, false);

    printf("8\n");

    // For multitasking
    // clone kernel_directory and switch to the clone
    current_directory = clone_page_directory(kernel_directory);
    switch_page_directory(current_directory);
    printf("9\n");
}

/// @brief Switch the top level page directory to given one
/// @param page_directory
void switch_page_directory(page_directory_t *page_directory)
{
    uint32_t cr0;
    current_directory = page_directory;
    asm volatile("mov %0, %%cr3" ::"r"(page_directory->physicalAddr));
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; // Enable paging
    asm volatile("mov %0, %%cr0" ::"r"(cr0));
}

/// @brief Get a page from a page directory
/// @param address Virtual address of the page to retrieve
/// @param make Should create a page if not exist
/// @param page_directory Page directory to get the page from
/// @return The pointer to the page or 0 if error
page_t *get_page(uint32_t address, bool make, page_directory_t *page_directory)
{
    // Convert address into index
    address /= 0x1000;

    // Find the page table containing the address
    uint32_t table_index = address / 1024;
    if (!page_directory->tables[table_index]) // If this page is already assigned
    {
        if (!make)
        {
            return 0; // Table not exist and not requested to create it
        }

        // Allocate and create a new page table
        uint32_t tmp;
        page_directory->tables[table_index] = (page_table_t *)kmalloc_aligned_physical(sizeof(page_table_t), &tmp);
        page_directory->tablesPhysical[table_index] = tmp | 0x7; // present, rw, user
    }

    return &page_directory->tables[table_index]->pages[address % 1024];
}

/// @brief Handler for page_fault; registered in ISR handler
/// @param r
void page_fault(struct regs *regs)
{
    // Faulting address is stored in CR2 register
    uint32_t faulting_address;
    asm volatile("mov %%cr2, %0" : "=r"(faulting_address));

    // Analyze the error code
    int present = !(regs->err_code & 0x1); // Page not present
    int rw = regs->err_code & 0x2;         // Write operation
    int us = regs->err_code & 0x4;         // User-mode
    int reserved = regs->err_code & 0x8;   // Overwritten CPU reserved page
    int id = regs->err_code & 0x10;        // Instruction fetch
    id = id;                               // Avoid warning

    printf("Page fault (%s%s%s%s) at %x\n",
           present ? "present" : "",
           rw ? "read-only" : "",
           us ? "user-mode" : "",
           reserved ? "reserved" : "",
           faulting_address);

    if (!present && !rw && !us)
    {
        printf("\nSupervisory process tried to read a non-present page entry.");
    }
    else if (!present && !rw && us)
    {
        printf("\nSupervisory process tried to read a page and cause a protection fault.");
    }
    else if (!present && rw && !us)
    {
        printf("\nSupervisory process tried to write to a non-present page entry.");
    }
    else if (!present && rw && us)
    {
        printf("\nSupervisory process tried to write a page and caused a protection fault.");
    }
    else if (present && !rw && !us)
    {
        printf("\nUser Process tried to read a non-present page entry.");
    }
    else if (present && !rw && us)
    {
        printf("\nUser process tried to read a page and caused a protection fault.");
    }
    else if (present && rw && !us)
    {
        printf("\nUer process tried to write to a non-present page entry.");
    }
    else if (present && rw && us)
    {
        printf("\nUser process tried to write a page and caused a protection fault.");
    }
    printf("\n");

    PANIC("Page fault");
}

/// @brief Returns index of the first unused frame in the frame bitset
/// @return Returns index * 4 * 8 + offset; otherwise -1
static uint32_t first_frame()
{
    uint32_t i, j;
    for (i = 0; i < INDEX_FROM_BIT(nframes); i++)
    {
        if (frames[i] == 0xFFFFFFFF) // When there is no bit set as free
        {
            continue;
        }

        for (j = 0; j < 32; j++) // Check which bit is set
        {
            uint32_t test = 0x1 << j;
            if (!(frames[i] & test)) // When not 0
            {
                return i * 4 * 8 + j;
            }
        }
    }

    return -1;
}

/// @brief Marks a frame as used in the frames bitset
/// @param frame_addr Provided as (index * 4 * 8 + offset) * 0x1000
static void set_frame(uint32_t frame_addr)
{
    uint32_t frame = frame_addr / 0x1000;
    uint32_t index = INDEX_FROM_BIT(frame);
    uint32_t offset = OFFSET_FROM_BIT(frame);
    frames[index] |= (0x1 << offset);
}

/// @brief Allocates a new frame
/// @param page
/// @param is_kernel
/// @param is_writable
void alloc_frame(page_t *page, bool is_kernel, bool is_writable)
{
    if (page->frame != 0)
    {
        return;
    }

    uint32_t index = first_frame();
    if (index == (uint32_t)-1)
    {
        PANIC("No free frames");
    }

    set_frame(index * 0x1000);
    page->present = 1;
    page->rw = (is_writable) ? 1 : 0;
    page->user = (is_kernel) ? 0 : 1;
    page->frame = index;
}

/// @brief Copy page to clone physically; defined in process.S
extern void copy_physical_page(uint32_t, uint32_t);

/// @brief Clones a page table
/// @param src
/// @param physical_address
/// @return The address of cloned table
page_table_t *clone_page_table(page_table_t *src, uint32_t *physical_address)
{
    // Create a new page-aligned page table
    page_table_t *page_table = (page_table_t *)kmalloc_aligned_physical(sizeof(page_table_t), physical_address);
    memset(page_table, 0, sizeof(page_table_t));

    // Get a new frame etc. for every entry in the table
    for (int i = 0; i < 1024; i++)
    {
        if (!src->pages[i].frame) // If the source frame is 0, no need to do anything
        {
            continue;
        }

        // Get a new frame for the page, no kernel, no writable
        alloc_frame(&page_table->pages[i], false, false);

        // Clone the flags
        page_table->pages[i].present = src->pages[i].present;
        page_table->pages[i].rw = src->pages[i].rw;
        page_table->pages[i].user = src->pages[i].user;
        page_table->pages[i].accessed = src->pages[i].accessed;
        page_table->pages[i].dirty = src->pages[i].dirty;

        // Physically copy the data from src to the table using process.S
        copy_physical_page(src->pages[i].frame * 0x1000, page_table->pages[i].frame * 0x1000);
    }

    return page_table;
}

/// @brief Clones a page directory
/// @param src
/// @return The address of cloned directory
page_directory_t *clone_page_directory(page_directory_t *src)
{
    uint32_t physical_address;

    // Make a new page directory and obtain its physical address
    page_directory_t *page_directory = (page_directory_t *)kmalloc_aligned_physical(sizeof(page_directory_t), &physical_address);
    memset(page_directory, 0, sizeof(page_directory_t));

    // Obtain the physical address of tablesPhysical
    uint32_t offset = (uint32_t)page_directory->tablesPhysical - (uint32_t)page_directory;
    page_directory->physicalAddr = physical_address + offset;

    // Copy each table unless it is 0
    for (int i = 0; i < 1024; i++)
    {
        if (!src->tables[i])
        {
            continue;
        }

        // Decide to link or copy a page;
        // link if it's part of the kernel (included in kernel directory), otherwise just copy it
        if (src->tables[i] == kernel_directory->tables[i])
        {
            // Link the page by pointer
            page_directory->tables[i] = src->tables[i];
            page_directory->tablesPhysical[i] = src->tablesPhysical[i];
        }
        else
        {
            // Copy the page
            uint32_t physical_address;
            page_directory->tables[i] = clone_page_table(src->tables[i], &physical_address);
            page_directory->tablesPhysical[i] = physical_address | 0x07; // Set writable, user, present
        }
    }

    return page_directory;
}