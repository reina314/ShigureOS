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

// Invoke panic and cause the kernel into infinite loop
#define PANIC(msg) panic(msg, __FILE__, __LINE__);
// Evaluate the expression and invoke panic if false
#define ASSERT(b) ((b) ? (void)0 : panic_assert(__FILE__, __LINE__, #b));

    extern void panic(const char *, const char *, uint32_t);
    extern void panic_assert(const char *, uint32_t, const char *);

    typedef void *type_t;
    typedef int8_t (*lessthan_predicate_t)(type_t, type_t);
    /// @brief Insertion sorted array; always remains in a sorted state between cells; pointer or anything
    typedef struct ordered_array
    {
        type_t *array;
        uint32_t size;
        uint32_t max_size;
        lessthan_predicate_t less_than;
    } ordered_array_t;

    // Create an ordered array
    ordered_array_t create_ordered_array(uint32_t, lessthan_predicate_t);
    ordered_array_t place_ordered_array(void *, uint32_t, lessthan_predicate_t);

    // Manipulate an ordered array
    void insert_ordered_array(type_t, ordered_array_t *);     // defined in ordered_array.c
    void remove_ordered_array(uint32_t, ordered_array_t *);   // defined in ordered_array.c
    type_t lookup_ordered_array(uint32_t, ordered_array_t *); // defined in ordered_array.c

#ifdef __cplusplus
}
#endif

#endif
