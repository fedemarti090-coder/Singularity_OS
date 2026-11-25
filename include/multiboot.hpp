#pragma once
#include <stdint.h>

static constexpr uint32_t MULTIBOOT_BOOTLOADER_MAGIC = 0x2BADB002; // indirizzo del multi boot
static constexpr uint32_t MULTIBOOT_FLAG_MMAP        = (1 << 6);
static constexpr uint32_t MULTIBOOT_MEMORY_AVAILABLE = 1;

struct __attribute__((packed)) MultibootMmapEntry {
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;
};

struct __attribute__((packed)) MultibootInfo {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
};