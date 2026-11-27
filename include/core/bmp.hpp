#pragma once
#include <stdint.h>
#include <stddef.h>

namespace Wallpaper {
    bool load_from_initrd(const char* path);
    bool is_loaded();
    void draw_to_backbuffer();
    void draw_rect_to_backbuffer(int32_t x, int32_t y, int32_t w, int32_t h);
    void free_decoded();
}