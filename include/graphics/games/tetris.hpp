#pragma once
#include <stdint.h>
#include "framebuffer.hpp"
#include "font.hpp"
#include "timer.hpp"

namespace TetrisGame
{
    inline constexpr int32_t COLS = 10;
    inline constexpr int32_t ROWS = 20;

    inline bool initialized = false;
    inline bool manual_paused = false;
    inline bool game_over = false;
    inline uint64_t last_tick = 0;
    inline uint32_t rng_state = 0x2468ACE1u;

    // 0 = vuoto, 1..7 = tipo pezzo (colore)
    inline uint8_t board[ROWS][COLS];

    inline int32_t cur_piece = 0;
    inline int32_t cur_rot = 0;
    inline int32_t cur_x = 3;
    inline int32_t cur_y = 0;
    inline int32_t next_piece = 1;

    inline int32_t score = 0;
    inline int32_t lines = 0;
    inline int32_t level = 1;

    // 7 pezzi x 4 rotazioni x 4 celle (dx, dy)
    inline constexpr int8_t SHAPES[7][4][4][2] = {
        // 0: I (ciano)
        {
            {{0, 1}, {1, 1}, {2, 1}, {3, 1}},
            {{2, 0}, {2, 1}, {2, 2}, {2, 3}},
            {{0, 2}, {1, 2}, {2, 2}, {3, 2}},
            {{1, 0}, {1, 1}, {1, 2}, {1, 3}}},
        // 1: O (giallo)
        {
            {{1, 0}, {2, 0}, {1, 1}, {2, 1}},
            {{1, 0}, {2, 0}, {1, 1}, {2, 1}},
            {{1, 0}, {2, 0}, {1, 1}, {2, 1}},
            {{1, 0}, {2, 0}, {1, 1}, {2, 1}}},
        // 2: T (viola/lilla)
        {
            {{1, 0}, {0, 1}, {1, 1}, {2, 1}},
            {{1, 0}, {1, 1}, {2, 1}, {1, 2}},
            {{0, 1}, {1, 1}, {2, 1}, {1, 2}},
            {{1, 0}, {0, 1}, {1, 1}, {1, 2}}},
        // 3: S (verde)
        {
            {{1, 0}, {2, 0}, {0, 1}, {1, 1}},
            {{1, 0}, {1, 1}, {2, 1}, {2, 2}},
            {{1, 1}, {2, 1}, {0, 2}, {1, 2}},
            {{0, 0}, {0, 1}, {1, 1}, {1, 2}}},
        // 4: Z (rosso/arancio)
        {
            {{0, 0}, {1, 0}, {1, 1}, {2, 1}},
            {{2, 0}, {1, 1}, {2, 1}, {1, 2}},
            {{0, 1}, {1, 1}, {1, 2}, {2, 2}},
            {{1, 0}, {0, 1}, {1, 1}, {0, 2}}},
        // 5: J (blu)
        {
            {{0, 0}, {0, 1}, {1, 1}, {2, 1}},
            {{1, 0}, {2, 0}, {1, 1}, {1, 2}},
            {{0, 1}, {1, 1}, {2, 1}, {2, 2}},
            {{1, 0}, {1, 1}, {0, 2}, {1, 2}}},
        // 6: L (arancio)
        {
            {{2, 0}, {0, 1}, {1, 1}, {2, 1}},
            {{1, 0}, {1, 1}, {1, 2}, {2, 2}},
            {{0, 1}, {1, 1}, {2, 1}, {0, 2}},
            {{0, 0}, {1, 0}, {1, 1}, {1, 2}}}};

    inline uint32_t piece_color(int32_t id)
    {
        switch (id)
        {
        case 1:
            return Framebuffer::rgb(72, 210, 240); // I
        case 2:
            return Framebuffer::rgb(245, 210, 70); // O
        case 3:
            return Framebuffer::rgb(185, 120, 255); // T
        case 4:
            return Framebuffer::rgb(138, 237, 128); // S
        case 5:
            return Framebuffer::rgb(240, 85, 95); // Z
        case 6:
            return Framebuffer::rgb(90, 145, 255); // J
        case 7:
            return Framebuffer::rgb(255, 107, 0); // L
        default:
            return Framebuffer::rgb(220, 220, 220);
        }
    }

    inline uint32_t next_rand()
    {
        rng_state ^= rng_state << 13;
        rng_state ^= rng_state >> 17;
        rng_state ^= rng_state << 5;
        return rng_state;
    }

    inline int32_t rand_piece()
    {
        rng_state ^= static_cast<uint32_t>(Timer::ticks()) * 2654435761u;
        return static_cast<int32_t>(next_rand() % 7u) + 1;
    }

    inline bool can_place(int32_t piece, int32_t rot, int32_t px, int32_t py)
    {
        int32_t p_idx = piece - 1;
        if (p_idx < 0 || p_idx >= 7)
            return false;
        int32_t r_idx = ((rot % 4) + 4) % 4;

        for (int32_t i = 0; i < 4; ++i)
        {
            int32_t bx = px + SHAPES[p_idx][r_idx][i][0];
            int32_t by = py + SHAPES[p_idx][r_idx][i][1];

            if (bx < 0 || bx >= COLS || by >= ROWS)
            {
                return false;
            }
            if (by >= 0 && board[by][bx] != 0)
            {
                return false;
            }
        }
        return true;
    }

    inline void spawn_next_piece()
    {
        cur_piece = (next_piece >= 1 && next_piece <= 7) ? next_piece : rand_piece();
        next_piece = rand_piece();
        cur_rot = 0;
        cur_x = COLS / 2 - 2;
        cur_y = 0;

        if (!can_place(cur_piece, cur_rot, cur_x, cur_y))
        {
            game_over = true;
        }
    }

    inline void reset_game()
    {
        for (int32_t r = 0; r < ROWS; ++r)
        {
            for (int32_t c = 0; c < COLS; ++c)
            {
                board[r][c] = 0;
            }
        }
        score = 0;
        lines = 0;
        level = 1;
        manual_paused = false;
        game_over = false;
        next_piece = rand_piece();
        spawn_next_piece();
        initialized = true;
    }

    inline void clear_lines()
    {
        int32_t cleared = 0;
        for (int32_t r = ROWS - 1; r >= 0; --r)
        {
            bool full = true;
            for (int32_t c = 0; c < COLS; ++c)
            {
                if (board[r][c] == 0)
                {
                    full = false;
                    break;
                }
            }
            if (full)
            {
                ++cleared;
                for (int32_t rr = r; rr > 0; --rr)
                {
                    for (int32_t c = 0; c < COLS; ++c)
                    {
                        board[rr][c] = board[rr - 1][c];
                    }
                }
                for (int32_t c = 0; c < COLS; ++c)
                {
                    board[0][c] = 0;
                }
                ++r; // ricontrolla la stessa riga dopo lo scorrimento
            }
        }

        if (cleared > 0)
        {
            lines += cleared;
            level = 1 + (lines / 10);
            int32_t base = 100;
            if (cleared == 2)
                base = 300;
            else if (cleared == 3)
                base = 500;
            else if (cleared >= 4)
                base = 800;
            score += base * level;
        }
    }

    inline void lock_piece()
    {
        int32_t p_idx = cur_piece - 1;
        int32_t r_idx = ((cur_rot % 4) + 4) % 4;
        for (int32_t i = 0; i < 4; ++i)
        {
            int32_t bx = cur_x + SHAPES[p_idx][r_idx][i][0];
            int32_t by = cur_y + SHAPES[p_idx][r_idx][i][1];
            if (bx >= 0 && bx < COLS && by >= 0 && by < ROWS)
            {
                board[by][bx] = static_cast<uint8_t>(cur_piece);
            }
        }
        clear_lines();
        spawn_next_piece();
    }

    inline void move_horz(int32_t dx)
    {
        if (can_place(cur_piece, cur_rot, cur_x + dx, cur_y))
        {
            cur_x += dx;
        }
    }

    inline void rotate_piece(int32_t dr)
    {
        int32_t target_rot = ((cur_rot + dr) % 4 + 4) % 4;
        if (can_place(cur_piece, target_rot, cur_x, cur_y))
        {
            cur_rot = target_rot;
            return;
        }
        // Piccoli kick laterali per evitare blocchi vicino ai muri
        if (can_place(cur_piece, target_rot, cur_x - 1, cur_y))
        {
            cur_x -= 1;
            cur_rot = target_rot;
            return;
        }
        if (can_place(cur_piece, target_rot, cur_x + 1, cur_y))
        {
            cur_x += 1;
            cur_rot = target_rot;
            return;
        }
    }

    inline void soft_drop()
    {
        if (can_place(cur_piece, cur_rot, cur_x, cur_y + 1))
        {
            cur_y += 1;
            score += 1;
        }
        else
        {
            lock_piece();
        }
    }

    inline void hard_drop()
    {
        int32_t dropped = 0;
        while (can_place(cur_piece, cur_rot, cur_x, cur_y + 1))
        {
            cur_y += 1;
            ++dropped;
        }
        score += dropped * 2;
        lock_piece();
    }

    inline void on_key(char c)
    {
        if (!initialized)
        {
            reset_game();
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
            reset_game();
            last_tick = Timer::ticks();
            return;
        }

        if (manual_paused || game_over)
        {
            return;
        }

        if (c == 'a' || c == 'A')
            move_horz(-1);
        else if (c == 'd' || c == 'D')
            move_horz(1);
        else if (c == 's' || c == 'S')
            soft_drop();
        else if (c == 'w' || c == 'W' || c == 'x' || c == 'X')
            rotate_piece(1);
        else if (c == 'z' || c == 'Z')
            rotate_piece(-1);
    }

    inline void on_arrow(uint8_t scancode)
    {
        if (!initialized)
        {
            reset_game();
        }
        if (manual_paused || game_over)
        {
            return;
        }

        if (scancode == 0x4B)
            move_horz(-1); // Sinistra
        else if (scancode == 0x4D)
            move_horz(1); // Destra
        else if (scancode == 0x50)
            soft_drop(); // Giù
        else if (scancode == 0x48)
            rotate_piece(1); // Su = ruota
    }

    inline void update(bool focused)
    {
        if (!initialized)
        {
            reset_game();
            last_tick = Timer::ticks();
            return;
        }

        uint64_t fall_ticks = 35u;
        if (level > 1)
        {
            uint64_t dec = static_cast<uint64_t>(level - 1) * 3u;
            fall_ticks = (dec >= 27u) ? 8u : (35u - dec);
        }

        uint64_t now = Timer::ticks();
        if (now - last_tick < fall_ticks)
        {
            return;
        }
        last_tick = now;

        if (manual_paused || game_over || !focused)
        {
            return;
        }

        if (can_place(cur_piece, cur_rot, cur_x, cur_y + 1))
        {
            cur_y += 1;
        }
        else
        {
            lock_piece();
        }
    }

    inline void draw_int(int32_t x, int32_t y, const char *label, int32_t val, uint32_t col)
    {
        char buf[32];
        int i = 0;
        while (label[i] != '\0' && i < 20)
        {
            buf[i] = label[i];
            ++i;
        }

        if (val < 0)
            val = 0;
        char tmp[12];
        int t = 0;
        if (val == 0)
        {
            tmp[t++] = '0';
        }
        else
        {
            while (val > 0 && t < 11)
            {
                tmp[t++] = static_cast<char>('0' + (val % 10));
                val /= 10;
            }
        }
        while (t > 0 && i < 31)
        {
            buf[i++] = tmp[--t];
        }
        buf[i] = '\0';
        Font8x16::draw_text(x, y, buf, col);
    }

    inline void draw_content(int32_t cx, int32_t cy, int32_t cw, int32_t ch, bool focused)
    {
        const uint32_t bg_board = Framebuffer::rgb(12, 14, 20);
        const uint32_t border_col = Framebuffer::rgb(65, 75, 95);
        const uint32_t grid_col = Framebuffer::rgb(26, 30, 40);
        const uint32_t text_col = Framebuffer::rgb(245, 247, 250);
        const uint32_t dim_col = Framebuffer::rgb(140, 148, 162);
        const uint32_t acc_col = Framebuffer::rgb(138, 237, 128);
        const uint32_t err_col = Framebuffer::rgb(240, 85, 95);
        const uint32_t box_bg = Framebuffer::rgb(14, 16, 22);

        if (cw < 220 || ch < 260)
        {
            Font8x16::draw_text(cx + 8, cy + 8, "Allarga finestra", dim_col);
            return;
        }

        update(focused);

        int32_t cell_w = (cw - 140) / ROWS; // margine per pannello laterale
        int32_t cell_h = (ch - 16) / ROWS;
        int32_t cell = (cell_w < cell_h) ? cell_w : cell_h;
        if (cell < 10)
            cell = 10;

        int32_t bw = COLS * cell;
        int32_t bh = ROWS * cell;
        int32_t bx = cx + (cw - (bw + 124)) / 2;
        if (bx < cx + 8)
            bx = cx + 8;
        int32_t by = cy + (ch - bh) / 2;
        if (by < cy + 8)
            by = cy + 8;

        // Sfondo e bordo griglia
        Framebuffer::fill_rect(bx - 2, by - 2, bw + 4, bh + 4, border_col);
        Framebuffer::fill_rect(bx, by, bw, bh, bg_board);

        // Celle bloccate
        for (int32_t r = 0; r < ROWS; ++r)
        {
            for (int32_t c = 0; c < COLS; ++c)
            {
                int32_t px = bx + c * cell;
                int32_t py = by + r * cell;
                if (board[r][c] != 0)
                {
                    Framebuffer::fill_rect(px + 1, py + 1, cell - 2, cell - 2, piece_color(board[r][c]));
                }
                else
                {
                    Framebuffer::fill_rect(px, py, 1, 1, grid_col);
                }
            }
        }

        // Pezzo attivo
        if (!game_over && cur_piece >= 1 && cur_piece <= 7)
        {
            int32_t p_idx = cur_piece - 1;
            int32_t r_idx = ((cur_rot % 4) + 4) % 4;
            uint32_t col = piece_color(cur_piece);
            for (int32_t i = 0; i < 4; ++i)
            {
                int32_t gx = cur_x + SHAPES[p_idx][r_idx][i][0];
                int32_t gy = cur_y + SHAPES[p_idx][r_idx][i][1];
                if (gy >= 0 && gy < ROWS && gx >= 0 && gx < COLS)
                {
                    Framebuffer::fill_rect(bx + gx * cell + 1, by + gy * cell + 1, cell - 2, cell - 2, col);
                }
            }
        }

        // Pannello laterale: punteggio e prossimo pezzo
        int32_t px_info = bx + bw + 14;
        draw_int(px_info, by + 4, "PTS ", score, text_col);
        draw_int(px_info, by + 28, "LVL ", level, acc_col);
        draw_int(px_info, by + 52, "LIN ", lines, dim_col);

        Font8x16::draw_text(px_info, by + 84, "NEXT", dim_col);
        int32_t prev_cell = (cell > 14) ? 14 : cell;
        if (prev_cell < 8)
            prev_cell = 8;
        int32_t n_idx = next_piece - 1;
        if (n_idx >= 0 && n_idx < 7)
        {
            uint32_t ncol = piece_color(next_piece);
            for (int32_t i = 0; i < 4; ++i)
            {
                int32_t sx = px_info + SHAPES[n_idx][0][i][0] * prev_cell;
                int32_t sy = by + 106 + SHAPES[n_idx][0][i][1] * prev_cell;
                Framebuffer::fill_rect(sx + 1, sy + 1, prev_cell - 2, prev_cell - 2, ncol);
            }
        }

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
            int32_t box_x = bx + (bw - box_w) / 2;
            int32_t box_y = by + (bh - box_h) / 2;

            Framebuffer::fill_rect(box_x, box_y, box_w, box_h, box_bg);
            Framebuffer::fill_rect(box_x, box_y, box_w, 2, acc_col);
            Framebuffer::fill_rect(box_x, box_y + box_h - 2, box_w, 2, acc_col);
            Framebuffer::fill_rect(box_x, box_y, 2, box_h, acc_col);
            Framebuffer::fill_rect(box_x + box_w - 2, box_y, 2, box_h, acc_col);

            Font8x16::draw_text(bx + (bw - tw) / 2, box_y + 10, title, game_over ? err_col : acc_col);
            Font8x16::draw_text(bx + (bw - hw) / 2, box_y + 30, hint, dim_col);
        }
    }
}