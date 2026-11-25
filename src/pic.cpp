#include "pic.hpp"
#include "io.hpp"

void Pic::remap(uint8_t offset1, uint8_t offset2) {
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);

    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    outb(PIC1_DATA, offset1);
    io_wait();
    outb(PIC2_DATA, offset2);
    io_wait();

    outb(PIC1_DATA, 0x04);
    io_wait();
    outb(PIC2_DATA, 0x02);
    io_wait();

    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

void Pic::send_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }
    outb(PIC1_COMMAND, PIC_EOI);
}

void Pic::set_mask(uint8_t irq_line) {
    uint16_t port = (irq_line < 8) ? PIC1_DATA : PIC2_DATA;
    if (irq_line >= 8) {
        irq_line -= 8;
    }
    uint8_t value = inb(port) | static_cast<uint8_t>(1 << irq_line);
    outb(port, value);
}

void Pic::clear_mask(uint8_t irq_line) {
    uint16_t port = (irq_line < 8) ? PIC1_DATA : PIC2_DATA;
    if (irq_line >= 8) {
        irq_line -= 8;
    }
    uint8_t value = inb(port) & static_cast<uint8_t>(~(1 << irq_line));
    outb(port, value);
}

void Pic::disable_all() {
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}