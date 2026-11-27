#include "mouse.hpp"
#include "idt.hpp"
#include "pic.hpp"
#include "io.hpp"
#include "serial.hpp"

namespace {
    constexpr uint16_t PS2_DATA_PORT   = 0x60;
    constexpr uint16_t PS2_STATUS_PORT = 0x64;
    constexpr uint16_t PS2_CMD_PORT    = 0x64;

    constexpr uint8_t  STATUS_OUT_FULL = 0x01;
    constexpr uint8_t  STATUS_IN_FULL  = 0x02;

    Mouse::State g_state;
    uint8_t g_packet[3] = {0, 0, 0};
    uint8_t g_pkt_idx = 0;

    bool wait_write() {
        for (uint32_t i = 0; i < 100000; ++i) {
            if ((inb(PS2_STATUS_PORT) & STATUS_IN_FULL) == 0) {
                return true;
            }
        }
        return false;
    }

    bool wait_read() {
        for (uint32_t i = 0; i < 100000; ++i) {
            if ((inb(PS2_STATUS_PORT) & STATUS_OUT_FULL) != 0) {
                return true;
            }
        }
        return false;
    }

    void flush_buffer() {
        for (uint32_t i = 0; i < 64; ++i) {
            if ((inb(PS2_STATUS_PORT) & STATUS_OUT_FULL) == 0) {
                break;
            }
            (void)inb(PS2_DATA_PORT);
        }
    }

    bool write_mouse(uint8_t cmd) {
        if (!wait_write()) {
            return false;
        }
        outb(PS2_CMD_PORT, 0xD4);
        if (!wait_write()) {
            return false;
        }
        outb(PS2_DATA_PORT, cmd);
        if (!wait_read()) {
            return false;
        }
        uint8_t ack = inb(PS2_DATA_PORT);
        return ack == 0xFA;
    }

    void handle_packet() {
        uint8_t flags = g_packet[0];
        if ((flags & 0x08) == 0) {
            g_pkt_idx = 0;
            return;
        }

        int32_t dx = static_cast<int8_t>(g_packet[1]);
        int32_t dy = static_cast<int8_t>(g_packet[2]);

        if ((flags & 0x40) != 0 || (flags & 0x80) != 0) {
            dx = 0;
            dy = 0;
        }

        g_state.left_button   = (flags & 0x01) != 0;
        g_state.right_button  = (flags & 0x02) != 0;
        g_state.middle_button = (flags & 0x04) != 0;

        g_state.x += dx;
        g_state.y -= dy;

        if (g_state.x < 0) g_state.x = 0;
        if (g_state.y < 0) g_state.y = 0;
        if (g_state.x > 1599) g_state.x = 1599;
        if (g_state.y > 899) g_state.y = 899;

        g_state.dirty = true;
    }

    void mouse_irq_callback(Registers*) {
        uint8_t b = inb(PS2_DATA_PORT);
        if (g_pkt_idx == 0 && (b & 0x08) == 0) {
            return;
        }
        g_packet[g_pkt_idx++] = b;
        if (g_pkt_idx == 3) {
            g_pkt_idx = 0;
            handle_packet();
        }
    }
}

namespace Mouse {

void init() {
    flush_buffer();

    if (!wait_write()) {
        Serial::print_line("[MOUSE] Timeout enable aux", Serial::COM1);
        return;
    }
    outb(PS2_CMD_PORT, 0xA8);

    if (!wait_write()) {
        return;
    }
    outb(PS2_CMD_PORT, 0x20);
    if (!wait_read()) {
        return;
    }
    uint8_t cmd_byte = inb(PS2_DATA_PORT);
    cmd_byte |= 0x02;
    cmd_byte &= ~0x20;

    if (!wait_write()) {
        return;
    }
    outb(PS2_CMD_PORT, 0x60);
    if (!wait_write()) {
        return;
    }
    outb(PS2_DATA_PORT, cmd_byte);

    if (!write_mouse(0xF6)) {
        Serial::print_line("[MOUSE] NACK su 0xF6", Serial::COM1);
    }
    if (!write_mouse(0xF4)) {
        Serial::print_line("[MOUSE] NACK su 0xF4 (enable reporting)", Serial::COM1);
        return;
    }

    flush_buffer();

    register_irq_handler(12, mouse_irq_callback);
    Pic::clear_mask(2);
    Pic::clear_mask(12);

    Serial::print_line("[MOUSE] PS/2 inizializzato su IRQ 12", Serial::COM1);
}

const State& state() {
    return g_state;
}

void clear_dirty() {
    g_state.dirty = false;
}

}