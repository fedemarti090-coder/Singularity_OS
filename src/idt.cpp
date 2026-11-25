#include "idt.hpp"
#include "vga.hpp"

extern "C" void idt_load(uint32_t idtr_addr);
extern "C" void isr0();
extern "C" void isr13();
extern "C" void isr14();

struct Registers {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
};

extern "C" void isr_handler(Registers* regs) {
    VgaTerminal term;
    term.print("CPU Exception! Vector: ", "lightred");
    (void)regs;
    while (true) {
        asm volatile("cli; hlt");
    }
}

Idt::Idt() : ptr{0, 0} {
    for (size_t i = 0; i < ENTRY_COUNT; ++i) {
        entries[i] = {0, 0, 0, 0, 0};
    }
}

void Idt::set_gate(uint8_t index, uint32_t handler, uint16_t selector, uint8_t type_attr) {
    entries[index].offset_low  = static_cast<uint16_t>(handler & 0xFFFF);
    entries[index].selector    = selector;
    entries[index].zero        = 0;
    entries[index].type_attr   = type_attr;
    entries[index].offset_high = static_cast<uint16_t>((handler >> 16) & 0xFFFF);
}

void Idt::init() {
    set_gate(0,  reinterpret_cast<uint32_t>(isr0),  0x08, 0x8E);
    set_gate(13, reinterpret_cast<uint32_t>(isr13), 0x08, 0x8E);
    set_gate(14, reinterpret_cast<uint32_t>(isr14), 0x08, 0x8E);

    ptr.limit = static_cast<uint16_t>(sizeof(IdtEntry) * ENTRY_COUNT - 1);
    ptr.base  = reinterpret_cast<uint32_t>(&entries[0]);

    idt_load(reinterpret_cast<uint32_t>(&ptr));
}