#ifndef _STDLIB_H
#define _STDLIB_H 1

#include <stdint.h>
#include <sys/cdefs.h>

#ifdef __cplusplus
extern "C"
{
#endif

    __attribute__((__noreturn__)) void abort(void);

    void outb(uint16_t port, uint8_t value); // defined in io.c
    uint8_t inb(uint16_t port);              // defined in io.c

    unsigned int round_up_to_multiple(unsigned int, unsigned int); // defined in round.c

    typedef void *type_t;
    typedef char (*lessthan_predicate_t)(type_t, type_t);
    /// @brief Insertion sorted array; always remains in a sorted state between cells; pointer or anything
    typedef struct ordered_array
    {
        type_t *array;
        uint32_t size;
        uint32_t max_size;
        lessthan_predicate_t less_than;
    } ordered_array_t;

#ifdef __cplusplus
}
#endif

#endif
