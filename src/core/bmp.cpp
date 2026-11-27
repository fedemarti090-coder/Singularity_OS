#include "bmp.hpp"
#include "initrd.hpp"
#include "framebuffer.hpp"
#include "memory.hpp"
#include "serial.hpp"

namespace {
    constexpr uint32_t MAX_WALLPAPER_W = 1600;
    constexpr uint32_t MAX_WALLPAPER_H = 900;
    constexpr size_t MAX_WALLPAPER_PIXELS = static_cast<size_t>(MAX_WALLPAPER_W) * MAX_WALLPAPER_H;

    alignas(16) uint32_t g_decoded_buf[MAX_WALLPAPER_PIXELS] = {0};
    uint32_t g_width = 0;
    uint32_t g_height = 0;
    bool g_loaded = false;

    uint16_t read_u16_le(const uint8_t* p) {
        return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
    }

    uint32_t read_u32_le(const uint8_t* p) {
        return static_cast<uint32_t>(p[0]) |
               (static_cast<uint32_t>(p[1]) << 8) |
               (static_cast<uint32_t>(p[2]) << 16) |
               (static_cast<uint32_t>(p[3]) << 24);
    }
}

namespace Wallpaper {

void free_decoded() {
    g_width = 0;
    g_height = 0;
    g_loaded = false;
}

bool load_from_initrd(const char* path) {
    free_decoded();

    const InitRdEntry* entry = InitRd::find(path);
    if (entry == nullptr || entry->is_dir || entry->data == nullptr || entry->size < 54) {
        Serial::print_line("[BMP] File non trovato o troppo piccolo", Serial::COM1);
        return false;
    }

    const uint8_t* data = reinterpret_cast<const uint8_t*>(entry->data);
    uint16_t bf_type = read_u16_le(data + 0);
    uint32_t data_offset = read_u32_le(data + 10);
    int32_t signed_w = static_cast<int32_t>(read_u32_le(data + 18));
    int32_t signed_h = static_cast<int32_t>(read_u32_le(data + 22));
    uint16_t bpp = read_u16_le(data + 28);
    uint32_t compression = read_u32_le(data + 30);

    if (bf_type != 0x4D42 || compression != 0 || (bpp != 24 && bpp != 32) || signed_w <= 0 || signed_h == 0) {
        Serial::print_line("[BMP] Header non supportato", Serial::COM1);
        return false;
    }

    bool top_down = (signed_h < 0);
    uint32_t w = static_cast<uint32_t>(signed_w);
    uint32_t h = static_cast<uint32_t>(top_down ? -signed_h : signed_h);

    if (w > MAX_WALLPAPER_W || h > MAX_WALLPAPER_H) {
        Serial::print_line("[BMP] Dimensioni oltre il limite del buffer", Serial::COM1);
        return false;
    }

    uint32_t bytes_per_pixel = bpp / 8;
    uint32_t row_stride = ((w * bytes_per_pixel + 3u) / 4u) * 4u;

    uint64_t required_bytes = static_cast<uint64_t>(data_offset) + static_cast<uint64_t>(row_stride) * h;
    if (required_bytes > entry->size) {
        Serial::print_line("[BMP] Dati pixel troncati", Serial::COM1);
        return false;
    }

    const uint8_t* pixel_base = data + data_offset;
    for (uint32_t y = 0; y < h; ++y) {
        uint32_t src_row = top_down ? y : ((h - 1u) - y);
        const uint8_t* row_ptr = pixel_base + static_cast<uint64_t>(src_row) * row_stride;

        for (uint32_t x = 0; x < w; ++x) {
            uint8_t b = row_ptr[x * bytes_per_pixel + 0];
            uint8_t g = row_ptr[x * bytes_per_pixel + 1];
            uint8_t r = row_ptr[x * bytes_per_pixel + 2];
            g_decoded_buf[static_cast<size_t>(y) * w + x] = Framebuffer::rgb(r, g, b);
        }
    }

    g_width = w;
    g_height = h;
    g_loaded = true;
    Serial::print_line("[BMP] Decodificato in buffer statico", Serial::COM1);
    return true;
}

bool is_loaded() {
    return g_loaded;
}

void draw_to_backbuffer() {
    if (!is_loaded()) {
        return;
    }

    uint32_t* back = Framebuffer::backbuffer();
    uint32_t pitch_pixels = Framebuffer::pitch() / sizeof(uint32_t);
    uint32_t fb_w = Framebuffer::width();
    uint32_t fb_h = Framebuffer::height();

    uint32_t copy_w = (g_width < fb_w) ? g_width : fb_w;
    uint32_t copy_h = (g_height < fb_h) ? g_height : fb_h;

    for (uint32_t y = 0; y < copy_h; ++y) {
        const uint32_t* src_line = g_decoded_buf + static_cast<size_t>(y) * g_width;
        uint32_t* dst_line = back + static_cast<size_t>(y) * pitch_pixels;
        memcpy(dst_line, src_line, static_cast<size_t>(copy_w) * sizeof(uint32_t));
    }
}

}