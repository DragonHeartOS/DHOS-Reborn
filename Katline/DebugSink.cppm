export module Katline:DebugSink;

import :FramebufferController;
import :SerialController;

namespace Katline {

extern Controller::FramebufferController k_framebuffer_controller;
extern Controller::SerialController k_serial_controller;

auto debug_write_serial(char const *buffer) -> void
{
	k_serial_controller.write_string(buffer);
}

auto debug_write_serial(char const *buffer, usize size) -> void
{
	k_serial_controller.write_string_safe(buffer, size);
}

auto debug_write_framebuffer(char const *buffer) -> void
{
	k_framebuffer_controller.put_string(buffer);
}

auto debug_write_framebuffer(char const *buffer, usize size) -> void
{
	k_framebuffer_controller.put_string_safe(buffer, size);
}

}
