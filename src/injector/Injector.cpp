#include "Injector.hpp"

#include <tlhelp32.h>

#include <chrono>
#include <cwctype>
#include <thread>
#include <vector>

namespace injector {

namespace {

std::wstring lower(std::wstring value) {
    for (wchar_t& character : value) {
        character = static_cast<wchar_t>(::towlower(character));
    }
    return value;
}

} // namespace

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

DWORD find_process_id(const wchar_t* exeName) {
    if (!exeName) {
        return 0;
    }

    const std::wstring wanted = lower(exeName);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    DWORD result = 0;
    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (lower(entry.szExeFile) == wanted) {
                result = entry.th32ProcessID;
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return result;
}

bool wait_for_process(const wchar_t* exeName, DWORD timeoutMs, DWORD& outPid) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);

    for (;;) {
        const DWORD pid = find_process_id(exeName);
        if (pid) {
            outPid = pid;
            return true;
        }
        if (std::chrono::steady_clock::now() >= deadline) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
}

bool inject_library(DWORD pid, const std::wstring& dllPath, std::string& error) {
    error.clear();

    HANDLE process = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
                                     PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
                                 FALSE, pid);
    if (!process) {
        process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    }
    if (!process) {
        error = "OpenProcess a echoue (code " + std::to_string(GetLastError()) + ")";
        return false;
    }

    const SIZE_T size = (dllPath.size() + 1) * sizeof(wchar_t);
    void* remote = VirtualAllocEx(process, nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote) {
        error = "VirtualAllocEx a echoue (code " + std::to_string(GetLastError()) + ")";
        CloseHandle(process);
        return false;
    }

    if (!WriteProcessMemory(process, remote, dllPath.c_str(), size, nullptr)) {
        error = "WriteProcessMemory a echoue (code " + std::to_string(GetLastError()) + ")";
        VirtualFreeEx(process, remote, 0, MEM_RELEASE);
        CloseHandle(process);
        return false;
    }

    HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
    auto loadLibrary = reinterpret_cast<LPTHREAD_START_ROUTINE>(
        GetProcAddress(kernel32, "LoadLibraryW"));
    if (!loadLibrary) {
        error = "LoadLibraryW introuvable";
        VirtualFreeEx(process, remote, 0, MEM_RELEASE);
        CloseHandle(process);
        return false;
    }

    HANDLE thread = CreateRemoteThread(process, nullptr, 0, loadLibrary, remote, 0, nullptr);
    if (!thread) {
        error = "CreateRemoteThread a echoue (code " + std::to_string(GetLastError()) + ")";
        VirtualFreeEx(process, remote, 0, MEM_RELEASE);
        CloseHandle(process);
        return false;
    }

    WaitForSingleObject(thread, 15000);

    DWORD exitCode = 0;
    GetExitCodeThread(thread, &exitCode);

    CloseHandle(thread);
    VirtualFreeEx(process, remote, 0, MEM_RELEASE);
    CloseHandle(process);

    if (exitCode == 0) {
        error = "LoadLibraryW a renvoye NULL dans le process cible";
        return false;
    }

    return true;
}

} // namespace injector
