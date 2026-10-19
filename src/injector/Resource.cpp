#include "Resource.hpp"

#include <string>

namespace resources {

namespace {

// MAKEINTRESOURCEW explicitement : ne depend pas de la definition de UNICODE.
constexpr WORD kResourceId = 10; // RT_RCDATA

HRSRC find_resource() {
    return FindResourceW(nullptr, L"RETRAC_CHEAT_DLL", MAKEINTRESOURCEW(kResourceId));
}

} // namespace

size_t embedded_dll_size() {
    const HRSRC resource = find_resource();
    return resource ? SizeofResource(nullptr, resource) : 0;
}

bool extract_cheat_dll(const std::wstring& destination, std::string& error) {
    const HRSRC resource = find_resource();
    if (!resource) {
        error = "ressource RETRAC_CHEAT_DLL absente";
        return false;
    }

    const HGLOBAL loaded = LoadResource(nullptr, resource);
    if (!loaded) {
        error = "LoadResource a echoue";
        return false;
    }

    const void* data = LockResource(loaded);
    const DWORD size = SizeofResource(nullptr, resource);
    if (!data || !size) {
        error = "ressource vide";
        return false;
    }

    HANDLE file = CreateFileW(destination.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        error = "CreateFileW a echoue (code " + std::to_string(GetLastError()) + ")";
        return false;
    }

    DWORD written = 0;
    const BOOL ok = WriteFile(file, data, size, &written, nullptr);
    CloseHandle(file);

    if (!ok || written != size) {
        error = "ecriture incomplete de la DLL";
        return false;
    }

    return true;
}

} // namespace resources
