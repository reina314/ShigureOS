#include <kernel/vm.h>
#include <kernel/pm.h>
#include <kernel/isr.h>
#include <stdint.h>
#include <stdbool.h>
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

void vm_initialize(void)
{
    frame_initialize(); // Initialize frame allocator first; defined in pm.c

    // Initialize kernel_pd
    kernel_pd = (page_directory_t *)kmalloc(sizeof(page_directory_t), true, 0);
    memset(kernel_pd, 0, sizeof(page_directory_t));
    for (int index = 0; index < PD_ENTRIES; index++)
        kernel_pd->entries[index] = (uint32_t)TABLE_RW;

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
    load_page_directory(kernel_pd->entries);
    enable_paging();

    // Implement kheap!!!
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
        if (!paddr)
        {
            ;
        }

        return (uint32_t)vaddr;
    }
    else
    {
        if (page_align && ((placement_address & (FRAME_SIZE - 1)) != 0)) // Not page-aligned; 0x1000 - 0x1 = 0xFFF
        {
            // Page-align placement_address
            placement_address &= ~(FRAME_SIZE - 1); // ~(FRAME_SIZE - 1) = ~0xFFF = 0xFFFFF000
            placement_address += FRAME_SIZE;
        }

        if (paddr)
            *paddr = placement_address;

        uint32_t temp = placement_address;
        placement_address += size;
        return temp;
    }
}

/// @brief Free memory area allocated on kernel heap
/// @param p
void kfree(void *p)
{
    ;
}

/// @brief Allocate virtual memory on a heap and return its starting virtual address
/// @param size In bytes
/// @param page_align Should allocated memory be page-aligned or not
/// @param heap Heap to allocate memory from
/// @return
void *malloc(uint32_t size, bool page_align, heap_t *heap)
{
    ;
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
    // Faulting address is stored in CR2 register
    uint32_t faulting_address;
    asm volatile("mov %%cr2, %0" : "=r"(faulting_address));

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
    int32_t pfn = pmalloc(PAGE_SIZE);
    if (pfn == -1)
        PANIC("No free frames");

    *page = (uint32_t)pfn << 12 | PAGE_PRESENT;
    if (!kernel)
        *page |= PAGE_USER; // Set user flag
    if (writable)
        *page |= PAGE_RW; // Set RW flag
}

/// @brief Create a new AVL node
/// @param base Virtual address
/// @param size Size of the block
/// @return Virtual address for the AVL node
avl_node_t *create_node(uint32_t base, size_t size)
{
    // avl_node_t *node;
    avl_node_t *node = (avl_node_t *)kmalloc(sizeof(avl_node_t), false, 0); // DEFINE malloc!!!!!
    node->base = base;
    node->size = size;
    node->height = 1;
    node->left = node->right = NULL;

    printf("ok!\n");
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
/// @return
avl_node_t *insert(avl_node_t *node, uint32_t base, size_t size)
{
    if (!node)
        return create_node(base, size);

    if (base < node->base)
        node->left = insert(node->left, base, size);
    else if (base > node->base)
        node->right = insert(node->right, base, size);
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
avl_node_t *find_min(avl_node_t *node)
{
    while (node->left)
        node = node->left;
    return node;
}

/// @brief Delete a virtual address block from AVL tree
/// @param root
/// @param base The virtual address of the block to delete
/// @return
avl_node_t *delete(avl_node_t *root, uint32_t base)
{
    if (!root)
        return root;

    if (base < root->base)
        root->left = delete (root->left, base);
    else if (base > root->base)
        root->right = delete (root->right, base);
    else
    {
        if (!root->left || !root->right)
        {
            avl_node_t *temp = root->left ? root->left : root->right;
            kfree(root); // DEFINE free !!!
            return temp;
        }

        avl_node_t *temp = find_min(root->right);
        root->base = temp->base;
        root->size = temp->size;
        root->right = delete (root->right, temp->base);
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

/// @brief Find a free block for allocation
/// @param root
/// @param size
/// @return NULL if not found
avl_node_t *allocate(avl_node_t *root, size_t size)
{
    if (!root)
        return NULL;

    if (root->size >= size)
    {
        avl_node_t *allocated = create_node(root->base, size);
        root = delete (root, root->base);
        return allocated;
    }

    avl_node_t *left_result = allocate(root->left, size);
    if (left_result)
        return left_result;

    return allocate(root->right, size);
}

/// @brief Print AVL tree in-order traversal
/// @param root
void inorder(avl_node_t *root)
{
    if (!root)
        return;

    inorder(root->left);
    printf("Base: %x, Size: %d\n", root->base, root->size);
    inorder(root->right);
}

void free_tree(avl_node_t *root)
{
    if (!root)
        return;

    free_tree(root->left);
    free_tree(root->right);
    kfree(root); // DEFINE free!!!
}
