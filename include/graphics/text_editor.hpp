#pragma once
#include <stdint.h>
#include <stddef.h>

namespace TextEditor
{

    constexpr size_t MAX_TEXT_LEN = 4096;
    constexpr size_t MAX_NAME_LEN = 64;

    enum class KeyCode : uint8_t
    {
        Left = 1,
        Right,
        Up,
        Down,
        Home,
        End,
        Delete
    };

    void init();
    bool open_file(const char *name);
    void new_buffer(const char *name = "untitled.txt");

    void on_char(char ch);
    void on_backspace();
    void on_key(KeyCode key);
    void on_mouse_click(int32_t mx, int32_t my, int32_t cx, int32_t cy, int32_t cw, int32_t ch);
    void scroll_lines(int32_t delta);

    bool save_to_initrd();

    bool is_open();
    const char *file_name();
    bool is_dirty();
    bool consume_dirty();

    void draw_content(int32_t cx, int32_t cy, int32_t cw, int32_t ch, bool focused);

}