#include <stdint.h>
#include "vga.hpp"
#include "gdt.hpp"
#include "idt.hpp"
#include "pic.hpp"
#include "keyboard.hpp"
#include "multiboot.hpp"
#include "pmm.hpp"
#include "memory.hpp"
#include "heap.hpp"
#include "serial.hpp"
#include "timer.hpp"
#include "shell.hpp"

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

    Serial::init();
    Serial::print_line("[BOOT] Singularity serial COM1 initialized", Serial::COM1);

    Keyboard::init();
    term.print("PS/2 Keyboard ready.", "yellow");

    if (magic == MULTIBOOT_BOOTLOADER_MAGIC) {
        Pmm::init(mb_info, reinterpret_cast<uint32_t>(kernel_start), reinterpret_cast<uint32_t>(kernel_end));
        Heap::init(16);

        char* buf = static_cast<char*>(Heap::alloc(32));
        const char* msg = "Singularity Heap OK";
        if (buf != nullptr) {
            memcpy(buf, msg, strlen(msg) + 1);
            if (strcmp(buf, msg) == 0) {
                term.print(buf, "lightgreen");
            }
            Heap::free(buf);
        }

        Serial::print("[PMM] Free frames: ", Serial::COM1);
    Serial::print_dec(Pmm::free_frames(), Serial::COM1);
    Serial::print_line("", Serial::COM1);
    } else {
        term.print("Multiboot magic mismatch!", "lightred");
    }

    Timer::init(100);
    Shell::init();

    asm volatile("sti");

    while (true) {
        asm volatile("hlt");
    }
}