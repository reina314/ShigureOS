#include <kernel/proc.h>
#include <kernel/vm.h>
#include <string.h>
#include <stdio.h> // for debug

/// @brief Currently running process
volatile proc_t *current_proc;
/// @brief Start of the process linked list
volatile proc_t *ready_queue;

extern page_directory_t *kernel_pd;                                                           // vm.c
extern page_directory_t *current_pd;                                                          // vm.c
extern uint32_t initial_esp;                                                                  // kernel.c
extern uint32_t get_eip();                                                                    // proc.S
extern void switch_physical_proc(uint32_t eip, uint32_t pd_pddr, uint32_t ebp, uint32_t esp); // procs.S

/// @brief Next available process id
uint32_t next_pid = 1;

/// @brief Initialize process system
/// @param
void proc_initialize(void)
{
    // Disable interrupts
    asm volatile("cli");

    // Relocate the stack to reset its base address
    move_stack((void *)0xE0000000, 0x4000);

    // Initialize the first process (kernel process)
    current_proc = ready_queue = (proc_t *)kmalloc(sizeof(proc_t), false, 0);
    current_proc->id = next_pid++;             // Set and increment
    current_proc->esp = current_proc->ebp = 0; // Init esp & ebp to 0
    current_proc->eip = 0;                     // Init eip to 0
    current_proc->pd = current_pd;
    current_proc->next = 0; // This is the only process so far

    // Enable interrupts
    asm volatile("sti");
}

/// @brief Create a new process; Clone parent address space and start child at the same EIS as its parent
/// @return PID
int fork(void)
{
    // Disable interrupts
    asm volatile("cli");

    // Create a pointer to the parent's proc structure for later reference
    proc_t *parent_proc = (proc_t *)current_proc;

    // Clone parent address space
    page_directory_t *pd = clone_page_directory(current_proc->pd);

    // Create a new proc
    proc_t *new_proc = (proc_t *)kmalloc(sizeof(proc_t), false, 0);
    new_proc->id = next_pid++;
    new_proc->esp = new_proc->ebp = 0;
    new_proc->eip = 0;
    new_proc->pd = pd;
    new_proc->next = 0; // This is the latest proc

    // Add this proc to the end of ready_queue
    proc_t *temp_proc = (proc_t *)ready_queue;
    while (temp_proc->next)
        temp_proc = temp_proc->next;
    temp_proc->next = new_proc;

    // Get the instruction pointer (entry point) for new proc
    uint32_t eip = get_eip();

    // Switch either to the parent or the child proc
    if (current_proc == parent_proc)
    {
        // Still parent
        uint32_t esp, ebp;
        asm volatile("mov %%esp, %0" : "=r"(esp)); // Read esp register
        asm volatile("mov %%ebp, %0" : "=r"(ebp)); // Read ebp register

        new_proc->esp = esp;
        new_proc->ebp = ebp;
        new_proc->eip = eip;

        // Enable interrupts
        asm volatile("sti");

        // Return PID of child proc
        return new_proc->id;
    }
    else
    {
        // Already fork is done; return 0 from child proc
        return 0;
    }
}

/// @brief Relocate stack memory
/// @param new_stack_start
/// @param size
void move_stack(void *new_stack_start, size_t size)
{
    // Allocate space for new stack (stack grows downward)
    for (uint32_t i = (uint32_t)new_stack_start; i >= (uint32_t)(new_stack_start - size); i -= PAGE_SIZE)
        // user and writable
        alloc_frame(get_page(i, current_pd, true), false, true);

    // Paging has changed (new stack pages allocated)
    // So flush TLB; read cr3 and write it back
    uint32_t pd_addr;
    asm volatile("mov %%cr3, %0" : "=r"(pd_addr)); // Read cr3
    asm volatile("mov %0, %%cr3" ::"r"(pd_addr));  // Write back to cr3

    // Read the current stack and base pointer
    uint32_t old_stack_ptr, old_base_ptr;
    asm volatile("mov %%esp, %0" : "=r"(old_stack_ptr));
    asm volatile("mov %%ebp, %0" : "=r"(old_base_ptr));

    // Calculate new stack and base pointer
    uint32_t offset = (uint32_t)new_stack_start - initial_esp;
    uint32_t new_stack_ptr = old_stack_ptr + offset;
    uint32_t new_base_ptr = old_base_ptr + offset;

    printf("\nold_stack_pointer : %x", old_stack_ptr);
    printf("\nold_base_pointer  : %x", old_base_ptr);
    printf("\noffset            : %x", offset);
    printf("\nnew_stack_pointer : %x", new_stack_ptr);
    printf("\nnew_base_pointer  : %x\n", new_base_ptr);

    // Copy the old stack to the new location
    memcpy((void *)new_stack_ptr, (void *)old_stack_ptr, (size_t)(initial_esp - old_stack_ptr));

    // Backtrace through the original stack;
    // EBP pushed in the stack still points to the old address so fix them
    for (uint32_t *ptr = (uint32_t *)new_stack_start; ptr > (uint32_t *)(new_stack_start - size); ptr--)
    {
        // If the value of temp is inside the range of the old stack, assume it is a base pointer
        // and remap it. This will unfortunately remap ANY value in this range, whether they are
        // base pointers or not.
        if ((old_stack_ptr < *ptr) && (*ptr < initial_esp))
            *ptr += offset;
    }

    // Change stacks
    asm volatile("mov %0, %%esp" ::"r"(new_stack_ptr));
    asm volatile("mov %0, %%ebp" ::"r"(new_base_ptr));
}

/// @brief Switch to the next process on ready_queue
/// @param r
void switch_proc(struct regs *r)
{
    r = r; // Avoid warning

    // If no process initialized then return
    if (!current_proc)
        return;

    // Read esp, ebp and eip for saving them later
    uint32_t esp, ebp, eip;
    asm volatile("mov %%esp, %0" : "=r"(esp));
    asm volatile("mov %%ebp, %0" : "=r"(ebp));
    // Read the instruction pointer. We do some cunning logic here:
    // One of two things could have happened when this function exits -
    //   (a) We called the function and it returned the EIP as requested.
    //   (b) We have just switched tasks, and because the saved EIP is essentially
    //       the instruction after get_eip(), it will seem as if get_eip has just
    //       returned.
    // In the second case we need to return immediately. To detect it we put a dummy
    // value in EAX further down at the end of this function. As C returns values in EAX,
    // it will look like the return value is this dummy value! (0xDEAD).
    eip = get_eip();

    if (eip == 0xDEAD)
        // Process was swtiched so return now
        return;

    // If not switched, save register values and then switch
    current_proc->eip = eip;
    current_proc->esp = esp;
    current_proc->ebp = ebp;

    // Get the next proc to run
    current_proc = current_proc->next;

    // Check if it ran out of the linked list, and start again from the beginning
    if (!current_proc)
        current_proc = ready_queue;

    eip = current_proc->eip;
    esp = current_proc->esp;
    ebp = current_proc->ebp;

    current_pd = current_proc->pd;

    // Set the cpu registers eip, esp, and ebp to the current proc registers
    switch_physical_proc(eip, current_pd->paddr_pde, ebp, esp);
}

/// @brief Return the pid of currently runncing process
/// @return PID
int getpid(void)
{
    return current_proc->id;
}
