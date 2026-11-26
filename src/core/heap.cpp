#include "heap.hpp"
#include "pmm.hpp"
#include "memory.hpp"
#include "paging.hpp"

Heap::BlockHeader* Heap::head = nullptr;
size_t Heap::heap_total_bytes = 0;
size_t Heap::heap_used_bytes = 0;

size_t Heap::align_up(size_t val, size_t align) {
    if (align == 0) {
        return val;
    }
    return (val + align - 1) & ~(align - 1);
}

void Heap::split_block(BlockHeader* block, size_t req_size) {
    size_t min_remainder = sizeof(BlockHeader) + 16;
    if (block->size < req_size + min_remainder) {
        return;
    }

    uint8_t* raw = reinterpret_cast<uint8_t*>(block + 1);
    auto* next_block = reinterpret_cast<BlockHeader*>(raw + req_size);

    next_block->size = block->size - req_size - sizeof(BlockHeader);
    next_block->free = true;
    next_block->next = block->next;
    next_block->prev = block;

    if (block->next != nullptr) {
        block->next->prev = next_block;
    }

    block->size = req_size;
    block->next = next_block;
}

void Heap::coalesce(BlockHeader* block) {
    if (block->next != nullptr && block->next->free) {
        BlockHeader* next_b = block->next;
        block->size += sizeof(BlockHeader) + next_b->size;
        block->next = next_b->next;
        if (next_b->next != nullptr) {
            next_b->next->prev = block;
        }
    }

    if (block->prev != nullptr && block->prev->free) {
        BlockHeader* prev_b = block->prev;
        prev_b->size += sizeof(BlockHeader) + block->size;
        prev_b->next = block->next;
        if (block->next != nullptr) {
            block->next->prev = prev_b;
        }
    }
}

bool Heap::expand(size_t min_bytes) {
    size_t needed = align_up(min_bytes + sizeof(BlockHeader), Pmm::PAGE_SIZE);
    size_t pages = needed / Pmm::PAGE_SIZE;
    if (pages == 0) {
        pages = 1;
    }

    uint32_t first_frame = Pmm::alloc_frame();
    if (first_frame == 0) {
        return false;
    }
    Paging::map_page(first_frame, first_frame, Paging::FLAG_PRESENT | Paging::FLAG_RW);

    uint32_t expected_next = first_frame + Pmm::PAGE_SIZE;
    size_t contiguous_pages = 1;

    while (contiguous_pages < pages) {
        uint32_t frame = Pmm::alloc_frame();
        if (frame == 0) {
            break;
        }
        if (frame == expected_next) {
            Paging::map_page(frame, frame, Paging::FLAG_PRESENT | Paging::FLAG_RW);
            expected_next += Pmm::PAGE_SIZE;
            ++contiguous_pages;
        } else {
            Pmm::free_frame(frame);
            break;
        }
    }

    size_t total_chunk = contiguous_pages * Pmm::PAGE_SIZE;
    auto* new_block = reinterpret_cast<BlockHeader*>(first_frame);
    new_block->size = total_chunk - sizeof(BlockHeader);
    new_block->free = true;
    new_block->next = nullptr;
    new_block->prev = nullptr;

    if (head == nullptr) {
        head = new_block;
    } else {
        BlockHeader* tail = head;
        while (tail->next != nullptr) {
            tail = tail->next;
        }
        tail->next = new_block;
        new_block->prev = tail;
        coalesce(new_block);
    }

    heap_total_bytes += total_chunk;
    return true;
}

void Heap::init(size_t initial_pages) {
    head = nullptr;
    heap_total_bytes = 0;
    heap_used_bytes = 0;
    if (initial_pages > 0) {
        expand(initial_pages * Pmm::PAGE_SIZE);
    }
}

void* Heap::alloc(size_t size, size_t alignment) {
    if (size == 0) {
        return nullptr;
    }

    size_t aligned_size = align_up(size, 16);
    size_t eff_align = (alignment < 16) ? 16 : alignment;

    for (int attempt = 0; attempt < 2; ++attempt) {
        BlockHeader* curr = head;
        while (curr != nullptr) {
            if (curr->free) {
                uintptr_t payload = reinterpret_cast<uintptr_t>(curr + 1);
                uintptr_t aligned_payload = align_up(payload, eff_align);
                size_t pad = static_cast<size_t>(aligned_payload - payload);
                size_t total_needed = pad + aligned_size;

                if (curr->size >= total_needed) {
                    if (pad >= sizeof(BlockHeader) + 16) {
                        auto* split_hdr = reinterpret_cast<BlockHeader*>(aligned_payload - sizeof(BlockHeader));
                        split_hdr->size = curr->size - pad;
                        split_hdr->free = true;
                        split_hdr->next = curr->next;
                        split_hdr->prev = curr;

                        if (curr->next != nullptr) {
                            curr->next->prev = split_hdr;
                        }

                        curr->size = pad - sizeof(BlockHeader);
                        curr->free = true;
                        curr->next = split_hdr;
                        curr = split_hdr;
                    }

                    split_block(curr, aligned_size);
                    curr->free = false;
                    heap_used_bytes += curr->size;
                    return reinterpret_cast<void*>(curr + 1);
                }
            }
            curr = curr->next;
        }

        if (attempt == 0) {
            if (!expand(aligned_size + eff_align)) {
                break;
            }
        }
    }

    return nullptr;
}

void Heap::free(void* ptr) {
    if (ptr == nullptr) {
        return;
    }

    auto* block = reinterpret_cast<BlockHeader*>(ptr) - 1;
    if (block->free) {
        return;
    }

    block->free = true;
    if (heap_used_bytes >= block->size) {
        heap_used_bytes -= block->size;
    } else {
        heap_used_bytes = 0;
    }

    coalesce(block);
}

size_t Heap::total_size() {
    return heap_total_bytes;
}

size_t Heap::used_size() {
    return heap_used_bytes;
}

size_t Heap::free_size() {
    if (heap_total_bytes <= heap_used_bytes) {
        return 0;
    }
    return heap_total_bytes - heap_used_bytes;
}

void* operator new(size_t size) {
    return Heap::alloc(size);
}

void* operator new[](size_t size) {
    return Heap::alloc(size);
}

void* operator new(size_t size, size_t align) {
    return Heap::alloc(size, align);
}

void operator delete(void* ptr) noexcept {
    Heap::free(ptr);
}

void operator delete[](void* ptr) noexcept {
    Heap::free(ptr);
}

void operator delete(void* ptr, size_t) noexcept {
    Heap::free(ptr);
}

void operator delete[](void* ptr, size_t) noexcept {
    Heap::free(ptr);
}