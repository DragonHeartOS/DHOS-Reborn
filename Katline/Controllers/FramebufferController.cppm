export module Katline:FramebufferController;

import CommonLib;
import :Font;

export {
	namespace Katline {

	namespace Controller {

	struct Framebuffer {
		u64 address;
		u16 width, height;
		u16 pitch;
		u16 bpp;
		u8 *data;
	};

	class FramebufferController {
	public:
		FramebufferController(Framebuffer *framebuffer);

		void plot_pixel(uint const y, uint const x);

		void rect(uint const y1, uint const x1, uint const y2, uint const x2);

		void draw_raw_character(char const ch, uint const y, uint const x,
		    bool const inverted = false);
		void draw_character(char const ch, bool const inverted = false);

		void put_character(char const ch, bool const inverted = false);

		void put_string_safe(
		    char const *string, usize const size, bool const inverted = false);
		void put_string(char const *string, bool const inverted = false);

		void scroll_down(uint const lines = 1);

		void put_logo(u8 const *data, uint const width, uint const height,
		    uint const x, uint const y);

		CL::Color::RGBColor color { CL::Color::WHITE };

		CL::Math::Point cursor_position {};

	private:
		Framebuffer *m_framebuffer;
	};

	}

	}
}

namespace Katline::Controller {

uint const FRAMEBUFFER_TEXT_Y_OFFSET = 72;

FramebufferController::FramebufferController(Framebuffer *framebuffer)
    : m_framebuffer { framebuffer }
{
	this->m_framebuffer = framebuffer;
}

void FramebufferController::plot_pixel(uint y, uint x)
{
	if (y > (uint)m_framebuffer->height - 1)
		y = m_framebuffer->height - 1;
	if (x > (uint)m_framebuffer->width - 1)
		x = m_framebuffer->width - 1;

	auto fb {
		m_framebuffer->data + x * 4 + y * m_framebuffer->pitch,
	};

	fb[0] = color.b;
	fb[1] = color.g;
	fb[2] = color.r;
	fb[3] = 0xff;
}

void FramebufferController::rect(uint y1, uint x1, uint y2, uint x2)
{
	if (x1 > x2) {
		uint temp_x = x1;
		x1 = x2;
		x2 = temp_x;
	}

	if (y1 > y2) {
		uint temp_y = y1;
		y1 = y2;
		y2 = temp_y;
	}

	for (auto i { y1 }; i < y2; i++)
		for (auto j { x1 }; j < x2; j++)
			plot_pixel(i, j);
}

void FramebufferController::draw_raw_character(
    char const ch, uint const y, uint const x, bool const inverted)
{
	auto const y_ { y + FRAMEBUFFER_TEXT_Y_OFFSET };
	for (uint temp_y {}; temp_y < 8; temp_y++) {
		for (uint temp_x {}; temp_x < 8; temp_x++) {
			auto character
			    = Katline::Font::KernelFontStd[(uint)ch * 8 + temp_y];
			character = (character >> temp_x) & 1;
			if (character == (!inverted))
				plot_pixel(temp_y + y_, x + 8 - temp_x);
		}
	}
}

void FramebufferController::draw_character(char const ch, bool const inverted)
{
	draw_raw_character(
	    ch, (uint)cursor_position.Y(), (uint)cursor_position.X(), inverted);
}

auto memset(void *destination, int const value, usize const size) -> void
{
	auto destination_ptr { (u8 *)destination };
	for (usize i {}; i < size; i++)
		destination_ptr[i] = (unsigned char)value;
}

void FramebufferController::put_character(char const ch, bool const inverted)
{
	if (ch == '\n') {
		cursor_position.SetX(0);

		if ((uint)cursor_position.Y() + FRAMEBUFFER_TEXT_Y_OFFSET + 8
		    > m_framebuffer->height) {
			scroll_down();
		} else {
			cursor_position.SetY(cursor_position.Y() + 8);
		}
	} else {
		draw_character(ch, inverted);

		cursor_position.SetX(cursor_position.X() + 8);

		if (cursor_position.X() > m_framebuffer->width) {
			cursor_position.SetX(0);
			cursor_position.SetY(cursor_position.Y() + 8);
		}
	}
}

void FramebufferController::put_string_safe(
    char const *string, usize const size, bool const inverted)
{
	for (usize i {}; i < size; i++)
		put_character(string[i], inverted);
}

void FramebufferController::put_string(char const *string, bool const inverted)
{
	while (string[0] != '\0') {
		put_character(string[0], inverted);
		string++;
	}
}

auto memcpy(void *dest, void *source, usize const size) -> void
{
	char *d = (char *)dest;
	char *s = (char *)source;
	for (usize i = 0; i < size; i++)
		d[i] = s[i];
}

void FramebufferController::scroll_down(uint const lines)
{
	memcpy(
	    m_framebuffer->data + m_framebuffer->pitch * FRAMEBUFFER_TEXT_Y_OFFSET,
	    m_framebuffer->data
	        + m_framebuffer->pitch * (FRAMEBUFFER_TEXT_Y_OFFSET + lines * 8),
	    m_framebuffer->pitch
	        * (m_framebuffer->height - lines * 8 - FRAMEBUFFER_TEXT_Y_OFFSET));

	memset(m_framebuffer->data
	        + m_framebuffer->pitch * (m_framebuffer->height - lines * 8),
	    0, m_framebuffer->pitch * (lines * 8));
}

void FramebufferController::put_logo(u8 const *data, uint const width,
    uint const height, uint const x, uint const y)
{
	auto const old_color { color };

	for (uint i {}; i < height; i++) {
		for (uint j {}; j < width; j++) {
			color.r = data[i * width * 3 + j * 3];
			color.g = data[i * width * 3 + j * 3 + 1];
			color.b = data[i * width * 3 + j * 3 + 2];

			plot_pixel(i + y, j + x);
		}
	}

	color = old_color;
}

}
