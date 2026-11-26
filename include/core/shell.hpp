#pragma once
#include <stddef.h>
#include <stdint.h>

enum class ShellLineKind : uint8_t {
    Output = 0,
    Command = 1
};

class Shell {
public:
    static constexpr size_t MAX_LINE_LEN = 128;
    static constexpr size_t MAX_HIST_LINES = 16;
    static constexpr size_t MAX_HIST_CHARS = 64;

    static void init();
    static void prompt();
    static void on_char(char c);

    static bool consume_dirty();
    static size_t history_count();
    static const char* history_at(size_t idx);
    static ShellLineKind history_kind_at(size_t idx);
    static const char* current_input();

private:
    static char line_buf[MAX_LINE_LEN];
    static size_t line_len;

    static char hist_lines[MAX_HIST_LINES][MAX_HIST_CHARS];
    static ShellLineKind hist_kinds[MAX_HIST_LINES];
    static size_t hist_count;
    static volatile bool dirty;

    static void ui_write(const char* text, bool newline, ShellLineKind kind = ShellLineKind::Output);
    static void ui_clear();
    static void out_text(const char* text, const char* color = "white", bool newline = false);
    static void out_uint(uint32_t val, const char* color = "white", bool newline = false);
    static void execute_line();
    static void cmd_help();
    static void cmd_clear();
    static void cmd_info();
    static void cmd_ticks();
    static void cmd_mem();

    static void ls();
    static void cat_file();
};