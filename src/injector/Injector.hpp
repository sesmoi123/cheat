#pragma once

#include <windows.h>

#include <cstdint>
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

// Vrai tant que la splash EAC est a l'ecran : EAC n'a pas fini de charger le jeu
// et refuse tout LoadLibrary. C'est une fenetre du process EAC, pas du jeu.
bool eac_splash_present();

// Charge la DLL dans le launcher pour valider le binaire : x64, imports, DllMain.
bool probe_local_load(const std::wstring& dllPath, std::string& error);

// Injecte la DLL telle quelle, sans rien copier dans le dossier du jeu.
bool inject_library(DWORD pid, const std::wstring& dllPath, std::string& error);

// Vrai des qu'une fenetre de jeu (classe UnrealWindow, taille de jeu) existe.
bool game_window_ready_impl(uintptr_t processOrHandle);

template <typename T = uintptr_t>
bool game_window_ready(T processOrHandle = 0) {
    return game_window_ready_impl(static_cast<uintptr_t>(processOrHandle));
}

} // namespace injector
