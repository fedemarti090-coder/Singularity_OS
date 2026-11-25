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