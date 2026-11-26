#pragma once
#include <stdint.h>
#include <stddef.h>

namespace Wallpaper {
    bool load_from_initrd(const char* path);
    bool is_loaded();
    void draw_to_backbuffer();
    void free_decoded();
}