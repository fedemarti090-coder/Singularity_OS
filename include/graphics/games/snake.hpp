#pragma once
#include <stdint.h>
#include "framebuffer.hpp"
#include "font.hpp"
#include "timer.hpp"

namespace SnakeGame
{
    inline constexpr int32_t CELL = 16;
    inline constexpr int32_t MAX_LEN = 256;

    enum class Dir : uint8_t
    {
        Up = 0,
        Right = 1,
        Down = 2,
        Left = 3
    };

    inline bool initialized = false;
    inline bool manual_paused = false;
    inline bool game_over = false;
    inline uint64_t last_tick = 0;
    inline uint32_t rng_state = 0x13579BDFu;

    inline int32_t seg_x[MAX_LEN];
    inline int32_t seg_y[MAX_LEN];
    inline int32_t length = 4;
    inline Dir dir = Dir::Right;
    inline Dir next_dir = Dir::Right;

    inline int32_t food_x = 10;
    inline int32_t food_y = 8;
    inline int32_t score = 0;

    inline uint32_t next_rand()
    {
        rng_state ^= rng_state << 13;
        rng_state ^= rng_state >> 17;
        rng_state ^= rng_state << 5;
        return rng_state;
    }

    inline bool occupies(int32_t gx, int32_t gy)
    {
        for (int32_t i = 0; i < length; ++i)
        {
            if (seg_x[i] == gx && seg_y[i] == gy)
            {
                return true;
            }
        }
        return false;
    }

    inline void spawn_food(int32_t cols, int32_t rows)
    {
        if (cols <= 0 || rows <= 0)
        {
            return;
        }
        rng_state ^= static_cast<uint32_t>(Timer::ticks()) * 2654435761u;
        for (int attempt = 0; attempt < 64; ++attempt)
        {
            int32_t fx = static_cast<int32_t>(next_rand() % static_cast<uint32_t>(cols));
            int32_t fy = static_cast<int32_t>(next_rand() % static_cast<uint32_t>(rows));
            if (!occupies(fx, fy))
            {
                food_x = fx;
                food_y = fy;
                return;
            }
        }
        food_x = 0;
        food_y = 0;
    }

    inline void reset_game(int32_t cw, int32_t ch)
    {
        int32_t cols = cw / CELL;
        int32_t rows = ch / CELL;
        if (cols < 6)
            cols = 6;
        if (rows < 6)
            rows = 6;

        int32_t start_x = cols / 2;
        int32_t start_y = rows / 2;
        if (start_x < 4)
            start_x = 4;

        length = 4;
        for (int32_t i = 0; i < length; ++i)
        {
            seg_x[i] = start_x - i;
            seg_y[i] = start_y;
        }

        dir = Dir::Right;
        next_dir = Dir::Right;
        score = 0;
        manual_paused = false;
        game_over = false;
        spawn_food(cols, rows);
        initialized = true;
    }

    inline void set_dir(Dir d)
    {
        if (d == Dir::Up && dir != Dir::Down)
            next_dir = Dir::Up;
        else if (d == Dir::Down && dir != Dir::Up)
            next_dir = Dir::Down;
        else if (d == Dir::Left && dir != Dir::Right)
            next_dir = Dir::Left;
        else if (d == Dir::Right && dir != Dir::Left)
            next_dir = Dir::Right;
    }

    inline void on_key(char c, int32_t cw, int32_t ch)
    {
        if (!initialized)
        {
            reset_game(cw, ch);
        }

        if (c == 27 || c == ' ')
        {
            if (!game_over)
            {
                manual_paused = !manual_paused;
            }
            return;
        }

        if (c == 'r' || c == 'R')
        {
            reset_game(cw, ch);
            last_tick = Timer::ticks();
            return;
        }

        if (manual_paused || game_over)
        {
            return;
        }

        if (c == 'w' || c == 'W')
            set_dir(Dir::Up);
        else if (c == 's' || c == 'S')
            set_dir(Dir::Down);
        else if (c == 'a' || c == 'A')
            set_dir(Dir::Left);
        else if (c == 'd' || c == 'D')
            set_dir(Dir::Right);
    }

    inline void on_arrow(uint8_t scancode, int32_t cw, int32_t ch)
    {
        if (!initialized)
        {
            reset_game(cw, ch);
        }
        if (manual_paused || game_over)
        {
            return;
        }

        if (scancode == 0x48)
            set_dir(Dir::Up);
        else if (scancode == 0x50)
            set_dir(Dir::Down);
        else if (scancode == 0x4B)
            set_dir(Dir::Left);
        else if (scancode == 0x4D)
            set_dir(Dir::Right);
    }

    inline void update(int32_t cw, int32_t ch, bool focused)
    {
        int32_t cols = cw / CELL;
        int32_t rows = ch / CELL;
        if (cols < 4 || rows < 4)
        {
            return;
        }

        if (!initialized)
        {
            reset_game(cw, ch);
            last_tick = Timer::ticks();
            return;
        }

        uint64_t now = Timer::ticks();
        if (now - last_tick < 8u) // ~12.5 mosse/sec se PIT = 100 Hz
        {
            return;
        }
        last_tick = now;

        if (manual_paused || game_over || !focused)
        {
            return;
        }

        dir = next_dir;
        int32_t nx = seg_x[0];
        int32_t ny = seg_y[0];

        if (dir == Dir::Up)
            ny -= 1;
        else if (dir == Dir::Down)
            ny += 1;
        else if (dir == Dir::Left)
            nx -= 1;
        else if (dir == Dir::Right)
            nx += 1;

        if (nx < 0 || ny < 0 || nx >= cols || ny >= rows)
        {
            game_over = true;
            return;
        }

        bool ate = (nx == food_x && ny == food_y);
        int32_t check_len = ate ? length : (length - 1);
        for (int32_t i = 0; i < check_len; ++i)
        {
            if (seg_x[i] == nx && seg_y[i] == ny)
            {
                game_over = true;
                return;
            }
        }

        if (ate && length < MAX_LEN)
        {
            ++length;
        }

        for (int32_t i = length - 1; i > 0; --i)
        {
            seg_x[i] = seg_x[i - 1];
            seg_y[i] = seg_y[i - 1];
        }
        seg_x[0] = nx;
        seg_y[0] = ny;

        if (ate)
        {
            ++score;
            spawn_food(cols, rows);
        }
    }

    inline void draw_score(int32_t x, int32_t y, int32_t val, uint32_t col)
    {
        char buf[16] = {'S', 'c', 'o', 'r', 'e', ':', ' ', '0', '\0'};
        if (val > 0)
        {
            char tmp[12];
            int t = 0;
            while (val > 0 && t < 11)
            {
                tmp[t++] = static_cast<char>('0' + (val % 10));
                val /= 10;
            }
            int idx = 7;
            while (t > 0 && idx < 15)
            {
                buf[idx++] = tmp[--t];
            }
            buf[idx] = '\0';
        }
        Font8x16::draw_text(x, y, buf, col);
    }

    inline void draw_content(int32_t cx, int32_t cy, int32_t cw, int32_t ch, bool focused)
    {
        const uint32_t head_col = Framebuffer::rgb(138, 237, 128);
        const uint32_t body_col = Framebuffer::rgb(109, 192, 90);
        const uint32_t food_col = Framebuffer::rgb(255, 107, 0);
        const uint32_t text_col = Framebuffer::rgb(245, 247, 250);
        const uint32_t dim_col = Framebuffer::rgb(140, 148, 162);
        const uint32_t box_bg = Framebuffer::rgb(14, 16, 22);

        if (cw < 160 || ch < 120)
        {
            Font8x16::draw_text(cx + 8, cy + 8, "Allarga finestra", dim_col);
            return;
        }

        update(cw, ch, focused);

        int32_t cols = cw / CELL;
        int32_t rows = ch / CELL;
        int32_t board_w = cols * CELL;
        int32_t board_h = rows * CELL;
        int32_t ox = cx + (cw - board_w) / 2;
        int32_t oy = cy + (ch - board_h) / 2;

        // Cibo
        Framebuffer::fill_rect(ox + food_x * CELL + 2, oy + food_y * CELL + 2, CELL - 4, CELL - 4, food_col);

        // Corpo e testa
        for (int32_t i = length - 1; i >= 0; --i)
        {
            uint32_t col = (i == 0) ? head_col : body_col;
            Framebuffer::fill_rect(ox + seg_x[i] * CELL + 1, oy + seg_y[i] * CELL + 1, CELL - 2, CELL - 2, col);
        }

        draw_score(ox + 8, oy + 6, score, text_col);

        if (manual_paused || !focused || game_over)
        {
            const char *title = game_over ? "GAME OVER" : "PAUSA";
            const char *hint = game_over
                                   ? "R: ricomincia"
                                   : (manual_paused ? "ESC: riprendi | R: reset" : "Click per riprendere");

            int32_t tw = (game_over ? 9 : 5) * 8;
            int32_t hw = (game_over ? 13 : (manual_paused ? 24 : 19)) * 8;
            int32_t box_w = ((tw > hw) ? tw : hw) + 28;
            int32_t box_h = 52;
            int32_t bx = cx + (cw - box_w) / 2;
            int32_t by = cy + (ch - box_h) / 2;

            Framebuffer::fill_rect(bx, by, box_w, box_h, box_bg);
            Framebuffer::fill_rect(bx, by, box_w, 2, head_col);
            Framebuffer::fill_rect(bx, by + box_h - 2, box_w, 2, head_col);
            Framebuffer::fill_rect(bx, by, 2, box_h, head_col);
            Framebuffer::fill_rect(bx + box_w - 2, by, 2, box_h, head_col);

            Font8x16::draw_text(cx + (cw - tw) / 2, by + 10, title, game_over ? food_col : head_col);
            Font8x16::draw_text(cx + (cw - hw) / 2, by + 30, hint, dim_col);
        }
    }
}