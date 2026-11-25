#include "tss.hpp"
#include "memory.hpp"
#include "serial.hpp"

TssEntry Tss::entry;

void Tss::flush(uint16_t sel) {
    asm volatile("ltr %0" : : "r"(sel) : "memory");
}

void Tss::init(uint32_t kernel_stack_top) {
    memset(&entry, 0, sizeof(TssEntry));
    entry.ss0 = KERNEL_DATA_SEL;
    entry.esp0 = kernel_stack_top;
    entry.iomap_base = sizeof(TssEntry);

    flush(TSS_SEL);

    Serial::print("[TSS] Initialized at ", Serial::COM1);
    Serial::print_hex32(base_address(), Serial::COM1);
    Serial::print(", ESP0=", Serial::COM1);
    Serial::print_hex32(entry.esp0, Serial::COM1);
    Serial::print_line("", Serial::COM1);
}

void Tss::set_kernel_stack(uint32_t esp0) {
    entry.esp0 = esp0;
}

uint32_t Tss::base_address() {
    return reinterpret_cast<uint32_t>(&entry);
}

uint32_t Tss::limit() {
    return static_cast<uint32_t>(sizeof(TssEntry) - 1u);
}