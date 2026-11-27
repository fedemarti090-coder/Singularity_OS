#include "keyboard.hpp"
#include "pic.hpp"
#include "io.hpp"
#include "vga.hpp"
#include "shell.hpp"
#include "_window.hpp"
#include "text_editor.hpp"
#include "pong.hpp"
#include "snake.hpp"
#include "tetris.hpp"

extern "C"
{
    struct Registers;
}

void register_irq_handler(uint8_t irq, void (*cb)(Registers *));

static void keyboard_irq_callback(Registers *)
{
    Keyboard::on_irq();
}

static const char scancode_set1[128] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', '\t',
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0, ' ', 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, '-', 0, 0, 0, '+', 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0};

char Keyboard::scancode_to_ascii(uint8_t scancode)
{
    if (scancode >= 128)
    {
        return 0;
    }
    return scancode_set1[scancode];
}

void Keyboard::init()
{
    while (inb(STATUS_PORT) & STATUS_OUT_FULL)
    {
        (void)inb(DATA_PORT);
    }

    register_irq_handler(1, keyboard_irq_callback);
    Pic::clear_mask(1);
}

void Keyboard::on_irq()
{
    static bool extended = false;

    uint8_t status = inb(STATUS_PORT);
    if (!(status & STATUS_OUT_FULL))
    {
        return;
    }

    uint8_t scancode = inb(DATA_PORT);

    if (scancode == 0xE0)
    {
        extended = true;
        return;
    }

    Window *active = WindowManager::focused_window();
    bool pong_focused = (active != nullptr && active->visible && active->kind == WindowKind::Pong);
    bool snake_focused = (active != nullptr && active->visible && active->kind == WindowKind::Snake);
    bool tetris_focused = (active != nullptr && active->visible && active->kind == WindowKind::Tetris);
    bool editor_focused = (active != nullptr && active->visible && active->kind == WindowKind::TextViewer);

    int32_t th = 0;
    int32_t cw = 0;
    int32_t ch = 0;
    if (active != nullptr)
    {
        th = (active->h < WindowManager::TITLE_H) ? active->h : WindowManager::TITLE_H;
        cw = (active->w > 2) ? (active->w - 2) : 0;
        ch = (active->h - th - 2 > 0) ? (active->h - th - 2) : 0;
    }

    if (extended)
    {
        extended = false;
        if (scancode & 0x80)
        {
            return;
        }
        if (pong_focused)
        {
            if (scancode == 0x48)
            {
                PongGame::on_key('w', ch);
            }
            else if (scancode == 0x50)
            {
                PongGame::on_key('s', ch);
            }
            return;
        }
        if (snake_focused)
        {
            SnakeGame::on_arrow(scancode, cw, ch);
            return;
        }
        if (tetris_focused)
        {
            TetrisGame::on_arrow(scancode);
            return;
        }
        if (editor_focused)
        {
            switch (scancode)
            {
            case 0x48:
                TextEditor::on_key(TextEditor::KeyCode::Up);
                break;
            case 0x50:
                TextEditor::on_key(TextEditor::KeyCode::Down);
                break;
            case 0x4B:
                TextEditor::on_key(TextEditor::KeyCode::Left);
                break;
            case 0x4D:
                TextEditor::on_key(TextEditor::KeyCode::Right);
                break;
            case 0x47:
                TextEditor::on_key(TextEditor::KeyCode::Home);
                break;
            case 0x4F:
                TextEditor::on_key(TextEditor::KeyCode::End);
                break;
            case 0x53:
                TextEditor::on_key(TextEditor::KeyCode::Delete);
                break;
            default:
                break;
            }
        }
        return;
    }

    if (scancode & 0x80)
    {
        return;
    }

    char c = scancode_to_ascii(scancode);
    if (c == 0)
    {
        return;
    }

    if (pong_focused)
    {
        PongGame::on_key(c, ch);
        return;
    }

    if (snake_focused)
    {
        SnakeGame::on_key(c, cw, ch);
        return;
    }

    if (tetris_focused)
    {
        TetrisGame::on_key(c);
        return;
    }

    if (editor_focused)
    {
        if (c == '\b')
        {
            TextEditor::on_backspace();
        }
        else
        {
            TextEditor::on_char(c);
        }
        return;
    }

    Shell::on_char(c);
}