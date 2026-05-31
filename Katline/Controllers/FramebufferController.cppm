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

	struct FramebufferController {
		struct Cell {
			char ch {};
			bool inverted {};
			CL::Color::RGBColor fg { CL::Color::WHITE };
			CL::Color::RGBColor bg { CL::Color::BLACK };
		};

		static constexpr uint MAX_COLS { 256 };
		static constexpr uint MAX_ROWS { 256 };

		FramebufferController(Framebuffer *framebuffer);

		void plot_pixel(uint y, uint x);
		void rect(uint y1, uint x1, uint y2, uint x2);

		void draw_raw_character(char ch, uint y, uint x, bool inverted = false);
		void draw_character(char ch, bool inverted = false);

		void put_character(char ch, bool inverted = false);
		void put_string_safe(
		    char const *string, usize size, bool inverted = false);
		void put_string(char const *string, bool inverted = false);

		void scroll_down(uint lines = 1);
		void redraw_text_area();

		void put_logo(u8 const *data, uint width, uint height, uint x, uint y);

		CL::Color::RGBColor color { CL::Color::WHITE };
		CL::Color::RGBColor background_color { CL::Color::BLACK };
		CL::Math::Point<uint> cursor_position {};

	private:
		static constexpr uint s_font_width { 8 };
		static constexpr uint s_font_height { 8 };
		static constexpr uint s_text_y_offset { 72 };

		static auto ansi_color(uint code) -> CL::Color::RGBColor;

		auto handle_ansi(char const *&string, usize *remaining = nullptr)
		    -> bool;
		auto text_x_offset() const -> uint;
		auto text_y_offset() const -> uint;
		auto columns() const -> uint;
		auto rows() const -> uint;
		auto text_row_to_physical(uint row) const -> uint;
		void clear_cell_row(uint row);
		void draw_cell(uint row, uint col);
		void draw_glyph_direct(char ch, uint y, uint x, CL::Color::RGBColor fg,
		    CL::Color::RGBColor bg, bool inverted);
		auto pack_color(CL::Color::RGBColor c) const -> u32;

		Framebuffer *m_framebuffer {};
		uint m_first_row {};
	};

	}

	}
}

namespace Katline::Controller {

static FramebufferController::Cell s_cells[FramebufferController::MAX_ROWS]
                                          [FramebufferController::MAX_COLS] {};

FramebufferController::FramebufferController(Framebuffer *framebuffer)
    : m_framebuffer { framebuffer }
{
}

auto FramebufferController::ansi_color(uint code) -> CL::Color::RGBColor
{
	switch (code) {
	case 30:
		return CL::Color::BLACK;
	case 31:
		return { 255, 0, 0 };
	case 32:
		return { 0, 255, 0 };
	case 33:
		return { 255, 255, 0 };
	case 34:
		return { 0, 0, 255 };
	case 35:
		return { 255, 0, 255 };
	case 36:
		return { 0, 255, 255 };
	case 37:
		return CL::Color::WHITE;
	default:
		return CL::Color::WHITE;
	}
}

bool FramebufferController::handle_ansi(char const *&s, usize *remaining)
{
	if (s[0] != '\x1b' || s[1] != '[')
		return false;

	auto *p = s + 2;
	uint value {};
	bool have_value {};

	while (*p) {
		if (*p >= '0' && *p <= '9') {
			value = value * 10 + static_cast<uint>(*p - '0');
			have_value = true;
			++p;
			continue;
		}

		if (*p == ';' || *p == 'm') {
			auto code = have_value ? value : 0;

			if (code == 0) {
				color = CL::Color::WHITE;
				background_color = CL::Color::BLACK;
			} else if (code >= 30 && code <= 37) {
				color = ansi_color(code);
			} else if (code >= 40 && code <= 47) {
				background_color = ansi_color(code - 10);
			}

			value = 0;
			have_value = false;

			if (*p == 'm') {
				++p;
				if (remaining)
					*remaining -= static_cast<usize>(p - s);
				s = p;
				return true;
			}

			++p;
			continue;
		}

		return false;
	}

	return false;
}

auto FramebufferController::text_x_offset() const -> uint
{
	auto text_width { columns() * s_font_width };
	if (text_width >= m_framebuffer->width)
		return 0;
	return (m_framebuffer->width - text_width) / 2;
}

auto FramebufferController::text_y_offset() const -> uint
{
	auto text_height { rows() * s_font_height };
	if (s_text_y_offset + text_height >= m_framebuffer->height)
		return s_text_y_offset;

	return s_text_y_offset
	    + ((m_framebuffer->height - s_text_y_offset - text_height) / 2);
}

auto FramebufferController::columns() const -> uint
{
	auto cols { static_cast<uint>(m_framebuffer->width) / s_font_width };
	return cols > MAX_COLS ? MAX_COLS : cols;
}

auto FramebufferController::rows() const -> uint
{
	if (m_framebuffer->height <= s_text_y_offset)
		return 0;

	auto usable_height { static_cast<uint>(m_framebuffer->height)
		- s_text_y_offset };
	auto rows { usable_height / s_font_height };
	return rows > MAX_ROWS ? MAX_ROWS : rows;
}

auto FramebufferController::text_row_to_physical(uint row) const -> uint
{
	auto r { rows() };
	if (!r)
		return 0;
	return (m_first_row + row) % r;
}

auto FramebufferController::pack_color(CL::Color::RGBColor c) const -> u32
{
	return static_cast<u32>(c.b) | (static_cast<u32>(c.g) << 8)
	    | (static_cast<u32>(c.r) << 16) | 0xff000000;
}

void FramebufferController::plot_pixel(uint y, uint x)
{
	if (!m_framebuffer || !m_framebuffer->data)
		return;

	if (y >= m_framebuffer->height)
		y = m_framebuffer->height - 1;
	if (x >= m_framebuffer->width)
		x = m_framebuffer->width - 1;

	auto *fb { m_framebuffer->data + x * 4 + y * m_framebuffer->pitch };

	fb[0] = color.b;
	fb[1] = color.g;
	fb[2] = color.r;
	fb[3] = 0xff;
}

void FramebufferController::rect(uint y1, uint x1, uint y2, uint x2)
{
	if (x1 > x2) {
		CL::swap(x1, x2);
	}

	if (y1 > y2) {
		CL::swap(y1, y2);
	}

	for (auto y { y1 }; y < y2; ++y)
		for (auto x { x1 }; x < x2; ++x)
			plot_pixel(y, x);
}

void FramebufferController::draw_glyph_direct(char ch, uint y, uint x,
    CL::Color::RGBColor fg, CL::Color::RGBColor bg, bool inverted)
{
	if (!m_framebuffer || !m_framebuffer->data)
		return;

	auto fg_color { pack_color(fg) };
	auto bg_color { pack_color(bg) };

	for (uint gy {}; gy < s_font_height; ++gy) {
		auto screen_y { y + gy };
		if (screen_y >= m_framebuffer->height)
			continue;

		auto glyph_row {
			Katline::Font::KernelFontStd[static_cast<u8>(ch) * 8 + gy]
		};

		auto *row { reinterpret_cast<u32 *>(
			m_framebuffer->data + screen_y * m_framebuffer->pitch) };

		for (uint gx {}; gx < s_font_width; ++gx) {
			auto screen_x { x + (s_font_width - 1 - gx) };
			if (screen_x >= m_framebuffer->width)
				continue;

			bool pixel_set { ((glyph_row >> gx) & 1) != 0 };
			bool draw_fg { inverted ? !pixel_set : pixel_set };
			row[screen_x] = draw_fg ? fg_color : bg_color;
		}
	}
}

void FramebufferController::draw_raw_character(
    char ch, uint y, uint x, bool inverted)
{
	draw_glyph_direct(ch, text_y_offset() + y, text_x_offset() + x,
	    background_color, color, inverted);
}

void FramebufferController::draw_character(char ch, bool inverted)
{
	draw_raw_character(ch, static_cast<uint>(cursor_position.y()),
	    static_cast<uint>(cursor_position.x()), inverted);
}

void FramebufferController::clear_cell_row(uint row)
{
	auto cols { columns() };
	auto physical { text_row_to_physical(row) };

	for (uint col {}; col < cols; ++col)
		s_cells[physical][col] = {};
}

void FramebufferController::draw_cell(uint row, uint col)
{
	auto cols { columns() };
	auto r { rows() };

	if (row >= r || col >= cols)
		return;

	auto physical { text_row_to_physical(row) };
	auto const &cell { s_cells[physical][col] };

	draw_glyph_direct(cell.ch ? cell.ch : ' ',
	    text_y_offset() + row * s_font_height,
	    text_x_offset() + col * s_font_width, cell.fg, cell.bg, cell.inverted);
}

void FramebufferController::redraw_text_area()
{
	auto r { rows() };
	auto cols { columns() };

	for (uint row {}; row < r; ++row)
		for (uint col {}; col < cols; ++col)
			draw_cell(row, col);
}

void FramebufferController::put_character(char ch, bool inverted)
{
	auto cols { columns() };
	auto r { rows() };

	if (!cols || !r)
		return;

	auto row { static_cast<uint>(cursor_position.y()) / s_font_height };
	auto col { static_cast<uint>(cursor_position.x()) / s_font_width };

	if (ch == '\n') {
		col = 0;
		++row;

		if (row >= r) {
			scroll_down();
			row = r - 1;
		}

		cursor_position.set_x(col * s_font_width);
		cursor_position.set_y(row * s_font_height);
		return;
	}

	if (col >= cols) {
		col = 0;
		++row;
	}

	if (row >= r) {
		scroll_down();
		row = r - 1;
	}

	auto physical { text_row_to_physical(row) };
	s_cells[physical][col] = {
		.ch = ch,
		.inverted = inverted,
		.fg = color,
		.bg = background_color,
	};

	draw_cell(row, col);

	++col;
	if (col >= cols) {
		col = 0;
		++row;

		if (row >= r) {
			scroll_down();
			row = r - 1;
		}
	}

	cursor_position.set_x(col * s_font_width);
	cursor_position.set_y(row * s_font_height);
}

void FramebufferController::put_string_safe(
    char const *string, usize size, bool inverted)
{
	while (size) {
		if (*string == '\x1b') {
			auto *old = string;
			auto old_size = size;

			if (handle_ansi(string, &size))
				continue;

			string = old;
			size = old_size;
		}

		put_character(*string++, inverted);
		--size;
	}
}

void FramebufferController::put_string(char const *string, bool inverted)
{
	while (*string) {
		if (handle_ansi(string))
			continue;

		put_character(*string++, inverted);
	}
}

void FramebufferController::scroll_down(uint lines)
{
	auto r { rows() };
	if (!lines || !r)
		return;

	if (lines >= r) {
		for (uint row {}; row < r; ++row)
			clear_cell_row(row);
		m_first_row = 0;
		redraw_text_area();
		return;
	}

	for (uint i {}; i < lines; ++i) {
		m_first_row = (m_first_row + 1) % r;
		clear_cell_row(r - 1);
	}

	redraw_text_area();
}

void FramebufferController::put_logo(
    u8 const *data, uint width, uint height, uint x, uint y)
{
	auto old_color { color };

	for (uint i {}; i < height; ++i) {
		for (uint j {}; j < width; ++j) {
			color.r = data[i * width * 3 + j * 3];
			color.g = data[i * width * 3 + j * 3 + 1];
			color.b = data[i * width * 3 + j * 3 + 2];

			plot_pixel(i + y, j + x);
		}
	}

	color = old_color;
}

}
