#pragma once
#include <stddef.h>
#include <stdint.h>

class Shell {
public:
    static constexpr size_t MAX_LINE_LEN = 128;

    static void init();
    static void prompt();
    static void on_char(char c);

private:
    static char line_buf[MAX_LINE_LEN];
    static size_t line_len;

    static void execute_line();
    static void cmd_help();
    static void cmd_clear();
    static void cmd_info();
    static void cmd_ticks();
    static void cmd_mem();
    
    // ... future funzioni shell
};