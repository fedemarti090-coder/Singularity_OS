#pragma once
#include <stdint.h>
#include "framebuffer.hpp"

namespace Cursor {

inline void draw(int32_t x, int32_t y) {
    static constexpr int W = 12;
    static constexpr int H = 16;
    static constexpr const char* SPRITE[H] = {
        "#...........",
        "##..........",
        "#*#.........",
        "#**#........",
        "#***#.......",
        "#****#......",
        "#*****#.....",
        "#******#....",
        "#*******#...",
        "#****#####..",
        "#*#**#......",
        "##.#**#.....",
        "#..#**#.....",
        "...#**#.....",
        "....#**#....",
        ".....##....."
    };

    const uint32_t border_color = Framebuffer::rgb(11, 12, 16);
    const uint32_t fill_color   = Framebuffer::rgb(255, 255, 255);

    for (int row = 0; row < H; ++row) {
        for (int col = 0; col < W; ++col) {
            char c = SPRITE[row][col];
            if (c == '#') {
                Framebuffer::put_pixel(x + col, y + row, border_color);
            } else if (c == '*') {
                Framebuffer::put_pixel(x + col, y + row, fill_color);
            }
        }
    }
}

}