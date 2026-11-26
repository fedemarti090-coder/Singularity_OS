#include "_window.hpp"
#include "framebuffer.hpp"
#include "font.hpp"

namespace WindowManager {

Window windows[MAX_WINDOWS];
size_t count = 0;

namespace {

void copy_title(char* dst, const char* src) {
    size_t i = 0;
    if (src != nullptr) {
        while (src[i] != '\0' && i < sizeof(Window::title) - 1) {
            dst[i] = src[i];
            ++i;
        }
    }
    dst[i] = '\0';
}

void draw_client_content(const Window& win, int32_t cx, int32_t cy, int32_t cw, int32_t ch) {
    (void)cw;
    (void)ch;
    const uint32_t dim_col = Framebuffer::rgb(175, 182, 194);
    const uint32_t acc_col = Framebuffer::rgb(138, 237, 128);

    if (win.kind == WindowKind::Shell) {
        Font8x16::draw_text(cx + 10, cy + 12, "Singularity Shell ready", acc_col);
        Font8x16::draw_text(cx + 10, cy + 32, "> help", dim_col);
    } else if (win.kind == WindowKind::Files) {
        Font8x16::draw_text(cx + 10, cy + 12, "initrd:/", acc_col);
        Font8x16::draw_text(cx + 10, cy + 32, "wallpaper.bmp", dim_col);
    } else if (win.kind == WindowKind::About) {
        Font8x16::draw_text(cx + 10, cy + 12, "Singularity OS", acc_col);
        Font8x16::draw_text(cx + 10, cy + 32, "Freestanding x86 GUI", dim_col);
    }
}

} // namespace

void init() {
    count = 0;
    for (size_t i = 0; i < MAX_WINDOWS; ++i) {
        windows[i].id = 0;
        windows[i].kind = WindowKind::None;
        windows[i].x = 0;
        windows[i].y = 0;
        windows[i].w = 0;
        windows[i].h = 0;
        windows[i].title[0] = '\0';
        windows[i].visible = false;
        windows[i].focused = false;
        windows[i].dragging = false;
        windows[i].drag_offset_x = 0;
        windows[i].drag_offset_y = 0;
    }
}

void clear_focus() {
    size_t n = (count < MAX_WINDOWS) ? count : MAX_WINDOWS;
    for (size_t i = 0; i < n; ++i) {
        windows[i].focused = false;
    }
}

Window* create(WindowKind kind, const char* title, int32_t x, int32_t y, int32_t w, int32_t h) {
    if (count >= MAX_WINDOWS) {
        return nullptr;
    }
    clear_focus();

    Window& win = windows[count];
    win.id = static_cast<uint32_t>(count + 1);
    win.kind = kind;
    win.x = x;
    win.y = y;
    win.w = w;
    win.h = h;
    copy_title(win.title, title);
    win.visible = true;
    win.focused = true;
    win.dragging = false;
    win.drag_offset_x = 0;
    win.drag_offset_y = 0;

    ++count;
    return &win;
}

void draw_one(const Window& win) {
    if (!win.visible || win.w <= 0 || win.h <= 0) {
        return;
    }

    const uint32_t bg_col     = Framebuffer::rgb(20, 22, 27);
    const uint32_t title_bg   = win.focused ? Framebuffer::rgb(36, 40, 48) : Framebuffer::rgb(26, 28, 34);
    const uint32_t border_col = win.focused ? Framebuffer::rgb(138, 237, 128) : Framebuffer::rgb(52, 56, 68);
    const uint32_t text_col   = Framebuffer::rgb(245, 247, 250);
    const uint32_t close_bg   = Framebuffer::rgb(165, 48, 48);

    Framebuffer::fill_rect(win.x, win.y, win.w, win.h, bg_col);

    int32_t th = (win.h < TITLE_H) ? win.h : TITLE_H;
    Framebuffer::fill_rect(win.x, win.y, win.w, th, title_bg);

    if (win.w >= 28 && th >= 18) {
        int32_t btn_size = 16;
        int32_t bx = win.x + win.w - btn_size - 4;
        int32_t by = win.y + (th - btn_size) / 2;
        Framebuffer::fill_rect(bx, by, btn_size, btn_size, close_bg);
        Font8x16::draw_text(bx + 4, win.y + (th - 16) / 2, "x", text_col);
    }

    if (th >= 16) {
        Font8x16::draw_text(win.x + 8, win.y + (th - 16) / 2, win.title, text_col);
    }

    if (win.h > th) {
        Framebuffer::fill_rect(win.x, win.y + th, win.w, 1, border_col);
    }

    Framebuffer::fill_rect(win.x, win.y, win.w, 1, border_col);
    Framebuffer::fill_rect(win.x, win.y + win.h - 1, win.w, 1, border_col);
    Framebuffer::fill_rect(win.x, win.y, 1, win.h, border_col);
    Framebuffer::fill_rect(win.x + win.w - 1, win.y, 1, win.h, border_col);

    int32_t cx = win.x + 1;
    int32_t cy = win.y + th + 1;
    int32_t cw = win.w - 2;
    int32_t ch = win.h - th - 2;
    if (cw > 0 && ch > 0) {
        draw_client_content(win, cx, cy, cw, ch);
    }
}

void draw_all() {
    size_t n = (count < MAX_WINDOWS) ? count : MAX_WINDOWS;
    for (size_t i = 0; i < n; ++i) {
        draw_one(windows[i]);
    }
}

} 