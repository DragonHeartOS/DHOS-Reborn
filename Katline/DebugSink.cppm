export module Katline:DebugSink;

import :SerialController;

namespace Katline {

extern Controller::SerialController k_serial_controller;

auto debug_write_serial(char const *buffer) -> void
{
	k_serial_controller.write_string(buffer);
}

auto debug_write_serial(char const *buffer, usize size) -> void
{
	k_serial_controller.write_string_safe(buffer, size);
}

}
