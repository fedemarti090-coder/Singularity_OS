#pragma once
#include <stdint.h>
#include <stddef.h>

struct __attribute__((packed)) GdtEntry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
};

struct __attribute__((packed)) GdtPtr {
    uint16_t limit;
    uint32_t base;
};

class Gdt {
public:
    static constexpr uint16_t NULL_SEG = 0x00;
    static constexpr uint16_t CODE_SEG = 0x08;
    static constexpr uint16_t DATA_SEG = 0x10;

    Gdt();
    void init();

private:
    static constexpr size_t ENTRY_COUNT = 6;

    GdtEntry entries[ENTRY_COUNT];
    GdtPtr ptr;

    void set_gate(size_t index, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran);
};