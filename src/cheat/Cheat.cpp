#include "Cheat.hpp"

#include <atomic>
#include <string>
#include <vector>

#include <windows.h>

#include <imgui.h>

#include "Config.hpp"
#include "Features/Aimbot.hpp"
#include "Features/BulletTP.hpp"
#include "Features/ESP.hpp"
#include "Logger.hpp"
#include "Menu/Menu.hpp"
#include "Render/Hook.hpp"
#include "SDK/Offsets.hpp"
#include "SDK/UE.hpp"

namespace cheat {

namespace {

std::atomic<bool> g_running{false};

std::string module_directory() {
    char path[MAX_PATH]{};
    HMODULE module = nullptr;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCSTR>(&module_directory), &module);
    if (!module) {
        return ".";
    }
    const DWORD length = GetModuleFileNameA(module, path, MAX_PATH);
    if (!length) {
        return ".";
    }
    std::string value(path, length);
    const size_t position = value.find_last_of("\\/");
    return position == std::string::npos ? std::string(".") : value.substr(0, position);
}

} // namespace

bool initialize() {
    const std::string directory = module_directory();

    logger::init(directory + "\\RetracOGFN.log");
    logger::info("Retrac OGFN charge (build %s %s)", __DATE__, __TIME__);

    off::write_defaults(directory + "\\offsets.json");
    off::load_from_file(directory + "\\offsets.json");
    config::load();

    if (!hook::install()) {
        logger::error("installation du hook de rendu impossible");
        return false;
    }

    g_running = true;
    logger::info("hook installe, en attente du rendu du jeu");
    return true;
}

void shutdown() {
    g_running = false;

    config::save();
    hook::uninstall();

    logger::info("Retrac OGFN decharge");
    logger::shutdown();
}

bool running() {
    return g_running;
}

void tick() {
    if (!ImGui::GetCurrentContext()) {
        return;
    }

    const sdk::Camera camera = sdk::get_camera();
    const std::vector<sdk::Player> players = sdk::gather_players(camera);

    esp::draw(camera, players);
    aimbot::update(camera, players);
    aimbot::draw_fov(camera);
    bullet_tp::update(camera, players);

    menu::render();
    menu::draw_watermark();
}

} // namespace cheat
