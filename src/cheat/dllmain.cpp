#include <windows.h>

#include <chrono>
#include <thread>

#include "Cheat.hpp"

namespace {

DWORD WINAPI main_thread(LPVOID parameter) {
    (void)parameter;

    if (!cheat::initialize()) {
        return 0;
    }

    while (cheat::running()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    return 0;
}

} // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);

        HANDLE thread = CreateThread(nullptr, 0, main_thread, module, 0, nullptr);
        if (thread) {
            CloseHandle(thread);
        }
    } else if (reason == DLL_PROCESS_DETACH) {
        if (reserved == nullptr) {
            cheat::shutdown();
        }
    }

    return TRUE;
}
