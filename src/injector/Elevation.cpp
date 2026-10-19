#include "Elevation.hpp"

#include <shellapi.h>

#include <string>

namespace elevation {

namespace {

// TokenElevation = verite UAC. CheckTokenMembership, lui, reste vrai pour un
// membre du groupe Administrateurs meme quand le process n'est PAS eleve.
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

} // namespace

bool is_elevated() {
    return token_is_elevated(GetCurrentProcess());
}

bool is_process_elevated(DWORD pid) {
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process) {
        return false;
    }
    const bool elevated = token_is_elevated(process);
    CloseHandle(process);
    return elevated;
}

bool relaunch_elevated(const wchar_t* extraArgs) {
    wchar_t path[MAX_PATH]{};
    if (!GetModuleFileNameW(nullptr, path, MAX_PATH)) {
        return false;
    }

    std::wstring parameters = L"--elevated";
    if (extraArgs && *extraArgs) {
        parameters += L" ";
        parameters += extraArgs;
    }

    SHELLEXECUTEINFOW info{};
    info.cbSize = sizeof(info);
    info.fMask = SEE_MASK_NOCLOSEPROCESS;
    info.lpVerb = L"runas";
    info.lpFile = path;
    info.lpParameters = parameters.c_str();
    info.nShow = SW_SHOWNORMAL;

    if (!ShellExecuteExW(&info)) {
        return false;
    }

    if (info.hProcess) {
        CloseHandle(info.hProcess);
    }
    return true;
}

} // namespace elevation
