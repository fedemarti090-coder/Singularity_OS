#include "paging.hpp"
#include "pmm.hpp"
#include "memory.hpp"
#include "serial.hpp"

uint32_t* Paging::kernel_dir = nullptr;

void Paging::invlpg(uint32_t virt_addr) {
    asm volatile("invlpg (%0)" : : "r"(virt_addr) : "memory");
}

void Paging::load_cr3(uint32_t phys_pd) {
    asm volatile("mov %0, %%cr3" : : "r"(phys_pd) : "memory");
}

void Paging::enable_paging_bit() {
    uint32_t cr0 = 0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000u;
    asm volatile("mov %0, %%cr0" : : "r"(cr0) : "memory");
}

uint32_t Paging::current_directory() {
    uint32_t cr3 = 0;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

uint32_t* Paging::get_or_create_pt(uint32_t pd_idx, bool create, uint32_t pt_flags) {
    if (kernel_dir == nullptr || pd_idx >= ENTRIES_PER_TABLE) {
        return nullptr;
    }

    uint32_t pde = kernel_dir[pd_idx];
    if (pde & FLAG_PRESENT) {
        return reinterpret_cast<uint32_t*>(pde & 0xFFFFF000u);
    }

    if (!create) {
        return nullptr;
    }

    uint32_t pt_frame = Pmm::alloc_frame();
    if (pt_frame == 0) {
        return nullptr;
    }

    uint32_t* pt = reinterpret_cast<uint32_t*>(pt_frame);
    memset(pt, 0, PAGE_SIZE);

    uint32_t flags = FLAG_PRESENT | FLAG_RW | (pt_flags & FLAG_USER);
    kernel_dir[pd_idx] = (pt_frame & 0xFFFFF000u) | flags;
    return pt;
}

bool Paging::map_page(uint32_t virt_addr, uint32_t phys_addr, uint32_t flags) {
    if (kernel_dir == nullptr) {
        return false;
    }

    uint32_t v_aligned = virt_addr & 0xFFFFF000u;
    uint32_t p_aligned = phys_addr & 0xFFFFF000u;
    uint32_t pd_idx = (v_aligned >> 22) & 0x3FFu;
    uint32_t pt_idx = (v_aligned >> 12) & 0x3FFu;

    uint32_t* pt = get_or_create_pt(pd_idx, true, flags);
    if (pt == nullptr) {
        return false;
    }

    pt[pt_idx] = p_aligned | (flags & 0xFFFu) | FLAG_PRESENT;
    invlpg(v_aligned);
    return true;
}

void Paging::unmap_page(uint32_t virt_addr) {
    if (kernel_dir == nullptr) {
        return;
    }

    uint32_t v_aligned = virt_addr & 0xFFFFF000u;
    uint32_t pd_idx = (v_aligned >> 22) & 0x3FFu;
    uint32_t pt_idx = (v_aligned >> 12) & 0x3FFu;

    uint32_t* pt = get_or_create_pt(pd_idx, false, 0);
    if (pt == nullptr) {
        return;
    }

    pt[pt_idx] = 0;
    invlpg(v_aligned);
}

uint32_t Paging::get_phys_addr(uint32_t virt_addr) {
    if (kernel_dir == nullptr) {
        return virt_addr;
    }

    uint32_t pd_idx = (virt_addr >> 22) & 0x3FFu;
    uint32_t pt_idx = (virt_addr >> 12) & 0x3FFu;
    uint32_t offset = virt_addr & 0xFFFu;

    uint32_t pde = kernel_dir[pd_idx];
    if (!(pde & FLAG_PRESENT)) {
        return 0;
    }

    const uint32_t* pt = reinterpret_cast<const uint32_t*>(pde & 0xFFFFF000u);
    uint32_t pte = pt[pt_idx];
    if (!(pte & FLAG_PRESENT)) {
        return 0;
    }

    return (pte & 0xFFFFF000u) | offset;
}

void Paging::init(uint32_t identity_map_bytes) {
    uint32_t pd_frame = Pmm::alloc_frame();
    if (pd_frame == 0) {
        Serial::print_line("[PAGING] Failed to allocate Page Directory frame!", Serial::COM1);
        return;
    }

    kernel_dir = reinterpret_cast<uint32_t*>(pd_frame);
    memset(kernel_dir, 0, PAGE_SIZE);

    uint32_t map_end = (identity_map_bytes + PAGE_SIZE - 1u) & ~(PAGE_SIZE - 1u);
    if (map_end < PAGE_SIZE) {
        map_end = PAGE_SIZE;
    }

    for (uint32_t addr = 0; addr < map_end; addr += PAGE_SIZE) {
        if (!map_page(addr, addr, FLAG_PRESENT | FLAG_RW)) {
            Serial::print_line("[PAGING] map_page failed during identity map!", Serial::COM1);
            return;
        }
    }

    load_cr3(pd_frame);
    enable_paging_bit();

    Serial::print("[PAGING] Enabled. CR3=", Serial::COM1);
    Serial::print_hex32(pd_frame, Serial::COM1);
    Serial::print(", identity mapped up to ", Serial::COM1);
    Serial::print_hex32(map_end, Serial::COM1);
    Serial::print_line("", Serial::COM1);
}