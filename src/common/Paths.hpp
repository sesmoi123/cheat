#pragma once

#include <windows.h>

#include <string>

namespace paths {

std::wstring module_directory(HMODULE module);
std::wstring local_app_data();
std::wstring data_directory();

std::string to_narrow(const std::wstring& value);
std::wstring to_wide(const std::string& value);

bool file_exists(const std::wstring& path);

} // namespace paths
