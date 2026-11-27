#include "_window.hpp"
#include "framebuffer.hpp"
#include "font.hpp"
#include "shell.hpp"
#include "timer.hpp"
#include "initrd.hpp"
#include "app/text_editor.hpp"
#include "games/pong.hpp"
#include "games/snake.hpp"

namespace WindowManager
{
    Window windows[MAX_WINDOWS];
    size_t count = 0;

    namespace
    {

        constexpr int32_t BTN_SIZE = 16;
        constexpr int32_t BTN_MARGIN = 4;
        constexpr int32_t BTN_GAP = 4;
        constexpr int32_t TASKBAR_RESERVE_H = 36;
        constexpr int32_t RESIZE_GRAB = 5;
        constexpr int32_t MIN_WIN_W = 140;
        constexpr int32_t MIN_WIN_H = 72;

        constexpr uint8_t RESIZE_NONE = 0;
        constexpr uint8_t RESIZE_LEFT = 1u << 0;
        constexpr uint8_t RESIZE_RIGHT = 1u << 1;
        constexpr uint8_t RESIZE_TOP = 1u << 2;
        constexpr uint8_t RESIZE_BOTTOM = 1u << 3;

        bool prev_left_down = false;

        size_t cstr_len(const char *s)
        {
            size_t n = 0;
            if (s != nullptr)
            {
                while (s[n] != '\0')
                {
                    ++n;
                }
            }
            return n;
        }

        void copy_title(char *dst, const char *src)
        {
            size_t i = 0;
            if (src != nullptr)
            {
                while (src[i] != '\0' && i < sizeof(Window::title) - 1)
                {
                    dst[i] = src[i];
                    ++i;
                }
            }
            dst[i] = '\0';
        }

        void draw_text_clipped(int32_t x, int32_t y, const char *s, int32_t max_px, uint32_t col)
        {
            if (s == nullptr || max_px < 8)
            {
                return;
            }
            char buf[128];
            size_t max_chars = static_cast<size_t>(max_px / 8);
            if (max_chars >= sizeof(buf))
            {
                max_chars = sizeof(buf) - 1;
            }
            size_t i = 0;
            while (i < max_chars && s[i] != '\0')
            {
                buf[i] = s[i];
                ++i;
            }
            buf[i] = '\0';
            Font8x16::draw_text(x, y, buf, col);
        }

        int32_t titlebar_height(const Window &win)
        {
            return (win.h < TITLE_H) ? win.h : TITLE_H;
        }

        bool has_title_buttons(const Window &win)
        {
            int32_t th = titlebar_height(win);
            return win.w >= 48 && th >= 18;
        }

        bool has_close_button(const Window &win)
        {
            int32_t th = titlebar_height(win);
            return win.w >= 28 && th >= 18;
        }

        void close_button_rect(const Window &win, int32_t &bx, int32_t &by, int32_t &bw, int32_t &bh)
        {
            int32_t th = titlebar_height(win);
            bw = BTN_SIZE;
            bh = BTN_SIZE;
            bx = win.x + win.w - bw - BTN_MARGIN;
            by = win.y + (th - bh) / 2;
        }

        void max_button_rect(const Window &win, int32_t &bx, int32_t &by, int32_t &bw, int32_t &bh)
        {
            int32_t cx, cy, cw, ch;
            close_button_rect(win, cx, cy, cw, ch);
            (void)cw;
            bw = BTN_SIZE;
            bh = BTN_SIZE;
            bx = cx - BTN_GAP - bw;
            by = cy;
        }

        bool point_in_rect(int32_t px, int32_t py, int32_t rx, int32_t ry, int32_t rw, int32_t rh)
        {
            return px >= rx && px < (rx + rw) && py >= ry && py < (ry + rh);
        }

        uint8_t hit_test_resize_edge(const Window &win, int32_t mx, int32_t my)
        {
            if (win.maximized || win.w <= 0 || win.h <= 0)
            {
                return RESIZE_NONE;
            }
            if (!point_in_rect(mx, my, win.x, win.y, win.w, win.h))
            {
                return RESIZE_NONE;
            }

            uint8_t edge = RESIZE_NONE;
            if (mx < win.x + RESIZE_GRAB)
            {
                edge |= RESIZE_LEFT;
            }
            else if (mx >= win.x + win.w - RESIZE_GRAB)
            {
                edge |= RESIZE_RIGHT;
            }

            if (my < win.y + RESIZE_GRAB)
            {
                edge |= RESIZE_TOP;
            }
            else if (my >= win.y + win.h - RESIZE_GRAB)
            {
                edge |= RESIZE_BOTTOM;
            }

            return edge;
        }

        void apply_resize(Window &win, int32_t mx, int32_t my)
        {
            int32_t dx = mx - win.resize_mouse_x;
            int32_t dy = my - win.resize_mouse_y;

            int32_t new_x = win.resize_start_x;
            int32_t new_y = win.resize_start_y;
            int32_t new_w = win.resize_start_w;
            int32_t new_h = win.resize_start_h;

            int32_t screen_w = static_cast<int32_t>(Framebuffer::width());
            int32_t screen_h = static_cast<int32_t>(Framebuffer::height());
            if (screen_w <= 0)
                screen_w = 1024;
            if (screen_h <= 0)
                screen_h = 768;

            int32_t max_bottom = screen_h - TASKBAR_RESERVE_H;
            if (max_bottom < MIN_WIN_H)
            {
                max_bottom = screen_h;
            }

            if (win.resize_edge & RESIZE_RIGHT)
            {
                new_w = win.resize_start_w + dx;
                if (new_w < MIN_WIN_W)
                {
                    new_w = MIN_WIN_W;
                }
                if (new_x + new_w > screen_w)
                {
                    new_w = screen_w - new_x;
                }
            }

            if (win.resize_edge & RESIZE_LEFT)
            {
                int32_t right = win.resize_start_x + win.resize_start_w;
                new_x = win.resize_start_x + dx;
                if (new_x < 0)
                {
                    new_x = 0;
                }
                if (right - new_x < MIN_WIN_W)
                {
                    new_x = right - MIN_WIN_W;
                }
                new_w = right - new_x;
            }

            if (win.resize_edge & RESIZE_BOTTOM)
            {
                new_h = win.resize_start_h + dy;
                if (new_h < MIN_WIN_H)
                {
                    new_h = MIN_WIN_H;
                }
                if (new_y + new_h > max_bottom)
                {
                    new_h = max_bottom - new_y;
                }
            }

            if (win.resize_edge & RESIZE_TOP)
            {
                int32_t bottom = win.resize_start_y + win.resize_start_h;
                new_y = win.resize_start_y + dy;
                if (new_y < 0)
                {
                    new_y = 0;
                }
                if (bottom - new_y < MIN_WIN_H)
                {
                    new_y = bottom - MIN_WIN_H;
                }
                new_h = bottom - new_y;
            }

            win.x = new_x;
            win.y = new_y;
            win.w = new_w;
            win.h = new_h;
        }

        void focus_top_visible()
        {
            clear_focus();
            size_t n = (count < MAX_WINDOWS) ? count : MAX_WINDOWS;
            for (size_t i = n; i > 0; --i)
            {
                Window &w = windows[i - 1];
                if (w.visible)
                {
                    w.focused = true;
                    return;
                }
            }
        }

        void bring_to_front(size_t idx)
        {
            size_t n = (count < MAX_WINDOWS) ? count : MAX_WINDOWS;
            if (n == 0 || idx >= n)
            {
                return;
            }
            if (idx + 1 < n)
            {
                Window target = windows[idx];
                for (size_t i = idx; i + 1 < n; ++i)
                {
                    windows[i] = windows[i + 1];
                }
                windows[n - 1] = target;
            }
            clear_focus();
            windows[n - 1].focused = true;
        }

        void stop_all_interactions()
        {
            size_t n = (count < MAX_WINDOWS) ? count : MAX_WINDOWS;
            for (size_t i = 0; i < n; ++i)
            {
                windows[i].dragging = false;
                windows[i].resizing = false;
                windows[i].resize_edge = RESIZE_NONE;
            }
        }

        void toggle_maximize(Window &win)
        {
            if (!win.maximized)
            {
                win.restore_x = win.x;
                win.restore_y = win.y;
                win.restore_w = win.w;
                win.restore_h = win.h;

                int32_t screen_w = static_cast<int32_t>(Framebuffer::width());
                int32_t screen_h = static_cast<int32_t>(Framebuffer::height());
                if (screen_w <= 0)
                    screen_w = 1024;
                if (screen_h <= 0)
                    screen_h = 768;

                int32_t usable_h = screen_h - TASKBAR_RESERVE_H;
                if (usable_h < TITLE_H + 20)
                {
                    usable_h = screen_h;
                }

                win.x = 0;
                win.y = 0;
                win.w = screen_w;
                win.h = usable_h;
                win.maximized = true;
            }
            else
            {
                win.x = win.restore_x;
                win.y = win.restore_y;
                win.w = win.restore_w;
                win.h = win.restore_h;
                win.maximized = false;
            }
            win.dragging = false;
            win.resizing = false;
            win.resize_edge = RESIZE_NONE;
        }

        void files_bar_rects(const Window &win, int32_t cx, int32_t cy, int32_t cw, int32_t ch,
                             int32_t &bar_y, int32_t &txt_bx, int32_t &txt_w,
                             int32_t &img_bx, int32_t &img_w, int32_t &btn_h)
        {
            (void)cy;
            const char *txt_lbl = "TEXT";
            const char *img_lbl = "IMG";
            txt_w = static_cast<int32_t>(cstr_len(txt_lbl)) * 8 + 10;
            img_w = static_cast<int32_t>(cstr_len(img_lbl)) * 8 + 10;
            btn_h = 20;
            int32_t gap = 6;

            bar_y = cy + ch - 28;
            img_bx = cx + cw - 8 - img_w;
            txt_bx = img_bx - gap - txt_w;
            (void)win;
        }

        void draw_client_content(const Window &win, int32_t cx, int32_t cy, int32_t cw, int32_t ch)
        {
            const uint32_t dim_col = Framebuffer::rgb(175, 182, 194);
            const uint32_t acc_col = Framebuffer::rgb(138, 237, 128);
            const uint32_t text_col = Framebuffer::rgb(245, 247, 250);
            const uint32_t border_col = win.focused ? Framebuffer::rgb(138, 237, 128) : Framebuffer::rgb(52, 56, 68);

            if (win.kind == WindowKind::Shell)
            {
                const uint32_t cmd_col = Framebuffer::rgb(138, 237, 128);
                const uint32_t rsp_col = Framebuffer::rgb(210, 216, 226);

                int32_t top_y = cy + 8;
                int32_t prompt_y = cy + ch - 24;
                int32_t max_hist_lines = (prompt_y - top_y) / 16;
                if (max_hist_lines < 0)
                {
                    max_hist_lines = 0;
                }

                size_t total = Shell::history_count();
                size_t max_h = static_cast<size_t>(max_hist_lines);
                size_t start_idx = (total > max_h) ? (total - max_h) : 0;
                int32_t inner_w = cw - 16;

                int32_t y = top_y;
                for (size_t i = start_idx; i < total; ++i)
                {
                    uint32_t col = (Shell::history_kind_at(i) == ShellLineKind::Command) ? cmd_col : rsp_col;
                    draw_text_clipped(cx + 8, y, Shell::history_at(i), inner_w, col);
                    y += 16;
                }

                if (prompt_y >= top_y && prompt_y + 16 <= cy + ch - 4)
                {
                    const char *cur = Shell::current_input();
                    size_t cur_len = 0;
                    while (cur[cur_len] != '\0')
                    {
                        ++cur_len;
                    }

                    int32_t prompt_x = cx + 8;
                    int32_t prompt_px = 13 * 8;
                    draw_text_clipped(prompt_x, prompt_y, "singularity> ", inner_w, acc_col);

                    int32_t avail_input_px = inner_w - prompt_px;
                    if (avail_input_px >= 8)
                    {
                        int32_t input_x = prompt_x + prompt_px;
                        draw_text_clipped(input_x, prompt_y, cur, avail_input_px, text_col);

                        int32_t cursor_x = input_x + static_cast<int32_t>(cur_len) * 8;
                        bool blink_on = ((Timer::ticks() / 50u) % 2u) == 0u;
                        if (win.focused && blink_on && cursor_x + 8 <= cx + cw - 8)
                        {
                            Framebuffer::fill_rect(cursor_x, prompt_y + 2, 8, 12, acc_col);
                        }
                    }
                }
            }
            else if (win.kind == WindowKind::Files)
            {
                const uint32_t sel_bg = Framebuffer::rgb(38, 52, 44);
                const uint32_t dir_col = Framebuffer::rgb(120, 210, 255);
                const uint32_t btn_txt_bg = Framebuffer::rgb(42, 58, 74);
                const uint32_t btn_img_bg = Framebuffer::rgb(58, 46, 74);

                draw_text_clipped(cx + 8, cy + 6, "initrd:/", cw - 16, acc_col);

                if (!InitRd::is_ready())
                {
                    draw_text_clipped(cx + 8, cy + 28, "initrd not ready", cw - 16, dim_col);
                    return;
                }

                int32_t list_y = cy + 28;
                int32_t row_h = 18;
                int32_t bar_y = 0, txt_bx = 0, txt_w = 0, img_bx = 0, img_w = 0, btn_h = 0;
                files_bar_rects(win, cx, cy, cw, ch, bar_y, txt_bx, txt_w, img_bx, img_w, btn_h);
                size_t n = InitRd::count();

                for (size_t i = 0; i < n; ++i)
                {
                    const InitRdEntry *e = InitRd::get_at(i);
                    if (e == nullptr)
                    {
                        continue;
                    }
                    int32_t ry = list_y + static_cast<int32_t>(i) * row_h;
                    if (bar_y > list_y + 16 && ry + row_h > bar_y - 8)
                    {
                        break;
                    }
                    if (ry + row_h > cy + ch - 4)
                    {
                        break;
                    }
                    if (static_cast<int32_t>(i) == win.selected_file && cw > 12)
                    {
                        Framebuffer::fill_rect(cx + 6, ry - 1, cw - 12, row_h, sel_bg);
                    }
                    uint32_t col = e->is_dir ? dir_col : text_col;
                    draw_text_clipped(cx + 12, ry, e->name, cw - 20, col);
                }

                if (bar_y > list_y + 16 && cw > 12)
                {
                    Framebuffer::fill_rect(cx + 6, bar_y - 6, cw - 12, 1, border_col);

                    const char *open_lbl = "apri come:";
                    int32_t open_w = static_cast<int32_t>(cstr_len(open_lbl)) * 8;
                    int32_t lbl_x = txt_bx - 8 - open_w;

                    if (lbl_x >= cx + 8)
                    {
                        Font8x16::draw_text(lbl_x, bar_y + 2, open_lbl, dim_col);
                    }

                    const char *file_lbl = "nessun file";
                    if (win.selected_file >= 0 && static_cast<size_t>(win.selected_file) < n)
                    {
                        const InitRdEntry *sel = InitRd::get_at(static_cast<size_t>(win.selected_file));
                        if (sel != nullptr)
                        {
                            file_lbl = sel->name;
                        }
                    }

                    int32_t max_file_px = ((lbl_x >= cx + 8) ? lbl_x : txt_bx) - 8 - (cx + 10);
                    if (max_file_px >= 8)
                    {
                        draw_text_clipped(cx + 10, bar_y + 2, file_lbl, max_file_px, text_col);
                    }

                    if (txt_bx > cx + 10)
                    {
                        Framebuffer::fill_rect(txt_bx, bar_y, txt_w, btn_h, btn_txt_bg);
                        Font8x16::draw_text(txt_bx + 5, bar_y + 2, "TEXT", text_col);
                    }
                    if (img_bx > cx + 10)
                    {
                        Framebuffer::fill_rect(img_bx, bar_y, img_w, btn_h, btn_img_bg);
                        Font8x16::draw_text(img_bx + 5, bar_y + 2, "IMG", text_col);
                    }
                }
            }
            else if (win.kind == WindowKind::TextViewer)
            {
                TextEditor::draw_content(cx, cy, cw, ch, win.focused);
            }
            else if (win.kind == WindowKind::About)
            {
                if (ch >= 28)
                {
                    draw_text_clipped(cx + 10, cy + 12, "Singularity OS", cw - 20, acc_col);
                }
                if (ch >= 48)
                {
                    draw_text_clipped(cx + 10, cy + 32, "Freestanding x86 GUI", cw - 20, dim_col);
                }
            }
            else if (win.kind == WindowKind::Pong)
            {
                PongGame::draw_content(cx, cy, cw, ch, win.focused);
            }
            else if (win.kind == WindowKind::Snake)
            {
                SnakeGame::draw_content(cx, cy, cw, ch, win.focused);
            }
        }

    } // namespace

    void init()
    {
        count = 0;
        prev_left_down = false;
        for (size_t i = 0; i < MAX_WINDOWS; ++i)
        {
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
            windows[i].resizing = false;
            windows[i].resize_edge = RESIZE_NONE;
            windows[i].resize_start_x = 0;
            windows[i].resize_start_y = 0;
            windows[i].resize_start_w = 0;
            windows[i].resize_start_h = 0;
            windows[i].resize_mouse_x = 0;
            windows[i].resize_mouse_y = 0;
            windows[i].selected_file = -1;
            windows[i].maximized = false;
            windows[i].restore_x = 0;
            windows[i].restore_y = 0;
            windows[i].restore_w = 0;
            windows[i].restore_h = 0;
        }
    }

    void clear_focus()
    {
        size_t n = (count < MAX_WINDOWS) ? count : MAX_WINDOWS;
        for (size_t i = 0; i < n; ++i)
        {
            windows[i].focused = false;
        }
    }

    Window *create(WindowKind kind, const char *title, int32_t x, int32_t y, int32_t w, int32_t h)
    {
        if (count >= MAX_WINDOWS)
        {
            return nullptr;
        }
        clear_focus();

        Window &win = windows[count];
        win.id = static_cast<uint32_t>(count + 1);
        win.kind = kind;
        win.x = x;
        win.y = y;
        win.w = (w < MIN_WIN_W) ? MIN_WIN_W : w;
        win.h = (h < MIN_WIN_H) ? MIN_WIN_H : h;
        copy_title(win.title, title);
        win.visible = true;
        win.focused = true;
        win.dragging = false;
        win.drag_offset_x = 0;
        win.drag_offset_y = 0;
        win.resizing = false;
        win.resize_edge = RESIZE_NONE;
        win.resize_start_x = 0;
        win.resize_start_y = 0;
        win.resize_start_w = 0;
        win.resize_start_h = 0;
        win.resize_mouse_x = 0;
        win.resize_mouse_y = 0;
        win.selected_file = -1;
        win.maximized = false;
        win.restore_x = 0;
        win.restore_y = 0;
        win.restore_w = 0;
        win.restore_h = 0;

        ++count;
        return &win;
    }

    bool on_mouse(int32_t mx, int32_t my, bool left_down)
    {
        bool changed = false;
        bool pressed_edge = left_down && !prev_left_down;
        bool released_edge = !left_down && prev_left_down;
        prev_left_down = left_down;

        size_t n = (count < MAX_WINDOWS) ? count : MAX_WINDOWS;

        if (released_edge)
        {
            for (size_t i = 0; i < n; ++i)
            {
                if (windows[i].dragging || windows[i].resizing)
                {
                    windows[i].dragging = false;
                    windows[i].resizing = false;
                    windows[i].resize_edge = RESIZE_NONE;
                    changed = true;
                }
            }
            return changed;
        }

        if (left_down && !pressed_edge)
        {
            for (size_t i = 0; i < n; ++i)
            {
                Window &w = windows[i];
                if (w.visible && w.resizing && !w.maximized)
                {
                    int32_t old_x = w.x;
                    int32_t old_y = w.y;
                    int32_t old_w = w.w;
                    int32_t old_h = w.h;
                    apply_resize(w, mx, my);
                    if (w.x != old_x || w.y != old_y || w.w != old_w || w.h != old_h)
                    {
                        changed = true;
                    }
                    return changed;
                }
                if (w.visible && w.dragging && !w.maximized)
                {
                    int32_t new_x = mx - w.drag_offset_x;
                    int32_t new_y = my - w.drag_offset_y;
                    if (new_y < 0)
                    {
                        new_y = 0;
                    }
                    if (new_x != w.x || new_y != w.y)
                    {
                        w.x = new_x;
                        w.y = new_y;
                        changed = true;
                    }
                    return changed;
                }
            }
            return false;
        }

        if (!pressed_edge)
        {
            return false;
        }

        stop_all_interactions();

        for (size_t i = n; i > 0; --i)
        {
            size_t idx = i - 1;
            Window &w = windows[idx];
            if (!w.visible || w.w <= 0 || w.h <= 0)
            {
                continue;
            }
            if (!point_in_rect(mx, my, w.x, w.y, w.w, w.h))
            {
                continue;
            }

            if (has_title_buttons(w))
            {
                int32_t bx, by, bw, bh;
                close_button_rect(w, bx, by, bw, bh);
                if (point_in_rect(mx, my, bx, by, bw, bh))
                {
                    w.visible = false;
                    w.focused = false;
                    w.dragging = false;
                    w.resizing = false;
                    w.resize_edge = RESIZE_NONE;
                    w.maximized = false;
                    focus_top_visible();
                    return true;
                }

                int32_t mbx, mby, mbw, mbh;
                max_button_rect(w, mbx, mby, mbw, mbh);
                if (point_in_rect(mx, my, mbx, mby, mbw, mbh))
                {
                    bring_to_front(idx);
                    toggle_maximize(windows[n - 1]);
                    return true;
                }
            }
            else if (has_close_button(w))
            {
                int32_t bx, by, bw, bh;
                close_button_rect(w, bx, by, bw, bh);
                if (point_in_rect(mx, my, bx, by, bw, bh))
                {
                    w.visible = false;
                    w.focused = false;
                    w.dragging = false;
                    w.resizing = false;
                    w.resize_edge = RESIZE_NONE;
                    w.maximized = false;
                    focus_top_visible();
                    return true;
                }
            }

            bring_to_front(idx);
            Window &top = windows[n - 1];

            uint8_t edge = hit_test_resize_edge(top, mx, my);
            if (edge != RESIZE_NONE)
            {
                top.resizing = true;
                top.resize_edge = edge;
                top.resize_start_x = top.x;
                top.resize_start_y = top.y;
                top.resize_start_w = top.w;
                top.resize_start_h = top.h;
                top.resize_mouse_x = mx;
                top.resize_mouse_y = my;
                return true;
            }

            int32_t th = titlebar_height(top);
            if (point_in_rect(mx, my, top.x, top.y, top.w, th))
            {
                if (!top.maximized)
                {
                    top.dragging = true;
                    top.drag_offset_x = mx - top.x;
                    top.drag_offset_y = my - top.y;
                }
            }
            else if (top.kind == WindowKind::Files)
            {
                int32_t cx = top.x + 1;
                int32_t cy = top.y + th + 1;
                int32_t cw = top.w - 2;
                int32_t ch = top.h - th - 2;
                int32_t list_top = cy + 28;
                int32_t row_h = 18;

                int32_t bar_y = 0, txt_bx = 0, txt_w = 0, img_bx = 0, img_w = 0, btn_h = 0;
                files_bar_rects(top, cx, cy, cw, ch, bar_y, txt_bx, txt_w, img_bx, img_w, btn_h);

                if (bar_y > list_top + 16 && point_in_rect(mx, my, txt_bx, bar_y, txt_w, btn_h))
                {
                    if (top.selected_file >= 0 && static_cast<size_t>(top.selected_file) < InitRd::count())
                    {
                        const InitRdEntry *sel = InitRd::get_at(static_cast<size_t>(top.selected_file));
                        if (sel != nullptr && !sel->is_dir)
                        {
                            TextEditor::open_file(sel->name);
                            open_or_focus(WindowKind::TextViewer, "Text Editor", 180, 120, 480, 320);
                        }
                    }
                    return true;
                }

                if (bar_y > list_top + 16 && point_in_rect(mx, my, img_bx, bar_y, img_w, btn_h))
                {
                    return true;
                }

                if (my >= list_top && mx >= top.x + 8 && mx < top.x + top.w - 8)
                {
                    int32_t row = (my - list_top) / row_h;
                    if (row >= 0 && static_cast<size_t>(row) < InitRd::count())
                    {
                        top.selected_file = row;
                    }
                }
            }
            else if (top.kind == WindowKind::TextViewer)
            {
                int32_t cx = top.x + 1;
                int32_t cy = top.y + th + 1;
                int32_t cw = top.w - 2;
                int32_t ch = top.h - th - 2;
                TextEditor::on_mouse_click(mx, my, cx, cy, cw, ch);
            }
            return true;
        }

        return false;
    }

    void draw_one(const Window &win)
    {
        if (!win.visible || win.w <= 0 || win.h <= 0)
        {
            return;
        }

        const uint32_t bg_col = Framebuffer::rgb(20, 22, 27);
        const uint32_t title_bg = win.focused ? Framebuffer::rgb(36, 40, 48) : Framebuffer::rgb(26, 28, 34);
        const uint32_t border_col = win.resizing
                                        ? Framebuffer::rgb(255, 196, 86)
                                        : (win.focused ? Framebuffer::rgb(138, 237, 128) : Framebuffer::rgb(52, 56, 68));
        const uint32_t text_col = Framebuffer::rgb(245, 247, 250);
        const uint32_t close_bg = Framebuffer::rgb(165, 48, 48);

        Framebuffer::fill_rect(win.x, win.y, win.w, win.h, bg_col);

        int32_t th = titlebar_height(win);
        Framebuffer::fill_rect(win.x, win.y, win.w, th, title_bg);

        if (has_title_buttons(win))
        {
            const uint32_t max_bg = Framebuffer::rgb(48, 54, 66);

            int32_t mbx, mby, mbw, mbh;
            max_button_rect(win, mbx, mby, mbw, mbh);
            Framebuffer::fill_rect(mbx, mby, mbw, mbh, max_bg);

            int32_t ix = mbx + 4;
            int32_t iy = mby + 4;
            int32_t iw = mbw - 8;
            int32_t ih = mbh - 8;
            Framebuffer::fill_rect(ix, iy, iw, 1, text_col);
            Framebuffer::fill_rect(ix, iy + ih - 1, iw, 1, text_col);
            Framebuffer::fill_rect(ix, iy, 1, ih, text_col);
            Framebuffer::fill_rect(ix + iw - 1, iy, 1, ih, text_col);

            int32_t bx, by, bw, bh;
            close_button_rect(win, bx, by, bw, bh);
            Framebuffer::fill_rect(bx, by, bw, bh, close_bg);
            Font8x16::draw_text(bx + 4, win.y + (th - 16) / 2, "x", text_col);
        }
        else if (has_close_button(win))
        {
            int32_t bx, by, bw, bh;
            close_button_rect(win, bx, by, bw, bh);
            Framebuffer::fill_rect(bx, by, bw, bh, close_bg);
            Font8x16::draw_text(bx + 4, win.y + (th - 16) / 2, "x", text_col);
        }

        if (th >= 16)
        {
            int32_t title_max_px = win.w - 16;
            if (has_title_buttons(win))
            {
                title_max_px = win.w - (BTN_SIZE * 2 + BTN_GAP + BTN_MARGIN * 2 + 12);
            }
            else if (has_close_button(win))
            {
                title_max_px = win.w - (BTN_SIZE + BTN_MARGIN * 2 + 12);
            }
            draw_text_clipped(win.x + 8, win.y + (th - 16) / 2, win.title, title_max_px, text_col);
        }

        if (win.h > th)
        {
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
        if (cw > 0 && ch > 0)
        {
            draw_client_content(win, cx, cy, cw, ch);
        }

        if (!win.maximized && win.w >= 24 && win.h >= th + 16)
        {
            int32_t gx = win.x + win.w - 8;
            int32_t gy = win.y + win.h - 8;
            Framebuffer::fill_rect(gx + 3, gy + 3, 3, 3, border_col);
            Framebuffer::fill_rect(gx, gy + 5, 2, 1, border_col);
            Framebuffer::fill_rect(gx + 5, gy, 1, 2, border_col);
        }
    }

    void draw_all()
    {
        size_t n = (count < MAX_WINDOWS) ? count : MAX_WINDOWS;
        for (size_t i = 0; i < n; ++i)
        {
            draw_one(windows[i]);
        }
    }

    Window *open_or_focus(WindowKind kind, const char *title, int32_t x, int32_t y, int32_t w, int32_t h)
    {
        size_t n = (count < MAX_WINDOWS) ? count : MAX_WINDOWS;
        for (size_t i = 0; i < n; ++i)
        {
            if (windows[i].kind == kind)
            {
                windows[i].visible = true;
                bring_to_front(i);
                return &windows[n - 1];
            }
        }
        return create(kind, title, x, y, w, h);
    }

    Window *focused_window()
    {
        size_t n = (count < MAX_WINDOWS) ? count : MAX_WINDOWS;
        for (size_t i = n; i > 0; --i)
        {
            Window &w = windows[i - 1];
            if (w.visible && w.focused)
            {
                return &w;
            }
        }
        return nullptr;
    }

}