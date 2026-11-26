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
char Shell::hist_lines[Shell::MAX_HIST_LINES][Shell::MAX_HIST_CHARS] = {{0}};
size_t Shell::hist_count = 0;
volatile bool Shell::dirty = false;
ShellLineKind Shell::hist_kinds[Shell::MAX_HIST_LINES] = {ShellLineKind::Output};

static void format_uint(uint32_t val, char* out, size_t cap) {
    if (cap == 0) {
        return;
    }
    if (val == 0) {
        out[0] = '0';
        out[1] = '\0';
        return;
    }
    char tmp[12];
    int idx = 0;
    while (val > 0 && idx < 11) {
        tmp[idx++] = static_cast<char>('0' + (val % 10));
        val /= 10;
    }
    size_t write_idx = 0;
    while (idx > 0 && write_idx + 1 < cap) {
        out[write_idx++] = tmp[--idx];
    }
    out[write_idx] = '\0';
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

void Shell::ui_write(const char* text, bool newline, ShellLineKind kind) {
    if (!text) {
        return;
    }
    if (hist_count == 0 || newline) {
        if (hist_count < MAX_HIST_LINES) {
            hist_lines[hist_count][0] = '\0';
            hist_kinds[hist_count] = kind;
            ++hist_count;
        } else {
            for (size_t i = 1; i < MAX_HIST_LINES; ++i) {
                memcpy(hist_lines[i - 1], hist_lines[i], MAX_HIST_CHARS);
                hist_kinds[i - 1] = hist_kinds[i];
            }
            hist_lines[MAX_HIST_LINES - 1][0] = '\0';
            hist_kinds[MAX_HIST_LINES - 1] = kind;
        }
    }

    char* dst = hist_lines[hist_count - 1];
    size_t len = strlen(dst);
    for (size_t i = 0; text[i] != '\0' && len + 1 < MAX_HIST_CHARS; ++i) {
        if (text[i] == '\b') {
            if (len > 0) {
                --len;
                dst[len] = '\0';
            }
        } else if (text[i] != '\n') {
            dst[len++] = text[i];
            dst[len] = '\0';
        }
    }
    dirty = true;
}

void Shell::ui_clear() {
    hist_count = 0;
    for (size_t i = 0; i < MAX_HIST_LINES; ++i) {
        hist_lines[i][0] = '\0';
    }
    dirty = true;
}

void Shell::out_text(const char* text, const char* color, bool newline) {
    if (g_term != nullptr) {
        g_term->print(text, color, newline);
    }
    ui_write(text, newline);
}

void Shell::out_uint(uint32_t val, const char* color, bool newline) {
    char num[16];
    format_uint(val, num, sizeof(num));
    out_text(num, color, newline);
}

void Shell::prompt() {
    if (g_term != nullptr) {
        g_term->print("singularity> ", "lightgreen", false);
    }
}

void Shell::init() {
    line_len = 0;
    line_buf[0] = '\0';
    ui_clear();
    prompt();
}

void Shell::cmd_help() {
    out_text("Available commands:", "yellow", true);
    out_text("  help       - show this command list", "white", true);
    out_text("  clear      - clear terminal", "white", true);
    out_text("  info       - system & architecture info", "white", true);
    out_text("  ticks      - show PIT tick count", "white", true);
    out_text("  mem        - show PMM & Heap stats", "white", true);
    out_text("  ls         - list initrd entries", "white", true);
    out_text("  cat <file> - print file from initrd", "white", true);
}

void Shell::cmd_clear() {
    if (g_term != nullptr) {
        g_term->clear();
    }
    ui_clear();
}

void Shell::cmd_info() {
    out_text("Singularity OS (x86 32-bit, Ring 0, Multiboot)", "lightcyan", true);
}

void Shell::cmd_ticks() {
    uint32_t t = Timer::ticks();
    out_text("Ticks: ", "white", false);
    out_uint(t, "yellow", false);
    out_text(" (~", "white", false);
    out_uint(t / 100, "yellow", false);
    out_text("s)", "white", true);
}

void Shell::cmd_mem() {
    out_text("PMM free/total: ", "white", false);
    out_uint(Pmm::free_frames(), "lightgreen", false);
    out_text("/", "white", false);
    out_uint(Pmm::total_frames(), "white", true);

    out_text("Heap used/total: ", "white", false);
    out_uint(static_cast<uint32_t>(Heap::used_size()), "lightcyan", false);
    out_text("/", "white", false);
    out_uint(static_cast<uint32_t>(Heap::total_size()), "white", true);
}

void Shell::ls() {
    if (!InitRd::is_ready()) {
        out_text("initrd not ready", "lightred", true);
        return;
    }

    for (size_t i = 0; i < InitRd::count(); ++i) {
        const InitRdEntry* entry = InitRd::get_at(i);
        if (entry == nullptr) {
            continue;
        }

        if (entry->is_dir) {
            out_text("[DIR]  ", "lightcyan", false);
            out_text(entry->name, "white", true);
        } else {
            out_text("[FILE] ", "lightgreen", false);
            out_text(entry->name, "white", false);
            out_text(" (", "white", false);
            out_uint(static_cast<uint32_t>(entry->size), "yellow", false);
            out_text(" B)", "white", true);
        }
    }
}

void Shell::cat_file() {
    if (!InitRd::is_ready()) {
        out_text("initrd not ready", "lightred", true);
        return;
    }

    const char* arg = line_buf + 3;
    while (*arg == ' ') {
        ++arg;
    }

    if (*arg == '\0') {
        out_text("Usage: cat <path>", "yellow", true);
        return;
    }

    const InitRdEntry* entry = InitRd::find(arg);
    if (entry == nullptr) {
        out_text("File not found: ", "lightred", false);
        out_text(arg, "white", true);
        return;
    }

    if (entry->is_dir) {
        out_text("Is a directory: ", "lightred", false);
        out_text(arg, "white", true);
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
        out_text(reinterpret_cast<const char*>(buf), "white", false);
        offset += static_cast<size_t>(n);
    }

    out_text("", "white", true);
}

void Shell::execute_line() {
    line_buf[line_len] = '\0';

    while (line_len > 0 && line_buf[line_len - 1] == ' ') {
        line_buf[--line_len] = '\0';
    }

    if (line_len == 0) {
        return;
    }

    ui_write("> ", true, ShellLineKind::Command);
    ui_write(line_buf, false, ShellLineKind::Command);

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
        out_text("Unknown command: ", "lightred", false);
        out_text(line_buf, "white", true);
    }

    line_len = 0;
    line_buf[0] = '\0';
}

void Shell::on_char(char c) {
    if (c == '\b') {
        if (line_len > 0) {
            --line_len;
            line_buf[line_len] = '\0';
            if (g_term != nullptr) {
                g_term->print("\b \b", "white", false);
            }
            dirty = true;
        }
        return;
    }

    if (c == '\n') {
        if (g_term != nullptr) {
            g_term->print("", "white", true);
        }
        execute_line();
        prompt();
        dirty = true;
        return;
    }

    if (c >= 32 && c < 127) {
        if (line_len < MAX_LINE_LEN - 1) {
            line_buf[line_len++] = c;
            line_buf[line_len] = '\0';
            if (g_term != nullptr) {
                char out[2] = { c, '\0' };
                g_term->print(out, "white", false);
            }
            dirty = true;
        }
    }
}

bool Shell::consume_dirty() {
    bool d = dirty;
    dirty = false;
    return d;
}

size_t Shell::history_count() {
    return hist_count;
}

const char* Shell::history_at(size_t idx) {
    if (idx >= hist_count) {
        return "";
    }
    return hist_lines[idx];
}

ShellLineKind Shell::history_kind_at(size_t idx) {
    if (idx >= hist_count) {
        return ShellLineKind::Output;
    }
    return hist_kinds[idx];
}

const char* Shell::current_input() {
    line_buf[line_len] = '\0';
    return line_buf;
}