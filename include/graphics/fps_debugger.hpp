#pragma once
#include <stdint.h>
#include "timer.hpp"
#include "framebuffer.hpp"
#include "font.hpp"

namespace FpsDebug
{
    inline bool enabled = false;

    inline uint32_t current_fps = 0;
    inline uint32_t frame_counter = 0;
    inline uint64_t window_start_tick = 0;
    inline bool initialized = false;

    inline void set_enabled(bool value)
    {
        enabled = value;
        if (!enabled)
        {
            frame_counter = 0;
            current_fps = 0;
            window_start_tick = Timer::ticks();
        }
        else
        {
            window_start_tick = Timer::ticks();
            frame_counter = 0;
            initialized = true;
        }
    }

    inline bool is_enabled()
    {
        return enabled;
    }

    inline void on_frame_presented()
    {
        if (!enabled)
        {
            return;
        }

        uint64_t now = Timer::ticks();
        if (!initialized)
        {
            initialized = true;
            window_start_tick = now;
            frame_counter = 0;
            return;
        }

        ++frame_counter;
        uint64_t elapsed64 = now - window_start_tick;
        if (elapsed64 >= 100u)
        {
            uint32_t elapsed32 = (elapsed64 > 0xFFFFFFFFu)
                                     ? 0xFFFFFFFFu
                                     : static_cast<uint32_t>(elapsed64);
            uint32_t numer = frame_counter * 100u;
            current_fps = numer / elapsed32;
            frame_counter = 0;
            window_start_tick = now;
        }
    }

    inline void draw_in_rect(int32_t rx, int32_t ry, int32_t rw, int32_t rh)
    {
        if (!enabled || rw <= 0 || rh <= 0)
        {
            return;
        }

        char buf[16];
        uint32_t v = current_fps;
        if (v > 999u)
        {
            v = 999u;
        }

        buf[0] = 'F';
        buf[1] = 'P';
        buf[2] = 'S';
        buf[3] = ':';
        buf[4] = ' ';

        int idx = 5;
        if (v >= 100u)
        {
            buf[idx++] = static_cast<char>('0' + (v / 100u));
            buf[idx++] = static_cast<char>('0' + ((v / 10u) % 10u));
            buf[idx++] = static_cast<char>('0' + (v % 10u));
        }
        else if (v >= 10u)
        {
            buf[idx++] = static_cast<char>('0' + (v / 10u));
            buf[idx++] = static_cast<char>('0' + (v % 10u));
        }
        else
        {
            buf[idx++] = static_cast<char>('0' + v);
        }
        buf[idx] = '\0';

        int32_t text_w = idx * 8;
        int32_t pad = 6;
        int32_t box_w = text_w + pad * 2;
        int32_t box_h = 20;
        int32_t bx = rx + rw - box_w - 6;
        int32_t by = ry + 6;

        if (bx < rx + 2 || by < ry + 2 || bx + box_w > rx + rw - 2 || by + box_h > ry + rh - 2)
        {
            return;
        }

        Framebuffer::fill_rect(bx, by, box_w, box_h, Framebuffer::rgb(15, 17, 22));
        Framebuffer::fill_rect(bx, by, box_w, 1, Framebuffer::rgb(138, 237, 128));
        Framebuffer::fill_rect(bx, by + box_h - 1, box_w, 1, Framebuffer::rgb(138, 237, 128));
        Framebuffer::fill_rect(bx, by, 1, box_h, Framebuffer::rgb(138, 237, 128));
        Framebuffer::fill_rect(bx + box_w - 1, by, 1, box_h, Framebuffer::rgb(138, 237, 128));
        Font8x16::draw_text(bx + pad, by + 2, buf, Framebuffer::rgb(138, 237, 128));
    }
}