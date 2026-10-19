#pragma once

#include <windows.h>

#include <string>
#include <vector>

namespace injector {

struct Target {
    DWORD pid = 0;
    std::wstring name;
};

std::vector<Target> find_fortnite_processes();
bool wait_for_fortnite(DWORD timeoutMs, std::vector<Target>& out);
bool is_process_alive(DWORD pid);

bool inject_library(DWORD pid, const std::wstring& dllPath, std::string& error);

} // namespace injector
