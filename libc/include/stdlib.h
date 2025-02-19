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

#ifdef __cplusplus
}
#endif

#endif
