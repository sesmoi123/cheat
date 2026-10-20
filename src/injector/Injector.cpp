#include "Injector.hpp"

#include <tlhelp32.h>

#include <algorithm>
#include <cwctype>
#include <string>
#include <vector>

namespace injector {

namespace {

std::wstring lower(std::wstring value) {
    for (wchar_t& character : value) {
        character = static_cast<wchar_t>(::towlower(character));
    }
    return value;
}

bool starts_with(const std::wstring& value, const wchar_t* prefix) {
    const size_t length = wcslen(prefix);
    return value.size() >= length && value.compare(0, length, prefix) == 0;
}

bool is_fortnite_client(const std::wstring& raw) {
    const std::wstring name = lower(raw);
    if (name.size() < 5 || name.compare(name.size() - 4, 4, L".exe") != 0) {
        return false;
    }
    if (name == L"fortnitelauncher.exe") {
        return false;
    }
    if (starts_with(name, L"fortniteclient")) {
        return true; // _EAC, _EAC_EOS, _BE...
    }
    return name == L"fortnite.exe";
}

int preference(const std::wstring& raw) {
    const std::wstring name = lower(raw);
    if (name == L"fortniteclient-win64-shipping.exe") {
        return 0;
    }
    if (starts_with(name, L"fortniteclient-win64-shipping")) {
        return 1;
    }
    if (starts_with(name, L"fortniteclient")) {
        return 2;
    }
    return 3;
}

// Minimum utile d'abord : un process protege EAC refuse PROCESS_ALL_ACCESS.
constexpr DWORD kAccessMasks[] = {
    PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ |
        PROCESS_QUERY_LIMITED_INFORMATION,
    PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
    PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE |
        PROCESS_VM_READ,
    PROCESS_ALL_ACCESS,
};

// EAC protege le process pendant son init : on retente 30 s.
HANDLE open_target(DWORD pid, DWORD& lastError) {
    for (int attempt = 0; attempt < 60; ++attempt) {
        for (const DWORD access : kAccessMasks) {
            HANDLE process = OpenProcess(access, FALSE, pid);
            if (process) {
                return process;
            }
            lastError = GetLastError();
        }
        Sleep(500);
    }
    return nullptr;
}

} // namespace

bool enable_debug_privilege() {
    static bool done = false;
    static bool enabled = false;
    if (done) {
        return enabled;
    }
    done = true;

    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
        return false;
    }

    LUID luid{};
    if (!LookupPrivilegeValueW(nullptr, SE_DEBUG_NAME, &luid)) {
        CloseHandle(token);
        return false;
    }

    TOKEN_PRIVILEGES privileges{};
    privileges.PrivilegeCount = 1;
    privileges.Privileges[0].Luid = luid;
    privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    const BOOL ok =
        AdjustTokenPrivileges(token, FALSE, &privileges, sizeof(privileges), nullptr, nullptr);
    const DWORD error = GetLastError();
    CloseHandle(token);

    enabled = ok && error == ERROR_SUCCESS;
    return enabled;
}

std::vector<Target> find_fortnite_processes() {
    std::vector<Target> found;

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return found;
    }

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);

    if (Process32FirstW(snapshot, &entry)) {
        do {
            const std::wstring name = entry.szExeFile;
            if (is_fortnite_client(name)) {
                Target target;
                target.pid = entry.th32ProcessID;
                target.name = name;
                found.push_back(target);
            }
        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);

    std::sort(found.begin(), found.end(), [](const Target& a, const Target& b) {
        const int left = preference(a.name);
        const int right = preference(b.name);
        if (left != right) {
            return left < right;
        }
        return a.pid < b.pid;
    });

    return found;
}

bool wait_for_fortnite(DWORD timeoutMs, std::vector<Target>& out) {
    const ULONGLONG deadline = GetTickCount64() + timeoutMs;

    for (;;) {
        out = find_fortnite_processes();
        if (!out.empty()) {
            return true;
        }
        if (GetTickCount64() >= deadline) {
            return false;
        }
        Sleep(300);
    }
}

bool is_process_alive(DWORD pid) {
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, pid);
    if (!process) {
        return false;
    }
    const DWORD wait = WaitForSingleObject(process, 0);
    CloseHandle(process);
    return wait == WAIT_TIMEOUT;
}

bool game_window_foreground(DWORD pid) {
    const HWND window = GetForegroundWindow();
    if (!window || !pid) {
        return false;
    }

    DWORD owner = 0;
    GetWindowThreadProcessId(window, &owner);
    return owner == pid;
}

bool inject_library(DWORD pid, const std::wstring& dllPath, std::string& error) {
    error.clear();

    enable_debug_privilege();

    DWORD lastError = 0;
    HANDLE process = open_target(pid, lastError);
    if (!process) {
        error = "OpenProcess a echoue (code " + std::to_string(lastError) + ")";
        return false;
    }

    // Chemin de la DLL ecrit dans le process cible.
    const SIZE_T size = (dllPath.size() + 1) * sizeof(wchar_t);
    void* remote = VirtualAllocEx(process, nullptr, size, MEM_COMMIT | MEM_RESERVE,
                                  PAGE_READWRITE);
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

    auto loadLibrary = reinterpret_cast<LPTHREAD_START_ROUTINE>(
        GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW"));

    HANDLE thread = CreateRemoteThread(process, nullptr, 0, loadLibrary, remote, 0, nullptr);
    if (!thread) {
        error = "CreateRemoteThread a echoue (code " + std::to_string(GetLastError()) + ")";
        VirtualFreeEx(process, remote, 0, MEM_RELEASE);
        CloseHandle(process);
        return false;
    }

    const DWORD wait = WaitForSingleObject(thread, 30000);

    DWORD module = 0;
    if (!GetExitCodeThread(thread, &module)) {
        module = 0;
    }

    CloseHandle(thread);
    VirtualFreeEx(process, remote, 0, MEM_RELEASE);
    CloseHandle(process);

    if (wait != WAIT_OBJECT_0) {
        error = "le thread distant n'a pas repondu (30 s)";
        return false;
    }
    if (!module) {
        error = "LoadLibraryW a renvoye NULL : EAC refuse le chargement";
        return false;
    }

    return true;
}

} // namespace injector
