#include "timer.hpp"
#include "pic.hpp"
#include "io.hpp"

extern "C" {
    struct Registers;
}

void register_irq_handler(uint8_t irq, void (*cb)(Registers*));

static volatile uint32_t g_ticks = 0;

static void timer_irq_callback(Registers*) {
    ++g_ticks;
}

void Timer::init(uint32_t frequency_hz) {
    if (frequency_hz == 0) {
        frequency_hz = 100;
    }

    uint32_t divisor = 1193182u / frequency_hz;
    if (divisor == 0) {
        divisor = 1;
    }
    if (divisor > 0xFFFFu) {
        divisor = 0xFFFFu;
    }

    outb(0x43, 0x36); // Channel 0, lobyte/hibyte, rate generator
    outb(0x40, static_cast<uint8_t>(divisor & 0xFFu));
    outb(0x40, static_cast<uint8_t>((divisor >> 8) & 0xFFu));

    register_irq_handler(0, timer_irq_callback);
    Pic::clear_mask(0);
}

uint32_t Timer::ticks() {
    return g_ticks;
}