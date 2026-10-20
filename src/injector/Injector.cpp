// A ajouter dans le fichier (il inclut deja <windows.h>)

namespace injector {

namespace {

struct WindowSearch {
    DWORD pid = 0;
    bool found = false;
};

BOOL CALLBACK enum_window_callback(HWND window, LPARAM param) {
    WindowSearch* search = reinterpret_cast<WindowSearch*>(param);

    DWORD owner = 0;
    GetWindowThreadProcessId(window, &owner);
    if (owner != search->pid || !IsWindowVisible(window)) {
        return TRUE;
    }

    RECT client{};
    if (GetClientRect(window, &client) && client.right > 0 && client.bottom > 0) {
        search->found = true;
        return FALSE; // fenetre trouvee, on arrete l'enumeration
    }
    return TRUE;
}

} // namespace

bool game_window_ready_impl(uintptr_t processOrHandle) {
    DWORD pid = static_cast<DWORD>(processOrHandle);

    // Si l'appelant a passe un HANDLE plutot qu'un PID, on recupere le vrai PID.
    const DWORD fromHandle = GetProcessId(reinterpret_cast<HANDLE>(processOrHandle));
    if (fromHandle != 0) {
        pid = fromHandle;
    }

    WindowSearch search;
    search.pid = pid;
    EnumWindows(enum_window_callback, reinterpret_cast<LPARAM>(&search));
    return search.found;
}

} // namespace injector
