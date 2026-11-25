#include "serial.hpp"
#include "io.hpp"

bool Serial::is_transmit_empty(uint16_t port) {
    return (inb(port + 5) & 0x20) != 0;
}

void Serial::init(uint16_t port) {
    outb(port + 1, 0x00); // Interrupt seriali OFF
    outb(port + 3, 0x80); // DLAB per impostare il baud rate divisor ON
    outb(port + 0, 0x01); // Divisor low byte: 115200 baud (115200 / 1)
    outb(port + 1, 0x00); // Divisor high byte
    outb(port + 3, 0x03); // 8 bit, no parity, 1 stop bit (8N1), DLAB = 0
    outb(port + 2, 0xC7); // Abilita FIFO, svuota code, soglia 14 byte
    outb(port + 4, 0x0B); // IRQs abilitate, RTS/DSR attivi
}

void Serial::put_char(char c, uint16_t port) {
    if (c == '\n') {
        put_char('\r', port);
    }
    while (!is_transmit_empty(port)) {
        asm volatile("pause");
    }
    outb(port, static_cast<uint8_t>(c));
}

void Serial::print(const char* str, uint16_t port) {
    if (!str) {
        return;
    }
    while (*str != '\0') {
        put_char(*str++, port);
    }
}

void Serial::print_line(const char* str, uint16_t port) {
    print(str, port);
    put_char('\n', port);
}

void Serial::print_hex32(uint32_t value, uint16_t port) {
    static const char hex_chars[] = "0123456789ABCDEF";
    print("0x", port);
    for (int i = 28; i >= 0; i -= 4) {
        put_char(hex_chars[(value >> i) & 0xF], port);
    }
}

void Serial::print_dec(uint32_t value, uint16_t port) {
    if (value == 0) {
        put_char('0', port);
        return;
    }

    char buf[11];
    int idx = 0;
    while (value > 0 && idx < 10) {
        buf[idx++] = static_cast<char>('0' + (value % 10));
        value /= 10;
    }

    while (idx > 0) {
        put_char(buf[--idx], port);
    }
}