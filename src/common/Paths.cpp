#include "Paths.hpp"

#include <filesystem>

namespace paths {

std::wstring module_directory(HMODULE module) {
    wchar_t buffer[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(module, buffer, MAX_PATH);
    if (!length) {
        return L".";
    }

    std::wstring path(buffer, length);
    const size_t position = path.find_last_of(L"\\/");
    return position == std::wstring::npos ? std::wstring(L".") : path.substr(0, position);
}

std::wstring local_app_data() {
    wchar_t buffer[MAX_PATH]{};
    const DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", buffer, MAX_PATH);
    return length ? std::wstring(buffer, length) : std::wstring(L".");
}

std::wstring data_directory() {
    std::wstring directory = local_app_data() + L"\\RetracOGFN";
    std::error_code ec;
    std::filesystem::create_directories(directory, ec);
    return directory;
}

std::string to_narrow(const std::wstring& value) {
    if (value.empty()) {
        return {};
    }
    const int size = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0) {
        return {};
    }
    std::string result(static_cast<size_t>(size - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, result.data(), size, nullptr, nullptr);
    return result;
}

std::wstring to_wide(const std::string& value) {
    if (value.empty()) {
        return {};
    }
    const int size = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
    if (size <= 0) {
        return {};
    }
    std::wstring result(static_cast<size_t>(size - 1), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, result.data(), size);
    return result;
}

bool file_exists(const std::wstring& path) {
    return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

} // namespace paths
