#include <Katline/Controllers/SerialController.h>

#include <Katline/Controllers/IO.h>
#include <Katline/Debug.h>
#include <Katline/Katline.h>

namespace Katline {

namespace Controller {

auto SerialController::init() -> bool
{
	m_enabled = false;

	IO::outb(PORT + 1, 0x00); // Disable all interrupts
	IO::outb(PORT + 3, 0x80); // Enable DLAB (set baud rate driver)
	IO::outb(PORT + 0, 0x03); // Set divisor to 3 (lo byte) 38400 baud
	IO::outb(PORT + 1, 0x00); //                  (hi byte)
	IO::outb(PORT + 3, 0x03); // 8 bits, no parity, one stop bit
	IO::outb(PORT + 2, 0xc7); // Enable FIFO, clear them, with 14-byte threshold
	IO::outb(PORT + 4, 0x0b); // IRQs enabled, RTS/DSR set
	IO::outb(PORT + 4, 0x1e); // Set in loopback mode, test the serial chip
	IO::outb(PORT + 0, 0xae); // Test serial chip (send byte 0xAE and check if
	                          // serial returns same byte)

	if (IO::inb(PORT + 0) != 0xAE) {
		Debug::write_formatted(
		    "[SerialController] Failed to initialize serial: test failed.\n");
		return false;
	}

	m_enabled = true;

	// Serial not faulty, set it to normal mode
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
	} else {
		return '\0';
	}
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

}
