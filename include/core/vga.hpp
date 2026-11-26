#pragma once

#include <stdint.h>
#include <stddef.h>

enum class VgaColor : uint8_t {
    Black         = 0x0,
    Blue          = 0x1,
    Green         = 0x2,
    Cyan          = 0x3,
    Red           = 0x4,
    Magenta       = 0x5,
    Brown         = 0x6,
    LightGrey     = 0x7,
    DarkGrey      = 0x8,
    LightBlue     = 0x9,
    LightGreen    = 0xA,
    LightCyan     = 0xB,
    LightRed      = 0xC,
    LightMagenta  = 0xD,
    LightBrown    = 0xE,
    White         = 0xF,
};

static inline uint8_t vga_entry_color(VgaColor fg, VgaColor bg) {
    return static_cast<uint8_t>(bg) << 4 | static_cast<uint8_t>(fg);
}

static inline uint16_t vga_entry(unsigned char c, uint8_t color) {
    return static_cast<uint16_t>(c) | (static_cast<uint16_t>(color) << 8);
}

class VgaTerminal {
public:
    static constexpr size_t WIDTH = 80;
    static constexpr size_t HEIGHT = 25;
    
    VgaTerminal();

    void clear();
    void set_color(VgaColor fg, VgaColor bg = VgaColor::Black);
    void put_char(char c);
    void write(const char* sts);
    void write_line(const char *std);
    void write_hex(uint32_t value, bool prefix = true);
    void write_dec(int32_t value);
    void scroll();
    void newline();
    void print(const char* str, const char* color_str = "white", bool n= true);

    void enable_cursor(uint8_t cursor_start = 14, uint8_t cursor_end = 15);
    void disable_cursor();
    void update_cursor();
    
private:
    volatile uint16_t* buffer;
    size_t row;
    size_t col;
    uint8_t color;

};

extern VgaTerminal* g_term;