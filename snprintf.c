#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>


struct stream {
    int idx;
    size_t size;
    char *stream;
};


enum conversion_state {
	//! \internal Normal state, passing characters through to the console.
	STATE_NORMAL,
	//! \internal Parsing an optional conversion flag.
	STATE_FLAG,
	//! \internal Parsing an optional field width specifier.
	STATE_WIDTH,
	//! \internal Looking for a period indicating a precision specifier.
	STATE_PERIOD,
	//! \internal Parsing an optional precision specifier.
	STATE_PRECISION,
	//! \internal Parsing an optional length modifier.
	STATE_LENGTH,
	//! \internal Parsing the conversion specifier.
	STATE_CONVSPEC,
};


struct printf_conversion {
	//! Minimum field width, or 0 if unspecified.
	int width;
	//! Minimum precision, or 0 if unspecified.
	int precision;
	//! Length modifier. This can be 'h', 'l' or 0 (default.)
	char length;
	//! Conversion specifier.
	char spec;
	//! Character to use for padding to specified width
	char pad_char;
	//! Conversion argument extracted from \a ap.
	union {
		//! Signed integer argument.
		long d;
		//! Unsigned integer argument.
		unsigned long u;
		//! Floating-point argument.
		double f;
		//! String argument.
		const char *s;
		//! Pointer argument.
		void *p;
		//! Where to store the result of a %n conversion.
		int *n;
	} arg;
};


static int isdigit(int c)
{
	return (c >= '0') && (c <= '9');
}


size_t generic_strlen(const char *str)
{
	size_t	len = 0;

	while (*str++)
		len++;

	return len;
}


static void stream_priv_putchar(struct stream *ss, char c)
{
        ss->stream[ss->idx++] = c;

        if (ss->idx == ss->size) {
                ss->idx = 0;
        }
}


static void stream_priv_write(struct stream *ss, const char *str, int len)
{
        for (int i = 0; i < len; ++i) {
                stream_priv_putchar(ss, str[i]);
        }
}


static int stream_priv_print_signed(struct stream *ss, struct printf_conversion *conv)
{
	char buf[32];
	long number = conv->arg.d;
	bool negative = false;
	int i = sizeof(buf);
	int len;
	char c;

	if (number == 0)
		buf[--i] = '0';

	if (number < 0) {
		negative = true;
		number = -number;
	}

	while (number) {
		c = '0' + number % 10;
		number /= 10;
		buf[--i] = c;
	}

	if (negative)
		buf[--i] = '-';

	if (conv->width > sizeof(buf))
		conv->width = sizeof(buf);

	while ((sizeof(buf) - i) < conv->width)
		buf[--i] = conv->pad_char;

	len = sizeof(buf) - i;
	stream_priv_write(ss, buf + i, len);

	return len;
}


static int stream_priv_print_unsigned(struct stream *ss, struct printf_conversion *conv)
{
	char buf[32];
	unsigned long number = conv->arg.u;
	int i = sizeof(buf);
	int len;
	char c;

	if (number == 0)
		buf[--i] = '0';

	switch (conv->spec) {
	case 'o':
		while (number) {
			c = '0' + (number & 7);
			number >>= 3;
			buf[--i] = c;
		}
		break;
	case 'u':
		while (number) {
			c = '0' + (number % 10);
			number /= 10;
			buf[--i] = c;
		}
		break;
	case 'x':
		while (number) {
			if ((number & 15) > 9)
				c = 'a' - 10 + (number & 15);
			else
				c = '0' + (number & 15);
			number >>= 4;
			buf[--i] = c;
		}
		break;
	case 'X':
		while (number) {
			if ((number & 15) > 9)
				c = 'A' - 10 + (number & 15);
			else
				c = '0' + (number & 15);
			number >>= 4;
			buf[--i] = c;
		}
		break;
	}

	if (conv->width > sizeof(buf))
		conv->width = sizeof(buf);

	while ((sizeof(buf) - i) < conv->width)
		buf[--i] = conv->pad_char;

	len = sizeof(buf) - i;
	stream_priv_write(ss, buf + i, len);

	return len;
}


static int stream_priv_putstr(struct stream *ss, const char *str)
{
	int len;

	len = generic_strlen(str);
	stream_priv_write(ss, str, len);

	return len;
}


int priv_snvprintf(char *s, size_t size, const char *fmt, va_list ap)
{
	int state = STATE_NORMAL;
	struct printf_conversion conv;
	int n = 0;
	char c;
    struct stream ss;

    ss.idx = 0;
    ss.size = size;
    ss.stream = s;

	while (true) {
                c = *fmt++;

		if (!c)
			break;

		switch (state) {
		case STATE_NORMAL:
			if (c == '%') {
				state = STATE_FLAG;
				conv.width = 0;
				conv.precision = 0;
				conv.length = 0;
				conv.pad_char = ' ';
			} else {
				stream_priv_putchar(&ss, c);
				n++;
			}
			break;

		case STATE_FLAG:
			state = STATE_WIDTH;

			/* We accept all standard flags, but we ignore some */
			switch (c) {
			case '0':
				conv.pad_char = '0';
				break;
			case '#':
			case '-':
			case ' ':
			case '+':
				break;

			case '%':
				/* %% -> output a literal '%' */
				stream_priv_putchar(&ss, c);
				n++;
				state = STATE_NORMAL;
				break;

			default:
				goto state_width;
			}
			break;

		state_width:
		case STATE_WIDTH:
			if (isdigit(c) && (c != '0' || conv.width != 0)) {
				conv.width *= 10;
				conv.width += c - '0';
				break;
			}

			state = STATE_PERIOD;
			/* fall through */

		case STATE_PERIOD:
			if (c != '.') {
				state = STATE_LENGTH;
				goto state_length;
			}
			state = STATE_PRECISION;
			break;

		case STATE_PRECISION:
			/* accept but ignore */
			if (isdigit(c))
				break;

			state = STATE_LENGTH;
			/* fall through */

		state_length:
		case STATE_LENGTH:
			/* SUSv2 only accepts h, l and L */
			if (c == 'h' || c == 'l' || c == 'L') {
				conv.length = c;
				break;
			} else if (c == 'z') {
				if (sizeof(size_t) == sizeof(long))
					conv.length = 'l';
				break;
			}

			state = STATE_CONVSPEC;
			/* fall through */

		case STATE_CONVSPEC:
			conv.spec = c;

			switch (c) {
			case 'd':
			case 'i':
				if (conv.length == 'l')
					conv.arg.d = va_arg(ap, long);
				else
					conv.arg.d = va_arg(ap, int);
				n += stream_priv_print_signed(&ss, &conv);
				break;
			case 'o':
			case 'u':
			case 'x':
			case 'X':
				if (conv.length == 'l')
					conv.arg.u = va_arg(ap, unsigned long);
				else
					conv.arg.u = va_arg(ap, unsigned int);
				n += stream_priv_print_unsigned(&ss, &conv);
				break;
			case 'c':
				conv.arg.d = va_arg(ap, int);
				stream_priv_putchar(&ss, conv.arg.d);
				n++;
				break;

			/* TODO: Handle floats if needed */

			case 'S':
				/* fall through */
			case 's':
				conv.arg.s = va_arg(ap, const char *);
				n += stream_priv_putstr(&ss, conv.arg.s);
				break;
			case 'p':
				conv.arg.p = va_arg(ap, void *);
				stream_priv_write(&ss, "0x", 2);
				n += 2;
				conv.spec = 'x';
				n += stream_priv_print_unsigned(&ss, &conv);
				break;
			case 'n':
				conv.arg.n = va_arg(ap, int *);
				*conv.arg.n = n;
				break;
			}

			state = STATE_NORMAL;
			break;
		}
	}

	return n;
}


int _snprintf(char *s, size_t n, const char *format, ...)
{
    va_list ap;
	int r;

	va_start(ap, format);
    r = priv_snvprintf(s, n, format, ap);
	va_end(ap);

	return r;
}
