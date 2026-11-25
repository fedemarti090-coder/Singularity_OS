#include "vga.hpp"
#include "gdt.hpp"
#include "idt.hpp"
#include "pic.hpp"

extern "C" void kernel_main() {
    VgaTerminal term;
    term.print("Singularity OS booting...", "lightgreen");

    Gdt gdt;
    gdt.init();
    term.print("GDT OK", "lightcyan");

    Pic::remap();
    Idt idt;
    idt.init();
    term.print("PIC + IDT OK", "lightcyan");

    asm volatile("sti");

    while (true) {
        asm volatile("hlt");
    }
}