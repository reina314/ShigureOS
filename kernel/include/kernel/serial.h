#ifndef _KERNEL_SERIAL_H
#define _KERNEL_SERIAL_H

#include <stdint.h>
#include <stdlib.h> // for inb

#define SERIAL_COM1_BASE 0x3F8

// all serial ports (COM1, COM2, COM3, COM4) have their ports in the same order relative to the base
#define SERIAL_DATA_PORT(base) (base)
#define SERIAL_FIFO_COMMAND_PORT(base) (base + 2)
#define SERIAL_LINE_COMMAND_PORT(base) (base + 3)
#define SERIAL_MODEM_COMMAND_PORT(base) (base + 4)
#define SERIAL_LINE_STATUS_PORT(base) (base + 5)

// Serial port will send the highest 8bit first and then the lowest 8bit
#define SERIAL_LINE_ENABLE_DLAB 0x80

void serial_configure_baud_rate(unsigned short, unsigned short);
void serial_configure_line(unsigned short);
void serial_configure_buffers(unsigned short);
void serial_configure_modem(unsigned short);
int is_serial_transmit_fifo_empty(unsigned int);
void serial_initialize(unsigned int);
void serial_write(unsigned int, char);
int is_serial_received(unsigned short);
char serial_read(unsigned short);

#endif