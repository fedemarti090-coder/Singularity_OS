#include "gdt.hpp"
#include "tss.hpp"

extern "C" void gdt_flush(uint32_t gdtr_addr);

Gdt::Gdt() : ptr{0, 0} {
    for (size_t i = 0; i < ENTRY_COUNT; ++i) {
        entries[i] = {0, 0, 0, 0, 0, 0};
    }
}

void Gdt::set_gate(size_t index, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    entries[index].base_low    = static_cast<uint16_t>(base & 0xFFFF);
    entries[index].base_mid    = static_cast<uint8_t>((base >> 16) & 0xFF);
    entries[index].base_high   = static_cast<uint8_t>((base >> 24) & 0xFF);

    entries[index].limit_low   = static_cast<uint16_t>(limit & 0xFFFF);
    entries[index].granularity = static_cast<uint8_t>(((limit >> 16) & 0x0F) | (gran & 0xF0));
    entries[index].access      = access;

    
}

void Gdt::init() {
    set_gate(0, 0, 0, 0, 0);
    set_gate(1, 0, 0xFFFFF, 0x9A, 0xCF);
    set_gate(2, 0, 0xFFFFF, 0x92, 0xCF);

    set_gate(3, 0, 0xFFFFFFFFu, 0xFA, 0xCF); // User Code (Ring 3, 0x18 -> RPL 3 = 0x1B)
    set_gate(4, 0, 0xFFFFFFFFu, 0xF2, 0xCF); // User Data (Ring 3, 0x20 -> RPL 3 = 0x23)
    set_gate(5, Tss::base_address(), Tss::limit(), 0x89, 0x00);

    ptr.limit = static_cast<uint16_t>(sizeof(GdtEntry) * ENTRY_COUNT - 1);
    ptr.base  = reinterpret_cast<uint32_t>(&entries[0]);

    gdt_flush(reinterpret_cast<uint32_t>(&ptr));
}