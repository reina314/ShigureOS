#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <kernel/tty.h>

#include "vga.h"

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
static uint16_t *const VGA_MEMORY = (uint16_t *)0xB8000;

static size_t terminal_x;
static size_t terminal_y;
static uint8_t terminal_color;
static uint16_t *terminal_buffer;

void terminal_initialize(void)
{
	terminal_x = 0;
	terminal_y = 0;
	terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
	terminal_buffer = VGA_MEMORY;
	for (size_t y = 0; y < VGA_HEIGHT; y++)
	{
		for (size_t x = 0; x < VGA_WIDTH; x++)
		{
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
	terminal_showlogo();
}

void terminal_setcolor(uint8_t color)
{
	terminal_color = color;
}

void terminal_putentryat(unsigned char c, uint8_t color, size_t x, size_t y)
{
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}

void terminal_delete_last_line(void)
{
	int index;

	for (index = (VGA_HEIGHT - 1) * VGA_WIDTH; index < VGA_HEIGHT * VGA_WIDTH; index++)
		terminal_buffer[index] = vga_entry(' ', terminal_color);
}

void terminal_scroll(int line)
{
	int index;

	for (index = line * VGA_WIDTH; index < (line + 1) * VGA_WIDTH; index++)
		terminal_buffer[index - VGA_WIDTH] = terminal_buffer[index];
}

void terminal_putchar(char c)
{
	unsigned char uc = c;

	switch (uc)
	{
	case '\n':
		terminal_x = 0;
		terminal_y++;
		break;

	default:
		terminal_putentryat(uc, terminal_color, terminal_x, terminal_y);
		terminal_x++;
		break;
	}

	if (terminal_x == VGA_WIDTH)
	{
		terminal_x = 0;
		terminal_y++;
	}

	if (terminal_y == VGA_HEIGHT)
	{
		for (int y = 1; y < VGA_HEIGHT; y++)
			terminal_scroll(y);
		terminal_x = 0;
		terminal_y = VGA_HEIGHT - 1;
	}
}

void terminal_write(const char *data, size_t size)
{
	for (size_t i = 0; i < size; i++)
		terminal_putchar(data[i]);
}

void terminal_writestring(const char *data)
{
	terminal_write(data, strlen(data));
}

void terminal_showlogo(void)
{
	terminal_writestring("MP\"\"\"\"\"\"`MMdP      oo                          \n");
	terminal_writestring("M  mmmmm..M88                                        \n");
	terminal_writestring("M.      `YM88d888b.dP.d8888b.dP    dP88d888b..d8888b.\n");
	terminal_writestring("MMMMMMM.  M88'  `888888'  `8888    8888'  `8888ooood8\n");
	terminal_writestring("M. .MMM'  M88    888888.  .8888.  .8888      88.  ...\n");
	terminal_writestring("Mb.     .dMdP    dPdP`8888P88`88888P'dP      `88888P'\n");
	terminal_writestring("MMMMMMMMMMM               .88                        \n");
	terminal_writestring("                      d8888P                         \n");
	terminal_writestring("Welcome to Shigure OS!\n");
}
