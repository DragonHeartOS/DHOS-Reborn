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
    u8* data;
};

class FramebufferController {
public:
    FramebufferController(Framebuffer* framebuffer);

    void plot_pixel(uint y, uint x);

    void rect(uint y1, uint x1, uint y2, uint x2);

    void draw_raw_character(char ch, uint y, uint x, bool inverted = false);
    void draw_character(char ch, bool inverted = false);

    void put_character(char ch, bool inverted = false);

    void put_string_safe(char const* string, size_t size, bool inverted = false);
    void put_string(char const* string, bool inverted = false);

    void scroll_down(uint lines = 1);

    void put_logo(u8 const* data, uint width, uint height, uint x, uint y);

    CL::Color::RGBColor color { CL::Color::WHITE };

    CL::Math::Point cursor_position {};

private:
    Framebuffer* m_framebuffer;
};

}

}
