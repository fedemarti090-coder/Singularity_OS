#include "memory.hpp"

extern "C" void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = static_cast<uint8_t*>(dest);
    const uint8_t* s = static_cast<const uint8_t*>(src);
    for (size_t i = 0; i < n; ++i) {
        d[i] = s[i];
    }
    return dest;
}

extern "C" void* memmove(void* dest, const void* src, size_t n) {
    uint8_t* d = static_cast<uint8_t*>(dest);
    const uint8_t* s = static_cast<const uint8_t*>(src);
    if (d == s || n == 0) {
        return dest;
    }
    if (d < s) {
        for (size_t i = 0; i < n; ++i) {
            d[i] = s[i];
        }
    } else {
        for (size_t i = n; i > 0; --i) {
            d[i - 1] = s[i - 1];
        }
    }
    return dest;
}

extern "C" void* memset(void* dest, int val, size_t n) {
    uint8_t* d = static_cast<uint8_t*>(dest);
    uint8_t b = static_cast<uint8_t>(val);
    for (size_t i = 0; i < n; ++i) {
        d[i] = b;
    }
    return dest;
}

extern "C" int memcmp(const void* s1, const void* s2, size_t n) {
    const uint8_t* a = static_cast<const uint8_t*>(s1);
    const uint8_t* b = static_cast<const uint8_t*>(s2);
    for (size_t i = 0; i < n; ++i) {
        if (a[i] != b[i]) {
            return static_cast<int>(a[i]) - static_cast<int>(b[i]);
        }
    }
    return 0;
}

extern "C" size_t strlen(const char* s) {
    size_t len = 0;
    while (s[len] != '\0') {
        ++len;
    }
    return len;
}

extern "C" int strcmp(const char* s1, const char* s2) {
    size_t i = 0;
    while (s1[i] != '\0' && s1[i] == s2[i]) {
        ++i;
    }
    return static_cast<uint8_t>(s1[i]) - static_cast<uint8_t>(s2[i]);
}

extern "C" int strncmp(const char* s1, const char* s2, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        uint8_t c1 = static_cast<uint8_t>(s1[i]);
        uint8_t c2 = static_cast<uint8_t>(s2[i]);
        if (c1 != c2) {
            return static_cast<int>(c1) - static_cast<int>(c2);
        }
        if (c1 == '\0') {
            return 0;
        }
    }
    return 0;
}