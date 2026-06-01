module;

#include <stdarg.h>

export module Katline:DebugImpl;

import CommonLib;
import :Sync;
import :Debug;
import :DebugSink;

namespace Katline::Debug {

static Sync::SpinLock s_output_lock;

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

static auto u64_to_dec(u64 value, char *out) -> void
{
	char temp[32];
	usize len {};

	do {
		temp[len++] = static_cast<char>('0' + (value % 10u));
		value /= 10u;
	} while (value != 0);

	for (usize i {}; i < len; ++i)
		out[i] = temp[len - i - 1];
	out[len] = '\0';
}

static auto i64_to_dec(i64 value, char *out) -> void
{
	if (value < 0) {
		*out++ = '-';
		u64_to_dec(0ull - static_cast<u64>(value), out);
		return;
	}

	u64_to_dec(static_cast<u64>(value), out);
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
	auto append_padded = [&](char const *s, usize len, usize width,
	                        bool zero_pad) -> void {
		if (width <= len) {
			append_string(s, len);
			return;
		}

		auto const pad_len { width - len };
		char const pad_char { zero_pad ? '0' : ' ' };

		if (zero_pad && len > 0 && s[0] == '-') {
			append_char('-');
			for (usize k {}; k < pad_len; ++k)
				append_char('0');
			append_string(s + 1, len - 1);
			return;
		}

		for (usize k {}; k < pad_len; ++k)
			append_char(pad_char);
		append_string(s, len);
	};

	while (str && str[i]) {
		if (str[i] == '%') {
			i++;

			int precision = -1;
			usize numeric_width = 0;
			bool zero_pad = false;
			enum class Length {
				Default,
				Long,
				LongLong,
				Size,
			};
			Length length { Length::Default };

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
				zero_pad = true;
				i++;

				while (str[i] >= '0' && str[i] <= '9') {
					numeric_width = numeric_width * 10
					    + static_cast<usize>(str[i] - '0');
					i++;
				}
			}

			if (str[i] == 'l') {
				i++;
				length = Length::Long;
				if (str[i] == 'l') {
					i++;
					length = Length::LongLong;
				}
			} else if (str[i] == 'z') {
				i++;
				length = Length::Size;
			}

			switch (str[i]) {
			case 'c': {
				append_char((char)va_arg(vl, int));
				break;
			}
			case 'd': {
				switch (length) {
				case Length::Long:
					i64_to_dec(static_cast<i64>(va_arg(vl, long)), temp);
					break;
				case Length::LongLong:
					i64_to_dec(va_arg(vl, long long), temp);
					break;
				case Length::Size:
					i64_to_dec(static_cast<i64>(va_arg(vl, usize)), temp);
					break;
				case Length::Default:
				default:
					itoa(va_arg(vl, int), temp);
					break;
				}
				auto const len { strlen(temp) };
				append_padded(temp, len, numeric_width, zero_pad);
				break;
			}
			case 'u': {
				switch (length) {
				case Length::Long:
					u64_to_dec(
					    static_cast<u64>(va_arg(vl, unsigned long)), temp);
					break;
				case Length::LongLong:
					u64_to_dec(va_arg(vl, unsigned long long), temp);
					break;
				case Length::Size:
					u64_to_dec(static_cast<u64>(va_arg(vl, usize)), temp);
					break;
				case Length::Default:
				default:
					u64_to_dec(
					    static_cast<u64>(va_arg(vl, unsigned int)), temp);
					break;
				}
				auto const len { strlen(temp) };
				append_padded(temp, len, numeric_width, zero_pad);
				break;
			}
			case 'x': {
				u64 value {};
				switch (length) {
				case Length::Long:
					value = static_cast<u64>(va_arg(vl, unsigned long));
					break;
				case Length::LongLong:
					value = va_arg(vl, unsigned long long);
					break;
				case Length::Size:
					value = static_cast<u64>(va_arg(vl, usize));
					break;
				case Length::Default:
				default:
					value = static_cast<u64>(va_arg(vl, unsigned int));
					break;
				}
				usize width = zero_pad ? numeric_width : 0;
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
		Sync::ScopedIrqSpinLock guard { s_output_lock };

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

	Sync::ScopedIrqSpinLock guard { s_output_lock };

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
