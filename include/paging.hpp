#pragma once
#include <stddef.h>
#include <stdint.h>

class Paging {
public:
    static constexpr uint32_t PAGE_SIZE = 4096;
    static constexpr uint32_t ENTRIES_PER_TABLE = 1024;

    static constexpr uint32_t FLAG_PRESENT  = 1u << 0;
    static constexpr uint32_t FLAG_RW       = 1u << 1;
    static constexpr uint32_t FLAG_USER     = 1u << 2;
    static constexpr uint32_t FLAG_ACCESSED = 1u << 5;
    static constexpr uint32_t FLAG_DIRTY    = 1u << 6;

    static void init(uint32_t identity_map_bytes = 32u * 1024u * 1024u);
    static bool map_page(uint32_t virt_addr, uint32_t phys_addr, uint32_t flags = FLAG_PRESENT | FLAG_RW);
    static void unmap_page(uint32_t virt_addr);
    static uint32_t get_phys_addr(uint32_t virt_addr);
    static void invlpg(uint32_t virt_addr);
    static uint32_t current_directory();

private:
    static uint32_t* kernel_dir;

    static uint32_t* get_or_create_pt(uint32_t pd_idx, bool create, uint32_t pt_flags);
    static void load_cr3(uint32_t phys_pd);
    static void enable_paging_bit();
};