#pragma once
#include <stdint.h>

class Keyboard {
public:
    static void init();
    static void on_irq();

private:
    static constexpr uint16_t DATA_PORT   = 0x60;
    static constexpr uint16_t STATUS_PORT = 0x64;
    static constexpr uint8_t  STATUS_OUT_FULL = 0x01;

    static char scancode_to_ascii(uint8_t scancode);
};