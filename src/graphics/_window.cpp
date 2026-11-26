#include "_window.hpp"
#include "framebuffer.hpp"
#include "font.hpp"

namespace WindowManager {

Window windows[MAX_WINDOWS];
size_t count = 0;

namespace {

constexpr int32_t CLOSE_BTN_SIZE = 16;
constexpr int32_t CLOSE_BTN_MARGIN = 4;
bool prev_left_down = false;

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

int32_t titlebar_height(const Window& win) {
    return (win.h < TITLE_H) ? win.h : TITLE_H;
}

bool has_close_button(const Window& win) {
    int32_t th = titlebar_height(win);
    return win.w >= 28 && th >= 18;
}

void close_button_rect(const Window& win, int32_t& bx, int32_t& by, int32_t& bw, int32_t& bh) {
    int32_t th = titlebar_height(win);
    bw = CLOSE_BTN_SIZE;
    bh = CLOSE_BTN_SIZE;
    bx = win.x + win.w - bw - CLOSE_BTN_MARGIN;
    by = win.y + (th - bh) / 2;
}

bool point_in_rect(int32_t px, int32_t py, int32_t rx, int32_t ry, int32_t rw, int32_t rh) {
    return px >= rx && px < (rx + rw) && py >= ry && py < (ry + rh);
}

void focus_top_visible() {
    clear_focus();
    size_t n = (count < MAX_WINDOWS) ? count : MAX_WINDOWS;
    for (size_t i = n; i > 0; --i) {
        Window& w = windows[i - 1];
        if (w.visible) {
            w.focused = true;
            return;
        }
    }
}

void bring_to_front(size_t idx) {
    size_t n = (count < MAX_WINDOWS) ? count : MAX_WINDOWS;
    if (n == 0 || idx >= n) {
        return;
    }
    if (idx + 1 < n) {
        Window target = windows[idx];
        for (size_t i = idx; i + 1 < n; ++i) {
            windows[i] = windows[i + 1];
        }
        windows[n - 1] = target;
    }
    clear_focus();
    windows[n - 1].focused = true;
}

void stop_all_drags() {
    size_t n = (count < MAX_WINDOWS) ? count : MAX_WINDOWS;
    for (size_t i = 0; i < n; ++i) {
        windows[i].dragging = false;
    }
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
    prev_left_down = false;
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

bool on_mouse(int32_t mx, int32_t my, bool left_down) {
    bool changed = false;
    bool pressed_edge = left_down && !prev_left_down;
    bool released_edge = !left_down && prev_left_down;
    prev_left_down = left_down;

    size_t n = (count < MAX_WINDOWS) ? count : MAX_WINDOWS;

    if (released_edge) {
        for (size_t i = 0; i < n; ++i) {
            if (windows[i].dragging) {
                windows[i].dragging = false;
                changed = true;
            }
        }
        return changed;
    }

    if (left_down && !pressed_edge) {
        for (size_t i = 0; i < n; ++i) {
            Window& w = windows[i];
            if (w.visible && w.dragging) {
                int32_t new_x = mx - w.drag_offset_x;
                int32_t new_y = my - w.drag_offset_y;
                if (new_y < 0) {
                    new_y = 0;
                }
                if (new_x != w.x || new_y != w.y) {
                    w.x = new_x;
                    w.y = new_y;
                    changed = true;
                }
                return changed;
            }
        }
        return false;
    }

    if (!pressed_edge) {
        return false;
    }

    stop_all_drags();

    for (size_t i = n; i > 0; --i) {
        size_t idx = i - 1;
        Window& w = windows[idx];
        if (!w.visible || w.w <= 0 || w.h <= 0) {
            continue;
        }
        if (!point_in_rect(mx, my, w.x, w.y, w.w, w.h)) {
            continue;
        }

        if (has_close_button(w)) {
            int32_t bx, by, bw, bh;
            close_button_rect(w, bx, by, bw, bh);
            if (point_in_rect(mx, my, bx, by, bw, bh)) {
                w.visible = false;
                w.focused = false;
                w.dragging = false;
                focus_top_visible();
                return true;
            }
        }

        bring_to_front(idx);
        Window& top = windows[n - 1];
        int32_t th = titlebar_height(top);
        if (point_in_rect(mx, my, top.x, top.y, top.w, th)) {
            top.dragging = true;
            top.drag_offset_x = mx - top.x;
            top.drag_offset_y = my - top.y;
        }
        return true;
    }

    return false;
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

    int32_t th = titlebar_height(win);
    Framebuffer::fill_rect(win.x, win.y, win.w, th, title_bg);

    if (has_close_button(win)) {
        int32_t bx, by, bw, bh;
        close_button_rect(win, bx, by, bw, bh);
        Framebuffer::fill_rect(bx, by, bw, bh, close_bg);
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