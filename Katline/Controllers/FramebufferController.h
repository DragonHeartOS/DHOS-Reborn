#pragma once

#include <CommonLib/Color.h>
#include <CommonLib/Math.h>

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

	void draw_raw_character(
	    char const ch, uint const y, uint const x, bool const inverted = false);
	void draw_character(char const ch, bool const inverted = false);

	void put_character(char const ch, bool const inverted = false);

	void put_string_safe(
	    char const *string, size_t const size, bool const inverted = false);
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
