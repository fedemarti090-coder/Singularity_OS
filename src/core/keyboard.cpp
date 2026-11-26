#include "keyboard.hpp"
#include "pic.hpp"
#include "io.hpp"
#include "vga.hpp"
#include "shell.hpp"

extern "C" {
    struct Registers;
}

void register_irq_handler(uint8_t irq, void (*cb)(Registers*));

static void keyboard_irq_callback(Registers*) {
    Keyboard::on_irq();
}

static const char scancode_set1[128] = {
    0,   27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+', 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0
};

char Keyboard::scancode_to_ascii(uint8_t scancode) {
    if (scancode >= 128) {
        return 0;
    }
    return scancode_set1[scancode];
}

void Keyboard::init() {
    while (inb(STATUS_PORT) & STATUS_OUT_FULL) {
        (void)inb(DATA_PORT);
    }

    register_irq_handler(1, keyboard_irq_callback);
    Pic::clear_mask(1);
}

void Keyboard::on_irq() {
    static bool extended = false;

    uint8_t status = inb(STATUS_PORT);
    if (!(status & STATUS_OUT_FULL)) {
        return;
    }

    uint8_t scancode = inb(DATA_PORT);

    if (scancode == 0xE0) {
        extended = true;
        return;
    }

    if (extended) {
        extended = false;
        return;
    }

    if (scancode & 0x80) {
        return;
    }

    char c = scancode_to_ascii(scancode);
    if (c != 0) {
        Shell::on_char(c);
    }
}