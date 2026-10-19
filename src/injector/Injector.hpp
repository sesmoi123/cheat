#pragma once

#include <windows.h>

#include <string>

namespace injector {

struct ProcessInfo {
    DWORD pid = 0;
    std::wstring name;
    std::wstring path;
};

DWORD find_process_id(const wchar_t* exeName);

bool wait_for_process(const wchar_t* exeName, DWORD timeoutMs, DWORD& outPid);

bool inject_library(DWORD pid, const std::wstring& dllPath, std::string& error);

std::wstring module_directory(HMODULE module);

// Active SeDebugPrivilege sur le process courant.
bool enable_debug_privilege();

// Le process courant est-il ELEVE ? (jeton UAC reel, PAS juste membre du groupe
// Administrateurs : sous UAC un membre non eleve conserve quand meme ce SID.)
bool is_elevated();

// Le process cible tourne-t-il en administrateur ?
bool is_process_elevated(DWORD pid);

// Relance l'exe avec l'elevation UAC (marqueur --elevated pour eviter les boucles).
bool relaunch_elevated();

} // namespace injector
