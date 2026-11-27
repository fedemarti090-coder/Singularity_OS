#pragma once
#include <stdint.h>
#include <stddef.h>
#include "framebuffer.hpp"

struct DirtyRect
{
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
};

namespace DirtyTracker
{
    inline constexpr size_t MAX_RECTS = 16;

    inline DirtyRect rects[MAX_RECTS];
    inline size_t count = 0;
    inline bool full_redraw = true;

    inline void clear()
    {
        count = 0;
        full_redraw = false;
    }

    inline void request_full()
    {
        full_redraw = true;
    }

    inline void add(int32_t x, int32_t y, int32_t w, int32_t h)
    {
        if (full_redraw || w <= 0 || h <= 0)
        {
            return;
        }

        int32_t sw = static_cast<int32_t>(Framebuffer::width());
        int32_t sh = static_cast<int32_t>(Framebuffer::height());
        int32_t x0 = (x < 0) ? 0 : x;
        int32_t y0 = (y < 0) ? 0 : y;
        int32_t x1 = (x + w > sw) ? sw : (x + w);
        int32_t y1 = (y + h > sh) ? sh : (y + h);

        if (x0 >= x1 || y0 >= y1)
        {
            return;
        }

        DirtyRect nr{x0, y0, x1 - x0, y1 - y0};

        // Unisce se interseca o tocca un rettangolo già presente
        for (size_t i = 0; i < count; ++i)
        {
            DirtyRect &r = rects[i];
            int32_t r_x1 = r.x + r.w;
            int32_t r_y1 = r.y + r.h;
            int32_t n_x1 = nr.x + nr.w;
            int32_t n_y1 = nr.y + nr.h;

            bool touches = !(n_x1 + 4 < r.x || nr.x - 4 > r_x1 ||
                             n_y1 + 4 < r.y || nr.y - 4 > r_y1);
            if (touches)
            {
                int32_t ux0 = (r.x < nr.x) ? r.x : nr.x;
                int32_t uy0 = (r.y < nr.y) ? r.y : nr.y;
                int32_t ux1 = (r_x1 > n_x1) ? r_x1 : n_x1;
                int32_t uy1 = (r_y1 > n_y1) ? r_y1 : n_y1;
                r.x = ux0;
                r.y = uy0;
                r.w = ux1 - ux0;
                r.h = uy1 - uy0;
                return;
            }
        }

        if (count < MAX_RECTS)
        {
            rects[count++] = nr;
        }
        else
        {
            // Troppi rettangoli distinti: meglio un redraw completo
            full_redraw = true;
        }
    }

    inline void flush_swap()
    {
        if (full_redraw)
        {
            Framebuffer::swap_buffers();
        }
        else
        {
            for (size_t i = 0; i < count; ++i)
            {
                Framebuffer::swap_rect(rects[i].x, rects[i].y, rects[i].w, rects[i].h);
            }
        }
        clear();
    }
}