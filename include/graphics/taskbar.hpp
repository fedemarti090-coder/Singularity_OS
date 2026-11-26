#pragma once
#include <stdint.h>
#include "framebuffer.hpp"
#include "font.hpp"
#include "icons.hpp"
#include "rtc.hpp"

namespace Taskbar {

inline void draw_u32_2digits(int32_t x, int32_t y, uint32_t value, uint32_t color) {
    char buf[3];
    uint32_t v = value % 100u;
    buf[0] = static_cast<char>('0' + (v / 10u));
    buf[1] = static_cast<char>('0' + (v % 10u));
    buf[2] = '\0';
    Font8x16::draw_text(x, y, buf, color);
}

inline void draw_clock(int32_t bar_y, int32_t fb_w, int8_t tz_offset_hours = 1) {
    RtcTime t = Rtc::read_time(tz_offset_hours);

    const int32_t box_w = 84;
    const int32_t box_h = 28;
    const int32_t box_x = fb_w - box_w - 6;
    const int32_t box_y = bar_y + 4;
    const int32_t txt_y = bar_y + 10;

    const uint32_t box_bg     = Framebuffer::rgb(26, 28, 34);
    const uint32_t box_border = Framebuffer::rgb(52, 56, 68);
    const uint32_t text_col   = Framebuffer::rgb(245, 247, 250);
    const uint32_t sep_col    = Framebuffer::rgb(138, 237, 128);

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

inline void draw() {
    const int32_t bar_h = 36;
    const int32_t bar_y = static_cast<int32_t>(Framebuffer::height()) - bar_h;
    const int32_t fb_w  = static_cast<int32_t>(Framebuffer::width());

    const int32_t btn_y = bar_y + 4;
    const int32_t btn_h = 28;
    const int32_t ico_y = bar_y + 10;
    const int32_t txt_y = bar_y + 10;

    const uint32_t bar_bg     = Framebuffer::rgb(17, 17, 17);
    const uint32_t bar_border = Framebuffer::rgb(45, 48, 56);
    const uint32_t btn_bg     = Framebuffer::rgb(32, 32, 32);
    const uint32_t text_col   = Framebuffer::rgb(245, 247, 250);

    Framebuffer::fill_rect(0, bar_y, fb_w, bar_h, bar_bg);
    Framebuffer::fill_rect(0, bar_y, fb_w, 1, bar_border);

    // 1. Start
    const int32_t start_x = 6;
    const int32_t start_w = 88;
    Framebuffer::fill_rect(start_x, btn_y, start_w, btn_h, btn_bg);
    Icons::draw(IconId::Start, start_x + 6, ico_y);
    Font8x16::draw_text(start_x + 28, txt_y, "Start", text_col);

    // 2. Shell
    const int32_t shell_x = start_x + start_w + 6;
    const int32_t shell_w = 96;
    Framebuffer::fill_rect(shell_x, btn_y, shell_w, btn_h, btn_bg);
    Icons::draw(IconId::Terminal, shell_x + 6, ico_y);
    Font8x16::draw_text(shell_x + 28, txt_y, "Shell", text_col);

    // 3. File
    const int32_t file_x = shell_x + shell_w + 6;
    const int32_t file_w = 88;
    Framebuffer::fill_rect(file_x, btn_y, file_w, btn_h, btn_bg);
    Icons::draw(IconId::Files, file_x + 6, ico_y);
    Font8x16::draw_text(file_x + 28, txt_y, "File", text_col);

    // 4. Orologio RTC a destra
    draw_clock(bar_y, fb_w, 1);
}

}