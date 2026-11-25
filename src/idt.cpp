#include "idt.hpp"
#include "vga.hpp"

#include "idt.hpp"
#include "pic.hpp"
#include "vga.hpp"

extern "C" void idt_load(uint32_t idtr_addr);
extern "C" void isr0();
extern "C" void isr13();
extern "C" void isr14();

extern "C" void irq0();
extern "C" void irq1();
extern "C" void irq2();
extern "C" void irq3();
extern "C" void irq4();
extern "C" void irq5();
extern "C" void irq6();
extern "C" void irq7();
extern "C" void irq8();
extern "C" void irq9();
extern "C" void irq10();
extern "C" void irq11();
extern "C" void irq12();
extern "C" void irq13();
extern "C" void irq14();
extern "C" void irq15();

struct Registers {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
};

using IrqCallback = void (*)(Registers*);
static IrqCallback irq_callbacks[16] = { nullptr };

void register_irq_handler(uint8_t irq, IrqCallback cb) {
    if (irq < 16) {
        irq_callbacks[irq] = cb;
    }
}

extern "C" void isr_handler(Registers* regs) {
    VgaTerminal term;
    term.print("CPU Exception!", "lightred");
    (void)regs;
    while (true) {
        asm volatile("cli; hlt");
    }
}

extern "C" void irq_handler(Registers* regs) {
    uint8_t irq = static_cast<uint8_t>(regs->int_no - 32);
    if (irq < 16 && irq_callbacks[irq] != nullptr) {
        irq_callbacks[irq](regs);
    }
    Pic::send_eoi(irq);
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

    set_gate(32, reinterpret_cast<uint32_t>(irq0),  0x08, 0x8E);
    set_gate(33, reinterpret_cast<uint32_t>(irq1),  0x08, 0x8E);
    set_gate(34, reinterpret_cast<uint32_t>(irq2),  0x08, 0x8E);
    set_gate(35, reinterpret_cast<uint32_t>(irq3),  0x08, 0x8E);
    set_gate(36, reinterpret_cast<uint32_t>(irq4),  0x08, 0x8E);
    set_gate(37, reinterpret_cast<uint32_t>(irq5),  0x08, 0x8E);
    set_gate(38, reinterpret_cast<uint32_t>(irq6),  0x08, 0x8E);
    set_gate(39, reinterpret_cast<uint32_t>(irq7),  0x08, 0x8E);
    set_gate(40, reinterpret_cast<uint32_t>(irq8),  0x08, 0x8E);
    set_gate(41, reinterpret_cast<uint32_t>(irq9),  0x08, 0x8E);
    set_gate(42, reinterpret_cast<uint32_t>(irq10), 0x08, 0x8E);
    set_gate(43, reinterpret_cast<uint32_t>(irq11), 0x08, 0x8E);
    set_gate(44, reinterpret_cast<uint32_t>(irq12), 0x08, 0x8E);
    set_gate(45, reinterpret_cast<uint32_t>(irq13), 0x08, 0x8E);
    set_gate(46, reinterpret_cast<uint32_t>(irq14), 0x08, 0x8E);
    set_gate(47, reinterpret_cast<uint32_t>(irq15), 0x08, 0x8E);

    ptr.limit = static_cast<uint16_t>(sizeof(IdtEntry) * ENTRY_COUNT - 1);
    ptr.base  = reinterpret_cast<uint32_t>(&entries[0]);

    idt_load(reinterpret_cast<uint32_t>(&ptr));
}