#include "initrd.hpp"
#include "paging.hpp"
#include "memory.hpp"
#include "serial.hpp"

namespace {

constexpr size_t TAR_BLOCK_SIZE = 512;

struct TarHeader {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char pad[12];
};

static_assert(sizeof(TarHeader) == TAR_BLOCK_SIZE, "TarHeader must be 512 bytes");

struct MultibootModule {
    uint32_t mod_start;
    uint32_t mod_end;
    uint32_t string;
    uint32_t reserved;
};

bool str_eq(const char* a, const char* b) {
    if (a == nullptr || b == nullptr) {
        return false;
    }
    while (*a != '\0' && *b != '\0') {
        if (*a != *b) {
            return false;
        }
        ++a;
        ++b;
    }
    return *a == *b;
}

bool is_all_zero(const uint8_t* ptr, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        if (ptr[i] != 0) {
            return false;
        }
    }
    return true;
}

} // namespace

InitRdEntry InitRd::entries[InitRd::MAX_ENTRIES];
size_t InitRd::entry_count = 0;
bool InitRd::ready = false;

uint64_t InitRd::parse_octal(const char* buf, size_t len) {
    uint64_t val = 0;
    for (size_t i = 0; i < len; ++i) {
        char c = buf[i];
        if (c == '\0' || c == ' ') {
            if (val != 0) {
                break;
            }
            continue;
        }
        if (c < '0' || c > '7') {
            break;
        }
        val = (val << 3) | static_cast<uint64_t>(c - '0');
    }
    return val;
}

const char* InitRd::normalize_path(const char* raw) {
    if (raw == nullptr) {
        return "";
    }
    if (raw[0] == '.' && raw[1] == '/') {
        raw += 2;
    }
    if (raw[0] == '/') {
        raw += 1;
    }
    return raw;
}

bool InitRd::init(const MultibootInfo* mb_info) {
    entry_count = 0;
    ready = false;

    if (mb_info == nullptr) {
        Serial::print_line("[INITRD] mb_info is null", Serial::COM1);
        return false;
    }

    const uint32_t flags = *reinterpret_cast<const uint32_t*>(mb_info);
    constexpr uint32_t MB_FLAG_MODS = (1u << 3);
    if ((flags & MB_FLAG_MODS) == 0) {
        Serial::print_line("[INITRD] No Multiboot module flag present", Serial::COM1);
        return false;
    }

    const auto* raw_bytes = reinterpret_cast<const uint8_t*>(mb_info);
    const uint32_t mods_count = *reinterpret_cast<const uint32_t*>(raw_bytes + 20);
    const uint32_t mods_addr = *reinterpret_cast<const uint32_t*>(raw_bytes + 24);

    if (mods_count == 0 || mods_addr == 0) {
        Serial::print_line("[INITRD] Empty module list", Serial::COM1);
        return false;
    }

    const auto* mods = reinterpret_cast<const MultibootModule*>(mods_addr);
    const uint32_t start = mods[0].mod_start;
    const uint32_t end = mods[0].mod_end;

    Serial::print("[INITRD] mod_start=", Serial::COM1);
    Serial::print_hex32(start, Serial::COM1);
    Serial::print(" mod_end=", Serial::COM1);
    Serial::print_hex32(end, Serial::COM1);
    Serial::print(" phys_start=", Serial::COM1);
    Serial::print_hex32(Paging::get_phys_addr(start), Serial::COM1);
    Serial::print(" phys_end=", Serial::COM1);
    Serial::print_hex32(Paging::get_phys_addr(end - 1u), Serial::COM1);
    Serial::print_line("", Serial::COM1);

    if (end <= start) {
        Serial::print_line("[INITRD] Invalid module range", Serial::COM1);
        return false;
    }

    const size_t total_size = static_cast<size_t>(end - start);
    const auto* archive = reinterpret_cast<const uint8_t*>(start);
    size_t offset = 0;

    while (offset + TAR_BLOCK_SIZE <= total_size && entry_count < MAX_ENTRIES) {
        const auto* hdr = reinterpret_cast<const TarHeader*>(archive + offset);
        if (is_all_zero(reinterpret_cast<const uint8_t*>(hdr), TAR_BLOCK_SIZE)) {
            break;
        }

        if (strncmp(hdr->magic, "ustar", 5) != 0) {
            Serial::print_line("[INITRD] Stopping parse: non-ustar block reached", Serial::COM1);
            break;
        }

        const size_t file_size = static_cast<size_t>(parse_octal(hdr->size, sizeof(hdr->size)));
        const uint8_t* data_ptr = archive + offset + TAR_BLOCK_SIZE;
        const size_t data_blocks = (file_size + TAR_BLOCK_SIZE - 1u) / TAR_BLOCK_SIZE;
        const size_t next_offset = offset + TAR_BLOCK_SIZE + data_blocks * TAR_BLOCK_SIZE;

        if (next_offset > total_size || next_offset < offset) {
            Serial::print_line("[INITRD] Truncated tar entry, stopping", Serial::COM1);
            break;
        }

        static char name_storage[MAX_ENTRIES][128];
        size_t pos = 0;
        for (size_t i = 0; i < sizeof(hdr->name) && hdr->name[i] != '\0'; ++i) {
            char c = hdr->name[i];
            if (c >= 32 && c < 127 && pos + 1 < sizeof(name_storage[entry_count])) {
                name_storage[entry_count][pos++] = c;
            }
        }
        name_storage[entry_count][pos] = '\0';

        const char* clean_name = normalize_path(name_storage[entry_count]);
        if (clean_name[0] != '\0') {
            bool dir = (hdr->typeflag == '5');
            size_t name_len = strlen(clean_name);
            if (name_len > 0 && clean_name[name_len - 1] == '/') {
                dir = true;
            }
            entries[entry_count].name = clean_name;
            entries[entry_count].data = data_ptr;
            entries[entry_count].size = file_size;
            entries[entry_count].is_dir = dir;
            ++entry_count;
        }

        offset = next_offset;
    }

    ready = true;
    Serial::print("[INITRD] Loaded entries: ", Serial::COM1);
    Serial::print_dec(static_cast<int32_t>(entry_count), Serial::COM1);
    Serial::print_line("", Serial::COM1);
    return true;
}

bool InitRd::is_ready() {
    return ready;
}

size_t InitRd::count() {
    return entry_count;
}

const InitRdEntry* InitRd::get_at(size_t index) {
    if (!ready || index >= entry_count) {
        return nullptr;
    }
    return &entries[index];
}

static const char* strip_leading_dots_slashes(const char* s) {
    if (s == nullptr) {
        return "";
    }
    while (true) {
        if (s[0] == '.' && s[1] == '/') {
            s += 2;
        } else if (s[0] == '/') {
            s += 1;
        } else {
            break;
        }
    }
    return s;
}

const InitRdEntry* InitRd::find(const char* path) {
    if (!ready || path == nullptr) {
        return nullptr;
    }
    const char* target = strip_leading_dots_slashes(normalize_path(path));
    for (size_t i = 0; i < entry_count; ++i) {
        const char* entry_name = strip_leading_dots_slashes(entries[i].name);
        if (str_eq(entry_name, target)) {
            return &entries[i];
        }
    }
    return nullptr;
}

int32_t InitRd::read(const char* path, uint8_t* dst, size_t max_len, size_t offset) {
    const InitRdEntry* entry = find(path);
    if (entry == nullptr || entry->is_dir || dst == nullptr || max_len == 0) {
        return -1;
    }
    if (offset >= entry->size) {
        return 0;
    }
    size_t avail = entry->size - offset;
    size_t to_copy = (avail < max_len) ? avail : max_len;
    memcpy(dst, entry->data + offset, to_copy);
    return static_cast<int32_t>(to_copy);
}