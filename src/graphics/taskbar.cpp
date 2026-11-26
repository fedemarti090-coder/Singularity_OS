#include "taskbar.hpp"
#include "mouse.hpp"

namespace Taskbar
{

    void init()
    {
        start_open = false;
        prev_left_down = Mouse::state().left_button;
    }

    bool is_start_open()
    {
        return start_open;
    }

    void close_start()
    {
        start_open = false;
    }

}