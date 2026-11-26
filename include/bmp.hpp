#pragma once
#include <stddef.h>
#include <stdint.h>

class Wallpaper {
public:
    static bool load_from_initrd(const char* path = "wallpaper.bmp");
    static bool is_loaded();
    static uint32_t width();
    static uint32_t height();
    static void draw_to_backbuffer();

private:
    static bool loaded;
    static uint32_t img_width;
    static uint32_t img_height;
    static uint32_t* pixel_buf;
};