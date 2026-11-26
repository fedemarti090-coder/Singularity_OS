#pragma once
#include <stddef.h>
#include <stdint.h>

class Heap {
public:
    static void init(size_t initial_pages = 16);
    static void* alloc(size_t size, size_t alignment = 16);
    static void free(void* ptr);

    static size_t total_size();
    static size_t used_size();
    static size_t free_size();

private:
    struct BlockHeader {
        size_t size;
        bool free;
        BlockHeader* next;
        BlockHeader* prev;
    };

    static BlockHeader* head;
    static size_t heap_total_bytes;
    static size_t heap_used_bytes;

    static bool expand(size_t min_bytes);
    static void split_block(BlockHeader* block, size_t req_size);
    static void coalesce(BlockHeader* block);
    static size_t align_up(size_t val, size_t align);
};

void* operator new(size_t size);
void* operator new[](size_t size);
void* operator new(size_t size, size_t align);
void operator delete(void* ptr) noexcept;
void operator delete[](void* ptr) noexcept;
void operator delete(void* ptr, size_t size) noexcept;
void operator delete[](void* ptr, size_t size) noexcept;