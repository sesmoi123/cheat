#include "Core.hpp"

#include "Config.hpp"
#include "Logger.hpp"
#include "Paths.hpp"
#include "Render/Render.hpp"
#include "SDK/Offsets.hpp"
#include "SDK/SDK.hpp"

namespace core {

namespace {

HMODULE g_module = nullptr;
volatile bool g_stop = false;

DWORD WINAPI main_thread(LPVOID) {
    logger::init(paths::to_narrow(paths::data_directory() + L"\\cheat.log"));
    logger::info("cheat injecte (module %p)", g_module);

    config::load();
    offsets::load();
    offsets::write_example();

    sdk::initialize();

    for (int attempt = 0; attempt < 40 && !render::installed(); ++attempt) {
        if (render::install()) {
            break;
        }
        Sleep(250);
    }

    if (render::installed()) {
        logger::info("hook DXGI installe");
    } else {
        logger::error("hook DXGI impossible");
    }

    while (!g_stop) {
        if (!render::installed()) {
            render::install();
        }
        Sleep(250);
    }

    render::uninstall();
    config::save();
    logger::info("cheat decharge");
    return 0;
}

} // namespace

void start(HMODULE module) {
    g_module = module;
    HANDLE thread = CreateThread(nullptr, 0, main_thread, nullptr, 0, nullptr);
    if (thread) {
        CloseHandle(thread);
    }
}

void stop() {
    g_stop = true;
}

HMODULE module() {
    return g_module;
}

std::wstring dll_directory() {
    return paths::module_directory(g_module);
}

} // namespace core
