#include <Katline/Controllers/FramebufferController.h>

#include <CommonLib/Color.h>
#include <CommonLib/Math.h>
#include <Katline/Font.h>
#include <Katline/Logo.h>

namespace Katline {

namespace Controller {

uint const FRAMEBUFFER_TEXT_Y_OFFSET = 72;

FramebufferController::FramebufferController(Framebuffer *framebuffer)
    : m_framebuffer(framebuffer)
{
	this->m_framebuffer = framebuffer;

	put_logo(Logo::Data, Logo::Width, Logo::Height, 10, 10);
}

void FramebufferController::plot_pixel(uint y, uint x)
{
	if (y > (uint)m_framebuffer->height - 1)
		y = m_framebuffer->height - 1;
	if (x > (uint)m_framebuffer->width - 1)
		x = m_framebuffer->width - 1;

	auto fb {
		m_framebuffer->data + x * 4 + // X
		y * m_framebuffer->pitch      // Y
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

// TODO: Put this in Marine
void memset(void *destination, int const value, size_t const size)
{
	auto destination_ptr { (u8 *)destination };
	for (size_t i {}; i < size; i++)
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
    char const *string, size_t const size, bool const inverted)
{
	for (size_t i {}; i < size; i++)
		put_character(string[i], inverted);
}

void FramebufferController::put_string(char const *string, bool const inverted)
{
	while (string[0] != '\0') {
		put_character(string[0], inverted);
		string++;
	}
}

// TODO: Put this in Marine
void memcpy(void *dest, void *source, size_t const size)
{
	char *d = (char *)dest;
	char *s = (char *)source;
	for (size_t i = 0; i < size; i++)
		d[i] = s[i];
}

// TODO: Fix this
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

}
