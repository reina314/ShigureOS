#include <stdlib.h>
#include <stdint.h>

/// @brief Write a byte out to the specified port
/// @param port I/O port number (16bit)
/// @param value The byte (8bit) to send to the port
void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile("outb %1, %0" ::"dN"(port), "a"(value));
}

/// @brief Read a byte from the specified port
/// @param port I/O port number (16bit)
/// @return A byte read for the port (8bit)
uint8_t inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "dN"(port));
    return ret;
}