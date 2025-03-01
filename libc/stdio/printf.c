#include <limits.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static char digits[] = "0123456789ABCDEF";

static bool print(const char *data, size_t length)
{
	const unsigned char *bytes = (const unsigned char *)data;
	for (size_t i = 0; i < length; i++)
		if (putchar(bytes[i]) == EOF)
			return false;
	return true;
}

// if the number is signed then sign=1; if unsigned then sign=0
static size_t printint(int number, int tgt_base, int sign)
{
	char buf[16];
	int i = 0;
	int isNegative = 0;
	unsigned int num;
	size_t len = 0;

	if (sign && number < 0)
	{
		isNegative = 1;
		num = -number;
	}
	else
		num = number;

	do
	{
		buf[i++] = digits[num % tgt_base];
	} while ((num /= tgt_base) != 0);

	if (isNegative)
		buf[i++] = '-';
	len = (size_t)i;

	while (--i >= 0)
		putchar(buf[i]);

	return len;
}

int printf(const char *restrict format, ...)
{
	va_list parameters;
	va_start(parameters, format);

	int i;
	int written = 0;
	size_t len;

	while (*format != '\0')
	{
		size_t maxrem = INT_MAX - written;

		if (format[0] != '%' || format[1] == '%')
		{
			if (format[0] == '%')
				format++;
			size_t amount = 1;
			while (format[amount] && format[amount] != '%')
				amount++;
			if (maxrem < amount)
			{
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(format, amount))
				return -1;
			format += amount;
			written += amount;
			continue;
		}

		const char *format_begun_at = format++;

		switch (*format)
		{
		case ('d'): // for signed integers
			format++;
			i = (int)va_arg(parameters, int);
			len = printint(i, 10, 1);
			if (maxrem < len)
			{
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			written += len;
			break;

		case ('u'): // for unsigned integers
			format++;
			i = (int)va_arg(parameters, int);
			len = printint(i, 10, 0);
			if (maxrem < len)
			{
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			written += len;
			break;

		case ('x'): // for hexadecimal integers
			format++;
			i = (int)va_arg(parameters, int);
			len = 2;
			if (!print("0x", len))
			{
				return -1;
			}
			len += printint(i, 16, 0);
			if (maxrem < len)
			{
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			written += len;
			break;

		case ('c'): // for characters
			format++;
			char c = (char)va_arg(parameters, int /* char promotes to int */);
			if (!maxrem)
			{
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(&c, sizeof(c)))
				return -1;
			written++;
			break;

		case ('s'): // for strings
			format++;
			const char *str = va_arg(parameters, const char *);
			len = strlen(str);
			if (maxrem < len)
			{
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(str, len))
				return -1;
			written += len;
			break;

		default:
			format = format_begun_at;
			len = strlen(format);
			if (maxrem < len)
			{
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(format, len))
				return -1;
			written += len;
			format += len;
			break;
		}
	}

	va_end(parameters);
	return written;
}
