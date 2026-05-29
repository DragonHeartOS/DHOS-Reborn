export module Katline:SerialController;

import CommonLib;
import :IO;

export {
	namespace Katline {

	namespace Controller {

	constexpr uint PORT { 0x3f8 };

	class SerialController {
	public:
		SerialController() { }

		bool init();
		int received();
		char read();
		int is_transmit_empty();
		void write(char ch);
		void write_string_safe(char const *string, usize size);
		void write_string(char const *string);

	private:
		bool m_enabled = false;
	};

	}

	}
}

namespace Katline {

namespace Debug {
void write_formatted(char const *str, ...);
}

}

namespace Katline::Controller {

auto SerialController::init() -> bool
{
	m_enabled = false;

	IO::outb(PORT + 1, 0x00);
	IO::outb(PORT + 3, 0x80);
	IO::outb(PORT + 0, 0x03);
	IO::outb(PORT + 1, 0x00);
	IO::outb(PORT + 3, 0x03);
	IO::outb(PORT + 2, 0xc7);
	IO::outb(PORT + 4, 0x0b);
	IO::outb(PORT + 4, 0x1e);
	IO::outb(PORT + 0, 0xae);

	if (IO::inb(PORT + 0) != 0xAE) {
		Debug::write_formatted(
		    "[SerialController] Failed to initialize serial: test failed.\n");
		return false;
	}

	m_enabled = true;
	IO::outb(PORT + 4, 0x0f);
	Debug::write_formatted("[SerialController] Initialized.\n");
	return true;
}

auto SerialController::received() -> int { return IO::inb(PORT + 5) & 1; }

auto SerialController::read() -> char
{
	if (m_enabled) {
		while (received() == 0)
			;

		return (char)IO::inb(PORT);
	}

	return '\0';
}

auto SerialController::is_transmit_empty() -> int
{
	return IO::inb(PORT + 5) & 0x20;
}

auto SerialController::write(char ch) -> void
{
	if (m_enabled) {
		while (is_transmit_empty() == 0)
			;

		IO::outb(PORT, (u8)ch);
	}
}

auto SerialController::write_string_safe(char const *string, usize size) -> void
{
	if (m_enabled)
		for (usize i {}; i < size; i++)
			write(string[i]);
}

auto SerialController::write_string(char const *string) -> void
{
	if (m_enabled) {
		while (string[0] != '\0') {
			write(string[0]);
			string++;
		}
	}
}

}
