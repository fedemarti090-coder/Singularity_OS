#pragma once
#include <stddef.h>
#include <stdint.h>
#include "multiboot.hpp"

class Framebuffer
{
public:
    static bool init(const MultibootInfo *mb_info, uint32_t virt_base = 0xFD000000u);
    static bool is_ready();

    static uint32_t width();
    static uint32_t height();
    static uint32_t pitch();
    static uint8_t bpp();
    static uint32_t *backbuffer();

    static inline uint32_t rgb(uint8_t r, uint8_t g, uint8_t b)
    {
        return (0xFFu << 24) | (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);
    }

    static void clear(uint32_t color);
    static void put_pixel(int32_t x, int32_t y, uint32_t color);
    static void fill_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
    static void swap_buffers();
    static void swap_rect(int32_t x, int32_t y, int32_t w, int32_t h);

private:
    static bool ready;
    static uint32_t *front_ptr;
    static uint32_t *back_ptr;
    static uint32_t fb_width;
    static uint32_t fb_height;
    static uint32_t fb_pitch;
    static uint32_t fb_size_bytes;
    static uint8_t fb_bpp;
};