module;

#include <stdarg.h>

export module Katline:DebugImpl;

import CommonLib;
import :Debug;
import :DebugSink;

namespace Katline::Debug {

static bool s_framebuffer_logging_enabled = false;

static auto write_formatted_impl(char const *str, va_list vl) -> void
{
	usize i = 0;
	usize j = 0;

	char buffer[100] = { 0 };
	char temp[20];

	while (str && str[i]) {
		if (str[i] == '%') {
			i++;

			int precision = -1;

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
