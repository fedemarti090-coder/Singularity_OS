#pragma once
#include <stdint.h>

namespace CalculatorApp
{
    void init();
    bool consume_dirty();

    void on_mouse_click(int32_t mx, int32_t my, int32_t cx, int32_t cy, int32_t cw, int32_t ch);
    void on_key(char c);

    void draw_content(int32_t cx, int32_t cy, int32_t cw, int32_t ch, bool focused);
}