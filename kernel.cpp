#include "include/vga.hpp"

extern "C" void kernel_main() {
    VgaTerminal term;

    term.print("", "");
    term.print("Hello Print LIB", "yellow");

    while (true) {
        asm volatile("hlt");
    }
}