#pragma once
#include <stddef.h>
#include <stdint.h>
#include "multiboot.hpp"

class Pmm {
public:
    static constexpr uint32_t PAGE_SIZE = 4096;

    static void init(const MultibootInfo* mb_info, uint32_t kernel_start, uint32_t kernel_end);
    static uint32_t alloc_frame();
    static void free_frame(uint32_t phys_addr);
    static void reserve_region(uint32_t base_addr, uint32_t length);
    static void release_region(uint32_t base_addr, uint32_t length);

    static uint32_t total_frames();
    static uint32_t used_frames();
    static uint32_t free_frames();

private:
    static constexpr uint32_t MAX_PHYS_BYTES = 0x40000000;
    static constexpr uint32_t MAX_FRAMES     = MAX_PHYS_BYTES / PAGE_SIZE;
    static constexpr uint32_t BITMAP_BYTES   = MAX_FRAMES / 8;

    static uint8_t bitmap[BITMAP_BYTES];
    static uint32_t total_frame_count;
    static uint32_t used_frame_count;

    static inline void set_bit(uint32_t frame) {
        bitmap[frame / 8] |= static_cast<uint8_t>(1u << (frame % 8));
    }

    static inline void clear_bit(uint32_t frame) {
        bitmap[frame / 8] &= static_cast<uint8_t>(~(1u << (frame % 8)));
    }

    static inline bool test_bit(uint32_t frame) {
        return (bitmap[frame / 8] & static_cast<uint8_t>(1u << (frame % 8))) != 0;
    }

    static inline uint32_t align_up_4k(uint32_t addr) {
        return (addr + PAGE_SIZE - 1u) & ~(PAGE_SIZE - 1u);
    }

    static inline uint32_t align_down_4k(uint32_t addr) {
        return addr & ~(PAGE_SIZE - 1u);
    }
};