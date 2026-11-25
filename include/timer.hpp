#pragma once
#include <stdint.h>

class Timer {
public:
    static void init(uint32_t frequency_hz = 100);
    static uint32_t ticks();
};