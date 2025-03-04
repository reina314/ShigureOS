#include <kernel/kb.h>
#include <kernel/irq.h>
#include <kernel/sh.h>
// #include <kernel/tty.h>
#include <stdlib.h> // for inb()
#include <stdio.h>
#include <stdbool.h>

static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static volatile int buffer_start = 0;
static volatile int buffer_end = 0;

/// @brief Check if buffer is empty or not
/// @return
static bool is_buffer_empty(void)
{
    return buffer_start == buffer_end;
}

/// @brief Check if buffer is full or not
/// @return
static bool is_buffer_full(void)
{
    return ((buffer_end + 1) % KEYBOARD_BUFFER_SIZE) == buffer_start;
}

/// @brief Push a character into the buffer
/// @param c
static void keyboard_buffer_put(char c)
{
    if (!is_buffer_full())
    {
        keyboard_buffer[buffer_end] = c;
        buffer_end = (buffer_end + 1) % KEYBOARD_BUFFER_SIZE;
    }
}

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
    (void)r; // Avoid unused parameter warning

    // Read from the keyboard data buffer
    unsigned char scancode = inb(0x60);

    // If the top bit of the input is set, then the key has just been released
    if (!(scancode & 0x80)) // If key is pressed (not released)
    {
        char c = kb_us[scancode];
        keyboard_buffer_put(c); // Store char in buffer
    }

    // When key is released
}

/// @brief Install IRQ handler for keyboard interrupt
void keyboard_install(void)
{
    register_request_handler(1, keyboard_handler);
}

/// @brief Retrieve character from keyboard buffer; FIFO
/// @return
char keyboard_getchar(void)
{
    while (is_buffer_empty())
        asm volatile("pause"); // Prevents CPU from spinning too fast
    // Wait until a key is available

    char c = keyboard_buffer[buffer_start];
    buffer_start = (buffer_start + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
}