#pragma once
#include <stdint.h>

namespace Mouse {

struct State {
    int32_t x = 512;
    int32_t y = 384;
    bool left_button = false;
    bool right_button = false;
    bool middle_button = false;
    bool dirty = true;
};

void init();
const State& state();
void clear_dirty();

}