#include "Memory.hpp"

namespace memory {

bool readable(uintptr_t address, size_t size) {
    if (!address || !size) {
        return false;
    }

    MEMORY_BASIC_INFORMATION info{};
    if (VirtualQuery(reinterpret_cast<LPCVOID>(address), &info, sizeof(info)) == 0) {
        return false;
    }
    if (info.State != MEM_COMMIT) {
        return false;
    }
    if (info.Protect & PAGE_NOACCESS || info.Protect & PAGE_GUARD) {
        return false;
    }

    const uintptr_t regionEnd = reinterpret_cast<uintptr_t>(info.BaseAddress) + info.RegionSize;
    return address + size <= regionEnd;
}

std::string read_string(uintptr_t address, size_t maxLength) {
    if (!address) {
        return {};
    }

    std::string result;
    result.reserve(maxLength);

    for (size_t index = 0; index < maxLength; ++index) {
        const char character = read<char>(address + index);
        if (!character) {
            break;
        }
        result.push_back(character);
    }

    return result;
}

uintptr_t module_base(const wchar_t* moduleName) {
    return reinterpret_cast<uintptr_t>(GetModuleHandleW(moduleName));
}

size_t module_size(const wchar_t* moduleName) {
    const uintptr_t base = module_base(moduleName);
    if (!base) {
        return 0;
    }

    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        return 0;
    }

    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        return 0;
    }

    return nt->OptionalHeader.SizeOfImage;
}

} // namespace memory
