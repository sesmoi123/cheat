#include "Injector.hpp"

#include "Elevation.hpp"
#include "Paths.hpp"

#include <tlhelp32.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cwctype>
#include <iterator>
#include <string>
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

bool starts_with(const std::wstring& value, const wchar_t* prefix) {
    const size_t length = wcslen(prefix);
    return value.size() >= length && value.compare(0, length, prefix) == 0;
}

// Toutes les variantes de client Fortnite, sans dependre d'un suffixe precis.
bool is_fortnite_client(const std::wstring& raw) {
    const std::wstring name = lower(raw);
    if (name.size() < 5 || name.compare(name.size() - 4, 4, L".exe") != 0) {
        return false;
    }
    if (name == L"fortnitelauncher.exe") {
        return false;
    }
    if (starts_with(name, L"fortniteclient")) {
        return true; // _EAC, _EAC_EOS, _BE, futurs suffixes...
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

// Droits demandes, du plus complet au plus tolerant.
constexpr DWORD kAccessMasks[] = {
    PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
        PROCESS_VM_WRITE | PROCESS_VM_READ,
    PROCESS_CREATE_THREAD | PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_OPERATION |
        PROCESS_VM_WRITE | PROCESS_VM_READ,
    PROCESS_ALL_ACCESS,
};

// EAC peut refuser le handle quelques secondes : on retente 20 s.
HANDLE open_target(DWORD pid, DWORD& lastError) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);

    for (;;) {
        for (const DWORD access : kAccessMasks) {
            HANDLE process = OpenProcess(access, FALSE, pid);
            if (process) {
                return process;
            }
            lastError = GetLastError();
        }

        if (std::chrono::steady_clock::now() >= deadline) {
            return nullptr;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
    }
}

std::wstring process_directory(DWORD pid) {
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process) {
        return {};
    }

    wchar_t buffer[MAX_PATH * 2]{};
    DWORD size = static_cast<DWORD>(std::size(buffer));
    std::wstring full;
    if (QueryFullProcessImageNameW(process, 0, buffer, &size)) {
        full.assign(buffer, size);
    }
    CloseHandle(process);

    const size_t slash = full.find_last_of(L"\\/");
    return slash == std::wstring::npos ? std::wstring() : full.substr(0, slash);
}

// Lance une fonction kernel32 a un argument wchar_t* dans le process cible.
bool run_remote(HANDLE process, const char* function, const std::wstring& argument) {
    const SIZE_T size = (argument.size() + 1) * sizeof(wchar_t);

    void* remote = VirtualAllocEx(process, nullptr, size, MEM_COMMIT | MEM_RESERVE,
                                  PAGE_READWRITE);
    if (!remote) {
        return false;
    }

    if (!WriteProcessMemory(process, remote, argument.c_str(), size, nullptr)) {
        VirtualFreeEx(process, remote, 0, MEM_RELEASE);
        return false;
    }

    auto routine = reinterpret_cast<LPTHREAD_START_ROUTINE>(
        GetProcAddress(GetModuleHandleW(L"kernel32.dll"), function));
    if (!routine) {
        VirtualFreeEx(process, remote, 0, MEM_RELEASE);
        return false;
    }

    HANDLE thread = CreateRemoteThread(process, nullptr, 0, routine, remote, 0, nullptr);
    if (!thread) {
        VirtualFreeEx(process, remote, 0, MEM_RELEASE);
        return false;
    }

    const DWORD wait = WaitForSingleObject(thread, 10000);
    CloseHandle(thread);
    VirtualFreeEx(process, remote, 0, MEM_RELEASE);
    return wait == WAIT_OBJECT_0;
}

// ---------------------------------------------------------------------------
// Detection de la fenetre de jeu : on enumere les fenetres du process cible au
// lieu de se fier au seul FindWindowW("UnrealWindow").
// ---------------------------------------------------------------------------

struct WindowSearch {
    DWORD pid = 0;
    bool found = false;
};

bool window_is_big_enough(HWND window) {
    RECT client{};
    if (!GetClientRect(window, &client)) {
        return false;
    }
    return client.right >= 320 && client.bottom >= 240;
}

bool window_is_unreal(HWND window) {
    wchar_t name[64]{};
    if (GetClassNameW(window, name, static_cast<int>(std::size(name))) == 0) {
        return false;
    }
    return wcscmp(name, L"UnrealWindow") == 0;
}

BOOL CALLBACK enum_window_callback(HWND window, LPARAM param) {
    WindowSearch* search = reinterpret_cast<WindowSearch*>(param);

    if (!IsWindowVisible(window)) {
        return TRUE;
    }

    DWORD owner = 0;
    GetWindowThreadProcessId(window, &owner);
    if (search->pid != 0 && owner != search->pid) {
        return TRUE;
    }

    if (window_is_unreal(window) || window_is_big_enough(window)) {
        search->found = true;
        return FALSE; // fenetre trouvee, on arrete l'enumeration
    }
    return TRUE;
}

// L'appelant peut passer un PID (petit entier) ou un HANDLE : on teste les deux.
bool any_window_for(DWORD pid) {
    if (pid == 0) {
        return false;
    }

    WindowSearch search;
    search.pid = pid;
    EnumWindows(enum_window_callback, reinterpret_cast<LPARAM>(&search));
    return search.found;
}

} // namespace

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
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);

    for (;;) {
        out = find_fortnite_processes();
        if (!out.empty()) {
            return true;
        }
        if (std::chrono::steady_clock::now() >= deadline) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
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

bool inject_library(DWORD pid, const std::wstring& dllPath, std::string& error) {
    error.clear();

    DWORD lastError = 0;
    HANDLE process = open_target(pid, lastError);
    if (!process) {
        if (lastError == ERROR_ACCESS_DENIED) {
            error = elevation::is_elevated()
                        ? "acces refuse (code 5) : ferme Overwolf / Afterburner / RivaTuner, puis "
                          "injecte dans le lobby."
                        : "acces refuse (code 5) : relance RetracOGFN.exe en administrateur.";
        } else if (lastError == ERROR_INVALID_PARAMETER) {
            error = "le process a disparu (code 87). Relance Fortnite puis reessaie.";
        } else {
            error = "OpenProcess a echoue (code " + std::to_string(lastError) + ")";
        }
        return false;
    }

    // On copie la DLL DANS le dossier du jeu : ses dependances (vcruntime, msvcp,
    // d3d, dxgi...) se resolvent alors exactement comme pour le jeu lui-meme.
    std::wstring directory = process_directory(pid);
    std::wstring target = dllPath;

    if (!directory.empty()) {
        const std::wstring copy = directory + L"\\RetracOGFN_inj.dll";
        if (CopyFileW(dllPath.c_str(), copy.c_str(), FALSE)) {
            target = copy;
        }
    }

    // Force le dossier de la DLL comme repertoire de recherche dans la cible.
    const size_t slash = target.find_last_of(L"\\/");
    if (slash != std::wstring::npos) {
        run_remote(process, "SetDllDirectoryW", target.substr(0, slash));
    }

    const SIZE_T size = (target.size() + 1) * sizeof(wchar_t);
    void* remote = VirtualAllocEx(process, nullptr, size, MEM_COMMIT | MEM_RESERVE,
                                  PAGE_READWRITE);
    if (!remote) {
        error = "VirtualAllocEx a echoue (code " + std::to_string(GetLastError()) + ")";
        CloseHandle(process);
        return false;
    }

    if (!WriteProcessMemory(process, remote, target.c_str(), size, nullptr)) {
        error = "WriteProcessMemory a echoue (code " + std::to_string(GetLastError()) + ")";
        VirtualFreeEx(process, remote, 0, MEM_RELEASE);
        CloseHandle(process);
        return false;
    }

    HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
    auto loadLibrary =
        reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(kernel32, "LoadLibraryW"));
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

    const DWORD wait = WaitForSingleObject(thread, 30000);

    DWORD exitCode = 0;
    if (!GetExitCodeThread(thread, &exitCode)) {
        exitCode = 0;
    }

    CloseHandle(thread);
    VirtualFreeEx(process, remote, 0, MEM_RELEASE);
    CloseHandle(process);

    if (wait != WAIT_OBJECT_0) {
        error = "le thread distant n'a pas repondu dans les temps";
        return false;
    }
    if (exitCode == 0) {
        error = "LoadLibraryW a renvoye NULL : dependance manquante dans " +
                paths::to_narrow(directory.empty() ? target : directory);
        return false;
    }

    return true;
}

bool game_window_ready_impl(uintptr_t processOrHandle) {
    // Interpretation 1 : c'est un HANDLE -> on en extrait le PID.
    if (processOrHandle != 0) {
        const DWORD fromHandle = GetProcessId(reinterpret_cast<HANDLE>(processOrHandle));
        if (fromHandle != 0 && any_window_for(fromHandle)) {
            return true;
        }

        // Interpretation 2 : c'est deja un PID.
        if (any_window_for(static_cast<DWORD>(processOrHandle))) {
            return true;
        }
    }

    // Repli (PID inconnu) : la classe de fenetre d'Unreal.
    return FindWindowW(L"UnrealWindow", nullptr) != nullptr;
}

} // namespace injector
