#pragma once

#include <windows.h>

#include <cstdint>
#include <cstring>

namespace mem {

inline uintptr_t module_base(const char* moduleName = nullptr) {
    return reinterpret_cast<uintptr_t>(GetModuleHandleA(moduleName));
}

inline bool valid(uintptr_t address, size_t size = 1) {
    if (!address) {
        return false;
    }
    MEMORY_BASIC_INFORMATION info{};
    if (!VirtualQuery(reinterpret_cast<LPCVOID>(address), &info, sizeof(info))) {
        return false;
    }
    if (info.State != MEM_COMMIT) {
        return false;
    }
    const DWORD protect = info.Protect;
    if ((protect & PAGE_NOACCESS) || (protect & PAGE_GUARD)) {
        return false;
    }
    return size <= info.RegionSize;
}

inline bool read_bytes(uintptr_t address, void* out, size_t size) {
    if (!address || !out || !size) {
        return false;
    }
    __try {
        std::memcpy(out, reinterpret_cast<const void*>(address), size);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

inline bool write_bytes(uintptr_t address, const void* data, size_t size) {
    if (!address || !data || !size) {
        return false;
    }
    __try {
        std::memcpy(reinterpret_cast<void*>(address), data, size);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

template <typename T>
inline bool read_safe(uintptr_t address, T& out) {
    return read_bytes(address, &out, sizeof(T));
}

template <typename T>
inline T read(uintptr_t address) {
    T value{};
    read_safe(address, value);
    return value;
}

template <typename T>
inline bool write(uintptr_t address, const T& value) {
    return write_bytes(address, &value, sizeof(T));
}

inline uintptr_t read_ptr(uintptr_t address) {
    return read<uintptr_t>(address);
}

template <typename... Offsets>
inline uintptr_t chain(uintptr_t base, Offsets... offsets) {
    uintptr_t address = base;
    ((address = address ? read_ptr(address + offsets) : 0), ...);
    return address;
}

} // namespace mem
