#pragma once
#include <stdint.h>
#include "io.hpp"

struct RtcTime {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
};

namespace Rtc {

inline uint8_t read_reg(uint8_t reg) {
    outb(0x70, reg);
    return inb(0x71);
}

inline bool is_updating() {
    return (read_reg(0x0A) & 0x80u) != 0;
}

inline uint8_t bcd_to_bin(uint8_t val) {
    return static_cast<uint8_t>((val & 0x0Fu) + ((val / 16u) * 10u));
}

inline RtcTime read_time(int8_t tz_offset_hours = 0) {
    while (is_updating()) {
    }

    uint8_t sec = read_reg(0x00);
    uint8_t min = read_reg(0x02);
    uint8_t hr  = read_reg(0x04);
    uint8_t reg_b = read_reg(0x0B);

    bool is_bcd = (reg_b & 0x04u) == 0;
    bool is_12h = (reg_b & 0x02u) == 0;
    bool pm     = (hr & 0x80u) != 0;

    if (is_bcd) {
        sec = bcd_to_bin(sec);
        min = bcd_to_bin(min);
        hr  = bcd_to_bin(static_cast<uint8_t>(hr & 0x7Fu));
    } else {
        hr = static_cast<uint8_t>(hr & 0x7Fu);
    }

    if (is_12h) {
        if (hr == 12u) {
            hr = 0u;
        }
        if (pm) {
            hr = static_cast<uint8_t>(hr + 12u);
        }
    }

    int32_t adjusted_hr = (static_cast<int32_t>(hr) + tz_offset_hours) % 24;
    if (adjusted_hr < 0) {
        adjusted_hr += 24;
    }

    return RtcTime{
        static_cast<uint8_t>(adjusted_hr),
        min,
        sec
    };
}

}