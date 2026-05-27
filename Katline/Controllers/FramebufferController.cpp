#include <Katline/Controllers/FramebufferController.h>

#include <CommonLib/Color.h>
#include <CommonLib/Math.h>
#include <Katline/Font.h>
#include <Katline/Logo.h>

namespace Katline {

namespace Controller {

uint const FRAMEBUFFER_TEXT_Y_OFFSET = 72;

FramebufferController::FramebufferController(Framebuffer* framebuffer)
    : m_framebuffer(framebuffer)
{
    this->m_framebuffer = framebuffer;

    PutLogo(Logo::Data, Logo::Width, Logo::Height, 10, 10);
}

void FramebufferController::PlotPixel(uint y, uint x)
{
    if (y > (uint)m_framebuffer->height - 1)
        y = m_framebuffer->height - 1;
    if (x > (uint)m_framebuffer->width - 1)
        x = m_framebuffer->width - 1;

    auto fb = m_framebuffer->data + x * 4 + // X
        y * m_framebuffer->pitch            // Y
        ;

    fb[0] = color.b;
    fb[1] = color.g;
    fb[2] = color.r;
    fb[3] = 0xff;
}

void FramebufferController::Rect(uint y1, uint x1, uint y2, uint x2)
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

    for (uint i = y1; i < y2; i++)
        for (uint j = x1; j < x2; j++)
            PlotPixel(i, j);
}

void FramebufferController::DrawRawCharacter(char ch, uint y, uint x, bool inverted)
{
    y += FRAMEBUFFER_TEXT_Y_OFFSET;
    for (uint temp_y = 0; temp_y < 8; temp_y++) {
        for (uint temp_x = 0; temp_x < 8; temp_x++) {
            auto character = Katline::Font::KernelFontStd[(uint)ch * 8 + temp_y];
            character = (character >> temp_x) & 1;
            if (character == (!inverted))
                PlotPixel(temp_y + y, x + 8 - temp_x);
        }
    }
}

void FramebufferController::DrawCharacter(char ch, bool inverted)
{
    DrawRawCharacter(ch, (uint)cursor_position.Y(), (uint)cursor_position.X(), inverted);
}

// TODO: Put this in Marine
void memset(void* destination, int value, size_t size)
{
    auto destination_ptr = (u8*)destination;
    for (size_t i = 0; i < size; i++)
        destination_ptr[i] = (unsigned char)value;
}

void FramebufferController::PutCharacter(char ch, bool inverted)
{
    if (ch == '\n') {
        cursor_position.SetX(0);

        if ((uint)cursor_position.Y() + FRAMEBUFFER_TEXT_Y_OFFSET + 8 > m_framebuffer->height) {
            ScrollDown();
        } else {
            cursor_position.SetY(cursor_position.Y() + 8);
        }
    } else {
        DrawCharacter(ch, inverted);

        cursor_position.SetX(cursor_position.X() + 8);

        if (cursor_position.X() > m_framebuffer->width) {
            cursor_position.SetX(0);
            cursor_position.SetY(cursor_position.Y() + 8);
        }
    }
}

void FramebufferController::PutStringSafe(char const* string, size_t size, bool inverted)
{
    for (size_t i = 0; i < size; i++)
        PutCharacter(string[i], inverted);
}

void FramebufferController::PutString(char const* string, bool inverted)
{
    while (string[0] != '\0') {
        PutCharacter(string[0], inverted);
        string++;
    }
}

// TODO: Put this in Marine
void memcpy(void* dest, void* source, size_t size)
{
    char* d = (char*)dest;
    char* s = (char*)source;
    for (size_t i = 0; i < size; i++)
        d[i] = s[i];
}

// TODO: Fix this
void FramebufferController::ScrollDown(uint lines)
{
    memcpy(
        m_framebuffer->data + m_framebuffer->pitch * FRAMEBUFFER_TEXT_Y_OFFSET,
        m_framebuffer->data + m_framebuffer->pitch * (FRAMEBUFFER_TEXT_Y_OFFSET + lines * 8),
        m_framebuffer->pitch * (m_framebuffer->height - lines * 8 - FRAMEBUFFER_TEXT_Y_OFFSET));

    memset(
        m_framebuffer->data + m_framebuffer->pitch * (m_framebuffer->height - lines * 8),
        0,
        m_framebuffer->pitch * (lines * 8));
}

void FramebufferController::PutLogo(u8 const* data, uint width, uint height, uint x, uint y)
{
    auto old_color = color;

    for (uint i = 0; i < height; i++) {
        for (uint j = 0; j < width; j++) {
            color.r = data[i * width * 3 + j * 3];
            color.g = data[i * width * 3 + j * 3 + 1];
            color.b = data[i * width * 3 + j * 3 + 2];

            PlotPixel(i + y, j + x);
        }
    }

    color = old_color;
}

}

}
