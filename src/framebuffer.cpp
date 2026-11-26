#include "framebuffer.hpp"
#include "paging.hpp"
#include "pmm.hpp"
#include "memory.hpp"
#include "serial.hpp"

bool Framebuffer::ready = false;
uint32_t* Framebuffer::front_ptr = nullptr;
uint32_t* Framebuffer::back_ptr = nullptr;
uint32_t Framebuffer::fb_width = 0;
uint32_t Framebuffer::fb_height = 0;
uint32_t Framebuffer::fb_pitch = 0;
uint32_t Framebuffer::fb_size_bytes = 0;
uint8_t Framebuffer::fb_bpp = 0;

bool Framebuffer::init(const MultibootInfo* mb_info, uint32_t virt_base) {
    ready = false;
    if (mb_info == nullptr || !(mb_info->flags & MULTIBOOT_FLAG_FRAMEBUFFER)) {
        Serial::print_line("[FB] Missing Multiboot framebuffer info", Serial::COM1);
        return false;
    }

    if (mb_info->framebuffer_addr == 0 || mb_info->framebuffer_width == 0 || mb_info->framebuffer_height == 0) {
        Serial::print_line("[FB] Invalid framebuffer dimensions/address", Serial::COM1);
        return false;
    }

    if (mb_info->framebuffer_bpp != 32) {
        Serial::print_line("[FB] Only 32bpp is supported currently", Serial::COM1);
        return false;
    }

    uint32_t phys_addr = static_cast<uint32_t>(mb_info->framebuffer_addr);
    fb_width = mb_info->framebuffer_width;
    fb_height = mb_info->framebuffer_height;
    fb_pitch = mb_info->framebuffer_pitch;
    fb_bpp = mb_info->framebuffer_bpp;
    fb_size_bytes = fb_pitch * fb_height;

    Serial::print("[FB] mb_phys=0x", Serial::COM1);
    Serial::print_hex32(phys_addr, Serial::COM1);
    Serial::print(" w=", Serial::COM1);
    Serial::print_dec(static_cast<int32_t>(fb_width), Serial::COM1);
    Serial::print(" h=", Serial::COM1);
    Serial::print_dec(static_cast<int32_t>(fb_height), Serial::COM1);
    Serial::print(" pitch=", Serial::COM1);
    Serial::print_dec(static_cast<int32_t>(fb_pitch), Serial::COM1);
    Serial::print(" size=", Serial::COM1);
    Serial::print_dec(static_cast<int32_t>(fb_size_bytes), Serial::COM1);
    Serial::print_line("", Serial::COM1);

    uint32_t phys_aligned = phys_addr & ~0xFFFu;
    uint32_t page_offset = phys_addr & 0xFFFu;
    uint64_t total_span = static_cast<uint64_t>(page_offset) + fb_size_bytes;
    uint32_t pages_needed = static_cast<uint32_t>((total_span + Paging::PAGE_SIZE - 1u) / Paging::PAGE_SIZE);

    Serial::print_line("[FB] Mapping pages...", Serial::COM1);
    for (uint32_t i = 0; i < pages_needed; ++i) {
        uint32_t v_page = virt_base + i * Paging::PAGE_SIZE;
        uint32_t p_page = phys_aligned + i * Paging::PAGE_SIZE;
        if (!Paging::map_page(v_page, p_page, Paging::FLAG_PRESENT | Paging::FLAG_RW)) {
            Serial::print("[FB] Failed mapping framebuffer page idx=", Serial::COM1);
            Serial::print_dec(static_cast<int32_t>(i), Serial::COM1);
            Serial::print_line("", Serial::COM1);
            return false;
        }
    }

    front_ptr = reinterpret_cast<uint32_t*>(virt_base + page_offset);

    Serial::print_line("[FB] Allocating backbuffer...", Serial::COM1);
    uint32_t back_pages = (fb_size_bytes + Paging::PAGE_SIZE - 1u) / Paging::PAGE_SIZE;
    constexpr uint32_t BACKBUFFER_VIRT_BASE = 0xF8000000u;

    for (uint32_t i = 0; i < back_pages; ++i) {
        uint32_t frame = Pmm::alloc_frame();
        if (frame == 0) {
            Serial::print("[FB] PMM out of frames for backbuffer at page ", Serial::COM1);
            Serial::print_dec(static_cast<int32_t>(i), Serial::COM1);
            Serial::print(" / ", Serial::COM1);
            Serial::print_dec(static_cast<int32_t>(back_pages), Serial::COM1);
            Serial::print_line("", Serial::COM1);
            return false;
        }
        uint32_t v_addr = BACKBUFFER_VIRT_BASE + i * Paging::PAGE_SIZE;
        if (!Paging::map_page(v_addr, frame, Paging::FLAG_PRESENT | Paging::FLAG_RW)) {
            Serial::print("[FB] Failed mapping backbuffer page idx=", Serial::COM1);
            Serial::print_dec(static_cast<int32_t>(i), Serial::COM1);
            Serial::print_line("", Serial::COM1);
            return false;
        }
    }

    back_ptr = reinterpret_cast<uint32_t*>(BACKBUFFER_VIRT_BASE);
    memset(back_ptr, 0, fb_size_bytes);
    ready = true;

    Serial::print("[FB] Ready: ", Serial::COM1);
    Serial::print_dec(static_cast<int32_t>(fb_width), Serial::COM1);
    Serial::print("x", Serial::COM1);
    Serial::print_dec(static_cast<int32_t>(fb_height), Serial::COM1);
    Serial::print(" pitch=", Serial::COM1);
    Serial::print_dec(static_cast<int32_t>(fb_pitch), Serial::COM1);
    Serial::print(" backbuffer=", Serial::COM1);
    Serial::print_hex32(reinterpret_cast<uint32_t>(back_ptr), Serial::COM1);
    Serial::print(" front_phys=", Serial::COM1);
    Serial::print_hex32(Paging::get_phys_addr(reinterpret_cast<uint32_t>(front_ptr)), Serial::COM1);
    Serial::print(" back0_phys=", Serial::COM1);
    Serial::print_hex32(Paging::get_phys_addr(reinterpret_cast<uint32_t>(back_ptr)), Serial::COM1);
    Serial::print_line("", Serial::COM1);
    return true;
}

bool Framebuffer::is_ready() {
    return ready;
}

uint32_t Framebuffer::width() { return fb_width; }
uint32_t Framebuffer::height() { return fb_height; }
uint32_t Framebuffer::pitch() { return fb_pitch; }
uint8_t Framebuffer::bpp() { return fb_bpp; }
uint32_t* Framebuffer::backbuffer() { return back_ptr; }

void Framebuffer::clear(uint32_t color) {
    fill_rect(0, 0, static_cast<int32_t>(fb_width), static_cast<int32_t>(fb_height), color);
}

void Framebuffer::put_pixel(int32_t x, int32_t y, uint32_t color) {
    if (!ready || x < 0 || y < 0) {
        return;
    }
    uint32_t ux = static_cast<uint32_t>(x);
    uint32_t uy = static_cast<uint32_t>(y);
    if (ux >= fb_width || uy >= fb_height) {
        return;
    }

    uint8_t* row = reinterpret_cast<uint8_t*>(back_ptr) + static_cast<uint64_t>(uy) * fb_pitch;
    reinterpret_cast<uint32_t*>(row)[ux] = color;
}

void Framebuffer::fill_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
    if (!ready || w <= 0 || h <= 0) {
        return;
    }

    int32_t x0 = x < 0 ? 0 : x;
    int32_t y0 = y < 0 ? 0 : y;
    int32_t x1 = x + w;
    int32_t y1 = y + h;

    if (x1 > static_cast<int32_t>(fb_width)) {
        x1 = static_cast<int32_t>(fb_width);
    }
    if (y1 > static_cast<int32_t>(fb_height)) {
        y1 = static_cast<int32_t>(fb_height);
    }
    if (x0 >= x1 || y0 >= y1) {
        return;
    }

    for (int32_t py = y0; py < y1; ++py) {
        uint8_t* row = reinterpret_cast<uint8_t*>(back_ptr) + static_cast<uint64_t>(py) * fb_pitch;
        uint32_t* pixels = reinterpret_cast<uint32_t*>(row);
        for (int32_t px = x0; px < x1; ++px) {
            pixels[px] = color;
        }
    }
}

void Framebuffer::swap_buffers() {
    if (!ready || front_ptr == nullptr || back_ptr == nullptr) {
        Serial::print_line("[FB] swap_buffers skipped: not ready", Serial::COM1);
        return;
    }
    uint32_t* dst = front_ptr;
    const uint32_t* src = back_ptr;
    uint32_t dwords = fb_size_bytes / sizeof(uint32_t);
    for (uint32_t i = 0; i < dwords; ++i) {
        dst[i] = src[i];
    }
}

