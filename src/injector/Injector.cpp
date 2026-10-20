#include "Injector.hpp"

#include "Elevation.hpp"
#include "Paths.hpp"

#include <tlhelp32.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
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

// ---------------------------------------------------------------------------
// Stub distant : LoadLibraryExW(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH) puis
// GetLastError(). Le dossier de la DLL est ainsi cherche en premier pour ses
// dependances, et on recupere le code d'erreur exact du thread distant.
// ---------------------------------------------------------------------------

constexpr size_t kStubPathOffset = 0x100;
constexpr size_t kStubResultOffset = 0x300;
constexpr size_t kStubErrorOffset = 0x308;
constexpr size_t kStubSize = 0x400;
constexpr DWORD kCrashExitCode = 0xC0000005;

struct StubResult {
    HMODULE module = nullptr;
    DWORD error = 0;
    DWORD exit = 0;
};

void push_u8(std::vector<uint8_t>& code, uint8_t value) {
    code.push_back(value);
}

void push_u64(std::vector<uint8_t>& code, uint64_t value) {
    for (int shift = 0; shift < 64; shift += 8) {
        code.push_back(static_cast<uint8_t>((value >> shift) & 0xFF));
    }
}

std::vector<uint8_t> build_stub(uintptr_t base) {
    const HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
    const uintptr_t loadLibraryEx =
        reinterpret_cast<uintptr_t>(GetProcAddress(kernel32, "LoadLibraryExW"));
    const uintptr_t getLastError =
        reinterpret_cast<uintptr_t>(GetProcAddress(kernel32, "GetLastError"));

    std::vector<uint8_t> code;
    code.reserve(96);

    // sub rsp, 0x28 (espace fantome + alignement 16)
    push_u8(code, 0x48); push_u8(code, 0x83); push_u8(code, 0xEC); push_u8(code, 0x28);

    push_u8(code, 0x48); push_u8(code, 0xB9); push_u64(code, base + kStubPathOffset);
    push_u8(code, 0x31); push_u8(code, 0xD2);                                  // xor edx, edx
    push_u8(code, 0x41); push_u8(code, 0xB8);
    push_u8(code, 0x08); push_u8(code, 0x00); push_u8(code, 0x00); push_u8(code, 0x00); // r8d = 8
    push_u8(code, 0x48); push_u8(code, 0xB8); push_u64(code, loadLibraryEx);
    push_u8(code, 0xFF); push_u8(code, 0xD0); // call LoadLibraryExW

    push_u8(code, 0x48); push_u8(code, 0xB9); push_u64(code, base + kStubResultOffset);
    push_u8(code, 0x48); push_u8(code, 0x89); push_u8(code, 0x01); // [rcx] = rax

    push_u8(code, 0x48); push_u8(code, 0xB8); push_u64(code, getLastError);
    push_u8(code, 0xFF); push_u8(code, 0xD0); // call GetLastError

    push_u8(code, 0x48); push_u8(code, 0xB9); push_u64(code, base + kStubErrorOffset);
    push_u8(code, 0x89); push_u8(code, 0x01); // [rcx] = eax

    push_u8(code, 0x48); push_u8(code, 0x83); push_u8(code, 0xC4); push_u8(code, 0x28);
    push_u8(code, 0x31); push_u8(code, 0xC0); // xor eax, eax
    push_u8(code, 0xC3);                      // ret
    return code;
}

bool run_stub(HANDLE process, const std::wstring& dllPath, StubResult& out, std::string& error) {
    void* remote = VirtualAllocEx(process, nullptr, kStubSize, MEM_COMMIT | MEM_RESERVE,
                                  PAGE_EXECUTE_READWRITE);
    if (!remote) {
        error = "VirtualAllocEx a echoue (code " + std::to_string(GetLastError()) + ")";
        return false;
    }

    const uintptr_t base = reinterpret_cast<uintptr_t>(remote);
    const std::vector<uint8_t> code = build_stub(base);
    const SIZE_T pathSize = (dllPath.size() + 1) * sizeof(wchar_t);

    if (!WriteProcessMemory(process, remote, code.data(), code.size(), nullptr) ||
        !WriteProcessMemory(process, reinterpret_cast<void*>(base + kStubPathOffset),
                            dllPath.c_str(), pathSize, nullptr)) {
        error = "WriteProcessMemory a echoue (code " + std::to_string(GetLastError()) + ")";
        VirtualFreeEx(process, remote, 0, MEM_RELEASE);
        return false;
    }

    HANDLE thread = CreateRemoteThread(process, nullptr, 0,
                                       reinterpret_cast<LPTHREAD_START_ROUTINE>(remote), nullptr, 0,
                                       nullptr);
    if (!thread) {
        error = "CreateRemoteThread a echoue (code " + std::to_string(GetLastError()) + ")";
        VirtualFreeEx(process, remote, 0, MEM_RELEASE);
        return false;
    }

    const DWORD wait = WaitForSingleObject(thread, 30000);
    DWORD exit = 0;
    if (!GetExitCodeThread(thread, &exit)) {
        exit = 0;
    }
    CloseHandle(thread);

    if (wait != WAIT_OBJECT_0) {
        error = "le thread distant n'a pas repondu dans les temps";
        VirtualFreeEx(process, remote, 0, MEM_RELEASE);
        return false;
    }

    ReadProcessMemory(process, reinterpret_cast<void*>(base + kStubResultOffset), &out.module,
                      sizeof(out.module), nullptr);
    ReadProcessMemory(process, reinterpret_cast<void*>(base + kStubErrorOffset), &out.error,
                      sizeof(out.error), nullptr);
    out.exit = exit;

    VirtualFreeEx(process, remote, 0, MEM_RELEASE);
    return true;
}

std::string describe_load_error(DWORD code, const std::wstring& where) {
    const std::string path = paths::to_narrow(where);

    switch (code) {
        case 0:
            return "EAC n'a pas fini de charger le jeu (aucun code renvoye) : attendre la "
                   "disparition de la splash EAC (" + path + ")";
        case ERROR_MOD_NOT_FOUND:
            return "dependance introuvable (code 126) : copie vcruntime140.dll, "
                   "vcruntime140_1.dll, msvcp140.dll et d3dcompiler_47.dll a cote de la DLL (" +
                   path + ")";
        case ERROR_PROC_NOT_FOUND:
            return "point d'entree introuvable (code 127) (" + path + ")";
        case ERROR_BAD_EXE_FORMAT:
            return "mauvaise architecture (code 193) : la DLL n'est pas en x64 (" + path + ")";
        case ERROR_DLL_INIT_FAILED:
            return "DllMain a echoue (code 1114) (" + path + ")";
        case ERROR_ACCESS_DENIED:
            return "acces refuse (code 5) : lance le launcher en administrateur (" + path + ")";
        default:
            return "LoadLibraryExW a echoue (code " + std::to_string(code) + ") (" + path + ")";
    }
}

// ---------------------------------------------------------------------------
// Lecture du PE : liste les DLL importees absentes de la machine.
// ---------------------------------------------------------------------------

size_t rva_to_offset(const std::vector<uint8_t>& image, DWORD rva) {
    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(image.data());
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(image.data() + dos->e_lfanew);
    const IMAGE_SECTION_HEADER* section = IMAGE_FIRST_SECTION(nt);

    for (WORD index = 0; index < nt->FileHeader.NumberOfSections; ++index, ++section) {
        const DWORD start = section->VirtualAddress;
        const DWORD end = start + std::max(section->Misc.VirtualSize, section->SizeOfRawData);
        if (rva >= start && rva < end) {
            return section->PointerToRawData + (rva - start);
        }
    }
    return 0;
}

std::vector<std::wstring> missing_imports(const std::wstring& dllPath, std::string& fatal) {
    std::vector<std::wstring> missing;
    fatal.clear();

    HANDLE file = CreateFileW(dllPath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        fatal = "DLL illisible";
        return missing;
    }

    LARGE_INTEGER size{};
    GetFileSizeEx(file, &size);
    std::vector<uint8_t> image(static_cast<size_t>(size.QuadPart));
    DWORD read = 0;
    const BOOL ok = ReadFile(file, image.data(), static_cast<DWORD>(image.size()), &read, nullptr);
    CloseHandle(file);

    if (!ok || !read || image.size() < sizeof(IMAGE_NT_HEADERS64)) {
        fatal = "lecture de la DLL impossible";
        return missing;
    }

    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(image.data());
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        fatal = "ce n'est pas un PE valide";
        return missing;
    }

    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(image.data() + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        fatal = "en-tete PE illisible";
        return missing;
    }
    if (nt->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64) {
        fatal = "la DLL n'est pas compilee en x64";
        return missing;
    }

    const IMAGE_DATA_DIRECTORY& directory =
        nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!directory.VirtualAddress) {
        return missing;
    }

    const size_t start = rva_to_offset(image, directory.VirtualAddress);
    if (!start) {
        return missing;
    }

    for (size_t index = 0;; ++index) {
        const size_t at = start + index * sizeof(IMAGE_IMPORT_DESCRIPTOR);
        if (at + sizeof(IMAGE_IMPORT_DESCRIPTOR) > image.size()) {
            break;
        }

        IMAGE_IMPORT_DESCRIPTOR descriptor{};
        memcpy(&descriptor, image.data() + at, sizeof(descriptor));
        if (!descriptor.Name) {
            break;
        }

        const size_t nameOffset = rva_to_offset(image, descriptor.Name);
        if (!nameOffset || nameOffset >= image.size()) {
            continue;
        }

        std::wstring name;
        for (size_t cursor = nameOffset; cursor < image.size() && image[cursor] && name.size() < 128;
             ++cursor) {
            name.push_back(static_cast<wchar_t>(image[cursor]));
        }
        if (name.empty()) {
            continue;
        }

        if (!GetModuleHandleW(name.c_str()) && !LoadLibraryW(name.c_str())) {
            missing.push_back(name);
        }
    }

    return missing;
}

// ---------------------------------------------------------------------------
// Fenetre de jeu : classe UnrealWindow et taille de jeu.
// ---------------------------------------------------------------------------

struct WindowSearch {
    DWORD pid = 0;
    bool found = false;
};

bool window_is_unreal(HWND window) {
    wchar_t name[64]{};
    if (GetClassNameW(window, name, static_cast<int>(std::size(name))) == 0) {
        return false;
    }
    return wcscmp(name, L"UnrealWindow") == 0;
}

bool window_is_big_enough(HWND window) {
    RECT client{};
    if (!GetClientRect(window, &client)) {
        return false;
    }
    return client.right >= 320 && client.bottom >= 240;
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

    if (!window_is_unreal(window) || !window_is_big_enough(window)) {
        return TRUE;
    }

    search->found = true;
    return FALSE; // fenetre trouvee, on arrete l'enumeration
}

bool any_unreal_window(DWORD pid) {
    if (!pid) {
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

bool eac_splash_present() {
    static const wchar_t* const kClasses[] = {L"EAC splash", L"EACsplash",
                                              L"EasyAntiCheat splash"};
    for (const wchar_t* name : kClasses) {
        if (FindWindowW(name, nullptr)) {
            return true;
        }
    }

    static const wchar_t* const kTitles[] = {L"Easy Anti-Cheat", L"EasyAntiCheat"};
    for (const wchar_t* title : kTitles) {
        if (FindWindowW(nullptr, title)) {
            return true;
        }
    }
    return false;
}

bool probe_local_load(const std::wstring& dllPath, std::string& error) {
    error.clear();

    std::string fatal;
    const std::vector<std::wstring> missing = missing_imports(dllPath, fatal);
    if (!fatal.empty()) {
        error = fatal;
        return false;
    }
    if (!missing.empty()) {
        error = "import(s) absent(s) sur la machine :";
        for (const std::wstring& name : missing) {
            error += " " + paths::to_narrow(name);
        }
        return false;
    }

    SetLastError(0);
    HMODULE module = LoadLibraryExW(dllPath.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (module) {
        return true;
    }

    error = "chargement local impossible (code " + std::to_string(GetLastError()) + ")";
    return false;
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

    StubResult stub;
    std::string stubError;
    const bool ran = run_stub(process, dllPath, stub, stubError);
    CloseHandle(process);

    if (!ran) {
        error = stubError;
        return false;
    }
    if (stub.exit == kCrashExitCode) {
        error = "crash du thread distant (0xC0000005) : DllMain a revoir";
        return false;
    }
    if (stub.module) {
        return true;
    }

    error = describe_load_error(stub.error, dllPath);
    return false;
}

bool game_window_ready_impl(uintptr_t processOrHandle) {
    if (processOrHandle != 0) {
        // Interpretation 1 : c'est un HANDLE -> on en extrait le PID.
        const DWORD fromHandle = GetProcessId(reinterpret_cast<HANDLE>(processOrHandle));
        if (any_unreal_window(fromHandle)) {
            return true;
        }

        // Interpretation 2 : c'est deja un PID.
        if (any_unreal_window(static_cast<DWORD>(processOrHandle))) {
            return true;
        }
    }

    // Repli (PID inconnu) : la classe de fenetre d'Unreal, tous process confondus.
    return FindWindowW(L"UnrealWindow", nullptr) != nullptr;
}

} // namespace injector
