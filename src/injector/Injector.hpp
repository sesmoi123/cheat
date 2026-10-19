#pragma once

#include <windows.h>

#include <string>

namespace injector {

struct ProcessInfo {
    DWORD pid = 0;
    std::wstring name;
    std::wstring path;
};

DWORD find_process_id(const wchar_t* exeName);

bool wait_for_process(const wchar_t* exeName, DWORD timeoutMs, DWORD& outPid);

bool inject_library(DWORD pid, const std::wstring& dllPath, std::string& error);

std::wstring module_directory(HMODULE module);

} // namespace injector
