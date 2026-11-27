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
#include "framebuffer.hpp"
#include "bmp.hpp"
#include "font.hpp"
#include "mouse.hpp"
#include "cursor.hpp"
#include "icons.hpp"
#include "taskbar.hpp"
#include "rtc.hpp"
#include "_window.hpp"
#include "text_editor.hpp"

extern "C" uint8_t kernel_start[];
extern "C" uint8_t kernel_end[];

static void demo_task_a(void *)
{
    Serial::print_line("[TASK A] entered", Serial::COM1);
    for (int i = 0; i < 3; ++i)
    {
        Serial::print_line("[TASK A] running", Serial::COM1);
        Scheduler::yield();
    }
}

static void demo_task_b(void *)
{
    for (int i = 0; i < 3; ++i)
    {
        Serial::print_line("[TASK B] running", Serial::COM1);
        Scheduler::yield();
    }
}

static void render_frame(bool fb_ok)
{
    if (!fb_ok)
    {
        return;
    }

    const auto &m = Mouse::state();
    Framebuffer::clear(Framebuffer::rgb(0, 0, 0));
    if (Wallpaper::is_loaded())
    {
        Wallpaper::draw_to_backbuffer();
    }
    Font8x16::draw_text(24, 24, "Singularity OS", Framebuffer::rgb(255, 255, 255));

    WindowManager::draw_all();
    Taskbar::draw();

    Cursor::draw(m.x, m.y);
    Framebuffer::swap_buffers();
    Mouse::clear_dirty();
}

extern "C" void kernel_main(uint32_t magic, const MultibootInfo *mb_info)
{
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

    Mouse::init();

    if (mb_info->flags & MULTIBOOT_FLAG_FRAMEBUFFER)
    {
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
    }
    else
    {
        Serial::print_line("[FB] Flag framebuffer NON presente!", Serial::COM1);
    }

    Keyboard::init();
    term.print("PS/2 Keyboard ready.", "yellow");

    bool fb_ok = false;

    if (magic == MULTIBOOT_BOOTLOADER_MAGIC)
    {
        Pmm::init(mb_info, reinterpret_cast<uint32_t>(kernel_start), reinterpret_cast<uint32_t>(kernel_end));
        Heap::init(16);
        Paging::init(32u * 1024u * 1024u);

        fb_ok = Framebuffer::init(mb_info);
        bool initrd_ok = InitRd::init(mb_info);

        WindowManager::init();

        if (initrd_ok)
        {
            for (size_t i = 0; i < InitRd::count(); ++i)
            {
                const InitRdEntry *e = InitRd::get_at(i);
                if (e != nullptr && !e->is_dir)
                {
                    Serial::print("[INITRD] file: ", Serial::COM1);
                    Serial::print(e->name, Serial::COM1);
                    Serial::print(" (", Serial::COM1);
                    Serial::print_dec(static_cast<int32_t>(e->size), Serial::COM1);
                    Serial::print_line(" bytes)", Serial::COM1);
                }
            }
            Wallpaper::load_from_initrd("wallpaper.bmp");
        }

        Scheduler::init();
        Scheduler::create("task_a", demo_task_a);
        Scheduler::create("task_b", demo_task_b);
        Scheduler::yield();
        Serial::print_line("[SCHED] Back in kernel_main", Serial::COM1);

        char *buf = static_cast<char *>(Heap::alloc(32));
        const char *msg = "Singularity Heap OK";
        if (buf != nullptr)
        {
            memcpy(buf, msg, strlen(msg) + 1);
            if (strcmp(buf, msg) == 0)
            {
                term.print(buf, "lightgreen");
            }
            Heap::free(buf);
        }

        if (fb_ok)
        {
            render_frame(fb_ok);
        }

        Serial::print("[PMM] Free frames: ", Serial::COM1);
        Serial::print_dec(Pmm::free_frames(), Serial::COM1);
        Serial::print_line("", Serial::COM1);
    }
    else
    {
        term.print("Multiboot magic mismatch!", "lightred");
    }

    Timer::init(100);
    Shell::init();
    Taskbar::init();

    asm volatile("sti");

    uint8_t last_rtc_sec = 0xFF;
    uint32_t last_blink_phase = 0xFFFFFFFFu;
    uint64_t last_render_tick = 0;
    constexpr uint64_t RENDER_STEP_TICKS = 2;

    while (true)
    {
        const auto &m = Mouse::state();
        bool left_down = m.left_button;

        bool tb_changed = Taskbar::on_mouse(m.x, m.y, left_down);
        bool win_changed = tb_changed ? false : WindowManager::on_mouse(m.x, m.y, left_down);
        if (win_changed)
        {
            Taskbar::close_start();
        }
        bool shell_changed = Shell::consume_dirty();

        uint32_t blink_phase = Timer::ticks() / 50u;
        bool blink_changed = (blink_phase != last_blink_phase);

        bool text_editor_changed = TextEditor::consume_dirty();

        Window *fw = WindowManager::focused_window();
        bool pong_active = (fw != nullptr && fw->visible && fw->kind == WindowKind::Pong);

        uint8_t now_sec = Rtc::read_time(1).seconds;
        bool ui_changed = m.dirty || tb_changed || win_changed || shell_changed || blink_changed || text_editor_changed;
        bool time_changed = (now_sec != last_rtc_sec);

        uint64_t now_tick = Timer::ticks();
        bool tick_ready = (now_tick - last_render_tick) >= RENDER_STEP_TICKS;

        if (fb_ok && tick_ready && (ui_changed || time_changed || pong_active))
        {
            last_render_tick = now_tick;
            last_rtc_sec = now_sec;
            last_blink_phase = blink_phase;
            render_frame(fb_ok);
        }
        asm volatile("hlt");
    }
}
