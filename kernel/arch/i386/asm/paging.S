.section .text

.global load_page_directory
.type load_page_directory, @function
load_page_directory:
    pushl %ebp
    movl %esp, %ebp
    movl 8(%esp), %eax   # Load first argument (page directory address)
    movl %eax, %cr3      # Set CR3 (page directory base register)
    movl %ebp, %esp
    popl %ebp
    ret

.global enable_paging
.type enable_paging, @function
enable_paging:
    pushl %ebp
    movl %esp, %ebp

    movl %cr0, %eax
    orl $0x80000000, %eax  # Set only bit 31 (Paging Enable - PG)
    movl %eax, %cr0

    # Flush TLB (important after enabling paging)
    movl %cr3, %eax
    movl %eax, %cr3

    movl %ebp, %esp
    popl %ebp
    ret
