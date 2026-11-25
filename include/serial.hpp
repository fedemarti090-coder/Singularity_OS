#pragma once
#include <stdint.h>

class Serial {
public:
    static constexpr uint16_t COM1 = 0x3F8;

    static void init(uint16_t port = COM1);
    static void put_char(char c, uint16_t port = COM1);
    static void print(const char* str, uint16_t port = COM1);
    static void print_line(const char* str, uint16_t port = COM1);
    static void print_hex32(uint32_t value, uint16_t port = COM1);
    static void print_dec(uint32_t value, uint16_t port = COM1);

private:
    static bool is_transmit_empty(uint16_t port);
};