#include "app/calculator.hpp"
#include "framebuffer.hpp"
#include "font.hpp"

namespace
{
    constexpr size_t MAX_DIGITS = 10;

    char g_expr[32] = "0";
    char g_display[24] = "0";
    int32_t g_acc = 0;
    int32_t g_current = 0;
    char g_pending_op = '\0';
    bool g_has_acc = false;
    bool g_entering_new = true;
    bool g_error = false;
    bool g_dirty = true;

    struct KeyBtn
    {
        const char *label;
        int32_t col;
        int32_t row;
        int32_t span_w;
    };

    constexpr KeyBtn KEYS[] = {
        {"7", 0, 0, 1},
        {"8", 1, 0, 1},
        {"9", 2, 0, 1},
        {"/", 3, 0, 1},
        {"4", 0, 1, 1},
        {"5", 1, 1, 1},
        {"6", 2, 1, 1},
        {"*", 3, 1, 1},
        {"1", 0, 2, 1},
        {"2", 1, 2, 1},
        {"3", 2, 2, 1},
        {"-", 3, 2, 1},
        {"C", 0, 3, 1},
        {"0", 1, 3, 1},
        {"=", 2, 3, 1},
        {"+", 3, 3, 1},
    };
    constexpr size_t NUM_KEYS = sizeof(KEYS) / sizeof(KEYS[0]);

    void cstr_copy(char *dst, size_t cap, const char *src)
    {
        if (cap == 0)
        {
            return;
        }
        size_t i = 0;
        if (src != nullptr)
        {
            while (i + 1 < cap && src[i] != '\0')
            {
                dst[i] = src[i];
                ++i;
            }
        }
        dst[i] = '\0';
    }

    void format_i32(char *out, size_t cap, int32_t val)
    {
        if (cap < 2)
        {
            if (cap == 1)
                out[0] = '\0';
            return;
        }
        if (val == 0)
        {
            out[0] = '0';
            out[1] = '\0';
            return;
        }

        bool neg = false;
        uint32_t uval = 0;
        if (val < 0)
        {
            neg = true;
            uval = static_cast<uint32_t>(-(val + 1)) + 1u;
        }
        else
        {
            uval = static_cast<uint32_t>(val);
        }

        char tmp[16];
        size_t len = 0;
        while (uval > 0 && len < sizeof(tmp))
        {
            uint32_t rem = uval % 10u;
            tmp[len++] = static_cast<char>('0' + rem);
            uval /= 10u;
        }

        size_t pos = 0;
        if (neg && pos + 1 < cap)
        {
            out[pos++] = '-';
        }
        while (len > 0 && pos + 1 < cap)
        {
            out[pos++] = tmp[--len];
        }
        out[pos] = '\0';
    }

    void sync_display()
    {
        if (g_error)
        {
            cstr_copy(g_display, sizeof(g_display), "ERR DIV 0");
            return;
        }
        if (g_entering_new && g_has_acc)
        {
            format_i32(g_display, sizeof(g_display), g_acc);
        }
        else
        {
            format_i32(g_display, sizeof(g_display), g_current);
        }
    }

    void reset_all()
    {
        g_acc = 0;
        g_current = 0;
        g_pending_op = '\0';
        g_has_acc = false;
        g_entering_new = true;
        g_error = false;
        cstr_copy(g_expr, sizeof(g_expr), "0");
        cstr_copy(g_display, sizeof(g_display), "0");
        g_dirty = true;
    }

    bool apply_op(int32_t a, int32_t b, char op, int32_t &out)
    {
        switch (op)
        {
        case '+':
            out = a + b;
            return true;
        case '-':
            out = a - b;
            return true;
        case '*':
            out = a * b;
            return true;
        case '/':
            if (b == 0)
            {
                return false;
            }
            out = a / b;
            return true;
        default:
            out = b;
            return true;
        }
    }

    void press_digit(char d)
    {
        if (g_error)
        {
            reset_all();
        }
        if (g_entering_new)
        {
            g_current = static_cast<int32_t>(d - '0');
            g_entering_new = false;
        }
        else
        {
            char tmp[16];
            format_i32(tmp, sizeof(tmp), g_current);
            size_t len = 0;
            while (tmp[len] != '\0')
                ++len;
            if (len < MAX_DIGITS)
            {
                int32_t sign = (g_current < 0) ? -1 : 1;
                uint32_t abs_v = (g_current < 0)
                                     ? (static_cast<uint32_t>(-(g_current + 1)) + 1u)
                                     : static_cast<uint32_t>(g_current);
                abs_v = abs_v * 10u + static_cast<uint32_t>(d - '0');
                g_current = sign * static_cast<int32_t>(abs_v);
            }
        }
        sync_display();
        g_dirty = true;
    }

    void press_operator(char op)
    {
        if (g_error)
        {
            reset_all();
            return;
        }

        if (!g_has_acc)
        {
            g_acc = g_current;
            g_has_acc = true;
        }
        else if (!g_entering_new && g_pending_op != '\0')
        {
            int32_t res = 0;
            if (!apply_op(g_acc, g_current, g_pending_op, res))
            {
                g_error = true;
                sync_display();
                g_dirty = true;
                return;
            }
            g_acc = res;
        }

        g_pending_op = op;
        g_entering_new = true;

        char num_buf[16];
        format_i32(num_buf, sizeof(num_buf), g_acc);
        g_expr[0] = '\0';
        size_t p = 0;
        for (size_t i = 0; num_buf[i] != '\0' && p + 3 < sizeof(g_expr); ++i)
        {
            g_expr[p++] = num_buf[i];
        }
        g_expr[p++] = ' ';
        g_expr[p++] = op;
        g_expr[p] = '\0';

        sync_display();
        g_dirty = true;
    }

    void press_equals()
    {
        if (g_error)
        {
            reset_all();
            return;
        }

        if (g_has_acc && g_pending_op != '\0')
        {
            int32_t rhs = g_entering_new ? g_acc : g_current;
            int32_t res = 0;
            if (!apply_op(g_acc, rhs, g_pending_op, res))
            {
                g_error = true;
                sync_display();
                g_dirty = true;
                return;
            }
            g_acc = res;
            g_current = res;
            g_pending_op = '\0';
            g_has_acc = false;
            g_entering_new = true;
            cstr_copy(g_expr, sizeof(g_expr), "=");
            sync_display();
            g_dirty = true;
        }
    }

    void compute_grid(int32_t cx, int32_t cy, int32_t cw, int32_t ch,
                      int32_t &gx, int32_t &gy, int32_t &cell_w, int32_t &cell_h, int32_t &pad)
    {
        pad = 6;
        int32_t disp_h = 56;
        gx = cx + 10;
        gy = cy + 10 + disp_h;
        int32_t avail_w = cw - 20;
        int32_t avail_h = ch - (gy - cy) - 10;
        if (avail_w < 40)
            avail_w = 40;
        if (avail_h < 40)
            avail_h = 40;

        cell_w = (avail_w - pad * 3) / 4;
        cell_h = (avail_h - pad * 3) / 4;
        if (cell_w < 16)
            cell_w = 16;
        if (cell_h < 16)
            cell_h = 16;
    }
}

namespace CalculatorApp
{
    void init()
    {
        reset_all();
    }

    bool consume_dirty()
    {
        bool d = g_dirty;
        g_dirty = false;
        return d;
    }

    void on_key(char c)
    {
        if (c >= '0' && c <= '9')
        {
            press_digit(c);
        }
        else if (c == '+' || c == '-' || c == '*' || c == '/')
        {
            press_operator(c);
        }
        else if (c == '=' || c == '\n' || c == '\r')
        {
            press_equals();
        }
        else if (c == 'c' || c == 'C' || c == 27)
        {
            reset_all();
        }
        else if (c == '\b' || c == 127)
        {
            if (!g_entering_new && !g_error)
            {
                g_current /= 10;
                sync_display();
                g_dirty = true;
            }
        }
    }

    void on_mouse_click(int32_t mx, int32_t my, int32_t cx, int32_t cy, int32_t cw, int32_t ch)
    {
        int32_t gx = 0, gy = 0, cell_w = 0, cell_h = 0, pad = 0;
        compute_grid(cx, cy, cw, ch, gx, gy, cell_w, cell_h, pad);

        for (size_t i = 0; i < NUM_KEYS; ++i)
        {
            const KeyBtn &k = KEYS[i];
            int32_t bx = gx + k.col * (cell_w + pad);
            int32_t by = gy + k.row * (cell_h + pad);
            int32_t bw = k.span_w * cell_w + (k.span_w - 1) * pad;
            int32_t bh = cell_h;

            if (mx >= bx && mx < (bx + bw) && my >= by && my < (by + bh))
            {
                char ch_lbl = k.label[0];
                if (ch_lbl >= '0' && ch_lbl <= '9')
                {
                    press_digit(ch_lbl);
                }
                else if (ch_lbl == 'C')
                {
                    reset_all();
                }
                else if (ch_lbl == '=')
                {
                    press_equals();
                }
                else
                {
                    press_operator(ch_lbl);
                }
                return;
            }
        }
    }

    void draw_content(int32_t cx, int32_t cy, int32_t cw, int32_t ch, bool focused)
    {
        (void)focused;
        if (cw <= 16 || ch <= 60)
        {
            return;
        }

        const uint32_t disp_bg = Framebuffer::rgb(11, 12, 16);
        const uint32_t inner_border = Framebuffer::rgb(52, 56, 68);
        const uint32_t expr_col = Framebuffer::rgb(150, 160, 175);
        const uint32_t val_col = g_error ? Framebuffer::rgb(255, 95, 95) : Framebuffer::rgb(245, 247, 250);
        const uint32_t num_bg = Framebuffer::rgb(24, 27, 33);
        const uint32_t op_bg = Framebuffer::rgb(28, 31, 38);
        const uint32_t clr_bg = Framebuffer::rgb(58, 26, 26);
        const uint32_t op_fg = Framebuffer::rgb(138, 237, 128);
        const uint32_t txt_fg = Framebuffer::rgb(245, 247, 250);

        int32_t disp_x = cx + 10;
        int32_t disp_y = cy + 10;
        int32_t disp_w = cw - 20;
        int32_t disp_h = 48;

        Framebuffer::fill_rect(disp_x, disp_y, disp_w, disp_h, disp_bg);
        Framebuffer::fill_rect(disp_x, disp_y, disp_w, 1, inner_border);
        Framebuffer::fill_rect(disp_x, disp_y + disp_h - 1, disp_w, 1, inner_border);
        Framebuffer::fill_rect(disp_x, disp_y, 1, disp_h, inner_border);
        Framebuffer::fill_rect(disp_x + disp_w - 1, disp_y, 1, disp_h, inner_border);

        Font8x16::draw_text(disp_x + 8, disp_y + 4, g_expr, expr_col);
        Font8x16::draw_text(disp_x + 8, disp_y + 24, g_display, val_col);

        int32_t gx = 0, gy = 0, cell_w = 0, cell_h = 0, pad = 0;
        compute_grid(cx, cy, cw, ch, gx, gy, cell_w, cell_h, pad);

        for (size_t i = 0; i < NUM_KEYS; ++i)
        {
            const KeyBtn &k = KEYS[i];
            int32_t bx = gx + k.col * (cell_w + pad);
            int32_t by = gy + k.row * (cell_h + pad);
            int32_t bw = k.span_w * cell_w + (k.span_w - 1) * pad;
            int32_t bh = cell_h;

            char ch_lbl = k.label[0];
            uint32_t bg = num_bg;
            uint32_t fg = txt_fg;
            if (ch_lbl == 'C')
            {
                bg = clr_bg;
            }
            else if (ch_lbl == '=' || ch_lbl == '+' || ch_lbl == '-' || ch_lbl == '*' || ch_lbl == '/')
            {
                bg = op_bg;
                fg = op_fg;
            }

            Framebuffer::fill_rect(bx, by, bw, bh, bg);
            Framebuffer::fill_rect(bx, by, bw, 1, inner_border);
            Framebuffer::fill_rect(bx, by + bh - 1, bw, 1, inner_border);
            Framebuffer::fill_rect(bx, by, 1, bh, inner_border);
            Framebuffer::fill_rect(bx + bw - 1, by, 1, bh, inner_border);

            int32_t tx = bx + (bw - 8) / 2;
            int32_t ty = by + (bh - 16) / 2;
            Font8x16::draw_text(tx, ty, k.label, fg);
        }
    }
}