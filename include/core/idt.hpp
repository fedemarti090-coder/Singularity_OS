#pragma once
#include <stdint.h>
#include <stddef.h>

struct __attribute__((packed)) IdtEntry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  zero;
    uint8_t  type_attr;
    uint16_t offset_high;
};

struct __attribute__((packed)) IdtPtr {
    uint16_t limit;
    uint32_t base;
};

struct Registers {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
};

using IrqCallback = void (*)(Registers*);
void register_irq_handler(uint8_t irq, IrqCallback cb);

class Idt {
public:
    Idt();
    void init();
    void set_gate(uint8_t index, uint32_t handler, uint16_t selector, uint8_t type_attr);

private:
    static constexpr size_t ENTRY_COUNT = 256;

    IdtEntry entries[ENTRY_COUNT];
    IdtPtr   ptr;
};