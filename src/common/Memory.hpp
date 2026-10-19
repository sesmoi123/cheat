#pragma once

#include <windows.h>

#include <cstdint>
#include <initializer_list>
#include <string>

namespace memory {

bool readable(uintptr_t address, size_t size);

template <typename T>
bool read(uintptr_t address, T& out) {
    if (!address) {
        return false;
    }
    __try {
        out = *reinterpret_cast<const T*>(address);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

template <typename T>
T read(uintptr_t address) {
    T value{};
    read(address, value);
    return value;
}

template <typename T>
bool write(uintptr_t address, const T& value) {
    if (!address) {
        return false;
    }
    __try {
        *reinterpret_cast<T*>(address) = value;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

// Suit une chaine de pointeurs : base + offset0 -> deref -> + offset1 -> ...
inline uintptr_t follow(uintptr_t base, std::initializer_list<uintptr_t> offsets) {
    uintptr_t address = base;
    for (uintptr_t offset : offsets) {
        if (!address) {
            return 0;
        }
        address = read<uintptr_t>(address + offset);
    }
    return address;
}

std::string read_string(uintptr_t address, size_t maxLength = 64);

uintptr_t module_base(const wchar_t* moduleName);
size_t module_size(const wchar_t* moduleName);

} // namespace memory
