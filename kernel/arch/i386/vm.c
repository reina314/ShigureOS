#include <kernel/vm.h>
#include <kernel/pm.h>
#include <kernel/isr.h> // for register_interrupt_handler
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h> // for align()
#include <string.h>
#include <stdio.h> // for debug
#include "regs.h"

extern uint32_t placement_address;

// Extract starting PFN from PTE (page)
#define PTE_TO_PFN(pte) (pte >> 12)

/// @brief Kernel's page directory
page_directory_t *kernel_pd = 0;
/// @brief Current page directory
page_directory_t *current_pd = 0;

/// @brief Pointer to the start of kernel heap
heap_t *kheap = 0;

extern void load_page_directory(uint32_t *); // defined in paging.S
extern void enable_paging(void);             // definedd in paging.S

#include <stdio.h>

/// @brief Only for debug purpose
/// @param node
/// @param depth
static void print_avl_tree(avl_node_t *node, int depth)
{
    if (!node)
        return;

    // Print right subtree first (for a more visual tree representation)
    print_avl_tree(node->right, depth + 1);

    // Indentation based on depth
    for (int i = 0; i < depth; i++)
        printf("    "); // 4 spaces per depth level

    // Print current node
    printf("Addr: %x | Size: %u | Height: %d\n", node->base, node->size, node->height);

    // Print left subtree
    print_avl_tree(node->left, depth + 1);
}

void vm_initialize(void)
{
    frame_initialize(); // Initialize frame allocator first; defined in pm.c

    // Initialize kernel_pd
    kernel_pd = (page_directory_t *)kmalloc(sizeof(page_directory_t), true, 0);
    memset(kernel_pd, 0, sizeof(page_directory_t));
    for (int index = 0; index < PD_ENTRIES; index++)
        kernel_pd->entries[index] = (uint32_t)TABLE_RW;
    kernel_pd->paddr_pde = (uint32_t)kernel_pd->entries;

    uint32_t vaddr;

    // Map the kernel heap
    // Not allocating the actual memory here because kernel needs identical mapping below
    for (vaddr = KHEAP_START; vaddr < KHEAP_START + KHEAP_INITIAL_SIZE; vaddr += PAGE_SIZE)
    {
        if (vaddr == KHEAP_START)
        {
            pte_t *test_entry = get_page(vaddr, kernel_pd, true);
            printf("\nFirst page entry  : %x", *test_entry);
            printf("\nFirst page addr   : %x\n", test_entry);
        }
        get_page(vaddr, kernel_pd, true);
    }

    // Map the kernel (heap not included) identically
    // Deliberately using while loop to ensure placement_address increases in each iteration
    vaddr = 0;
    while (vaddr < placement_address + PAGE_SIZE)
    {
        // Kernel code is readable but not writable from user space
        alloc_frame(get_page(vaddr, kernel_pd, true), false, false);
        vaddr += PAGE_SIZE;
    }

    // Map kheap pages to frames
    for (vaddr = KHEAP_START; vaddr < KHEAP_START + KHEAP_INITIAL_SIZE; vaddr += PAGE_SIZE)
        alloc_frame(get_page(vaddr, kernel_pd, true), false, false);

    // Register page fault handler before enabling paging
    register_interrupt_handler(14, page_fault);

    // Enable paging
    load_page_directory((uint32_t *)kernel_pd->paddr_pde);
    enable_paging();

    // Initialize the kernel heap
    kheap = heap_init(KHEAP_START, KHEAP_START + KHEAP_INITIAL_SIZE, KHEAP_MAX_SIZE, false, false);

    // For multitasking
    // Clone kernel_pd and switch to the clone
    current_pd = clone_page_directory(kernel_pd);
    // printf("\nkernel_pd  : %x", kernel_pd->paddr_pde);
    // printf("\ncurrent_pd : %x\n", current_pd->paddr_pde);
    switch_page_directory(current_pd);
}

/// @brief Create a heap area ranging from start_addr to end_addr
/// @param start_addr Must be page-aligned
/// @param end_addr Must be page-aligned
/// @param max_size Max size the heap can expands to
/// @param supervisor If the heap is for kernel or not
/// @param readonly If the heap is read-only or not
/// @return The pointer to heap_t
heap_t *heap_init(uint32_t start_addr, uint32_t end_addr, uint32_t max_size, bool supervisor, bool readonly)
{
    heap_t *heap = (heap_t *)kmalloc(sizeof(heap_t), false, 0);
    if (!heap)
        return NULL; // Handle allocation failure

    // Make sure start_addr & end_addr are page-aligned
    start_addr = ALIGN(start_addr, PAGE_SIZE);
    end_addr = ALIGN(end_addr, PAGE_SIZE);

    heap->root = avl_insert(NULL, start_addr, (size_t)(end_addr - start_addr));
    heap->start_addr = start_addr;
    heap->end_addr = end_addr;
    heap->max_size = max_size;
    heap->supervisor = supervisor;
    heap->readonly = readonly;

    return heap;
}

/// @brief Expand a heap area if possible
/// @param heap The pointer to the heap to expand
/// @param size Size-delta in bytes
/// @return The entire size of the heap area after allocation; 0 if any error
static uint32_t expand_heap(heap_t *heap, size_t size)
{
    uint32_t new_end_addr = heap->end_addr + (uint32_t)size;
    // Page-align new end address
    new_end_addr = ALIGN(new_end_addr, PAGE_SIZE);

    if (new_end_addr > heap->start_addr + heap->max_size)
        return 0; // Beyond heap's max end address

    // Make sure that kmalloc increases placement_address each time by using while loop
    uint32_t temp_end_addr = heap->end_addr;
    while (temp_end_addr < new_end_addr)
    {
        alloc_frame(get_page(temp_end_addr, kernel_pd, true),
                    (heap->supervisor) ? true : false,
                    (heap->readonly) ? false : true);
        temp_end_addr += PAGE_SIZE;
    }

    heap->end_addr = new_end_addr;
    return heap->end_addr - heap->start_addr; // Success
}

/// @brief Reduce a heap area if possible
/// @param heap The pointer to the heap to reduce
/// @param size Size-delta in bytes
/// @return The entire size of the heap area after reduction; 0 if any error
static uint32_t reduce_heap(heap_t *heap, size_t size)
{
    uint32_t new_end_addr = heap->end_addr - (uint32_t)size;
    // Page-align new end address
    new_end_addr = ALIGN(new_end_addr, PAGE_SIZE);
    if (new_end_addr == heap->end_addr)
        // If aligned end address remains the same, do nothing
        return 0;

    // Shouldn't go smaller than HEAP_MIN_SIZE
    if ((new_end_addr - heap->start_addr) < HEAP_MIN_SIZE)
        new_end_addr = heap->start_addr + HEAP_MIN_SIZE;

    if (new_end_addr > heap->start_addr + heap->max_size)
        return 0; // Beyond heap's max end address

    // Make sure that kmalloc increases placement_address each time by using while loop
    uint32_t temp_end_addr = heap->end_addr - PAGE_SIZE;
    while (new_end_addr < temp_end_addr)
    {
        free_frame(get_page(temp_end_addr, kernel_pd, false));
        temp_end_addr -= PAGE_SIZE;
    }

    heap->end_addr = new_end_addr;
    return heap->end_addr - heap->start_addr; // Success
}

/// @brief Allocate a size of memory on a heap area
/// @param heap The pointer to the heap to allocate memory from
/// @param page_align Should the allocated memory be page-aligned or not
/// @param size In bytes
/// @return The base address of memory allocated
static uint32_t heap_alloc(heap_t *heap, bool page_align, size_t size)
{
    uint32_t alloc_addr;
    header_t *header;

    // If page-align specified, add PAGE_SIZE - 1 (Max alignment offset) to size for alignment
    avl_node_t *best_fit = (page_align) ? find_best_fit(heap->root, size + (size_t)(PAGE_SIZE - 1) + sizeof(header_t)) : find_best_fit(heap->root, size + sizeof(header_t));

    if (!best_fit)
    {
        alloc_addr = heap->end_addr;

        // Page-align if specified
        if (page_align)
            alloc_addr = ALIGN(alloc_addr + sizeof(header_t), PAGE_SIZE) - sizeof(header_t);

        // Expand heap if possible
        if (expand_heap(heap, (size_t)(alloc_addr - heap->end_addr) + sizeof(header_t) + size) == 0) // Return 0
            return (uint32_t)NULL;                                                                   // Out of memory

        // Store metadata before the allocated block
        header = (header_t *)alloc_addr;
        header->magic = HEAP_MAGIC;
        header->size = size;
    }
    else
    {
        alloc_addr = best_fit->base;

        // If page-alignment is required and alloc_addr is misaligned
        if (page_align && (alloc_addr & (PAGE_SIZE - 1)) != (PAGE_SIZE - sizeof(header_t)))
        {
            uint32_t aligned_addr = ALIGN(alloc_addr + sizeof(header_t), PAGE_SIZE) - sizeof(header_t);
            uint32_t alignment_offset = aligned_addr - alloc_addr;

            // Create a new fragment block before the aligned region if it's large enough
            if (alignment_offset > sizeof(header_t))
                heap->root = avl_insert(heap->root, alloc_addr, (size_t)alignment_offset);

            // Adjust the best-fit block
            alloc_addr = aligned_addr;
            best_fit->base = alloc_addr + sizeof(header_t) + size;
            best_fit->size -= (alignment_offset + sizeof(header_t) + size);
        }
        else
        {
            // Normal allocation from best-fit block
            best_fit->base += size + sizeof(header_t);
            best_fit->size -= size + sizeof(header_t);
        }

        // Remove the block if it's completely used
        if (best_fit->size == 0)
            heap->root = avl_delete(heap->root, best_fit->base);

        // Store metadata before the allocated block
        header = (header_t *)alloc_addr;
        header->magic = HEAP_MAGIC;
        header->size = size;
    }

    // Return address of actual data excluding header
    return alloc_addr + (uint32_t)sizeof(header_t);
}

/// @brief Free a size of memory located at the base address; basically a wrapper for avl_insert() with some coalescing (merging) feature
/// @param heap The pointer to the heap to free memory from
/// @param base Base address of memory to free
/// @param size In bytes
void heap_free(heap_t *heap, uint32_t base, size_t size)
{
    // Validate input; prevent out-of-bounds or zero-sited frees
    if (size == 0 || base < heap->start_addr || (base + size) > heap->end_addr)
    {
        printf("Invalid free request: Base=%x, Size=%x\n", base, size);
        return;
    }

    // Verify that the entire requested region is free
    avl_node_t *current = heap->root;
    while (current)
    {
        // Check if any allocated block overlaps with the range being freed
        if ((base >= current->base && base < current->base + current->size) ||               // Base inside allocated block
            (base + size > current->base && base + size <= current->base + current->size) || // End inside allocated block
            (base < current->base && base + size > current->base + current->size))           // Completely overlaps
        {
            printf("Overlapping free detected at %x (Size: %x)\n", base, size);
            return;
        }

        if (base < current->base)
            current = current->left;
        else
            current = current->right;
    }

    // Find potential neighboring free blocks
    avl_node_t *left_neighbor = NULL, *right_neighbor = NULL;
    current = heap->root;

    while (current)
    {
        if (current->base + current->size == base)
            left_neighbor = current;
        else if ((base + size) == current->base)
            right_neighbor = current;

        if (left_neighbor && right_neighbor)
            break;

        if (base < current->base)    // If the target node is located on the left side of current
            current = current->left; // Move left
        else
            current = current->right; // Move right
    }

    // Merge with adjacent free blocks
    if (left_neighbor)
    {
        base = left_neighbor->base;
        size += left_neighbor->size;
        heap->root = avl_delete(heap->root, left_neighbor->base);
    }
    if (right_neighbor)
    {
        size += right_neighbor->size;
        heap->root = avl_delete(heap->root, right_neighbor->base);
    }

    // Check if heap->root is NULL or not
    // I know this ternary is redundant but somehow it doesn't work otherwise
    heap->root = (heap->root) ? avl_insert(heap->root, base, size) : avl_insert(NULL, base, size);
}

/// @brief Allocate virtual memory on a heap; a wrapper for heap_alloc
/// @param size In bytes
/// @param page_align Should allocated memory be page-aligned or not
/// @param heap Heap to allocate memory from
/// @return The starting virtual address of allocated memory
void *malloc(uint32_t size, bool page_align, heap_t *heap)
{
    return (void *)heap_alloc(heap, page_align, size);
}

/// @brief Allocate virtual memory on kernel heap
/// @param size In bytes
/// @param page_align Should the allocated memory be page-aligned or not
/// @param paddr Physical address; 0 to ignore
/// @return Starting virtual address of kernel heap
uint32_t kmalloc(uint32_t size, bool page_align, uint32_t *paddr)
{
    if (kheap != 0)
    {
        void *vaddr = malloc(size, page_align, kheap);
        if (paddr != 0)
        {
            pte_t *page = get_page((uint32_t)vaddr, kernel_pd, false);
            *paddr = ((uint32_t)*page & 0xFFFFF000) + ((uint32_t)vaddr & 0xFFF);
        }

        return (uint32_t)vaddr;
    }
    else
    {
        // add header here!
        if (page_align)
            placement_address = ALIGN(placement_address, FRAME_SIZE);

        if (paddr)
            *paddr = placement_address;

        uint32_t temp = placement_address;
        placement_address += size;
        return temp;
    }
}

/// @brief Free memory previously allocated on a heap area; a wrapper for heap_free
/// @param heap The pointer to heap to free memory from
/// @param ptr The pointer to the memory base
void free(heap_t *heap, void *ptr)
{
    if (!heap || !ptr) // NULL pointer or Zero size!
        return;

    header_t *header = (header_t *)((size_t)ptr - sizeof(header_t));

    if (header->magic != HEAP_MAGIC)
    {
        printf("Invalid magic number\n");
        return;
    }

    // Get the full block size (including header)
    size_t block_size = header->size + sizeof(header_t);
    heap_free(heap, (uint32_t)header, block_size);
}

/// @brief Free memory area allocated on kernel heap
/// @param ptr The pointer to the memory base
/// @param size Size of contiguous memory to free
void kfree(void *ptr)
{
    return free(kheap, ptr);
}

/// @brief Switch page directory
/// @param pd The pointer to page directory to switch to
void switch_page_directory(page_directory_t *pd)
{
    uint32_t cr0;
    current_pd = pd;

    asm volatile("mov %0, %%cr3" ::"r"(pd->paddr_pde) : "memory"); // Flush TLB
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; // Enable paging
    asm volatile("mov %0, %%cr0" ::"r"(cr0));
}

/// @brief Copy page to clone physically; defined in process.S
extern void copy_physical_page(uint32_t, uint32_t);

/// @brief Clone a page table
/// @param src_pt The pointer to page table to make clone of
/// @param paddr The pointer to the clone page table
/// @return The address of cloned page table
page_table_t *clone_page_table(page_table_t *src_pt, uint32_t *paddr)
{
    // Create a new page-aligned page table
    page_table_t *dest_pt = (page_table_t *)kmalloc(sizeof(page_table_t), true, paddr);
    memset(dest_pt, 0, sizeof(page_table_t));

    // Copy info for every entry in the table
    for (int i = 0; i < PT_ENTRIES; i++)
    {
        if ((src_pt->entries[i] >> 12) == 0) // If the source PFN is 0, do nothing
        {
            dest_pt->entries[i] = (uint32_t)PAGE_RW;
            continue;
        }

        // Get a new frame for the page, not kernel, not writable
        alloc_frame(&dest_pt->entries[i], false, false);

        // Clone the flags
        if (src_pt->entries[i] & PAGE_PRESENT)
            dest_pt->entries[i] |= PAGE_PRESENT;
        if (src_pt->entries[i] & PAGE_RW)
            dest_pt->entries[i] |= PAGE_RW;
        if (src_pt->entries[i] & PAGE_USER)
            dest_pt->entries[i] |= PAGE_USER;
        if (src_pt->entries[i] & PAGE_ACCESSED)
            dest_pt->entries[i] |= PAGE_ACCESSED;
        if (src_pt->entries[i] & PAGE_DIRTY)
            dest_pt->entries[i] |= PAGE_DIRTY;

        // Copy physical memory pointed by src_pt to the dest_pt; process.S
        copy_physical_page((src_pt->entries[i] >> 12) * FRAME_SIZE, (dest_pt->entries[i] >> 12) * FRAME_SIZE);
    }

    return dest_pt;
}

/// @brief Clone a page directory
/// @param src_pd The pointer to page directory to make clone of
/// @return The address of cloned page directory
page_directory_t *clone_page_directory(page_directory_t *src_pd)
{
    uint32_t paddr;

    // Make a new page directory and obtain its physical address
    page_directory_t *dest_pd = (page_directory_t *)kmalloc(sizeof(page_directory_t), true, &paddr);
    memset(dest_pd, 0, sizeof(page_directory_t));

    // Get the physical address of the PDE
    uint32_t offset = (uint32_t)dest_pd->entries - (uint32_t)dest_pd;
    dest_pd->paddr_pde = paddr + offset;

    // Copy each table unless it is only TABLE_RW (initial value)
    for (int i = 0; i < PD_ENTRIES; i++)
    {
        if (!src_pd->tables[i])
        {
            dest_pd->entries[i] = (uint32_t)TABLE_RW;
            continue;
        }

        // Decide to link or copy a page;
        // link if it's part of the kernel (included in kernel_pd), otherwise copy it
        if (src_pd->tables[i] == kernel_pd->tables[i])
        {
            // Link the page using pointer
            dest_pd->tables[i] = src_pd->tables[i];
            dest_pd->entries[i] = src_pd->entries[i];
        }
        else
        {
            // Copy the page
            dest_pd->tables[i] = clone_page_table(src_pd->tables[i], &paddr);
            dest_pd->entries[i] = paddr |
                                  TABLE_PRESENT |
                                  TABLE_RW |
                                  TABLE_USER; // Set writable, user, present
        }
    }

    return dest_pd;
}

/// @brief Get a page from a page directory
/// @param vaddr Virtual address of a page to retrieve
/// @param pd Page directory to retrieve a page from
/// @param create Whether to create a new page if not exist
/// @return Page (page table entry); NULL if not exist
pte_t *get_page(uint32_t vaddr, page_directory_t *pd, bool create)
{
    // Get the index of page directory and page table
    uint32_t pd_index = vaddr >> 22;            // Top 10 bits
    uint32_t pt_index = (vaddr >> 12) & 0x03FF; // Middle 10 bits

    // If the page table doesn't exist, create it if allowed
    if (!pd->tables[pd_index])
    {
        if (!create)
            return NULL; // Page table not found and not allowed to create it

        // Allocate a new page table
        uint32_t paddr;
        page_table_t *new_table = (page_table_t *)kmalloc(sizeof(page_table_t), true, &paddr);
        if (!new_table)
            return NULL; // Memory allocation failed

        // Initialize the new table with not present pages
        for (int i = 0; i < PT_ENTRIES; i++)
            new_table->entries[i] = PAGE_RW; // Mark pages as RW (not present)

        // Store the new table in the directory
        pd->tables[pd_index] = new_table;
        pd->entries[pd_index] = paddr |
                                TABLE_PRESENT |
                                TABLE_RW;
    }

    // Return the page entry from the table
    return &pd->tables[pd_index]->entries[pt_index];
}

/// @brief Handler for page_fault; registered in ISR handler
/// @param regs
void page_fault(struct regs *regs)
{
    printf("invoked!\n");

    // Faulting address is stored in CR2 register
    uint32_t faulting_address;
    asm volatile("mov %%cr2, %0" : "=r"(faulting_address));

    printf("oops!\n");

    // Analyze the error code
    bool present = !(regs->err_code & PAGE_PRESENT); // Page not present
    bool rw = regs->err_code & PAGE_RW;              // Write operation
    bool us = regs->err_code & PAGE_USER;            // User-mode
    bool reserved = regs->err_code & PAGE_WRITETHRU; // Overwritten CPU reserved page
    bool id = regs->err_code & PAGE_CACHE_DIS;       // Instruction fetch
    id = id;                                         // Avoid warning

    printf("Page fault (%s%s%s%s) at %x\n",
           present ? "present" : "",
           rw ? "read-only" : "",
           us ? "user-mode" : "",
           reserved ? "reserved" : "",
           faulting_address);

    if (!present && !rw && !us)
        printf("\nSupervisory process tried to read a non-present page entry.");
    else if (!present && !rw && us)
        printf("\nSupervisory process tried to read a page and cause a protection fault.");
    else if (!present && rw && !us)
        printf("\nSupervisory process tried to write to a non-present page entry.");
    else if (!present && rw && us)
        printf("\nSupervisory process tried to write a page and caused a protection fault.");
    else if (present && !rw && !us)
        printf("\nUser Process tried to read a non-present page entry.");
    else if (present && !rw && us)
        printf("\nUser process tried to read a page and caused a protection fault.");
    else if (present && rw && !us)
        printf("\nUer process tried to write to a non-present page entry.");
    else if (present && rw && us)
        printf("\nUser process tried to write a page and caused a protection fault.");
    printf("\n");

    PANIC("Page fault");
}

/// @brief Allocate a frame to a page
/// @param page Pointer to page table entry with a proper format
/// @param kernel If the page is kernel or not
/// @param writable If the page is writable or read-only
void alloc_frame(pte_t *page, bool kernel, bool writable)
{
    if (PTE_TO_PFN(*page) != (uint32_t)0) // If already allocated
        return;

    // Allocate physical frame and get its PFN
    int32_t pfn = pmalloc(FRAME_SIZE);
    if (pfn == -1)
        PANIC("No free frames");

    *page = (uint32_t)pfn << 12 | PAGE_PRESENT;
    if (!kernel)
        *page |= PAGE_USER; // Set user flag
    if (writable)
        *page |= PAGE_RW; // Set RW flag
}

/// @brief Free a frame from a page
/// @param page
void free_frame(pte_t *page)
{
    uint32_t pfn = PTE_TO_PFN(*page);
    if (!pfn) // If already freed
        return;

    // Free physical frame
    pfree(pfn, FRAME_SIZE);
    *page &= PAGE_RW; // Only RW bit is set
};

/// @brief Create a new AVL node
/// @param base Virtual address
/// @param size Size of the block
/// @return Virtual address for the AVL node
static avl_node_t *create_node(uint32_t base, size_t size)
{
    // avl_node_t *node;
    avl_node_t *node = (avl_node_t *)kmalloc(sizeof(avl_node_t), false, 0); // DEFINE malloc!!!!!
    node->base = base;
    node->size = size;
    node->height = 1;
    node->left = node->right = NULL;

    return node;
}

/// @brief Get height of a node
/// @param node
/// @return
static int height(avl_node_t *node)
{
    return node ? node->height : 0;
}

/// @brief Get balance factor of a node
/// @param node
/// @return
static int get_balance(avl_node_t *node)
{
    return node ? height(node->left) - height(node->right) : 0;
}

/// @brief Right rotate subtree rooted with y
/// @param y
/// @return
static avl_node_t *rotate_right(avl_node_t *y) // A
{
    avl_node_t *x = y->left;   // B
    avl_node_t *T2 = x->right; // B's right subtree
    x->right = y;
    y->left = T2;
    y->height = 1 + ((height(y->left) > height(y->right)) ? height(y->left) : height(y->right));
    x->height = 1 + ((height(x->left) > height(y->right)) ? height(x->left) : height(x->right));
    return x;
}

/// @brief Left rotate subtree rooted with x
/// @param y
/// @return
static avl_node_t *rotate_left(avl_node_t *x) // A
{
    avl_node_t *y = x->right; // B
    avl_node_t *T2 = y->left; // B's left subtree
    y->left = x;
    x->right = T2;
    x->height = 1 + ((height(x->left) > height(y->right)) ? height(x->left) : height(x->right));
    y->height = 1 + ((height(y->left) > height(y->right)) ? height(y->left) : height(y->right));
    return y;
}

/// @brief Insert a new virtual address block into the AVL tree
/// @param node
/// @param base Base virtual address to insert into
/// @param size
/// @return The pointer to the node; node if error
avl_node_t *avl_insert(avl_node_t *node, uint32_t base, size_t size)
{
    // Input validation
    if (size == 0)
        return node;

    if (!node)
        return create_node(base, size);

    if (base < node->base)
        node->left = avl_insert(node->left, base, size);
    else if (base > node->base)
        node->right = avl_insert(node->right, base, size);
    else
        return node; // Duplicate base address

    // Update height
    node->height = 1 + ((height(node->left) > height(node->right)) ? height(node->left) : height(node->right));

    // Get balance factor and balance the tree
    int balance = get_balance(node);

    // Left heavy (right rotation)
    if (balance > 1 && base < node->left->base)
        return rotate_right(node);

    // Right heavy (left rotation)
    if (balance < -1 && base > node->right->base)
        return rotate_left(node);

    // Left-Right
    if (balance > 1 && base > node->left->base)
    {
        node->left = rotate_left(node->left);
        return rotate_right(node);
    }

    // Right-Left
    if (balance < -1 && base < node->right->base)
    {
        node->right = rotate_right(node->right);
        return rotate_left(node);
    }

    return node;
}

/// @brief Find the node with the smallest base (used for deletion)
/// @param node
/// @return
avl_node_t *find_min_base(avl_node_t *node)
{
    while (node->left)
        node = node->left;
    return node;
}

/// @brief Delete a virtual address block from AVL tree
/// @param root
/// @param base The virtual address of the block to delete
/// @return The pointer to new root node; NULL if the last-remained node
avl_node_t *avl_delete(avl_node_t *root, uint32_t base)
{
    if (!root)
        return root;

    if (base < root->base)
        root->left = avl_delete(root->left, base);
    else if (base > root->base)
        root->right = avl_delete(root->right, base);
    else // now root is the target of deletion
    {
        if (!root->left && !root->right) // Leaf node
        {
            // kfree(root);
            return NULL;
        }
        else if (!root->left || !root->right) // Only one child exists
        {
            avl_node_t *temp = root->left ? root->left : root->right;
            // kfree(root);
            return temp;
        }

        avl_node_t *temp = find_min_base(root->right);
        root->base = temp->base;
        root->size = temp->size;
        root->right = avl_delete(root->right, temp->base);
    }

    if (!root)
        return root;

    root->height = 1 + ((height(root->left) > height(root->right)) ? height(root->left) : height(root->right));
    int balance = get_balance(root);

    if (balance > 1 && get_balance(root->left) >= 0)
        return rotate_right(root);

    if (balance < -1 && get_balance(root->right) <= 0)
        return rotate_left(root);

    if (balance > 1 && get_balance(root->left) < 0)
    {
        root->left = rotate_left(root->left);
        return rotate_right(root);
    }

    if (balance < -1 && get_balance(root->right) > 0)
    {
        root->right = rotate_right(root->right);
        return rotate_left(root);
    }

    return root;
}

/// @brief Search for a best-fit (smallest) block for given size recursively
/// @param node AVL root node to search inside
/// @param size In bytes
/// @return The pointer to the best-fit node; NULL if not found
avl_node_t *find_best_fit(avl_node_t *node, size_t size)
{
    if (!node)
        return NULL;

    avl_node_t *best = NULL;
    if (node->size >= size)
    {
        best = node;
        avl_node_t *left_best = find_best_fit(node->left, size);
        if (left_best && left_best->size < best->size)
            best = left_best;
    }

    // Return left-most node if available
    return best ? best : find_best_fit(node->right, size);
}

void free_avl_tree(avl_node_t *root)
{
    if (!root)
        return;

    free_avl_tree(root->left);
    free_avl_tree(root->right);
    // kfree(root);
}
