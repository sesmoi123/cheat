#include "Scanner.hpp"

#include "Memory.hpp"

#include <string>
#include <vector>

namespace scanner {

namespace {

bool parse_pattern(const char* pattern, std::vector<uint8_t>& bytes, std::vector<bool>& mask) {
    if (!pattern) {
        return false;
    }

    const std::string text(pattern);
    std::string token;

    for (size_t index = 0; index <= text.size(); ++index) {
        const char character = index < text.size() ? text[index] : ' ';
        if (character != ' ') {
            token.push_back(character);
            continue;
        }
        if (token.empty()) {
            continue;
        }

        if (token == "?" || token == "??") {
            bytes.push_back(0);
            mask.push_back(false);
        } else {
            bytes.push_back(static_cast<uint8_t>(std::stoul(token, nullptr, 16)));
            mask.push_back(true);
        }
        token.clear();
    }

    return !bytes.empty();
}

} // namespace

uintptr_t find_pattern(uintptr_t start, size_t size, const char* pattern) {
    std::vector<uint8_t> bytes;
    std::vector<bool> mask;
    if (!parse_pattern(pattern, bytes, mask)) {
        return 0;
    }

    const size_t count = bytes.size();
    size_t offset = 0;

    while (offset + count <= size) {
        if (!memory::readable(start + offset, count)) {
            offset += 0x1000 - (offset & 0xFFF);
            continue;
        }

        const size_t pageEnd = (offset & ~static_cast<size_t>(0xFFF)) + 0x1000;
        for (; offset + count <= size && offset < pageEnd; ++offset) {
            bool matched = true;
            for (size_t index = 0; index < count; ++index) {
                if (!mask[index]) {
                    continue;
                }
                const auto value = *reinterpret_cast<const uint8_t*>(start + offset + index);
                if (value != bytes[index]) {
                    matched = false;
                    break;
                }
            }
            if (matched) {
                return start + offset;
            }
        }
    }

    return 0;
}

uintptr_t find_pattern(const wchar_t* moduleName, const char* pattern) {
    const uintptr_t base = memory::module_base(moduleName);
    const size_t size = memory::module_size(moduleName);
    if (!base || !size) {
        return 0;
    }
    return find_pattern(base, size, pattern);
}

} // namespace scanner
