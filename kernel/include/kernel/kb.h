#ifndef _KERNEL_KB_H
#define _KERNEL_KB_H

#define KEYBOARD_BUFFER_SIZE 128

void keyboard_install(void);
char keyboard_getchar(void);

#endif