#pragma once
#include <stdint.h>
#include "framebuffer.hpp"
#include "font.hpp"
#include "timer.hpp"

namespace SpaceInvadersGame
{
    inline constexpr int32_t SPEED_SCALE = 2;

    inline constexpr int32_t INV_ROWS = 3;
    inline constexpr int32_t INV_COLS = 8;
    inline constexpr int32_t MAX_ENEMY_BULLETS = 4;

    inline constexpr int32_t PLAYER_W = 22;
    inline constexpr int32_t PLAYER_H = 10;
    inline constexpr int32_t INV_W = 16;
    inline constexpr int32_t INV_H = 12;
    inline constexpr int32_t INV_GAP_X = 10;
    inline constexpr int32_t INV_GAP_Y = 10;

    inline bool initialized = false;
    inline bool manual_paused = false;
    inline bool game_over = false;
    inline uint64_t last_tick = 0;
    inline uint32_t rng_state = 0x5A17C39Du;

    inline int32_t player_x = 120;
    inline bool p_bullet_active = false;
    inline int32_t p_bullet_x = 0;
    inline int32_t p_bullet_y = 0;

    inline bool inv_alive[INV_ROWS][INV_COLS];
    inline int32_t inv_offset_x = 20;
    inline int32_t inv_offset_y = 32;
    inline int32_t inv_dir = 1;
    inline uint32_t inv_move_accum = 0;

    inline bool e_bullet_active[MAX_ENEMY_BULLETS];
    inline int32_t e_bullet_x[MAX_ENEMY_BULLETS];
    inline int32_t e_bullet_y[MAX_ENEMY_BULLETS];
    inline uint32_t e_fire_accum = 0;

    inline int32_t score = 0;
    inline int32_t lives = 3;
    inline int32_t wave = 1;

    inline int32_t clamp_i32(int32_t v, int32_t lo, int32_t hi)
    {
        if (v < lo)
            return lo;
        if (v > hi)
            return hi;
        return v;
    }

    inline bool rects_overlap(int32_t ax, int32_t ay, int32_t aw, int32_t ah,
                              int32_t bx, int32_t by, int32_t bw, int32_t bh)
    {
        return (ax < bx + bw) && (ax + aw > bx) &&
               (ay < by + bh) && (ay + ah > by);
    }

    inline uint32_t next_rand()
    {
        rng_state ^= rng_state << 13;
        rng_state ^= rng_state >> 17;
        rng_state ^= rng_state << 5;
        return rng_state;
    }

    inline int32_t alive_count()
    {
        int32_t n = 0;
        for (int32_t r = 0; r < INV_ROWS; ++r)
        {
            for (int32_t c = 0; c < INV_COLS; ++c)
            {
                if (inv_alive[r][c])
                    ++n;
            }
        }
        return n;
    }

    inline void spawn_wave(int32_t cw)
    {
        for (int32_t r = 0; r < INV_ROWS; ++r)
        {
            for (int32_t c = 0; c < INV_COLS; ++c)
            {
                inv_alive[r][c] = true;
            }
        }
        int32_t total_w = INV_COLS * INV_W + (INV_COLS - 1) * INV_GAP_X;
        inv_offset_x = (cw > total_w + 20) ? ((cw - total_w) / 2) : 10;
        inv_offset_y = 32 + ((wave - 1) * 8);
        if (inv_offset_y > 72)
            inv_offset_y = 72;
        inv_dir = 1;
        inv_move_accum = 0;
        p_bullet_active = false;
        for (int32_t i = 0; i < MAX_ENEMY_BULLETS; ++i)
        {
            e_bullet_active[i] = false;
        }
    }

    inline void reset_game(int32_t cw, int32_t ch)
    {
        score = 0;
        lives = 3;
        wave = 1;
        manual_paused = false;
        game_over = false;
        p_bullet_active = false;
        e_fire_accum = 0;
        player_x = (cw - PLAYER_W) / 2;
        if (player_x < 4)
            player_x = 4;
        spawn_wave(cw);
        (void)ch;
        initialized = true;
    }

    inline void fire_player_bullet(int32_t ch)
    {
        if (p_bullet_active || manual_paused || game_over)
            return;
        int32_t py = ch - PLAYER_H - 12;
        p_bullet_active = true;
        p_bullet_x = player_x + PLAYER_W / 2 - 1;
        p_bullet_y = py - 6;
    }

    inline void move_player(int32_t dx, int32_t cw)
    {
        if (manual_paused || game_over)
            return;
        int32_t max_x = cw - PLAYER_W - 4;
        if (max_x < 4)
            max_x = 4;
        player_x = clamp_i32(player_x + dx * SPEED_SCALE, 4, max_x);
    }

    inline void on_key(char c, int32_t cw, int32_t ch)
    {
        if (!initialized)
        {
            reset_game(cw, ch);
        }

        if (c == 27)
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

        if (c == 'a' || c == 'A')
            move_player(-12, cw);
        else if (c == 'd' || c == 'D')
            move_player(12, cw);
        else if (c == ' ' || c == 'w' || c == 'W')
            fire_player_bullet(ch);
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

        if (scancode == 0x4B)
            move_player(-12, cw);
        else if (scancode == 0x4D)
            move_player(12, cw);
        else if (scancode == 0x48)
            fire_player_bullet(ch);
    }

    inline void spawn_enemy_bullet()
    {
        int slot = -1;
        for (int32_t i = 0; i < MAX_ENEMY_BULLETS; ++i)
        {
            if (!e_bullet_active[i])
            {
                slot = i;
                break;
            }
        }
        if (slot < 0)
            return;

        rng_state ^= static_cast<uint32_t>(Timer::ticks()) * 1103515245u;
        int32_t start_col = static_cast<int32_t>(next_rand() % INV_COLS);
        for (int32_t offset = 0; offset < INV_COLS; ++offset)
        {
            int32_t c = (start_col + offset) % INV_COLS;
            for (int32_t r = INV_ROWS - 1; r >= 0; --r)
            {
                if (inv_alive[r][c])
                {
                    int32_t ix = inv_offset_x + c * (INV_W + INV_GAP_X);
                    int32_t iy = inv_offset_y + r * (INV_H + INV_GAP_Y);
                    e_bullet_active[slot] = true;
                    e_bullet_x[slot] = ix + INV_W / 2 - 1;
                    e_bullet_y[slot] = iy + INV_H + 2;
                    return;
                }
            }
        }
    }

    inline void update(int32_t cw, int32_t ch, bool focused)
    {
        if (cw < 180 || ch < 160)
            return;

        if (!initialized)
        {
            reset_game(cw, ch);
            last_tick = Timer::ticks();
            return;
        }

        uint64_t now = Timer::ticks();
        if (now - last_tick < 2u)
        {
            return;
        }
        last_tick = now;

        if (manual_paused || game_over || !focused)
        {
            return;
        }

        int32_t py = ch - PLAYER_H - 12;
        int32_t max_px = cw - PLAYER_W - 4;
        if (max_px < 4)
            max_px = 4;
        player_x = clamp_i32(player_x, 4, max_px);

        if (p_bullet_active)
        {
            p_bullet_y -= 9 * SPEED_SCALE;
            if (p_bullet_y < 22)
            {
                p_bullet_active = false;
            }
            else
            {
                bool hit = false;
                for (int32_t r = 0; r < INV_ROWS && !hit; ++r)
                {
                    for (int32_t c = 0; c < INV_COLS; ++c)
                    {
                        if (!inv_alive[r][c])
                            continue;
                        int32_t ix = inv_offset_x + c * (INV_W + INV_GAP_X);
                        int32_t iy = inv_offset_y + r * (INV_H + INV_GAP_Y);
                        if (rects_overlap(p_bullet_x, p_bullet_y, 3, 7, ix, iy, INV_W, INV_H))
                        {
                            inv_alive[r][c] = false;
                            p_bullet_active = false;
                            score += (INV_ROWS - r) * 10 * wave;
                            hit = true;
                            break;
                        }
                    }
                }
            }
        }

        if (alive_count() == 0)
        {
            ++wave;
            score += 100 * wave;
            spawn_wave(cw);
            return;
        }

        int32_t rem = alive_count();
        uint32_t step_delay = 7u;
        if (rem <= 4)
            step_delay = 2u;
        else if (rem <= 10)
            step_delay = 4u;
        else if (rem <= 16)
            step_delay = 5u;

        inv_move_accum += 1u;
        if (inv_move_accum >= step_delay)
        {
            inv_move_accum = 0;

            int32_t min_x = cw;
            int32_t max_x = 0;
            int32_t lowest_y = 0;
            for (int32_t r = 0; r < INV_ROWS; ++r)
            {
                for (int32_t c = 0; c < INV_COLS; ++c)
                {
                    if (!inv_alive[r][c])
                        continue;
                    int32_t ix = inv_offset_x + c * (INV_W + INV_GAP_X);
                    int32_t iy = inv_offset_y + r * (INV_H + INV_GAP_Y);
                    if (ix < min_x)
                        min_x = ix;
                    if (ix + INV_W > max_x)
                        max_x = ix + INV_W;
                    if (iy + INV_H > lowest_y)
                        lowest_y = iy + INV_H;
                }
            }

            int32_t dx = inv_dir * (6 + wave) * SPEED_SCALE;
            if ((inv_dir > 0 && max_x + dx >= cw - 6) ||
                (inv_dir < 0 && min_x + dx <= 6))
            {
                inv_dir = -inv_dir;
                inv_offset_y += 10 * SPEED_SCALE;
            }
            else
            {
                inv_offset_x += dx;
            }

            if (lowest_y >= py - 2)
            {
                game_over = true;
                return;
            }
        }

        e_fire_accum += 1u;
        uint32_t fire_interval = (wave > 4) ? 12u : (22u - static_cast<uint32_t>(wave - 1) * 3u);
        if (fire_interval < 8u)
            fire_interval = 8u;
        if (e_fire_accum >= fire_interval)
        {
            e_fire_accum = 0;
            spawn_enemy_bullet();
        }

        for (int32_t i = 0; i < MAX_ENEMY_BULLETS; ++i)
        {
            if (!e_bullet_active[i])
                continue;
            e_bullet_y[i] += (5 + (wave / 2)) * SPEED_SCALE;
            if (e_bullet_y[i] >= ch - 4)
            {
                e_bullet_active[i] = false;
                continue;
            }
            if (rects_overlap(e_bullet_x[i], e_bullet_y[i], 3, 7, player_x, py, PLAYER_W, PLAYER_H))
            {
                e_bullet_active[i] = false;
                --lives;
                player_x = (cw - PLAYER_W) / 2;
                if (lives <= 0)
                {
                    lives = 0;
                    game_over = true;
                    return;
                }
            }
        }
    }

    inline void draw_hud(int32_t x, int32_t y, int32_t sc, int32_t lv, int32_t wv, uint32_t col)
    {
        char buf[48];
        int idx = 0;
        const char *p1 = "PTS:";
        for (int i = 0; p1[i] != '\0'; ++i)
            buf[idx++] = p1[i];

        if (sc < 0)
            sc = 0;
        char tmp[12];
        int t = 0;
        if (sc == 0)
            tmp[t++] = '0';
        else
        {
            while (sc > 0 && t < 11)
            {
                tmp[t++] = static_cast<char>('0' + (sc % 10));
                sc /= 10;
            }
        }
        while (t > 0 && idx < 44)
            buf[idx++] = tmp[--t];

        const char *p2 = "  HP:";
        for (int i = 0; p2[i] != '\0' && idx < 44; ++i)
            buf[idx++] = p2[i];
        buf[idx++] = static_cast<char>('0' + clamp_i32(lv, 0, 9));

        const char *p3 = "  W:";
        for (int i = 0; p3[i] != '\0' && idx < 44; ++i)
            buf[idx++] = p3[i];
        buf[idx++] = static_cast<char>('0' + clamp_i32(wv, 0, 9));

        buf[idx] = '\0';
        Font8x16::draw_text(x, y, buf, col);
    }

    inline void draw_content(int32_t cx, int32_t cy, int32_t cw, int32_t ch, bool focused)
    {
        const uint32_t bg_col = Framebuffer::rgb(10, 12, 18);
        const uint32_t player_col = Framebuffer::rgb(138, 237, 128);
        const uint32_t inv_top_col = Framebuffer::rgb(255, 107, 0);
        const uint32_t inv_mid_col = Framebuffer::rgb(185, 120, 255);
        const uint32_t inv_bot_col = Framebuffer::rgb(72, 210, 240);
        const uint32_t bullet_p_col = Framebuffer::rgb(245, 247, 250);
        const uint32_t bullet_e_col = Framebuffer::rgb(240, 85, 95);
        const uint32_t text_col = Framebuffer::rgb(245, 247, 250);
        const uint32_t dim_col = Framebuffer::rgb(140, 148, 162);
        const uint32_t box_bg = Framebuffer::rgb(14, 16, 22);

        if (cw < 180 || ch < 160)
        {
            Font8x16::draw_text(cx + 8, cy + 8, "Allarga finestra", dim_col);
            return;
        }

        update(cw, ch, focused);

        Framebuffer::fill_rect(cx, cy, cw, ch, bg_col);
        draw_hud(cx + 8, cy + 6, score, lives, wave, text_col);

        for (int32_t r = 0; r < INV_ROWS; ++r)
        {
            uint32_t icol = (r == 0) ? inv_top_col : ((r == 1) ? inv_mid_col : inv_bot_col);
            for (int32_t c = 0; c < INV_COLS; ++c)
            {
                if (!inv_alive[r][c])
                    continue;
                int32_t ix = cx + inv_offset_x + c * (INV_W + INV_GAP_X);
                int32_t iy = cy + inv_offset_y + r * (INV_H + INV_GAP_Y);
                Framebuffer::fill_rect(ix, iy, INV_W, INV_H, icol);
                Framebuffer::fill_rect(ix + 3, iy + 3, 3, 3, bg_col);
                Framebuffer::fill_rect(ix + INV_W - 6, iy + 3, 3, 3, bg_col);
            }
        }

        int32_t py = cy + ch - PLAYER_H - 12;
        int32_t px = cx + player_x;
        Framebuffer::fill_rect(px + PLAYER_W / 2 - 2, py - 4, 4, 4, player_col);
        Framebuffer::fill_rect(px, py, PLAYER_W, PLAYER_H, player_col);

        if (p_bullet_active)
        {
            Framebuffer::fill_rect(cx + p_bullet_x, cy + p_bullet_y, 3, 7, bullet_p_col);
        }

        for (int32_t i = 0; i < MAX_ENEMY_BULLETS; ++i)
        {
            if (e_bullet_active[i])
            {
                Framebuffer::fill_rect(cx + e_bullet_x[i], cy + e_bullet_y[i], 3, 7, bullet_e_col);
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
            int32_t bx = cx + (cw - box_w) / 2;
            int32_t by = cy + (ch - box_h) / 2;

            Framebuffer::fill_rect(bx, by, box_w, box_h, box_bg);
            Framebuffer::fill_rect(bx, by, box_w, 2, player_col);
            Framebuffer::fill_rect(bx, by + box_h - 2, box_w, 2, player_col);
            Framebuffer::fill_rect(bx, by, 2, box_h, player_col);
            Framebuffer::fill_rect(bx + box_w - 2, by, 2, box_h, player_col);

            Font8x16::draw_text(cx + (cw - tw) / 2, by + 10, title, game_over ? bullet_e_col : player_col);
            Font8x16::draw_text(cx + (cw - hw) / 2, by + 30, hint, dim_col);
        }
    }
}