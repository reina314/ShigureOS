#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

__attribute__((__noreturn__)) void abort(void)
{
#if defined(__is_libk)
	// TODO: Add proper kernel panic.
	printf("kernel: panic: abort()\n");
#else
	// TODO: Abnormally terminate the process as if by SIGABRT.
	printf("abort()\n");
#endif
	while (1)
	{
	}
	__builtin_unreachable();
}

__attribute__((__noreturn__)) extern void panic(const char *message, const char *file, uint32_t line)
{
	asm volatile("cli"); // Disable interrupts

	printf("PANIC(%s) at %s : %d\n", message, file, (int)line);
	for (;;)
		;
}

__attribute__((__noreturn__)) extern void panic_assert(const char *file, uint32_t line, const char *desc)
{
	asm volatile("cli"); // Disable interrupts

	printf("ASSERTION_FAILED(%s) at %s : %d\n", desc, file, (int)line);
	for (;;)
		;
}
