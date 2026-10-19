#include <windows.h>

#include <cstdio>
#include <cwchar>
#include <iostream>
#include <string>
#include <vector>

#include "Elevation.hpp"
#include "Injector.hpp"
#include "Logger.hpp"
#include "Paths.hpp"
#include "Resource.hpp"

namespace {

constexpr wchar_t kDllName[] = L"RetracOGFN.dll";

void enable_dpi_awareness() {
    if (HMODULE user32 = GetModuleHandleW(L"user32.dll")) {
        using SetContextFn = BOOL(WINAPI*)(HANDLE);
        auto setContext =
            reinterpret_cast<SetContextFn>(GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
        const HANDLE perMonitorV2 = reinterpret_cast<HANDLE>(static_cast<INT_PTR>(-4));
        if (setContext && setContext(perMonitorV2)) {
            return;
        }
    }
    SetProcessDPIAware();
}

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
    std::cout << "\n  \x1b[90mESP  |  Aimbot  |  Bullet TP  |  Manette\x1b[0m\n\n";
}

void log_line(const char* tag, const char* color, const std::string& text) {
    std::cout << "  \x1b[" << color << "m[" << tag << "]\x1b[0m " << text << "\n";
}

void pause() {
    std::cout << "\n  Appuie sur Entree pour quitter...";
    std::cin.get();
}

// Cherche une DLL a cote de l'exe (mode dev), sinon extrait la DLL embarquee.
std::wstring resolve_dll(const std::wstring& exeDirectory, std::string& error) {
    const std::wstring sideBySide = exeDirectory + L"\\" + kDllName;
    if (paths::file_exists(sideBySide)) {
        return sideBySide;
    }

    const std::wstring base = paths::data_directory();

    const std::wstring primary = base + L"\\" + kDllName;
    if (resources::extract_cheat_dll(primary, error)) {
        return primary;
    }

    // DLL precedente encore verrouillee par le jeu : nom horodate.
    const std::wstring fallback =
        base + L"\\RetracOGFN_" + std::to_wstring(GetTickCount64()) + L".dll";
    if (resources::extract_cheat_dll(fallback, error)) {
        return fallback;
    }

    return {};
}

bool is_relaunched() {
    return wcsstr(GetCommandLineW(), L"--elevated") != nullptr;
}

bool auto_elevate() {
    if (elevation::is_elevated()) {
        log_line("ok", "92", "Administrateur : OK");
        return false;
    }

    if (!is_relaunched()) {
        log_line("info", "96", "Demande des droits administrateur (UAC)...");
        if (elevation::relaunch_elevated(nullptr)) {
            Sleep(1200);
            return true;
        }
        log_line("avert", "93", "UAC refusee : l'injection va probablement echouer (code 5).");
        return false;
    }

    log_line("avert", "93", "Toujours pas administrateur apres elevation.");
    return false;
}

} // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
    enable_dpi_awareness();
    attach_console();
    enable_colors();
    banner();

    if (auto_elevate()) {
        return 0;
    }

    const std::wstring exeDirectory = paths::module_directory(nullptr);

    std::string error;
    const std::wstring dllPath = resolve_dll(exeDirectory, error);
    if (dllPath.empty()) {
        log_line("erreur", "91", "Extraction de la DLL impossible : " + error);
        pause();
        return 1;
    }

    logger::init(paths::to_narrow(paths::data_directory() + L"\\launcher.log"));
    logger::info("launcher: elevated=%d dll=%s", elevation::is_elevated() ? 1 : 0,
                 paths::to_narrow(dllPath).c_str());

    log_line("ok", "92", "Cheat pret (" + std::to_string(resources::embedded_dll_size() / 1024) +
                             " Ko embarques)");
    log_line("info", "96", "Recherche de Fortnite...");

    std::vector<injector::Target> targets = injector::find_fortnite_processes();
    if (targets.empty()) {
        log_line("info", "93", "Fortnite n'est pas lance. En attente (120 s)...");
        if (!injector::wait_for_fortnite(120000, targets)) {
            log_line("erreur", "91", "Fortnite absent. Lance le jeu puis relance ce programme.");
            pause();
            return 1;
        }
    }

    // Le process apparait au splash EAC, bien avant la fenetre de jeu.
    log_line("info", "96", "Attente de la fenetre de jeu (splash EAC en cours)...");
    for (int second = 0; second < 180 && !injector::game_window_ready(); ++second) {
        Sleep(1000);
    }
    if (!injector::game_window_ready()) {
        log_line("avert", "93", "Fenetre de jeu absente : on injecte quand meme.");
    } else {
        log_line("ok", "92", "Fenetre de jeu detectee.");
    }

    bool injected = false;
    for (int attempt = 1; attempt <= 5 && !injected; ++attempt) {
        if (targets.empty()) {
            injector::wait_for_fortnite(15000, targets);
        }

        for (const injector::Target& target : targets) {
            if (!injector::is_process_alive(target.pid)) {
                continue;
            }

            log_line("info", "96", "Injection dans " + paths::to_narrow(target.name) + " (PID " +
                                       std::to_string(target.pid) + ")...");

            if (injector::inject_library(target.pid, dllPath, error)) {
                injected = true;
                log_line("ok", "92", "Injection reussie !");
                log_line("info", "96", "Inser ou LB+RB en jeu pour ouvrir le menu.");
                break;
            }

            log_line("avert", "93", error);
        }

        if (!injected) {
            targets.clear();
            if (attempt < 5) {
                log_line("info", "93",
                         "Nouvelle tentative dans 4 s (" + std::to_string(attempt) + "/5)...");
                Sleep(4000);
            }
        }
    }

    if (!injected) {
        log_line("erreur", "91", "Injection echouee. DirectX 11, dans le lobby, en admin.");
    }

    pause();
    return injected ? 0 : 1;
}
