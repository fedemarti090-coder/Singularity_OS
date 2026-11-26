#include "initrd.hpp"
#include "shell.hpp"
#include "vga.hpp"
#include "timer.hpp"
#include "pmm.hpp"
#include "heap.hpp"
#include "memory.hpp"
#include "serial.hpp"

char Shell::line_buf[Shell::MAX_LINE_LEN] = {0};
size_t Shell::line_len = 0;

static void print_uint_vga(uint32_t val, const char* color = "white") {
    if (val == 0) {
        g_term->print("0", color, false);
        return;
    }
    char buf[12];
    int idx = 0;
    while (val > 0 && idx < 11) {
        buf[idx++] = static_cast<char>('0' + (val % 10));
        val /= 10;
    }
    buf[idx] = '\0';
    for (int i = 0; i < idx / 2; ++i) {
        char tmp = buf[i];
        buf[i] = buf[idx - 1 - i];
        buf[idx - 1 - i] = tmp;
    }
    g_term->print(buf, color, false);
}

static bool starts_with(const char* str, const char* prefix) {
    if (!str || !prefix) {
        return false;
    }
    while (*prefix != '\0') {
        if (*str != *prefix) {
            return false;
        }
        ++str;
        ++prefix;
    }
    return true;
}

void Shell::prompt() {
    if (!g_term) {
        return;
    }
    g_term->print("singularity> ", "lightgreen", false);
}

void Shell::init() {
    line_len = 0;
    line_buf[0] = '\0';
    prompt();
}

void Shell::cmd_help() {
    g_term->print("Available commands:", "yellow", true);
    g_term->print("  help       - show this command list", "white", true);
    g_term->print("  clear      - clear VGA terminal", "white", true);
    g_term->print("  info       - system & architecture info", "white", true);
    g_term->print("  ticks      - show PIT tick count", "white", true);
    g_term->print("  mem        - show PMM & Heap stats", "white", true);
    g_term->print("  ls         - list initrd entries", "white", true);
    g_term->print("  cat <file> - print file from initrd", "white", true);
}

void Shell::cmd_clear() {
    if (g_term != nullptr) {
        g_term->clear();
    }
}

void Shell::cmd_info() {
    g_term->print("Singularity OS (x86 32-bit, Ring 0, Multiboot)", "lightcyan", true);
}

void Shell::cmd_ticks() {
    uint32_t t = Timer::ticks();
    g_term->print("Ticks: ", "white", false);
    print_uint_vga(t, "yellow");
    g_term->print(" (~", "white", false);
    print_uint_vga(t / 100, "yellow");
    g_term->print("s)", "white", true);
}

void Shell::cmd_mem() {
    g_term->print("PMM free/total frames: ", "white", false);
    print_uint_vga(Pmm::free_frames(), "lightgreen");
    g_term->print("/", "white", false);
    print_uint_vga(Pmm::total_frames(), "white");
    g_term->print("", "white", true);

    g_term->print("Heap used/total bytes: ", "white", false);
    print_uint_vga(static_cast<uint32_t>(Heap::used_size()), "lightcyan");
    g_term->print("/", "white", false);
    print_uint_vga(static_cast<uint32_t>(Heap::total_size()), "white");
    g_term->print("", "white", true);
}

void Shell::ls() {
    if (!g_term) {
        return;
    }

    if (!InitRd::is_ready()) {
        g_term->print("initrd not ready", "lightred", true);
        return;
    }

    for (size_t i = 0; i < InitRd::count(); ++i) {
        const InitRdEntry* entry = InitRd::get_at(i);
        if (entry == nullptr) {
            continue;
        }

        if (entry->is_dir) {
            g_term->print("[DIR]  ", "lightcyan", false);
            g_term->print(entry->name, "white", true);
        } else {
            g_term->print("[FILE] ", "lightgreen", false);
            g_term->print(entry->name, "white", false);
            g_term->print(" (", "white", false);
            print_uint_vga(static_cast<uint32_t>(entry->size), "yellow");
            g_term->print(" B)", "white", true);
        }
    }
}

void Shell::cat_file() {
    if (!g_term) {
        return;
    }

    if (!InitRd::is_ready()) {
        g_term->print("initrd not ready", "lightred", true);
        return;
    }

    const char* arg = line_buf + 3;
    while (*arg == ' ') {
        ++arg;
    }

    if (*arg == '\0') {
        g_term->print("Usage: cat <path>", "yellow", true);
        return;
    }

    const InitRdEntry* entry = InitRd::find(arg);
    if (entry == nullptr) {
        g_term->print("File not found: ", "lightred", false);
        g_term->print(arg, "white", true);
        return;
    }

    if (entry->is_dir) {
        g_term->print("Is a directory: ", "lightred", false);
        g_term->print(arg, "white", true);
        return;
    }

    uint8_t buf[512];
    size_t offset = 0;

    while (offset < entry->size) {
        int32_t n = InitRd::read(arg, buf, sizeof(buf) - 1, offset);
        if (n <= 0) {
            break;
        }
        buf[n] = '\0';
        g_term->print(reinterpret_cast<const char*>(buf), "white", false);
        offset += static_cast<size_t>(n);
    }

    g_term->print("", "white", true);
}

void Shell::execute_line() {
    line_buf[line_len] = '\0';

    while (line_len > 0 && line_buf[line_len - 1] == ' ') {
        line_buf[--line_len] = '\0';
    }

    if (line_len == 0) {
        return;
    }

    Serial::print("[SHELL] cmd: ", Serial::COM1);
    Serial::print_line(line_buf, Serial::COM1);

    if (strcmp(line_buf, "help") == 0) {
        cmd_help();
    } else if (strcmp(line_buf, "clear") == 0) {
        cmd_clear();
    } else if (strcmp(line_buf, "info") == 0) {
        cmd_info();
    } else if (strcmp(line_buf, "ticks") == 0) {
        cmd_ticks();
    } else if (strcmp(line_buf, "mem") == 0) {
        cmd_mem();
    } else if (strcmp(line_buf, "ls") == 0) {
        ls();
    } else if (strcmp(line_buf, "cat") == 0 || starts_with(line_buf, "cat ")) {
        cat_file();
    } else {
        g_term->print("Unknown command: ", "lightred", false);
        g_term->print(line_buf, "white", true);
    }

    line_len = 0;
    line_buf[0] = '\0';
}

void Shell::on_char(char c) {
    if (!g_term) {
        return;
    }

    if (c == '\b') {
        if (line_len > 0) {
            --line_len;
            line_buf[line_len] = '\0';
            g_term->print("\b \b", "white", false);
        }
        return;
    }

    if (c == '\n') {
        g_term->print("", "white", true);
        execute_line();
        prompt();
        return;
    }

    if (c >= 32 && c < 127) {
        if (line_len < MAX_LINE_LEN - 1) {
            line_buf[line_len++] = c;
            line_buf[line_len] = '\0';
            char out[2] = { c, '\0' };
            g_term->print(out, "white", false);
        }
    }
}