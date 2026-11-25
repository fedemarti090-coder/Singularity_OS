#include "vga.hpp"
#include "io.hpp"

VgaTerminal* g_term = nullptr;

VgaTerminal::VgaTerminal()
    : buffer(reinterpret_cast<volatile uint16_t*>(0xB8000)),
      row(0),
      col(0),
      color(vga_entry_color(VgaColor::LightGrey, VgaColor::Black)) {
    clear();
    enable_cursor();
}

void VgaTerminal::set_color(VgaColor fg, VgaColor bg) {
    color = vga_entry_color(fg, bg);
}

void VgaTerminal::clear() {
    uint16_t blank = vga_entry(' ', color);
    for (size_t y = 0; y < HEIGHT; ++y) {
        for (size_t x = 0; x < WIDTH; ++x) {
            buffer[y * WIDTH + x] = blank;
        }
    }
    row = 0;
    col = 0;

    update_cursor();
}

void VgaTerminal::scroll() {
    uint16_t blank = vga_entry(' ', color);

    for (size_t y = 1; y < HEIGHT; ++y) {
        for (size_t x = 0; x < WIDTH; ++x) {
            buffer[(y - 1) * WIDTH + x] = buffer[y * WIDTH + x];
        }
    }

    for (size_t x = 0; x < WIDTH; ++x) {
        buffer[(HEIGHT - 1) * WIDTH + x] = blank;
    }

    row = HEIGHT - 1;
}

void VgaTerminal::newline() {
    col = 0;
    ++row;
    if (row >= HEIGHT) {
        scroll();
    }
}

void VgaTerminal::put_char(char c) {
    if (c == '\n') {
        newline();
        return;
    }
    if (c == '\r') {
        col = 0;
        return;
    }
    if (c == '\t') {
        size_t next_tab = (col + 4) & ~static_cast<size_t>(3);
        while (col < next_tab) {
            put_char(' ');
        }
        return;
    }
    if (c == '\b') {
        if (col > 0) {
            --col;
            buffer[row * WIDTH + col] = vga_entry(' ', color);
        } else if (row > 0) {
            --row;
            col = WIDTH - 1;
            buffer[row * WIDTH + col] = vga_entry(' ', color);
        }
        return;
    }

    buffer[row * WIDTH + col] = vga_entry(static_cast<unsigned char>(c), color);
    ++col;
    if (col >= WIDTH) {
        newline();
    }
    update_cursor();
}

void VgaTerminal::write(const char* str) {
    if (!str) return;
    for (size_t i = 0; str[i] != '\0'; ++i) {
        put_char(str[i]);
    }
}

void VgaTerminal::write_line(const char* str) {
    write(str);
    newline();
}

void VgaTerminal::write_hex(uint32_t value, bool prefix) {
    static const char hex_chars[] = "0123456789ABCDEF";
    if (prefix) {
        write("0x");
    }
    for (int shift = 28; shift >= 0; shift -= 4) {
        put_char(hex_chars[(value >> shift) & 0xF]);
    }
}

void VgaTerminal::write_dec(int32_t value) {
    if (value < 0) {
        put_char('-');
        uint32_t abs_val = static_cast<uint32_t>(-(value + 1)) + 1;
        write_dec(static_cast<int32_t>(abs_val));
        return;
    }

    char buf[12];
    int idx = 0;
    uint32_t uval = static_cast<uint32_t>(value);

    do {
        buf[idx++] = static_cast<char>('0' + (uval % 10));
        uval /= 10;
    } while (uval > 0);

    while (idx > 0) {
        put_char(buf[--idx]);
    }
}

namespace {

bool str_eq(const char* a, const char* b) {
    if (!a || !b) return false;
    size_t i = 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) return false;
        ++i;
    }
    return a[i] == '\0' && b[i] == '\0';
}

VgaColor parse_vga_color(const char* name) {
    if (!name) return VgaColor::LightGrey;

    if (str_eq(name, "black"))        return VgaColor::Black;
    if (str_eq(name, "blue"))         return VgaColor::Blue;
    if (str_eq(name, "green"))        return VgaColor::Green;
    if (str_eq(name, "cyan"))         return VgaColor::Cyan;
    if (str_eq(name, "red"))          return VgaColor::Red;
    if (str_eq(name, "magenta"))      return VgaColor::Magenta;
    if (str_eq(name, "brown"))        return VgaColor::Brown;
    if (str_eq(name, "lightgrey"))    return VgaColor::LightGrey;
    if (str_eq(name, "darkgrey"))     return VgaColor::DarkGrey;
    if (str_eq(name, "lightblue"))    return VgaColor::LightBlue;
    if (str_eq(name, "lightgreen"))   return VgaColor::LightGreen;
    if (str_eq(name, "lightcyan"))    return VgaColor::LightCyan;
    if (str_eq(name, "lightred"))     return VgaColor::LightRed;
    if (str_eq(name, "lightmagenta")) return VgaColor::LightMagenta;
    if (str_eq(name, "yellow") || str_eq(name, "lightbrown")) return VgaColor::LightBrown;
    if (str_eq(name, "white"))        return VgaColor::White;

    return VgaColor::LightGrey;
}

} // namespace

void VgaTerminal::print(const char* str, const char* color_str, bool n) {
    uint8_t prev_color = color;
    VgaColor fg = parse_vga_color(color_str);

    set_color(fg, VgaColor::Black);
    write(str);
    if (n) {
        newline();
    }

    color = prev_color;
}

void VgaTerminal::enable_cursor(uint8_t cursor_start, uint8_t cursor_end) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, (inb(0x3D5) & 0xC0) | cursor_start);

    outb(0x3D4, 0x0B);
    outb(0x3D5, (inb(0x3D5) & 0xE0) | cursor_end);

    update_cursor();
}

void VgaTerminal::disable_cursor() {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}

void VgaTerminal::update_cursor() {
    uint16_t pos = static_cast<uint16_t>(row * WIDTH + col);

    outb(0x3D4, 0x0F);
    outb(0x3D5, static_cast<uint8_t>(pos & 0xFF));

    outb(0x3D4, 0x0E);
    outb(0x3D5, static_cast<uint8_t>((pos >> 8) & 0xFF));
}