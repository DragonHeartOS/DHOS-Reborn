module;

#include <stdarg.h>

export module Katline:DebugImpl;

import CommonLib;
import :Debug;
import :DebugSink;

namespace Katline::Debug {

static bool s_framebuffer_logging_enabled = false;

static auto u64_to_hex(u64 value, char *out, usize width) -> void
{
	static constexpr char digits[] = "0123456789abcdef";
	char temp[32];
	usize len {};

	do {
		temp[len++] = digits[value & 0x0fu];
		value >>= 4;
	} while (value != 0);

	while (len < width)
		temp[len++] = '0';

	for (usize i {}; i < len; ++i)
		out[i] = temp[len - i - 1];
	out[len] = '\0';
}

static auto write_formatted_impl(char const *str, va_list vl) -> void
{
	usize i = 0;
	usize j = 0;

	char buffer[2048] = { 0 };
	char temp[32];

	while (str && str[i]) {
		if (str[i] == '%') {
			i++;

			int precision = -1;
			usize hex_width = 0;
			bool zero_pad_hex = false;

			if (str[i] == '.') {
				i++;

				if (str[i] == '*') {
					precision = va_arg(vl, int);
					i++;
				} else {
					precision = 0;

					while (str[i] >= '0' && str[i] <= '9') {
						precision = precision * 10 + (str[i] - '0');
						i++;
					}
				}
			} else if (str[i] == '0') {
				zero_pad_hex = true;
				i++;

				while (str[i] >= '0' && str[i] <= '9') {
					hex_width
					    = hex_width * 10 + static_cast<usize>(str[i] - '0');
					i++;
				}
			}

			switch (str[i]) {
			case 'c': {
				buffer[j++] = (char)va_arg(vl, int);
				break;
			}
			case 'd': {
				itoa(va_arg(vl, int), temp);
				strcpy(&buffer[j], temp);
				j += strlen(temp);
				break;
			}
			case 'x': {
				u64 value = va_arg(vl, u64);
				usize width = zero_pad_hex ? hex_width : 0;
				u64_to_hex(value, temp, width);
				strcpy(&buffer[j], temp);
				j += strlen(temp);
				break;
			}
			case 's': {
				char const *s = va_arg(vl, char const *);

				if (!s)
					s = "(null)";

				usize len = strlen(s);

				if (precision >= 0 && static_cast<usize>(precision) < len)
					len = static_cast<usize>(precision);

				for (usize k = 0; k < len; k++)
					buffer[j++] = s[k];

				break;
			}
			case '%': {
				buffer[j++] = '%';
				break;
			}
			default: {
				buffer[j++] = '%';
				buffer[j++] = str[i];
				break;
			}
			}

		} else {
			buffer[j++] = str[i];
		}

		i++;
	}

	buffer[j] = '\0';
	debug_write_serial(buffer);

	if (s_framebuffer_logging_enabled)
		debug_write_framebuffer(buffer);
}

auto set_framebuffer_logging_enabled(bool enabled) -> void
{
	s_framebuffer_logging_enabled = enabled;
}

auto write_formatted(char const *str, ...) -> void
{
	va_list vl;
	va_start(vl, str);
	write_formatted_impl(str, vl);
	va_end(vl);
}

auto print_formatted(char const *str, ...) -> void
{
	va_list vl;
	va_start(vl, str);
	write_formatted_impl(str, vl);
	va_end(vl);
}

}
