#pragma once
#include <stdint.h>
#include "io.hpp"

class Pic {
public:
    static constexpr uint8_t MASTER_OFFSET = 0x20; // 32
    static constexpr uint8_t SLAVE_OFFSET  = 0x28; // 40

    static void remap(uint8_t offset1 = MASTER_OFFSET, uint8_t offset2 = SLAVE_OFFSET);
    static void send_eoi(uint8_t irq);
    static void set_mask(uint8_t irq_line);
    static void clear_mask(uint8_t irq_line);
    static void disable_all();

private:
    static constexpr uint16_t PIC1_COMMAND = 0x20;
    static constexpr uint16_t PIC1_DATA    = 0x21;
    static constexpr uint16_t PIC2_COMMAND = 0xA0;
    static constexpr uint16_t PIC2_DATA    = 0xA1;

    static constexpr uint8_t PIC_EOI       = 0x20;
    static constexpr uint8_t ICW1_INIT     = 0x10;
    static constexpr uint8_t ICW1_ICW4     = 0x01;
    static constexpr uint8_t ICW4_8086     = 0x01;

    static inline void io_wait() {
        outb(0x80, 0);
    }
};