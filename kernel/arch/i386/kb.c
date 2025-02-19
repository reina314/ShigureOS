#include <kernel/irq.h>
#include <kernel/tty.h>
#include <kernel/kb.h>
#include <stdio.h>
#include <stdlib.h>

/// @brief Defines keyboard layout (default: US)
unsigned char kb_us[128] = {
    0,
    27,
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
    '0',
    '-',
    '=',
    '\b', /* Backspace */
    '\t', /* Tab */
    'q',
    'w',
    'e',
    'r',
    't',
    'y',
    'u',
    'i',
    'o',
    'p',
    '[',
    ']',
    '\n', /* Enter key */
    0,    /* 29   - Control */
    'a',
    's',
    'd',
    'f',
    'g',
    'h',
    'j',
    'k',
    'l',
    ';',
    '\'',
    '`',
    0, /* Left shift */
    '\\',
    'z',
    'x',
    'c',
    'v',
    'b',
    'n',
    'm',
    ',',
    '.',
    '/',
    0, /* Right shift */
    '*',
    0,   /* Alt */
    ' ', /* Space bar */
    0,   /* Caps lock */
    0,   /* 59 - F1 key ... > */
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,      /* < ... F10 */
    0,      /* 69 - Num lock*/
    0,      /* Scroll Lock */
    0,      /* Home key */
    '\xDD', /* Up Arrow */
    0,      /* Page Up */
    '-',
    0, /* Left Arrow */
    0,
    0, /* Right Arrow */
    '+',
    0, /* 79 - End key*/
    0, /* Down Arrow */
    0, /* Page Down */
    0, /* Insert Key */
    0, /* Delete Key */
    0,
    0,
    0,
    0, /* F11 Key */
    0, /* F12 Key */
    0, /* All other keys are undefined */
};

/// @brief Handles keyboard interrupt
/// @param r
void keyboard_handler(struct regs *r)
{
    unsigned char scancode;
    printf("kb interrupt");

    // Read from the keyboard data buffer
    scancode = inb(0x60);

    // If the top bit of the input is set, then the key has just been released
    if (scancode & 0x80)
    {
        // Here to check if shift, alt or ctrl key are released
    }
    else
    {
        // Hold a key down, then it triggers multiple interrupts
        terminal_putchar(kb_us[scancode]);
    }
}

/// @brief Install IRQ handler for keyboard interrupt
void keyboard_install(void)
{
    printf("Installing keyboard handler\n");
    register_request_handler(1, keyboard_handler);
}