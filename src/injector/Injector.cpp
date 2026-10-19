#include "Injector.hpp"

#include <shellapi.h>
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

// Lecture du jeton : TokenElevation = etat UAC reel, contrairement a
// CheckTokenMembership qui reste vrai pour un membre non eleve.
bool token_is_elevated(HANDLE process) {
    HANDLE token = nullptr;
    if (!OpenProcessToken(process, TOKEN_QUERY, &token)) {
        return false;
    }

    TOKEN_ELEVATION elevation{};
    DWORD size = sizeof(elevation);
    const BOOL ok = GetTokenInformation(token, TokenElevation, &elevation, size, &size);
    CloseHandle(token);

    return ok != FALSE && elevation.TokenIsElevated != 0;
}

// Fortnite + EasyAntiCheat : on retente l'ouverture du handle pendant un moment,
// avec plusieurs jeux de droits, car l'anti-cheat peut refuser temporairement.
// Ordre du plus permissif au plus tolerant (certains anti-cheats refusent
// PROCESS_QUERY_INFORMATION tout en acceptant PROCESS_QUERY_LIMITED_INFORMATION).
constexpr DWORD kAccessMasks[] = {
    PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
        PROCESS_VM_WRITE | PROCESS_VM_READ,
    PROCESS_CREATE_THREAD | PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_OPERATION |
        PROCESS_VM_WRITE | PROCESS_VM_READ,
    PROCESS_ALL_ACCESS,
};

HANDLE open_target(DWORD pid, DWORD& lastError) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);

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

bool enable_debug_privilege() {
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

    const BOOL adjusted =
        AdjustTokenPrivileges(token, FALSE, &privileges, sizeof(privileges), nullptr, nullptr);
    const DWORD lastError = GetLastError();
    CloseHandle(token);

    return adjusted != FALSE && lastError == ERROR_SUCCESS;
}

bool is_elevated() {
    return token_is_elevated(GetCurrentProcess());
}

bool is_process_elevated(DWORD pid) {
    // PROCESS_QUERY_LIMITED_INFORMATION est accorde meme depuis un process non
    // eleve : on peut donc tester le jeton de la cible sans droits d'ecriture.
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process) {
        return false;
    }
    const bool elevated = token_is_elevated(process);
    CloseHandle(process);
    return elevated;
}

bool relaunch_elevated() {
    wchar_t path[MAX_PATH]{};
    if (!GetModuleFileNameW(nullptr, path, MAX_PATH)) {
        return false;
    }

    SHELLEXECUTEINFOW info{};
    info.cbSize = sizeof(info);
    info.fMask = SEE_MASK_NOCLOSEPROCESS;
    info.lpVerb = L"runas";
    info.lpFile = path;
    info.lpParameters = L"--elevated";
    info.nShow = SW_SHOWNORMAL;

    if (!ShellExecuteExW(&info)) {
        // UAC refuse par l'utilisateur : pas de nouveau process lance.
        return false;
    }

    if (info.hProcess) {
        CloseHandle(info.hProcess);
    }
    return true;
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

    enable_debug_privilege();

    DWORD lastError = 0;
    HANDLE process = open_target(pid, lastError);
    if (!process) {
        if (lastError == ERROR_ACCESS_DENIED) {
            if (!is_elevated()) {
                error = "acces refuse (code 5) : le jeu tourne en administrateur mais pas ce "
                        "programme. Ferme cette fenetre et relance RetracOGFN.exe en "
                        "administrateur (accepte l'UAC).";
            } else if (is_process_elevated(pid)) {
                error = "acces refuse (code 5) malgre l'elevation. Ferme Overwolf / MSI "
                        "Afterburner / RivaTuner et reessaie, puis injecte une fois dans le "
                        "lobby.";
            } else {
                error = "acces refuse (code 5) sur le process cible.";
            }
        } else if (lastError == ERROR_INVALID_PARAMETER) {
            error = "le process a disparu (code 87). Relance Fortnite puis reessaie.";
        } else {
            error = "OpenProcess a echoue (code " + std::to_string(lastError) + ")";
        }
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

    const DWORD wait = WaitForSingleObject(thread, 20000);

    DWORD exitCode = 0;
    GetExitCodeThread(thread, &exitCode);

    CloseHandle(thread);
    VirtualFreeEx(process, remote, 0, MEM_RELEASE);
    CloseHandle(process);

    if (wait != WAIT_OBJECT_0) {
        error = "le thread distant n'a pas repondu dans les temps";
        return false;
    }

    if (exitCode == 0) {
        error = "LoadLibraryW a renvoye NULL dans le process cible";
        return false;
    }

    return true;
}

} // namespace injector
