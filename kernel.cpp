#include <stdint.h>
#include <stddef.h>

static volatile uint16_t* const VGA_MEMORY = reinterpret_cast<volatile uint16_t*>(0xB8000);
static constexpr size_t VGA_WIDTH = 80;
static constexpr size_t VGA_HEIGHT = 25;

enum class VgaColor : uint8_t {
    Black = 0,
    Green = 2,
    LightGreen = 10,
    White = 15
};

static inline uint8_t make_color(VgaColor fg, VgaColor bg) {
    return static_cast<uint8_t>(fg) | (static_cast<uint8_t>(bg) << 4);
}

static inline uint16_t make_vga_entry(char c, uint8_t color) {
    return static_cast<uint16_t>(c) | (static_cast<uint16_t>(color) << 8);
}

static void clear_screen(uint8_t color) {
    uint16_t blank = make_vga_entry(' ', color);
    for (size_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; ++i) {
        VGA_MEMORY[i] = blank;
    }
}

static void print_at(const char* str, size_t x, size_t y, uint8_t color) {
    size_t index = y * VGA_WIDTH + x;
    for (size_t i = 0; str[i] != '\0'; ++i) {
        VGA_MEMORY[index + i] = make_vga_entry(str[i], color);
    }
}

extern "C" void kernel_main() {
    uint8_t default_color = make_color(VgaColor::White, VgaColor::Black);
    uint8_t accent_color = make_color(VgaColor::LightGreen, VgaColor::Black);

    clear_screen(default_color);
    print_at("[OK] Kernel avviato in Protected Mode (32-bit)", 2, 2, accent_color);
    print_at("Benvenuto nel tuo sistema operativo bare-metal!", 2, 4, default_color);
}