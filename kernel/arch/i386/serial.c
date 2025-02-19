#include <kernel/serial.h>
#include <stdint.h>

/// @brief Sets the baud rate of given serial port; the rate will be 115200 / (divisor) bit/s
/// @param com COM port to configure
/// @param divisor Divisor
void serial_configure_baud_rate(unsigned short com, unsigned short divisor)
{
    outb(SERIAL_LINE_COMMAND_PORT(com), SERIAL_LINE_ENABLE_DLAB);
    outb(SERIAL_DATA_PORT(com), (divisor >> 8) & 0x00FF);
    outb(SERIAL_DATA_PORT(com), divisor & 0x00FF);
}

/* Bit:      | 7 | 6 | 5 4 3 | 2 | 1 0 |
 * Content   | d | b | prty  | s |  dl |
 * Value     | 0 | 0 | 0 0 0 | 0 | 1 1 | = 0x03 */
/// @brief Given port will have a data len of 8 bits without parity bits, and 1 stop bit with break control disabled
/// @param com COM port to configure
void serial_configure_line(unsigned short com)
{
    outb(SERIAL_LINE_COMMAND_PORT(com), 0x03);
}

/** serial_configure_buffers:
 * Configures the buffers of the given serial port.
 * lvl: How many bytes should be stored in the FIFO buffers
 * bs: If the buffers should be 16 or 64 bytes large
 * r: Reserved for future use
 * dma: How the serial port data should be accessed
 * clt: Clear the transmission FIFO buffer
 * clr: Clear the receiver FIFO buffer
 * e: If the FIFO buffer should be enabled or not
 */

/// @brief Configures the given port's buffers
/// @param com COM port to configure
void serial_configure_buffers(unsigned short com)
{
    /*
     * Bit:     | 7 6 | 5  | 4 | 3   | 2   | 1   | 0 |
     * Content: | lvl | bs | r | dma | clt | clr | e |
     * Value:   | 1 1 | 0  | 0 | 0   | 1   | 1   | 1 | = 0xC7
     *
     *  Enables FIFO
     *  Clear both receiver and transmission FIFO queues
     * Use 14 bytes as size of queue */
    outb(SERIAL_FIFO_COMMAND_PORT(com), 0xC7);
}

/// @brief Configures the modem control regs
/// @param
void serial_configure_modem(unsigned short com)
{
    /*
     * Bit:     | 7 | 6 | 5  | 4  | 3   | 2   | 1   | 0   |
     * Content: | r | r | af | lb | ao2 | ao1 | rts | dtr |
     * Value:   | 0 | 0 | 0  | 0  | 0   | 0   | 1   | 1   | = 0x03 */
    outb(SERIAL_MODEM_COMMAND_PORT(com), 0x03);
}

/// @brief Checks the FIFO queue of given port
/// @param com COM port
/// @return 0 if the FIFO queue is not empty; otherwise 1
int is_serial_transmit_fifo_empty(unsigned int com)
{
    /* 0x20 = 0010 0000 */ // The transmit fifo queue bit
    return inb(SERIAL_LINE_STATUS_PORT(com)) & 0x20;
}

/// @brief To be called in kernel main
/// @param  com COM port
void serial_initialize(unsigned int base)
{
    // Set Serial baud rate divisor to 3 (38400 baud)
    outb(base + 1, 0x00); // disable all interrupts
    serial_configure_baud_rate(base, 3);
    serial_configure_line(base);
    serial_configure_buffers(base);
    serial_configure_modem(base);
}

/// @brief Writes a character to given port when FIFO queue is empty
/// @param com  COM port
/// @param c Character to write
void serial_write(unsigned int com, char c)
{
    while (is_serial_transmit_fifo_empty(com) == 0)
        ;
    outb(com, c);
}

/// @brief Check if data is waiting to be read in the port
/// @param
/// @return 1 if data is waiting; otherwise 0
int is_serial_received(unsigned short com)
{
    return inb(SERIAL_LINE_STATUS_PORT(com)) & 1;
}

/// @brief
/// @param com COM port
/// @return The last char in the serial read buffer
char serial_read(unsigned short com)
{
    while (is_serial_received(com) == 0)
        ;
    return inb(com);
}