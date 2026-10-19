#include <windows.h>
#include <shellapi.h>

#include <cstdio>
#include <cwchar>
#include <iostream>
#include <string>

#include "Injector.hpp"
#include "Logger.hpp"
#include "Resource.hpp"

namespace {

constexpr wchar_t kGameProcess[] = L"FortniteClient-Win64-Shipping.exe";
constexpr wchar_t kGameProcessEac[] = L"FortniteClient-Win64-Shipping_EAC.exe";
constexpr wchar_t kDllName[] = L"RetracOGFN.dll";

void attach_console() {
    if (!AllocConsole()) {
        return;
    }

    FILE* dummy = nullptr;
    freopen_s(&dummy, "CONOUT$", "w", stdout);
    freopen_s(&dummy, "CONOUT$", "w", stderr);
    freopen_s(&dummy, "CONIN$", "r", stdin);

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleTitleW(L"Retrac OGFN - Launcher");
}

void enable_colors() {
    const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    if (output == INVALID_HANDLE_VALUE) {
        return;
    }
    DWORD mode = 0;
    if (GetConsoleMode(output, &mode)) {
        SetConsoleMode(output, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
}

void banner() {
    std::cout << "\n";
    std::cout << "  \x1b[95m ____      _            _     \x1b[0m\n";
    std::cout << "  \x1b[95m|  _ \\ ___| |_ _ __ __ _| |___ \x1b[0m   \x1b[90mOGFN\x1b[0m\n";
    std::cout << "  \x1b[95m| |_) / _ \\ __| '__/ _` | / __|\x1b[0m\n";
    std::cout << "  \x1b[95m|  _ <  __/ |_| | | (_| | \\__ \\\x1b[0m\n";
    std::cout << "  \x1b[95m|_| \\_\\___|\\__|_|  \\__,_|_|___/\x1b[0m\n";
    std::cout << "\n  \x1b[90mESP  |  Aimbot  |  Bullet TP\x1b[0m\n\n";
}

void log_line(const char* tag, const char* color, const std::string& text) {
    std::cout << "  \x1b[" << color << "m[" << tag << "]\x1b[0m " << text << "\n";
}

void pause() {
    std::cout << "\n  Appuie sur Entree pour quitter...";
    std::cin.get();
}

std::wstring local_app_data() {
    wchar_t buffer[MAX_PATH]{};
    const DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", buffer, MAX_PATH);
    return length ? std::wstring(buffer, length) : std::wstring(L".");
}

std::string to_narrow(const std::wstring& value) {
    if (value.empty()) {
        return {};
    }
    const int size = WideCharToMultiByte(CP_ACP, 0, value.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string result(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(CP_ACP, 0, value.c_str(), -1, result.data(), size, nullptr, nullptr);
    if (!result.empty() && result.back() == '\0') {
        result.pop_back();
    }
    return result;
}

bool file_exists(const std::wstring& path) {
    return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

// Cherche d'abord une DLL posee a cote de l'exe (mode dev), sinon extrait la
// DLL embarquee dans %LOCALAPPDATA%\RetracOGFN.
std::wstring resolve_dll(const std::wstring& exeDirectory, std::string& error) {
    const std::wstring sideBySide = exeDirectory + L"\\" + kDllName;
    if (file_exists(sideBySide)) {
        return sideBySide;
    }

    const std::wstring base = local_app_data() + L"\\RetracOGFN";
    CreateDirectoryW(base.c_str(), nullptr);

    const std::wstring primary = base + L"\\" + kDllName;
    if (resources::extract_cheat_dll(primary, error)) {
        return primary;
    }

    // La DLL precedente est peut-etre encore chargee par le jeu : nom horodate.
    const std::wstring fallback = base + L"\\RetracOGFN_" + std::to_wstring(GetTickCount64()) + L".dll";
    if (resources::extract_cheat_dll(fallback, error)) {
        return fallback;
    }

    return {};
}

bool is_relaunched() {
    return wcsstr(GetCommandLineW(), L"--elevated") != nullptr;
}

} // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
    attach_console();
    enable_colors();
    banner();

    // Fortnite tourne avec EasyAntiCheat, donc en administrateur. Un process non
    // eleve ne peut pas demander PROCESS_VM_WRITE sur un process eleve :
    // OpenProcess renvoie "acces refuse" (code 5). L'elevation est OBLIGATOIRE.
    if (!injector::is_elevated()) {
        if (!is_relaunched() && injector::relaunch_elevated()) {
            log_line("info", "96", "Elevation demandee (UAC) - relance en administrateur...");
            Sleep(1500);
            return 0;
        }
        log_line("avert", "93", "Pas en administrateur : l'injection va echouer (code 5).");
        log_line("avert", "93", "Relance en clic droit > Executer en tant qu'administrateur.");
    } else {
        log_line("ok", "92", "Droits administrateur actifs");
    }

    const std::wstring exeDirectory = injector::module_directory(nullptr);

    std::string error;
    const std::wstring dllPath = resolve_dll(exeDirectory, error);
    if (dllPath.empty()) {
        log_line("erreur", "91", "Extraction de la DLL impossible : " + error);
        pause();
        return 1;
    }

    logger::init(to_narrow(local_app_data() + L"\\RetracOGFN\\launcher.log"));
    logger::info("launcher started, elevated=%d, dll=%s", injector::is_elevated() ? 1 : 0,
                 to_narrow(dllPath).c_str());

    log_line("ok", "92",
             "Cheat pret (" + std::to_string(resources::embedded_dll_size() / 1024) + " Ko embarques)");
    log_line("info", "96", "Recherche de Fortnite...");

    DWORD pid = injector::find_process_id(kGameProcess);
    if (!pid) {
        pid = injector::find_process_id(kGameProcessEac);
    }

    if (!pid) {
        log_line("info", "93", "Fortnite n'est pas lance. En attente (60 s)...");
        if (!injector::wait_for_process(kGameProcess, 60000, pid)) {
            log_line("erreur", "91", "Fortnite toujours absent. Lance le jeu puis relance ce programme.");
            pause();
            return 1;
        }
    }

    log_line("ok", "92", "Process trouve (PID " + std::to_string(pid) + ")");

    if (!injector::is_elevated() && injector::is_process_elevated(pid)) {
        log_line("avert", "93", "Le jeu tourne en administrateur, ce programme non.");
    }

    log_line("info", "96", "Injection en cours...");

    if (!injector::inject_library(pid, dllPath, error)) {
        log_line("erreur", "91", "Injection echouee : " + error);
        pause();
        return 1;
    }

    log_line("ok", "92", "Injection reussie !");
    log_line("info", "96", "Appuie sur Inser en jeu pour ouvrir le menu.");

    pause();
    return 0;
}
