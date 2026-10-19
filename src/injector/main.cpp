#include <windows.h>

#include <cstdio>
#include <iostream>
#include <string>

#include "Injector.hpp"
#include "Logger.hpp"

namespace {

constexpr wchar_t kGameProcess[] = L"FortniteClient-Win64-Shipping.exe";
constexpr wchar_t kGameProcessEac[] = L"FortniteClient-Win64-Shipping_EAC.exe";
constexpr wchar_t kDllName[] = L"RetracOGFN.dll";

void enable_colors() {
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
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

} // namespace

int main() {
    SetConsoleTitleW(L"Retrac OGFN - Launcher");
    enable_colors();
    banner();

    const std::wstring directory = injector::module_directory(nullptr);
    const std::wstring dllPath = directory + L"\\" + kDllName;

    if (GetFileAttributesW(dllPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        log_line("erreur", "91", "DLL introuvable : " + std::string(dllPath.begin(), dllPath.end()));
        log_line("info", "93", "Compile le projet puis relance depuis le dossier build/bin.");
        std::cout << "\n  Appuie sur Entree pour quitter...";
        std::cin.get();
        return 1;
    }

    logger::init(std::string(directory.begin(), directory.end()) + "\\launcher.log");
    logger::info("launcher started, dll=%s", std::string(dllPath.begin(), dllPath.end()).c_str());

    log_line("info", "96", "Recherche de Fortnite...");

    DWORD pid = injector::find_process_id(kGameProcess);
    if (!pid) {
        pid = injector::find_process_id(kGameProcessEac);
    }

    if (!pid) {
        log_line("info", "93", "Fortnite n'est pas lance. En attente (60 s)...");
        if (!injector::wait_for_process(kGameProcess, 60000, pid)) {
            log_line("erreur", "91", "Fortnite toujours absent. Lance le jeu puis relance ce programme.");
            std::cout << "\n  Appuie sur Entree pour quitter...";
            std::cin.get();
            return 1;
        }
    }

    log_line("ok", "92", "Process trouve (PID " + std::to_string(pid) + ")");
    log_line("info", "96", "Injection de RetracOGFN.dll...");

    std::string error;
    if (!injector::inject_library(pid, dllPath, error)) {
        log_line("erreur", "91", "Injection echouee : " + error);
        std::cout << "\n  Appuie sur Entree pour quitter...";
        std::cin.get();
        return 1;
    }

    log_line("ok", "92", "Injection reussie !");
    log_line("info", "96", "Appuie sur Inser en jeu pour ouvrir le menu.");

    std::cout << "\n  Launcher pret. Appuie sur Entree pour fermer cette fenetre...";
    std::cin.get();
    return 0;
}
