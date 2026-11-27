#pragma once
#include <stdint.h>
#include "framebuffer.hpp"
#include "font.hpp"
#include "timer.hpp"

namespace PongGame
{
    inline constexpr int32_t PADDLE_W = 8;
    inline constexpr int32_t PADDLE_H = 44;
    inline constexpr int32_t BALL_SIZE = 8;
    inline constexpr int32_t MARGIN = 12;

    inline bool initialized = false;
    inline bool manual_paused = false;
    inline uint64_t last_tick = 0;

    inline int32_t left_y = 60;
    inline int32_t right_y = 60;
    inline int32_t ball_x = 160;
    inline int32_t ball_y = 90;
    inline int32_t ball_vx = 16;
    inline int32_t ball_vy = 12;
    inline int32_t score_left = 0;
    inline int32_t score_right = 0;

    inline int32_t clamp_i32(int32_t v, int32_t lo, int32_t hi)
    {
        if (v < lo)
            return lo;
        if (v > hi)
            return hi;
        return v;
    }

    inline void reset_ball(int32_t cw, int32_t ch, int32_t dir)
    {
        ball_x = (cw - BALL_SIZE) / 2;
        ball_y = (ch - BALL_SIZE) / 2;
        ball_vx = (dir >= 0) ? 16 : -16;
        ball_vy = ((score_left + score_right) % 2 == 0) ? 12 : -12;
    }

    inline void reset_match(int32_t cw, int32_t ch)
    {
        score_left = 0;
        score_right = 0;
        left_y = (ch - PADDLE_H) / 2;
        right_y = (ch - PADDLE_H) / 2;
        manual_paused = false;
        reset_ball(cw, ch, 1);
        initialized = true;
    }

    inline void on_key(char c, int32_t ch)
    {
        if (c == 27 || c == ' ') // ESC o Spazio
        {
            manual_paused = !manual_paused;
            return;
        }

        if (manual_paused)
        {
            if (c == 'r' || c == 'R')
            {
                initialized = false;
            }
            return;
        }

        if (c == 'w' || c == 'W')
        {
            left_y = clamp_i32(left_y - 16, 4, ch - PADDLE_H - 4);
        }
        else if (c == 's' || c == 'S')
        {
            left_y = clamp_i32(left_y + 16, 4, ch - PADDLE_H - 4);
        }
        else if (c == 'r' || c == 'R')
        {
            initialized = false;
        }
    }

    inline void update(int32_t cw, int32_t ch, bool focused)
    {
        if (!initialized)
        {
            reset_match(cw, ch);
            last_tick = Timer::ticks();
            return;
        }

        uint64_t now = Timer::ticks();
        if (now - last_tick < 2u)
        {
            return;
        }
        last_tick = now;

        if (manual_paused || !focused)
        {
            return;
        }

        left_y = clamp_i32(left_y, 4, ch - PADDLE_H - 4);
        right_y = clamp_i32(right_y, 4, ch - PADDLE_H - 4);

        int32_t ai_center = right_y + PADDLE_H / 2;
        int32_t ball_center = ball_y + BALL_SIZE / 2;
        constexpr int32_t ai_step = 11;
        if (ai_center < ball_center - 5)
        {
            right_y += ai_step;
        }
        else if (ai_center > ball_center + 5)
        {
            right_y -= ai_step;
        }
        right_y = clamp_i32(right_y, 4, ch - PADDLE_H - 4);

        ball_x += ball_vx;
        ball_y += ball_vy;

        if (ball_y <= 4)
        {
            ball_y = 4;
            if (ball_vy < 0)
                ball_vy = -ball_vy;
        }
        else if (ball_y + BALL_SIZE >= ch - 4)
        {
            ball_y = ch - 4 - BALL_SIZE;
            if (ball_vy > 0)
                ball_vy = -ball_vy;
        }

        int32_t lp_x = MARGIN;
        if (ball_vx < 0 &&
            ball_x <= lp_x + PADDLE_W &&
            ball_x + BALL_SIZE >= lp_x &&
            ball_y + BALL_SIZE >= left_y &&
            ball_y <= left_y + PADDLE_H)
        {
            ball_x = lp_x + PADDLE_W;
            ball_vx = -ball_vx;
        }

        int32_t rp_x = cw - MARGIN - PADDLE_W;
        if (ball_vx > 0 &&
            ball_x + BALL_SIZE >= rp_x &&
            ball_x <= rp_x + PADDLE_W &&
            ball_y + BALL_SIZE >= right_y &&
            ball_y <= right_y + PADDLE_H)
        {
            ball_x = rp_x - BALL_SIZE;
            ball_vx = -ball_vx;
        }

        if (ball_x < 0)
        {
            ++score_right;
            reset_ball(cw, ch, 1);
        }
        else if (ball_x + BALL_SIZE > cw)
        {
            ++score_left;
            reset_ball(cw, ch, -1);
        }
    }

    inline void draw_score_num(int32_t x, int32_t y, int32_t val, uint32_t col)
    {
        char buf[12];
        int idx = 0;
        if (val <= 0)
        {
            buf[idx++] = '0';
        }
        else
        {
            char tmp[12];
            int t = 0;
            while (val > 0 && t < 11)
            {
                tmp[t++] = static_cast<char>('0' + (val % 10));
                val /= 10;
            }
            while (t > 0)
            {
                buf[idx++] = tmp[--t];
            }
        }
        buf[idx] = '\0';
        Font8x16::draw_text(x, y, buf, col);
    }

    inline void draw_content(int32_t cx, int32_t cy, int32_t cw, int32_t ch, bool focused)
    {
        const uint32_t acc_col = Framebuffer::rgb(138, 237, 128);
        const uint32_t text_col = Framebuffer::rgb(245, 247, 250);
        const uint32_t dim_col = Framebuffer::rgb(140, 148, 162);
        const uint32_t line_col = Framebuffer::rgb(46, 52, 64);
        const uint32_t box_bg = Framebuffer::rgb(14, 16, 22);
        const uint32_t box_border = Framebuffer::rgb(138, 237, 128);

        if (cw < 160 || ch < 90)
        {
            Font8x16::draw_text(cx + 8, cy + 8, "Allarga finestra", dim_col);
            return;
        }

        update(cw, ch, focused);

        int32_t mid_x = cx + cw / 2;
        for (int32_t y = cy + 6; y < cy + ch - 6; y += 12)
        {
            Framebuffer::fill_rect(mid_x - 1, y, 2, 6, line_col);
        }

        draw_score_num(mid_x - 32, cy + 8, score_left, acc_col);
        draw_score_num(mid_x + 24, cy + 8, score_right, text_col);

        Framebuffer::fill_rect(cx + MARGIN, cy + left_y, PADDLE_W, PADDLE_H, acc_col);
        Framebuffer::fill_rect(cx + cw - MARGIN - PADDLE_W, cy + right_y, PADDLE_W, PADDLE_H, text_col);
        Framebuffer::fill_rect(cx + ball_x, cy + ball_y, BALL_SIZE, BALL_SIZE, acc_col);

        bool show_pause = manual_paused || !focused;
        if (show_pause)
        {
            const char *title = "PAUSA";
            const char *hint = manual_paused ? "ESC: riprendi | R: reset" : "Click per riprendere";

            int32_t title_w = 5 * 8;
            int32_t hint_w = (manual_paused ? 24 : 19) * 8;
            int32_t box_w = ((title_w > hint_w) ? title_w : hint_w) + 28;
            int32_t box_h = 52;
            int32_t bx = cx + (cw - box_w) / 2;
            int32_t by = cy + (ch - box_h) / 2;

            Framebuffer::fill_rect(bx, by, box_w, box_h, box_bg);
            Framebuffer::fill_rect(bx, by, box_w, 2, box_border);
            Framebuffer::fill_rect(bx, by + box_h - 2, box_w, 2, box_border);
            Framebuffer::fill_rect(bx, by, 2, box_h, box_border);
            Framebuffer::fill_rect(bx + box_w - 2, by, 2, box_h, box_border);

            Font8x16::draw_text(cx + (cw - title_w) / 2, by + 10, title, acc_col);
            Font8x16::draw_text(cx + (cw - hint_w) / 2, by + 30, hint, dim_col);
        }
    }
}