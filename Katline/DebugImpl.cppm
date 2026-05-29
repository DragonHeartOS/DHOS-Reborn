module;

#include <stdarg.h>

export module Katline:DebugImpl;

import CommonLib;
import :Debug;
import :DebugSink;

namespace Katline::Debug {

static CL::Atomic<bool> s_framebuffer_logging_enabled { false };
static CL::Atomic<bool> s_log_queue_ready { false };

struct LogEntry {
	u32 len {};
	char data[1024] {};
};

static CL::MpscQueue<LogEntry, 256> s_log_queue;

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
	auto append_char = [&](char ch) -> void {
		if (j + 1 < sizeof(buffer))
			buffer[j++] = ch;
	};
	auto append_string = [&](char const *s, usize len) -> void {
		for (usize k {}; k < len && j + 1 < sizeof(buffer); ++k)
			buffer[j++] = s[k];
	};

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
				append_char((char)va_arg(vl, int));
				break;
			}
			case 'd': {
				itoa(va_arg(vl, int), temp);
				append_string(temp, strlen(temp));
				break;
			}
			case 'x': {
				u64 value = va_arg(vl, u64);
				usize width = zero_pad_hex ? hex_width : 0;
				u64_to_hex(value, temp, width);
				append_string(temp, strlen(temp));
				break;
			}
			case 's': {
				char const *s = va_arg(vl, char const *);

				if (!s)
					s = "(null)";

				usize len = strlen(s);

				if (precision >= 0 && static_cast<usize>(precision) < len)
					len = static_cast<usize>(precision);

				append_string(s, len);

				break;
			}
			case '%': {
				append_char('%');
				break;
			}
			default: {
				append_char('%');
				append_char(str[i]);
				break;
			}
			}

		} else {
			append_char(str[i]);
		}

		i++;
	}

	buffer[j] = '\0';

	LogEntry entry {};
	usize const copy_len {
		j < (sizeof(entry.data) - 1) ? j : (sizeof(entry.data) - 1)
	};
	entry.len = static_cast<u32>(copy_len);
	for (usize i {}; i < copy_len; ++i)
		entry.data[i] = buffer[i];
	entry.data[copy_len] = '\0';

	if (!s_log_queue_ready.load()) {
		debug_write_serial(entry.data, entry.len);
		if (s_framebuffer_logging_enabled.load())
			debug_write_framebuffer(entry.data, entry.len);
		return;
	}

	s_log_queue.push_blocking(entry);
}

auto init_log_queue() -> void
{
	s_log_queue.init();
	s_log_queue_ready.store(true);
}

auto set_framebuffer_logging_enabled(bool enabled) -> void
{
	s_framebuffer_logging_enabled.store(enabled);
}

auto drain_logs() -> void
{
	if (!s_log_queue_ready.load())
		return;

	while (auto entry = s_log_queue.try_pop()) {
		debug_write_serial(entry->data, entry->len);
		if (s_framebuffer_logging_enabled.load())
			debug_write_framebuffer(entry->data, entry->len);
	}
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
