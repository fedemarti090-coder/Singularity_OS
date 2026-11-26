#include "text_editor.hpp"
#include "initrd.hpp"
#include "framebuffer.hpp"
#include "font.hpp"

namespace TextEditor
{

    namespace
    {

        char g_name[MAX_NAME_LEN] = {0};
        char g_buf[MAX_TEXT_LEN] = {0};
        size_t g_len = 0;
        size_t g_cursor = 0;
        int32_t g_scroll = 0;
        bool g_open = false;
        bool g_dirty = false;
        bool g_ui_dirty = false;

        int32_t g_last_body_x = 0;
        int32_t g_last_body_y = 0;
        int32_t g_last_body_w = 0;
        int32_t g_last_body_h = 0;
        int32_t g_last_cols = 1;

        void copy_str(char *dst, const char *src, size_t max_len)
        {
            size_t i = 0;
            if (src != nullptr && max_len > 0)
            {
                while (src[i] != '\0' && i < max_len - 1)
                {
                    dst[i] = src[i];
                    ++i;
                }
            }
            if (max_len > 0)
            {
                dst[i] = '\0';
            }
        }

        void sync_null_term()
        {
            if (g_len < MAX_TEXT_LEN)
            {
                g_buf[g_len] = '\0';
            }
            else
            {
                g_buf[MAX_TEXT_LEN - 1] = '\0';
            }
        }

        void cursor_line_col(size_t pos, int32_t cols, int32_t &out_line, int32_t &out_col)
        {
            if (cols < 1)
                cols = 1;
            if (pos > g_len)
                pos = g_len;

            int32_t line = 0;
            int32_t col = 0;
            for (size_t i = 0; i < pos; ++i)
            {
                char c = g_buf[i];
                if (c == '\n' || col >= cols)
                {
                    ++line;
                    col = 0;
                    if (c != '\n')
                    {
                        col = 1;
                    }
                }
                else
                {
                    ++col;
                }
            }
            out_line = line;
            out_col = col;
        }

        size_t pos_from_line_col(int32_t target_line, int32_t target_col, int32_t cols)
        {
            if (cols < 1)
                cols = 1;
            if (target_line < 0)
                target_line = 0;
            if (target_col < 0)
                target_col = 0;

            int32_t line = 0;
            int32_t col = 0;
            size_t i = 0;

            while (i < g_len && line < target_line)
            {
                char c = g_buf[i];
                if (c == '\n' || col >= cols)
                {
                    ++line;
                    col = 0;
                    if (c == '\n')
                    {
                        ++i;
                    }
                }
                else
                {
                    ++col;
                    ++i;
                }
            }

            while (i < g_len && col < target_col)
            {
                char c = g_buf[i];
                if (c == '\n' || col >= cols)
                {
                    break;
                }
                ++col;
                ++i;
            }

            return i;
        }

        void ensure_cursor_visible(int32_t visible_rows, int32_t cols)
        {
            if (visible_rows < 1)
                visible_rows = 1;
            int32_t cur_line = 0;
            int32_t cur_col = 0;
            cursor_line_col(g_cursor, cols, cur_line, cur_col);
            (void)cur_col;

            if (cur_line < g_scroll)
            {
                g_scroll = cur_line;
            }
            else if (cur_line >= g_scroll + visible_rows)
            {
                g_scroll = cur_line - visible_rows + 1;
            }
            if (g_scroll < 0)
            {
                g_scroll = 0;
            }
        }

        void draw_u32(int32_t x, int32_t y, uint32_t val, uint32_t col)
        {
            char tmp[16];
            int32_t idx = 0;
            if (val == 0)
            {
                tmp[idx++] = '0';
            }
            else
            {
                char rev[16];
                int32_t r = 0;
                while (val > 0 && r < 15)
                {
                    rev[r++] = static_cast<char>('0' + (val % 10u));
                    val /= 10u;
                }
                while (r > 0)
                {
                    tmp[idx++] = rev[--r];
                }
            }
            tmp[idx] = '\0';
            Font8x16::draw_text(x, y, tmp, col);
        }

    } // namespace

    void init()
    {
        g_name[0] = '\0';
        g_buf[0] = '\0';
        g_len = 0;
        g_cursor = 0;
        g_scroll = 0;
        g_open = false;
        g_dirty = false;
        g_ui_dirty = false;
    }

    void new_buffer(const char *name)
    {
        copy_str(g_name, (name != nullptr && name[0] != '\0') ? name : "untitled.txt", MAX_NAME_LEN);
        g_buf[0] = '\0';
        g_len = 0;
        g_cursor = 0;
        g_scroll = 0;
        g_open = true;
        g_dirty = false;
        g_ui_dirty = true;
    }

    bool open_file(const char *name)
    {
        if (name == nullptr || name[0] == '\0')
        {
            return false;
        }

        copy_str(g_name, name, MAX_NAME_LEN);
        g_len = 0;
        g_cursor = 0;
        g_scroll = 0;
        g_dirty = false;
        g_open = true;
        g_ui_dirty = true;

        if (!InitRd::is_ready())
        {
            const char *msg = "[initrd non disponibile]";
            copy_str(g_buf, msg, MAX_TEXT_LEN);
            while (g_buf[g_len] != '\0')
            {
                ++g_len;
            }
            g_cursor = g_len;
            return true;
        }

        uint8_t raw[MAX_TEXT_LEN - 1];
        int32_t rd = InitRd::read(name, raw, sizeof(raw), 0);
        if (rd > 0)
        {
            size_t n = static_cast<size_t>(rd);
            for (size_t i = 0; i < n && g_len < MAX_TEXT_LEN - 1; ++i)
            {
                char c = static_cast<char>(raw[i]);
                if (c == '\r')
                {
                    continue;
                }
                if (c == '\t')
                {
                    for (int s = 0; s < 4 && g_len < MAX_TEXT_LEN - 1; ++s)
                    {
                        g_buf[g_len++] = ' ';
                    }
                }
                else if (c == '\n' || (c >= 32 && c <= 126))
                {
                    g_buf[g_len++] = c;
                }
            }
        }

        sync_null_term();
        g_cursor = g_len;
        return true;
    }

    void on_char(char ch)
    {
        if (!g_open || g_len >= MAX_TEXT_LEN - 1)
        {
            return;
        }
        if (ch == '\r')
        {
            ch = '\n';
        }
        if (ch != '\n' && (ch < 32 || ch > 126))
        {
            return;
        }
        if (g_cursor > g_len)
        {
            g_cursor = g_len;
        }

        for (size_t i = g_len; i > g_cursor; --i)
        {
            g_buf[i] = g_buf[i - 1];
        }
        g_buf[g_cursor] = ch;
        ++g_len;
        ++g_cursor;
        g_dirty = true;
        g_ui_dirty = true;
        sync_null_term();

        int32_t vis_rows = (g_last_body_h > 0) ? (g_last_body_h / 16) : 10;
        ensure_cursor_visible(vis_rows, g_last_cols);
    }

    void on_backspace()
    {
        if (!g_open || g_len == 0 || g_cursor == 0)
        {
            return;
        }
        if (g_cursor > g_len)
        {
            g_cursor = g_len;
        }
        for (size_t i = g_cursor - 1; i + 1 < g_len; ++i)
        {
            g_buf[i] = g_buf[i + 1];
        }
        --g_len;
        --g_cursor;
        g_dirty = true;
        g_ui_dirty = true;
        sync_null_term();

        int32_t vis_rows = (g_last_body_h > 0) ? (g_last_body_h / 16) : 10;
        ensure_cursor_visible(vis_rows, g_last_cols);
    }

    void on_key(KeyCode key)
    {
        if (!g_open)
        {
            return;
        }

        int32_t line = 0;
        int32_t col = 0;
        cursor_line_col(g_cursor, g_last_cols, line, col);

        switch (key)
        {
        case KeyCode::Left:
            if (g_cursor > 0)
            {
                --g_cursor;
                g_ui_dirty = true;
            }
            break;
        case KeyCode::Right:
            if (g_cursor < g_len)
            {
                ++g_cursor;
                g_ui_dirty = true;
            }
            break;
        case KeyCode::Up:
            if (line > 0)
            {
                g_cursor = pos_from_line_col(line - 1, col, g_last_cols);
                g_ui_dirty = true;
            }
            break;
        case KeyCode::Down:
            g_cursor = pos_from_line_col(line + 1, col, g_last_cols);
            g_ui_dirty = true;
            break;
        case KeyCode::Home:
            g_cursor = pos_from_line_col(line, 0, g_last_cols);
            g_ui_dirty = true;
            break;
        case KeyCode::End:
            g_cursor = pos_from_line_col(line, g_last_cols, g_last_cols);
            g_ui_dirty = true;
            break;
        case KeyCode::Delete:
            if (g_cursor < g_len)
            {
                for (size_t i = g_cursor; i + 1 < g_len; ++i)
                {
                    g_buf[i] = g_buf[i + 1];
                }
                --g_len;
                g_dirty = true;
                g_ui_dirty = true;
                sync_null_term();
            }
            break;
        }

        int32_t vis_rows = (g_last_body_h > 0) ? (g_last_body_h / 16) : 10;
        ensure_cursor_visible(vis_rows, g_last_cols);
    }

    void on_mouse_click(int32_t mx, int32_t my, int32_t cx, int32_t cy, int32_t cw, int32_t ch)
    {
        (void)cx;
        (void)cy;
        (void)cw;
        (void)ch;
        if (!g_open || g_last_body_w <= 0 || g_last_body_h <= 0)
        {
            return;
        }
        if (mx < g_last_body_x || mx >= g_last_body_x + g_last_body_w ||
            my < g_last_body_y || my >= g_last_body_y + g_last_body_h)
        {
            return;
        }

        int32_t rel_row = (my - g_last_body_y) / 16;
        int32_t rel_col = (mx - g_last_body_x) / 8;
        if (rel_row < 0)
            rel_row = 0;
        if (rel_col < 0)
            rel_col = 0;
        if (rel_col > g_last_cols)
            rel_col = g_last_cols;

        g_cursor = pos_from_line_col(g_scroll + rel_row, rel_col, g_last_cols);
        g_ui_dirty = true;
    }

    void scroll_lines(int32_t delta)
    {
        g_scroll += delta;
        if (g_scroll < 0)
        {
            g_scroll = 0;
        }
        g_ui_dirty = true;
    }

    bool save_to_initrd()
    {
        if (!g_open)
        {
            return false;
        }
        g_dirty = false;
        g_ui_dirty = true;
        return true;
    }

    bool is_open()
    {
        return g_open;
    }

    const char *file_name()
    {
        return g_name;
    }

    bool is_dirty()
    {
        return g_dirty;
    }

    bool consume_dirty()
    {
        bool d = g_ui_dirty;
        g_ui_dirty = false;
        return d;
    }

    void draw_content(int32_t cx, int32_t cy, int32_t cw, int32_t ch, bool focused)
    {
        const uint32_t bar_bg = Framebuffer::rgb(28, 31, 38);
        const uint32_t line_col = Framebuffer::rgb(65, 70, 82);
        const uint32_t acc_col = Framebuffer::rgb(138, 237, 128);
        const uint32_t dim_col = Framebuffer::rgb(165, 172, 185);
        const uint32_t text_col = Framebuffer::rgb(245, 247, 250);

        int32_t top_h = 24;
        int32_t bot_h = 22;
        if (ch <= top_h + bot_h + 12 || cw <= 16)
        {
            return;
        }

        Framebuffer::fill_rect(cx, cy, cw, top_h, bar_bg);
        Framebuffer::fill_rect(cx, cy + top_h, cw, 1, line_col);

        const char *title = (g_name[0] != '\0') ? g_name : "untitled.txt";
        Font8x16::draw_text(cx + 8, cy + 4, title, acc_col);
        if (g_dirty)
        {
            Font8x16::draw_text(cx + cw - 20, cy + 4, "*", acc_col);
        }

        int32_t status_y = cy + ch - bot_h;
        Framebuffer::fill_rect(cx, status_y - 1, cw, 1, line_col);
        Framebuffer::fill_rect(cx, status_y, cw, bot_h, bar_bg);

        int32_t body_x = cx + 8;
        int32_t body_y = cy + top_h + 6;
        int32_t body_w = cw - 16;
        int32_t body_h = (status_y - 4) - body_y;
        if (body_w < 8 || body_h < 16)
        {
            return;
        }

        int32_t max_cols = body_w / 8;
        if (max_cols < 1)
            max_cols = 1;
        if (max_cols > 120)
            max_cols = 120;

        int32_t max_rows = body_h / 16;
        if (max_rows < 1)
            max_rows = 1;

        g_last_body_x = body_x;
        g_last_body_y = body_y;
        g_last_body_w = body_w;
        g_last_body_h = body_h;
        g_last_cols = max_cols;

        int32_t cur_row = 0;
        int32_t cur_col = 0;
        cursor_line_col(g_cursor, max_cols, cur_row, cur_col);

        char line_buf[128];
        int32_t col_accum = 0;
        int32_t logical_line = 0;

        for (size_t i = 0; i <= g_len; ++i)
        {
            bool at_end = (i == g_len);
            char c = at_end ? '\n' : g_buf[i];

            if (c == '\n' || col_accum >= max_cols)
            {
                if (logical_line >= g_scroll && (logical_line - g_scroll) < max_rows)
                {
                    line_buf[col_accum] = '\0';
                    int32_t py = body_y + (logical_line - g_scroll) * 16;
                    Font8x16::draw_text(body_x, py, line_buf, text_col);
                }
                ++logical_line;
                col_accum = 0;
                if (c != '\n' && !at_end)
                {
                    line_buf[col_accum++] = c;
                }
            }
            else
            {
                line_buf[col_accum++] = c;
            }
        }

        if (g_len == 0 && !g_dirty)
        {
            Font8x16::draw_text(body_x, body_y, "Inizia a scrivere...", dim_col);
        }

        if (focused)
        {
            int32_t vis_row = cur_row - g_scroll;
            if (vis_row >= 0 && vis_row < max_rows && cur_col <= max_cols)
            {
                int32_t px = body_x + cur_col * 8;
                int32_t py = body_y + vis_row * 16;
                Framebuffer::fill_rect(px, py, 2, 16, acc_col);
            }
        }

        int32_t sy = status_y + 3;
        Font8x16::draw_text(cx + 8, sy, "Ln ", dim_col);
        draw_u32(cx + 32, sy, static_cast<uint32_t>(cur_row + 1), text_col);
        Font8x16::draw_text(cx + 72, sy, "Col ", dim_col);
        draw_u32(cx + 104, sy, static_cast<uint32_t>(cur_col + 1), text_col);

        if (cw >= 240)
        {
            Font8x16::draw_text(cx + cw - 96, sy, "B:", dim_col);
            draw_u32(cx + cw - 76, sy, static_cast<uint32_t>(g_len), acc_col);
        }
    }

}