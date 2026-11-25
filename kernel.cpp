#include <stdint.h>
#include "vga.hpp"
#include "gdt.hpp"
#include "idt.hpp"
#include "pic.hpp"
#include "keyboard.hpp"
#include "multiboot.hpp"
#include "pmm.hpp"

extern "C" uint8_t kernel_start[];
extern "C" uint8_t kernel_end[];

extern "C" void kernel_main(uint32_t magic, const MultibootInfo* mb_info) {
    VgaTerminal term;
    g_term = &term;

    term.print("Singularity OS booting...", "lightgreen");

    Gdt gdt;
    gdt.init();
    term.print("GDT OK", "lightcyan");

    Pic::remap();
    Pic::disable_all();

    Idt idt;
    idt.init();
    term.print("PIC + IDT OK", "lightcyan");

    Keyboard::init();
    term.print("PS/2 Keyboard ready.", "yellow");

    if (magic == MULTIBOOT_BOOTLOADER_MAGIC) {
        Pmm::init(mb_info, reinterpret_cast<uint32_t>(kernel_start), reinterpret_cast<uint32_t>(kernel_end));
        term.print("PMM initialized.", "lightcyan");
    } else {
        term.print("Multiboot magic mismatch!", "lightred");
    }

    asm volatile("sti");

    while (true) {
        asm volatile("hlt");
    }
}