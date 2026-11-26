#pragma once
#include <stdint.h>
#include "framebuffer.hpp"
#include "font.hpp"
#include "icons.hpp"
#include "rtc.hpp"
#include "io.hpp"
#include "_window.hpp"

namespace Taskbar
{

    inline constexpr int32_t BAR_H = 36;
    inline constexpr int32_t START_X = 6;
    inline constexpr int32_t START_W = 88;
    inline constexpr int32_t SHELL_X = START_X + START_W + 6;
    inline constexpr int32_t SHELL_W = 96;
    inline constexpr int32_t FILE_X = SHELL_X + SHELL_W + 6;
    inline constexpr int32_t FILE_W = 88;

    inline constexpr int32_t MENU_ITEM_H = 26;
    inline constexpr int32_t MENU_W = 168;
    inline constexpr int32_t MENU_PAD = 4;
    inline constexpr int32_t MENU_ITEMS = 6;

    inline bool prev_left_down = false;
    inline bool start_open = false;

    void init();
    bool is_start_open();
    void close_start();

    inline void port_outw(uint16_t port, uint16_t val)
    {
        asm volatile("outw %0, %1" : : "a"(val), "Nd"(port));
    }

    inline void trigger_reboot()
    {
        for (int i = 0; i < 100000; ++i)
        {
            if ((inb(0x64) & 0x02u) == 0u)
            {
                break;
            }
        }
        outb(0x64, 0xFE);
        while (true)
        {
            asm volatile("cli; hlt");
        }
    }

    inline void trigger_shutdown()
    {
        port_outw(0x604, 0x2000);
        port_outw(0xB004, 0x2000);
        while (true)
        {
            asm volatile("cli; hlt");
        }
    }

    inline void start_menu_rect(int32_t &mx, int32_t &my, int32_t &mw, int32_t &mh, int32_t tb_y)
    {
        mw = MENU_W;
        mh = MENU_PAD * 2 + MENU_ITEMS * MENU_ITEM_H;
        mx = START_X;
        my = tb_y - mh - 2;
    }

    inline bool point_in_rect(int32_t px, int32_t py, int32_t rx, int32_t ry, int32_t rw, int32_t rh)
    {
        return px >= rx && px < (rx + rw) && py >= ry && py < (ry + rh);
    }

    inline void draw_u32_2digits(int32_t x, int32_t y, uint32_t value, uint32_t color)
    {
        char buf[3];
        uint32_t v = value % 100u;
        buf[0] = static_cast<char>('0' + (v / 10u));
        buf[1] = static_cast<char>('0' + (v % 10u));
        buf[2] = '\0';
        Font8x16::draw_text(x, y, buf, color);
    }

    inline void draw_clock(int32_t bar_y, int32_t fb_w, int8_t tz_offset_hours = 1)
    {
        RtcTime t = Rtc::read_time(tz_offset_hours);

        const int32_t box_w = 84;
        const int32_t box_h = 28;
        const int32_t box_x = fb_w - box_w - 6;
        const int32_t box_y = bar_y + 4;
        const int32_t txt_y = bar_y + 10;

        const uint32_t box_bg = Framebuffer::rgb(26, 28, 34);
        const uint32_t box_border = Framebuffer::rgb(52, 56, 68);
        const uint32_t text_col = Framebuffer::rgb(245, 247, 250);
        const uint32_t sep_col = Framebuffer::rgb(138, 237, 128);

        Framebuffer::fill_rect(box_x, box_y, box_w, box_h, box_bg);
        Framebuffer::fill_rect(box_x, box_y, box_w, 1, box_border);
        Framebuffer::fill_rect(box_x, box_y + box_h - 1, box_w, 1, box_border);
        Framebuffer::fill_rect(box_x, box_y, 1, box_h, box_border);
        Framebuffer::fill_rect(box_x + box_w - 1, box_y, 1, box_h, box_border);

        int32_t cx = box_x + 10;
        draw_u32_2digits(cx, txt_y, t.hours, text_col);
        Font8x16::draw_text(cx + 16, txt_y, ":", sep_col);
        draw_u32_2digits(cx + 24, txt_y, t.minutes, text_col);
        Font8x16::draw_text(cx + 40, txt_y, ":", sep_col);
        draw_u32_2digits(cx + 48, txt_y, t.seconds, text_col);
    }

    inline void draw()
    {
        const int32_t bar_y = static_cast<int32_t>(Framebuffer::height()) - BAR_H;
        const int32_t fb_w = static_cast<int32_t>(Framebuffer::width());

        const int32_t btn_y = bar_y + 4;
        const int32_t btn_h = 28;
        const int32_t ico_y = bar_y + 10;
        const int32_t txt_y = bar_y + 10;

        const uint32_t bar_bg = Framebuffer::rgb(17, 17, 17);
        const uint32_t bar_border = Framebuffer::rgb(45, 48, 56);
        const uint32_t btn_bg = Framebuffer::rgb(32, 32, 32);
        const uint32_t active_bg = Framebuffer::rgb(44, 58, 48);
        const uint32_t text_col = Framebuffer::rgb(245, 247, 250);

        Framebuffer::fill_rect(0, bar_y, fb_w, BAR_H, bar_bg);
        Framebuffer::fill_rect(0, bar_y, fb_w, 1, bar_border);

        // 1. Start
        Framebuffer::fill_rect(START_X, btn_y, START_W, btn_h, start_open ? active_bg : btn_bg);
        Icons::draw(IconId::Start, START_X + 6, ico_y);
        Font8x16::draw_text(START_X + 28, txt_y, "Start", text_col);

        // 2. Shell
        Framebuffer::fill_rect(SHELL_X, btn_y, SHELL_W, btn_h, btn_bg);
        Icons::draw(IconId::Terminal, SHELL_X + 6, ico_y);
        Font8x16::draw_text(SHELL_X + 28, txt_y, "Shell", text_col);

        // 3. File
        Framebuffer::fill_rect(FILE_X, btn_y, FILE_W, btn_h, btn_bg);
        Icons::draw(IconId::Files, FILE_X + 6, ico_y);
        Font8x16::draw_text(FILE_X + 28, txt_y, "File", text_col);

        // 4. Orologio RTC a destra
        draw_clock(bar_y, fb_w, 1);

        if (start_open)
        {
            int32_t sx, sy, sw, sh;
            start_menu_rect(sx, sy, sw, sh, bar_y);

            const uint32_t bg_col = Framebuffer::rgb(24, 27, 33);
            const uint32_t border_col = Framebuffer::rgb(138, 237, 128);
            const uint32_t item_bg = Framebuffer::rgb(34, 38, 46);
            const uint32_t sep_col = Framebuffer::rgb(95, 102, 115);

            Framebuffer::fill_rect(sx, sy, sw, sh, bg_col);
            Framebuffer::fill_rect(sx, sy, sw, 1, border_col);
            Framebuffer::fill_rect(sx, sy + sh - 1, sw, 1, border_col);
            Framebuffer::fill_rect(sx, sy, 1, sh, border_col);
            Framebuffer::fill_rect(sx + sw - 1, sy, 1, sh, border_col);

            const char *labels[MENU_ITEMS] = {
                "Spegni",
                "Riavvia",
                "--------",
                "Shell",
                "Files",
                "About"};

            for (int32_t i = 0; i < MENU_ITEMS; ++i)
            {
                int32_t iy = sy + MENU_PAD + i * MENU_ITEM_H;
                if (i == 2)
                {
                    Font8x16::draw_text(sx + 12, iy + (MENU_ITEM_H - 16) / 2, labels[i], sep_col);
                }
                else
                {
                    Framebuffer::fill_rect(sx + 4, iy, sw - 8, MENU_ITEM_H - 2, item_bg);
                    Font8x16::draw_text(sx + 12, iy + (MENU_ITEM_H - 16) / 2, labels[i], text_col);
                }
            }
        }
    }

    inline bool on_mouse(int32_t mx, int32_t my, bool left_down)
    {
        bool pressed_edge = left_down && !prev_left_down;
        prev_left_down = left_down;

        if (!pressed_edge)
        {
            return false;
        }

        const int32_t bar_y = static_cast<int32_t>(Framebuffer::height()) - BAR_H;
        const int32_t btn_y = bar_y + 4;
        const int32_t btn_h = 28;

        if (point_in_rect(mx, my, START_X, btn_y, START_W, btn_h))
        {
            start_open = !start_open;
            return true;
        }

        if (start_open)
        {
            int32_t sx, sy, sw, sh;
            start_menu_rect(sx, sy, sw, sh, bar_y);

            if (point_in_rect(mx, my, sx, sy, sw, sh))
            {
                int32_t idx = (my - (sy + MENU_PAD)) / MENU_ITEM_H;
                if (idx == 2)
                {
                    return true;
                }
                start_open = false;
                if (idx == 0)
                {
                    trigger_shutdown();
                    return true;
                }
                if (idx == 1)
                {
                    trigger_reboot();
                    return true;
                }
                if (idx == 3)
                {
                    WindowManager::open_or_focus(WindowKind::Shell, "Singularity Shell", 120, 90, 420, 260);
                    return true;
                }
                if (idx == 4)
                {
                    WindowManager::open_or_focus(WindowKind::Files, "Singularity File", 150, 110, 420, 260);
                    return true;
                }
                if (idx == 5)
                {
                    WindowManager::open_or_focus(WindowKind::About, "About", 180, 130, 300, 160);
                    return true;
                }
                return true;
            }

            if (!point_in_rect(mx, my, 0, bar_y, static_cast<int32_t>(Framebuffer::width()), BAR_H))
            {
                start_open = false;
                return true;
            }
        }

        if (point_in_rect(mx, my, SHELL_X, btn_y, SHELL_W, btn_h))
        {
            start_open = false;
            WindowManager::open_or_focus(WindowKind::Shell, "Singularity Shell", 120, 90, 420, 260);
            return true;
        }

        if (point_in_rect(mx, my, FILE_X, btn_y, FILE_W, btn_h))
        {
            start_open = false;
            WindowManager::open_or_focus(WindowKind::Files, "Singularity File", 150, 110, 420, 260);
            return true;
        }

        return false;
    }

}