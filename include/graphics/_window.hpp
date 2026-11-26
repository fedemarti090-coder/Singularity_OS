#pragma once
#include <stddef.h>
#include <stdint.h>

enum class WindowKind : uint8_t {
    None = 0,
    Shell,
    Files,
    About
};

struct Window {
    uint32_t id;
    WindowKind kind;
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
    char title[32];
    bool visible;
    bool focused;
    bool dragging;
    int32_t drag_offset_x;
    int32_t drag_offset_y;
};

namespace WindowManager {

inline constexpr size_t MAX_WINDOWS = 8;
inline constexpr int32_t TITLE_H = 24;

extern Window windows[MAX_WINDOWS];
extern size_t count;

void init();
void clear_focus();
Window* create(WindowKind kind, const char* title, int32_t x, int32_t y, int32_t w, int32_t h);
void draw_one(const Window& win);
void draw_all();

}