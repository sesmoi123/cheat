#pragma once

#include <windows.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace pattern {

struct Signature {
    std::vector<uint8_t> bytes;
    std::vector<uint8_t> mask;
};

inline Signature parse(const char* text) {
    Signature signature;
    std::string token;

    for (const char* cursor = text;; ++cursor) {
        const char character = *cursor;
        if (character == ' ' || character == '\t' || character == '\0') {
            if (!token.empty()) {
                if (token == "?" || token == "??" || token == "**") {
                    signature.bytes.push_back(0x00);
                    signature.mask.push_back(0x00);
                } else {
                    signature.bytes.push_back(static_cast<uint8_t>(std::strtoul(token.c_str(), nullptr, 16)));
                    signature.mask.push_back(0xFF);
                }
                token.clear();
            }
            if (character == '\0') {
                break;
            }
        } else {
            token.push_back(character);
        }
    }

    return signature;
}

inline uintptr_t find_in_range(const Signature& signature, uintptr_t start, size_t size) {
    if (signature.bytes.empty() || size < signature.bytes.size()) {
        return 0;
    }

    const auto* data = reinterpret_cast<const uint8_t*>(start);
    const size_t last = size - signature.bytes.size();

    for (size_t i = 0; i <= last; ++i) {
        bool matched = true;
        for (size_t j = 0; j < signature.bytes.size(); ++j) {
            if (signature.mask[j] && data[i + j] != signature.bytes[j]) {
                matched = false;
                break;
            }
        }
        if (matched) {
            return start + i;
        }
    }

    return 0;
}

inline uintptr_t find_module(const char* text, const char* moduleName = nullptr) {
    const HMODULE module = GetModuleHandleA(moduleName);
    if (!module) {
        return 0;
    }

    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(module);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        return 0;
    }

    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(
        reinterpret_cast<uintptr_t>(module) + static_cast<uintptr_t>(dos->e_lfanew));
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        return 0;
    }

    const Signature signature = parse(text);
    const auto* section = IMAGE_FIRST_SECTION(nt);

    for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i) {
        if (!(section[i].Characteristics & IMAGE_SCN_MEM_EXECUTE)) {
            continue;
        }
        const uintptr_t base = reinterpret_cast<uintptr_t>(module) + section[i].VirtualAddress;
        const size_t size = section[i].Misc.VirtualSize ? section[i].Misc.VirtualSize
                                                        : section[i].SizeOfRawData;
        const uintptr_t hit = find_in_range(signature, base, size);
        if (hit) {
            return hit;
        }
    }

    return 0;
}

inline uintptr_t resolve_relative(uintptr_t instruction, int offsetPosition, int instructionSize) {
    if (!instruction) {
        return 0;
    }
    const auto relative = *reinterpret_cast<const int32_t*>(instruction + offsetPosition);
    return instruction + instructionSize + relative;
}

} // namespace pattern
