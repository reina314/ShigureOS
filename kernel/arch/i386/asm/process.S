.section .text
.global copy_physical_page
.type copy_physical_page, @function
copy_physical_page:
    pushl %ebx  # Preserve the content of %ebx (__cdcel calling convention)
    pushfl      # Save EFLAGS

    cli                 # Disable interrupts; need to set these before disabling paging
    movl 12(%esp), %ebx # Load source address 32bits (esp + 12)
    movl 16(%esp), %ecx # Load destination address 32bits (esp + 16)

    movl %cr0, %edx         # Get the control of CR0
    andl $0x7fffffff, %edx  # Clear the paging bit (bit 31)
    movl %edx, %cr0         # Disable paging

    movl $1024, %edx        # 1024 * 4 bytes to copy

.loop:
    movl (%ebx), %eax # Load dword(32bits) from source
    movl %eax, (%ecx) # Store dword(32bits) to destination
    addl $4, %ebx     # Increment source address
    addl $4, %ecx     # Increment destination address
    decl %edx         # Decrement counter
    jnz .loop         # Loop if not zero

    movl %cr0, %edx         # Get CR0 again
    orl $0x80000001, %edx   # Set the paging bit (bit 31) and protection bit (bit 1)
    movl %edx, %cr0         # Enable paging

    popfl       # Restore EFLAGS
    popl %ebx   # Restore %ebx
    ret


