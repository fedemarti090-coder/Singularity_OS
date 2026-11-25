#include "vga.hpp"
#include "gdt.hpp"

extern "C" void kernel_main() {
    VgaTerminal term;
    term.print("1. Prima di GDT", "yellow");

    Gdt gdt;
    gdt.init();

    term.print("2. Dopo GDT", "lightgreen");

    while (true) {
        asm volatile("cli; hlt");
    }
}