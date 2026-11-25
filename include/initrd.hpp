#pragma once

#include <stddef.h>
#include <stdint.h>
#include "multiboot.hpp"

struct InitRdEntry {
    const char* name;
    const uint8_t* data;
    size_t size;
    bool is_dir;
};

class InitRd {
public:
    static constexpr size_t MAX_ENTRIES = 64;

    static bool init(const MultibootInfo* mb_info);
    static bool is_ready();
    static size_t count();
    static const InitRdEntry* get_at(size_t index);
    static const InitRdEntry* find(const char* path);
    static int32_t read(const char* path, uint8_t* dst, size_t max_len, size_t offset = 0);

private:
    static InitRdEntry entries[MAX_ENTRIES];
    static size_t entry_count;
    static bool ready;

    static uint64_t parse_octal(const char* buf, size_t len);
    static const char* normalize_path(const char* raw);
};