#include "vga.hpp"
#include "gdt.hpp"
#include "idt.hpp"

extern "C" void kernel_main() {
    VgaTerminal term;
    term.print("Singularity OS booting...", "lightgreen");

    Gdt gdt;
    gdt.init();
    term.print("GDT OK", "lightcyan");

    Idt idt;
    idt.init();
    term.print("IDT OK", "lightcyan");

    while (true) {
        asm volatile("cli; hlt");
    }
}