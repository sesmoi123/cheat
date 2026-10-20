#pragma once

#include <windows.h>

#include <string>
#include <vector>

namespace injector {

struct Target {
    DWORD pid = 0;
    std::wstring name;
};

// Sans ce privilege, EAC refuse OpenProcess sur le process protege du jeu (code 5).
bool enable_debug_privilege();

std::vector<Target> find_fortnite_processes();
bool wait_for_fortnite(DWORD timeoutMs, std::vector<Target>& out);
bool is_process_alive(DWORD pid);

// Vrai si la fenetre au premier plan est celle du jeu (pendant le chargement,
// c'est la splash EAC qui a le focus).
bool game_window_foreground(DWORD pid);

// Injection classique : CreateRemoteThread + LoadLibraryW sur le chemin de la DLL.
bool inject_library(DWORD pid, const std::wstring& dllPath, std::string& error);

} // namespace injector
