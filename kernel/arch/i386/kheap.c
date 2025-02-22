#include <kernel/kheap.h>
#include <kernel/paging.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

extern uint32_t placement_address;
extern page_directory_t *kernel_directory;

/// @brief Kernel's heap memory
heap_t *kheap = 0;

/// @brief Checks if a->size is less than b->size
/// @param a
/// @param b
/// @return 1 or 0
static int8_t header_t_less_than(void *a, void *b)
{
    return (((header_t *)a)->size < ((header_t *)b)->size) ? 1 : 0;
}

/// @brief Create a heap area ranging from start_address to end_address
/// @param start_address Should be aligned with 0x1000
/// @param end_address Should be aligned with 0x1000
/// @param max Max size the heap can be resized to
/// @param supervisor If for kernel or not
/// @param readonly
/// @return
heap_t *create_heap(uint32_t start_address, uint32_t end_address, uint32_t max_address, bool supervisor, bool readonly)
{
    heap_t *heap = (heap_t *)kmalloc(sizeof(heap_t));

    ASSERT(start_address % 0x1000 == 0);
    ASSERT(end_address % 0x1000 == 0);

    // Initialize the index
    heap->index = place_ordered_array((void *)start_address, HEAP_INDEX_SIZE, &header_t_less_than);

    // Shift the start address forward until where to put data
    start_address += sizeof(type_t) * HEAP_INDEX_SIZE;

    // Make the start_address page-aligned
    if ((start_address && 0xFFF) != 0)
    {
        start_address &= 0xFFFFF000;
        start_address += 0x1000;
    }

    heap->start_address = start_address;
    heap->end_address = end_address;
    heap->max_address = max_address;
    heap->supervisor = supervisor;
    heap->readonly = readonly;

    // Start with a single large hole in the index
    header_t *hole = (header_t *)start_address;
    hole->size = end_address - start_address;
    hole->magic = HEAP_MAGIC;
    hole->is_hole = 1;
    insert_ordered_array((void *)hole, &heap->index);

    return heap;
}

/// @brief Finds the smallest hole available in virtual space
/// @param size
/// @param page_align if align or not; 0 with no alignment
/// @param heap
/// @return The heap->index the smallest hole resides; return -1 if could not find
static int32_t find_smallest_hole(uint32_t size, bool page_align, heap_t *heap)
{
    uint32_t iterator = 0;
    int32_t best_index = -1;
    uint32_t best_size = 0xFFFFFFFF;

    while (iterator < heap->index.size)
    {
        header_t *header = (header_t *)lookup_ordered_array(iterator, &heap->index);
        uint32_t location = (uint32_t)header;
        int32_t offset = 0;

        // If page alignment requested, page-align the starting point of the header
        if (page_align && ((location + sizeof(header_t)) & 0x00000FFF) != 0) // Check if lower 12 bit are set (misaligned) or not;
        {
            offset = 0x1000 - (location + sizeof(header_t)) % 0x1000; // 0x1000 is page size
        }

        int32_t hole_size = (int32_t)header->size - offset; // offset is used to align so should be subtracted from size

        // Check if the hole is large enough
        if (hole_size >= (int32_t)size)
        {
            if ((uint32_t)hole_size < best_size)
            {
                best_size = (uint32_t)hole_size;
                best_index = iterator;
            }
        }

        iterator++;
    }

    return best_index;
}

/// @brief Expand given heap to new_size
/// @param new_size
/// @param heap
static void expand(uint32_t new_size, heap_t *heap)
{
    uint32_t old_size = heap->end_address - heap->start_address;
    // Check if requested size is larger than current one
    ASSERT(new_size > old_size);

    // Obtain next nearest page boundary
    if ((new_size & 0x00000FFF) != 0) // Not aligned with 0x1000
    {
        new_size &= 0xFFFFF000; // Round down to nearest page boundary
        new_size += 0x1000;     // Round up
    }

    // Check if the new heap area not reach max_address
    ASSERT(heap->start_address + new_size <= heap->max_address);

    uint32_t i = old_size;
    while (i < new_size)
    {
        alloc_frame(get_page(heap->start_address + i, 1, kernel_directory),
                    (heap->supervisor) ? true : false,
                    (heap->readonly) ? false : true);
        i += 0x1000;
    }

    heap->end_address = heap->start_address + new_size;
}

/// @brief Allocates memory in the given heap
/// @param size Size of memory to allocate
/// @param page_align If align or not; 0 with no alignment
/// @param heap Address of heap to allocate from
/// @return Address of memory allocated
void *alloc(uint32_t size, bool page_align, heap_t *heap)
{
    // Add the size of header and footer
    uint32_t new_size = size + sizeof(header_t) + sizeof(footer_t);

    int32_t iterator = find_smallest_hole(new_size, page_align, heap);

    // If nothing found, need to expand the heap memory
    if (iterator == -1)
    {
        uint32_t old_length = heap->end_address - heap->start_address;
        uint32_t old_end_address = heap->end_address;

        expand(old_length + new_size, heap);
        uint32_t new_length = heap->end_address - heap->start_address;

        // Find the endmost header
        iterator = 0;
        int32_t index = -1;
        uint32_t value = 0x0;
        while (iterator < (int32_t)heap->index.size)
        {
            uint32_t tmp = (uint32_t)lookup_ordered_array(iterator, &heap->index); // tmp holds address
            if (tmp > value)
            {
                value = tmp;
                index = iterator;
            }

            iterator++;
        }

        // If no header found, then add one
        if (index == (int32_t)-1)
        {
            header_t *header = (header_t *)old_end_address;
            header->magic = HEAP_MAGIC;
            header->size = new_length - old_length;
            header->is_hole = 1;

            footer_t *footer = (footer_t *)(old_end_address + header->size - sizeof(footer_t));
            footer->magic = HEAP_MAGIC;
            footer->header = header;

            insert_ordered_array((void *)header, &heap->index);
        }
        else
        {
            // Adjust the last header
            header_t *header = lookup_ordered_array(index, &heap->index);
            header->size += new_length - old_length;

            // Rewrite the footer
            footer_t *footer = (footer_t *)((uint32_t)header + header->size - sizeof(footer_t));
            footer->header = header;
            footer->magic = HEAP_MAGIC;
        }

        // Now have enough memory space; recurse the function again to allocate memory
        return alloc(size, page_align, heap);
    }

    // Enough memory space found
    header_t *original_hole_header = (header_t *)lookup_ordered_array(iterator, &heap->index);
    uint32_t original_hole_position = (uint32_t)original_hole_header;
    uint32_t original_hole_size = original_hole_header->size;

    // Decide if split the hole into 2 parts or not
    // If the loss of the hole space is smaller than the overhead of creating a new hole
    if (original_hole_size - new_size < sizeof(header_t) + sizeof(footer_t))
    {
        // Just increase the requested size to the size of the hole found
        size += original_hole_size - new_size;
        new_size = original_hole_size;
    }

    // Page align the data as necessary and create a new hole in front of the block
    if (page_align && (original_hole_position & 0x00000FFF))
    {
        uint32_t new_location = original_hole_position + 0x1000 - (original_hole_position & 0xFFF) - sizeof(header_t);
        header_t *hole_header = (header_t *)original_hole_position;
        hole_header->size = 0x1000 - (original_hole_position & 0xFFF) - sizeof(header_t);
        hole_header->magic = HEAP_MAGIC;
        hole_header->is_hole = 1;

        footer_t *hole_footer = (footer_t *)((uint32_t)new_location - sizeof(footer_t));
        hole_footer->magic = HEAP_MAGIC;
        hole_footer->header = hole_header;

        original_hole_position = new_location;
        original_hole_size = original_hole_size - hole_header->size;
    }
    else
    {
        // This old hole is not required anymore so delete it from the array
        remove_ordered_array(iterator, &heap->index);
    }

    // Overwrite the original header
    header_t *block_header = (header_t *)original_hole_position;
    block_header->magic = HEAP_MAGIC;
    block_header->is_hole = 0;
    block_header->size = new_size;

    // Footer
    footer_t *block_footer = (footer_t *)(original_hole_position + sizeof(header_t) + size);
    block_footer->magic = HEAP_MAGIC;
    block_footer->header = block_header;

    // Add new hole after the allocated block if new_size is larger than the original
    if (original_hole_size - new_size > 0)
    {
        header_t *hole_header = (header_t *)(original_hole_position + sizeof(header_t) + size + sizeof(footer_t));
        hole_header->magic = HEAP_MAGIC;
        hole_header->is_hole = 1;
        hole_header->size = original_hole_size - new_size;

        footer_t *hole_footer = (footer_t *)((uint32_t)hole_header + hole_header->size - sizeof(footer_t));
        if ((uint32_t)hole_footer < heap->end_address)
        {
            hole_footer->magic = HEAP_MAGIC;
            hole_footer->header = hole_header;
        }

        // Put the new hole in the index
        insert_ordered_array((void *)hole_header, &heap->index);
    }

    return (void *)((uint32_t)block_header + sizeof(header_t));
}

/// @brief Allocates memory in the kernel heap
/// @param size Size of memory to allocate
/// @param align Should page align or not
/// @param physical_address Pointer to returned physical address; 0 if not necessary
/// @return
static uint32_t kmalloc_internal(uint32_t size, bool align, uint32_t *physical_address)
{
    if (kheap != 0)
    {
        void *addr = alloc(size, align, kheap);
        if (physical_address != 0)
        {
            page_t *page = get_page((uint32_t)addr, 0, kernel_directory);
            *physical_address = (page->frame * 0x1000 + ((uint32_t)addr & 0xFFF));
        }
        return (uint32_t)addr;
    }
    else
    {
        if (align && ((placement_address & 0x00000FFF) != 0))
        {
            // Align the placement address
            placement_address &= 0xFFFFF000;
            placement_address += 0x1000;
        }
        if (physical_address)
        {
            *physical_address = placement_address;
        }
        uint32_t tmp = placement_address;
        placement_address += size;
        return tmp;
    }
}

/// @brief Allocates page-aligned memory to the kernel heap
/// @param size
/// @return
uint32_t kmalloc_aligned(uint32_t size)
{
    return kmalloc_internal(size, true, 0);
}

/// @brief Allocates memory and returns the physical address in *physical_address
/// @param size
/// @param physical_address
/// @return
uint32_t kmalloc_physical(uint32_t size, uint32_t *physical_address)
{
    return kmalloc_internal(size, false, physical_address);
}

/// @brief Allocates page-aligned memory to the kernel heap
/// @param size
/// @param physical_address
/// @return The physical address is *physical_address
uint32_t kmalloc_aligned_physical(uint32_t size, uint32_t *physical_address)
{
    return kmalloc_internal(size, true, physical_address);
}

/// @brief Allocate unaligned memory to the kernel heap
/// @param size
/// @return
uint32_t kmalloc(uint32_t size)
{
    return kmalloc_internal(size, false, 0);
}
