#pragma once

#include <windows.h>

#include <cstdint>

namespace scanner {

// Motif style Cheat Engine : "48 8B 05 ?? ?? ?? ?? 48 85 C0".
uintptr_t find_pattern(uintptr_t start, size_t size, const char* pattern);
uintptr_t find_pattern(const wchar_t* moduleName, const char* pattern);

} // namespace scanner
