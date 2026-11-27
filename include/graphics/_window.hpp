#pragma once
#include <stddef.h>
#include <stdint.h>

enum class WindowKind : uint8_t
{
    None = 0,
    Shell = 1,
    Files = 2,
    About = 3,
    TextViewer = 4,
    ImageViewer = 5,
    Pong = 6
};

// main struct di una finestra software
struct Window
{
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
    bool resizing;
    uint8_t resize_edge;
    int32_t resize_start_x;
    int32_t resize_start_y;
    int32_t resize_start_w;
    int32_t resize_start_h;
    int32_t resize_mouse_x;
    int32_t resize_mouse_y;
    int32_t selected_file;
    bool maximized;
    int32_t restore_x;
    int32_t restore_y;
    int32_t restore_w;
    int32_t restore_h;
    char opened_file[64];
};

namespace WindowManager
{

    inline constexpr size_t MAX_WINDOWS = 8;
    inline constexpr int32_t TITLE_H = 24;

    extern Window windows[MAX_WINDOWS];
    extern size_t count;

    void init();
    void clear_focus();
    Window *create(WindowKind kind, const char *title, int32_t x, int32_t y, int32_t w, int32_t h);
    Window *open_or_focus(WindowKind kind, const char *title, int32_t x, int32_t y, int32_t w, int32_t h);
    bool on_mouse(int32_t mx, int32_t my, bool left_down);
    void draw_one(const Window &win);
    void draw_all();
    Window *focused_window();

}