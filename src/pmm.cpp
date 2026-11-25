#include "pmm.hpp"

uint8_t Pmm::bitmap[Pmm::BITMAP_BYTES] = {0};
uint32_t Pmm::total_frame_count = 0;
uint32_t Pmm::used_frame_count = 0;

void Pmm::release_region(uint32_t base_addr, uint32_t length) {
    if (length == 0) {
        return;
    }

    uint64_t start64 = align_up_4k(base_addr);
    uint64_t end64   = align_down_4k(static_cast<uint64_t>(base_addr) + length);

    if (end64 > MAX_PHYS_BYTES) {
        end64 = MAX_PHYS_BYTES;
    }
    if (start64 >= end64) {
        return;
    }

    uint32_t start_frame = static_cast<uint32_t>(start64 / PAGE_SIZE);
    uint32_t end_frame   = static_cast<uint32_t>(end64 / PAGE_SIZE);

    for (uint32_t f = start_frame; f < end_frame; ++f) {
        if (test_bit(f)) {
            clear_bit(f);
            if (used_frame_count > 0) {
                --used_frame_count;
            }
        }
    }
}

void Pmm::reserve_region(uint32_t base_addr, uint32_t length) {
    if (length == 0) {
        return;
    }

    uint64_t start64 = align_down_4k(base_addr);
    uint64_t end64   = align_up_4k(static_cast<uint64_t>(base_addr) + length);

    if (end64 > MAX_PHYS_BYTES) {
        end64 = MAX_PHYS_BYTES;
    }
    if (start64 >= end64) {
        return;
    }

    uint32_t start_frame = static_cast<uint32_t>(start64 / PAGE_SIZE);
    uint32_t end_frame   = static_cast<uint32_t>(end64 / PAGE_SIZE);

    for (uint32_t f = start_frame; f < end_frame; ++f) {
        if (!test_bit(f)) {
            set_bit(f);
            ++used_frame_count;
        }
    }
}

void Pmm::init(const MultibootInfo* mb_info, uint32_t kernel_start, uint32_t kernel_end) {
    for (uint32_t i = 0; i < BITMAP_BYTES; ++i) {
        bitmap[i] = 0xFF;
    }
    total_frame_count = 0;
    used_frame_count = MAX_FRAMES;

    if (mb_info != nullptr && (mb_info->flags & MULTIBOOT_FLAG_MMAP)) {
        uint32_t mmap_ptr = mb_info->mmap_addr;
        uint32_t mmap_end = mb_info->mmap_addr + mb_info->mmap_length;

        while (mmap_ptr < mmap_end) {
            const auto* entry = reinterpret_cast<const MultibootMmapEntry*>(mmap_ptr);

            if (entry->type == MULTIBOOT_MEMORY_AVAILABLE && entry->addr < MAX_PHYS_BYTES) {
                uint64_t region_start = entry->addr;
                uint64_t region_end   = entry->addr + entry->len;

                if (region_end > MAX_PHYS_BYTES) {
                    region_end = MAX_PHYS_BYTES;
                }

                uint32_t aligned_start = align_up_4k(static_cast<uint32_t>(region_start));
                uint32_t aligned_end   = align_down_4k(static_cast<uint32_t>(region_end));

                if (aligned_end > aligned_start) {
                    uint32_t frames_in_region = (aligned_end - aligned_start) / PAGE_SIZE;
                    total_frame_count += frames_in_region;
                    release_region(aligned_start, aligned_end - aligned_start);
                }
            }

            mmap_ptr += entry->size + sizeof(entry->size);
        }
    }

    reserve_region(0x00000000, 0x00100000);

    if (kernel_end > kernel_start) {
        reserve_region(kernel_start, kernel_end - kernel_start);
    }

    uint32_t bitmap_start = reinterpret_cast<uint32_t>(&bitmap[0]);
    reserve_region(bitmap_start, BITMAP_BYTES);

    if (mb_info != nullptr) {
        reserve_region(reinterpret_cast<uint32_t>(mb_info), sizeof(MultibootInfo));
        if (mb_info->flags & MULTIBOOT_FLAG_MMAP) {
            reserve_region(mb_info->mmap_addr, mb_info->mmap_length);
        }
    }
}

uint32_t Pmm::alloc_frame() {
    for (uint32_t byte_idx = 0; byte_idx < BITMAP_BYTES; ++byte_idx) {
        if (bitmap[byte_idx] == 0xFF) {
            continue;
        }
        for (uint32_t bit_idx = 0; bit_idx < 8; ++bit_idx) {
            uint32_t frame = byte_idx * 8 + bit_idx;
            if (frame >= MAX_FRAMES) {
                return 0;
            }
            if (!test_bit(frame)) {
                set_bit(frame);
                ++used_frame_count;
                return frame * PAGE_SIZE;
            }
        }
    }
    return 0;
}

void Pmm::free_frame(uint32_t phys_addr) {
    if (phys_addr < 0x00100000) {
        return;
    }
    uint32_t frame = align_down_4k(phys_addr) / PAGE_SIZE;
    if (frame < MAX_FRAMES && test_bit(frame)) {
        clear_bit(frame);
        if (used_frame_count > 0) {
            --used_frame_count;
        }
    }
}

uint32_t Pmm::total_frames() {
    return total_frame_count;
}

uint32_t Pmm::used_frames() {
    return used_frame_count;
}

uint32_t Pmm::free_frames() {
    if (total_frame_count <= used_frame_count) {
        return 0;
    }
    return total_frame_count - used_frame_count;
}