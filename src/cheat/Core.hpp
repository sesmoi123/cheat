#pragma once

#include <windows.h>

#include <string>

namespace core {

void start(HMODULE module);
void stop();

HMODULE module();
std::wstring dll_directory();

} // namespace core
