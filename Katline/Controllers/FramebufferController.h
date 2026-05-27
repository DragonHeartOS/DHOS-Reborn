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

    void PlotPixel(uint y, uint x);

    void Rect(uint y1, uint x1, uint y2, uint x2);

    void DrawRawCharacter(char ch, uint y, uint x, bool inverted = false);
    void DrawCharacter(char ch, bool inverted = false);

    void PutCharacter(char ch, bool inverted = false);

    void PutStringSafe(char const* string, size_t size, bool inverted = false);
    void PutString(char const* string, bool inverted = false);

    void ScrollDown(uint lines = 1);

    void PutLogo(u8 const* data, uint width, uint height, uint x, uint y);

    CL::Color::RGBColor color = CL::Color::WHITE;

    CL::Math::Point cursor_position;

private:
    Framebuffer* m_framebuffer;
};

}

}
