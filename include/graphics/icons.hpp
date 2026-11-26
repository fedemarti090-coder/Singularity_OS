#pragma once
#include <stddef.h>
#include <stdint.h>
#include "framebuffer.hpp"

enum class IconId : uint8_t {
    Start = 0,
    Terminal,
    Files,
    Info,
    Power,
    Count
};

struct IconSpec {
    const char* const* rows;
    uint32_t primary;
    uint32_t accent;
};

namespace Icons {

inline constexpr int SIZE = 16;

inline constexpr const char* START_SPRITE[SIZE] = {
    "................",
    "....######......",
    "..##########....",
    ".####....####...",
    ".###..##..###...",
    "###..####..###..",
    "###..####..###..",
    "###...##...###..",
    "###........###..",
    ".###......###...",
    ".####....####...",
    "..##########....",
    "....######......",
    "................",
    "................",
    "................"
};

inline constexpr const char* TERM_SPRITE[SIZE] = {
    ".##############.",
    "#..............#",
    "#..##..........#",
    "#...##.........#",
    "#....##..####..#",
    "#...##.........#",
    "#..##..........#",
    "#..............#",
    "#..............#",
    "#..............#",
    "#..............#",
    "#..............#",
    "#..............#",
    "#..............#",
    ".##############.",
    "................"
};

inline constexpr const char* INFO_SPRITE[SIZE] = {
    "....######......",
    "..##########....",
    ".####....####...",
    ".####..##.####..",
    ".####.....####..",
    ".####.######.###",
    ".####....####...",
    ".####.######.###",
    ".####.######.###",
    ".####.######.###",
    ".####....####...",
    "..##########....",
    "....######......",
    "................",
    "................",
    "................"
};

inline constexpr const char* POWER_SPRITE[SIZE] = {
    ".......##.......",
    ".......##.......",
    "....###..###....",
    "..###......###..",
    ".##..........##.",
    ".##..........##.",
    ".##..........##.",
    ".##..........##.",
    ".##..........##.",
    "..###......###..",
    "....########....",
    "................",
    "................",
    "................",
    "................",
    "................"
};

inline constexpr const char* FILES_SPRITE[SIZE] = {
    "................",
    "..######........",
    ".########.......",
    ".##############.",
    "#..............#",
    "#..............#",
    "#..............#",
    "#..............#",
    "#..............#",
    "#..............#",
    "#..............#",
    "#..............#",
    "#..............#",
    ".##############.",
    "................",
    "................"
};

inline const IconSpec SPECS[static_cast<size_t>(IconId::Count)] = {
    { START_SPRITE, Framebuffer::rgb(245, 247, 250), Framebuffer::rgb(138, 237, 128) },
    { TERM_SPRITE,  Framebuffer::rgb(245, 247, 250), Framebuffer::rgb(138, 237, 128) },
    { FILES_SPRITE, Framebuffer::rgb(245, 247, 250), Framebuffer::rgb(138, 237, 128) },
    { INFO_SPRITE,  Framebuffer::rgb(245, 247, 250), Framebuffer::rgb(138, 237, 128) },
    { POWER_SPRITE, Framebuffer::rgb(245, 247, 250), Framebuffer::rgb(255, 107, 0)   }
};

inline const IconSpec& get_spec(IconId id) {
    size_t idx = static_cast<size_t>(id);
    if (idx >= static_cast<size_t>(IconId::Count)) {
        idx = 0;
    }
    return SPECS[idx];
}

inline void draw(IconId id, int32_t x, int32_t y, bool use_custom_primary = false, uint32_t custom_primary = 0) {
    const IconSpec& spec = get_spec(id);
    uint32_t fg = use_custom_primary ? custom_primary : spec.primary;

    for (int32_t row = 0; row < SIZE; ++row) {
        const char* line = spec.rows[row];
        for (int32_t col = 0; col < SIZE; ++col) {
            char ch = line[col];
            if (ch == '#') {
                Framebuffer::put_pixel(x + col, y + row, fg);
            } else if (ch == 'A') {
                Framebuffer::put_pixel(x + col, y + row, spec.accent);
            }
        }
    }
}

}