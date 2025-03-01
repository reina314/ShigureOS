#ifndef _STDLIB_H
#define _STDLIB_H 1

#include <stdint.h>
#include <sys/cdefs.h>

#ifdef __cplusplus
extern "C"
{
#endif

// Types generally used
#define NULL ((void *)0)
    typedef unsigned long size_t;

    __attribute__((__noreturn__)) void abort(void);

    void outb(uint16_t port, uint8_t value); // defined in io.c
    uint8_t inb(uint16_t port);              // defined in io.c

// Invoke panic and cause the kernel into infinite loop
#define PANIC(msg) panic(msg, __FILE__, __LINE__);
// Evaluate the expression and invoke panic if false
#define ASSERT(b) ((b) ? (void)0 : panic_assert(__FILE__, __LINE__, #b));

    extern void panic(const char *, const char *, uint32_t);
    extern void panic_assert(const char *, uint32_t, const char *);

// Round up a value to the nearest alignment boundary
#define ALIGN(value, alignment) (((value) + (alignment - 1)) & ~(alignment - 1))

#ifdef __cplusplus
}
#endif

#endif
