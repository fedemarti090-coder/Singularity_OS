#include <stdint.h>
#include "initrd.hpp"
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
#include "paging.hpp"
#include "tss.hpp"
#include "task.hpp"

extern "C" uint8_t kernel_start[];
extern "C" uint8_t kernel_end[];

// ES di 2 task

static void demo_task_a(void*) {
    Serial::print_line("[TASK A] entered", Serial::COM1);
    for (int i = 0; i < 3; ++i) {
        Serial::print_line("[TASK A] running", Serial::COM1);
        Scheduler::yield();
    }
}

static void demo_task_b(void*) {
    for (int i = 0; i < 3; ++i) {
        Serial::print_line("[TASK B] running", Serial::COM1);
        Scheduler::yield();
    }
}

extern "C" void kernel_main(uint32_t magic, const MultibootInfo* mb_info) {
    VgaTerminal term;
    g_term = &term;

    term.print("Singularity OS booting...", "lightgreen");

    Gdt gdt;
    gdt.init();
    term.print("GDT OK", "lightcyan");

    uint32_t current_esp = 0;
    asm volatile("mov %%esp, %0" : "=r"(current_esp));
    Tss::init(current_esp);

    Pic::remap();
    Pic::disable_all();

    Idt idt;
    idt.init();
    term.print("PIC + IDT OK", "lightcyan");

    Serial::init();
    Serial::print_line("[BOOT] Singularity serial COM1 initialized", Serial::COM1);

    if (mb_info->flags & MULTIBOOT_FLAG_FRAMEBUFFER) {
        Serial::print("[FB] Addr: ", Serial::COM1);
        Serial::print_hex32(static_cast<uint32_t>(mb_info->framebuffer_addr), Serial::COM1);
        Serial::print(" | ", Serial::COM1);
        Serial::print_dec(mb_info->framebuffer_width, Serial::COM1);
        Serial::print("x", Serial::COM1);
        Serial::print_dec(mb_info->framebuffer_height, Serial::COM1);
        Serial::print("x", Serial::COM1);
        Serial::print_dec(mb_info->framebuffer_bpp, Serial::COM1);
        Serial::print(" | Pitch: ", Serial::COM1);
        Serial::print_dec(mb_info->framebuffer_pitch, Serial::COM1);
        Serial::print_line("", Serial::COM1);
    } else {
        Serial::print_line("[FB] Flag framebuffer NON presente!", Serial::COM1);
    }

    Keyboard::init();
    term.print("PS/2 Keyboard ready.", "yellow");

    

    if (magic == MULTIBOOT_BOOTLOADER_MAGIC) {
        Pmm::init(mb_info, reinterpret_cast<uint32_t>(kernel_start), reinterpret_cast<uint32_t>(kernel_end));
        Heap::init(16);
        Paging::init(32u * 1024u * 1024u);

        if (InitRd::init(mb_info)) {
            for (size_t i = 0; i < InitRd::count(); ++i) {
                const InitRdEntry* e = InitRd::get_at(i);
                if (e != nullptr && !e->is_dir) {
                    Serial::print("[INITRD] file: ", Serial::COM1);
                    Serial::print(e->name, Serial::COM1);
                    Serial::print(" (", Serial::COM1);
                    Serial::print_dec(static_cast<int32_t>(e->size), Serial::COM1);
                    Serial::print_line(" bytes)", Serial::COM1);
                }
            }
        }

        Scheduler::init();
        Scheduler::create("task_a", demo_task_a);
        Scheduler::create("task_b", demo_task_b);
        Scheduler::yield();
        Serial::print_line("[SCHED] Back in kernel_main", Serial::COM1);

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