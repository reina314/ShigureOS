#include <stdint.h>

#ifndef _REGS_H
#define _REGS_H

/// @brief Defines the stack after running ISR
struct regs
{
    unsigned int gs, fs, ed, ds;
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax; // esp is useless
    unsigned int int_no, err_code;
    unsigned int eip, cs, eflags, usresp, ss;
};

typedef struct registers
{
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // esp is useless
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, usresp, ss;
} registers_t;

#endif