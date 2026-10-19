#pragma once

#include <windows.h>

#include <string>

namespace resources {

size_t embedded_dll_size();
bool extract_cheat_dll(const std::wstring& destination, std::string& error);

} // namespace resources
