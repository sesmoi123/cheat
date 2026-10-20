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

// Charge la DLL dans le launcher pour valider le binaire : architecture x64,
// imports presents, DllMain. EAC n'entre pas en jeu dans ce test.
bool probe_local_load(const std::wstring& dllPath, std::string& error);

bool inject_library(DWORD pid, const std::wstring& dllPath, std::string& error);

// Vrai des qu'une fenetre de jeu (classe UnrealWindow, taille de jeu) existe.
// Accepte aussi bien un PID (entier) qu'un HANDLE, sans cast cote appelant.
bool game_window_ready_impl(uintptr_t processOrHandle);

template <typename T = uintptr_t>
bool game_window_ready(T processOrHandle = 0) {
    return game_window_ready_impl(static_cast<uintptr_t>(processOrHandle));
}

} // namespace injector
