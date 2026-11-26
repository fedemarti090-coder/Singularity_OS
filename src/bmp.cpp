#include "bmp.hpp"
#include "initrd.hpp"
#include "framebuffer.hpp"
#include "paging.hpp"
#include "serial.hpp"

namespace {

#pragma pack(push, 1)
struct BmpFileHeader {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
};

struct BmpInfoHeader {
    uint32_t biSize;
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
};
#pragma pack(pop)

void log_addr_info(const char* label, const void* ptr) {
    uint32_t v = reinterpret_cast<uint32_t>(ptr);
    uint32_t p = Paging::get_phys_addr(v);
    Serial::print("[DBG] ", Serial::COM1);
    Serial::print(label, Serial::COM1);
    Serial::print(" virt=", Serial::COM1);
    Serial::print_hex32(v, Serial::COM1);
    Serial::print(" phys=", Serial::COM1);
    Serial::print_hex32(p, Serial::COM1);
    Serial::print_line("", Serial::COM1);
}

} // namespace

bool Wallpaper::loaded = false;
uint32_t Wallpaper::img_width = 0;
uint32_t Wallpaper::img_height = 0;
uint32_t* Wallpaper::pixel_buf = nullptr;

bool Wallpaper::load_from_initrd(const char* path) {
    loaded = false;
    img_width = 0;
    img_height = 0;
    pixel_buf = nullptr;

    const InitRdEntry* entry = InitRd::find(path);
    if (entry == nullptr || entry->is_dir || entry->data == nullptr) {
        Serial::print("[BMP] File not found in InitRd: ", Serial::COM1);
        Serial::print_line(path ? path : "(null)", Serial::COM1);
        return false;
    }

    Serial::print("[BMP] entry.size=", Serial::COM1);
    Serial::print_dec(static_cast<int32_t>(entry->size), Serial::COM1);
    Serial::print_line("", Serial::COM1);
    log_addr_info("bmp_raw", entry->data);
    log_addr_info("bmp_end", entry->data + entry->size - 1u);

    if (entry->size < sizeof(BmpFileHeader) + sizeof(BmpInfoHeader)) {
        Serial::print_line("[BMP] File too small for BMP headers", Serial::COM1);
        return false;
    }

    const uint8_t* raw = entry->data;
    const auto* fh = reinterpret_cast<const BmpFileHeader*>(raw);
    const auto* ih = reinterpret_cast<const BmpInfoHeader*>(raw + sizeof(BmpFileHeader));

    Serial::print("[BMP] bfType=", Serial::COM1);
    Serial::print_hex32(fh->bfType, Serial::COM1);
    Serial::print(" bfSize=", Serial::COM1);
    Serial::print_dec(static_cast<int32_t>(fh->bfSize), Serial::COM1);
    Serial::print(" bfOffBits=", Serial::COM1);
    Serial::print_dec(static_cast<int32_t>(fh->bfOffBits), Serial::COM1);
    Serial::print(" w=", Serial::COM1);
    Serial::print_dec(ih->biWidth, Serial::COM1);
    Serial::print(" h=", Serial::COM1);
    Serial::print_dec(ih->biHeight, Serial::COM1);
    Serial::print(" bpp=", Serial::COM1);
    Serial::print_dec(ih->biBitCount, Serial::COM1);
    Serial::print(" comp=", Serial::COM1);
    Serial::print_dec(static_cast<int32_t>(ih->biCompression), Serial::COM1);
    Serial::print_line("", Serial::COM1);

    if (fh->bfType != 0x4D42u || ih->biCompression != 0) {
        Serial::print_line("[BMP] Unsupported BMP format", Serial::COM1);
        return false;
    }

    if (ih->biBitCount != 24 && ih->biBitCount != 32) {
        Serial::print_line("[BMP] Only 24-bit and 32-bit BMP supported", Serial::COM1);
        return false;
    }

    if (ih->biWidth <= 0 || ih->biHeight == 0) {
        Serial::print_line("[BMP] Invalid BMP dimensions", Serial::COM1);
        return false;
    }

    bool top_down = (ih->biHeight < 0);
    uint32_t w = static_cast<uint32_t>(ih->biWidth);
    uint32_t h = static_cast<uint32_t>(top_down ? -ih->biHeight : ih->biHeight);
    uint32_t bpp = ih->biBitCount / 8u;
    uint64_t row_stride = ((static_cast<uint64_t>(w) * bpp + 3u) / 4u) * 4u;

    uint64_t required_end = static_cast<uint64_t>(fh->bfOffBits) + row_stride * h;
    if (required_end > entry->size) {
        Serial::print_line("[BMP] Truncated pixel data in InitRd", Serial::COM1);
        return false;
    }

    img_width = w;
    img_height = h;
    loaded = true;

    const uint8_t* px = raw + fh->bfOffBits;
    Serial::print("[BMP] px_start bytes: ", Serial::COM1);
    for (int i = 0; i < 12; ++i) {
        Serial::print_hex32(px[i], Serial::COM1);
        Serial::print(" ", Serial::COM1);
    }
    Serial::print_line("", Serial::COM1);

    const uint8_t* mid_px = px + (row_stride * (h / 2u));
    Serial::print("[BMP] mid_row bytes: ", Serial::COM1);
    for (int i = 0; i < 12; ++i) {
        Serial::print_hex32(mid_px[i], Serial::COM1);
        Serial::print(" ", Serial::COM1);
    }
    Serial::print_line("", Serial::COM1);

    const uint8_t* last_px = px + (row_stride * (h - 1u));
    Serial::print("[BMP] last_row bytes: ", Serial::COM1);
    for (int i = 0; i < 12; ++i) {
        Serial::print_hex32(last_px[i], Serial::COM1);
        Serial::print(" ", Serial::COM1);
    }
    Serial::print_line("", Serial::COM1);

    return true;
}

void Wallpaper::draw_to_backbuffer() {
    if (!loaded || !Framebuffer::is_ready()) {
        Serial::print_line("[BMP] draw skipped: not loaded or FB not ready", Serial::COM1);
        return;
    }

    const InitRdEntry* entry = InitRd::find("wallpaper.bmp");
    if (entry == nullptr || entry->data == nullptr) {
        Serial::print_line("[BMP] draw skipped: entry missing", Serial::COM1);
        return;
    }

    const uint8_t* raw = entry->data;
    const auto* fh = reinterpret_cast<const BmpFileHeader*>(raw);
    const auto* ih = reinterpret_cast<const BmpInfoHeader*>(raw + sizeof(BmpFileHeader));

    bool top_down = (ih->biHeight < 0);
    uint32_t w = img_width;
    uint32_t h = img_height;
    uint32_t bpp = ih->biBitCount / 8u;
    uint64_t row_stride = ((static_cast<uint64_t>(w) * bpp + 3u) / 4u) * 4u;
    const uint8_t* pixels = raw + fh->bfOffBits;

    uint32_t max_x = (w < Framebuffer::width()) ? w : Framebuffer::width();
    uint32_t max_y = (h < Framebuffer::height()) ? h : Framebuffer::height();

    for (uint32_t y = 0; y < max_y; ++y) {
        uint64_t src_y = top_down ? y : ((h - 1u) - y);
        const uint8_t* src_row = pixels + src_y * row_stride;

        for (uint32_t x = 0; x < max_x; ++x) {
            uint64_t p = static_cast<uint64_t>(x) * bpp;
            uint8_t b = src_row[p + 0u];
            uint8_t g = src_row[p + 1u];
            uint8_t r = (bpp >= 3u) ? src_row[p + 2u] : 0u;
            Framebuffer::put_pixel(static_cast<int32_t>(x), static_cast<int32_t>(y), Framebuffer::rgb(r, g, b));
        }
    }

    uint32_t* bb = Framebuffer::backbuffer();
    Serial::print("[BMP] backbuffer[0..3]: ", Serial::COM1);
    for (int i = 0; i < 4; ++i) {
        Serial::print_hex32(bb[i], Serial::COM1);
        Serial::print(" ", Serial::COM1);
    }
    Serial::print_line("", Serial::COM1);
}

bool Wallpaper::is_loaded() {
    return loaded;
}

uint32_t Wallpaper::width() {
    return img_width;
}

uint32_t Wallpaper::height() {
    return img_height;
}